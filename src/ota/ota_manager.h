/**
 * @file ota_manager.h
 * @brief OTA Manager - Core State Machine and Campaign Management
 * @version 1.0
 * @date 2026-04-26
 *
 * OTA Manager implements the core state machine for OTA updates
 * according to the OTA General Specification.
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "ota_types.h"
#include "../bootloader/bl_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define OTA_MANAGER_MAX_CAMPAIGNS       4
#define OTA_MANAGER_MAX_CALLBACKS       8
#define OTA_MANAGER_SESSION_TIMEOUT_MS  30000

/* ============================================================================
 * 前置条件检查结果
 * ============================================================================ */
typedef enum {
    OTA_PRECOND_OK = 0,
    OTA_PRECOND_BATTERY_LOW,
    OTA_PRECOND_VEHICLE_MOVING,
    OTA_PRECOND_ENGINE_RUNNING,
    OTA_PRECOND_GEAR_NOT_PARK,
    OTA_PRECOND_PARKING_BRAKE_OFF,
    OTA_PRECOND_USER_NOT_CONFIRMED,
    OTA_PRECOND_NETWORK_UNAVAILABLE,
    OTA_PRECOND_INSUFFICIENT_STORAGE
} ota_precondition_status_t;

/* ============================================================================
 * 前置条件结构
 * ============================================================================ */
typedef struct {
    float battery_voltage_min_v;        /* 最低电池电压 */
    uint8_t vehicle_speed_max_kmh;      /* 最大车速 */
    bool engine_must_be_off;            /* 引擎必须关闭 */
    bool parking_brake_required;        /* 需要拉手刹 */
    bool gear_must_be_park;             /* 必须在P档 */
    bool user_confirmation_required;    /* 需要用户确认 */
} ota_preconditions_t;

/* ============================================================================
 * 回滚信息
 * ============================================================================ */
typedef struct {
    bool rollback_triggered;            /* 回滚已触发 */
    uint32_t source_partition;          /* 来源分区 */
    uint32_t target_partition;          /* 目标分区 */
    char original_version[OTA_MAX_VERSION_LEN];
    char failed_version[OTA_MAX_VERSION_LEN];
    uint64_t rollback_time;
    ota_error_t rollback_reason;
} ota_rollback_info_t;

/* ============================================================================
 * 下载缓存信息
 * ============================================================================ */
typedef struct {
    uint8_t *buffer;                    /* 缓存缓冲区 */
    uint32_t buffer_size;               /* 缓冲区大小 */
    uint32_t used_size;                 /* 已使用大小 */
    char cached_package_id[OTA_MAX_PACKAGE_ID_LEN]; /* 缓存的包ID */
    uint64_t download_offset;           /* 下载偏移量 (用于断点续传) */
    bool valid;                         /* 缓存是否有效 */
} ota_download_cache_t;

/* ============================================================================
 * OTA Manager配置
 * ============================================================================ */
typedef struct {
    /* 超时配置 */
    uint32_t download_timeout_ms;
    uint32_t verify_timeout_ms;
    uint32_t install_timeout_ms;
    
    /* 重试配置 */
    uint8_t max_download_retries;
    uint8_t max_verify_retries;
    uint8_t max_install_retries;
    
    /* 回滚配置 */
    uint8_t max_boot_attempts;          /* 最大启动尝试次数 */
    bool auto_rollback_on_failure;      /* 失败时自动回滚 */
    
    /* 回调函数 */
    ota_state_change_cb_t on_state_change;
    ota_ecu_status_cb_t on_ecu_status;
    void *user_data;
} ota_manager_config_t;

/* ============================================================================
 * OTA Manager上下文
 * ============================================================================ */
typedef struct {
    /* 配置 */
    ota_manager_config_t config;
    
    /* 状态机 */
    ota_state_t current_state;
    ota_state_t previous_state;
    ota_error_t last_error;
    
    /* 活动管理 */
    ota_campaign_info_t campaigns[OTA_MANAGER_MAX_CAMPAIGNS];
    uint8_t num_campaigns;
    ota_campaign_info_t *active_campaign;
    
    /* 下载缓存 */
    ota_download_cache_t download_cache;
    
    /* 回滚信息 */
    ota_rollback_info_t rollback_info;
    
    /* 分区管理器引用 */
    bl_partition_manager_t *partition_mgr;
    
    /* 统计信息 */
    uint64_t state_entry_time;          /* 进入当前状态的时间 */
    uint32_t current_progress;          /* 当前进度 (0-100) */
    uint8_t retry_count;                /* 当前操作重试次数 */
    
    /* 初始化标志 */
    bool initialized;
    bool update_in_progress;
    bool cancel_requested;
} ota_manager_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化OTA Manager
 * @param ctx OTA Manager上下文
 * @param config 配置参数
 * @param partition_mgr 分区管理器引用
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_init(
    ota_manager_context_t *ctx,
    const ota_manager_config_t *config,
    bl_partition_manager_t *partition_mgr
);

/**
 * @brief 反初始化OTA Manager
 * @param ctx OTA Manager上下文
 */
void ota_manager_deinit(ota_manager_context_t *ctx);

/**
 * @brief 接收并验证更新活动
 * @param ctx OTA Manager上下文
 * @param campaign 活动信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_receive_campaign(
    ota_manager_context_t *ctx,
    const ota_campaign_info_t *campaign
);

/**
 * @brief 开始下载固件
 * @param ctx OTA Manager上下文
 * @param campaign_id 活动ID
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_start_download(
    ota_manager_context_t *ctx,
    const char *campaign_id
);

/**
 * @brief 暂停下载
 * @param ctx OTA Manager上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_pause_download(ota_manager_context_t *ctx);

/**
 * @brief 恢复下载
 * @param ctx OTA Manager上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_resume_download(ota_manager_context_t *ctx);

/**
 * @brief 开始验证包
 * @param ctx OTA Manager上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_start_verify(ota_manager_context_t *ctx);

/**
 * @brief 开始安装固件
 * @param ctx OTA Manager上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_start_install(ota_manager_context_t *ctx);

/**
 * @brief 激活新固件 (切换分区并重启)
 * @param ctx OTA Manager上下文
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_activate(ota_manager_context_t *ctx);

/**
 * @brief 触发回滚
 * @param ctx OTA Manager上下文
 * @param reason 回滚原因
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_trigger_rollback(
    ota_manager_context_t *ctx,
    ota_error_t reason
);

/**
 * @brief 取消更新
 * @param ctx OTA Manager上下文
 */
void ota_manager_cancel_update(ota_manager_context_t *ctx);

/**
 * @brief 获取当前状态
 * @param ctx OTA Manager上下文
 * @return 当前OTA状态
 */
ota_state_t ota_manager_get_state(const ota_manager_context_t *ctx);

/**
 * @brief 获取最后错误码
 * @param ctx OTA Manager上下文
 * @return 最后错误码
 */
ota_error_t ota_manager_get_last_error(const ota_manager_context_t *ctx);

/**
 * @brief 检查前置条件
 * @param ctx OTA Manager上下文
 * @param preconditions 前置条件配置
 * @return 前置条件检查结果
 */
ota_precondition_status_t ota_manager_check_preconditions(
    const ota_manager_context_t *ctx,
    const ota_preconditions_t *preconditions
);

/**
 * @brief 获取下载进度
 * @param ctx OTA Manager上下文
 * @param bytes_downloaded 已下载字节数
 * @param total_bytes 总字节数
 * @param percentage 百分比
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_get_download_progress(
    const ota_manager_context_t *ctx,
    uint64_t *bytes_downloaded,
    uint64_t *total_bytes,
    uint8_t *percentage
);

/**
 * @brief 获取活动信息
 * @param ctx OTA Manager上下文
 * @param campaign_id 活动ID
 * @param info 输出活动信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_get_campaign(
    const ota_manager_context_t *ctx,
    const char *campaign_id,
    ota_campaign_info_t *info
);

/**
 * @brief 获取活动列表
 * @param ctx OTA Manager上下文
 * @param campaigns 活动数组输出
 * @param max_count 最大数量
 * @param actual_count 实际数量输出
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_get_campaign_list(
    const ota_manager_context_t *ctx,
    ota_campaign_info_t *campaigns,
    uint8_t max_count,
    uint8_t *actual_count
);

/**
 * @brief 设置状态变更回调
 * @param ctx OTA Manager上下文
 * @param callback 回调函数
 */
void ota_manager_set_state_callback(
    ota_manager_context_t *ctx,
    ota_state_change_cb_t callback
);

/**
 * @brief 设置ECU状态回调
 * @param ctx OTA Manager上下文
 * @param callback 回调函数
 */
void ota_manager_set_ecu_callback(
    ota_manager_context_t *ctx,
    ota_ecu_status_cb_t callback
);

/**
 * @brief 初始化下载缓存
 * @param ctx OTA Manager上下文
 * @param buffer 缓存缓冲区
 * @param buffer_size 缓冲区大小
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_init_download_cache(
    ota_manager_context_t *ctx,
    uint8_t *buffer,
    uint32_t buffer_size
);

/**
 * @brief 获取下载缓存信息 (用于断点续传)
 * @param ctx OTA Manager上下文
 * @param cache_info 输出缓存信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_get_download_cache(
    const ota_manager_context_t *ctx,
    ota_download_cache_t *cache_info
);

/**
 * @brief 清除下载缓存
 * @param ctx OTA Manager上下文
 */
void ota_manager_clear_download_cache(ota_manager_context_t *ctx);

/**
 * @brief 周期处理函数 (需在主循环中调用)
 * @param ctx OTA Manager上下文
 */
void ota_manager_cyclic_process(ota_manager_context_t *ctx);

/**
 * @brief 检查是否可以回滚
 * @param ctx OTA Manager上下文
 * @return true可以回滚
 */
bool ota_manager_can_rollback(const ota_manager_context_t *ctx);

/**
 * @brief 获取回滚信息
 * @param ctx OTA Manager上下文
 * @param info 输出回滚信息
 * @return OTA_ERR_OK成功
 */
ota_error_t ota_manager_get_rollback_info(
    const ota_manager_context_t *ctx,
    ota_rollback_info_t *info
);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MANAGER_H */
