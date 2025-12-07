/**
 * @file protocol.h
 * @author Jiafeng Chen (jiafengchen0104@gmail.com)
 * @brief 语音聊天服务器协议定义
 * @version 0.1
 * @date 2025-11-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef CHAT_SERVER_PROTOCOL_H
#define CHAT_SERVER_PROTOCOL_H

/**
 * @brief 语音聊天协议数据包类型枚举
 * 
 */
enum PacketType : uint8_t {   
    JOIN_ROOM = 1,    
    LEAVE_ROOM = 2
};

#pragma pack(push, 1)  // 开启1字节对齐
#include <cstdint>

struct VoiceChatPacketHeader {
    union {
        uint8_t header;  // 整个头部字节
        struct {
            uint8_t type : 7;  // 7bit类型
            uint8_t c    : 1;  // 1bit标识符
        } bits;
    };
    uint8_t reserved[3]; // 保留字节以对齐到4字节边界
    uint32_t payload_len; // 负载长度
};

/**
 * @brief 语音聊天协议数据包结构
 * c: 标识符 1bit（0代表纯数据，1代表控制包）
 * type: 控制包类型 7bit（见PacketType枚举）
 * data: 可变长度数据
 * +--------+----------------------+
 * |c| type | Data (variable size) |
 * +--------+----------------------+
 */
struct VoiceChatPacket {
    VoiceChatPacketHeader header; // 包头
    char data[];       // 可变长度数据（柔性数组）
};

/**
 * @brief 加入房间数据包结构
 * |-----------------+----------------+
 * | Field           | Size (bytes)   |
 * +-----------------+----------------+
 * | room_id         | 8              |
 * | user_id         | 8              |
 * +-----------------+----------------+
 */
struct JoinRoomPacket {
    uint64_t room_id;  // 房间ID
    uint64_t user_id;  // 用户ID
};

#pragma pack(pop)      // 恢复默认对齐



#endif // CHAT_SERVER_PROTOCOL_H