#include "connection_manager.h"
#include <algorithm>
#include <vector>

ConnectionManager::ConnectionManager() : ConnectionManager(Config()) {}

ConnectionManager::ConnectionManager(const Config& cfg) : config(cfg) {}

bool ConnectionManager::AllowConnection(const std::string& ip_address) {
    // Check if banned
    if (IsBanned(ip_address)) {
        return false;
    }
    
    // Check max connections
    if (current_connections >= config.max_total_connections) {
        return false;
    }
    
    // Check rate limit
    w32::LockGuard lock(rate_mutex);
    auto now = std::chrono::steady_clock::now();
    auto one_second_ago = now - std::chrono::seconds(1);
    
    // Remove old timestamps
    while (!connection_timestamps.empty() && connection_timestamps.front() < one_second_ago) {
        connection_timestamps.pop_front();
    }
    
    if (connection_timestamps.size() >= (size_t)config.max_connections_per_second) {
        return false;
    }
    
    connection_timestamps.push_back(now);
    return true;
}

bool ConnectionManager::AllowMessage(int client_id) {
    if (IsMuted(client_id)) {
        return false;
    }
    
    w32::LockGuard lock(message_mutex);
    auto now = std::chrono::steady_clock::now();
    auto one_minute_ago = now - std::chrono::minutes(1);
    
    auto& timestamps = client_messages[client_id];
    
    // Remove old timestamps
    while (!timestamps.empty() && timestamps.front() < one_minute_ago) {
        timestamps.pop_front();
    }
    
    return timestamps.size() < (size_t)config.max_messages_per_minute;
}

void ConnectionManager::RecordMessage(int client_id) {
    w32::LockGuard lock(message_mutex);
    client_messages[client_id].push_back(std::chrono::steady_clock::now());
    
    // Also update activity
    UpdateActivity(client_id);
}

bool ConnectionManager::IsBanned(const std::string& ip_address) {
    w32::LockGuard lock(ban_mutex);
    return banned_ips.find(ip_address) != banned_ips.end();
}

void ConnectionManager::Ban(const std::string& ip_address) {
    w32::LockGuard lock(ban_mutex);
    banned_ips.insert(ip_address);
}

void ConnectionManager::Unban(const std::string& ip_address) {
    w32::LockGuard lock(ban_mutex);
    banned_ips.erase(ip_address);
}

std::vector<int> ConnectionManager::CheckTimeouts(const std::vector<CLIENT_INFO>& clients) {
    std::vector<int> timed_out;
    auto now = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(config.connection_timeout_seconds);
    
    w32::LockGuard lock(activity_mutex);
    for (const auto& client : clients) {
        auto it = last_activity.find(client.id);
        if (it != last_activity.end()) {
            if (now - it->second > timeout) {
                timed_out.push_back(client.id);
            }
        }
    }
    
    return timed_out;
}

void ConnectionManager::Mute(int client_id, int duration_seconds) {
    w32::LockGuard lock(mute_mutex);
    if (duration_seconds == 0) {
        muted_clients[client_id] = std::chrono::steady_clock::time_point::max();
    } else {
        muted_clients[client_id] = std::chrono::steady_clock::now() + std::chrono::seconds(duration_seconds);
    }
}

void ConnectionManager::Unmute(int client_id) {
    w32::LockGuard lock(mute_mutex);
    muted_clients.erase(client_id);
}

bool ConnectionManager::IsMuted(int client_id) {
    w32::LockGuard lock(mute_mutex);
    auto it = muted_clients.find(client_id);
    if (it == muted_clients.end()) {
        return false;
    }
    
    // Check if mute has expired
    if (it->second != std::chrono::steady_clock::time_point::max() &&
        std::chrono::steady_clock::now() > it->second) {
        muted_clients.erase(it);
        return false;
    }
    
    return true;
}

void ConnectionManager::UpdateActivity(int client_id) {
    w32::LockGuard lock(activity_mutex);
    last_activity[client_id] = std::chrono::steady_clock::now();
}
