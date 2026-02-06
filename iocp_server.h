#ifndef IOCP_SERVER_H
#define IOCP_SERVER_H

#include "sockutil.h"
#include "thread_pool.h"
#include "win32_compat.h"
#include <unordered_map>
#include <functional>
#include <vector>

/**
 * @brief High-performance IOCP-based server
 * 
 * Uses Windows I/O Completion Ports for scalable async I/O.
 * Integrates with ThreadPool for task processing.
 */
class IOCPServer {
public:
    using MessageHandler = std::function<void(int client_id, const char* message, int length)>;
    using ConnectHandler = std::function<void(int client_id, SOCKET socket)>;
    using DisconnectHandler = std::function<void(int client_id)>;

    /**
     * @brief Construct IOCP server
     * @param port Port to listen on
     * @param pool Reference to thread pool
     */
    IOCPServer(int port, ThreadPool& pool);
    
    /**
     * @brief Destructor
     */
    ~IOCPServer();
    
    // Non-copyable
    IOCPServer(const IOCPServer&) = delete;
    IOCPServer& operator=(const IOCPServer&) = delete;
    
    /**
     * @brief Start the server
     * @return true if started successfully
     */
    bool Start();
    
    /**
     * @brief Stop the server
     */
    void Stop();
    
    /**
     * @brief Check if server is running
     */
    bool IsRunning() const { return running.load(); }
    
    /**
     * @brief Send message to specific client
     */
    bool Send(int client_id, const char* message, int length);
    
    /**
     * @brief Send message to all clients except sender
     */
    void Broadcast(const char* message, int length, int exclude_id = -1);
    
    /**
     * @brief Disconnect a client
     */
    void DisconnectClient(int client_id);
    
    /**
     * @brief Get client info
     */
    CLIENT_INFO* GetClient(int client_id);
    
    /**
     * @brief Get all connected clients
     */
    std::vector<CLIENT_INFO> GetAllClients();
    
    /**
     * @brief Set event handlers
     */
    void OnMessage(MessageHandler handler) { on_message = handler; }
    void OnConnect(ConnectHandler handler) { on_connect = handler; }
    void OnDisconnect(DisconnectHandler handler) { on_disconnect = handler; }

private:
    // Core components
    HANDLE completion_port;
    SOCKET listen_socket;
    ThreadPool& thread_pool;
    
    // State
    std::atomic<bool> running{false};
    std::atomic<int> next_client_id{1};
    
    // Client management
    std::unordered_map<int, CLIENT_INFO> clients;
    std::unordered_map<SOCKET, int> socket_to_id;
    w32::Mutex clients_mutex;
    
    // Worker threads for IOCP
    std::vector<w32::Thread> io_workers;
    
    // Event handlers
    MessageHandler on_message;
    ConnectHandler on_connect;
    DisconnectHandler on_disconnect;
    
    // Internal methods
    void IOCPWorkerThread();
    void AcceptConnections();
    void HandleAccept(SOCKET client_socket);
    void PostRead(PER_IO_DATA* io_data);
    void PostWrite(int client_id, const char* data, int length);
    void HandleRead(PER_IO_DATA* io_data, DWORD bytes_transferred);
    void HandleWrite(PER_IO_DATA* io_data, DWORD bytes_transferred);
    void CleanupClient(int client_id);
    
    int port_;
};

#endif // IOCP_SERVER_H
