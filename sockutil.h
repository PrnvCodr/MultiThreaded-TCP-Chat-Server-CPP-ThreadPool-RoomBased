#ifndef SOCKUTIL_H
#define SOCKUTIL_H

// Windows-specific headers
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7+
#endif
#define WIN32_LEAN_AND_MEAN
// Suppress warnings for older functions
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <chrono>
#include <mswsock.h>
#include <string>
#include <vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#endif

// Constants
constexpr int MAX_CLIENTS = 1000;
constexpr int MAX_LEN = 2048;

/**
 * @brief Client State
 */
enum class ClientState { DISCONNECTED, CONNECTED, AUTHENTICATED };

/**
 * @brief I/O Operation Types for IOCP
 */
enum class IOOperation { READ, WRITE, ACCEPT };

/**
 * @brief Extended Overlapped structure for IOCP
 */
struct PER_IO_DATA {
  OVERLAPPED overlapped;
  WSABUF wsa_buf;
  char buffer[MAX_LEN];
  IOOperation operation;
  int client_id;
  SOCKET socket;

  PER_IO_DATA() {
    ZeroMemory(&overlapped, sizeof(OVERLAPPED));
    ZeroMemory(buffer, MAX_LEN);
    wsa_buf.buf = buffer;
    wsa_buf.len = MAX_LEN;
    socket = INVALID_SOCKET;
  }
};

/**
 * @brief Client Information
 */
struct CLIENT_INFO {
  int id;
  SOCKET socket;
  std::string name;
  std::string ip_address;
  ClientState state;
  std::chrono::steady_clock::time_point connected_at;
  std::chrono::steady_clock::time_point last_activity;
  int message_count = 0;
  std::string current_room;
};

// Utility functions
std::string GetSocketAddress(SOCKET sock);
bool InitializeWinsock();
void CleanupWinsock();
SOCKET CreateListenSocket(int port);
SOCKET CreateClientSocket(const char *ip, int port);
void SetNonBlocking(SOCKET sock);

#endif // SOCKUTIL_H
