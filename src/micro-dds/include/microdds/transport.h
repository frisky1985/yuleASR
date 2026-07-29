/** @file transport.h
 * @brief Micro-DDS UDP传输层API头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 完整的UDP传输层API，支持DDS发现协议和多播通信
 */

#ifndef MICRODDS_TRANSPORT_H
#define MICRODDS_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 包含文件
 * ============================================================================ */
#include "types.h"
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * 配置定义
 * ============================================================================ */

/** @brief 默认DDS UDP基础端口 */
#define MICRODDS_UDP_PORT_BASE 7400U

/** @brief 用户数据端口偏移 */
#define MICRODDS_UDP_USER_PORT_OFFSET 10U

/** @brief 最大UDP负载大小 */
#define MICRODDS_UDP_MAX_PAYLOAD 1472U

/** @brief 最大对端数量 */
#define MICRODDS_MAX_PEERS 16U

/** @brief 最大多播组数量 */
#define MICRODDS_MAX_MULTICAST_GROUPS 4U

/** @brief 默认DDS发现多播地址 */
#define MICRODDS_DEFAULT_MULTICAST_ADDR "239.255.0.1"

/** @brief PDP宣告周期 (毫秒) */
#define MICRODDS_PDP_ANNOUNCE_PERIOD_MS 3000U

/** @brief PDP租约持续时间 (毫秒) */
#define MICRODDS_PDP_LEASE_DURATION_MS 10000U

/* ============================================================================
 * 初始化和关闭
 * ============================================================================ */

/**
 * @brief 初始化UDP传输层
 * @param domain_id DDS域ID
 * @return true 初始化成功
 * @note 会创建两个UDP套接字:
 *       - 发现端口: 7400 + domain_id
 *       - 用户数据端口: 7410 + domain_id
 */
bool MicroDDS_UDP_Init(uint32_t domain_id);

/**
 * @brief 关闭UDP传输层
 * @note 会关闭所有套接字并清理资源
 */
void MicroDDS_UDP_Shutdown(void);

/**
 * @brief 检查传输层是否已初始化
 * @return true 已初始化
 */
bool MicroDDS_UDP_IsInitialized(void);

/* ============================================================================
 * 多播组操作
 * ============================================================================ */

/**
 * @brief 加入多播组
 * @param group_address 多播组地址字符串 (如 "239.255.0.1")
 * @return true 加入成功
 * @note 多播地址范围: 224.0.0.0 到 239.255.255.255
 */
bool MicroDDS_UDP_JoinMulticast(const char* group_address);

/**
 * @brief 离开多播组
 * @param group_address 多播组地址字符串
 * @return true 离开成功
 */
bool MicroDDS_UDP_LeaveMulticast(const char* group_address);

/**
 * @brief 发送数据到多播组
 * @param data 数据指针
 * @param length 数据长度
 * @param group_address 多播组地址，为NULL时使用默认地址
 * @return true 发送成功
 */
bool MicroDDS_UDP_SendMulticast(const uint8_t* data, uint16_t length, const char* group_address);

/* ============================================================================
 * 对端管理
 * ============================================================================ */

/**
 * @brief 添加对端节点
 * @param address IPv4地址 (主机字节序)
 * @param port 端口号 (主机字节序)
 * @return true 添加成功
 * @note 例如: MicroDDS_UDP_AddPeer(0xC0A80101, 7410) 添加 192.168.1.1:7410
 */
bool MicroDDS_UDP_AddPeer(uint32_t address, uint16_t port);

/**
 * @brief 移除对端节点
 * @param address IPv4地址 (主机字节序)
 * @param port 端口号 (主机字节序)
 * @return true 移除成功
 */
bool MicroDDS_UDP_RemovePeer(uint32_t address, uint16_t port);

/**
 * @brief 获取对端数量
 * @return 对端数量
 */
uint32_t MicroDDS_UDP_GetPeerCount(void);

/* ============================================================================
 * 数据发送和接收
 * ============================================================================ */

/**
 * @brief 发送数据到指定地址
 * @param data 数据指针
 * @param length 数据长度 (最大1472字节)
 * @param dest_address 目标IP地址 (网络字节序)
 * @param dest_port 目标端口 (主机字节序)
 * @return true 发送成功
 */
bool MicroDDS_UDP_SendTo(const uint8_t* data, uint16_t length,
                         uint32_t dest_address, uint16_t dest_port);

/**
 * @brief 发送数据到所有对端 (组播到已知对端)
 * @param data 数据指针
 * @param length 数据长度 (最大1472字节)
 * @return true 至少一个对端发送成功
 */
bool MicroDDS_UDP_Send(const uint8_t* data, uint16_t length);

/**
 * @brief 接收数据包
 * @param buffer 数据缓冲区
 * @param buffer_size 缓冲区大小
 * @param src_address 输出源地址 (网络字节序, 可为NULL)
 * @param src_port 输出源端口 (主机字节序, 可为NULL)
 * @param timeout_ms 超时时间 (毫秒), 0表示非阻塞
 * @return 接收的字节数, 0表示无数据或超时
 * @note 此函数同时处理PDP发现消息
 */
uint16_t MicroDDS_UDP_Receive(uint8_t* buffer, uint16_t buffer_size,
                               uint32_t* src_address, uint16_t* src_port,
                               uint32_t timeout_ms);

/* ============================================================================
 * 地址和端口信息
 * ============================================================================ */

/**
 * @brief 获取本地发现端口号
 * @return 发现端口号 (主机字节序)
 */
uint16_t MicroDDS_UDP_GetDiscoveryPort(void);

/**
 * @brief 获取本地用户数据端口号
 * @return 用户数据端口号 (主机字节序)
 */
uint16_t MicroDDS_UDP_GetUserPort(void);

/**
 * @brief 获取本地参与者ID
 * @return 唯一的参与者ID
 */
uint32_t MicroDDS_UDP_GetParticipantId(void);

/* ============================================================================
 * DDS发现协议 (PDP)
 * ============================================================================ */

/**
 * @brief 获取发现的参与者数量
 * @return 活跃的参与者数量
 */
uint32_t MicroDDS_UDP_GetDiscoveredParticipantCount(void);

/**
 * @brief 获取发现的参与者信息
 * @param index 参与者索引 (0 到 GetDiscoveredParticipantCount()-1)
 * @param participant_id 输出参与者ID (可为NULL)
 * @param address 输出IP地址 (网络字节序, 可为NULL)
 * @param port 输出端口号 (主机字节序, 可为NULL)
 * @return true 获取成功
 */
bool MicroDDS_UDP_GetDiscoveredParticipant(uint32_t index, uint32_t* participant_id,
                                           uint32_t* address, uint16_t* port);

/* ============================================================================
 * 统计和调试
 * ============================================================================ */

/**
 * @brief 设置平台抽象函数 (用于非POSIX/非FreeRTOS平台)
 * @param get_time_fn 获取时间函数指针 (毫秒)
 * @param sleep_fn 睡眠函数指针 (毫秒)
 * @note 如果不设置，会使用内置的平台检测实现
 */
void MicroDDS_UDP_SetPlatformHooks(uint32_t (*get_time_fn)(void), void (*sleep_fn)(uint32_t));

/**
 * @brief 获取传输统计信息
 * @param packets_sent 输出发送包数 (可为NULL)
 * @param packets_received 输出接收包数 (可为NULL)
 * @param packets_dropped 输出丢弃包数 (可为NULL)
 * @param bytes_sent 输出发送字节数 (可为NULL)
 * @param bytes_received 输出接收字节数 (可为NULL)
 */
void MicroDDS_UDP_GetStats(uint32_t* packets_sent, uint32_t* packets_received,
                           uint32_t* packets_dropped, uint32_t* bytes_sent,
                           uint32_t* bytes_received);

/**
 * @brief 清除统计信息
 */
void MicroDDS_UDP_ClearStats(void);

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_TRANSPORT_H */
