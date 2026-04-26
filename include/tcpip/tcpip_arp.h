/**
 * @file tcpip_arp.h
 * @brief ARP协议 - 地址解析协议
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 * @note RFC 826 compliant
 */

#ifndef TCPIP_ARP_H
#define TCPIP_ARP_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ARP配置定义
 * ============================================================================ */

#define TCPIP_ARP_CACHE_TIMEOUT_MS      600000u     /**< 缓存超时 10分钟 */
#define TCPIP_ARP_RETRY_TIMEOUT_MS      1000u       /**< 重试超时 1秒 */
#define TCPIP_ARP_MAX_RETRIES           3u          /**< 最大重试次数 */
#define TCPIP_ARP_REQUEST_TIMEOUT_MS    5000u       /**< 请求总超时 5秒 */

/* ============================================================================
 * ARP缓存条目类型
 * ============================================================================ */

/** ARP缓存条目状态 */
typedef enum {
    TCPIP_ARP_ENTRY_FREE = 0u,          /**< 空闲条目 */
    TCPIP_ARP_ENTRY_PENDING,            /**< 等待解析 */
    TCPIP_ARP_ENTRY_VALID,              /**< 有效条目 */
    TCPIP_ARP_ENTRY_STATIC              /**< 静态条目 */
} tcpip_arp_entry_state_t;

/** ARP缓存条目 */
typedef struct {
    tcpip_ip_addr_t ip_addr;            /**< IP地址 */
    uint8_t mac_addr[6];                /**< MAC地址 */
    tcpip_arp_entry_state_t state;      /**< 条目状态 */
    uint32_t timestamp;                 /**< 更新时间戳 */
    uint8_t retry_count;                /**< 重试计数 */
    uint8_t iface_id;                   /**< 所属接口 */
} tcpip_arp_entry_t;

/* ============================================================================
 * ARP初始化API
 * ============================================================================ */

/**
 * @brief 初始化ARP模块
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_init(void);

/**
 * @brief 反初始化ARP模块
 */
void tcpip_arp_deinit(void);

/**
 * @brief ARP主循环处理 (缓存超时管理)
 * @param elapsed_ms 经过毫秒数
 */
void tcpip_arp_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * ARP缓存管理API
 * ============================================================================ */

/**
 * @brief 查找MAC地址
 * @param ip_addr 目标IP地址
 * @param iface_id 接口ID
 * @param mac_addr MAC地址输出
 * @return TCPIP_OK找到，TCPIP_ERROR_NOT_FOUND需要发送请求
 */
tcpip_error_t tcpip_arp_lookup(tcpip_ip_addr_t ip_addr,
                                uint8_t iface_id,
                                uint8_t *mac_addr);

/**
 * @brief 添加/更新缓存条目
 * @param ip_addr IP地址
 * @param mac_addr MAC地址
 * @param iface_id 接口ID
 * @param is_static 是否静态条目
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_add_entry(tcpip_ip_addr_t ip_addr,
                                   const uint8_t *mac_addr,
                                   uint8_t iface_id,
                                   bool is_static);

/**
 * @brief 删除缓存条目
 * @param ip_addr IP地址
 * @param iface_id 接口ID (0xFF = 所有接口)
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_del_entry(tcpip_ip_addr_t ip_addr, uint8_t iface_id);

/**
 * @brief 清空所有缓存条目
 * @param iface_id 接口ID (0xFF = 所有接口)
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_flush(uint8_t iface_id);

/**
 * @brief 获取缓存条目
 * @param index 条目索引
 * @param entry 条目信息输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_get_entry(uint8_t index, tcpip_arp_entry_t *entry);

/**
 * @brief 获取有效条目数
 * @return 有效条目数
 */
uint8_t tcpip_arp_get_entry_count(void);

/* ============================================================================
 * ARP请求/响应API
 * ============================================================================ */

/**
 * @brief 发送ARP请求
 * @param target_ip 目标IP地址
 * @param iface_id 出口接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_send_request(tcpip_ip_addr_t target_ip,
                                      uint8_t iface_id);

/**
 * @brief 发送ARP响应
 * @param target_ip 目标IP地址
 * @param target_mac 目标MAC地址
 * @param iface_id 出口接口ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_send_reply(tcpip_ip_addr_t target_ip,
                                    const uint8_t *target_mac,
                                    uint8_t iface_id);

/**
 * @brief 处理接收的ARP包
 * @param iface_id 入口接口ID
 * @param packet ARP包指针
 * @param len 包长度
 * @return TCPIP_OK成功处理
 */
tcpip_error_t tcpip_arp_process_packet(uint8_t iface_id,
                                        const uint8_t *packet,
                                        uint16_t len);

/**
 * @brief 检查是否有待处理的ARP请求
 * @param pending true有待处理请求
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_has_pending(bool *pending);

/* ============================================================================
 * ARP解析等待API
 * ============================================================================ */

/**
 * @brief 等待ARP解析完成
 * @param ip_addr 目标IP地址
 * @param timeout_ms 超时(毫秒)
 * @return TCPIP_OK成功，TCPIP_ERROR_TIMEOUT超时
 */
tcpip_error_t tcpip_arp_wait_for_resolve(tcpip_ip_addr_t ip_addr,
                                          uint32_t timeout_ms);

/**
 * @brief 取消等待ARP解析
 * @param ip_addr 目标IP地址
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_cancel_wait(tcpip_ip_addr_t ip_addr);

/* ============================================================================
 * 特殊地址检查API
 * ============================================================================ */

/**
 * @brief 检查IP地址是否需要ARP解析
 * @param ip_addr IP地址
 * @param iface_id 接口ID
 * @param need_arp 需要ARP输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_is_needed(tcpip_ip_addr_t ip_addr,
                                   uint8_t iface_id,
                                   bool *need_arp);

/**
 * @brief 检查IP地址是否是广播或多播地址
 * @param ip_addr IP地址
 * @return true是广播/多播地址
 */
bool tcpip_arp_is_broadcast(tcpip_ip_addr_t ip_addr);

/**
 * @brief 获取广播MAC地址
 * @param mac_addr MAC地址输出缓冲区
 */
void tcpip_arp_get_broadcast_mac(uint8_t *mac_addr);

/* ============================================================================
 * ARP统计信息API
 * ============================================================================ */

/**
 * @brief 获取ARP统计信息
 * @param request_count ARP请求数输出
 * @param reply_count ARP响应数输出
 * @param cache_hits 缓存命中数输出
 * @param cache_misses 缓存未命中数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_get_stats(uint32_t *request_count,
                                   uint32_t *reply_count,
                                   uint32_t *cache_hits,
                                   uint32_t *cache_misses);

/**
 * @brief 清除ARP统计
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_clear_stats(void);

/**
 * @brief 设置缓存超时时间
 * @param timeout_ms 超时时间(毫秒)
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_set_cache_timeout(uint32_t timeout_ms);

/**
 * @brief 获取缓存超时时间
 * @param timeout_ms 超时时间输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_arp_get_cache_timeout(uint32_t *timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_ARP_H */
