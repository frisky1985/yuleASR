/*******************************************************************************
 * @file qos_monitor.h
 * @brief DDS QoS性能监控模块 - 车载场景
 *
 * 功能特点：
 * - 延迟统计（min/max/avg/p99/p999）
 * - 吞吐量监控
 * - 丢包率统计
 * - 实时图表（文本模式）
 * - 车载低开销设计（<1% CPU）
 ******************************************************************************/

#ifndef DDS_QOS_MONITOR_H
#define DDS_QOS_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 版本和常量
 *============================================================================*/
#define DDS_QOS_MONITOR_VERSION_MAJOR   1
#define DDS_QOS_MONITOR_VERSION_MINOR   0
#define DDS_QOS_MONITOR_VERSION_PATCH   0

#define DDS_QOS_MAX_TOPICS              256
#define DDS_QOS_MAX_ENTITIES            1024
#define DDS_QOS_SAMPLE_WINDOW_SIZE      10000   /* 采样窗口大小 */
#define DDS_QOS_PERCENTILE_BUCKETS      1000    /* 百分位桶数 */
#define DDS_QOS_GRAPH_WIDTH             80      /* 图表宽度 */
#define DDS_QOS_GRAPH_HEIGHT            20      /* 图表高度 */

/* 车载特定阈值 */
#define DDS_QOS_LATENCY_WARNING_US      1000    /* 1ms警告阈值 */
#define DDS_QOS_LATENCY_CRITICAL_US     5000    /* 5ms严重阈值 */
#define DDS_QOS_JITTER_WARNING_US       500     /* 500us警告 */
#define DDS_QOS_LOSS_WARNING_PPM        1000    /* 0.1% 警告 */
#define DDS_QOS_LOSS_CRITICAL_PPM       10000   /* 1% 严重 */

/*==============================================================================
 * QoS指标类型
 *============================================================================*/
typedef enum {
    DDS_QOS_METRIC_LATENCY      = 0x01,     /* 延迟 */
    DDS_QOS_METRIC_THROUGHPUT   = 0x02,     /* 吞吐量 */
    DDS_QOS_METRIC_LOSS_RATE    = 0x04,     /* 丢包率 */
    DDS_QOS_METRIC_JITTER       = 0x08,     /* 抖动 */
    DDS_QOS_METRIC_AVAILABILITY = 0x10,     /* 可用性 */
    DDS_QOS_METRIC_ALL          = 0xFF
} dds_qos_metric_type_t;

/*==============================================================================
 * 延迟统计结构
 *============================================================================*/
typedef struct dds_latency_stats {
    /* 基础统计 */
    uint64_t    sample_count;
    uint64_t    total_samples;
    
    /* 延迟值 (微秒) */
    uint64_t    min_us;
    uint64_t    max_us;
    double      avg_us;
    double      std_dev_us;
    
    /* 百分位数 */
    uint64_t    p50_us;         /* median */
    uint64_t    p90_us;
    uint64_t    p95_us;
    uint64_t    p99_us;
    uint64_t    p999_us;
    uint64_t    p9999_us;
    
    /* 历史记录 */
    uint64_t    history_min_us;
    uint64_t    history_max_us;
    double      history_avg_us;
    
    /* 时间戳 */
    uint64_t    first_sample_time;
    uint64_t    last_sample_time;
} dds_latency_stats_t;

/*==============================================================================
 * 吞吐量统计结构
 *============================================================================*/
typedef struct dds_throughput_stats {
    /* 基础统计 */
    uint64_t    total_messages;
    uint64_t    total_bytes;
    
    /* 当前速率 */
    double      current_msg_per_sec;
    double      current_bytes_per_sec;
    double      current_mbps;
    
    /* 峰值速率 */
    double      peak_msg_per_sec;
    double      peak_bytes_per_sec;
    double      peak_mbps;
    
    /* 平均速率 */
    double      avg_msg_per_sec;
    double      avg_bytes_per_sec;
    
    /* 时间 */
    uint64_t    measurement_interval_us;
    uint64_t    total_duration_us;
} dds_throughput_stats_t;

/*==============================================================================
 * 丢包率统计结构
 *============================================================================*/
typedef struct dds_loss_stats {
    /* 基础统计 */
    uint64_t    expected_sequences;
    uint64_t    received_sequences;
    uint64_t    lost_sequences;
    uint64_t    duplicate_sequences;
    uint64_t    out_of_order_sequences;
    
    /* 丢包率 (parts per million) */
    uint32_t    loss_rate_ppm;          /* 丢包率 */
    uint32_t    duplicate_rate_ppm;     /* 重复率 */
    uint32_t    ooo_rate_ppm;           /* 乱序率 */
    
    /* 连续丢包 */
    uint32_t    consecutive_losses;
    uint32_t    max_consecutive_losses;
    
    /* 连续接收 */
    uint32_t    consecutive_received;
    uint32_t    max_consecutive_received;
    
    /* 连接状态 */
    bool        connection_healthy;
    uint32_t    health_score;           /* 0-100 */
} dds_loss_stats_t;

/*==============================================================================
 * 抖动统计结构
 *============================================================================*/
typedef struct dds_jitter_stats {
    /* 基础统计 */
    uint64_t    sample_count;
    
    /* 抖动值 (微秒) */
    uint64_t    min_jitter_us;
    uint64_t    max_jitter_us;
    double      avg_jitter_us;
    double      current_jitter_us;
    
    /* 变化率 */
    double      jitter_variance;
    double      jitter_std_dev;
    
    /* 连续稳定时间 */
    uint64_t    stable_duration_us;
    
    /* 是否稳定 */
    bool        is_stable;
    uint32_t    stability_score;        /* 0-100 */
} dds_jitter_stats_t;

/*==============================================================================
 * 可用性统计结构
 *============================================================================*/
typedef struct dds_availability_stats {
    /* 运行时间 */
    uint64_t    total_uptime_us;
    uint64_t    total_downtime_us;
    uint64_t    last_status_change_time;
    
    /* 可用性百分比 */
    double      availability_percent;
    
    /* 状态转换次数 */
    uint32_t    up_to_down_count;
    uint32_t    down_to_up_count;
    
    /* 当前状态 */
    bool        is_available;
    
    /* MTBF/MTTR */
    double      mtbf_us;                /* 平均故障间隔 */
    double      mttr_us;                /* 平均恢复时间 */
} dds_availability_stats_t;

/*==============================================================================
 * 综合QoS状态
 *============================================================================*/
typedef struct dds_qos_status {
    /* 各项统计 */
    dds_latency_stats_t         latency;
    dds_throughput_stats_t      throughput;
    dds_loss_stats_t            loss;
    dds_jitter_stats_t          jitter;
    dds_availability_stats_t    availability;
    
    /* 综合评分 */
    uint32_t    overall_score;          /* 0-100 */
    uint32_t    latency_score;
    uint32_t    throughput_score;
    uint32_t    reliability_score;
    
    /* 警告状态 */
    bool        has_warnings;
    bool        has_errors;
    bool        has_critical;
    
    /* 警告计数 */
    uint32_t    warning_count;
    uint32_t    error_count;
    uint32_t    critical_count;
} dds_qos_status_t;

/*==============================================================================
 * 监控实体
 *============================================================================*/
typedef struct dds_qos_monitor dds_qos_monitor_t;

/* 监控对象类型 */
typedef enum {
    DDS_QOS_OBJ_TOPIC       = 0,
    DDS_QOS_OBJ_WRITER      = 1,
    DDS_QOS_OBJ_READER      = 2,
    DDS_QOS_OBJ_PARTICIPANT = 3,
    DDS_QOS_OBJ_CONNECTION  = 4
} dds_qos_obj_type_t;

/* 监控对象信息 */
typedef struct dds_qos_obj_info {
    dds_qos_obj_type_t  type;
    char                name[64];
    char                guid[32];
    char                topic_name[128];
} dds_qos_obj_info_t;

/*==============================================================================
 * 图表配置
 *============================================================================*/
typedef enum {
    DDS_GRAPH_TYPE_LINE,        /* 折线图 */
    DDS_GRAPH_TYPE_BAR,         /* 柱状图 */
    DDS_GRAPH_TYPE_HISTOGRAM,   /* 直方图 */
    DDS_GRAPH_TYPE_HEATMAP      /* 热力图 */
} dds_graph_type_t;

typedef struct dds_graph_config {
    dds_graph_type_t    type;
    uint32_t            width;
    uint32_t            height;
    bool                show_legend;
    bool                show_labels;
    bool                auto_scale;
    double              y_min;
    double              y_max;
    char                title[64];
    char                x_label[32];
    char                y_label[32];
} dds_graph_config_t;

/*==============================================================================
 * 监控配置
 *============================================================================*/
typedef struct dds_qos_config {
    /* 采样配置 */
    uint32_t            sample_window_size;
    uint64_t            sampling_interval_us;
    uint64_t            report_interval_us;
    
    /* 阈值配置 */
    uint64_t            latency_warning_us;
    uint64_t            latency_critical_us;
    uint32_t            loss_warning_ppm;
    uint32_t            loss_critical_ppm;
    uint64_t            jitter_warning_us;
    
    /* 实时图表 */
    bool                enable_live_graph;
    dds_graph_config_t  graph_config;
    
    /* 车载配置 */
    bool                enable_low_overhead;
    uint32_t            cpu_limit_percent;
    bool                enable_hv_integrity;
} dds_qos_config_t;

/*==============================================================================
 * 回调函数类型
 *============================================================================*/
typedef void (*dds_qos_callback_t)(
    const dds_qos_status_t* status,
    void* user_data
);

typedef void (*dds_qos_alert_callback_t)(
    dds_qos_metric_type_t metric,
    uint32_t severity,          /* 1=warning, 2=error, 3=critical */
    const char* description,
    void* user_data
);

/*==============================================================================
 * 生命周期
 *============================================================================*/

/**
 * @brief 初始化QoS监控系统
 */
int dds_qos_monitor_init(const dds_qos_config_t* config);

/**
 * @brief 反初始化QoS监控系统
 */
void dds_qos_monitor_deinit(void);

/**
 * @brief 创建监控实例
 */
dds_qos_monitor_t* dds_qos_monitor_create(const dds_qos_obj_info_t* info);

/**
 * @brief 销毁监控实例
 */
void dds_qos_monitor_destroy(dds_qos_monitor_t* monitor);

/*==============================================================================
 * 数据采集
 *============================================================================*/

/**
 * @brief 记录延迟样本
 * @param latency_us 延迟微秒数
 */
void dds_qos_record_latency(dds_qos_monitor_t* monitor, uint64_t latency_us);

/**
 * @brief 记录消息发送
 */
void dds_qos_record_send(dds_qos_monitor_t* monitor, uint64_t timestamp_us, uint32_t size);

/**
 * @brief 记录消息接收
 * @param sequence_num 序列号（用于丢包检测）
 */
void dds_qos_record_receive(dds_qos_monitor_t* monitor, uint64_t timestamp_us, 
                            uint32_t size, uint64_t sequence_num);

/**
 * @brief 记录连接状态变化
 */
void dds_qos_record_connection_state(dds_qos_monitor_t* monitor, bool connected);

/*==============================================================================
 * 状态查询
 *============================================================================*/

/**
 * @brief 获取当前QoS状态
 */
void dds_qos_get_status(dds_qos_monitor_t* monitor, dds_qos_status_t* status);

/**
 * @brief 获取延迟统计
 */
void dds_qos_get_latency_stats(dds_qos_monitor_t* monitor, dds_latency_stats_t* stats);

/**
 * @brief 获取吞吐量统计
 */
void dds_qos_get_throughput_stats(dds_qos_monitor_t* monitor, dds_throughput_stats_t* stats);

/**
 * @brief 获取丢包率统计
 */
void dds_qos_get_loss_stats(dds_qos_monitor_t* monitor, dds_loss_stats_t* stats);

/*==============================================================================
 * 回调注册
 *============================================================================*/

/**
 * @brief 注册状态回调
 */
int dds_qos_register_callback(dds_qos_callback_t callback, uint64_t interval_ms, void* user_data);

/**
 * @brief 注销状态回调
 */
void dds_qos_unregister_callback(dds_qos_callback_t callback);

/**
 * @brief 注册警告回调
 */
int dds_qos_register_alert_callback(dds_qos_alert_callback_t callback, void* user_data);

/**
 * @brief 注销警告回调
 */
void dds_qos_unregister_alert_callback(dds_qos_alert_callback_t callback);

/*==============================================================================
 * 图表生成
 *============================================================================*/

/**
 * @brief 生成文本图表
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 生成的字符数
 */
int dds_qos_generate_graph(dds_qos_monitor_t* monitor,
                           dds_qos_metric_type_t metric,
                           const dds_graph_config_t* config,
                           char* buffer, size_t buffer_size);

/**
 * @brief 生成多指标对比图
 */
int dds_qos_generate_multi_graph(dds_qos_monitor_t** monitors,
                                 uint32_t monitor_count,
                                 dds_qos_metric_type_t metric,
                                 char* buffer, size_t buffer_size);

/**
 * @brief 生成直方图
 */
int dds_qos_generate_histogram(dds_qos_monitor_t* monitor,
                               dds_qos_metric_type_t metric,
                               char* buffer, size_t buffer_size);

/*==============================================================================
 * 报告生成
 *============================================================================*/

/**
 * @brief 生成文本报告
 */
int dds_qos_generate_text_report(dds_qos_monitor_t* monitor, char* buffer, size_t buffer_size);

/**
 * @brief 生成JSON报告
 */
int dds_qos_generate_json_report(dds_qos_monitor_t* monitor, char* buffer, size_t buffer_size);

/**
 * @brief 生成CSV数据
 */
int dds_qos_export_csv(dds_qos_monitor_t* monitor, const char* filename);

/**
 * @brief 导出UDS诊断格式
 */
int dds_qos_export_uds_format(dds_qos_monitor_t* monitor, uint8_t* buffer, uint32_t* size);

/*==============================================================================
 * 高级功能
 *============================================================================*/

/**
 * @brief 设置性能预算
 * @param latency_budget_us 延迟预算
 * @param throughput_budget_mbps 带宽预算
 */
void dds_qos_set_budgets(dds_qos_monitor_t* monitor, 
                         uint64_t latency_budget_us,
                         double throughput_budget_mbps);

/**
 * @brief 检查性能预算毅行情况
 * @return 违约次数
 */
uint32_t dds_qos_check_budget_violations(dds_qos_monitor_t* monitor);

/**
 * @brief 获取推荐的QoS调整
 */
int dds_qos_get_recommendations(dds_qos_monitor_t* monitor, char* buffer, size_t buffer_size);

/**
 * @brief 重置所有统计
 */
void dds_qos_reset_stats(dds_qos_monitor_t* monitor);

/**
 * @brief 获取监控器列表
 */
int dds_qos_get_all_monitors(dds_qos_monitor_t** monitors, uint32_t max_count, uint32_t* actual_count);

#ifdef __cplusplus
}
#endif

#endif /* DDS_QOS_MONITOR_H */
