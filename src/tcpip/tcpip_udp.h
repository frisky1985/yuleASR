/**
 * @file tcpip_udp.h
 * @brief UDP用户数据报协议
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 * @note RFC 768 compliant
 */

#ifndef TCPIP_UDP_H
#define TCPIP_UDP_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * UDP初始化管理API
 * ============================================================================ */

/**
 * @brief 初始化UDP层
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_init(void);

/**
 * @brief 反初始化UDP层
 */
void tcpip_udp_deinit(void);

/**
 * @brief 创建UDP端点
 * @param socket_id 关联的Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_create(tcpip_socket_id_t socket_id);

/**
 * @brief 关闭UDP端点
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_close(tcpip_socket_id_t socket_id);

/* ============================================================================
 * UDP数据传输API
 * ============================================================================ */

/**
 * @brief 发送UDP数据报
 * @param socket_id Socket ID
 * @param dest_addr 目标IP地址
 * @param dest_port 目标端口
 * @param data 数据指针
 * @param len 数据长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_send(tcpip_socket_id_t socket_id,
                              tcpip_ip_addr_t dest_addr,
                              tcpip_port_t dest_port,
                              const uint8_t *data,
                              uint16_t len);

/**
 * @brief 发送UDP数据报 (带源端口指定)
 * @param socket_id Socket ID
 * @param src_port 源端口
 * @param dest_addr 目标IP地址
 * @param dest_port 目标端口
 * @param data 数据指针
 * @param len 数据长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_send_to(tcpip_socket_id_t socket_id,
                                 tcpip_port_t src_port,
                                 tcpip_ip_addr_t dest_addr,
                                 tcpip_port_t dest_port,
                                 const uint8_t *data,
                                 uint16_t len);

/**
 * @brief 接收UDP数据报
 * @param socket_id Socket ID
 * @param src_addr 源地址输出
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param received_len 实际接收长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_recv(tcpip_socket_id_t socket_id,
                              tcpip_sockaddr_t *src_addr,
                              uint8_t *buffer,
                              uint16_t buffer_size,
                              uint16_t *received_len);

/**
 * @brief 检查UDP接收缓冲区是否有数据
 * @param socket_id Socket ID
 * @param available_len 可用数据长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_get_rx_available(tcpip_socket_id_t socket_id,
                                          uint16_t *available_len);

/* ============================================================================
 * UDP组播API
 * ============================================================================ */

/**
 * @brief 加入多播组
 * @param socket_id Socket ID
 * @param mcast_addr 多播地址
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_join_multicast(tcpip_socket_id_t socket_id,
                                        tcpip_ip_addr_t mcast_addr);

/**
 * @brief 离开多播组
 * @param socket_id Socket ID
 * @param mcast_addr 多播地址
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_leave_multicast(tcpip_socket_id_t socket_id,
                                         tcpip_ip_addr_t mcast_addr);

/**
 * @brief 设置多播TTL
 * @param socket_id Socket ID
 * @param ttl TTL值
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_set_mcast_ttl(tcpip_socket_id_t socket_id, uint8_t ttl);

/**
 * @brief 设置广播使能
 * @param socket_id Socket ID
 * @param enable true启用，false禁用
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_set_broadcast(tcpip_socket_id_t socket_id, bool enable);

/* ============================================================================
 * UDP包处理API (内部使用)
 * ============================================================================ */

/**
 * @brief 处理接收的UDP数据报
 * @param iface_id 入口接口ID
 * @param src_addr 源IP地址
 * @param dst_addr 目的IP地址
 * @param packet 数据包指针 (包含UDP头)
 * @param len 数据长度
 * @return TCPIP_OK成功处理
 */
tcpip_error_t tcpip_udp_process_packet(uint8_t iface_id,
                                        tcpip_ip_addr_t src_addr,
                                        tcpip_ip_addr_t dst_addr,
                                        const uint8_t *packet,
                                        uint16_t len);

/**
 * @brief UDP主循环处理
 * @param elapsed_ms 经过毫秒数
 */
void tcpip_udp_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * UDP端点查找API
 * ============================================================================ */

/**
 * @brief 查找监听端口的UDP Socket
 * @param local_port 本地端口
 * @param socket_id Socket ID输出
 * @return TCPIP_OK找到
 */
tcpip_error_t tcpip_udp_find_socket(tcpip_port_t local_port,
                                     tcpip_socket_id_t *socket_id);

/**
 * @brief 检查端口是否被占用
 * @param port 端口号
 * @param in_use 是否被占用输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_is_port_in_use(tcpip_port_t port, bool *in_use);

/**
 * @brief 获取UDP统计信息
 * @param active_sockets 活动Socket数输出
 * @param total_rx 总接收字节数输出
 * @param total_tx 总发送字节数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_udp_get_stats(uint8_t *active_sockets,
                                   uint64_t *total_rx,
                                   uint64_t *total_tx);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_UDP_H */
