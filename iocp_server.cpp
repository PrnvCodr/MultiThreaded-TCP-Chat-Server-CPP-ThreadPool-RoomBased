#include "iocp_server.h"
#include <iostream>
#include <algorithm>

IOCPServer::IOCPServer(int port, ThreadPool& pool)
    : completion_port(INVALID_HANDLE_VALUE)
    , listen_socket(INVALID_SOCKET)
    , thread_pool(pool)
    , port_(port)
{
}

IOCPServer::~IOCPServer() {
    Stop();
}

bool IOCPServer::Start() {
    // Create listen socket
    listen_socket = CreateListenSocket(port_);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "[IOCP] Failed to create listen socket" << std::endl;
        return false;
    }
    
    // Create I/O Completion Port
    completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (completion_port == NULL) {
        std::cerr << "[IOCP] CreateIoCompletionPort failed: " << GetLastError() << std::endl;
        closesocket(listen_socket);
        return false;
    }
    
    // Associate listen socket with IOCP (for accept notifications)
    if (CreateIoCompletionPort((HANDLE)listen_socket, completion_port, 0, 0) == NULL) {
        std::cerr << "[IOCP] Failed to associate listen socket: " << GetLastError() << std::endl;
        CloseHandle(completion_port);
        closesocket(listen_socket);
        return false;
    }
    
    running.store(true);
    
    // Start IOCP worker threads (one per CPU core)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    int num_workers = sys_info.dwNumberOfProcessors;
    if (num_workers == 0) num_workers = 1;
    
    std::cout << "[IOCP] Starting " << num_workers << " I/O worker threads" << std::endl;
    
    for (int i = 0; i < num_workers; ++i) {
        io_workers.push_back(w32::Thread([this] { IOCPWorkerThread(); }));
    }
    
    // Start accept thread
    // Note: Thread wrapper expects lambda or functor
    // We create a detached thread implicitly by not storing it, or we should manage it.
    // The previous code detached it. w32::Thread joins on destruction unless handled.
    // Let's create a thread that runs AcceptConnections, but we need to keep it alive or detach it.
    // w32::Thread has detach().
    w32::Thread accept_thread(&IOCPServer::AcceptConnections, this);
    accept_thread.detach();
    
    std::cout << "[IOCP] Server started on port " << port_ << std::endl;
    return true;
}

void IOCPServer::Stop() {
    if (!running.load()) {
        return;
    }
    
    running.store(false);
    
    // Close listen socket to stop accepting
    if (listen_socket != INVALID_SOCKET) {
        closesocket(listen_socket);
        listen_socket = INVALID_SOCKET;
    }
    
    // Post completion packets to wake up worker threads
    for (size_t i = 0; i < io_workers.size(); ++i) {
        PostQueuedCompletionStatus(completion_port, 0, 0, NULL);
    }
    
    // Wait for workers to finish
    // w32::Thread destructor joins if joinable, so clearing vector joins them.
    for (auto& worker : io_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    io_workers.clear();
    
    // Close all client connections
    {
        w32::LockGuard lock(clients_mutex);
        for (auto& pair : clients) {
            if (pair.second.socket != INVALID_SOCKET) {
                closesocket(pair.second.socket);
            }
        }
        clients.clear();
        socket_to_id.clear();
    }
    
    // Close IOCP handle
    if (completion_port != INVALID_HANDLE_VALUE) {
        CloseHandle(completion_port);
        completion_port = INVALID_HANDLE_VALUE;
    }
    
    std::cout << "[IOCP] Server stopped" << std::endl;
}

void IOCPServer::IOCPWorkerThread() {
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    LPOVERLAPPED overlapped;
    
    while (running.load()) {
        BOOL result = GetQueuedCompletionStatus(
            completion_port,
            &bytes_transferred,
            &completion_key,
            &overlapped,
            1000  // 1 second timeout
        );
        
        if (!running.load()) {
            break;
        }
        
        if (!result) {
            DWORD error = GetLastError();
            if (error == WAIT_TIMEOUT) {
                continue;
            }
            if (overlapped != NULL) {
                // I/O operation failed
                PER_IO_DATA* io_data = CONTAINING_RECORD(overlapped, PER_IO_DATA, overlapped);
                std::cerr << "[IOCP] I/O error for client " << io_data->client_id 
                          << ": " << error << std::endl;
                CleanupClient(io_data->client_id);
                delete io_data;
            }
            continue;
        }
        
        if (overlapped == NULL) {
            // Shutdown signal
            break;
        }
        
        PER_IO_DATA* io_data = CONTAINING_RECORD(overlapped, PER_IO_DATA, overlapped);
        
        if (bytes_transferred == 0 && io_data->operation != IOOperation::ACCEPT) {
            // Client disconnected gracefully
            std::cout << "[IOCP] Client " << io_data->client_id << " disconnected" << std::endl;
            CleanupClient(io_data->client_id);
            delete io_data;
            continue;
        }
        
        // Handle based on operation type
        switch (io_data->operation) {
            case IOOperation::READ:
                HandleRead(io_data, bytes_transferred);
                break;
            case IOOperation::WRITE:
                HandleWrite(io_data, bytes_transferred);
                break;
            default:
                break;
        }
    }
}

void IOCPServer::AcceptConnections() {
    std::cout << "[IOCP] Accept thread started" << std::endl;
    
    while (running.load()) {
        sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(listen_socket, (sockaddr*)&client_addr, &addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            if (running.load()) {
                int error = WSAGetLastError();
                if (error != WSAEINTR && error != WSAENOTSOCK) {
                    std::cerr << "[IOCP] Accept failed: " << error << std::endl;
                }
            }
            continue;
        }
        
        HandleAccept(client_socket);
    }
    
    std::cout << "[IOCP] Accept thread stopped" << std::endl;
}

void IOCPServer::HandleAccept(SOCKET client_socket) {
    // Associate with IOCP
    if (CreateIoCompletionPort((HANDLE)client_socket, completion_port, 0, 0) == NULL) {
        std::cerr << "[IOCP] Failed to associate client socket: " << GetLastError() << std::endl;
        closesocket(client_socket);
        return;
    }
    
    // Create client info
    int client_id = next_client_id.fetch_add(1);
    
    {
        w32::LockGuard lock(clients_mutex);
        
        CLIENT_INFO client;
        client.id = client_id;
        client.socket = client_socket;
        client.state = ClientState::CONNECTED;
        client.connected_at = std::chrono::steady_clock::now();
        client.last_activity = client.connected_at;
        client.ip_address = GetSocketAddress(client_socket);
        client.name = "anonymous";
        client.current_room = "general";
        
        clients[client_id] = client;
        socket_to_id[client_socket] = client_id;
    }
    
    std::cout << "[IOCP] New client " << client_id << " from " 
              << GetSocketAddress(client_socket) << std::endl;
    
    // Trigger connect callback
    if (on_connect) {
        thread_pool.enqueue([this, client_id, client_socket]() {
            on_connect(client_id, client_socket);
        });
    }
    
    // Post initial read
    PER_IO_DATA* io_data = new PER_IO_DATA();
    io_data->operation = IOOperation::READ;
    io_data->client_id = client_id;
    io_data->socket = client_socket;
    PostRead(io_data);
}

void IOCPServer::PostRead(PER_IO_DATA* io_data) {
    ZeroMemory(&io_data->overlapped, sizeof(OVERLAPPED));
    ZeroMemory(io_data->buffer, MAX_LEN);
    io_data->wsa_buf.buf = io_data->buffer;
    io_data->wsa_buf.len = MAX_LEN;
    
    DWORD flags = 0;
    DWORD bytes_recv = 0;
    
    int result = WSARecv(
        io_data->socket,
        &io_data->wsa_buf,
        1,
        &bytes_recv,
        &flags,
        &io_data->overlapped,
        NULL
    );
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[IOCP] WSARecv failed: " << error << std::endl;
            CleanupClient(io_data->client_id);
            delete io_data;
        }
    }
}

void IOCPServer::PostWrite(int client_id, const char* data, int length) {
    SOCKET sock = INVALID_SOCKET;
    {
        w32::LockGuard lock(clients_mutex);
        auto it = clients.find(client_id);
        if (it == clients.end()) {
            return;
        }
        sock = it->second.socket;
    }
    
    PER_IO_DATA* io_data = new PER_IO_DATA();
    io_data->operation = IOOperation::WRITE;
    io_data->client_id = client_id;
    io_data->socket = sock;
    memcpy(io_data->buffer, data, std::min(length, (int)MAX_LEN));
    io_data->wsa_buf.buf = io_data->buffer;
    io_data->wsa_buf.len = length;
    
    DWORD bytes_sent = 0;
    
    int result = WSASend(
        sock,
        &io_data->wsa_buf,
        1,
        &bytes_sent,
        0,
        &io_data->overlapped,
        NULL
    );
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            std::cerr << "[IOCP] WSASend failed: " << error << std::endl;
            delete io_data;
        }
    }
}

void IOCPServer::HandleRead(PER_IO_DATA* io_data, DWORD bytes_transferred) {
    // Update last activity
    {
        w32::LockGuard lock(clients_mutex);
        auto it = clients.find(io_data->client_id);
        if (it != clients.end()) {
            it->second.last_activity = std::chrono::steady_clock::now();
            it->second.message_count++;
        }
    }
    
    // Trigger message callback via thread pool
    if (on_message && bytes_transferred > 0) {
        int client_id = io_data->client_id;
        std::string message(io_data->buffer, bytes_transferred);
        
        thread_pool.enqueue([this, client_id, message]() {
            on_message(client_id, message.c_str(), (int)message.length());
        });
    }
    
    // Post another read
    PostRead(io_data);
}

void IOCPServer::HandleWrite(PER_IO_DATA* io_data, DWORD bytes_transferred) {
    // Write completed, free the IO data
    delete io_data;
}

void IOCPServer::CleanupClient(int client_id) {
    SOCKET sock = INVALID_SOCKET;
    
    {
        w32::LockGuard lock(clients_mutex);
        auto it = clients.find(client_id);
        if (it != clients.end()) {
            sock = it->second.socket;
            socket_to_id.erase(sock);
            clients.erase(it);
        }
    }
    
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    
    // Trigger disconnect callback
    if (on_disconnect) {
        thread_pool.enqueue([this, client_id]() {
            on_disconnect(client_id);
        });
    }
}

bool IOCPServer::Send(int client_id, const char* message, int length) {
    w32::LockGuard lock(clients_mutex);
    auto it = clients.find(client_id);
    if (it == clients.end()) {
        return false;
    }
    
    PostWrite(client_id, message, length);
    return true;
}

void IOCPServer::Broadcast(const char* message, int length, int exclude_id) {
    w32::LockGuard lock(clients_mutex);
    for (const auto& pair : clients) {
        if (pair.first != exclude_id) {
            PostWrite(pair.first, message, length);
        }
    }
}

void IOCPServer::DisconnectClient(int client_id) {
    CleanupClient(client_id);
}

CLIENT_INFO* IOCPServer::GetClient(int client_id) {
    w32::LockGuard lock(clients_mutex);
    auto it = clients.find(client_id);
    if (it != clients.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<CLIENT_INFO> IOCPServer::GetAllClients() {
    w32::LockGuard lock(clients_mutex);
    std::vector<CLIENT_INFO> result;
    for (const auto& pair : clients) {
        result.push_back(pair.second);
    }
    return result;
}
