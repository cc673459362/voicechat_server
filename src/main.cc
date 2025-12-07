#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "user/UserConnection.h"
#include "protocol/protocol.h"
#include "room/RoomManager.h"

// 从网络字节序（big-endian）缓冲读取 uint64_t（8 字节）
static uint64_t read_net_u64(const char* p) {
    uint32_t hi_net, lo_net;
    memcpy(&hi_net, p, 4);
    memcpy(&lo_net, p + 4, 4);
    uint32_t hi = ntohl(hi_net);
    uint32_t lo = ntohl(lo_net);
    return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

void handleJoinRoom(std::shared_ptr<UserConnection> conn, const char* data, size_t len) {
    // room_id (uint64) + user_id (uint64)
    if (len != sizeof(uint64_t) * 2) {
        std::cerr << "Invalid JOIN_ROOM packet length (expect 16 bytes for two uint64_t)" << std::endl;
        return;
    }

    uint64_t room_id = read_net_u64(data);
    uint64_t user_id = read_net_u64(data + 8);

    // 加入房间管理器（RoomManager 必须支持 uint64_t）
    RoomManager::getInstance().addToRoom(room_id, user_id, conn);
    std::cout << "User " << user_id << " joined room " << room_id << std::endl;
}

void handleVoiceData(std::shared_ptr<UserConnection> conn, const char* data, size_t len) {
    auto room = RoomManager::getInstance().getRoom(conn->getRoomId());
    if (!room) {
        std::cerr << "User not in any room" << std::endl;
        return;
    }

    // 广播给房间内其他成员
    room->broadcastVoice(data, len, conn->getUserId());
    std::cout << "Forwarded voice data (" << len << " bytes)" << std::endl;
    std::cout << "Raw bytes (hex): ";
    for (size_t i = 0; i < len; i++) {
        std::cout << std::hex << (int)(unsigned char)data[i] << " ";
    }
    std::cout << std::endl;
}

void handleLeaveRoom(std::shared_ptr<UserConnection> conn, const char* data, size_t len) {
    uint64_t user_id = conn->getUserId();
    RoomManager::getInstance().removeFromRoom(conn->getRoomId(), user_id);
    std::cout << "User " << user_id << " left room" << std::endl;
}

void handleControlPacket(std::shared_ptr<UserConnection> conn, const VoiceChatPacketHeader* header, const char* data, size_t len) {
    switch (header->bits.type) {
        case JOIN_ROOM:
            handleJoinRoom(conn, data, len);
            break;
        case LEAVE_ROOM:
            handleLeaveRoom(conn, data, len);
            break;
        default:
            std::cerr << "Unknown control packet type: " << static_cast<int>(header->bits.type) << std::endl;
            break;
    }
}

void processCompletePacket(std::shared_ptr<UserConnection> user_conn, const char* data, size_t len) {
    const VoiceChatPacketHeader* header = reinterpret_cast<const VoiceChatPacketHeader*>(data);
    
    // ✅ 字节序转换（网络字节序 -> 主机字节序）
    const uint32_t payload_len = ntohl(header->payload_len);
    
    std::cout << "🎯 处理完整包: type=" << static_cast<int>(header->bits.type)
              << ", c=" << static_cast<int>(header->bits.c)
              << ", payload_len=" << payload_len
              << ", 总大小=" << len << " 字节" << std::endl;
    
    const char* payload = data + sizeof(VoiceChatPacketHeader);
    
    if (sizeof(VoiceChatPacketHeader) + payload_len != len) {
        std::cerr << "❌ 数据包长度不匹配: header=" << sizeof(VoiceChatPacketHeader)
                  << " + payload=" << payload_len << " != total=" << len << std::endl;
        return;
    }
    
    switch (header->bits.c) {
        case 1: // 控制包
            std::cout << "🎮 处理控制包: type=" << static_cast<int>(header->bits.type) << std::endl;
            handleControlPacket(user_conn, header, payload, payload_len);
            break;
        case 0: // 语音数据包
            std::cout << "🎵 处理语音数据包: " << payload_len << " 字节" << std::endl;
            handleVoiceData(user_conn, payload, payload_len);
            break;
        default:
            std::cerr << "❌ 未知包类型: c=" << static_cast<int>(header->bits.c) << std::endl;
    }
}

void dealConnectionData(std::shared_ptr<UserConnection> user_conn, const char* raw_data, size_t len) {
    std::cout << "📦 收到数据: " << len << " 字节" 
              << ", 缓冲区已有: " << user_conn->pending_packet_data.size() << " 字节" 
              << std::endl;
    
    if (user_conn->pending_packet_data.empty()) {
        if (len < sizeof(VoiceChatPacketHeader)) {
            std::cerr << "❌ 数据太短，无法解析头部: " << len << " < " << sizeof(VoiceChatPacketHeader) << std::endl;
            return;
        }

        const VoiceChatPacketHeader* header = reinterpret_cast<const VoiceChatPacketHeader*>(raw_data);
        // ✅ 字节序转换
        const uint32_t payload_len = ntohl(header->payload_len);
        const size_t total_packet_size = sizeof(VoiceChatPacketHeader) + payload_len;
        
        std::cout << "📋 解析头部: type=" << static_cast<int>(header->bits.type) 
                  << ", c=" << static_cast<int>(header->bits.c)
                  << ", payload_len=" << payload_len
                  << ", 总包大小=" << total_packet_size << std::endl;
        
        if (len >= total_packet_size) {
            std::cout << "✅ 完整数据包，开始处理" << std::endl;
            processCompletePacket(user_conn, raw_data, total_packet_size);
            
            size_t remaining_len = len - total_packet_size;
            if (remaining_len > 0) {
                std::cout << "🔄 剩余数据: " << remaining_len << " 字节，继续处理" << std::endl;
                const char* remaining_data = raw_data + total_packet_size;
                dealConnectionData(user_conn, remaining_data, remaining_len);
            }
        } else {
            std::cout << "📥 分包数据，开始收集。需要: " << total_packet_size 
                      << " 字节，当前: " << len << " 字节" << std::endl;
            user_conn->expected_payload_len = payload_len;
            user_conn->pending_packet_data.assign(raw_data, raw_data + len);
        }
    } else {
        std::cout << "📥 继续收集分包数据: +" << len << " 字节" << std::endl;
        user_conn->pending_packet_data.insert(user_conn->pending_packet_data.end(), 
                                             raw_data, raw_data + len);
        
        const size_t current_size = user_conn->pending_packet_data.size();
        const size_t header_size = sizeof(VoiceChatPacketHeader);
        
        if (current_size >= header_size) {
            const VoiceChatPacketHeader* header = 
                reinterpret_cast<const VoiceChatPacketHeader*>(user_conn->pending_packet_data.data());
            // ✅ 字节序转换
            const uint32_t payload_len = ntohl(header->payload_len);
            const size_t total_packet_size = header_size + payload_len;
            
            std::cout << "📊 缓冲区状态: " << current_size << "/" << total_packet_size 
                      << " 字节 (header: " << header_size << ", payload: " << payload_len << ")" << std::endl;
            
            if (current_size >= total_packet_size) {
                std::cout << "🎯 分包收集完成，开始处理" << std::endl;
                processCompletePacket(user_conn, user_conn->pending_packet_data.data(), total_packet_size);
                
                std::vector<char> remaining_data(
                    user_conn->pending_packet_data.begin() + total_packet_size,
                    user_conn->pending_packet_data.end()
                );
                
                std::cout << "🗑️ 移除已处理数据: " << total_packet_size 
                          << " 字节，剩余: " << remaining_data.size() << " 字节" << std::endl;
                
                user_conn->pending_packet_data = std::move(remaining_data);
                
                if (!user_conn->pending_packet_data.empty()) {
                    std::cout << "🔄 继续处理缓冲区剩余数据" << std::endl;
                    dealConnectionData(user_conn, nullptr, 0);
                }
            } else {
                std::cout << "⏳ 分包未完成，继续等待" << std::endl;
            }
        } else {
            std::cout << "⏳ 头部数据不完整，继续等待。当前: " << current_size 
                      << "/" << header_size << " 字节" << std::endl;
        }
    }
}

// 处理单个客户端连接的线程函数
void handleClient(std::shared_ptr<UserConnection> user_conn) {
    char buffer[1024];
    int clientFd = user_conn->getFd();
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            std::cerr << "Client disconnected or error occurred." << std::endl;
            close(clientFd);
            break;
        }

        std::cout << "Received data: " << buffer << std::endl;
        dealConnectionData(user_conn, buffer, bytesReceived);
    }
}


int main() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket creation failed");
        return 1;
    }

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(20008);

    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind failed");
        close(serverFd);
        return 1;
    }

    if (listen(serverFd, 5) == -1) {
        perror("listen failed");
        close(serverFd);
        return 1;
    }

    

    std::cout << "Server listening on port 20008..." << std::endl;

    while (true) {
        struct sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);

        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);
        if (clientFd == -1) {
            perror("accept failed");
            continue;
        }

        // 创建客户端连接
        std::shared_ptr<UserConnection> user_conn =std::make_shared<UserConnection>(clientFd, clientAddr);

        // 为每个客户端创建独立线程
        std::thread(handleClient, user_conn).detach();
    }

    close(serverFd);
    return 0;
}
