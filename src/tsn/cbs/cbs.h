/**
 * @file cbs.h
 * @brief 基于信用的调度器 (IEEE 802.1Qav-2009)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note SR Class A (2ms) / SR Class B (50ms)
 * @note 符合ASIL-D安全等级
 */

#ifndef TSN_CBS_H
#define TSN_CBS_H

#include "../../common/types/eth_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * IEEE 802.1Qav 常量定义
 * ============================================================================ */

/** SR类型定义 */
#define CBS_SR_CLASS_A                  0
#define CBS_SR_CLASS_B                  1
#define CBS_SR_CLASS_MAX                2

/** 默认延迟限制 */
#define CBS_CLASS_A_MAX_LATENCY_US      2000    /* 2ms */
#define CBS_CLASS_B_MAX_LATENCY_US      50000   /* 50ms */

/** 默认间隔 */
#define CBS_CLASS_A_INTERVAL_US         125     /* 125us */
#define CBS_CLASS_B_INTERVAL_US         250     /* 250us */

/** 传输选择算法参数 */
#define CBS_TRANSMISSION_OVERRUN_US     100     /* 传输超时 */
#define CBS_IDLE_SLOPE_MAX_PERCENT      75      /* 最大空闲斜率百分比 */

/** 信用单位 */
#define CBS_CREDIT_UNIT_BITS            1       /* 信用以位为单位 */

/** ASIL-D安全相关 */
#define CBS_MAX_NEGATIVE_CREDIT         (-1000000)  /* 最大负信用 */
#define CBS_SAFETY_BANDWIDTH_THRESHOLD  80          /* 带宽阈值百分比 */

/* ============================================================================
 * SR Class 配置
 * ============================================================================ */

/** SR类定义 */
typedef enum {
    CBS_SR_CLASS_NONE = -1,
    CBS_SR_CLASS_A_TYPE = 0,        /* 严格实时 */
    CBS_SR_CLASS_B_TYPE = 1,        /* 实时 */
} cbs_sr_class_type_t;

/** SR Class配置 */
typedef struct {
    cbs_sr_class_type_t class_type;     /* SR类型 */
    uint32_t max_interval_frames;       /* 每间隔最大帧数 */
    uint32_t max_frame_size;            /* 最大帧大小 */
    uint32_t max_latency_us;            /* 最大延迟 (us) */
    uint32_t interval_us;               /* 间隔时间 (us) */
    uint8_t priority;                   /* 优先级 */
    uint16_t vid;                       /* VLAN ID */
} cbs_sr_class_config_t;

/* ============================================================================
 * CBS端口配置
 * ============================================================================ */

/** CBS队列配置 */
typedef struct {
    uint8_t queue_id;                   /* 队列ID */
    cbs_sr_class_type_t sr_class;       /* SR类型 */
    
    /* Idle Slope - 空闲斜率 (bits per second) */
    int64_t idle_slope_bps;
    
    /* Send Slope - 发送斜率 (bits per second) */
    int64_t send_slope_bps;
    
    /* 最小信用值 */
    int64_t low_credit;
    
    /* 最大信用值 */
    int64_t high_credit;
    
    /* 传输超时 */
    uint32_t transmission_overrun_us;
} cbs_queue_config_t;

/** CBS端口配置 */
typedef struct {
    uint16_t port_id;                       /* 端口ID */
    uint32_t port_transmit_rate_bps;        /* 端口传输速率 */
    
    /* SR Class配置 */
    cbs_sr_class_config_t sr_classes[CBS_SR_CLASS_MAX];
    uint32_t sr_class_count;
    
    /* 队列配置 */
    cbs_queue_config_t queue_configs[CBS_SR_CLASS_MAX];
    uint32_t queue_count;
    
    /* 安全相关 */
    bool enable_safety_checks;              /* 启用安全检查 */
    uint32_t bandwidth_alarm_threshold;     /* 带宽告警阈值 */
} cbs_port_config_t;

/* ============================================================================
 * 运行时状态
 * ============================================================================ */

/** CBS队列状态 */
typedef struct {
    int64_t current_credit;             /* 当前信用值 */
    int64_t min_credit_seen;            /* 观察到的最小信用 */
    int64_t max_credit_seen;            /* 观察到的最大信用 */
    
    /* 传输状态 */
    bool transmission_in_progress;      /* 传输进行中 */
    uint64_t transmission_start_time_ns;  /* 传输开始时间 */
    
    /* 统计 */
    uint64_t frames_transmitted;        /* 已发送帧数 */
    uint64_t bytes_transmitted;         /* 已发送字节数 */
    uint64_t credit_violations;         /* 信用违规数 */
    uint32_t transmission_overruns;     /* 传输超时数 */
} cbs_queue_state_t;

/** CBS统计信息 */
typedef struct {
    /* 队列统计 */
    cbs_queue_state_t queue_states[CBS_SR_CLASS_MAX];
    
    /* 端口统计 */
    uint64_t total_frames_transmitted;
    uint64_t total_bytes_transmitted;
    uint64_t total_credit_violations;
    
    /* SR Class延迟统计 */
    uint32_t max_latency_observed_us[CBS_SR_CLASS_MAX];
    uint32_t min_latency_observed_us[CBS_SR_CLASS_MAX];
    uint64_t latency_violations[CBS_SR_CLASS_MAX];
} cbs_stats_t;

/* ============================================================================
 * 安全监控
 * ============================================================================ */

typedef struct {
    /* 带宽监控 */
    uint32_t bandwidth_utilization_percent;   /* 带宽利用率 */
    bool bandwidth_threshold_exceeded;        /* 是否超过阈值 */
    
    /* 信用监控 */
    int64_t min_credit_limit;
    int64_t max_credit_limit;
    bool credit_limit_violated;
    
    /* 延迟监控 */
    bool latency_violated;
    uint32_t latency_violation_count;
    
    /* 故障检测 */
    bool fault_detected;
    uint32_t fault_code;
} cbs_safety_monitor_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 信用更新回调 */
typedef void (*cbs_credit_update_callback_t)(
    uint16_t port_id,
    uint8_t queue_id,
    int64_t credit,
    void *user_data
);

/** 传输许可回调 */
typedef void (*cbs_transmit_permission_callback_t)(
    uint16_t port_id,
    uint8_t queue_id,
    bool permitted,
    int64_t required_credit,
    void *user_data
);

/** 带宽告警回调 */
typedef void (*cbs_bandwidth_alert_callback_t)(
    uint16_t port_id,
    uint32_t bandwidth_percent,
    void *user_data
);

/** 安全告警回调 */
typedef void (*cbs_safety_alert_callback_t)(
    uint16_t port_id,
    uint32_t alert_type,
    const char *alert_msg,
    void *user_data
);

/* ============================================================================
 * 初始化和配置API
 * ============================================================================ */

/**
 * @brief 初始化CBS调度器
 * @return ETH_OK成功
 */
eth_status_t cbs_init(void);

/**
 * @brief 反初始化CBS调度器
 */
void cbs_deinit(void);

/**
 * @brief 配置CBS端口
 * @param port_id 端口ID
 * @param config 端口配置
 * @return ETH_OK成功
 */
eth_status_t cbs_config_port(uint16_t port_id, const cbs_port_config_t *config);

/**
 * @brief 获取CBS端口配置
 * @param port_id 端口ID
 * @param config 配置输出
 * @return ETH_OK成功
 */
eth_status_t cbs_get_port_config(uint16_t port_id, cbs_port_config_t *config);

/**
 * @brief 配置SR Class
 * @param port_id 端口ID
 * @param class_index Class索引
 * @param sr_config SR Class配置
 * @return ETH_OK成功
 */
eth_status_t cbs_config_sr_class(uint16_t port_id, uint8_t class_index,
                                  const cbs_sr_class_config_t *sr_config);

/**
 * @brief 配置CBS队列
 * @param port_id 端口ID
 * @param queue_index 队列索引
 * @param queue_config 队列配置
 * @return ETH_OK成功
 */
eth_status_t cbs_config_queue(uint16_t port_id, uint8_t queue_index,
                               const cbs_queue_config_t *queue_config);

/* ============================================================================
 * 信用管理API
 * ============================================================================ */

/**
 * @brief 更新CBS信用
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param elapsed_time_ns 经过时间 (ns)
 * @return ETH_OK成功
 */
eth_status_t cbs_update_credit(uint16_t port_id, uint8_t queue_id, 
                                uint64_t elapsed_time_ns);

/**
 * @brief 获取当前信用
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param credit 信用值输出
 * @return ETH_OK成功
 */
eth_status_t cbs_get_credit(uint16_t port_id, uint8_t queue_id, int64_t *credit);

/**
 * @brief 设置信用
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param credit 信用值
 * @return ETH_OK成功
 */
eth_status_t cbs_set_credit(uint16_t port_id, uint8_t queue_id, int64_t credit);

/**
 * @brief 恢复信用 (在传输完成后)
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param frame_size 帧大小
 * @return ETH_OK成功
 */
eth_status_t cbs_decrement_credit(uint16_t port_id, uint8_t queue_id, 
                                   uint32_t frame_size);

/* ============================================================================
 * 传输控制API
 * ============================================================================ */

/**
 * @brief 检查是否可以传输
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param frame_size 帧大小
 * @param can_transmit 是否可传输输出
 * @return ETH_OK成功
 */
eth_status_t cbs_can_transmit(uint16_t port_id, uint8_t queue_id, 
                               uint32_t frame_size, bool *can_transmit);

/**
 * @brief 开始传输
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param frame_size 帧大小
 * @return ETH_OK成功
 */
eth_status_t cbs_start_transmission(uint16_t port_id, uint8_t queue_id, 
                                     uint32_t frame_size);

/**
 * @brief 完成传输
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param frame_size 帧大小
 * @param actual_time_ns 实际传输时间 (ns)
 * @return ETH_OK成功
 */
eth_status_t cbs_complete_transmission(uint16_t port_id, uint8_t queue_id,
                                        uint32_t frame_size, uint64_t actual_time_ns);

/* ============================================================================
 * SR Class API
 * ============================================================================ */

/**
 * @brief 获取SR Class优先级
 * @param class_type SR类型
 * @param priority 优先级输出
 * @return ETH_OK成功
 */
eth_status_t cbs_get_sr_class_priority(cbs_sr_class_type_t class_type, uint8_t *priority);

/**
 * @brief 获取SR Class最大延迟
 * @param class_type SR类型
 * @param latency_us 延迟输出 (微秒)
 * @return ETH_OK成功
 */
eth_status_t cbs_get_sr_class_latency(cbs_sr_class_type_t class_type, uint32_t *latency_us);

/**
 * @brief 计算Idle Slope
 * @param port_transmit_rate_bps 端口传输速率
 * @param bandwidth_percent 带宽百分比
 * @param idle_slope idle slope输出
 * @return ETH_OK成功
 */
eth_status_t cbs_calc_idle_slope(uint32_t port_transmit_rate_bps,
                                  uint32_t bandwidth_percent,
                                  int64_t *idle_slope);

/**
 * @brief 计算Send Slope
 * @param port_transmit_rate_bps 端口传输速率
 * @param idle_slope idle slope
 * @param send_slope send slope输出
 * @return ETH_OK成功
 */
eth_status_t cbs_calc_send_slope(uint32_t port_transmit_rate_bps,
                                  int64_t idle_slope,
                                  int64_t *send_slope);

/* ============================================================================
 * 状态查询API
 * ============================================================================ */

/**
 * @brief 获取统计信息
 * @param port_id 端口ID
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t cbs_get_stats(uint16_t port_id, cbs_stats_t *stats);

/**
 * @brief 清除统计信息
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t cbs_clear_stats(uint16_t port_id);

/**
 * @brief 获取队列状态
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param state 状态输出
 * @return ETH_OK成功
 */
eth_status_t cbs_get_queue_state(uint16_t port_id, uint8_t queue_id, 
                                  cbs_queue_state_t *state);

/* ============================================================================
 * 回调注册API
 * ============================================================================ */

/**
 * @brief 注册信用更新回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t cbs_register_credit_update_callback(cbs_credit_update_callback_t callback, 
                                                  void *user_data);

/**
 * @brief 注册传输许可回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t cbs_register_transmit_permission_callback(cbs_transmit_permission_callback_t callback,
                                                        void *user_data);

/**
 * @brief 注册带宽告警回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t cbs_register_bandwidth_alert_callback(cbs_bandwidth_alert_callback_t callback,
                                                    void *user_data);

/**
 * @brief 注册安全告警回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t cbs_register_safety_alert_callback(cbs_safety_alert_callback_t callback,
                                                 void *user_data);

/* ============================================================================
 * 安全监控API
 * ============================================================================ */

/**
 * @brief 初始化安全监控
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t cbs_init_safety_monitor(uint16_t port_id);

/**
 * @brief 执行安全检查
 * @param port_id 端口ID
 * @return ETH_OK通过检查
 */
eth_status_t cbs_run_safety_checks(uint16_t port_id);

/**
 * @brief 获取安全监控状态
 * @param port_id 端口ID
 * @param monitor 监控状态输出
 * @return ETH_OK成功
 */
eth_status_t cbs_get_safety_monitor(uint16_t port_id, cbs_safety_monitor_t *monitor);

/**
 * @brief 检查带宽使用率
 * @param port_id 端口ID
 * @param utilization_percent 利用率输出 (百分比)
 * @return ETH_OK成功
 */
eth_status_t cbs_check_bandwidth_utilization(uint16_t port_id, 
                                              uint32_t *utilization_percent);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 创建标准车载CBS配置
 * @param port_id 端口ID
 * @param port_rate_mbps 端口速率 (Mbps)
 * @param config 配置输出
 * @return ETH_OK成功
 */
eth_status_t cbs_create_automotive_config(uint16_t port_id, 
                                           uint32_t port_rate_mbps,
                                           cbs_port_config_t *config);

/**
 * @brief 计算所需信用
 * @param frame_size 帧大小
 * @param send_slope send slope
 * @param required_credit 所需信用输出
 * @return ETH_OK成功
 */
eth_status_t cbs_calc_required_credit(uint32_t frame_size,
                                       int64_t send_slope,
                                       int64_t *required_credit);

/**
 * @brief 打印CBS状态
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t cbs_print_status(uint16_t port_id);

#ifdef __cplusplus
}
#endif

#endif /* TSN_CBS_H */
