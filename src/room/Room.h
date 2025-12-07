/**
 * @file Room.h
 * @author Jiafeng Chen (jiafengchen0104@gmail.com)
 * @brief 语音聊天服务器房间定义
 * @version 0.1
 * @date 2025-11-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef VOICECHAT_SERVER_ROOM_H
#define VOICECHAT_SERVER_ROOM_H

#include <memory>
#include <unordered_map>
#include "user/UserConnection.h"

/**
 * @brief 语音聊天房间类
 * 
 */
class Room {
  public:
    Room(const uint64_t id, const std::string& name);
    ~Room();

    void addUser(uint64_t userId, std::shared_ptr<UserConnection> conn);
    void removeUser(uint64_t userId);
    bool isEmpty() const { return conn_.empty(); }
    void broadcastVoice(const char* data, size_t len, uint64_t excludeUserId);
  private:
    /**
     * @brief 房间ID
     * 
     */
    const uint64_t id_;
    /**
     * @brief 房间名称
     * 
     */
    const std::string name_;
    /**
     * @brief 存储客户端连接的映射
     * @key: 用户ID
     * @value: 用户连接对象
     */
    std::unordered_map<uint64_t, std::shared_ptr<UserConnection> > conn_; // 存储客户端连接
};

#endif // VOICECHAT_SERVER_ROOM_H