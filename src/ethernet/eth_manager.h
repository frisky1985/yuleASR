/**
 * @file eth_manager.h
 * @brief 车载以太网管理器 - 链路状态监控、自动协商、错误统计
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 符合AUTOSAR MCAL规范
 * @note 支持功能安全ASIL等级
 */

#ifndef ETH_MANAGER_H
#define ETH_MANAGER_H

#include "driver/eth_mac_driver.h"
#include "driver/eth_dma.h"
#include "driver/eth_automotive.h"
#include "../common/types/eth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 管理器配置定义
 * ============================================================================ */

/** 链路监控配置 */
typedef struct {
    bool enable_link_monitoring;        /* 启用链路监控 */
    uint32_t link_check_interval_ms;    /* 链路检查间隔(毫秒) */
    uint32_t link_down_threshold;       /* 链路断开阈值次数 */
    bool auto_recovery;                 /* 自动恢复 */
} eth_link_monitor_config_t;

/** 错误统计配置 */
typedef struct {
    bool enable_error_counting;         /* 启用错误计数 */
    uint32_t error_threshold;           /* 错误阈值 */
    bool enable_error_callback;         /* 启用错误回调 */
    bool log_errors;                    /* 记录错误日志 */
} eth_error_config_t;

/** 自动协商配置 */
typedef struct {
    bool enable_auto_negotiation;       /* 启用自动协商 */
    uint32_t negotiation_timeout_ms;    /* 协商超时(毫秒) */
    uint32_t retry_count;               /* 重试次数 */
    bool fallback_to_100m;              /* 协商失败后回退100M */
} eth_negotiation_config_t;

/** 以太网管理器配置 */
typedef struct {
    eth_link_monitor_config_t link_monitor;     /* 链路监控 */
    eth_error_config_t error_config;            /* 错误统计 */
    eth_negotiation_config_t negotiation;       /* 自动协商 */
    eth_mac_config_t mac_config;                /* MAC配置 */
    eth_dma_config_t dma_config;                /* DMA配置 */
    automotive_phy_config_t phy_config;         /* PHY配置 */
} eth_manager_config_t;

/* ============================================================================
 * 管理器状态定义
 * ============================================================================ */

/** 以太网管理器状态 */
typedef enum {
    ETH_MANAGER_STATE_UNINIT = 0,       /* 未初始化 */
    ETH_MANAGER_STATE_INIT,             /* 已初始化 */
    ETH_MANAGER_STATE_LINK_DOWN,        /* 链路断开 */
    ETH_MANAGER_STATE_LINK_UP,          /* 链路连接 */
    ETH_MANAGER_STATE_ERROR             /* 错误状态 */
} eth_manager_state_t;

/** 综合统计信息 */
typedef struct {
    /* MAC统计 */
    eth_mac_stats_t mac_stats;
    /* DMA统计 */
    eth_dma_stats_t dma_stats;
    /* PHY统计 */
    automotive_phy_stats_t phy_stats;
    /* 管理器统计 */
    uint32_t link_up_count;
    uint32_t link_down_count;
    uint32_t auto_neg_count;
    uint32_t error_count;
    uint32_t recovery_count;
    uint32_t tx_throughput_mbps;        /* 发送吞吐量(Mbps) */
    uint32_t rx_throughput_mbps;        /* 接收吞吐量(Mbps) */
} eth_manager_stats_t;

/** 错误信息 */
typedef struct {
    uint32_t error_code;                /* 错误代码 */
    const char *error_string;           /* 错误描述 */
    uint64_t timestamp;                 /* 错误发生时间 */
} eth_error_info_t;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/** 链路状态变化回调 */
typedef void (*eth_manager_link_callback_t)(eth_link_status_t status, void *user_data);

/** 错误回调 */
typedef void (*eth_manager_error_callback_t)(const eth_error_info_t *error, void *user_data);

/** 统计更新回调 */
typedef void (*eth_manager_stats_callback_t)(const eth_manager_stats_t *stats, void *user_data);

/* ============================================================================
 * 管理器API
 * ============================================================================ */

/**
 * @brief 初始化以太网管理器
 * @param config 管理器配置
 * @return ETH_OK成功
 */
eth_status_t eth_manager_init(const eth_manager_config_t *config);

/**
 * @brief 反初始化以太网管理器
 */
void eth_manager_deinit(void);

/**
 * @brief 启动以太网管理器
 * @return ETH_OK成功
 */
eth_status_t eth_manager_start(void);

/**
 * @brief 停止以太网管理器
 * @return ETH_OK成功
 */
eth_status_t eth_manager_stop(void);

/**
 * @brief 获取管理器当前状态
 * @param state 状态输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_get_state(eth_manager_state_t *state);

/* ============================================================================
 * 链路管理API
 * ============================================================================ */

/**
 * @brief 设置链路状态变化回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_manager_register_link_callback(eth_manager_link_callback_t callback, void *user_data);

/**
 * @brief 检查链路状态
 * @param status 链路状态输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_check_link(eth_link_status_t *status);

/**
 * @brief 等待链路连接
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功，ETH_TIMEOUT超时
 */
eth_status_t eth_manager_wait_for_link(uint32_t timeout_ms);

/**
 * @brief 强制链路检测
 * @return ETH_OK成功
 */
eth_status_t eth_manager_force_link_check(void);

/* ============================================================================
 * 自动协商API
 * ============================================================================ */

/**
 * @brief 启动自动协商
 * @return ETH_OK成功
 */
eth_status_t eth_manager_start_auto_negotiation(void);

/**
 * @brief 获取自动协商状态
 * @param complete 协商是否完成输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_get_negotiation_status(bool *complete);

/**
 * @brief 设置强制速率模式(禁用自动协商)
 * @param mode 速率模式
 * @return ETH_OK成功
 */
eth_status_t eth_manager_set_forced_mode(eth_mode_t mode);

/**
 * @brief 等待自动协商完成
 * @param timeout_ms 超时时间(毫秒)
 * @return ETH_OK成功，ETH_TIMEOUT超时
 */
eth_status_t eth_manager_wait_for_negotiation(uint32_t timeout_ms);

/* ============================================================================
 * 错误统计API
 * ============================================================================ */

/**
 * @brief 获取综合统计信息
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_get_stats(eth_manager_stats_t *stats);

/**
 * @brief 清除综合统计信息
 * @return ETH_OK成功
 */
eth_status_t eth_manager_clear_stats(void);

/**
 * @brief 设置错误回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return ETH_OK成功
 */
eth_status_t eth_manager_register_error_callback(eth_manager_error_callback_t callback, void *user_data);

/**
 * @brief 获取最后错误信息
 * @param error 错误信息输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_get_last_error(eth_error_info_t *error);

/**
 * @brief 清除错误状态
 * @return ETH_OK成功
 */
eth_status_t eth_manager_clear_error(void);

/**
 * @brief 获取错误字符串
 * @param error_code 错误代码
 * @return 错误描述字符串
 */
const char* eth_manager_error_to_string(uint32_t error_code);

/* ============================================================================
 * 诊断和恢复API
 * ============================================================================ */

/**
 * @brief 执行全面诊断
 * @param diagnostics 诊断结果输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_run_diagnostics(automotive_phy_diagnostics_t *diagnostics);

/**
 * @brief 执行软复位
 * @return ETH_OK成功
 */
eth_status_t eth_manager_soft_reset(void);

/**
 * @brief 执行硬复位
 * @return ETH_OK成功
 */
eth_status_t eth_manager_hard_reset(void);

/**
 * @brief 尝试自动恢复
 * @return ETH_OK成功
 */
eth_status_t eth_manager_auto_recovery(void);

/**
 * @brief 检查是否需要恢复
 * @param need_recovery 需要恢复输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_check_recovery_needed(bool *need_recovery);

/* ============================================================================
 * 进阶功能API
 * ============================================================================ */

/**
 * @brief 设置统计更新回调
 * @param callback 回调函数
 * @param user_data 用户数据
 * @param interval_ms 更新间隔(毫秒, 0表示禁用)
 * @return ETH_OK成功
 */
eth_status_t eth_manager_register_stats_callback(eth_manager_stats_callback_t callback,
                                                  void *user_data,
                                                  uint32_t interval_ms);

/**
 * @brief 更新吞吐量计算
 * @return ETH_OK成功
 */
eth_status_t eth_manager_update_throughput(void);

/**
 * @brief 获取当前配置
 * @param config 配置输出
 * @return ETH_OK成功
 */
eth_status_t eth_manager_get_config(eth_manager_config_t *config);

/**
 * @brief 验证配置有效性
 * @param config 配置输入
 * @return ETH_OK有效
 */
eth_status_t eth_manager_validate_config(const eth_manager_config_t *config);

/**
 * @brief 打印诊断信息
 * @return ETH_OK成功
 */
eth_status_t eth_manager_print_diagnostics(void);

#ifdef __cplusplus
}
#endif

#endif /* ETH_MANAGER_H */
