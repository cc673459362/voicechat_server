
#ifndef VOICECHAT_SERVER_ROOMMANAGER_H
#define VOICECHAT_SERVER_ROOMMANAGER_H
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "room/Room.h"
#include "user/UserConnection.h"

class RoomManager {
  public:
    static RoomManager& getInstance() {
        if (instance == nullptr) {  // 第一次检查（避免不必要的加锁）
            std::lock_guard<std::mutex> lock(mtx);
            if (instance == nullptr) {  // 第二次检查（确保唯一实例）
                instance = new RoomManager();
            }
        }
        return *instance;
    }

    // 删除拷贝构造和赋值运算符
    RoomManager(const RoomManager&) = delete;
    RoomManager& operator=(const RoomManager&) = delete;

    /**
     * @brief 添加用户到房间
     * 
     * @param roomId 
     * @param userId 
     * @param conn 
     * @return true if successful
     */
    bool addToRoom(const uint64_t roomId, const uint64_t userId, std::shared_ptr<UserConnection> conn);
    /**
     * @brief 从房间移除用户
     * 
     * @param roomId 
     * @param userId 
     */
    void removeFromRoom(const uint64_t roomId, uint64_t userId);
    /**
     * @brief 获取房间
     * 
     * @param roomId 
     * @return std::shared_ptr<Room> 
     */
    std::shared_ptr<Room> getRoom(const uint64_t roomId);
  private:
    RoomManager() = default;
    static RoomManager* instance;
    /**
     * @brief 存储房间的映射
     * @key: 房间ID
     * @value: 房间对象
     */
    std::unordered_map<uint64_t, std::shared_ptr<Room>> rooms; // 存储房间
    static std::mutex mtx;
};


#endif // VOICECHAT_SERVER_ROOMMANAGER_H