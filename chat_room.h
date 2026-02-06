#ifndef CHAT_ROOM_H
#define CHAT_ROOM_H

#include "win32_compat.h"
#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


/**
 * @brief Represents a single chat room
 */
struct Room {
  std::string name;
  std::string topic;
  std::unordered_set<int> members; // Client IDs
  int owner_id;
  std::chrono::steady_clock::time_point created_at;
  bool is_private;
  std::string password; // Only if private

  Room(const std::string &room_name = "", int owner = 0)
      : name(room_name), owner_id(owner), is_private(false) {
    created_at = std::chrono::steady_clock::now();
  }
};

/**
 * @brief Manages chat rooms
 */
class ChatRoomManager {
public:
  ChatRoomManager();

  // Non-copyable
  ChatRoomManager(const ChatRoomManager &) = delete;
  ChatRoomManager &operator=(const ChatRoomManager &) = delete;

  /**
   * @brief Create a new room
   * @return true if created successfully
   */
  bool CreateRoom(const std::string &name, int owner_id,
                  bool is_private = false, const std::string &password = "");

  /**
   * @brief Delete a room
   */
  bool DeleteRoom(const std::string &name, int requester_id);

  /**
   * @brief Join a room
   */
  bool JoinRoom(const std::string &name, int client_id,
                const std::string &password = "");

  /**
   * @brief Leave current room
   */
  void LeaveRoom(int client_id);

  /**
   * @brief Get client's current room
   */
  std::string GetClientRoom(int client_id);

  /**
   * @brief Set room topic
   */
  bool SetTopic(const std::string &room_name, const std::string &topic,
                int requester_id);

  /**
   * @brief Get list of all public rooms
   */
  std::vector<std::string> ListRooms();

  /**
   * @brief Get members of a room
   */
  std::vector<int> GetRoomMembers(const std::string &room_name);

  /**
   * @brief Check if room exists
   */
  bool RoomExists(const std::string &name);

  /**
   * @brief Get room info as string
   */
  std::string GetRoomInfo(const std::string &name);

  /**
   * @brief Get all client IDs in the same room
   */
  std::vector<int> GetRoommates(int client_id);

private:
  w32::Mutex rooms_mutex;
  std::unordered_map<std::string, Room> rooms;
  std::unordered_map<int, std::string> client_rooms; // client_id -> room_name
};

#endif // CHAT_ROOM_H
