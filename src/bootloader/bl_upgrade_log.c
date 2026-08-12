/**
 * @file bl_upgrade_log.c
 * @brief Bootloader Upgrade Audit Log Module Implementation
 * @version 1.0
 * @date 2026-08-12
 *
 * 升级日志环形缓冲实现 (RS-OTA-03):
 * - 记录: 时间戳 / 版本号 / 来源 / 签名结果 / 结果状态
 * - NVM 持久化 (Save/Load, 经 bl_partition_manager 的 Flash 驱动)
 * - 环形缓冲, 满则覆盖最旧条目; 每条目独立 CRC32 保护
 * UNECE R156 §7.1.1 SUMS / GB 44496-2024 §7.2 对齐
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stddef.h>
#include "bl_upgrade_log.h"
#include "bl_partition.h"
#include "bl_time.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define BL_UPGRADE_LOG_MODULE_NAME      "BL_ULOG"
#define BL_UPGRADE_LOG_LOG_LEVEL        DDS_LOG_LEVEL_INFO

/* 条目 CRC 覆盖长度 (排除 crc32 字段自身) */
#define BL_UPGRADE_LOG_ENTRY_CRC_SIZE   (uint32_t)offsetof(bl_upgrade_log_entry_t, crc32)
/* 头部 CRC 覆盖长度 (排除 crc32 字段自身) */
#define BL_UPGRADE_LOG_HEADER_CRC_SIZE  (uint32_t)offsetof(bl_upgrade_log_header_t, crc32)

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

    for (uint32_t i = 0U; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0U; j < 8U; j++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

/**
 * @brief 校验条目完整性
 */
static bool validate_entry(const bl_upgrade_log_entry_t *entry)
{
    return (calculate_crc32((const uint8_t *)entry, BL_UPGRADE_LOG_ENTRY_CRC_SIZE)
            == entry->crc32);
}

/**
 * @brief 更新条目 CRC
 */
static void update_entry_crc(bl_upgrade_log_entry_t *entry)
{
    entry->crc32 = calculate_crc32((const uint8_t *)entry, BL_UPGRADE_LOG_ENTRY_CRC_SIZE);
}

/**
 * @brief 校验头部完整性
 */
static bool validate_header(const bl_upgrade_log_header_t *header)
{
    if (header->magic != BL_UPGRADE_LOG_MAGIC) {
        return false;
    }
    if (header->record_version != BL_UPGRADE_LOG_RECORD_VERSION) {
        return false;
    }
    if ((header->capacity == 0U) || (header->capacity > BL_UPGRADE_LOG_MAX_ENTRIES)) {
        return false;
    }
    if (header->count > header->capacity) {
        return false;
    }
    return (calculate_crc32((const uint8_t *)header, BL_UPGRADE_LOG_HEADER_CRC_SIZE)
            == header->crc32);
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

bl_upgrade_log_error_t Boot_UpgradeLog_Init(
    bl_upgrade_log_context_t *ctx,
    const bl_upgrade_log_config_t *config
)
{
    if (ctx == NULL) {
        return BL_UPGRADE_LOG_ERROR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(bl_upgrade_log_context_t));

    if (config != NULL) {
        memcpy(&ctx->config, config, sizeof(bl_upgrade_log_config_t));
    }

    /* 容量: 0 → 默认; 超上限 → 拒绝 */
    if (ctx->config.capacity == 0U) {
        ctx->config.capacity = BL_UPGRADE_LOG_DEFAULT_CAPACITY;
    }
    if (ctx->config.capacity > BL_UPGRADE_LOG_MAX_ENTRIES) {
        return BL_UPGRADE_LOG_ERROR_INVALID_PARAM;
    }

    ctx->header.magic = BL_UPGRADE_LOG_MAGIC;
    ctx->header.record_version = BL_UPGRADE_LOG_RECORD_VERSION;
    ctx->header.capacity = ctx->config.capacity;
    ctx->header.count = 0U;
    ctx->header.head = 0U;
    ctx->header.crc32 = 0U;

    ctx->initialized = true;

    DDS_LOG(BL_UPGRADE_LOG_LOG_LEVEL, BL_UPGRADE_LOG_MODULE_NAME,
            "Upgrade log initialized (capacity %u)", ctx->header.capacity);

    return BL_UPGRADE_LOG_OK;
}

void Boot_UpgradeLog_Deinit(bl_upgrade_log_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(bl_upgrade_log_context_t));

    DDS_LOG(BL_UPGRADE_LOG_LOG_LEVEL, BL_UPGRADE_LOG_MODULE_NAME,
            "Upgrade log deinitialized");
}

bl_upgrade_log_error_t Boot_UpgradeLog_Write(
    bl_upgrade_log_context_t *ctx,
    bl_upgrade_log_entry_t *entry
)
{
    if ((ctx == NULL) || (entry == NULL)) {
        return BL_UPGRADE_LOG_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED;
    }

    /* 时间戳必须来自真实时间源 (禁止 0 时间戳充当恒真) */
    uint64_t now_ms = 0U;
    if (!bl_time_get_ms(&now_ms)) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Time source unavailable, log entry not recorded");
        return BL_UPGRADE_LOG_ERROR_TIME_UNAVAILABLE;
    }

    uint32_t capacity = ctx->header.capacity;
    uint32_t idx;

    if (ctx->header.count < capacity) {
        /* 追加到尾部 */
        idx = (ctx->header.head + ctx->header.count) % capacity;
        ctx->header.count++;
    } else {
        /* 环形已满: 覆盖最旧条目 */
        idx = ctx->header.head;
        ctx->header.head = (ctx->header.head + 1U) % capacity;
    }

    /* 填充并写回条目 (时间戳由本模块填充, 保证审计一致性) */
    entry->timestamp_ms = now_ms;
    update_entry_crc(entry);
    memcpy(&ctx->entries[idx], entry, sizeof(bl_upgrade_log_entry_t));

    DDS_LOG(BL_UPGRADE_LOG_LOG_LEVEL, BL_UPGRADE_LOG_MODULE_NAME,
            "Log entry %u: version=0x%08X source=%u sig=%u result=%u",
            idx, entry->version, entry->source,
            entry->signature_result, entry->result);

    return BL_UPGRADE_LOG_OK;
}

bl_upgrade_log_error_t Boot_UpgradeLog_Read(
    const bl_upgrade_log_context_t *ctx,
    uint32_t index,
    bl_upgrade_log_entry_t *entry
)
{
    if ((ctx == NULL) || (entry == NULL)) {
        return BL_UPGRADE_LOG_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED;
    }
    if (index >= ctx->header.count) {
        return BL_UPGRADE_LOG_ERROR_INDEX_OUT_OF_RANGE;
    }

    uint32_t idx = (ctx->header.head + index) % ctx->header.capacity;
    memcpy(entry, &ctx->entries[idx], sizeof(bl_upgrade_log_entry_t));

    /* 条目完整性校验 (诊断读取时发现损坏必须显式报错) */
    if (!validate_entry(entry)) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Log entry %u corrupted (CRC mismatch)", index);
        return BL_UPGRADE_LOG_ERROR_ENTRY_CORRUPTED;
    }

    return BL_UPGRADE_LOG_OK;
}

bl_upgrade_log_error_t Boot_UpgradeLog_GetCount(
    const bl_upgrade_log_context_t *ctx,
    uint32_t *count
)
{
    if ((ctx == NULL) || (count == NULL)) {
        return BL_UPGRADE_LOG_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED;
    }

    *count = ctx->header.count;
    return BL_UPGRADE_LOG_OK;
}

bl_upgrade_log_error_t Boot_UpgradeLog_Clear(bl_upgrade_log_context_t *ctx)
{
    if (ctx == NULL) {
        return BL_UPGRADE_LOG_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED;
    }

    memset(ctx->entries, 0, sizeof(ctx->entries));
    ctx->header.count = 0U;
    ctx->header.head = 0U;

    DDS_LOG(BL_UPGRADE_LOG_LOG_LEVEL, BL_UPGRADE_LOG_MODULE_NAME,
            "Upgrade log cleared");

    return BL_UPGRADE_LOG_OK;
}

bl_upgrade_log_error_t Boot_UpgradeLog_Save(
    bl_upgrade_log_context_t *ctx,
    uint32_t address
)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED;
    }

    bl_partition_manager_t *part_mgr = (bl_partition_manager_t *)ctx->config.storage;
    if ((part_mgr == NULL) || (part_mgr->flash_driver == NULL)) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Storage (partition manager) not available");
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }
    const bl_flash_driver_t *flash = part_mgr->flash_driver;

    /* 更新头部 CRC */
    ctx->header.crc32 = calculate_crc32((const uint8_t *)&ctx->header,
                                        BL_UPGRADE_LOG_HEADER_CRC_SIZE);

    uint32_t total_size = (uint32_t)sizeof(bl_upgrade_log_header_t)
                          + ctx->header.capacity * (uint32_t)sizeof(bl_upgrade_log_entry_t);

    /* 解锁写保护 */
    if (part_mgr->write_protected && (flash->unlock != NULL)) {
        flash->unlock();
    }

    /* 擦除 NVM 区域 */
    if (flash->erase(address, total_size) != 0) {
        if (flash->lock != NULL) {
            flash->lock();
        }
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Failed to erase upgrade log at 0x%08X", address);
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }

    /* 写入头部 + 条目数组 (固定布局: capacity 条目) */
    int32_t result = flash->program(address, (const uint8_t *)&ctx->header,
                                    (uint32_t)sizeof(bl_upgrade_log_header_t));
    if ((result == 0) && (ctx->header.capacity > 0U)) {
        result = flash->program(address + (uint32_t)sizeof(bl_upgrade_log_header_t),
                                (const uint8_t *)ctx->entries,
                                ctx->header.capacity * (uint32_t)sizeof(bl_upgrade_log_entry_t));
    }

    /* 读回验证 */
    if ((result == 0) && (flash->verify != NULL)) {
        result = flash->verify(address, (const uint8_t *)&ctx->header,
                               (uint32_t)sizeof(bl_upgrade_log_header_t));
        if ((result == 0) && (ctx->header.capacity > 0U)) {
            result = flash->verify(
                address + (uint32_t)sizeof(bl_upgrade_log_header_t),
                (const uint8_t *)ctx->entries,
                ctx->header.capacity * (uint32_t)sizeof(bl_upgrade_log_entry_t));
        }
    }

    if (flash->lock != NULL) {
        flash->lock();
    }

    if (result != 0) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Failed to write upgrade log at 0x%08X: %d", address, result);
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }

    DDS_LOG(BL_UPGRADE_LOG_LOG_LEVEL, BL_UPGRADE_LOG_MODULE_NAME,
            "Upgrade log saved to 0x%08X (%u entries)",
            address, ctx->header.count);

    return BL_UPGRADE_LOG_OK;
}

bl_upgrade_log_error_t Boot_UpgradeLog_Load(
    bl_upgrade_log_context_t *ctx,
    uint32_t address
)
{
    if ((ctx == NULL) || !ctx->initialized) {
        return BL_UPGRADE_LOG_ERROR_NOT_INITIALIZED;
    }

    bl_partition_manager_t *part_mgr = (bl_partition_manager_t *)ctx->config.storage;
    if ((part_mgr == NULL) || (part_mgr->flash_driver == NULL)) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Storage (partition manager) not available");
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }

    /* 读取到临时结构, 避免损坏数据污染运行态日志 */
    bl_upgrade_log_header_t loaded_header;
    bl_upgrade_log_entry_t loaded_entries[BL_UPGRADE_LOG_MAX_ENTRIES];
    memset(&loaded_header, 0, sizeof(loaded_header));
    memset(loaded_entries, 0, sizeof(loaded_entries));

    int32_t result = part_mgr->flash_driver->read(
        address, (uint8_t *)&loaded_header, (uint32_t)sizeof(bl_upgrade_log_header_t));
    if (result != 0) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Failed to read upgrade log header at 0x%08X: %d", address, result);
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }

    /* 头部完整性 + 容量一致性校验 */
    if (!validate_header(&loaded_header)) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Invalid upgrade log header at 0x%08X (magic/CRC)", address);
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }
    if (loaded_header.capacity != ctx->header.capacity) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Upgrade log capacity mismatch: %u != %u",
                loaded_header.capacity, ctx->header.capacity);
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }

    /* 读取条目数组 */
    result = part_mgr->flash_driver->read(
        address + (uint32_t)sizeof(bl_upgrade_log_header_t),
        (uint8_t *)loaded_entries,
        loaded_header.capacity * (uint32_t)sizeof(bl_upgrade_log_entry_t));
    if (result != 0) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_UPGRADE_LOG_MODULE_NAME,
                "Failed to read upgrade log entries at 0x%08X: %d", address, result);
        return BL_UPGRADE_LOG_ERROR_STORAGE_ERROR;
    }

    /* 加载到运行态 */
    memcpy(&ctx->header, &loaded_header, sizeof(bl_upgrade_log_header_t));
    memcpy(ctx->entries, loaded_entries, sizeof(loaded_entries));

    DDS_LOG(BL_UPGRADE_LOG_LOG_LEVEL, BL_UPGRADE_LOG_MODULE_NAME,
            "Upgrade log loaded from 0x%08X (%u entries)",
            address, ctx->header.count);

    return BL_UPGRADE_LOG_OK;
}

const char* Boot_UpgradeLog_SourceToString(uint8_t source)
{
    switch (source) {
        case BL_UPGRADE_LOG_SOURCE_OTA:
            return "OTA";
        case BL_UPGRADE_LOG_SOURCE_DIAGNOSTIC:
            return "Diagnostic";
        case BL_UPGRADE_LOG_SOURCE_LOCAL:
            return "Local";
        default:
            return "Unknown";
    }
}

const char* Boot_UpgradeLog_ResultToString(uint8_t result)
{
    switch (result) {
        case BL_UPGRADE_LOG_RESULT_SUCCESS:
            return "Success";
        case BL_UPGRADE_LOG_RESULT_FAILED:
            return "Failed";
        case BL_UPGRADE_LOG_RESULT_ABORTED:
            return "Aborted";
        case BL_UPGRADE_LOG_RESULT_ROLLBACK:
            return "Rollback";
        case BL_UPGRADE_LOG_RESULT_TIMEOUT:
            return "Timeout";
        default:
            return "Unknown";
    }
}
