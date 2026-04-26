/**
 * @file dds_eth_transport.h
 * @brief DDS RTPS over ETH传输层 - DDS-RTPS 2.5 compliant
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 实现DDS RTPS over Ethernet传输层
 * @note 支持Participant发现协议(PDP)和多播通信
 */

#ifndef DDS_ETH_TRANSPORT_H
#define DDS_ETH_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RTPS over ETH 常量定义
 * ============================================================================ */

/** RTPS默认多播地址 (DDS规范) */
#define DDS_ETH_MULTICAST_ADDR_PREFIX    0x239, 0x255, 0x0, 0x1
#define DDS_ETH_MULTICAST_PORT_BASE      7400
#define DDS_ETH_METATRAFFIC_MULTICAST_PORT_OFFSET  0
#define DDS_ETH_USERTRAFFIC_MULTICAST_PORT_OFFSET  1

/** RTPS协议标识 */
#define DDS_RTPS_PROTOCOL_ID             0x52545053U  /* "RTPS" */
#define DDS_RTPS_PROTOCOL_VERSION_2_5    0x0205

/** 最大传输单元 */
#define DDS_ETH_MAX_UDP_PAYLOAD          65507U
#define DDS_ETH_MAX_RTPS_PACKET_SIZE     65000U

/** 传输层缓冲区配置 */
#define DDS_ETH_TX_BUFFER_SIZE           4096U
#define DDS_ETH_RX_BUFFER_SIZE           8192U
#define DDS_ETH_MAX_ENDPOINTS            32U
#define DDS_ETH_MAX_PARTICIPANTS         16U

/* ============================================================================
 * RTPS消息类型定义
 * ============================================================================ */

/** RTPS消息头 */
typedef struct __attribute__((packed)) {
    uint32_t protocol_id;           /* RTPS协议标识 */
    uint16_t protocol_version;      /* 协议版本 */
    uint16_t vendor_id;             /* 厂商ID */
    uint8_t  guid_prefix[12];       /* GUID前缀 */
} dds_rtps_header_t;

/** RTPS子消息类型 */
typedef enum {
    DDS_RTPS_SUBMSG_PAD             = 0x01,
    DDS_RTPS_SUBMSG_ACKNACK         = 0x06,
    DDS_RTPS_SUBMSG_HEARTBEAT       = 0x07,
    DDS_RTPS_SUBMSG_GAP             = 0x08,
    DDS_RTPS_SUBMSG_INFO_TS         = 0x09,
    DDS_RTPS_SUBMSG_INFO_SRC        = 0x0C,
    DDS_RTPS_SUBMSG_INFO_DST        = 0x0D,
    DDS_RTPS_SUBMSG_INFO_REPLY      = 0x0E,
    DDS_RTPS_SUBMSG_INFO_REPLY_IP4  = 0x0F,
    DDS_RTPS_SUBMSG_DATA            = 0x15,
    DDS_RTPS_SUBMSG_DATA_FRAG       = 0x16,
    DDS_RTPS_SUBMSG_NACK_FRAG       = 0x17,
    DDS_RTPS_SUBMSG_HEARTBEAT_FRAG  = 0x18,
    DDS_RTPS_SUBMSG_HEARTBEAT_BATCH = 0x19,
    DDS_RTPS_SUBMSG_SEC_BODY        = 0x30,
    DDS_RTPS_SUBMSG_SEC_PREFIX      = 0x31,
    DDS_RTPS_SUBMSG_SEC_POSTFIX     = 0x32,
    DDS_RTPS_SUBMSG_SRTPS_PREFIX    = 0x33,
    DDS_RTPS_SUBMSG_SRTPS_POSTFIX   = 0x34,
} dds_rtps_submsg_kind_t;

/** RTPS子消息标志 */
typedef enum {
    DDS_RTPS_SUBMSG_FLAG_ENDIANNESS = 0x01,
    DDS_RTPS_SUBMSG_FLAG_INLINE_QOS = 0x02,
    DDS_RTPS_SUBMSG_FLAG_DATA_PRESENT = 0x04,
    DDS_RTPS_SUBMSG_FLAG_KEY_PRESENT = 0x08,
} dds_rtps_submsg_flags_t;

/** RTPS子消息头 */
typedef struct __attribute__((packed)) {
    uint8_t submessage_id;          /* 子消息类型 */
    uint8_t flags;                  /* 标志位 */
    uint16_t submessage_length;     /* 子消息长度 */
} dds_rtps_submsg_header_t;

/** RTPS DATA子消息 */
typedef struct __attribute__((packed)) {
    dds_rtps_submsg_header_t header;
    uint16_t extra_flags;           /* 额外标志 */
    uint16_t octets_to_inline_qos;  /* 到Inline QoS的字节数 */
    uint8_t  reader_id[4];          /* Reader实体ID */
    uint8_t  writer_id[4];          /* Writer实体ID */
    uint32_t writer_seq_num_high;   /* Writer序列号高位 */
    uint32_t writer_seq_num_low;    /* Writer序列号低位 */
    /* 后面跟随serialized_payload */
} dds_rtps_data_submsg_t;

/** RTPS HEARTBEAT子消息 */
typedef struct __attribute__((packed)) {
    dds_rtps_submsg_header_t header;
    uint8_t  reader_id[4];
    uint8_t  writer_id[4];
    uint32_t first_seq_num_high;
    uint32_t first_seq_num_low;
    uint32_t last_seq_num_high;
    uint32_t last_seq_num_low;
    uint32_t count;
} dds_rtps_heartbeat_submsg_t;

/** RTPS ACKNACK子消息 */
typedef struct __attribute__((packed)) {
    dds_rtps_submsg_header_t header;
    uint8_t  reader_id[4];
    uint8_t  writer_id[4];
    uint32_t reader_sn_state;       /* Bitmap简化表示 */
    uint32_t count;
} dds_rtps_acknack_submsg_t;

/* ============================================================================
 * Endpoint映射管理
 * ============================================================================ */

/** Endpoint类型 */
typedef enum {
    DDS_ENDPOINT_TYPE_UNKNOWN = 0,
    DDS_ENDPOINT_TYPE_READER,
    DDS_ENDPOINT_TYPE_WRITER,
} dds_endpoint_type_t;

/** Endpoint GUID */
typedef struct {
    uint8_t prefix[12];             /* GUID前缀 */
    uint8_t entity_id[4];           /* 实体ID */
} dds_guid_t;

/** Endpoint网络地址 */
typedef struct {
    eth_ip_addr_t ip_addr;          /* IP地址 */
    uint16_t port;                  /* UDP端口 */
    bool is_multicast;              /* 是否多播 */
    eth_ip_addr_t multicast_addr;   /* 多播地址 */
} dds_endpoint_locator_t;

/** Endpoint配置 */
typedef struct {
    dds_guid_t guid;                /* Endpoint GUID */
    dds_endpoint_type_t type;       /* 端点类型 */
    dds_endpoint_locator_t locator; /* 网络定位器 */
    dds_topic_name_t topic_name;    /* 主题名称 */
    dds_type_name_t type_name;      /* 数据类型名称 */
    dds_qos_t qos;                  /* QoS配置 */
    uint32_t reader_writer_id;      /* Reader/Writer ID */
} dds_endpoint_config_t;

/** Endpoint条目 */
typedef struct {
    bool used;                      /* 是否使用中 */
    dds_endpoint_config_t config;   /* Endpoint配置 */
    uint32_t last_activity_ms;      /* 最后活动时间 */
    uint32_t packet_count;          /* 包计数 */
} dds_endpoint_entry_t;

/* ============================================================================
 * Participant管理
 * ============================================================================ */

/** Participant信息 */
typedef struct {
    bool active;                    /* 是否活跃 */
    dds_guid_t guid;                /* Participant GUID */
    eth_ip_addr_t unicast_addr;     /* 单播地址 */
    uint16_t metatraffic_unicast_port;   /* 元数据单播端口 */
    uint16_t usertraffic_unicast_port;   /* 用户数据单播端口 */
    uint16_t metatraffic_multicast_port; /* 元数据多播端口 */
    uint16_t usertraffic_multicast_port; /* 用户数据多播端口 */
    uint32_t protocol_version;      /* 协议版本 */
    uint32_t vendor_id;             /* 厂商ID */
    uint32_t last_seen_ms;          /* 最后可见时间 */
    uint32_t lease_duration_ms;     /* 租约持续时间 */
} dds_participant_info_t;

/** Participant表条目 */
typedef struct {
    bool used;                      /* 是否使用中 */
    dds_participant_info_t info;    /* Participant信息 */
    uint32_t discovery_time_ms;     /* 发现时间 */
} dds_participant_entry_t;

/* ============================================================================
 * 传输层配置
 * ============================================================================ */

/** DDS ETH传输层配置 */
typedef struct {
    eth_ip_addr_t local_ip;         /* 本地IP地址 */
    eth_mac_addr_t local_mac;       /* 本地MAC地址 */
    uint16_t domain_id;             /* DDS域ID */
    
    /* 端口配置 */
    uint16_t base_port;             /* 基础端口 */
    uint16_t port_offset;           /* 端口偏移 */
    
    /* 多播配置 */
    bool enable_multicast;          /* 启用多播 */
    eth_ip_addr_t multicast_addr;   /* 多播地址 */
    eth_mac_addr_t multicast_mac;   /* 多播MAC地址 */
    
    /* 缓冲区配置 */
    uint32_t tx_buffer_size;        /* 发送缓冲区大小 */
    uint32_t rx_buffer_size;        /* 接收缓冲区大小 */
    
    /* 定时器配置 */
    uint32_t heartbeat_period_ms;   /* 心跳周期 */
    uint32_t discovery_period_ms;   /* 发现周期 */
    uint32_t lease_duration_ms;     /* 租约持续时间 */
    
    /* QoS配置 */
    bool enable_qos_mapping;        /* 启用QoS到网络参数映射 */
} dds_eth_transport_config_t;

/** 传输层统计 */
typedef struct {
    uint32_t tx_packets;            /* 发送包数 */
    uint32_t rx_packets;            /* 接收包数 */
    uint32_t tx_bytes;              /* 发送字节数 */
    uint32_t rx_bytes;              /* 接收字节数 */
    uint32_t tx_errors;             /* 发送错误数 */
    uint32_t rx_errors;             /* 接收错误数 */
    uint32_t dropped_packets;       /* 丢弃包数 */
    uint32_t heartbeat_sent;        /* 发送心跳数 */
    uint32_t heartbeat_received;    /* 接收心跳数 */
    uint32_t acknack_sent;          /* 发送ACKNACK数 */
    uint32_t acknack_received;      /* 接收ACKNACK数 */
    uint32_t active_endpoints;      /* 活跃Endpoint数 */
    uint32_t active_participants;   /* 活跃Participant数 */
} dds_eth_transport_stats_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 数据接收回调 */
typedef void (*dds_eth_data_rx_cb_t)(const uint8_t *data, uint32_t len, 
                                      const dds_endpoint_locator_t *locator,
                                      void *user_data);

/** 发现回调 */
typedef void (*dds_eth_discovery_cb_t)(const dds_participant_info_t *participant,
                                        bool joined, void *user_data);

/** 链路事件回调 */
typedef void (*dds_eth_link_event_cb_t)(bool up, void *user_data);

/* ============================================================================
 * 传输层API
 * ============================================================================ */

/**
 * @brief 初始化DDS ETH传输层
 * @param config 传输层配置
 * @return ETH_OK成功
 */
eth_status_t dds_eth_transport_init(const dds_eth_transport_config_t *config);

/**
 * @brief 反初始化DDS ETH传输层
 */
void dds_eth_transport_deinit(void);

/**
 * @brief 启动传输层
 * @return ETH_OK成功
 */
eth_status_t dds_eth_transport_start(void);

/**
 * @brief 停止传输层
 * @return ETH_OK成功
 */
eth_status_t dds_eth_transport_stop(void);

/**
 * @brief 注册数据接收回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_eth_register_data_callback(dds_eth_data_rx_cb_t callback, void *user_data);

/**
 * @brief 注册发现回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t dds_eth_register_discovery_callback(dds_eth_discovery_cb_t callback, void *user_data);

/**
 * @brief 发送RTPS数据报
 * @param data 数据指针
 * @param len 数据长度
 * @param locator 目标定位器
 * @param reliable 是否可靠传输
 * @return ETH_OK成功
 */
eth_status_t dds_eth_send_rtps(const uint8_t *data, uint32_t len,
                                const dds_endpoint_locator_t *locator,
                                bool reliable);

/**
 * @brief 处理接收的数据
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t dds_eth_transport_process(uint32_t timeout_ms);

/**
 * @brief 获取传输层统计
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t dds_eth_get_stats(dds_eth_transport_stats_t *stats);

/**
 * @brief 清空传输层统计
 * @return ETH_OK成功
 */
eth_status_t dds_eth_clear_stats(void);

/* ============================================================================
 * Endpoint管理API
 * ============================================================================ */

/**
 * @brief 注册Endpoint
 * @param config Endpoint配置
 * @param endpoint_id 输出Endpoint ID
 * @return ETH_OK成功
 */
eth_status_t dds_eth_register_endpoint(const dds_endpoint_config_t *config, 
                                        uint32_t *endpoint_id);

/**
 * @brief 注销Endpoint
 * @param endpoint_id Endpoint ID
 * @return ETH_OK成功
 */
eth_status_t dds_eth_unregister_endpoint(uint32_t endpoint_id);

/**
 * @brief 查找Endpoint
 * @param guid Endpoint GUID
 * @param endpoint 输出Endpoint配置
 * @return ETH_OK找到
 */
eth_status_t dds_eth_find_endpoint(const dds_guid_t *guid, 
                                    dds_endpoint_config_t *endpoint);

/**
 * @brief 获取所有活跃Endpoint
 * @param endpoints 输出Endpoint数组
 * @param max_count 最大数量
 * @param actual_count 实际数量输出
 * @return ETH_OK成功
 */
eth_status_t dds_eth_get_active_endpoints(dds_endpoint_config_t *endpoints,
                                           uint32_t max_count,
                                           uint32_t *actual_count);

/* ============================================================================
 * Participant管理API
 * ============================================================================ */

/**
 * @brief 添加远程Participant
 * @param info Participant信息
 * @return ETH_OK成功
 */
eth_status_t dds_eth_add_participant(const dds_participant_info_t *info);

/**
 * @brief 移除远程Participant
 * @param guid Participant GUID
 * @return ETH_OK成功
 */
eth_status_t dds_eth_remove_participant(const dds_guid_t *guid);

/**
 * @brief 获取远程Participant
 * @param guid Participant GUID
 * @param info 输出Participant信息
 * @return ETH_OK找到
 */
eth_status_t dds_eth_get_participant(const dds_guid_t *guid,
                                      dds_participant_info_t *info);

/**
 * @brief 获取所有活跃Participant
 * @param participants 输出Participant数组
 * @param max_count 最大数量
 * @param actual_count 实际数量输出
 * @return ETH_OK成功
 */
eth_status_t dds_eth_get_active_participants(dds_participant_info_t *participants,
                                              uint32_t max_count,
                                              uint32_t *actual_count);

/**
 * @brief 更新Participant活动时间
 * @param guid Participant GUID
 * @return ETH_OK成功
 */
eth_status_t dds_eth_refresh_participant(const dds_guid_t *guid);

/* ============================================================================
 * 多播组管理API
 * ============================================================================ */

/**
 * @brief 加入多播组
 * @param multicast_addr 多播地址
 * @param multicast_mac 多播MAC地址
 * @return ETH_OK成功
 */
eth_status_t dds_eth_join_multicast_group(eth_ip_addr_t multicast_addr,
                                           const eth_mac_addr_t multicast_mac);

/**
 * @brief 离开多播组
 * @param multicast_addr 多播地址
 * @return ETH_OK成功
 */
eth_status_t dds_eth_leave_multicast_group(eth_ip_addr_t multicast_addr);

/**
 * @brief 计算DDS多播地址
 * @param domain_id 域ID
 * @param participant_id Participant ID
 * @param multicast_addr 输出多播地址
 * @return ETH_OK成功
 */
eth_status_t dds_eth_calc_multicast_addr(uint32_t domain_id, uint32_t participant_id,
                                          eth_ip_addr_t *multicast_addr);

/* ============================================================================
 * RTPS消息封装/解封API
 * ============================================================================ */

/**
 * @brief 创建RTPS消息头
 * @param header 输出消息头
 * @param guid_prefix GUID前缀
 * @return ETH_OK成功
 */
eth_status_t dds_eth_create_rtps_header(dds_rtps_header_t *header,
                                         const uint8_t guid_prefix[12]);

/**
 * @brief 解析RTPS消息头
 * @param data 数据指针
 * @param len 数据长度
 * @param header 输出消息头
 * @param header_len 输出头部长度
 * @return ETH_OK成功
 */
eth_status_t dds_eth_parse_rtps_header(const uint8_t *data, uint32_t len,
                                        dds_rtps_header_t *header,
                                        uint32_t *header_len);

/**
 * @brief 封装RTPS DATA子消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param writer_id Writer实体ID
 * @param reader_id Reader实体ID
 * @param seq_num 序列号
 * @param payload 数据负载
 * @param payload_len 负载长度
 * @param actual_len 实际长度输出
 * @return ETH_OK成功
 */
eth_status_t dds_eth_encapsulate_data(uint8_t *buffer, uint32_t buf_size,
                                       const uint8_t writer_id[4],
                                       const uint8_t reader_id[4],
                                       uint64_t seq_num,
                                       const uint8_t *payload,
                                       uint32_t payload_len,
                                       uint32_t *actual_len);

/**
 * @brief 解封RTPS DATA子消息
 * @param data 数据指针
 * @param len 数据长度
 * @param writer_id 输出Writer ID
 * @param reader_id 输出Reader ID
 * @param seq_num 输出序列号
 * @param payload 输出负载指针
 * @param payload_len 输出负载长度
 * @return ETH_OK成功
 */
eth_status_t dds_eth_decapsulate_data(const uint8_t *data, uint32_t len,
                                       uint8_t writer_id[4],
                                       uint8_t reader_id[4],
                                       uint64_t *seq_num,
                                       const uint8_t **payload,
                                       uint32_t *payload_len);

/**
 * @brief 创建HEARTBEAT子消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param writer_id Writer ID
 * @param first_seq 第一个序列号
 * @param last_seq 最后一个序列号
 * @param count 心跳计数
 * @param actual_len 实际长度输出
 * @return ETH_OK成功
 */
eth_status_t dds_eth_create_heartbeat(uint8_t *buffer, uint32_t buf_size,
                                       const uint8_t writer_id[4],
                                       uint64_t first_seq,
                                       uint64_t last_seq,
                                       uint32_t count,
                                       uint32_t *actual_len);

/**
 * @brief 创建ACKNACK子消息
 * @param buffer 输出缓冲区
 * @param buf_size 缓冲区大小
 * @param reader_id Reader ID
 * @param writer_id Writer ID
 * @param bitmap ACK位图
 * @param count ACK计数
 * @param actual_len 实际长度输出
 * @return ETH_OK成功
 */
eth_status_t dds_eth_create_acknack(uint8_t *buffer, uint32_t buf_size,
                                     const uint8_t reader_id[4],
                                     const uint8_t writer_id[4],
                                     uint32_t bitmap,
                                     uint32_t count,
                                     uint32_t *actual_len);

#ifdef __cplusplus
}
#endif

#endif /* DDS_ETH_TRANSPORT_H */
