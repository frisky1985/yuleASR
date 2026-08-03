/**
 * @file tcpip_icmp.h
 * @brief ICMP协议 - Internet控制报文协议
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 * @note RFC 792 compliant
 */

#ifndef TCPIP_ICMP_H
#define TCPIP_ICMP_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ICMP配置定义
 * ============================================================================ */

#define TCPIP_ICMP_ECHO_DATA_SIZE       56u     /**< 默认Echo数据大小 */
#define TCPIP_ICMP_ECHO_TIMEOUT_MS      5000u   /**< Echo超时 5秒 */
#define TCPIP_ICMP_MAX_PENDING          4u      /**< 最大待处理响应数 */

/* ============================================================================
 * ICMP错误类型定义
 * ============================================================================ */

/** ICMP目的不可达代码 */
typedef enum {
    TCPIP_ICMP_NET_UNREACHABLE      = 0u,   /**< 网络不可达 */
    TCPIP_ICMP_HOST_UNREACHABLE     = 1u,   /**< 主机不可达 */
    TCPIP_ICMP_PROTO_UNREACHABLE    = 2u,   /**< 协议不可达 */
    TCPIP_ICMP_PORT_UNREACHABLE     = 3u,   /**< 端口不可达 */
    TCPIP_ICMP_FRAG_NEEDED          = 4u,   /**< 需要分片但DF置位 */
    TCPIP_ICMP_SOURCE_ROUTE_FAIL    = 5u    /**< 源路由失败 */
} tcpip_icmp_dest_unreach_code_t;

/* ============================================================================
 * ICMP初始化API
 * ============================================================================ */

/**
 * @brief 初始化ICMP模块
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_init(void);

/**
 * @brief 反初始化ICMP模块
 */
void tcpip_icmp_deinit(void);

/**
 * @brief ICMP主循环处理
 * @param elapsed_ms 经过毫秒数
 */
void tcpip_icmp_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * ICMP Echo (Ping) API
 * ============================================================================ */

/**
 * @brief 发送ICMP Echo请求 (Ping)
 * @param dest_addr 目标IP地址
 * @param iface_id 出口接口ID
 * @param data 数据指针 (可为NULL)
 * @param data_len 数据长度
 * @param seq 序列号
 * @param id 标识符
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_send_echo_request(tcpip_ip_addr_t dest_addr,
                                            uint8_t iface_id,
                                            const uint8_t *data,
                                            uint16_t data_len,
                                            uint16_t seq,
                                            uint16_t id);

/**
 * @brief 发送ICMP Echo响应
 * @param dest_addr 目标IP地址
 * @param iface_id 出口接口ID
 * @param data 原请求数据指针
 * @param data_len 数据长度
 * @param seq 序列号
 * @param id 标识符
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_send_echo_reply(tcpip_ip_addr_t dest_addr,
                                          uint8_t iface_id,
                                          const uint8_t *data,
                                          uint16_t data_len,
                                          uint16_t seq,
                                          uint16_t id);

/**
 * @brief 等待并处理Echo响应
 * @param id 等待的标识符
 * @param timeout_ms 超时(毫秒)
 * @param rtt_ms 往返时间输出(毫秒, 可为NULL)
 * @return TCPIP_OK收到响应，TCPIP_ERROR_TIMEOUT超时
 */
tcpip_error_t tcpip_icmp_wait_for_echo_reply(uint16_t id,
                                              uint32_t timeout_ms,
                                              uint32_t *rtt_ms);

/**
 * @brief 执行Ping操作 (同步)
 * @param dest_addr 目标IP地址
 * @param iface_id 出口接口ID (0xFF自动选择)
 * @param count Ping次数
 * @param timeout_ms 每次超时
 * @param reply_count 收到响应数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_ping(tcpip_ip_addr_t dest_addr,
                               uint8_t iface_id,
                               uint8_t count,
                               uint32_t timeout_ms,
                               uint8_t *reply_count);

/**
 * @brief 注册Ping响应回调 (异步)
 * @param callback 响应回调函数
 * @param user_data 用户数据
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_register_ping_callback(void (*callback)(
                                                   tcpip_ip_addr_t from,
                                                   uint16_t seq,
                                                   uint32_t rtt_ms,
                                                   void *user_data),
                                                 void *user_data);

/**
 * @brief 注销Ping响应回调
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_unregister_ping_callback(void);

/* ============================================================================
 * ICMP错误报文API
 * ============================================================================ */

/**
 * @brief 发送目的不可达消息
 * @param dest_addr 目标IP地址
 * @param iface_id 出口接口ID
 * @param code 错误代码
 * @param original_packet 原始数据包 (用于返回IP头和前8字节数据)
 * @param packet_len 原始数据包长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_send_dest_unreach(tcpip_ip_addr_t dest_addr,
                                            uint8_t iface_id,
                                            tcpip_icmp_dest_unreach_code_t code,
                                            const uint8_t *original_packet,
                                            uint16_t packet_len);

/**
 * @brief 发送超时消息 (TTL达到0)
 * @param dest_addr 目标IP地址
 * @param iface_id 出口接口ID
 * @param original_packet 原始数据包
 * @param packet_len 原始数据包长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_send_time_exceeded(tcpip_ip_addr_t dest_addr,
                                             uint8_t iface_id,
                                             const uint8_t *original_packet,
                                             uint16_t packet_len);

/* ============================================================================
 * ICMP包处理API
 * ============================================================================ */

/**
 * @brief 处理接收的ICMP包
 * @param iface_id 入口接口ID
 * @param src_addr 源IP地址
 * @param packet ICMP包指针
 * @param len 包长度
 * @return TCPIP_OK成功处理
 */
tcpip_error_t tcpip_icmp_process_packet(uint8_t iface_id,
                                         tcpip_ip_addr_t src_addr,
                                         const uint8_t *packet,
                                         uint16_t len);

/* ============================================================================
 * ICMP统计信息API
 * ============================================================================ */

/**
 * @brief 获取ICMP统计信息
 * @param echo_req_count Echo请求数输出
 * @param echo_rep_count Echo响应数输出
 * @param unreach_count 不可达消息数输出
 * @param error_count 错误数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_get_stats(uint32_t *echo_req_count,
                                    uint32_t *echo_rep_count,
                                    uint32_t *unreach_count,
                                    uint32_t *error_count);

/**
 * @brief 清除ICMP统计
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_clear_stats(void);

/**
 * @brief 启用/禁用ICMP Echo响应
 * @param enable true启用，false禁用
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_set_echo_reply(bool enable);

/**
 * @brief 获取Echo响应状态
 * @param enabled 启用状态输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_icmp_get_echo_reply(bool *enabled);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_ICMP_H */
