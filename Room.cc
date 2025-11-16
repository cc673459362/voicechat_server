#include "Room.h"

Room::Room(const uint64_t id, const std::string& name) : id_(id), name_(name) {}
Room::~Room() {}
void Room::addUser(uint64_t userId, std::shared_ptr<UserConnection> conn) {
    conn_[userId] = conn;
}

void Room::removeUser(uint64_t userId) {
    conn_.erase(userId);
}

void Room::broadcastVoice(const char* data, size_t len, uint64_t excludeUserId) {
    for (const auto& [userId, userConn] : conn_) {
        //if (userId != excludeUserId) {
            send(userConn->getFd(), data, len, 0);
        //}
    }
}
