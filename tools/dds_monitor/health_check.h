/*******************************************************************************
 * @file health_check.h
 * @brief DDS健康检查模块 - 车载场景
 *
 * 功能特点：
 * - 节点健康状态
 * - 主题匹配状态
 * - 连接质量检测
 * - 故障预警
 * - 车载安全审计支持
 ******************************************************************************/

#ifndef DDS_HEALTH_CHECK_H
#define DDS_HEALTH_CHECK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 版本和常量
 *============================================================================*/
#define DDS_HEALTH_VERSION_MAJOR    1
#define DDS_HEALTH_VERSION_MINOR    0
#define DDS_HEALTH_VERSION_PATCH    0

#define DDS_HEALTH_MAX_NODES        256
#define DDS_HEALTH_MAX_TOPICS       512
#define DDS_HEALTH_MAX_CONNECTIONS  1024
#define DDS_HEALTH_MAX_ALARMS       64
#define DDS_HEALTH_CHECK_INTERVAL_MS 1000

/* 车载特定定义 */
#define DDS_HEALTH_ASIL_D_TIMEOUT_MS    50      /* ASIL D级别超时 */
#define DDS_HEALTH_ASIL_B_TIMEOUT_MS    100     /* ASIL B级别超时 */
#define DDS_HEALTH_QM_TIMEOUT_MS        500     /* QM级别超时 */

/*==============================================================================
 * 健康状态定义
 *============================================================================*/
typedef enum {
    DDS_HEALTH_UNKNOWN      = 0,    /* 未知 */
    DDS_HEALTH_HEALTHY      = 1,    /* 健康 */
    DDS_HEALTH_DEGRADED     = 2,    /* 退化 */
    DDS_HEALTH_UNHEALTHY    = 3,    /* 不健康 */
    DDS_HEALTH_CRITICAL     = 4,    /* 危急 */
    DDS_HEALTH_OFFLINE      = 5     /* 离线 */
} dds_health_status_t;

/* 健康状态名称 */
#define DDS_HEALTH_STATUS_NAMES \
    { "UNKNOWN", "HEALTHY", "DEGRADED", "UNHEALTHY", "CRITICAL", "OFFLINE" }

/*==============================================================================
 * 健康检查项类型
 *============================================================================*/
typedef enum {
    DDS_CHECK_NODE_STATUS       = 0x0001,
    DDS_CHECK_TOPIC_MATCH       = 0x0002,
    DDS_CHECK_CONNECTION_QUALITY = 0x0004,
    DDS_CHECK_QOS_COMPLIANCE    = 0x0008,
    DDS_CHECK_SECURITY_STATUS   = 0x0010,
    DDS_CHECK_RESOURCE_USAGE    = 0x0020,
    DDS_CHECK_MEMORY_USAGE      = 0x0040,
    DDS_CHECK_CPU_USAGE         = 0x0080,
    DDS_CHECK_LATENCY_SPIKE     = 0x0100,
    DDS_CHECK_PACKET_LOSS       = 0x0200,
    DDS_CHECK_ALL               = 0xFFFF
} dds_health_check_type_t;

/*==============================================================================
 * 节点健康信息
 *============================================================================*/
typedef struct dds_node_health {
    char                    node_name[64];
    char                    guid[32];
    dds_health_status_t     status;
    uint32_t                status_score;       /* 0-100 */
    
    /* 时间戳 */
    uint64_t                last_seen_time;
    uint64_t                last_heartbeat_time;
    uint64_t                registration_time;
    
    /* 超时检测 */
    uint32_t                timeout_ms;
    uint32_t                missed_heartbeats;
    uint32_t                max_missed_heartbeats;
    
    /* ASIL等级 */
    uint8_t                 asil_level;         /* 0=QM, 1=ASIL A, 2=ASIL B, 3=ASIL C, 4=ASIL D */
    
    /* 资源使用 */
    uint32_t                memory_usage_kb;
    uint32_t                cpu_usage_percent;
    uint32_t                thread_count;
    
    /* 扩展 */
    bool                    is_critical;        /* 关键节点 */
    uint32_t                restart_count;
    char                    version[32];
} dds_node_health_t;

/*==============================================================================
 * 主题匹配状态
 *============================================================================*/
typedef struct dds_topic_match {
    char                    topic_name[128];
    char                    type_name[128];
    
    /* 匹配状态 */
    dds_health_status_t     match_status;
    uint32_t                writer_count;
    uint32_t                reader_count;
    bool                    has_matched;        /* 至少有一个匹配 */
    
    /* QoS兼容性 */
    bool                    qos_compatible;
    uint32_t                qos_mismatches;
    
    /* 时间戳 */
    uint64_t                last_match_time;
    uint64_t                last_unmatch_time;
    
    /* 活动状态 */
    bool                    has_active_data;
    uint64_t                last_data_time;
} dds_topic_match_t;

/*==============================================================================
 * 连接质量信息
 *============================================================================*/
typedef struct dds_connection_quality {
    char                    local_guid[32];
    char                    remote_guid[32];
    char                    topic_name[128];
    
    /* 基本状态 */
    dds_health_status_t     quality_status;
    bool                    is_connected;
    
    /* 质量指标 */
    uint32_t                latency_us;
    uint32_t                jitter_us;
    uint32_t                loss_rate_ppm;
    uint32_t                throughput_kbps;
    
    /* 连接统计 */
    uint64_t                connection_time;
    uint64_t                disconnection_time;
    uint32_t                reconnection_count;
    
    /* 报文统计 */
    uint64_t                sent_packets;
    uint64_t                received_packets;
    uint64_t                lost_packets;
    uint64_t                duplicate_packets;
    
    /* 健康评分 */
    uint32_t                health_score;       /* 0-100 */
    uint32_t                stability_score;    /* 0-100 */
} dds_connection_quality_t;

/*==============================================================================
 * 资源使用状态
 *============================================================================*/
typedef struct dds_resource_usage {
    /* 内存 */
    uint64_t    total_memory_kb;
    uint64_t    used_memory_kb;
    uint64_t    free_memory_kb;
    uint32_t    memory_usage_percent;
    
    /* CPU */
    uint32_t    cpu_usage_percent;
    uint32_t    cpu_count;
    double      load_average[3];
    
    /* 网络 */
    uint64_t    network_rx_bytes;
    uint64_t    network_tx_bytes;
    uint32_t    network_rx_errors;
    uint32_t    network_tx_errors;
    
    /* 文件描述符 */
    uint32_t    open_fds;
    uint32_t    max_fds;
    
    /* 线程 */
    uint32_t    thread_count;
    uint32_t    max_threads;
} dds_resource_usage_t;

/*==============================================================================
 * 故障预警
 *============================================================================*/
typedef enum {
    DDS_ALARM_NONE          = 0,
    DDS_ALARM_WARNING       = 1,
    DDS_ALARM_ERROR         = 2,
    DDS_ALARM_CRITICAL      = 3
} dds_alarm_severity_t;

typedef enum {
    DDS_ALARM_TYPE_NODE_TIMEOUT       = 1,
    DDS_ALARM_TYPE_TOPIC_MISMATCH     = 2,
    DDS_ALARM_TYPE_CONNECTION_LOST    = 3,
    DDS_ALARM_TYPE_QOS_VIOLATION      = 4,
    DDS_ALARM_TYPE_SECURITY_BREACH    = 5,
    DDS_ALARM_TYPE_RESOURCE_EXHAUSTED = 6,
    DDS_ALARM_TYPE_LATENCY_SPIKE      = 7,
    DDS_ALARM_TYPE_PACKET_LOSS_HIGH   = 8,
    DDS_ALARM_TYPE_MEMORY_LEAK        = 9,
    DDS_ALARM_TYPE_CPU_OVERLOAD       = 10
} dds_alarm_type_t;

typedef struct dds_health_alarm {
    uint32_t                alarm_id;
    dds_alarm_type_t        type;
    dds_alarm_severity_t    severity;
    dds_health_status_t     affected_status;
    
    char                    description[256];
    char                    affected_entity[64];
    uint64_t                timestamp;
    
    bool                    is_active;
    uint32_t                occurrence_count;
    uint64_t                last_occurrence;
    
    /* 自动恢复 */
    bool                    auto_recoverable;
    uint64_t                recovery_time;
} dds_health_alarm_t;

/*==============================================================================
 * 综合健康报告
 *============================================================================*/
typedef struct dds_health_report {
    /* 总体状态 */
    dds_health_status_t     overall_status;
    uint32_t                overall_score;
    uint64_t                report_time;
    uint64_t                uptime_seconds;
    
    /* 统计 */
    uint32_t                healthy_nodes;
    uint32_t                degraded_nodes;
    uint32_t                unhealthy_nodes;
    uint32_t                offline_nodes;
    
    uint32_t                matched_topics;
    uint32_t                unmatched_topics;
    
    uint32_t                healthy_connections;
    uint32_t                degraded_connections;
    
    /* 资源 */
    dds_resource_usage_t    resource_usage;
    
    /* 告警 */
    uint32_t                active_alarms;
    uint32_t                warning_count;
    uint32_t                error_count;
    uint32_t                critical_count;
} dds_health_report_t;

/*==============================================================================
 * 健康检查器
 *============================================================================*/
typedef struct dds_health_checker dds_health_checker_t;

/*==============================================================================
 * 配置
 *============================================================================*/
typedef struct dds_health_config {
    /* 检查间隔 */
    uint32_t    check_interval_ms;
    uint32_t    node_timeout_ms;
    
    /* 检查项 */
    uint32_t    check_mask;         /* dds_health_check_type_t 组合 */
    
    /* 阈值 */
    uint32_t    latency_warning_ms;
    uint32_t    latency_critical_ms;
    uint32_t    loss_warning_ppm;
    uint32_t    loss_critical_ppm;
    uint32_t    memory_warning_percent;
    uint32_t    memory_critical_percent;
    uint32_t    cpu_warning_percent;
    uint32_t    cpu_critical_percent;
    
    /* 自动恢复 */
    bool        enable_auto_recovery;
    uint32_t    max_restart_attempts;
    
    /* 安全审计 */
    bool        enable_audit_logging;
} dds_health_config_t;

/*==============================================================================
 * 回调类型
 *============================================================================*/
typedef void (*dds_health_callback_t)(
    const dds_health_report_t* report,
    void* user_data
);

typedef void (*dds_alarm_callback_t)(
    const dds_health_alarm_t* alarm,
    void* user_data
);

typedef void (*dds_node_callback_t)(
    const dds_node_health_t* node,
    dds_health_status_t old_status,
    dds_health_status_t new_status,
    void* user_data
);

/*==============================================================================
 * 生命周期
 *============================================================================*/

/**
 * @brief 初始化健康检查系统
 */
int dds_health_init(const dds_health_config_t* config);

/**
 * @brief 反初始化健康检查系统
 */
void dds_health_deinit(void);

/**
 * @brief 创建健康检查器实例
 */
dds_health_checker_t* dds_health_checker_create(const char* name);

/**
 * @brief 销毁健康检查器
 */
void dds_health_checker_destroy(dds_health_checker_t* checker);

/*==============================================================================
 * 节点管理
 *============================================================================*/

/**
 * @brief 注册节点
 */
int dds_health_register_node(const dds_node_health_t* node_info);

/**
 * @brief 更新节点心跳
 */
int dds_health_update_heartbeat(const char* node_guid);

/**
 * @brief 注销节点
 */
void dds_health_unregister_node(const char* node_guid);

/**
 * @brief 获取节点健康状态
 */
int dds_health_get_node_status(const char* node_guid, dds_node_health_t* status);

/**
 * @brief 获取所有节点状态
 */
int dds_health_get_all_nodes(dds_node_health_t* nodes, uint32_t max_count, uint32_t* actual_count);

/*==============================================================================
 * 主题管理
 *============================================================================*/

/**
 * @brief 注册主题
 */
int dds_health_register_topic(const char* topic_name, const char* type_name);

/**
 * @brief 更新主题匹配状态
 */
int dds_health_update_topic_match(const char* topic_name, bool matched);

/**
 * @brief 获取主题匹配状态
 */
int dds_health_get_topic_match(const char* topic_name, dds_topic_match_t* match);

/**
 * @brief 获取所有主题状态
 */
int dds_health_get_all_topics(dds_topic_match_t* topics, uint32_t max_count, uint32_t* actual_count);

/*==============================================================================
 * 连接质量
 *============================================================================*/

/**
 * @brief 注册连接
 */
int dds_health_register_connection(const char* local_guid, const char* remote_guid,
                                   const char* topic_name);

/**
 * @brief 更新连接质量
 */
int dds_health_update_connection_quality(const char* local_guid, const char* remote_guid,
                                          const dds_connection_quality_t* quality);

/**
 * @brief 获取连接质量
 */
int dds_health_get_connection_quality(const char* local_guid, const char* remote_guid,
                                       dds_connection_quality_t* quality);

/*==============================================================================
 * 健康检查
 *============================================================================*/

/**
 * @brief 执行健康检查
 */
int dds_health_perform_check(dds_health_checker_t* checker);

/**
 * @brief 获取健康报告
 */
void dds_health_get_report(dds_health_report_t* report);

/**
 * @brief 获取最新资源使用状态
 */
void dds_health_get_resource_usage(dds_resource_usage_t* usage);

/*==============================================================================
 * 告警管理
 *============================================================================*/

/**
 * @brief 获取活动告警
 */
int dds_health_get_active_alarms(dds_health_alarm_t* alarms, uint32_t max_count, 
                                  uint32_t* actual_count);

/**
 * @brief 确认告警
 */
void dds_health_acknowledge_alarm(uint32_t alarm_id);

/**
 * @brief 清除告警
 */
void dds_health_clear_alarm(uint32_t alarm_id);

/**
 * @brief 注册告警回调
 */
int dds_health_register_alarm_callback(dds_alarm_callback_t callback, void* user_data);

/**
 * @brief 注销告警回调
 */
void dds_health_unregister_alarm_callback(dds_alarm_callback_t callback);

/*==============================================================================
 * 回调注册
 *============================================================================*/

/**
 * @brief 注册健康状态变化回调
 */
int dds_health_register_status_callback(dds_health_callback_t callback, 
                                         uint32_t interval_ms, void* user_data);

/**
 * @brief 注销健康状态回调
 */
void dds_health_unregister_status_callback(dds_health_callback_t callback);

/**
 * @brief 注册节点状态变化回调
 */
int dds_health_register_node_callback(dds_node_callback_t callback, void* user_data);

/**
 * @brief 注销节点状态回调
 */
void dds_health_unregister_node_callback(dds_node_callback_t callback);

/*==============================================================================
 * 报告生成
 *============================================================================*/

/**
 * @brief 生成文本健康报告
 */
int dds_health_generate_text_report(char* buffer, size_t buffer_size);

/**
 * @brief 生成JSON健康报告
 */
int dds_health_generate_json_report(char* buffer, size_t buffer_size);

/**
 * @brief 生成UDS诊断格式报告
 */
int dds_health_export_uds_format(uint8_t* buffer, uint32_t* size);

/**
 * @brief 生成OTA上报格式
 */
int dds_health_export_ota_format(uint8_t* buffer, uint32_t max_size, uint32_t* actual_size);

/*==============================================================================
 * 高级功能
 *============================================================================*/

/**
 * @brief 启用/禁用特定检查项
 */
void dds_health_enable_check(dds_health_check_type_t check_type, bool enable);

/**
 * @brief 设置节点超时
 */
void dds_health_set_node_timeout(const char* node_guid, uint32_t timeout_ms);

/**
 * @brief 执行故障排查
 */
int dds_health_troubleshoot(const char* entity_guid, char* diagnosis, size_t diagnosis_size);

/**
 * @brief 获取建议的修复操作
 */
int dds_health_get_remediation(const char* entity_guid, char** actions, uint32_t max_actions);

/**
 * @brief 复位特定检查
 */
void dds_health_reset_check(dds_health_check_type_t check_type);

#ifdef __cplusplus
}
#endif

#endif /* DDS_HEALTH_CHECK_H */
