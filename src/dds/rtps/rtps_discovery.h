/**
 * @file rtps_discovery.h
 * @brief RTPS发现协议 - 参与者发现与端点发现
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现RTPS 2.x规范中的简单发现协议(SDP)
 * - 参与者发现协议(PDP)
 * - 端点发现协议(EDP)
 * 支持车载场景的确定性发现（固定时间窗口<100ms）
 */

#ifndef RTPS_DISCOVERY_H
#define RTPS_DISCOVERY_H

#include "../../../common/types/eth_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RTPS发现协议常量定义
 * ============================================================================ */

/** RTPS协议标识 */
#define RTPS_PROTOCOL_NAME      "RTPS"
#define RTPS_PROTOCOL_VERSION_MAJOR 2
#define RTPS_PROTOCOL_VERSION_MINOR 4

/** 发现端口计算 */
#define RTPS_DISCOVERY_DOMAIN_ID_DEFAULT    0
#define RTPS_DISCOVERY_PORT_BASE            7400
#define RTPS_DISCOVERY_PORT_DOMAIN_GAIN     250
#define RTPS_DISCOVERY_PORT_PARTICIPANT_GAIN 2

/** 发现协议超时配置（车载场景优化） */
#define RTPS_DISCOVERY_ANNOUNCE_PERIOD_MS   20      /* 发现公告周期 */
#define RTPS_DISCOVERY_INITIAL_DELAY_MS     10      /* 初始发现延迟 */
#define RTPS_DISCOVERY_MATCH_TIMEOUT_MS     100     /* 匹配超时（<100ms快速启动） */
#define RTPS_DISCOVERY_LEASE_DURATION_MS    5000    /* 租约持续时间 */
#define RTPS_DISCOVERY_RELIABLE_PERIOD_MS   50      /* 可靠发现周期 */

/** GUID相关常量 */
#define RTPS_GUID_PREFIX_SIZE   12
#define RTPS_ENTITY_ID_SIZE     4
#define RTPS_GUID_SIZE          16

/** 序列号常量 */
#define RTPS_SEQNUM_UNKNOWN     ((int64_t)-1)
#define RTPS_SEQNUM_INITIAL     1

/* ============================================================================
 * RTPS发现数据类型定义
 * ============================================================================ */

/** GUID前缀 */
typedef uint8_t rtps_guid_prefix_t[RTPS_GUID_PREFIX_SIZE];

/** Entity ID */
typedef uint8_t rtps_entity_id_t[RTPS_ENTITY_ID_SIZE];

/** 完整GUID */
typedef struct {
    rtps_guid_prefix_t prefix;
    rtps_entity_id_t entity_id;
} rtps_guid_t;

/** 序列号 */
typedef struct {
    int32_t high;
    uint32_t low;
} rtps_sequence_number_t;

/** 发现协议类型 */
typedef enum {
    RTPS_DISCOVERY_TYPE_SIMPLE = 0,     /* 简单发现协议 */
    RTPS_DISCOVERY_TYPE_STATIC,         /* 静态发现 */
    RTPS_DISCOVERY_TYPE_AUTOSAR,        /* AUTOSAR扩展发现 */
} rtps_discovery_type_t;

/** 发现协议状态 */
typedef enum {
    RTPS_DISCOVERY_STATE_INIT = 0,
    RTPS_DISCOVERY_STATE_ANNOUNCING,    /* 正在公告 */
    RTPS_DISCOVERY_STATE_DISCOVERING,   /* 发现中 */
    RTPS_DISCOVERY_STATE_MATCHED,       /* 已匹配 */
    RTPS_DISCOVERY_STATE_FAILED,
} rtps_discovery_state_t;

/** 端点类型 */
typedef enum {
    RTPS_ENDPOINT_TYPE_UNKNOWN = 0,
    RTPS_ENDPOINT_TYPE_WRITER,          /* DataWriter */
    RTPS_ENDPOINT_TYPE_READER,          /* DataReader */
} rtps_endpoint_type_t;

/** 传输优先级（AUTOSAR扩展） */
typedef enum {
    RTPS_PRIORITY_BACKGROUND = 0,
    RTPS_PRIORITY_BELOW_NORMAL = 1,
    RTPS_PRIORITY_NORMAL = 2,
    RTPS_PRIORITY_ABOVE_NORMAL = 3,
    RTPS_PRIORITY_HIGH = 4,
    RTPS_PRIORITY_REALTIME = 5,
} rtps_transport_priority_t;

/* ============================================================================
 * 参与者发现协议(PDP)数据结构
 * ============================================================================ */

/** 参与者代理数据 (SPDPdiscoveredParticipantData) */
typedef struct rtps_participant_proxy {
    rtps_guid_t guid;                           /* 参与者GUID */
    rtps_guid_prefix_t guid_prefix;
    uint32_t protocol_version;                  /* 协议版本 */
    uint32_t vendor_id;                         /* 厂商ID */
    bool expects_inline_qos;                    /* 是否期望内联QoS */
    uint8_t metatraffic_unicast_locator_count;  /* 元数据单播定位器数量 */
    uint8_t metatraffic_multicast_locator_count;/* 元数据多播定位器数量 */
    uint8_t default_unicast_locator_count;      /* 默认单播定位器数量 */
    uint8_t default_multicast_locator_count;    /* 默认多播定位器数量 */
    uint32_t available_builtin_endpoints;       /* 可用内置端点 */
    uint32_t lease_duration_ms;                 /* 租约持续时间 */
    uint32_t manual_liveliness_count;           /* 手动活跃计数 */
    uint32_t builtin_endpoint_qos;              /* 内置端点QoS */
    
    /* AUTOSAR扩展字段 */
    bool autosar_deterministic;                 /* 确定性发现 */
    uint32_t autosar_startup_time_ms;           /* 启动时间预算 */
    rtps_transport_priority_t priority;         /* 传输优先级 */
    
    uint64_t last_seen_timestamp;               /* 最后活跃时间 */
    rtps_discovery_state_t state;               /* 发现状态 */
    struct rtps_participant_proxy *next;        /* 链表指针 */
} rtps_participant_proxy_t;

/* ============================================================================
 * 端点发现协议(EDP)数据结构
 * ============================================================================ */

/** 端点代理数据 */
typedef struct rtps_endpoint_proxy {
    rtps_guid_t guid;                           /* 端点GUID */
    rtps_guid_t participant_guid;               /* 所属参与者GUID */
    rtps_guid_t topic_guid;                     /* 主题GUID */
    rtps_endpoint_type_t type;                  /* 端点类型 */
    
    char topic_name[128];                       /* 主题名称 */
    char type_name[128];                        /* 类型名称 */
    
    uint8_t unicast_locator_count;
    uint8_t multicast_locator_count;
    
    /* QoS策略 */
    uint32_t reliability_kind;                  /* 可靠性类型 */
    uint32_t durability_kind;                   /* 持久性类型 */
    uint32_t deadline_ms;                       /* 截止期限 */
    uint32_t latency_budget_ms;                 /* 延迟预算 */
    uint32_t ownership_strength;                /* 所有权强度 */
    
    /* 协议状态 */
    rtps_sequence_number_t next_seq_number;     /* 下一个序列号 */
    rtps_sequence_number_t max_seq_number;      /* 最大序列号 */
    
    /* AUTOSAR扩展 */
    rtps_transport_priority_t transport_priority;
    bool critical_data;                         /* 关键数据标记 */
    uint32_t activation_time_us;                /* 激活时间（微秒） */
    
    rtps_discovery_state_t state;
    struct rtps_endpoint_proxy *next;
} rtps_endpoint_proxy_t;

/* ============================================================================
 * 发现协议配置与上下文
 * ============================================================================ */

/** 发现协议配置 */
typedef struct {
    rtps_discovery_type_t type;                 /* 发现类型 */
    uint32_t domain_id;                         /* 域ID */
    rtps_guid_prefix_t guid_prefix;             /* GUID前缀 */
    
    /* 时序配置 */
    uint32_t initial_announce_period_ms;
    uint32_t announce_period_ms;
    uint32_t lease_duration_ms;
    uint32_t match_timeout_ms;
    
    /* 参与者配置 */
    uint8_t max_participants;
    uint8_t max_endpoints_per_participant;
    
    /* AUTOSAR扩展配置 */
    bool autosar_mode;                          /* AUTOSAR模式 */
    uint32_t deterministic_window_ms;           /* 确定性窗口 */
    bool enable_fast_startup;                   /* 快速启动 */
} rtps_discovery_config_t;

/** 发现协议上下文 */
typedef struct {
    rtps_discovery_config_t config;
    rtps_participant_proxy_t *participants;     /* 参与者链表 */
    rtps_endpoint_proxy_t *endpoints;           /* 端点链表 */
    
    /* 协议状态 */
    bool initialized;
    bool active;
    uint64_t start_timestamp;
    uint32_t announcement_count;
    
    /* 统计信息 */
    uint32_t participants_discovered;
    uint32_t endpoints_matched;
    uint32_t discovery_failures;
    
    /* 回调函数 */
    void (*on_participant_discovered)(rtps_participant_proxy_t *participant);
    void (*on_endpoint_matched)(rtps_endpoint_proxy_t *endpoint, bool matched);
    void (*on_discovery_complete)(void);
} rtps_discovery_context_t;

/* ============================================================================
 * 发现协议API
 * ============================================================================ */

/**
 * @brief 初始化发现协议
 * @param ctx 发现协议上下文
 * @param config 发现协议配置
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_init(rtps_discovery_context_t *ctx, 
                                  const rtps_discovery_config_t *config);

/**
 * @brief 反初始化发现协议
 * @param ctx 发现协议上下文
 */
void rtps_discovery_deinit(rtps_discovery_context_t *ctx);

/**
 * @brief 启动发现协议
 * @param ctx 发现协议上下文
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_start(rtps_discovery_context_t *ctx);

/**
 * @brief 停止发现协议
 * @param ctx 发现协议上下文
 */
void rtps_discovery_stop(rtps_discovery_context_t *ctx);

/**
 * @brief 处理发现周期（定期调用）
 * @param ctx 发现协议上下文
 * @param current_time_ms 当前时间戳
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_process(rtps_discovery_context_t *ctx, 
                                     uint64_t current_time_ms);

/**
 * @brief 处理收到的发现报文
 * @param ctx 发现协议上下文
 * @param data 报文数据
 * @param len 数据长度
 * @param source_addr 源地址
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_handle_packet(rtps_discovery_context_t *ctx,
                                           const uint8_t *data,
                                           uint32_t len,
                                           const void *source_addr);

/**
 * @brief 创建参与者发现数据报文
 * @param ctx 发现协议上下文
 * @param buffer 输出缓冲区
 * @param max_len 缓冲区最大长度
 * @param actual_len 实际生成长度
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_create_participant_data(rtps_discovery_context_t *ctx,
                                                      uint8_t *buffer,
                                                      uint32_t max_len,
                                                      uint32_t *actual_len);

/**
 * @brief 创建端点发现数据报文
 * @param ctx 发现协议上下文
 * @param endpoint 端点代理
 * @param buffer 输出缓冲区
 * @param max_len 缓冲区最大长度
 * @param actual_len 实际生成长度
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_create_endpoint_data(rtps_discovery_context_t *ctx,
                                                   rtps_endpoint_proxy_t *endpoint,
                                                   uint8_t *buffer,
                                                   uint32_t max_len,
                                                   uint32_t *actual_len);

/**
 * @brief 添加本地端点到发现协议
 * @param ctx 发现协议上下文
 * @param endpoint 端点代理
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_add_local_endpoint(rtps_discovery_context_t *ctx,
                                                 rtps_endpoint_proxy_t *endpoint);

/**
 * @brief 移除本地端点
 * @param ctx 发现协议上下文
 * @param guid 端点GUID
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_remove_local_endpoint(rtps_discovery_context_t *ctx,
                                                    const rtps_guid_t *guid);

/**
 * @brief 查找参与者代理
 * @param ctx 发现协议上下文
 * @param guid_prefix GUID前缀
 * @return 参与者代理指针，未找到返回NULL
 */
rtps_participant_proxy_t* rtps_discovery_find_participant(
    rtps_discovery_context_t *ctx,
    const rtps_guid_prefix_t guid_prefix);

/**
 * @brief 查找端点代理
 * @param ctx 发现协议上下文
 * @param guid 端点GUID
 * @return 端点代理指针，未找到返回NULL
 */
rtps_endpoint_proxy_t* rtps_discovery_find_endpoint(
    rtps_discovery_context_t *ctx,
    const rtps_guid_t *guid);

/**
 * @brief 检查发现是否完成（所有端点已匹配）
 * @param ctx 发现协议上下文
 * @return true完成
 */
bool rtps_discovery_is_complete(rtps_discovery_context_t *ctx);

/**
 * @brief 获取发现统计信息
 * @param ctx 发现协议上下文
 * @param participants_discovered 输出发现的参与者数量
 * @param endpoints_matched 输出匹配的端点数量
 * @param time_elapsed_ms 输出经过时间
 * @return ETH_OK成功
 */
eth_status_t rtps_discovery_get_stats(rtps_discovery_context_t *ctx,
                                        uint32_t *participants_discovered,
                                        uint32_t *endpoints_matched,
                                        uint32_t *time_elapsed_ms);

/**
 * @brief 设置发现回调
 * @param ctx 发现协议上下文
 * @param on_participant_discovered 参与者发现回调
 * @param on_endpoint_matched 端点匹配回调
 * @param on_discovery_complete 发现完成回调
 */
void rtps_discovery_set_callbacks(rtps_discovery_context_t *ctx,
                                   void (*on_participant_discovered)(rtps_participant_proxy_t *),
                                   void (*on_endpoint_matched)(rtps_endpoint_proxy_t *, bool),
                                   void (*on_discovery_complete)(void));

/**
 * @brief 获取计算后的发现端口
 * @param domain_id 域ID
 * @param participant_id 参与者ID
 * @param is_metatraffic 是否为元数据流量
 * @return 端口号
 */
uint16_t rtps_discovery_get_port(uint32_t domain_id, 
                                  uint32_t participant_id,
                                  bool is_metatraffic);

/**
 * @brief 生成GUID前缀（基于MAC地址和时间戳）
 * @param prefix 输出GUID前缀
 * @param mac_addr MAC地址
 * @param domain_id 域ID
 * @return ETH_OK成功
 */
eth_status_t rtps_guid_prefix_generate(rtps_guid_prefix_t prefix,
                                        const eth_mac_addr_t mac_addr,
                                        uint32_t domain_id);

/**
 * @brief 比较两个GUID
 * @param guid1 GUID1
 * @param guid2 GUID2
 * @return true相等
 */
bool rtps_guid_equal(const rtps_guid_t *guid1, const rtps_guid_t *guid2);

/**
 * @brief 比较序列号
 * @param seq1 序列号1
 * @param seq2 序列号2
 * @return <0: seq1<seq2, 0:相等, >0: seq1>seq2
 */
int rtps_seqnum_compare(const rtps_sequence_number_t *seq1,
                         const rtps_sequence_number_t *seq2);

/**
 * @brief 递增序列号
 * @param seq 序列号
 */
void rtps_seqnum_increment(rtps_sequence_number_t *seq);

#ifdef __cplusplus
}
#endif

#endif /* RTPS_DISCOVERY_H */
