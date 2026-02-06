#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "sockutil.h"
#include "win32_compat.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>


/**
 * @brief Manages connection rate limiting, heartbeat, and spam prevention
 */
class ConnectionManager {
public:
  /**
   * @brief Configuration for connection management
   */
  struct Config {
    int max_connections_per_second = 50;  // Rate limit for new connections
    int max_messages_per_minute = 60;     // Spam prevention
    int heartbeat_interval_seconds = 30;  // Heartbeat check interval
    int connection_timeout_seconds = 120; // Disconnect if no activity
    int max_total_connections = 1000;     // Maximum concurrent connections
  };

  explicit ConnectionManager(const Config &config);
  ConnectionManager();

  // Non-copyable due to mutexes
  ConnectionManager(const ConnectionManager &) = delete;
  ConnectionManager &operator=(const ConnectionManager &) = delete;

  /**
   * @brief Check if a new connection should be allowed
   * @param ip_address Client IP address
   * @return true if connection is allowed
   */
  bool AllowConnection(const std::string &ip_address);

  /**
   * @brief Check if a client can send a message (rate limiting)
   * @param client_id Client ID
   * @return true if message is allowed
   */
  bool AllowMessage(int client_id);

  /**
   * @brief Record a message from client (for rate limiting)
   */
  void RecordMessage(int client_id);

  /**
   * @brief Check if client is banned
   */
  bool IsBanned(const std::string &ip_address);

  /**
   * @brief Ban an IP address
   */
  void Ban(const std::string &ip_address);

  /**
   * @brief Unban an IP address
   */
  void Unban(const std::string &ip_address);

  /**
   * @brief Check for timed-out connections
   * @return Vector of client IDs that should be disconnected
   */
  std::vector<int> CheckTimeouts(const std::vector<CLIENT_INFO> &clients);

  /**
   * @brief Mute a client
   */
  void Mute(int client_id, int duration_seconds = 0); // 0 = permanent

  /**
   * @brief Unmute a client
   */
  void Unmute(int client_id);

  /**
   * @brief Check if client is muted
   */
  bool IsMuted(int client_id);

  /**
   * @brief Update activity timestamp for a client
   */
  void UpdateActivity(int client_id);

  /**
   * @brief Get connection count
   */
  int GetConnectionCount() const { return current_connections; }

  /**
   * @brief Increment connection count (call on new connection)
   */
  void OnConnect() { current_connections++; }

  /**
   * @brief Decrement connection count (call on disconnect)
   */
  void OnDisconnect() {
    if (current_connections > 0)
      current_connections--;
  }

private:
  Config config;

  // Rate limiting for connections
  w32::Mutex rate_mutex;
  std::deque<std::chrono::steady_clock::time_point> connection_timestamps;

  // Message rate limiting per client
  w32::Mutex message_mutex;
  std::unordered_map<int, std::deque<std::chrono::steady_clock::time_point>>
      client_messages;

  // Banned IPs
  w32::Mutex ban_mutex;
  std::unordered_set<std::string> banned_ips;

  // Muted clients (with optional expiry)
  w32::Mutex mute_mutex;
  std::unordered_map<int, std::chrono::steady_clock::time_point>
      muted_clients; // time_point::max() = permanent

  // Activity tracking
  w32::Mutex activity_mutex;
  std::unordered_map<int, std::chrono::steady_clock::time_point> last_activity;

  std::atomic<int> current_connections{0};
};

#endif // CONNECTION_MANAGER_H
