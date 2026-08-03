/**
 * @file dds_tsn_transport.h
 * @brief DDS TSN传输层 - TSN (Time-Sensitive Networking) 支持
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 实现TSN Talker/Listener配置
 * @note 支持Stream ID管理和时间同步
 */

#ifndef DDS_TSN_TRANSPORT_H
#define DDS_TSN_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../common/types/eth_types.h"
#include "../../../tsn/tsn_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TSN 常量定义
 * ============================================================================ */

#define DDS_TSN_MAX_STREAMS             16U
#define DDS_TSN_MAX_TALKERS             8U
#define DDS_TSN_MAX_LISTENERS           8U
#define DDS_TSN_STREAM_ID_LEN           8U
#define DDS_TSN_VLAN_PRIORITY_MAX       7U
#define DDS_TSN_DEFAULT_VLAN_ID         100U
#define DDS_TSN_DEFAULT_PRIORITY        3U

/* TSN Stream 状态 */
typedef enum {
    DDS_TSN_STREAM_STATE_IDLE = 0,       /* 空闲状态 */
    DDS_TSN_STREAM_STATE_CONFIGURING,    /* 配置中 */
    DDS_TSN_STREAM_STATE_READY,          /* 就绪 */
    DDS_TSN_STREAM_STATE_RUNNING,        /* 运行中 */
    DDS_TSN_STREAM_STATE_ERROR           /* 错误状态 */
} dds_tsn_stream_state_t;

/* TSN Stream 类型 */
typedef enum {
    DDS_TSN_STREAM_TYPE_TALKER = 0,      /* 发送Stream */
    DDS_TSN_STREAM_TYPE_LISTENER,        /* 接收Stream */
    DDS_TSN_STREAM_TYPE_BOTH             /* 双向Stream */
} dds_tsn_stream_type_t;

/* ============================================================================
 * TSN Stream ID 和标识
 * ============================================================================ */

/** TSN Stream ID (64-bit) */
typedef struct {
    uint8_t id[DDS_TSN_STREAM_ID_LEN];   /* Stream ID 字节数组 */
} dds_tsn_stream_id_t;

/** TSN Stream标识符 */
typedef struct {
    dds_tsn_stream_id_t stream_id;       /* Stream ID */
    eth_mac_addr_t destination;          /* 目标MAC地址 */
    uint16_t vlan_id;                    /* VLAN ID */
    uint8_t priority;                    /* VLAN优先级 (0-7) */
} dds_tsn_stream_identification_t;

/* ============================================================================
 * TSN QoS 类型
 * ============================================================================ */

/** TSN流量规范 */
typedef struct {
    uint32_t interval_ns;                /* 间隔时间(纳秒) */
    uint32_t max_frame_size;             /* 最大帧大小 */
    uint32_t max_interval_frames;        /* 每间隔最大帧数 */
    uint32_t max_frame_count;            /* 最大帧计数 */
} dds_tsn_traffic_spec_t;

/** TSN用户到网络映射 */
typedef struct {
    eth_mac_addr_t source_addr;          /* 源MAC地址 */
    eth_mac_addr_t dest_addr;            /* 目标MAC地址 */
    uint16_t vlan_id;                    /* VLAN ID */
    uint8_t priority;                    /* 优先级 */
} dds_tsn_user_to_network_t;

/** TSN接口能力 */
typedef struct {
    uint32_t max_bandwidth;              /* 最大带宽 (Kbps) */
    uint32_t min_latency_ns;             /* 最小延迟 (纳秒) */
    bool supports_cbs;                   /* 支持CBS */
    bool supports_tas;                   /* 支持TAS */
    bool supports_ats;                   /* 支持ATS */
    uint8_t num_queues;                  /* 队列数量 */
} dds_tsn_interface_capabilities_t;

/* ============================================================================
 * TSN Talker 配置
 * ============================================================================ */

/** TSN Talker端点 */
typedef struct {
    dds_tsn_stream_identification_t id;  /* Stream标识 */
    dds_tsn_traffic_spec_t traffic_spec; /* 流量规范 */
    dds_tsn_user_to_network_t mapping;   /* 用户到网络映射 */
    uint32_t accumulated_latency_ns;     /* 累积延迟 */
    bool failed;                         /* 是否失败 */
    uint8_t failure_code;                /* 失败代码 */
} dds_tsn_talker_endpoint_t;

/** TSN Talker状态 */
typedef struct {
    dds_tsn_stream_state_t state;        /* Stream状态 */
    uint32_t frames_sent;                /* 已发送帧数 */
    uint64_t bytes_sent;                 /* 已发送字节数 */
    uint32_t tx_errors;                  /* 发送错误数 */
    uint32_t accumulated_latency_ns;     /* 累积延迟 */
    bool stream_active;                  /* Stream是否活跃 */
} dds_tsn_talker_status_t;

/* ============================================================================
 * TSN Listener 配置
 * ============================================================================ */

/** TSN Listener端点 */
typedef struct {
    dds_tsn_stream_identification_t id;  /* Stream标识 */
    eth_mac_addr_t source_addr;          /* 期望的源MAC地址 */
    uint16_t vlan_id;                    /* VLAN ID */
    uint32_t accumulated_latency_ns;     /* 累积延迟 */
    uint32_t max_latency_ns;             /* 最大允许延迟 */
} dds_tsn_listener_endpoint_t;

/** TSN Listener状态 */
typedef struct {
    dds_tsn_stream_state_t state;        /* Stream状态 */
    uint32_t frames_received;            /* 已接收帧数 */
    uint64_t bytes_received;             /* 已接收字节数 */
    uint32_t rx_errors;                  /* 接收错误数 */
    uint32_t late_frames;                /* 迟到帧数 */
    uint32_t accumulated_latency_ns;     /* 测量的延迟 */
    bool stream_active;                  /* Stream是否活跃 */
    bool source_active;                  /* 源是否活跃 */
} dds_tsn_listener_status_t;

/* ============================================================================
 * TSN Stream 配置
 * ============================================================================ */

typedef struct {
    dds_tsn_stream_identification_t id;  /* Stream标识 */
    dds_tsn_stream_type_t type;          /* Stream类型 */
    
    union {
        dds_tsn_talker_endpoint_t talker;
        dds_tsn_listener_endpoint_t listener;
    } endpoint;
    
    /* QoS映射 */
    dds_qos_t dds_qos;                   /* 关联的DDS QoS */
    dds_topic_name_t topic_name;         /* 关联的DDS主题 */
    
    /* TSN特定配置 */
    bool enable_cbs;                     /* 启用Credit-Based Shaper */
    bool enable_tas;                     /* 启用Time-Aware Shaper */
    bool enable_frame_preemption;        /* 启用帧预先取代 */
    uint32_t max_latency_ns;             /* 最大延迟要求 */
    
} dds_tsn_stream_config_t;

/* ============================================================================
 * TSN 统计信息
 * ============================================================================ */

typedef struct {
    uint32_t active_talkers;             /* 活跃Talker数 */
    uint32_t active_listeners;           /* 活跃Listener数 */
    uint64_t total_frames_sent;          /* 总发送帧数 */
    uint64_t total_frames_received;      /* 总接收帧数 */
    uint64_t total_bytes_sent;           /* 总发送字节数 */
    uint64_t total_bytes_received;       /* 总接收字节数 */
    uint32_t total_tx_errors;            /* 总发送错误 */
    uint32_t total_rx_errors;            /* 总接收错误 */
    uint32_t late_delivery_count;        /* 迟到交付数 */
    uint32_t stream_reservation_failures; /* Stream预留失败数 */
} dds_tsn_transport_stats_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

typedef void (*dds_tsn_stream_state_cb_t)(const dds_tsn_stream_id_t *stream_id,
                                           dds_tsn_stream_state_t new_state,
                                           void *user_data);

typedef void (*dds_tsn_data_rx_cb_t)(const dds_tsn_stream_id_t *stream_id,
                                      const uint8_t *data,
                                      uint32_t len,
                                      uint32_t timestamp_ns,
                                      void *user_data);

typedef void (*dds_tsn_latency_violation_cb_t)(const dds_tsn_stream_id_t *stream_id,
                                                uint32_t measured_latency_ns,
                                                uint32_t max_latency_ns,
                                                void *user_data);

/* ============================================================================
 * TSN传输层API
 * ============================================================================ */

/**
 * @brief 初始化DDS TSN传输层
 * @param local_mac 本地MAC地址
 * @param vlan_id 默认VLAN ID
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_transport_init(const eth_mac_addr_t local_mac, uint16_t vlan_id);

/**
 * @brief 反初始化DDS TSN传输层
 */
void dds_tsn_transport_deinit(void);

/**
 * @brief 启动TSN传输层
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_transport_start(void);

/**
 * @brief 停止TSN传输层
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_transport_stop(void);

/**
 * @brief TSN传输层主函数
 * @param elapsed_ms 经过的毫秒数
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_transport_main_function(uint32_t elapsed_ms);

/* ============================================================================
 * Stream管理API
 * ============================================================================ */

/**
 * @brief 创建TSN Stream
 * @param config Stream配置
 * @param stream_handle 输出Stream句柄
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_create_stream(const dds_tsn_stream_config_t *config,
                                    uint32_t *stream_handle);

/**
 * @brief 删除TSN Stream
 * @param stream_handle Stream句柄
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_delete_stream(uint32_t stream_handle);

/**
 * @brief 获取Stream配置
 * @param stream_handle Stream句柄
 * @param config 输出配置
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_get_stream_config(uint32_t stream_handle,
                                        dds_tsn_stream_config_t *config);

/**
 * @brief 更新Stream配置
 * @param stream_handle Stream句柄
 * @param config 新配置
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_update_stream_config(uint32_t stream_handle,
                                           const dds_tsn_stream_config_t *config);

/**
 * @brief 获取Stream状态
 * @param stream_handle Stream句柄
 * @param state 输出状态
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_get_stream_state(uint32_t stream_handle,
                                       dds_tsn_stream_state_t *state);

/* ============================================================================
 * Talker API
 * ============================================================================ */

/**
 * @brief 配置TSN Talker
 * @param stream_handle Stream句柄
 * @param talker_config Talker配置
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_configure_talker(uint32_t stream_handle,
                                       const dds_tsn_talker_endpoint_t *talker_config);

/**
 * @brief 获取Talker状态
 * @param stream_handle Stream句柄
 * @param status 状态输出
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_get_talker_status(uint32_t stream_handle,
                                        dds_tsn_talker_status_t *status);

/**
 * @brief 通过TSN Stream发送数据
 * @param stream_handle Stream句柄
 * @param data 数据指针
 * @param len 数据长度
 * @param timestamp_ns 发送时间戳(纳秒)
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_send_stream_data(uint32_t stream_handle,
                                       const uint8_t *data,
                                       uint32_t len,
                                       uint64_t timestamp_ns);

/* ============================================================================
 * Listener API
 * ============================================================================ */

/**
 * @brief 配置TSN Listener
 * @param stream_handle Stream句柄
 * @param listener_config Listener配置
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_configure_listener(uint32_t stream_handle,
                                         const dds_tsn_listener_endpoint_t *listener_config);

/**
 * @brief 获取Listener状态
 * @param stream_handle Stream句柄
 * @param status 状态输出
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_get_listener_status(uint32_t stream_handle,
                                          dds_tsn_listener_status_t *status);

/**
 * @brief 注册TSN数据接收回调
 * @param stream_handle Stream句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_register_rx_callback(uint32_t stream_handle,
                                           dds_tsn_data_rx_cb_t callback,
                                           void *user_data);

/**
 * @brief 注销TSN数据接收回调
 * @param stream_handle Stream句柄
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_unregister_rx_callback(uint32_t stream_handle);

/* ============================================================================
 * Stream ID管理API
 * ============================================================================ */

/**
 * @brief 生成唯一的Stream ID
 * @param topic_name 主题名称
 * @param domain_id 域ID
 * @param stream_id 输出Stream ID
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_generate_stream_id(const char *topic_name,
                                         uint32_t domain_id,
                                         dds_tsn_stream_id_t *stream_id);

/**
 * @brief 解析Stream ID
 * @param stream_id Stream ID
 * @param topic_name 输出主题名称
 * @param topic_name_len 主题名称缓冲区大小
 * @param domain_id 输出域ID
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_parse_stream_id(const dds_tsn_stream_id_t *stream_id,
                                      char *topic_name,
                                      uint32_t topic_name_len,
                                      uint32_t *domain_id);

/**
 * @brief 比较两个Stream ID
 * @param a 第一个Stream ID
 * @param b 第二个Stream ID
 * @return true 相同
 */
bool dds_tsn_stream_id_equal(const dds_tsn_stream_id_t *a, const dds_tsn_stream_id_t *b);

/**
 * @brief 将Stream ID转换为字符串
 * @param stream_id Stream ID
 * @param str 输出字符串缓冲区
 * @param str_len 缓冲区大小
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_stream_id_to_string(const dds_tsn_stream_id_t *stream_id,
                                          char *str, uint32_t str_len);

/* ============================================================================
 * Stream预留和调度API
 * ============================================================================ */

/**
 * @brief 预留Stream资源
 * @param stream_handle Stream句柄
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_reserve_stream(uint32_t stream_handle);

/**
 * @brief 释放Stream预留资源
 * @param stream_handle Stream句柄
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_release_stream(uint32_t stream_handle);

/**
 * @brief 配置TAS (Time-Aware Shaper)
 * @param stream_handle Stream句柄
 * @param gate_control_list 门控制列表
 * @param list_length 列表长度
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_configure_tas(uint32_t stream_handle,
                                    const void *gate_control_list,
                                    uint32_t list_length);

/**
 * @brief 配置CBS (Credit-Based Shaper)
 * @param stream_handle Stream句柄
 * @param idle_slope Idle Slope (位速率)
 * @param send_slope Send Slope (位速率)
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_configure_cbs(uint32_t stream_handle,
                                    uint64_t idle_slope,
                                    uint64_t send_slope);

/* ============================================================================
 * 状态回调API
 * ============================================================================ */

/**
 * @brief 注册Stream状态变化回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_register_state_callback(dds_tsn_stream_state_cb_t callback,
                                              void *user_data);

/**
 * @brief 注册延迟违规回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_register_latency_violation_callback(
    dds_tsn_latency_violation_cb_t callback, void *user_data);

/* ============================================================================
 * 统计API
 * ============================================================================ */

/**
 * @brief 获取TSN传输层统计
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_get_stats(dds_tsn_transport_stats_t *stats);

/**
 * @brief 清空TSN传输层统计
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_clear_stats(void);

/**
 * @brief 获取Stream统计
 * @param stream_handle Stream句柄
 * @param frames 帧计数输出
 * @param bytes 字节计数输出
 * @param latency_ns 平均延迟输出(纳秒)
 * @return ETH_OK成功
 */
eth_status_t dds_tsn_get_stream_stats(uint32_t stream_handle,
                                       uint64_t *frames,
                                       uint64_t *bytes,
                                       uint32_t *latency_ns);

#ifdef __cplusplus
}
#endif

#endif /* DDS_TSN_TRANSPORT_H */
