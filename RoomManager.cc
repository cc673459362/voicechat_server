#include "RoomManager.h"

// 初始化静态成员
RoomManager* RoomManager::instance = nullptr;
std::mutex RoomManager::mtx;

bool RoomManager::addToRoom(const uint64_t roomId, const uint64_t userId, std::shared_ptr<UserConnection> conn) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = rooms.find(roomId);
    if (it == rooms.end()) {
        // 房间不存在，创建新房间
        auto newRoom = std::make_shared<Room>(roomId,std::to_string(roomId)); // 端口号暂时设为0
        rooms[roomId] = newRoom;
        it = rooms.find(roomId);
    }
    conn->setUserId(userId);
    conn->setRoomId(roomId);
    // 假设Room类有addUser方法
    it->second->addUser(userId, conn);
    return true;
}

void RoomManager::removeFromRoom(const uint64_t roomId, uint64_t userId) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        // 假设Room类有removeUser方法
        it->second->removeUser(userId);
        // 如果房间空了，可以选择删除房间
        if (it->second->isEmpty()) {
            rooms.erase(it);
        }
    }
}

std::shared_ptr<Room> RoomManager::getRoom(const uint64_t roomId) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second;
    }
    return nullptr;
}