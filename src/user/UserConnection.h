/**
 * @file UserConnection.h
 * @author Jiafeng Chen (jiafengchen0104@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-09-21
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef VOICECHAT_SERVER_USER_H
#define VOICECHAT_SERVER_USER_H

#include <netinet/in.h>
#include <string>
#include <vector>
#include <mutex>

class UserConnection {
public:
    UserConnection(int socket_fd, const struct sockaddr_in& addr)
        : socket_fd_(socket_fd), addr_(addr) {};
    ~UserConnection() {};

    int getFd() const { return socket_fd_; }

    void setUserId(uint64_t user_id) { user_id_ = user_id; }
    uint64_t getUserId() const { return user_id_; }

    void setRoomId(const uint64_t room_id) { room_id_ = room_id; }
    uint64_t getRoomId() const { return room_id_; }
private:
    int socket_fd_; // 客户端socket文件描述符
    struct sockaddr_in addr_; // 客户端地址信息
    uint64_t user_id_; // 用户ID
    uint64_t room_id_; // 房间ID

public:
    std::vector<char> pending_packet_data;
    size_t expected_payload_len; // 期望的负载长度
};

#endif // VOICECHAT_SERVER_USER_H
