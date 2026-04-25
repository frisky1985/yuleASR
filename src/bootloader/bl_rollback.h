/**
 * @file bl_rollback.h
 * @brief Bootloader Rollback Mechanism
 * @version 1.0
 * @date 2026-04-25
 *
 * 自动回滚机制，支持更新失败后恢复到之前版本
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#ifndef BL_ROLLBACK_H
#define BL_ROLLBACK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define BL_ROLLBACK_MAJOR_VERSION       1
#define BL_ROLLBACK_MINOR_VERSION       0
#define BL_ROLLBACK_PATCH_VERSION       0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define BL_ROLLBACK_MAX_HISTORY         4
#define BL_ROLLBACK_MAGIC               0x524F4C42  /* "ROLB" */

/* 回滚原因 */
#define BL_ROLLBACK_REASON_NONE                 0x00
#define BL_ROLLBACK_REASON_BOOT_FAILURE         0x01
#define BL_ROLLBACK_REASON_WATCHDOG_TIMEOUT     0x02
#define BL_ROLLBACK_REASON_SECURITY_VIOLATION   0x03
#define BL_ROLLBACK_REASON_VERSION_MISMATCH     0x04
#define BL_ROLLBACK_REASON_HASH_MISMATCH        0x05
#define BL_ROLLBACK_REASON_SIGNATURE_INVALID    0x06
#define BL_ROLLBACK_REASON_USER_REQUEST         0x07
#define BL_ROLLBACK_REASON_OTA_TIMEOUT          0x08
#define BL_ROLLBACK_REASON_FLASH_ERROR          0x09
#define BL_ROLLBACK_REASON_UNKNOWN              0xFF

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    BL_ROLLBACK_OK = 0,
    BL_ROLLBACK_ERROR_INVALID_PARAM = -1,
    BL_ROLLBACK_ERROR_NOT_INITIALIZED = -2,
    BL_ROLLBACK_ERROR_NO_PREVIOUS_VERSION = -3,
    BL_ROLLBACK_ERROR_STORAGE_ERROR = -4,
    BL_ROLLBACK_ERROR_ROLLBACK_FAILED = -5,
    BL_ROLLBACK_ERROR_ALREADY_ROLLING_BACK = -6,
    BL_ROLLBACK_ERROR_NO_VALID_PARTITION = -7,
    BL_ROLLBACK_ERROR_VERSION_NOT_FOUND = -8
} bl_rollback_error_t;

/* ============================================================================
 * 回滚状态
 * ============================================================================ */
typedef enum {
    BL_ROLLBACK_STATE_IDLE = 0,         /* 空闲状态 */
    BL_ROLLBACK_STATE_CHECKING,         /* 检查是否需要回滚 */
    BL_ROLLBACK_STATE_PREPARING,        /* 准备回滚 */
    BL_ROLLBACK_STATE_EXECUTING,        /* 执行回滚 */
    BL_ROLLBACK_STATE_VERIFYING,        /* 验证回滚结果 */
    BL_ROLLBACK_STATE_COMPLETED,        /* 回滚完成 */
    BL_ROLLBACK_STATE_FAILED            /* 回滚失败 */
} bl_rollback_state_t;

/* ============================================================================
 * 版本历史记录
 * ============================================================================ */
typedef struct {
    uint32_t version;                   /* 固件版本号 */
    uint32_t partition_id;              /* 分区ID */
    uint8_t  partition_name[16];        /* 分区名称 */
    uint64_t install_time;              /* 安装时间 */
    uint64_t last_boot_time;            /* 上次启动时间 */
    uint32_t boot_count;                /* 启动次数 */
    uint32_t boot_success_count;        /* 成功启动次数 */
    bool     is_valid;                  /* 是否有效 */
    uint8_t  hash[32];                  /* 固件哈希 */
} bl_version_history_entry_t;

/* ============================================================================
 * 回滚记录
 * ============================================================================ */
typedef struct {
    uint32_t magic;                     /* 魔法数 */
    uint32_t version;                   /* 结构版本 */
    uint32_t rollback_count;            /* 总回滚次数 */
    
    /* 当前回滚 */
    bool     active;                    /* 回滚是否活跃 */
    uint8_t  reason;                    /* 回滚原因 */
    uint32_t source_version;            /* 回滚源版本 (失败的版本) */
    uint32_t target_version;            /* 回滚目标版本 */
    uint32_t source_partition;          /* 源分区 */
    uint32_t target_partition;          /* 目标分区 */
    uint64_t rollback_time;             /* 回滚执行时间 */
    bool     verified;                  /* 回滚是否已验证 */
    
    /* 历史记录 */
    bl_version_history_entry_t history[BL_ROLLBACK_MAX_HISTORY];
    uint32_t history_count;
    
    /* 统计 */
    uint32_t total_boot_attempts;       /* 总启动尝试次数 */
    uint32_t consecutive_failures;      /* 连续失败次数 */
    uint32_t max_consecutive_failures;  /* 最大连续失败次数 */
    
    /* CRC */
    uint32_t crc32;
} bl_rollback_record_t;

/* ============================================================================
 * 启动检查结果
 * ============================================================================ */
typedef enum {
    BL_BOOT_RESULT_SUCCESS = 0,         /* 启动成功 */
    BL_BOOT_RESULT_FAILURE,             /* 启动失败 */
    BL_BOOT_RESULT_WATCHDOG_TIMEOUT,    /* 看门狗超时 */
    BL_BOOT_RESULT_SECURITY_VIOLATION,  /* 安全违规 */
    BL_BOOT_RESULT_UNKNOWN              /* 未知 */
} bl_boot_result_t;

/* ============================================================================
 * 回滚配置
 * ============================================================================ */
typedef struct {
    uint32_t max_boot_attempts;         /* 最大启动尝试次数 */
    uint32_t max_consecutive_failures;  /* 最大连续失败次数 */
    bool     auto_rollback_enabled;     /* 自动回滚使能 */
    bool     preserve_history;          /* 保留版本历史 */
    uint32_t rollback_delay_ms;         /* 回滚延迟 */
    
    /* 回调函数 */
    void (*on_rollback_start)(uint8_t reason, uint32_t from_version, uint32_t to_version);
    void (*on_rollback_complete)(bool success);
    void (*on_version_switch)(uint32_t old_version, uint32_t new_version);
} bl_rollback_config_t;

/* ============================================================================
 * 回滚管理器上下文
 * ============================================================================ */
typedef struct {
    bl_rollback_config_t config;
    bl_rollback_state_t  state;
    bl_rollback_record_t record;
    
    /* 当前版本信息 */
    uint32_t current_version;
    uint32_t current_partition;
    
    /* 启动计数 */
    uint32_t boot_attempt_counter;
    bool     boot_completed;
    
    /* 分区管理器接口 */
    void *partition_manager;
    
    /* DEM报告回调 */
    void (*report_dtc)(uint32_t dtc_code, uint8_t status);
    
    bool initialized;
} bl_rollback_manager_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化回滚管理器
 * @param mgr 回滚管理器上下文
 * @param config 配置参数
 * @param partition_mgr 分区管理器上下文
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_init(
    bl_rollback_manager_t *mgr,
    const bl_rollback_config_t *config,
    void *partition_mgr
);

/**
 * @brief 反初始化回滚管理器
 * @param mgr 回滚管理器上下文
 */
void bl_rollback_deinit(bl_rollback_manager_t *mgr);

/**
 * @brief 记录新版本安装
 * @param mgr 回滚管理器上下文
 * @param version 版本号
 * @param partition_id 分区ID
 * @param hash 固件哈希
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_record_install(
    bl_rollback_manager_t *mgr,
    uint32_t version,
    uint32_t partition_id,
    const uint8_t hash[32]
);

/**
 * @brief 记录启动尝试
 * @param mgr 回滚管理器上下文
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_record_boot_attempt(bl_rollback_manager_t *mgr);

/**
 * @brief 记录启动结果
 * @param mgr 回滚管理器上下文
 * @param result 启动结果
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_record_boot_result(
    bl_rollback_manager_t *mgr,
    bl_boot_result_t result
);

/**
 * @brief 检查是否需要回滚
 * @param mgr 回滚管理器上下文
 * @param need_rollback 输出是否需要回滚
 * @param target_version 输出目标版本 (可为NULL)
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_check_needed(
    bl_rollback_manager_t *mgr,
    bool *need_rollback,
    uint32_t *target_version
);

/**
 * @brief 执行回滚
 * @param mgr 回滚管理器上下文
 * @param reason 回滚原因
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_execute(
    bl_rollback_manager_t *mgr,
    uint8_t reason
);

/**
 * @brief 确认回滚完成 (在成功启动到旧版本后调用)
 * @param mgr 回滚管理器上下文
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_confirm(bl_rollback_manager_t *mgr);

/**
 * @brief 清除回滚状态 (在确认新版本稳定后调用)
 * @param mgr 回滚管理器上下文
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_clear(bl_rollback_manager_t *mgr);

/**
 * @brief 获取前一个有效版本
 * @param mgr 回滚管理器上下文
 * @param version 输出版本号
 * @param partition_id 输出分区ID
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_get_previous_version(
    bl_rollback_manager_t *mgr,
    uint32_t *version,
    uint32_t *partition_id
);

/**
 * @brief 获取版本历史
 * @param mgr 回滚管理器上下文
 * @param history 输出历史记录数组
 * @param max_entries 最大条目数
 * @param num_entries 实际条目数 (输出)
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_get_history(
    bl_rollback_manager_t *mgr,
    bl_version_history_entry_t *history,
    uint32_t max_entries,
    uint32_t *num_entries
);

/**
 * @brief 保存回滚记录到持久存储
 * @param mgr 回滚管理器上下文
 * @param address 存储地址
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_save_record(
    bl_rollback_manager_t *mgr,
    uint32_t address
);

/**
 * @brief 从持久存储加载回滚记录
 * @param mgr 回滚管理器上下文
 * @param address 存储地址
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_load_record(
    bl_rollback_manager_t *mgr,
    uint32_t address
);

/**
 * @brief 获取当前回滚状态
 * @param mgr 回滚管理器上下文
 * @return 当前状态
 */
bl_rollback_state_t bl_rollback_get_state(const bl_rollback_manager_t *mgr);

/**
 * @brief 获取当前回滚记录
 * @param mgr 回滚管理器上下文
 * @param record 输出记录
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_get_record(
    const bl_rollback_manager_t *mgr,
    bl_rollback_record_t *record
);

/**
 * @brief 设置当前版本
 * @param mgr 回滚管理器上下文
 * @param version 版本号
 * @param partition_id 分区ID
 */
void bl_rollback_set_current_version(
    bl_rollback_manager_t *mgr,
    uint32_t version,
    uint32_t partition_id
);

/**
 * @brief 设置DEM报告回调
 * @param mgr 回滚管理器上下文
 * @param report_func DTC报告函数
 */
void bl_rollback_set_dem_callback(
    bl_rollback_manager_t *mgr,
    void (*report_func)(uint32_t dtc_code, uint8_t status)
);

/**
 * @brief 获取回滚原因字符串
 * @param reason 回滚原因代码
 * @return 回滚原因字符串
 */
const char* bl_rollback_reason_to_string(uint8_t reason);

/**
 * @brief 重置所有回滚状态 (调试用)
 * @param mgr 回滚管理器上下文
 * @return BL_ROLLBACK_OK成功
 */
bl_rollback_error_t bl_rollback_reset(bl_rollback_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* BL_ROLLBACK_H */
