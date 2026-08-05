/**
 * @file dds_advanced_pubsub.h
 * @brief DDS高级发布订阅功能模块整合头文件
 * @version 1.0
 * @date 2026-04-24
 * 
 * 车载DDS应用特性：
 * - 内容过滤主题(ContentFilteredTopic)
 * - 时间过滤(TimeBasedFilter)
 * - 所有权策略(Ownership)
 * - 持久化服务(PersistenceService)
 * 
 * 车载要求支持：
 * - ASIL-D安全等级
 * - 确定性执行时间 < 100us
 * - 低延迟数据传输
 */

#ifndef DDS_ADVANCED_PUBSUB_H
#define DDS_ADVANCED_PUBSUB_H

/* ============================================================================
 * 模块引用
 * ============================================================================ */

#include "content_filtered_topic.h"
#include "time_filter.h"
#include "ownership.h"
#include "persistence.h"

/* ============================================================================
 * 版本信息
 * ============================================================================ */

#define DDS_ADVANCED_PUBSUB_VERSION_MAJOR   1
#define DDS_ADVANCED_PUBSUB_VERSION_MINOR   0
#define DDS_ADVANCED_PUBSUB_VERSION_PATCH   0

/* ============================================================================
 * 高级PubSub配置结构
 * ============================================================================ */

/** 完整的高级发布者配置 */
typedef struct {
    // 基础DDS配置
    dds_qos_t base_qos;
    
    // 内容过滤
    bool enable_content_filter;
    cft_config_t content_filter_config;
    cft_type_descriptor_t type_descriptor;
    
    // 时间过滤
    bool enable_time_filter;
    tbf_config_t time_filter_config;
    
    // 所有权
    bool enable_ownership;
    own_config_t ownership_config;
    
    // 持久化
    bool enable_persistence;
    per_config_t persistence_config;
} dds_advanced_writer_config_t;

/** 完整的高级订阅者配置 */
typedef struct {
    // 基础DDS配置
    dds_qos_t base_qos;
    
    // 内容过滤
    bool enable_content_filter;
    cft_config_t content_filter_config;
    cft_type_descriptor_t type_descriptor;
    
    // 时间过滤
    bool enable_time_filter;
    tbf_config_t time_filter_config;
    
    // 持久化恢复
    bool enable_persistence_recovery;
    per_durability_level_t min_durability_level;
} dds_advanced_reader_config_t;

/* ============================================================================
 * 车载场景预设配置
 * ============================================================================ */

/**
 * @brief 获取ADAS高速传感器数据的预设配置
 * 优化点：低延迟 + 峰值保持压缩
 */
static inline void dds_get_adas_sensor_config(dds_advanced_writer_config_t *config) {
    memset(config, 0, sizeof(dds_advanced_writer_config_t));
    
    // QoS配置
    config->base_qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    config->base_qos.durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL;
    config->base_qos.deadline_ms = 10;
    
    // 时间过滤: 5ms分隔 + 峰值保持
    config->enable_time_filter = true;
    config->time_filter_config.minimum_separation_us = 5000;
    config->time_filter_config.policy = TBF_POLICY_PEAK_HOLD;
    config->time_filter_config.ts_source = TBF_TS_SOURCE_SYNC;
    config->time_filter_config.validate_timestamp = true;
    config->time_filter_config.timestamp_tolerance_us = 50;
    
    // 所有权: EXCLUSIVE模式
    config->enable_ownership = true;
    config->ownership_config.type = OWN_TYPE_EXCLUSIVE;
    config->ownership_config.initial_strength = 100;
    config->ownership_config.transfer_policy = OWN_TRANSFER_GRACEFUL;
}

/**
 * @brief 获取车辆控制指令的预设配置
 * 优化点：确定性 + 持久化
 */
static inline void dds_get_vehicle_control_config(dds_advanced_writer_config_t *config) {
    memset(config, 0, sizeof(dds_advanced_writer_config_t));
    
    // QoS配置
    config->base_qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    config->base_qos.durability = DDS_QOS_DURABILITY_PERSISTENT;
    config->base_qos.deadline_ms = 5;
    
    // 所有权: EXCLUSIVE模式 + 高强度
    config->enable_ownership = true;
    config->ownership_config.type = OWN_TYPE_EXCLUSIVE;
    config->ownership_config.initial_strength = 1000;
    config->ownership_config.auto_strength_adjust = false;
    
    // 持久化: PERSISTENT级别
    config->enable_persistence = true;
    config->persistence_config.level = PER_LEVEL_PERSISTENT;
    config->persistence_config.storage_type = PER_STORAGE_FILE;
    config->persistence_config.enable_checksum = true;
    config->persistence_config.auto_flush_interval_ms = 10;
}

/**
 * @brief 获取HMI状态更新的预设配置
 * 优化点：SHARED模式 + 内容过滤
 */
static inline void dds_get_hmi_status_config(dds_advanced_reader_config_t *config) {
    memset(config, 0, sizeof(dds_advanced_reader_config_t));
    
    // QoS配置
    config->base_qos.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    config->base_qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    
    // 内容过滤: 只关心重要状态变化
    config->enable_content_filter = true;
    strncpy(config->content_filter_config.filter_expression, 
            "priority > 0", CFT_MAX_FILTER_LENGTH - 1);
    config->content_filter_config.enable_optimization = true;
    config->content_filter_config.max_exec_time_us = 50;
}

/* ============================================================================
 * 初始化/反初始化函数
 * ============================================================================ */

/**
 * @brief 初始化高级PubSub模块
 * @return ETH_OK成功
 */
static inline eth_status_t dds_advanced_pubsub_init(void) {
    eth_status_t status;
    
    status = cft_init();
    if (status != ETH_OK) return status;
    
    status = tbf_init();
    if (status != ETH_OK) return status;
    
    status = own_init();
    if (status != ETH_OK) return status;
    
    status = per_init();
    if (status != ETH_OK) return status;
    
    return ETH_OK;
}

/**
 * @brief 反初始化高级PubSub模块
 */
static inline void dds_advanced_pubsub_deinit(void) {
    per_deinit();
    own_deinit();
    tbf_deinit();
    cft_deinit();
}

#endif /* DDS_ADVANCED_PUBSUB_H */
