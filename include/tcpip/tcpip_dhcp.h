/**
 * @file tcpip_dhcp.h
 * @brief DHCP客户端 - 动态主机配置协议
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 * @note RFC 2131 compliant
 */

#ifndef TCPIP_DHCP_H
#define TCPIP_DHCP_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DHCP配置定义
 * ============================================================================ */

#define TCPIP_DHCP_DISCOVER_TIMEOUT_MS  10000u  /**< 发现超时 10秒 */
#define TCPIP_DHCP_REQUEST_TIMEOUT_MS   10000u  /**< 请求超时 10秒 */
#define TCPIP_DHCP_RENEW_TIMEOUT_MS     60000u  /**< 续租重试超时 60秒 */
#define TCPIP_DHCP_MAX_RETRIES          3u      /**< 最大重试次数 */

/* ============================================================================
 * DHCP消息类型
 * ============================================================================ */

typedef enum {
    TCPIP_DHCP_DISCOVER = 1u,
    TCPIP_DHCP_OFFER    = 2u,
    TCPIP_DHCP_REQUEST  = 3u,
    TCPIP_DHCP_DECLINE  = 4u,
    TCPIP_DHCP_ACK      = 5u,
    TCPIP_DHCP_NAK      = 6u,
    TCPIP_DHCP_RELEASE  = 7u,
    TCPIP_DHCP_INFORM   = 8u
} tcpip_dhcp_msg_type_t;

/* ============================================================================
 * DHCP选项定义
 * ============================================================================ */

#define TCPIP_DHCP_OPT_PAD              0u
#define TCPIP_DHCP_OPT_SUBNET_MASK      1u
#define TCPIP_DHCP_OPT_ROUTER           3u
#define TCPIP_DHCP_OPT_DNS_SERVER       6u
#define TCPIP_DHCP_OPT_HOSTNAME         12u
#define TCPIP_DHCP_OPT_DOMAIN_NAME      15u
#define TCPIP_DHCP_OPT_MTU              26u
#define TCPIP_DHCP_OPT_BROADCAST        28u
#define TCPIP_DHCP_OPT_REQUESTED_IP     50u
#define TCPIP_DHCP_OPT_LEASE_TIME       51u
#define TCPIP_DHCP_OPT_MSG_TYPE         53u
#define TCPIP_DHCP_OPT_SERVER_ID        54u
#define TCPIP_DHCP_OPT_PARAM_LIST       55u
#define TCPIP_DHCP_OPT_RENEWAL_TIME     58u
#define TCPIP_DHCP_OPT_REBINDING_TIME   59u
#define TCPIP_DHCP_OPT_CLIENT_ID        61u
#define TCPIP_DHCP_OPT_END              255u

/* ============================================================================
 * DHCP状态回调类型
 * ============================================================================ */

/** DHCP状态变化回调 */
typedef void (*tcpip_dhcp_state_cb_t)(uint8_t iface_id,
                                       tcpip_dhcp_state_t state,
                                       const tcpip_ip_config_t *config,
                                       void *user_data);

/* ============================================================================
 * DHCP初始化API
 * ============================================================================ */

/**
 * @brief 初始化DHCP客户端
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_init(void);

/**
 * @brief 反初始化DHCP客户端
 */
void tcpip_dhcp_deinit(void);

/**
 * @brief DHCP主循环处理
 * @param elapsed_ms 经过毫秒数
 */
void tcpip_dhcp_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * DHCP配置API
 * ============================================================================ */

/**
 * @brief 启用接口DHCP客户端
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_enable(uint8_t iface_id);

/**
 * @brief 禁用接口DHCP客户端
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_disable(uint8_t iface_id);

/**
 * @brief 立即开始DHCP协商
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_start(uint8_t iface_id);

/**
 * @brief 停止DHCP客户端
 * @param iface_id 接口ID
 * @param release 是否发送RELEASE消息
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_stop(uint8_t iface_id, bool release);

/**
 * @brief 立即续租
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_renew(uint8_t iface_id);

/**
 * @brief 立即重绑定
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_rebind(uint8_t iface_id);

/* ============================================================================
 * DHCP状态查询API
 * ============================================================================ */

/**
 * @brief 获取DHCP状态
 * @param iface_id 接口ID
 * @param state 状态输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_get_state(uint8_t iface_id,
                                    tcpip_dhcp_state_t *state);

/**
 * @brief 检查DHCP是否已绑定
 * @param iface_id 接口ID
 * @param bound 绑定状态输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_is_bound(uint8_t iface_id, bool *bound);

/**
 * @brief 获取DHCP获取的配置
 * @param iface_id 接口ID
 * @param config 配置输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_get_config(uint8_t iface_id,
                                     tcpip_ip_config_t *config);

/**
 * @brief 获取租约剩余时间
 * @param iface_id 接口ID
 * @param remaining_ms 剩余毫秒数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_get_lease_remaining(uint8_t iface_id,
                                              uint32_t *remaining_ms);

/**
 * @brief 获取续租时间点
 * @param iface_id 接口ID
 * @param t1_ms T1时间(毫秒)输出
 * @param t2_ms T2时间(毫秒)输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_get_timers(uint8_t iface_id,
                                     uint32_t *t1_ms,
                                     uint32_t *t2_ms);

/* ============================================================================
 * DHCP回调API
 * ============================================================================ */

/**
 * @brief 注册DHCP状态变化回调
 * @param iface_id 接口ID (0xFF = 所有接口)
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_register_callback(uint8_t iface_id,
                                            tcpip_dhcp_state_cb_t callback,
                                            void *user_data);

/**
 * @brief 注销DHCP状态变化回调
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_unregister_callback(uint8_t iface_id);

/* ============================================================================
 * DHCP数据包处理API
 * ============================================================================ */

/**
 * @brief 处理接收的DHCP数据报
 * @param iface_id 入口接口ID
 * @param packet 数据包指针
 * @param len 数据长度
 * @return TCPIP_OK成功处理
 */
tcpip_error_t tcpip_dhcp_process_packet(uint8_t iface_id,
                                         const uint8_t *packet,
                                         uint16_t len);

/* ============================================================================
 * DHCP选项配置API
 * ============================================================================ */

/**
 * @brief 设置客户端标识符
 * @param iface_id 接口ID
 * @param client_id 客户端ID指针
 * @param len 客户端ID长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_set_client_id(uint8_t iface_id,
                                        const uint8_t *client_id,
                                        uint8_t len);

/**
 * @brief 设置请求的参数列表
 * @param iface_id 接口ID
 * @param params 参数类型数组
 * @param count 参数个数
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_set_param_list(uint8_t iface_id,
                                         const uint8_t *params,
                                         uint8_t count);

/**
 * @brief 设置主机名
 * @param iface_id 接口ID
 * @param hostname 主机名
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_set_hostname(uint8_t iface_id,
                                       const char *hostname);

/**
 * @brief 设置请求的IP地址 (偏好地址)
 * @param iface_id 接口ID
 * @param ip_addr 偏好IP地址
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_set_requested_ip(uint8_t iface_id,
                                           tcpip_ip_addr_t ip_addr);

/* ============================================================================
 * DHCP统计信息API
 * ============================================================================ */

/**
 * @brief 获取DHCP统计信息
 * @param iface_id 接口ID
 * @param discover_count DISCOVER消息数输出
 * @param offer_count OFFER消息数输出
 * @param request_count REQUEST消息数输出
 * @param ack_count ACK消息数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_get_stats(uint8_t iface_id,
                                    uint32_t *discover_count,
                                    uint32_t *offer_count,
                                    uint32_t *request_count,
                                    uint32_t *ack_count);

/**
 * @brief 清除DHCP统计
 * @param iface_id 接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_dhcp_clear_stats(uint8_t iface_id);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_DHCP_H */
