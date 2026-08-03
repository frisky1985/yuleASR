/**
 * @file tsn_stack.h
 * @brief TSN协议栈整合头文件
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 车载TSN协议栈
 * @note 符合IEEE 802.1AS/Qbv/Qbu/Qav/Qcc/Qca
 * @note 支持ASIL-D安全等级
 */

#ifndef TSN_STACK_H
#define TSN_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TSN模块导入
 * ============================================================================ */

#include "gptp/gptp.h"
#include "tas/tas.h"
#include "fp/frame_preemption.h"
#include "cbs/cbs.h"
#include "srp/stream_reservation.h"

/* ============================================================================
 * TSN版本信息
 * ============================================================================ */

#define TSN_STACK_VERSION_MAJOR         1
#define TSN_STACK_VERSION_MINOR         0
#define TSN_STACK_VERSION_PATCH         0

#define TSN_STACK_VERSION_STRING        "1.0.0"

/* ============================================================================
 * TSN配置
 * ============================================================================ */

typedef struct {
    /* gPTP配置 */
    gptp_config_t gptp_config;
    
    /* TAS配置 */
    bool tas_enabled;
    
    /* CBS配置 */
    bool cbs_enabled;
    
    /* 帧抢占配置 */
    bool fp_enabled;
    
    /* 流预留配置 */
    bool srp_enabled;
    
    /* ASIL-D安全配置 */
    bool asil_d_enabled;
    uint32_t safety_monitor_interval_ms;
} tsn_stack_config_t;

/* ============================================================================
 * TSN状态
 * ============================================================================ */

typedef enum {
    TSN_STATE_UNINIT = 0,
    TSN_STATE_INIT,
    TSN_STATE_RUNNING,
    TSN_STATE_STOPPED,
    TSN_STATE_ERROR,
} tsn_state_t;

typedef struct {
    tsn_state_t state;
    bool gptp_active;
    bool tas_active;
    bool cbs_active;
    bool fp_active;
    bool srp_active;
    uint32_t active_domain_count;
    uint32_t active_port_count;
} tsn_stack_status_t;

/* ============================================================================
 * TSN统计信息
 * ============================================================================ */

typedef struct {
    /* gPTP统计 */
    uint32_t synced_domains;
    uint32_t total_domains;
    int64_t avg_sync_precision_ns;
    
    /* TAS统计 */
    uint64_t tas_cycles_completed;
    uint32_t tas_schedule_errors;
    
    /* CBS统计 */
    uint64_t cbs_frames_transmitted;
    uint64_t cbs_credit_violations;
    
    /* 帧抢占统计 */
    uint64_t fp_preemptions_count;
    uint64_t fp_reassembly_success;
    
    /* 流预留统计 */
    uint32_t srp_active_streams;
    uint32_t srp_reserved_bandwidth_bps;
} tsn_stack_stats_t;

/* ============================================================================
 * 初始化和配置API
 * ============================================================================ */

/**
 * @brief 初始化TSN协议栈
 * @param config TSN配置
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_init(const tsn_stack_config_t *config);

/**
 * @brief 反初始化TSN协议栈
 */
void tsn_stack_deinit(void);

/**
 * @brief 启动TSN协议栈
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_start(void);

/**
 * @brief 停止TSN协议栈
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_stop(void);

/**
 * @brief 获取TSN状态
 * @param status 状态输出
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_get_status(tsn_stack_status_t *status);

/* ============================================================================
 * 车载配置API
 * ============================================================================ */

/**
 * @brief 创建标准车载TSN配置
 * @param port_rate_mbps 端口速率 (Mbps)
 * @param config 配置输出
 * @return ETH_OK成功
 */
eth_status_t tsn_create_automotive_config(uint32_t port_rate_mbps, 
                                           tsn_stack_config_t *config);

/**
 * @brief 配置车载时间同步域
 * @param domain_index 域索引
 * @param domain_config 域配置
 * @return ETH_OK成功
 */
eth_status_t tsn_config_automotive_domain(uint8_t domain_index,
                                           const gptp_domain_config_t *domain_config);

/**
 * @brief 配置车载调度器
 * @param port_id 端口ID
 * @param tas_config TAS配置
 * @param cbs_config CBS配置
 * @return ETH_OK成功
 */
eth_status_t tsn_config_automotive_scheduler(uint16_t port_id,
                                              const tas_port_config_t *tas_config,
                                              const cbs_port_config_t *cbs_config);

/* ============================================================================
 * 运行时API
 * ============================================================================ */

/**
 * @brief 运行TSN主循环 (每毫秒调用一次)
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_run_cycle(void);

/**
 * @brief 等待时间同步
 * @param domain_index 域索引
 * @param timeout_ms 超时时间 (毫秒)
 * @return ETH_OK成功
 */
eth_status_t tsn_wait_for_sync(uint8_t domain_index, uint32_t timeout_ms);

/**
 * @brief 发送TSN流数据
 * @param port_id 端口ID
 * @param queue_id 队列ID
 * @param data 数据
 * @param len 长度
 * @param priority 优先级
 * @return ETH_OK成功
 */
eth_status_t tsn_transmit_frame(uint16_t port_id, uint8_t queue_id,
                                 const uint8_t *data, uint32_t len,
                                 uint8_t priority);

/* ============================================================================
 * 统计和监控API
 * ============================================================================ */

/**
 * @brief 获取TSN统计信息
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_get_stats(tsn_stack_stats_t *stats);

/**
 * @brief 清除TSN统计信息
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_clear_stats(void);

/**
 * @brief 执行安全检查
 * @return ETH_OK通过检查
 */
eth_status_t tsn_stack_run_safety_checks(void);

/**
 * @brief 打印TSN状态
 * @return ETH_OK成功
 */
eth_status_t tsn_stack_print_status(void);

/* ============================================================================
 * 版本信息API
 * ============================================================================ */

/**
 * @brief 获取TSN版本字符串
 * @return 版本字符串
 */
const char* tsn_stack_get_version_string(void);

/**
 * @brief 获取TSN版本号
 * @param major 主版本输出
 * @param minor 次版本输出
 * @param patch 补丁版本输出
 */
void tsn_stack_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

#ifdef __cplusplus
}
#endif

#endif /* TSN_STACK_H */
