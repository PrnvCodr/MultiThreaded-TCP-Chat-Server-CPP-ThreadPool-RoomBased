#include "chat_room.h"
#include <sstream>
#include <algorithm>

ChatRoomManager::ChatRoomManager() {
    // Create default "general" room
    Room general("general", 0);
    general.topic = "Welcome to the chat server!";
    rooms["general"] = general;
}

bool ChatRoomManager::CreateRoom(const std::string& name, int owner_id, bool is_private, const std::string& password) {
    w32::LockGuard lock(rooms_mutex);
    
    // Check if room already exists
    if (rooms.find(name) != rooms.end()) {
        return false;
    }
    
    // Create new room
    Room room(name, owner_id);
    room.is_private = is_private;
    room.password = password;
    rooms[name] = room;
    
    return true;
}

bool ChatRoomManager::DeleteRoom(const std::string& name, int requester_id) {
    w32::LockGuard lock(rooms_mutex);
    
    // Can't delete general room
    if (name == "general") {
        return false;
    }
    
    auto it = rooms.find(name);
    if (it == rooms.end()) {
        return false;
    }
    
    // Only owner or admin (id 0) can delete
    if (it->second.owner_id != requester_id && requester_id != 0) {
        return false;
    }
    
    // Move all members to general
    for (int client_id : it->second.members) {
        client_rooms[client_id] = "general";
        rooms["general"].members.insert(client_id);
    }
    
    rooms.erase(it);
    return true;
}

bool ChatRoomManager::JoinRoom(const std::string& name, int client_id, const std::string& password) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = rooms.find(name);
    if (it == rooms.end()) {
        return false;
    }
    
    // Check password for private rooms
    if (it->second.is_private && it->second.password != password) {
        return false;
    }
    
    // Leave current room first
    auto current_it = client_rooms.find(client_id);
    if (current_it != client_rooms.end()) {
        auto room_it = rooms.find(current_it->second);
        if (room_it != rooms.end()) {
            room_it->second.members.erase(client_id);
        }
    }
    
    // Join new room
    it->second.members.insert(client_id);
    client_rooms[client_id] = name;
    
    return true;
}

void ChatRoomManager::LeaveRoom(int client_id) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = client_rooms.find(client_id);
    if (it != client_rooms.end()) {
        auto room_it = rooms.find(it->second);
        if (room_it != rooms.end()) {
            room_it->second.members.erase(client_id);
        }
        client_rooms.erase(it);
    }
}

std::string ChatRoomManager::GetClientRoom(int client_id) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = client_rooms.find(client_id);
    if (it != client_rooms.end()) {
        return it->second;
    }
    return "general";
}

bool ChatRoomManager::SetTopic(const std::string& room_name, const std::string& topic, int requester_id) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = rooms.find(room_name);
    if (it == rooms.end()) {
        return false;
    }
    
    // Only owner or admin can set topic
    if (it->second.owner_id != requester_id && requester_id != 0) {
        return false;
    }
    
    it->second.topic = topic;
    return true;
}

std::vector<std::string> ChatRoomManager::ListRooms() {
    w32::LockGuard lock(rooms_mutex);
    
    std::vector<std::string> room_list;
    for (const auto& pair : rooms) {
        if (!pair.second.is_private) {
            room_list.push_back(pair.first);
        }
    }
    
    std::sort(room_list.begin(), room_list.end());
    return room_list;
}

std::vector<int> ChatRoomManager::GetRoomMembers(const std::string& room_name) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = rooms.find(room_name);
    if (it == rooms.end()) {
        return {};
    }
    
    return std::vector<int>(it->second.members.begin(), it->second.members.end());
}

bool ChatRoomManager::RoomExists(const std::string& name) {
    w32::LockGuard lock(rooms_mutex);
    return rooms.find(name) != rooms.end();
}

std::string ChatRoomManager::GetRoomInfo(const std::string& name) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = rooms.find(name);
    if (it == rooms.end()) {
        return "Room not found";
    }
    
    std::stringstream ss;
    ss << "Room: #" << it->second.name << "\n";
    ss << "Topic: " << it->second.topic << "\n";
    ss << "Members: " << it->second.members.size() << "\n";
    ss << "Private: " << (it->second.is_private ? "Yes" : "No") << "\n";
    
    return ss.str();
}

std::vector<int> ChatRoomManager::GetRoommates(int client_id) {
    w32::LockGuard lock(rooms_mutex);
    
    auto it = client_rooms.find(client_id);
    if (it == client_rooms.end()) {
        // Default to general room
        auto general_it = rooms.find("general");
        if (general_it != rooms.end()) {
            return std::vector<int>(general_it->second.members.begin(), 
                                    general_it->second.members.end());
        }
        return {};
    }
    
    auto room_it = rooms.find(it->second);
    if (room_it == rooms.end()) {
        return {};
    }
    
    return std::vector<int>(room_it->second.members.begin(), 
                            room_it->second.members.end());
}
