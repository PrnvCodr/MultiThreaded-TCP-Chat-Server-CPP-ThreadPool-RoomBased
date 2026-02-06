#ifndef MESSAGE_STORE_H
#define MESSAGE_STORE_H

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <chrono>
#include <fstream>
#include "win32_compat.h"

/**
 * @brief A chat message record
 */
struct ChatMessage {
    int sender_id;
    std::string sender_name;
    std::string room;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    
    ChatMessage() : sender_id(0) {}
    ChatMessage(int id, const std::string& name, const std::string& r, const std::string& msg)
        : sender_id(id), sender_name(name), room(r), content(msg) {
        timestamp = std::chrono::system_clock::now();
    }
    
    std::string GetTimestampString() const;
    std::string ToString() const;
};

/**
 * @brief Persistent message storage with in-memory cache
 */
class MessageStore {
public:
    /**
     * @brief Configuration
     */
    struct Config {
        size_t max_messages_per_room = 100;  // In-memory cache size
        size_t max_file_size_mb = 10;        // Max log file size before rotation
        std::string log_directory = "./chat_logs";
        bool enable_persistence = true;
    };
    
    explicit MessageStore(const Config& config);
    MessageStore(); // Default constructor
    
    ~MessageStore();
    
    // Non-copyable due to mutexes
    MessageStore(const MessageStore&) = delete;
    MessageStore& operator=(const MessageStore&) = delete;
    
    /**
     * @brief Store a message
     */
    void Store(const ChatMessage& message);
    
    /**
     * @brief Get recent messages from a room
     * @param room Room name
     * @param count Number of messages to retrieve
     */
    std::vector<ChatMessage> GetRecent(const std::string& room, size_t count = 10);
    
    /**
     * @brief Get messages from a specific sender
     */
    std::vector<ChatMessage> GetBySender(int sender_id, size_t count = 10);
    
    /**
     * @brief Search messages containing text
     */
    std::vector<ChatMessage> Search(const std::string& query, const std::string& room = "", size_t max_results = 20);
    
    /**
     * @brief Get total message count
     */
    size_t GetTotalCount() const;
    
    /**
     * @brief Clear all messages (for a room or all)
     */
    void Clear(const std::string& room = "");
    
    /**
     * @brief Flush pending writes to disk
     */
    void Flush();

private:
    Config config;
    
    // In-memory cache per room
    mutable w32::Mutex cache_mutex;
    std::unordered_map<std::string, std::deque<ChatMessage>> room_messages;
    
    // File output
    w32::Mutex file_mutex;
    std::ofstream log_file;
    size_t current_file_size = 0;
    
    void OpenLogFile();
    void RotateLogFile();
    void WriteToFile(const ChatMessage& message);
};

#endif // MESSAGE_STORE_H
