/**
 * @file tcpip_tcp.h
 * @brief TCP传输层控制协议
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 * @note RFC 793, RFC 1122, RFC 5681 compliant
 */

#ifndef TCPIP_TCP_H
#define TCPIP_TCP_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TCP连接管理API
 * ============================================================================ */

/**
 * @brief 初始化TCP层
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_init(void);

/**
 * @brief 反初始化TCP层
 */
void tcpip_tcp_deinit(void);

/**
 * @brief 创建TCP连接结构
 * @param socket_id 关联的Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_create(tcpip_socket_id_t socket_id);

/**
 * @brief 关闭TCP连接
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_close(tcpip_socket_id_t socket_id);

/**
 * @brief 强制关闭TCP连接 (RST包)
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_abort(tcpip_socket_id_t socket_id);

/* ============================================================================
 * TCP连接控制API
 * ============================================================================ */

/**
 * @brief 主动打开连接 (SYN)
 * @param socket_id Socket ID
 * @param dest_addr 目标IP地址
 * @param dest_port 目标端口
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_connect(tcpip_socket_id_t socket_id,
                                 tcpip_ip_addr_t dest_addr,
                                 tcpip_port_t dest_port);

/**
 * @brief 开始监听连接 (LISTEN)
 * @param socket_id Socket ID
 * @param backlog 最大待处理连接数
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_listen(tcpip_socket_id_t socket_id, uint8_t backlog);

/**
 * @brief 接受入站连接
 * @param listen_socket 监听Socket ID
 * @param new_socket 新连接Socket ID输出
 * @param remote_addr 远程地址输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_accept(tcpip_socket_id_t listen_socket,
                                tcpip_socket_id_t *new_socket,
                                tcpip_sockaddr_t *remote_addr);

/* ============================================================================
 * TCP数据传输API
 * ============================================================================ */

/**
 * @brief 发送数据
 * @param socket_id Socket ID
 * @param data 数据指针
 * @param len 数据长度
 * @param sent_len 实际发送长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_send(tcpip_socket_id_t socket_id,
                              const uint8_t *data,
                              uint16_t len,
                              uint16_t *sent_len);

/**
 * @brief 接收数据
 * @param socket_id Socket ID
 * @param buffer 接收缓冲区
 * @param buffer_size 缓冲区大小
 * @param received_len 实际接收长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_recv(tcpip_socket_id_t socket_id,
                              uint8_t *buffer,
                              uint16_t buffer_size,
                              uint16_t *received_len);

/**
 * @brief 开始发送FIN (半关闭)
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_shutdown(tcpip_socket_id_t socket_id);

/* ============================================================================
 * TCP状态查询API
 * ============================================================================ */

/**
 * @brief 获取TCP连接状态
 * @param socket_id Socket ID
 * @param state 状态输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_get_state(tcpip_socket_id_t socket_id,
                                   tcpip_tcp_state_t *state);

/**
 * @brief 检查连接是否已建立
 * @param socket_id Socket ID
 * @param established 是否已建立输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_is_connected(tcpip_socket_id_t socket_id,
                                      bool *established);

/**
 * @brief 检查是否有可读数据
 * @param socket_id Socket ID
 * @param available_len 可读字节数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_get_rx_available(tcpip_socket_id_t socket_id,
                                          uint16_t *available_len);

/**
 * @brief 获取远程地址
 * @param socket_id Socket ID
 * @param addr 地址信息输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_get_remote_addr(tcpip_socket_id_t socket_id,
                                         tcpip_sockaddr_t *addr);

/* ============================================================================
 * TCP参数配置API
 * ============================================================================ */

/**
 * @brief 设置TCP参数
 * @param socket_id Socket ID
 * @param keepalive 保活使能
 * @param keepalive_time 保活时间(秒)
 * @param keepalive_interval 保活间隔(秒)
 * @param keepalive_probes 保活探测次数
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_set_keepalive(tcpip_socket_id_t socket_id,
                                       bool keepalive,
                                       uint16_t keepalive_time,
                                       uint16_t keepalive_interval,
                                       uint8_t keepalive_probes);

/**
 * @brief 设置TCP窗口大小
 * @param socket_id Socket ID
 * @param rx_window 接收窗口
 * @param tx_window 发送窗口
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_set_window(tcpip_socket_id_t socket_id,
                                    uint16_t rx_window,
                                    uint16_t tx_window);

/**
 * @brief 设置TCP重传参数
 * @param socket_id Socket ID
 * @param max_retries 最大重传次数
 * @param retry_timeout_ms 重传超时(毫秒)
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_set_retry_params(tcpip_socket_id_t socket_id,
                                          uint8_t max_retries,
                                          uint32_t retry_timeout_ms);

/**
 * @brief 设置Nagle算法
 * @param socket_id Socket ID
 * @param enable true启用，false禁用
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_set_nagle(tcpip_socket_id_t socket_id, bool enable);

/**
 * @brief 设置TCP MSS
 * @param socket_id Socket ID
 * @param mss 最大段大小
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_set_mss(tcpip_socket_id_t socket_id, uint16_t mss);

/* ============================================================================
 * TCP包处理API (内部使用)
 * ============================================================================ */

/**
 * @brief 处理接收的TCP数据包
 * @param iface_id 入口接口ID
 * @param src_addr 源IP地址
 * @param dst_addr 目的IP地址
 * @param packet 数据包指针
 * @param len 数据长度
 * @return TCPIP_OK成功处理
 */
tcpip_error_t tcpip_tcp_process_packet(uint8_t iface_id,
                                        tcpip_ip_addr_t src_addr,
                                        tcpip_ip_addr_t dst_addr,
                                        const uint8_t *packet,
                                        uint16_t len);

/**
 * @brief TCP主循环处理 (重传计时等)
 * @param elapsed_ms 经过毫秒数
 */
void tcpip_tcp_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * TCP连接表操作内部API
 * ============================================================================ */

/**
 * @brief 查找TCP连接
 * @param local_port 本地端口
 * @param remote_addr 远程地址
 * @param remote_port 远程端口
 * @param socket_id Socket ID输出
 * @return TCPIP_OK找到连接
 */
tcpip_error_t tcpip_tcp_find_connection(tcpip_port_t local_port,
                                         tcpip_ip_addr_t remote_addr,
                                         tcpip_port_t remote_port,
                                         tcpip_socket_id_t *socket_id);

/**
 * @brief 获取空闲连接数
 * @return 空闲连接数
 */
uint8_t tcpip_tcp_get_free_connections(void);

/**
 * @brief 获取TCP连接统计信息
 * @param active_conn 活动连接数输出
 * @param total_rx 总接收字节数输出
 * @param total_tx 总发送字节数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_tcp_get_stats(uint8_t *active_conn,
                                   uint64_t *total_rx,
                                   uint64_t *total_tx);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_TCP_H */
