/**
 * @file transport_manager.h
 * @brief 传输层管理器 - 多传输层协调与路由决策
 * @version 1.0
 * @date 2026-04-24
 * 
 * @note 自动选择最优传输路径
 * @note 适配车载分布式架构
 */

#ifndef TRANSPORT_MANAGER_H
#define TRANSPORT_MANAGER_H

#include "transport_interface.h"
#include "udp_transport.h"
#include "shm_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 管理器配置定义
 * ============================================================================ */

/** 最大传输层数量 */
#define TM_MAX_TRANSPORTS       8
#define TM_MAX_ENDPOINTS        64
#define TM_MAX_ROUTES           128

/** 路由决策策略 */
typedef enum {
    TM_ROUTE_AUTO = 0,        /* 自动选择 */
    TM_ROUTE_PREFER_SHM,      /* 优先共享内存 */
    TM_ROUTE_PREFER_UDP,      /* 优先UDP */
    TM_ROUTE_DETERMINISTIC,   /* 确定性路由 */
    TM_ROUTE_LOW_LATENCY,     /* 低延迟 */
    TM_ROUTE_HIGH_THROUGHPUT, /* 高吞吐量 */
    TM_ROUTE_REDUNDANT        /* 冗余传输 */
} tm_routing_policy_t;

/** 端点类型 */
typedef enum {
    TM_EP_UNKNOWN = 0,
    TM_EP_LOCAL_PROCESS,      /* 本地进程 */
    TM_EP_LOCAL_NODE,         /* 本地节点 */
    TM_EP_REMOTE_NODE,        /* 远程节点 */
    TM_EP_MULTICAST_GROUP     /* 组播组 */
} tm_endpoint_type_t;

/** 传输层状态 */
typedef enum {
    TM_STATE_INIT = 0,
    TM_STATE_ACTIVE,
    TM_STATE_DEGRADED,
    TM_STATE_FAILED,
    TM_STATE_SHUTDOWN
} tm_transport_state_t;

/* ============================================================================
 * 车载特定配置
 * ============================================================================ */

/** 时间感知路由 */
typedef struct {
    bool enable_time_aware;       /* 启用时间感知 */
    uint32_t gcl_cycle_us;        /* 门控列表周期 */
    uint8_t time_slots[8];        /* 时间隙配置 */
    uint32_t slot_duration_us[8]; /* 时间隙时长 */
} tm_time_aware_config_t;

/** 网络拓扑 */
typedef struct {
    uint32_t node_id;             /* 本地节点ID */
    uint32_t domain_id;           /* DDS域ID */
    uint8_t ecu_id[6];            /* ECU标识 */
    char node_name[32];           /* 节点名称 */
} tm_node_info_t;

/* ============================================================================
 * 核心数据结构
 * ============================================================================ */

/** 传输层入口 */
typedef struct {
    uint32_t id;                        /* 传输层ID */
    transport_type_t type;              /* 传输类型 */
    tm_transport_state_t state;         /* 当前状态 */
    transport_handle_t *handle;         /* 传输句柄 */
    void *config;                       /* 传输配置 */
    uint32_t priority;                  /* 优先级 */
    uint32_t latency_us;                /* 测量延迟 */
    uint32_t throughput_mbps;           /* 测量吞吐量 */
    uint64_t last_used;                 /* 最后使用时间 */
    uint32_t error_count;               /* 错误计数 */
} tm_transport_entry_t;

/** 路由条目 */
typedef struct {
    uint32_t route_id;                  /* 路由ID */
    uint32_t src_endpoint;              /* 源端点ID */
    uint32_t dst_endpoint;              /* 目标端点ID */
    transport_type_t preferred_type;    /* 优先传输类型 */
    uint32_t primary_transport;         /* 主传输层ID */
    uint32_t backup_transport;          /* 备份传输层ID */
    bool enable_failover;               /* 启用故障转移 */
    uint32_t qos_profile;               /* QoS配置文件ID */
} tm_route_entry_t;

/** 端点信息 */
typedef struct {
    uint32_t endpoint_id;               /* 端点ID */
    tm_endpoint_type_t type;            /* 端点类型 */
    uint32_t node_id;                   /* 所属节点ID */
    eth_ip_addr_t ip_addr;              /* IP地址(远程端点) */
    char shm_name[64];                  /* 共享内存名(本地端点) */
    uint16_t port;                      /* 端口号 */
    bool is_local;                      /* 是否本地端点 */
    uint64_t last_seen;                 /* 最后活跃时间 */
} tm_endpoint_entry_t;

/* ============================================================================
 * 管理器配置
 * ============================================================================ */

/** 传输管理器配置 */
typedef struct {
    /* 路由策略 */
    tm_routing_policy_t routing_policy;
    
    /* 自动选择阈值 */
    uint32_t shm_latency_threshold_us;    /* 共享内存延迟阈值 */
    uint32_t udp_latency_threshold_us;    /* UDP延迟阈值 */
    uint32_t shm_size_threshold;          /* 共享内存大小阈值 */
    
    /* 故障检测 */
    uint32_t health_check_interval_ms;    /* 健康检查间隔 */
    uint32_t heartbeat_timeout_ms;        /* 心跳超时 */
    uint32_t failover_threshold;          /* 故障转移阈值 */
    
    /* TSN配置 */
    tm_time_aware_config_t time_aware;
    
    /* 车载网络 */
    tm_node_info_t node_info;
    
    /* 线程配置 */
    uint8_t manager_cpu_affinity;
    uint32_t manager_stack_size;
    uint32_t manager_priority;
    
} tm_config_t;

/* ============================================================================
 * 统计信息
 * ============================================================================ */

/** 管理器统计 */
typedef struct {
    uint64_t total_tx_messages;
    uint64_t total_rx_messages;
    uint64_t total_tx_bytes;
    uint64_t total_rx_bytes;
    uint64_t routing_decisions;
    uint64_t route_failovers;
    uint64_t errors;
    uint32_t active_transports;
    uint32_t active_routes;
    uint32_t active_endpoints;
    uint32_t avg_routing_time_us;
} tm_stats_t;

/* ============================================================================
 * 传输管理器API
 * ============================================================================ */

/**
 * @brief 初始化传输管理器
 * @param config 管理器配置
 * @return ETH_OK成功
 */
eth_status_t tm_init(const tm_config_t *config);

/**
 * @brief 反初始化传输管理器
 * @return ETH_OK成功
 */
eth_status_t tm_deinit(void);

/**
 * @brief 注册传输层
 * @param type 传输类型
 * @param config 传输配置
 * @param transport_id 输出传输层ID
 * @return ETH_OK成功
 */
eth_status_t tm_register_transport(transport_type_t type,
                                    const void *config,
                                    uint32_t *transport_id);

/**
 * @brief 注销传输层
 * @param transport_id 传输层ID
 * @return ETH_OK成功
 */
eth_status_t tm_unregister_transport(uint32_t transport_id);

/**
 * @brief 注册端点
 * @param endpoint 端点信息
 * @param endpoint_id 输出端点ID
 * @return ETH_OK成功
 */
eth_status_t tm_register_endpoint(const tm_endpoint_entry_t *endpoint,
                                   uint32_t *endpoint_id);

/**
 * @brief 注销端点
 * @param endpoint_id 端点ID
 * @return ETH_OK成功
 */
eth_status_t tm_unregister_endpoint(uint32_t endpoint_id);

/**
 * @brief 创建路由
 * @param src_endpoint 源端点ID
 * @param dst_endpoint 目标端点ID
 * @param policy 路由策略
 * @param route_id 输出路由ID
 * @return ETH_OK成功
 */
eth_status_t tm_create_route(uint32_t src_endpoint,
                              uint32_t dst_endpoint,
                              tm_routing_policy_t policy,
                              uint32_t *route_id);

/**
 * @brief 删除路由
 * @param route_id 路由ID
 * @return ETH_OK成功
 */
eth_status_t tm_delete_route(uint32_t route_id);

/**
 * @brief 发送数据(管理器自动路由)
 * @param src_endpoint 源端点ID
 * @param dst_endpoint 目标端点ID
 * @param data 数据指针
 * @param len 数据长度
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t tm_send(uint32_t src_endpoint,
                      uint32_t dst_endpoint,
                      const uint8_t *data,
                      uint32_t len,
                      uint32_t timeout_ms);

/**
 * @brief 接收数据
 * @param endpoint_id 端点ID
 * @param buffer 接收缓冲区
 * @param max_len 最大长度
 * @param received_len 实际接收长度
 * @param timeout_ms 超时时间
 * @return ETH_OK成功
 */
eth_status_t tm_receive(uint32_t endpoint_id,
                         uint8_t *buffer,
                         uint32_t max_len,
                         uint32_t *received_len,
                         uint32_t timeout_ms);

/**
 * @brief 获取推荐传输类型
 * @param src_endpoint 源端点ID
 * @param dst_endpoint 目标端点ID
 * @param recommended_type 输出推荐传输类型
 * @return ETH_OK成功
 */
eth_status_t tm_get_recommended_transport(uint32_t src_endpoint,
                                           uint32_t dst_endpoint,
                                           transport_type_t *recommended_type);

/**
 * @brief 设置路由策略
 * @param policy 新策略
 * @return ETH_OK成功
 */
eth_status_t tm_set_routing_policy(tm_routing_policy_t policy);

/**
 * @brief 获取传输层状态
 * @param transport_id 传输层ID
 * @param state 输出状态
 * @return ETH_OK成功
 */
eth_status_t tm_get_transport_status(uint32_t transport_id,
                                      tm_transport_state_t *state);

/**
 * @brief 执行传输层健康检查
 * @return ETH_OK成功
 */
eth_status_t tm_health_check(void);

/**
 * @brief 强制切换传输路径
 * @param route_id 路由ID
 * @param new_transport_id 新传输层ID
 * @return ETH_OK成功
 */
eth_status_t tm_force_transport_switch(uint32_t route_id,
                                        uint32_t new_transport_id);

/**
 * @brief 获取管理器统计信息
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t tm_get_stats(tm_stats_t *stats);

/**
 * @brief 清除统计信息
 * @return ETH_OK成功
 */
eth_status_t tm_clear_stats(void);

/**
 * @brief 设置车载网络信息
 * @param node_info 节点信息
 * @return ETH_OK成功
 */
eth_status_t tm_set_node_info(const tm_node_info_t *node_info);

/**
 * @brief 更新端点信息
 * @param endpoint_id 端点ID
 * @param endpoint 新端点信息
 * @return ETH_OK成功
 */
eth_status_t tm_update_endpoint(uint32_t endpoint_id,
                                 const tm_endpoint_entry_t *endpoint);

/**
 * @brief 获取路由延迟统计
 * @param route_id 路由ID
 * @param avg_latency_us 平均延迟输出(微秒)
 * @param max_latency_us 最大延迟输出(微秒)
 * @return ETH_OK成功
 */
eth_status_t tm_get_route_latency(uint32_t route_id,
                                   uint32_t *avg_latency_us,
                                   uint32_t *max_latency_us);

#ifdef __cplusplus
}
#endif

#endif /* TRANSPORT_MANAGER_H */
