/**
 * @file udp_transport.h
 * @brief UDP/RTPS传输层 - 车载以太网优化实现
 * @version 1.0
 * @date 2026-04-24
 * 
 * @note 支持汽车以太网100BASE-T1/1000BASE-T1
 * @note 支持TSN时间敏感网络预留
 * @note AUTOSAR风格命名和错误处理
 */

#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include "transport_interface.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 车载以太网特定定义
 * ============================================================================ */

/** 汽车以太网速率类型 */
typedef enum {
    AUTOMOTIVE_ETH_100BASE_T1 = 0,    /* 100Mbps汽车以太网 */
    AUTOMOTIVE_ETH_1000BASE_T1,        /* 1Gbps汽车以太网 */
    AUTOMOTIVE_ETH_STANDARD            /* 标准以太网 */
} automotive_eth_speed_t;

/** TSN流量类别 */
typedef enum {
    TSN_TC_CRITICAL = 0,      /* 关键控制流量 - 最高优先级 */
    TSN_TC_MEDIA,             /* 音视频媒体流 */
    TSN_TC_DIAGNOSTICS,       /* 诊断流量 */
    TSN_TC_BULK,              /* 批量数据 - 最低优先级 */
    TSN_TC_COUNT
} tsn_traffic_class_t;

/** TSN配置 */
typedef struct {
    bool enable_tsn;                    /* 启用TSN支持 */
    uint8_t traffic_class;              /* TSN流量类别 */
    uint32_t time_slot_us;              /* TSN时隙(微秒) */
    uint32_t bandwidth_reservation;     /* 带宽预留百分比 */
    uint32_t max_latency_us;            /* 最大延迟要求(微秒) */
} tsn_config_t;

/* ============================================================================
 * UDP/RTPS传输配置
 * ============================================================================ */

/** RTPS协议常量 */
#define RTPS_HEADER_SIZE        20      /* RTPS头部长度 */
#define RTPS_MAX_PAYLOAD        65507   /* 最大UDP载荷 */
#define RTPS_MAGIC_COOKIE       0x52545053  /* "RTPS" */

/** 车载优化参数 */
#define UDP_PRIORITY_QUEUES     4       /* 优先级队列数量 */
#define UDP_MAX_SOCKETS         16      /* 最大socket数量 */
#define UDP_RX_BUFFER_SIZE      65536   /* 接收缓冲区大小 */
#define UDP_TX_BUFFER_SIZE      65536   /* 发送缓冲区大小 */
#define UDP_COO_DEFAULT_US      100     /* 默认协同偏移(微秒) */

/** 传输模式 */
typedef enum {
    UDP_MODE_UNICAST = 0,     /* 单播模式 */
    UDP_MODE_MULTICAST,       /* 组播模式 */
    UDP_MODE_BROADCAST        /* 广播模式 */
} udp_transport_mode_t;

/** UDP传输扩展配置 */
typedef struct {
    transport_config_t base;
    
    /* 车载以太网配置 */
    automotive_eth_speed_t eth_speed;
    
    /* TSN配置 */
    tsn_config_t tsn;
    
    /* 传输模式 */
    udp_transport_mode_t mode;
    
    /* 组播配置 */
    eth_ip_addr_t multicast_addr;
    uint8_t multicast_ttl;
    
    /* 优先级队列配置 */
    uint8_t priority_queue_map[UDP_PRIORITY_QUEUES];
    uint32_t queue_weights[UDP_PRIORITY_QUEUES];
    
    /* 确定性延迟配置 */
    uint32_t deadline_us;           /* 截止时间(微秒) */
    uint32_t offset_us;             /* 协同偏移(微秒) */
    bool enable_deadline;           /* 启用截止时间调度 */
    
    /* 时间触发以太网(TTE)配置 */
    bool enable_tte;
    uint64_t cycle_time_ns;         /* TTE周期时间(纳秒) */
    uint64_t transmit_window_ns;    /* 发送窗口(纳秒) */
    
} udp_transport_config_t;

/** UDP传输统计信息 */
typedef struct {
    uint64_t tx_packets;            /* 发送包数 */
    uint64_t tx_bytes;              /* 发送字节数 */
    uint64_t rx_packets;            /* 接收包数 */
    uint64_t rx_bytes;              /* 接收字节数 */
    uint64_t tx_drops;              /* 发送丢包数 */
    uint64_t rx_drops;              /* 接收丢包数 */
    uint64_t deadline_misses;       /* 截止时间错过次数 */
    uint64_t deadline_violations;   /* 截止时间违规次数 */
    uint32_t latency_min_us;        /* 最小延迟 */
    uint32_t latency_max_us;        /* 最大延迟 */
    uint32_t latency_avg_us;        /* 平均延迟 */
    uint32_t jitter_us;             /* 抖动 */
} udp_transport_stats_t;

/* ============================================================================
 * UDP/RTPS API
 * ============================================================================ */

/**
 * @brief 创建UDP传输实例
 * @param config UDP传输配置
 * @return 传输句柄，NULL表示失败
 */
transport_handle_t* udp_transport_create(const udp_transport_config_t *config);

/**
 * @brief 销毁UDP传输实例
 * @param handle 传输句柄
 * @return ETH_OK成功
 */
eth_status_t udp_transport_destroy(transport_handle_t *handle);

/**
 * @brief 发送数据（带优先级）
 * @param handle 传输句柄
 * @param data 数据指针
 * @param len 数据长度
 * @param priority 优先级(0-3，0最高)
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t udp_transport_send_priority(transport_handle_t *handle,
                                          const uint8_t *data,
                                          uint32_t len,
                                          uint8_t priority,
                                          uint32_t timeout_ms);

/**
 * @brief 加入组播组
 * @param handle 传输句柄
 * @param multicast_addr 组播地址
 * @return ETH_OK成功
 */
eth_status_t udp_transport_join_multicast(transport_handle_t *handle,
                                           eth_ip_addr_t multicast_addr);

/**
 * @brief 离开组播组
 * @param handle 传输句柄
 * @param multicast_addr 组播地址
 * @return ETH_OK成功
 */
eth_status_t udp_transport_leave_multicast(transport_handle_t *handle,
                                            eth_ip_addr_t multicast_addr);

/**
 * @brief 设置TSN流量整形
 * @param handle 传输句柄
 * @param traffic_class 流量类别
 * @param config TSN配置
 * @return ETH_OK成功
 */
eth_status_t udp_transport_set_tsn_shaping(transport_handle_t *handle,
                                            tsn_traffic_class_t traffic_class,
                                            const tsn_config_t *config);

/**
 * @brief 配置时间触发发送
 * @param handle 传输句柄
 * @param enable 启用/禁用
 * @param cycle_time_ns 周期时间(纳秒)
 * @param transmit_window_ns 发送窗口(纳秒)
 * @return ETH_OK成功
 */
eth_status_t udp_transport_set_time_triggered(transport_handle_t *handle,
                                               bool enable,
                                               uint64_t cycle_time_ns,
                                               uint64_t transmit_window_ns);

/**
 * @brief 获取传输统计信息
 * @param handle 传输句柄
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t udp_transport_get_stats(transport_handle_t *handle,
                                      udp_transport_stats_t *stats);

/**
 * @brief 清除传输统计
 * @param handle 传输句柄
 * @return ETH_OK成功
 */
eth_status_t udp_transport_clear_stats(transport_handle_t *handle);

/**
 * @brief 设置汽车以太网参数
 * @param handle 传输句柄
 * @param speed 以太网速率
 * @return ETH_OK成功
 */
eth_status_t udp_transport_set_automotive_eth(transport_handle_t *handle,
                                               automotive_eth_speed_t speed);

/**
 * @brief 设置确定性调度参数
 * @param handle 传输句柄
 * @param deadline_us 截止时间(微秒)
 * @param offset_us 协同偏移(微秒)
 * @return ETH_OK成功
 */
eth_status_t udp_transport_set_deterministic(transport_handle_t *handle,
                                              uint32_t deadline_us,
                                              uint32_t offset_us);

/**
 * @brief 获取当前传输延迟
 * @param handle 传输句柄
 * @param latency_us 延迟输出(微秒)
 * @return ETH_OK成功
 */
eth_status_t udp_transport_get_latency(transport_handle_t *handle,
                                        uint32_t *latency_us);

#ifdef __cplusplus
}
#endif

#endif /* UDP_TRANSPORT_H */
