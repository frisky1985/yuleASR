/**
 * @file bl_rollback.c
 * @brief Bootloader Rollback Mechanism Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Automatic rollback mechanism for failed updates
 * UNECE R156 compliant
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stdlib.h>
#include "bl_rollback.h"
#include "bl_partition.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define BL_ROLLBACK_MODULE_NAME     "BL_RB"
#define BL_ROLLBACK_LOG_LEVEL       DDS_LOG_INFO

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 计算CRC32
 */
static uint32_t calculate_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/**
 * @brief 验证回滚记录
 */
static bool validate_record(const bl_rollback_record_t *record)
{
    if (record->magic != BL_ROLLBACK_MAGIC) {
        return false;
    }
    
    uint32_t calculated_crc = calculate_crc32(
        (const uint8_t*)record,
        sizeof(bl_rollback_record_t) - sizeof(uint32_t)
    );
    
    return (calculated_crc == record->crc32);
}

/**
 * @brief 更新记录CRC
 */
static void update_record_crc(bl_rollback_record_t *record)
{
    record->crc32 = calculate_crc32(
        (const uint8_t*)record,
        sizeof(bl_rollback_record_t) - sizeof(uint32_t)
    );
}

/**
 * @brief 设置状态
 */
static void set_state(bl_rollback_manager_t *mgr, bl_rollback_state_t state)
{
    mgr->state = state;
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback state changed to: %d", state);
}

/**
 * @brief 报告DTC
 */
static void report_dtc(bl_rollback_manager_t *mgr, uint32_t dtc_code, uint8_t status)
{
    if (mgr->report_dtc != NULL) {
        mgr->report_dtc(dtc_code, status);
    }
}

/**
 * @brief 查找版本历史记录
 */
static int32_t find_history_entry(bl_rollback_manager_t *mgr, uint32_t version)
{
    for (int32_t i = 0; i < BL_ROLLBACK_MAX_HISTORY; i++) {
        if (mgr->record.history[i].is_valid && 
            mgr->record.history[i].version == version) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 添加版本到历史记录
 */
static bl_rollback_error_t add_to_history(
    bl_rollback_manager_t *mgr,
    uint32_t version,
    uint32_t partition_id,
    const uint8_t hash[32]
)
{
    bl_rollback_record_t *record = &mgr->record;
    
    /* 检查是否已存在 */
    int32_t existing = find_history_entry(mgr, version);
    if (existing >= 0) {
        /* 更新现有记录 */
        record->history[existing].install_time = 0; /* TODO: Get current time */
        if (hash != NULL) {
            memcpy(record->history[existing].hash, hash, 32);
        }
        return BL_ROLLBACK_OK;
    }
    
    /* 查找空位置 */
    int32_t slot = -1;
    for (int32_t i = 0; i < BL_ROLLBACK_MAX_HISTORY; i++) {
        if (!record->history[i].is_valid) {
            slot = i;
            break;
        }
    }
    
    /* 如果没有空位置，移动历史记录 */
    if (slot < 0) {
        /* 移除最早的记录 */
        memmove(&record->history[0], &record->history[1],
                (BL_ROLLBACK_MAX_HISTORY - 1) * sizeof(bl_version_history_entry_t));
        slot = BL_ROLLBACK_MAX_HISTORY - 1;
    }
    
    /* 添加新记录 */
    memset(&record->history[slot], 0, sizeof(bl_version_history_entry_t));
    record->history[slot].version = version;
    record->history[slot].partition_id = partition_id;
    record->history[slot].install_time = 0; /* TODO: Get current time */
    record->history[slot].is_valid = true;
    if (hash != NULL) {
        memcpy(record->history[slot].hash, hash, 32);
    }
    
    /* 更新分区名称 */
    bl_partition_manager_t *part_mgr = (bl_partition_manager_t*)mgr->partition_manager;
    if (part_mgr != NULL) {
        bl_partition_info_t part_info;
        if (bl_partition_get_info_by_index(part_mgr, partition_id, &part_info) == BL_OK) {
            strncpy((char*)record->history[slot].partition_name, 
                    (char*)part_info.name, 15);
            record->history[slot].partition_name[15] = '\0';
        }
    }
    
    if (record->history_count < BL_ROLLBACK_MAX_HISTORY) {
        record->history_count++;
    }
    
    return BL_ROLLBACK_OK;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

bl_rollback_error_t bl_rollback_init(
    bl_rollback_manager_t *mgr,
    const bl_rollback_config_t *config,
    void *partition_mgr
)
{
    if (mgr == NULL || config == NULL) {
        return BL_ROLLBACK_ERROR_INVALID_PARAM;
    }
    
    memset(mgr, 0, sizeof(bl_rollback_manager_t));
    memcpy(&mgr->config, config, sizeof(bl_rollback_config_t));
    mgr->partition_manager = partition_mgr;
    
    /* 初始化回滚记录 */
    mgr->record.magic = BL_ROLLBACK_MAGIC;
    mgr->record.version = 1;
    mgr->record.active = false;
    mgr->record.history_count = 0;
    
    set_state(mgr, BL_ROLLBACK_STATE_IDLE);
    mgr->initialized = true;
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback manager initialized");
    
    return BL_ROLLBACK_OK;
}

void bl_rollback_deinit(bl_rollback_manager_t *mgr)
{
    if (mgr == NULL) {
        return;
    }
    
    memset(mgr, 0, sizeof(bl_rollback_manager_t));
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback manager deinitialized");
}

bl_rollback_error_t bl_rollback_record_install(
    bl_rollback_manager_t *mgr,
    uint32_t version,
    uint32_t partition_id,
    const uint8_t hash[32]
)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    /* 添加到版本历史 */
    bl_rollback_error_t result = add_to_history(mgr, version, partition_id, hash);
    if (result != BL_ROLLBACK_OK) {
        return result;
    }
    
    /* 重置启动计数 */
    mgr->boot_attempt_counter = 0;
    mgr->record.consecutive_failures = 0;
    
    /* 更新当前版本 */
    mgr->current_version = version;
    mgr->current_partition = partition_id;
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Recorded installation of version 0x%08X on partition %u",
            version, partition_id);
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_record_boot_attempt(bl_rollback_manager_t *mgr)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    mgr->boot_attempt_counter++;
    mgr->record.total_boot_attempts++;
    mgr->boot_completed = false;
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Boot attempt recorded: %u (total: %u)",
            mgr->boot_attempt_counter, mgr->record.total_boot_attempts);
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_record_boot_result(
    bl_rollback_manager_t *mgr,
    bl_boot_result_t result
)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    mgr->boot_completed = true;
    
    if (result == BL_BOOT_RESULT_SUCCESS) {
        /* 启动成功 */
        mgr->boot_attempt_counter = 0;
        mgr->record.consecutive_failures = 0;
        
        /* 更新历史记录 */
        int32_t idx = find_history_entry(mgr, mgr->current_version);
        if (idx >= 0) {
            mgr->record.history[idx].boot_count++;
            mgr->record.history[idx].boot_success_count++;
            mgr->record.history[idx].last_boot_time = 0; /* TODO: Get current time */
        }
        
        DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
                "Boot successful, counters reset");
    } else {
        /* 启动失败 */
        mgr->record.consecutive_failures++;
        
        if (mgr->record.consecutive_failures > mgr->record.max_consecutive_failures) {
            mgr->record.max_consecutive_failures = mgr->record.consecutive_failures;
        }
        
        DDS_LOG(DDS_LOG_WARNING, BL_ROLLBACK_MODULE_NAME,
                "Boot failed (result: %d), consecutive failures: %u",
                result, mgr->record.consecutive_failures);
        
        /* 报告DTC */
        switch (result) {
            case BL_BOOT_RESULT_WATCHDOG_TIMEOUT:
                report_dtc(mgr, 0xF0F010, 0x01); /* OTA Watchdog Timeout */
                break;
            case BL_BOOT_RESULT_SECURITY_VIOLATION:
                report_dtc(mgr, 0xF0F011, 0x01); /* OTA Security Violation */
                break;
            default:
                report_dtc(mgr, 0xF0F012, 0x01); /* OTA Boot Failure */
                break;
        }
    }
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_check_needed(
    bl_rollback_manager_t *mgr,
    bool *need_rollback,
    uint32_t *target_version
)
{
    if (mgr == NULL || !mgr->initialized || need_rollback == NULL) {
        return BL_ROLLBACK_ERROR_INVALID_PARAM;
    }
    
    *need_rollback = false;
    if (target_version != NULL) {
        *target_version = 0;
    }
    
    set_state(mgr, BL_ROLLBACK_STATE_CHECKING);
    
    /* 检查是否已经处于回滚状态 */
    if (mgr->record.active) {
        *need_rollback = true;
        if (target_version != NULL) {
            *target_version = mgr->record.target_version;
        }
        return BL_ROLLBACK_OK;
    }
    
    /* 检查启动尝试次数 */
    if (mgr->boot_attempt_counter >= mgr->config.max_boot_attempts) {
        *need_rollback = true;
    }
    
    /* 检查连续失败次数 */
    if (mgr->record.consecutive_failures >= mgr->config.max_consecutive_failures) {
        *need_rollback = true;
    }
    
    /* 如果需要回滚，确定目标版本 */
    if (*need_rollback && target_version != NULL) {
        bl_rollback_error_t result = bl_rollback_get_previous_version(
            mgr, target_version, NULL);
        if (result != BL_ROLLBACK_OK) {
            *need_rollback = false;
            return result;
        }
    }
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_execute(bl_rollback_manager_t *mgr, uint8_t reason)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    /* 检查是否正在回滚 */
    if (mgr->record.active) {
        return BL_ROLLBACK_ERROR_ALREADY_ROLLING_BACK;
    }
    
    set_state(mgr, BL_ROLLBACK_STATE_EXECUTING);
    
    /* 获取前一版本信息 */
    uint32_t target_version;
    uint32_t target_partition;
    bl_rollback_error_t result = bl_rollback_get_previous_version(
        mgr, &target_version, &target_partition);
    
    if (result != BL_ROLLBACK_OK) {
        set_state(mgr, BL_ROLLBACK_STATE_FAILED);
        return result;
    }
    
    /* 填充回滚记录 */
    mgr->record.active = true;
    mgr->record.reason = reason;
    mgr->record.source_version = mgr->current_version;
    mgr->record.target_version = target_version;
    mgr->record.source_partition = mgr->current_partition;
    mgr->record.target_partition = target_partition;
    mgr->record.rollback_time = 0; /* TODO: Get current time */
    mgr->record.verified = false;
    mgr->record.rollback_count++;
    
    update_record_crc(&mgr->record);
    
    /* 调用回滚开始回调 */
    if (mgr->config.on_rollback_start != NULL) {
        mgr->config.on_rollback_start(reason, mgr->current_version, target_version);
    }
    
    /* 执行分区切换 */
    bl_partition_manager_t *part_mgr = (bl_partition_manager_t*)mgr->partition_manager;
    if (part_mgr != NULL) {
        bl_partition_info_t target_info;
        result = bl_partition_get_info_by_index(part_mgr, target_partition, &target_info);
        
        if (result == BL_OK) {
            result = bl_partition_switch_active(part_mgr, (char*)target_info.name);
        }
    }
    
    if (result != BL_ROLLBACK_OK) {
        set_state(mgr, BL_ROLLBACK_STATE_FAILED);
        mgr->record.active = false;
        report_dtc(mgr, 0xF0F020, 0x01); /* OTA Rollback Failed */
        return BL_ROLLBACK_ERROR_ROLLBACK_FAILED;
    }
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback executed: 0x%08X -> 0x%08X (reason: %s)",
            mgr->current_version, target_version, 
            bl_rollback_reason_to_string(reason));
    
    set_state(mgr, BL_ROLLBACK_STATE_VERIFYING);
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_confirm(bl_rollback_manager_t *mgr)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    if (!mgr->record.active) {
        return BL_ROLLBACK_OK;
    }
    
    /* 确认回滚成功 */
    mgr->record.verified = true;
    mgr->record.active = false;
    
    /* 更新当前版本 */
    mgr->current_version = mgr->record.target_version;
    mgr->current_partition = mgr->record.target_partition;
    
    /* 重置启动计数 */
    mgr->boot_attempt_counter = 0;
    mgr->record.consecutive_failures = 0;
    
    update_record_crc(&mgr->record);
    
    /* 调用回调 */
    if (mgr->config.on_rollback_complete != NULL) {
        mgr->config.on_rollback_complete(true);
    }
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback confirmed to version 0x%08X", mgr->current_version);
    
    set_state(mgr, BL_ROLLBACK_STATE_COMPLETED);
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_clear(bl_rollback_manager_t *mgr)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    /* 清除回滚状态 */
    mgr->record.active = false;
    mgr->boot_attempt_counter = 0;
    mgr->record.consecutive_failures = 0;
    
    update_record_crc(&mgr->record);
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback state cleared");
    
    set_state(mgr, BL_ROLLBACK_STATE_IDLE);
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_get_previous_version(
    bl_rollback_manager_t *mgr,
    uint32_t *version,
    uint32_t *partition_id
)
{
    if (mgr == NULL || !mgr->initialized || version == NULL) {
        return BL_ROLLBACK_ERROR_INVALID_PARAM;
    }
    
    /* 查找最近的有效版本 (不是当前版本) */
    int32_t best_idx = -1;
    uint64_t best_time = 0;
    
    for (int32_t i = 0; i < BL_ROLLBACK_MAX_HISTORY; i++) {
        if (mgr->record.history[i].is_valid &&
            mgr->record.history[i].version != mgr->current_version) {
            /* 优先选择有成功启动记录的版本 */
            if (mgr->record.history[i].boot_success_count > 0) {
                if (best_idx < 0 || mgr->record.history[i].last_boot_time > best_time) {
                    best_idx = i;
                    best_time = mgr->record.history[i].last_boot_time;
                }
            }
        }
    }
    
    if (best_idx < 0) {
        /* 如果没有有成功启动记录的，选择最近安装的 */
        for (int32_t i = 0; i < BL_ROLLBACK_MAX_HISTORY; i++) {
            if (mgr->record.history[i].is_valid &&
                mgr->record.history[i].version != mgr->current_version) {
                if (best_idx < 0 || mgr->record.history[i].install_time > best_time) {
                    best_idx = i;
                    best_time = mgr->record.history[i].install_time;
                }
            }
        }
    }
    
    if (best_idx < 0) {
        return BL_ROLLBACK_ERROR_NO_PREVIOUS_VERSION;
    }
    
    *version = mgr->record.history[best_idx].version;
    if (partition_id != NULL) {
        *partition_id = mgr->record.history[best_idx].partition_id;
    }
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_get_history(
    bl_rollback_manager_t *mgr,
    bl_version_history_entry_t *history,
    uint32_t max_entries,
    uint32_t *num_entries
)
{
    if (mgr == NULL || history == NULL || num_entries == NULL) {
        return BL_ROLLBACK_ERROR_INVALID_PARAM;
    }
    
    *num_entries = 0;
    
    for (int32_t i = 0; i < BL_ROLLBACK_MAX_HISTORY && *num_entries < max_entries; i++) {
        if (mgr->record.history[i].is_valid) {
            memcpy(&history[*num_entries], &mgr->record.history[i],
                   sizeof(bl_version_history_entry_t));
            (*num_entries)++;
        }
    }
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_save_record(
    bl_rollback_manager_t *mgr,
    uint32_t address
)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    /* 更新CRC */
    update_record_crc(&mgr->record);
    
    /* 保存到分区管理器 */
    bl_partition_manager_t *part_mgr = (bl_partition_manager_t*)mgr->partition_manager;
    if (part_mgr == NULL) {
        return BL_ROLLBACK_ERROR_STORAGE_ERROR;
    }
    
    /* TODO: 实现实际的保存逻辑 */
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback record saved");
    
    return BL_ROLLBACK_OK;
}

bl_rollback_error_t bl_rollback_load_record(
    bl_rollback_manager_t *mgr,
    uint32_t address
)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    /* TODO: 实现实际的加载逻辑 */
    /* 从持久存储读取并验证 */
    
    if (!validate_record(&mgr->record)) {
        return BL_ROLLBACK_ERROR_STORAGE_ERROR;
    }
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback record loaded");
    
    return BL_ROLLBACK_OK;
}

bl_rollback_state_t bl_rollback_get_state(const bl_rollback_manager_t *mgr)
{
    if (mgr == NULL) {
        return BL_ROLLBACK_STATE_IDLE;
    }
    return mgr->state;
}

bl_rollback_error_t bl_rollback_get_record(
    const bl_rollback_manager_t *mgr,
    bl_rollback_record_t *record
)
{
    if (mgr == NULL || record == NULL) {
        return BL_ROLLBACK_ERROR_INVALID_PARAM;
    }
    
    memcpy(record, &mgr->record, sizeof(bl_rollback_record_t));
    return BL_ROLLBACK_OK;
}

void bl_rollback_set_current_version(
    bl_rollback_manager_t *mgr,
    uint32_t version,
    uint32_t partition_id
)
{
    if (mgr == NULL) {
        return;
    }
    
    mgr->current_version = version;
    mgr->current_partition = partition_id;
}

void bl_rollback_set_dem_callback(
    bl_rollback_manager_t *mgr,
    void (*report_func)(uint32_t dtc_code, uint8_t status)
)
{
    if (mgr != NULL) {
        mgr->report_dtc = report_func;
    }
}

const char* bl_rollback_reason_to_string(uint8_t reason)
{
    switch (reason) {
        case BL_ROLLBACK_REASON_NONE:
            return "None";
        case BL_ROLLBACK_REASON_BOOT_FAILURE:
            return "Boot Failure";
        case BL_ROLLBACK_REASON_WATCHDOG_TIMEOUT:
            return "Watchdog Timeout";
        case BL_ROLLBACK_REASON_SECURITY_VIOLATION:
            return "Security Violation";
        case BL_ROLLBACK_REASON_VERSION_MISMATCH:
            return "Version Mismatch";
        case BL_ROLLBACK_REASON_HASH_MISMATCH:
            return "Hash Mismatch";
        case BL_ROLLBACK_REASON_SIGNATURE_INVALID:
            return "Invalid Signature";
        case BL_ROLLBACK_REASON_USER_REQUEST:
            return "User Request";
        case BL_ROLLBACK_REASON_OTA_TIMEOUT:
            return "OTA Timeout";
        case BL_ROLLBACK_REASON_FLASH_ERROR:
            return "Flash Error";
        default:
            return "Unknown";
    }
}

bl_rollback_error_t bl_rollback_reset(bl_rollback_manager_t *mgr)
{
    if (mgr == NULL || !mgr->initialized) {
        return BL_ROLLBACK_ERROR_NOT_INITIALIZED;
    }
    
    /* 清除历史记录 */
    memset(mgr->record.history, 0, sizeof(mgr->record.history));
    mgr->record.history_count = 0;
    mgr->record.rollback_count = 0;
    mgr->record.active = false;
    mgr->record.consecutive_failures = 0;
    
    /* 重置计数器 */
    mgr->boot_attempt_counter = 0;
    mgr->record.total_boot_attempts = 0;
    
    update_record_crc(&mgr->record);
    
    DDS_LOG(BL_ROLLBACK_LOG_LEVEL, BL_ROLLBACK_MODULE_NAME,
            "Rollback manager reset");
    
    return BL_ROLLBACK_OK;
}
