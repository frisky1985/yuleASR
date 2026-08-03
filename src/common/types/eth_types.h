/**
 * @file eth_types.h
 * @brief 以太网开发核心数据类型定义
 * @version 1.0
 * @date 2026-04-24
 */

#ifndef ETH_TYPES_H
#define ETH_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 基础类型定义
 * ============================================================================ */

/** 成功/失败返回值 */
typedef enum {
    ETH_OK = 0,
    ETH_ERROR = -1,
    ETH_TIMEOUT = -2,
    ETH_INVALID_PARAM = -3,
    ETH_NO_MEMORY = -4,
    ETH_BUSY = -5,
    ETH_NOT_INIT = -6,
} eth_status_t;

/** MAC地址类型 (6字节) */
typedef uint8_t eth_mac_addr_t[6];

/** IP地址类型 (IPv4) */
typedef uint32_t eth_ip_addr_t;

/** 数据包缓冲区 */
typedef struct {
    uint8_t *data;
    uint32_t len;
    uint32_t max_len;
} eth_buffer_t;

/* ============================================================================
 * 以太网设备配置
 * ============================================================================ */

/** 以太网工作模式 */
typedef enum {
    ETH_MODE_10M_HALF = 0,
    ETH_MODE_10M_FULL,
    ETH_MODE_100M_HALF,
    ETH_MODE_100M_FULL,
    ETH_MODE_1000M_FULL,
    ETH_MODE_AUTO_NEGOTIATION,
} eth_mode_t;

/** 以太网连接状态 */
typedef enum {
    ETH_LINK_DOWN = 0,
    ETH_LINK_UP,
} eth_link_status_t;

/** 以太网驱动配置 */
typedef struct {
    eth_mac_addr_t mac_addr;
    eth_mode_t mode;
    bool enable_dma;
    bool enable_checksum_offload;
    uint32_t rx_buffer_size;
    uint32_t tx_buffer_size;
    uint8_t rx_desc_count;
    uint8_t tx_desc_count;
} eth_config_t;

/* ============================================================================
 * DDS相关类型
 * ============================================================================ */

/** DDS域ID */
typedef uint32_t dds_domain_id_t;

/** DDS QoS策略 */
typedef enum {
    DDS_QOS_RELIABILITY_BEST_EFFORT = 0,
    DDS_QOS_RELIABILITY_RELIABLE,
} dds_qos_reliability_t;

typedef enum {
    DDS_QOS_DURABILITY_VOLATILE = 0,
    DDS_QOS_DURABILITY_TRANSIENT_LOCAL,
    DDS_QOS_DURABILITY_TRANSIENT,
    DDS_QOS_DURABILITY_PERSISTENT,
} dds_qos_durability_t;

/** DDS服务质量配置 */
typedef struct {
    dds_qos_reliability_t reliability;
    dds_qos_durability_t durability;
    uint32_t deadline_ms;
    uint32_t latency_budget_ms;
    uint32_t history_depth;
} dds_qos_t;

/** DDS主题名称 */
typedef char dds_topic_name_t[64];

/** DDS数据类型名称 */
typedef char dds_type_name_t[64];

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 以太网数据接收回调 */
typedef void (*eth_rx_callback_t)(eth_buffer_t *buffer, void *user_data);

/** 连接状态变化回调 */
typedef void (*eth_link_callback_t)(eth_link_status_t status, void *user_data);

/** DDS数据接收回调 */
typedef void (*dds_data_callback_t)(const void *data, uint32_t size, void *user_data);

/* ============================================================================
 * 宏定义工具
 * ============================================================================ */

#define ETH_MAC_ADDR(a,b,c,d,e,f) {a,b,c,d,e,f}
#define ETH_IP_ADDR(a,b,c,d) ((((uint32_t)(a))<<24) | (((uint32_t)(b))<<16) | \
                              (((uint32_t)(c))<<8)  | ((uint32_t)(d)))

#define ETH_MAC_IS_EQUAL(addr1, addr2) \
    (((addr1)[0] == (addr2)[0]) && \
     ((addr1)[1] == (addr2)[1]) && \
     ((addr1)[2] == (addr2)[2]) && \
     ((addr1)[3] == (addr2)[3]) && \
     ((addr1)[4] == (addr2)[4]) && \
     ((addr1)[5] == (addr2)[5]))

#define ETH_MAC_IS_BROADCAST(addr) \
    (((addr)[0] == 0xFF) && ((addr)[1] == 0xFF) && ((addr)[2] == 0xFF) && \
     ((addr)[3] == 0xFF) && ((addr)[4] == 0xFF) && ((addr)[5] == 0xFF))

#ifdef __cplusplus
}
#endif

#endif /* ETH_TYPES_H */
