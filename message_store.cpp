#include "message_store.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <windows.h>


std::string ChatMessage::GetTimestampString() const {
  auto time_t = std::chrono::system_clock::to_time_t(timestamp);
  std::tm tm;
  w32::LocalTime(&tm, &time_t);

  std::stringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

std::string ChatMessage::ToString() const {
  std::stringstream ss;
  ss << "[" << GetTimestampString() << "] ";
  ss << "[#" << room << "] ";
  ss << sender_name << ": " << content;
  return ss.str();
}

MessageStore::MessageStore() : MessageStore(Config()) {}

MessageStore::MessageStore(const Config &cfg) : config(cfg) {
  if (config.enable_persistence) {
    // Create log directory if it doesn't exist
    // Windows implementation using CreateDirectory
    if (GetFileAttributesA(config.log_directory.c_str()) ==
        INVALID_FILE_ATTRIBUTES) {
      CreateDirectoryA(config.log_directory.c_str(), NULL);
    }
    OpenLogFile();
  }
}

MessageStore::~MessageStore() {
  Flush();
  if (log_file.is_open()) {
    log_file.close();
  }
}

void MessageStore::Store(const ChatMessage &message) {
  // Store in memory cache
  {
    w32::LockGuard lock(cache_mutex);
    auto &messages = room_messages[message.room];
    messages.push_back(message);

    // Trim if over limit
    while (messages.size() > config.max_messages_per_room) {
      messages.pop_front();
    }
  }

  // Write to file
  if (config.enable_persistence) {
    WriteToFile(message);
  }
}

std::vector<ChatMessage> MessageStore::GetRecent(const std::string &room,
                                                 size_t count) {
  w32::LockGuard lock(cache_mutex);

  std::vector<ChatMessage> result;

  auto it = room_messages.find(room);
  if (it == room_messages.end()) {
    return result;
  }

  const auto &messages = it->second;
  size_t start = messages.size() > count ? messages.size() - count : 0;

  for (size_t i = start; i < messages.size(); ++i) {
    result.push_back(messages[i]);
  }

  return result;
}

std::vector<ChatMessage> MessageStore::GetBySender(int sender_id,
                                                   size_t count) {
  w32::LockGuard lock(cache_mutex);

  std::vector<ChatMessage> result;

  for (const auto &pair : room_messages) {
    for (const auto &msg : pair.second) {
      if (msg.sender_id == sender_id) {
        result.push_back(msg);
        if (result.size() >= count) {
          return result;
        }
      }
    }
  }

  return result;
}

std::vector<ChatMessage> MessageStore::Search(const std::string &query,
                                              const std::string &room,
                                              size_t max_results) {
  w32::LockGuard lock(cache_mutex);

  std::vector<ChatMessage> result;
  std::string lower_query = query;
  std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(),
                 ::tolower);

  auto search_in_room = [&](const std::deque<ChatMessage> &messages) {
    for (const auto &msg : messages) {
      if (result.size() >= max_results)
        break;

      std::string lower_content = msg.content;
      std::transform(lower_content.begin(), lower_content.end(),
                     lower_content.begin(), ::tolower);

      if (lower_content.find(lower_query) != std::string::npos) {
        result.push_back(msg);
      }
    }
  };

  if (!room.empty()) {
    auto it = room_messages.find(room);
    if (it != room_messages.end()) {
      search_in_room(it->second);
    }
  } else {
    for (const auto &pair : room_messages) {
      search_in_room(pair.second);
      if (result.size() >= max_results)
        break;
    }
  }

  return result;
}

size_t MessageStore::GetTotalCount() const {
  w32::LockGuard lock(cache_mutex);

  size_t total = 0;
  for (const auto &pair : room_messages) {
    total += pair.second.size();
  }
  return total;
}

void MessageStore::Clear(const std::string &room) {
  w32::LockGuard lock(cache_mutex);

  if (room.empty()) {
    room_messages.clear();
  } else {
    room_messages.erase(room);
  }
}

void MessageStore::Flush() {
  w32::LockGuard lock(file_mutex);
  if (log_file.is_open()) {
    log_file.flush();
  }
}

void MessageStore::OpenLogFile() {
  w32::LockGuard lock(file_mutex);

  // Create filename with date
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
  w32::LocalTime(&tm, &time_t);

  std::stringstream filename;
  filename << config.log_directory << "\\chat_"; // Use Windows backslash
  filename << std::put_time(&tm, "%Y%m%d");
  filename << ".log";

  log_file.open(filename.str(), std::ios::app);
  if (!log_file.is_open()) {
    std::cerr << "[MessageStore] Failed to open log file: " << filename.str()
              << std::endl;
    config.enable_persistence = false;
  } else {
    // Get file size using Windows API
    HANDLE hFile = CreateFileA(filename.str().c_str(), GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
      current_file_size = GetFileSize(hFile, NULL);
      CloseHandle(hFile);
    }
  }
}

void MessageStore::RotateLogFile() {
  if (log_file.is_open()) {
    log_file.close();
  }

  current_file_size = 0;
  OpenLogFile();
}

void MessageStore::WriteToFile(const ChatMessage &message) {
  w32::LockGuard lock(file_mutex);

  if (!log_file.is_open()) {
    return;
  }

  std::string line = message.ToString() + "\n";
  log_file << line;
  current_file_size += line.size();

  // Check if rotation needed
  if (current_file_size >= config.max_file_size_mb * 1024 * 1024) {
    RotateLogFile();
  }
}
