#include "sockutil.h"
#include <iostream>

// Color codes
std::vector<std::string> colors = {
    "\033[31m",  // Red
    "\033[32m",  // Green
    "\033[33m",  // Yellow
    "\033[34m",  // Blue
    "\033[35m",  // Magenta
    "\033[36m"   // Cyan
};

/**
 * @brief Initialize Winsock library
 * @return true if successful
 */
bool InitializeWinsock() {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        std::cerr << "[Winsock] WSAStartup failed: " << result << std::endl;
        return false;
    }
    
    std::cout << "[Winsock] Initialized version " 
              << LOBYTE(wsa_data.wVersion) << "." 
              << HIBYTE(wsa_data.wVersion) << std::endl;
    return true;
}

/**
 * @brief Cleanup Winsock library
 */
void CleanupWinsock() {
    WSACleanup();
    std::cout << "[Winsock] Cleaned up" << std::endl;
}

/**
 * @brief Create a listening socket bound to port
 * @param port Port number to listen on
 * @return Socket handle or INVALID_SOCKET on error
 */
SOCKET CreateListenSocket(int port) {
    SOCKET listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
                                      NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listen_socket == INVALID_SOCKET) {
        std::cerr << "[Socket] WSASocket failed: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }
    
    // Allow address reuse
    int opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    // Bind to address
    sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[Socket] Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    
    // Start listening
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[Socket] Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    
    std::cout << "[Socket] Listening on port " << port << std::endl;
    return listen_socket;
}

/**
 * @brief Create client socket and connect to server
 * @param ip Server IP address
 * @param port Server port
 * @return Socket handle or INVALID_SOCKET on error
 */
SOCKET CreateClientSocket(const char* ip, int port) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "[Socket] socket() failed: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }
    
    sockaddr_in server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "[Socket] Invalid IP address" << std::endl;
        return INVALID_SOCKET;
    }
    
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "[Socket] Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        return INVALID_SOCKET;
    }
    
    return client_socket;
}

/**
 * @brief Set socket to non-blocking mode
 */
void SetSocketNonBlocking(SOCKET sock) {
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
}

/**
 * @brief Get human-readable error message
 */
std::string GetErrorMessage(int error_code) {
    char* msg_buf = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, error_code, 0, (LPSTR)&msg_buf, 0, NULL);
    
    std::string message = msg_buf ? msg_buf : "Unknown error";
    LocalFree(msg_buf);
    return message;
}

/**
 * @brief Get IP:Port string for a socket
 */
std::string GetSocketAddress(SOCKET sock) {
    sockaddr_in addr;
    int len = sizeof(addr);
    if (getpeername(sock, (sockaddr*)&addr, &len) == 0) {
        // Use inet_ntoa for IPv4 compatibility (older but standard on Windows)
        char* ip_str = inet_ntoa(addr.sin_addr);
        if (ip_str) {
            return std::string(ip_str) + ":" + std::to_string(ntohs(addr.sin_port));
        }
    }
    return "unknown";
}


/**
 * @brief Get color code by index
 */
std::string color(int code) {
    if (colors.empty()) return "";
    return colors[code % colors.size()];
}
