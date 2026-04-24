/**
 * @file tas.h
 * @brief 时间感知调度器 (IEEE 802.1Qbv-2015)
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 支持车载TSN应用
 * @note 确定性延迟<1ms
 * @note 符合ASIL-D安全等级
 */

#ifndef TSN_TAS_H
#define TSN_TAS_H

#include "../../common/types/eth_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * IEEE 802.1Qbv 常量定义
 * ============================================================================ */

/** 最大队列数量 */
#define TAS_MAX_QUEUES                  8

/** 最大GCL条目数 */
#define TAS_MAX_GCL_ENTRIES             1024

/** 最大循环时间 (微秒) */
#define TAS_MAX_CYCLE_TIME_US           1000000  /* 1秒 */

/** 默认循环时间 (微秒) */
#define TAS_DEFAULT_CYCLE_TIME_US       1000     /* 1ms */

/** 序列化时间阈值 (微秒) */
#define TAS_HOLD_ADVANCE_US             100      /* 100us */

/** 推迟边界 (微秒) */
#define TAS_MAX_SDU_TRANSMISSION_TIME   250      /* 250us for 2KB frame @ 1Gbps */

/** ASIL-D安全相关 */
#define TAS_SAFETY_GATE_ENABLE_MASK     0xFF
#define TAS_SAFETY_MONITOR_INTERVAL_US  100

/* ============================================================================
 * 门状态定义
 * ============================================================================ */

/** 门状态 */
typedef enum {
    TAS_GATE_CLOSED = 0,            /* 门关闭 - 不允许传输 */
    TAS_GATE_OPEN = 1,              /* 门打开 - 允许传输 */
} tas_gate_state_t;

/** 门控列表条目 */
typedef struct {
    uint8_t gate_states;            /* 每位代表一个队列的门状态 */
    uint32_t time_interval_us;      /* 时间间隔 (微秒) */
} tas_gcl_entry_t;

/** 门控列表配置 */
typedef struct {
    tas_gcl_entry_t entries[TAS_MAX_GCL_ENTRIES];
    uint32_t entry_count;           /* GCL条目数量 */
    uint32_t cycle_time_us;         /* 循环时间 (微秒) */
    uint32_t cycle_time_extension_us;  /* 循环时间扩展 (微秒) */
    uint64_t base_time_sec;         /* 基准时间 (秒) */
    uint32_t base_time_nsec;        /* 基准时间 (纳秒) */
} tas_gcl_config_t;

/* ============================================================================
 * 队列和优先级配置
 * ============================================================================ */

/** 队列优先级 */
typedef enum {
    TAS_PRIORITY_CRITICAL = 0,      /* 严格实时 (SR Class A) */
    TAS_PRIORITY_REALTIME = 1,      /* 实时 (SR Class B) */
    TAS_PRIORITY_HIGH = 2,          /* 高优先级 */
    TAS_PRIORITY_MEDIUM = 3,        /* 中等优先级 */
    TAS_PRIORITY_LOW = 4,           /* 低优先级 */
    TAS_PRIORITY_BEST_EFFORT = 5,   /* 最佳努力 */
} tas_priority_t;

/** 队列配置 */
typedef struct {
    uint8_t queue_id;               /* 队列ID */
    tas_priority_t priority;        /* 优先级 */
    uint32_t max_sdu_bytes;         /* 最大SDU大小 */
    bool transmission_overrun_enable;  /* 传输超时使能 */
    uint32_t transmission_overrun_us;  /* 传输超时阈值 */
} tas_queue_config_t;

/* ============================================================================
 * TAS端口配置
 * ============================================================================ */

/** 调度行为 */
typedef enum {
    TAS_SCHEDULE_STRICT = 0,        /* 严格调度 - 按GCL执行 */
    TAS_SCHEDULE_LENIENT = 1,       /* 宽松调度 - 允许某些灵活性 */
} tas_schedule_behavior_t;

/** TAS端口配置 */
typedef struct {
    uint16_t port_id;                       /* 端口ID */
    bool gate_enabled;                      /* 门控使能 */
    tas_schedule_behavior_t schedule_behavior;  /* 调度行为 */
    
    /* GCL配置 */
    tas_gcl_config_t gcl_config;            /* 门控列表配置 */
    
    /* 队列配置 */
    tas_queue_config_t queue_configs[TAS_MAX_QUEUES];
    uint8_t queue_count;                    /* 队列数量 */
    
    /* 时间参数 */
    uint32_t hold_advance_us;               /* 保持提前量 */
    uint32_t release_advance_us;            /* 释放提前量 */
    
    /* 安全相关 */
    bool enable_safety_checks;              /* 启用安全检查 */
    uint32_t safety_monitor_interval_us;    /* 安全监控间隔 */
} tas_port_config_t;

/* ============================================================================
 * 运行时状态
 * ============================================================================ */

/** 当前门状态 */
typedef struct {
    uint8_t gate_states;            /* 当前门状态 */
    uint32_t current_gcl_index;     /* 当前GCL索引 */
    uint64_t cycle_start_time_ns;   /* 循环开始时间 */
    uint64_t entry_start_time_ns;   /* 当前条目开始时间 */
} tas_gate_status_t;

/** 队列统计信息 */
typedef struct {
    uint64_t tx_frames;             /* 发送帧数 */
    uint64_t tx_bytes;              /* 发送字节数 */
    uint64_t dropped_frames;        /* 丢弃帧数 */
    uint64_t blocked_frames;        /* 阻塞帧数 */
    uint32_t transmission_overruns; /* 传输超时数 */
    uint64_t total_delay_ns;        /* 总延迟 */
} tas_queue_stats_t;

/** 调度器统计 */
typedef struct {
    uint64_t cycles_completed;      /* 已完成循环数 */
    uint64_t gcl_transitions;       /* GCL转换次数 */
    uint32_t schedule_errors;       /* 调度错误数 */
    uint32_t gate_conflict_errors;  /* 门冲突错误数 */
    uint64_t last_cycle_time_ns;    /* 上次循环时间 */
} tas_scheduler_stats_t;

/* ============================================================================
 * 安全监控结构
 * ============================================================================ */

/** 安全监控状态 */
typedef struct {
    /* 门状态监控 */
    uint8_t expected_gate_states;
    uint8_t actual_gate_states;
    uint32_t gate_mismatch_count;
    
    /* 调度完整性 */
    bool schedule_integrity_ok;
    bool gcl_integrity_ok;
    
    /* 时间监控 */
    uint32_t cycle_time_violations;
    int64_t max_cycle_jitter_ns;
    
    /* 故障检测 */
    bool fault_detected;
    uint32_t fault_code;
} tas_safety_monitor_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 门状态变化回调 */
typedef void (*tas_gate_change_callback_t)(
    uint16_t port_id,
    uint8_t old_states,
    uint8_t new_states,
    void *user_data
);

/** 调度错误回调 */
typedef void (*tas_schedule_error_callback_t)(
    uint16_t port_id,
    uint32_t error_code,
    const char *error_msg,
    void *user_data
);

/** 安全告警回调 */
typedef void (*tas_safety_alert_callback_t)(
    uint16_t port_id,
    uint32_t alert_type,
    const char *alert_msg,
    void *user_data
);

/* ============================================================================
 * 初始化和配置API
 * ============================================================================ */

/**
 * @brief 初始化TAS调度器
 * @return ETH_OK成功
 */
eth_status_t tas_init(void);

/**
 * @brief 反初始化TAS调度器
 */
void tas_deinit(void);

/**
 * @brief 配置TAS端口
 * @param port_id 端口ID
 * @param config 端口配置
 * @return ETH_OK成功
 */
eth_status_t tas_config_port(uint16_t port_id, const tas_port_config_t *config);

/**
 * @brief 获取TAS端口配置
 * @param port_id 端口ID
 * @param config 配置输出
 * @return ETH_OK成功
 */
eth_status_t tas_get_port_config(uint16_t port_id, tas_port_config_t *config);

/**
 * @brief 更新GCL配置
 * @param port_id 端口ID
 * @param gcl_config GCL配置
 * @return ETH_OK成功
 */
eth_status_t tas_update_gcl(uint16_t port_id, const tas_gcl_config_t *gcl_config);

/**
 * @brief 启用/禁用门控
 * @param port_id 端口ID
 * @param enable 使能状态
 * @return ETH_OK成功
 */
eth_status_t tas_enable_gate_control(uint16_t port_id, bool enable);

/**
 * @brief 设置队列门状态
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param state 门状态
 * @return ETH_OK成功
 */
eth_status_t tas_set_queue_gate(uint16_t port_id, uint8_t queue_id, tas_gate_state_t state);

/* ============================================================================
 * 调度执行API
 * ============================================================================ */

/**
 * @brief 启动调度器
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t tas_start_scheduler(uint16_t port_id);

/**
 * @brief 停止调度器
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t tas_stop_scheduler(uint16_t port_id);

/**
 * @brief 执行调度循环 (在每个时间片调用)
 * @param port_id 端口ID
 * @param current_time_ns 当前时间 (纳秒)
 * @return ETH_OK成功
 */
eth_status_t tas_run_schedule_cycle(uint16_t port_id, uint64_t current_time_ns);

/**
 * @brief 检查队列是否可以传输
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param frame_size 帧大小
 * @param can_transmit 是否可传输输出
 * @return ETH_OK成功
 */
eth_status_t tas_can_transmit(uint16_t port_id, uint8_t queue_id, 
                               uint32_t frame_size, bool *can_transmit);

/**
 * @brief 获取当前可用时间窗口
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param remaining_time_us 剩余时间输出 (微秒)
 * @return ETH_OK成功
 */
eth_status_t tas_get_time_window(uint16_t port_id, uint8_t queue_id, 
                                  uint32_t *remaining_time_us);

/* ============================================================================
 * 状态查询API
 * ============================================================================ */

/**
 * @brief 获取当前门状态
 * @param port_id 端口ID
 * @param status 状态输出
 * @return ETH_OK成功
 */
eth_status_t tas_get_gate_status(uint16_t port_id, tas_gate_status_t *status);

/**
 * @brief 获取队列统计信息
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t tas_get_queue_stats(uint16_t port_id, uint8_t queue_id, 
                                  tas_queue_stats_t *stats);

/**
 * @brief 获取调度器统计信息
 * @param port_id 端口ID
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t tas_get_scheduler_stats(uint16_t port_id, tas_scheduler_stats_t *stats);

/**
 * @brief 获取当前GCL索引
 * @param port_id 端口ID
 * @param index GCL索引输出
 * @return ETH_OK成功
 */
eth_status_t tas_get_current_gcl_index(uint16_t port_id, uint32_t *index);

/* ============================================================================
 * 回调注册API
 * ============================================================================ */

/**
 * @brief 注册门状态变化回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t tas_register_gate_change_callback(tas_gate_change_callback_t callback, void *user_data);

/**
 * @brief 注册调度错误回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t tas_register_schedule_error_callback(tas_schedule_error_callback_t callback, void *user_data);

/**
 * @brief 注册安全告警回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t tas_register_safety_alert_callback(tas_safety_alert_callback_t callback, void *user_data);

/* ============================================================================
 * 安全监控API
 * ============================================================================ */

/**
 * @brief 初始化安全监控
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t tas_init_safety_monitor(uint16_t port_id);

/**
 * @brief 执行安全检查
 * @param port_id 端口ID
 * @return ETH_OK通过检查
 */
eth_status_t tas_run_safety_checks(uint16_t port_id);

/**
 * @brief 获取安全监控状态
 * @param port_id 端口ID
 * @param monitor 监控状态输出
 * @return ETH_OK成功
 */
eth_status_t tas_get_safety_monitor(uint16_t port_id, tas_safety_monitor_t *monitor);

/**
 * @brief 验证GCL配置有效性
 * @param gcl_config GCL配置
 * @param valid 是否有效输出
 * @return ETH_OK成功
 */
eth_status_t tas_validate_gcl(const tas_gcl_config_t *gcl_config, bool *valid);

/**
 * @brief 检查调度完整性
 * @param port_id 端口ID
 * @param integrity_ok 完整性状态输出
 * @return ETH_OK成功
 */
eth_status_t tas_check_schedule_integrity(uint16_t port_id, bool *integrity_ok);

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 创建标准车载GCL配置
 * @param gcl_config GCL配置输出
 * @param cycle_time_ms 循环时间 (毫秒)
 * @return ETH_OK成功
 */
eth_status_t tas_create_automotive_gcl(tas_gcl_config_t *gcl_config, uint32_t cycle_time_ms);

/**
 * @brief 计算帧传输时间
 * @param frame_size 帧大小 (字节)
 * @param link_speed_mbps 链路速率 (Mbps)
 * @return 传输时间 (微秒)
 */
uint32_t tas_calc_transmission_time(uint32_t frame_size, uint32_t link_speed_mbps);

/**
 * @brief 打印调度器状态
 * @param port_id 端口ID
 * @return ETH_OK成功
 */
eth_status_t tas_print_status(uint16_t port_id);

#ifdef __cplusplus
}
#endif

#endif /* TSN_TAS_H */
