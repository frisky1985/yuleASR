/**
 * @file bl_antrollback.c
 * @brief Bootloader Anti-Rollback Counter Module Implementation
 * @version 1.0
 * @date 2026-08-12
 *
 * 单调递增抗回滚计数器实现, 持久化于独立 NVM 区 (RS-OTA-01)。
 * 多槽位磨损均衡: 每次写入轮转槽位, 任一槽位损坏不影响其余槽位恢复。
 * UNECE R156 / GB 44496-2024 对齐
 * ASIL-D Safety Level
 */

#include <string.h>
#include <stddef.h>
#include "bl_antrollback.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define BL_ANTIROLLBACK_MODULE_NAME     "BL_ARB"
#define BL_ANTIROLLBACK_LOG_LEVEL       DDS_LOG_LEVEL_INFO

/* 槽位记录 CRC 覆盖长度 (排除 crc32 字段自身) */
#define BL_ANTIROLLBACK_SLOT_CRC_SIZE   (uint32_t)offsetof(bl_antrollback_slot_t, crc32)

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
 * @brief 校验槽位记录完整性 (魔数 + CRC32)
 */
static bool validate_slot(const bl_antrollback_slot_t *slot)
{
    if (slot->magic != BL_ANTIROLLBACK_MAGIC) {
        return false;
    }
    if (slot->record_version != BL_ANTIROLLBACK_RECORD_VERSION) {
        return false;
    }
    return (calculate_crc32((const uint8_t *)slot, BL_ANTIROLLBACK_SLOT_CRC_SIZE)
            == slot->crc32);
}

/**
 * @brief 读取槽位记录
 */
static bl_antrollback_error_t read_slot(
    const bl_antrollback_context_t *ctx,
    uint32_t slot_index,
    bl_antrollback_slot_t *slot
)
{
    uint32_t slot_addr = ctx->base_address
                         + slot_index * (uint32_t)sizeof(bl_antrollback_slot_t);
    int32_t result = ctx->flash->read(slot_addr, (uint8_t *)slot,
                                      (uint32_t)sizeof(bl_antrollback_slot_t));
    if (result != 0) {
        return BL_ANTIROLLBACK_ERROR_STORAGE_ERROR;
    }
    return BL_ANTIROLLBACK_OK;
}

/**
 * @brief 扫描全部槽位, 恢复当前计数器值
 */
static bl_antrollback_error_t scan_slots(bl_antrollback_context_t *ctx)
{
    bl_antrollback_slot_t slot;
    bool found = false;
    uint32_t best_seq = 0U;
    uint32_t best_counter = 0U;
    uint32_t best_slot = 0U;

    for (uint32_t i = 0U; i < ctx->slots; i++) {
        bl_antrollback_error_t result = read_slot(ctx, i, &slot);
        if (result != BL_ANTIROLLBACK_OK) {
            DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_ANTIROLLBACK_MODULE_NAME,
                    "Failed to read slot %u", i);
            continue;
        }
        if (!validate_slot(&slot)) {
            /* 槽位无效 (擦除态/损坏) — 跳过, 不阻塞恢复 */
            DDS_LOG(DDS_LOG_LEVEL_WARN, BL_ANTIROLLBACK_MODULE_NAME,
                    "Slot %u invalid (magic/CRC), skipped", i);
            continue;
        }
        /* 以最大写入序号为准; 序号相同取较大计数器值 */
        if ((!found) || (slot.write_seq > best_seq) ||
            ((slot.write_seq == best_seq) && (slot.counter > best_counter))) {
            found = true;
            best_seq = slot.write_seq;
            best_counter = slot.counter;
            best_slot = i;
        }
    }

    if (!found) {
        /* 全新 NVM / 全部槽位无效: 从 0 开始 */
        ctx->counter = 0U;
        ctx->active_slot = 0U;
        ctx->write_seq = 0U;
        DDS_LOG(BL_ANTIROLLBACK_LOG_LEVEL, BL_ANTIROLLBACK_MODULE_NAME,
                "No valid slot found, counter starts at 0");
        return BL_ANTIROLLBACK_OK;
    }

    ctx->counter = best_counter;
    ctx->active_slot = best_slot;
    ctx->write_seq = best_seq;

    DDS_LOG(BL_ANTIROLLBACK_LOG_LEVEL, BL_ANTIROLLBACK_MODULE_NAME,
            "Counter restored: %u (slot %u, seq %u)",
            ctx->counter, ctx->active_slot, ctx->write_seq);

    return BL_ANTIROLLBACK_OK;
}

/* ============================================================================
 * API函数实现
 * ============================================================================ */

bl_antrollback_error_t Boot_AntiRollback_Init(
    bl_antrollback_context_t *ctx,
    const bl_flash_driver_t *flash,
    uint32_t base_address,
    uint32_t slots
)
{
    if ((ctx == NULL) || (flash == NULL) || (flash->read == NULL) ||
        (flash->erase == NULL) || (flash->program == NULL)) {
        return BL_ANTIROLLBACK_ERROR_INVALID_PARAM;
    }

    memset(ctx, 0, sizeof(bl_antrollback_context_t));

    if (slots == 0U) {
        slots = BL_ANTIROLLBACK_DEFAULT_SLOTS;
    }
    if (slots > BL_ANTIROLLBACK_MAX_SLOTS) {
        return BL_ANTIROLLBACK_ERROR_INVALID_PARAM;
    }

    ctx->slots = slots;
    ctx->base_address = base_address;
    ctx->flash = flash;

    bl_antrollback_error_t result = scan_slots(ctx);
    if (result != BL_ANTIROLLBACK_OK) {
        return result;
    }

    ctx->initialized = true;

    DDS_LOG(BL_ANTIROLLBACK_LOG_LEVEL, BL_ANTIROLLBACK_MODULE_NAME,
            "Anti-rollback counter initialized at 0x%08X (%u slots)",
            base_address, slots);

    return BL_ANTIROLLBACK_OK;
}

void Boot_AntiRollback_Deinit(bl_antrollback_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(bl_antrollback_context_t));

    DDS_LOG(BL_ANTIROLLBACK_LOG_LEVEL, BL_ANTIROLLBACK_MODULE_NAME,
            "Anti-rollback counter deinitialized");
}

bl_antrollback_error_t Boot_AntiRollback_Read(
    const bl_antrollback_context_t *ctx,
    uint32_t *counter
)
{
    if ((ctx == NULL) || (counter == NULL)) {
        return BL_ANTIROLLBACK_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED;
    }

    *counter = ctx->counter;
    return BL_ANTIROLLBACK_OK;
}

bl_antrollback_error_t Boot_AntiRollback_Write(
    bl_antrollback_context_t *ctx,
    uint32_t counter
)
{
    if (ctx == NULL) {
        return BL_ANTIROLLBACK_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED;
    }

    /* 单调递增: 拒绝回退 (防回滚攻击) */
    if (counter < ctx->counter) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_ANTIROLLBACK_MODULE_NAME,
                "Decrement attempt rejected: %u < %u", counter, ctx->counter);
        return BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT;
    }

    if (counter == UINT32_MAX) {
        /* 已达最大值: 再递增将溢出 */
        if (ctx->counter == UINT32_MAX) {
            return BL_ANTIROLLBACK_ERROR_COUNTER_FULL;
        }
    }

    /* 磨损均衡: 轮转到下一槽位 */
    uint32_t next_slot = (ctx->active_slot + 1U) % ctx->slots;
    uint32_t slot_addr = ctx->base_address
                         + next_slot * (uint32_t)sizeof(bl_antrollback_slot_t);

    bl_antrollback_slot_t slot;
    memset(&slot, 0, sizeof(slot));
    slot.magic = BL_ANTIROLLBACK_MAGIC;
    slot.record_version = BL_ANTIROLLBACK_RECORD_VERSION;
    slot.counter = counter;
    slot.write_seq = ctx->write_seq + 1U;
    slot.crc32 = calculate_crc32((const uint8_t *)&slot, BL_ANTIROLLBACK_SLOT_CRC_SIZE);

    /* 擦除目标槽位 */
    if (ctx->flash->erase(slot_addr, (uint32_t)sizeof(bl_antrollback_slot_t)) != 0) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_ANTIROLLBACK_MODULE_NAME,
                "Failed to erase slot at 0x%08X", slot_addr);
        return BL_ANTIROLLBACK_ERROR_STORAGE_ERROR;
    }

    /* 编程槽位记录 */
    if (ctx->flash->program(slot_addr, (const uint8_t *)&slot,
                            (uint32_t)sizeof(bl_antrollback_slot_t)) != 0) {
        DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_ANTIROLLBACK_MODULE_NAME,
                "Failed to program slot at 0x%08X", slot_addr);
        return BL_ANTIROLLBACK_ERROR_STORAGE_ERROR;
    }

    /* 读回校验 (可选, 驱动支持时执行) */
    if (ctx->flash->verify != NULL) {
        if (ctx->flash->verify(slot_addr, (const uint8_t *)&slot,
                               (uint32_t)sizeof(bl_antrollback_slot_t)) != 0) {
            DDS_LOG(DDS_LOG_LEVEL_ERROR, BL_ANTIROLLBACK_MODULE_NAME,
                    "Verify failed for slot at 0x%08X", slot_addr);
            return BL_ANTIROLLBACK_ERROR_STORAGE_ERROR;
        }
    }

    ctx->counter = counter;
    ctx->active_slot = next_slot;
    ctx->write_seq = slot.write_seq;

    DDS_LOG(BL_ANTIROLLBACK_LOG_LEVEL, BL_ANTIROLLBACK_MODULE_NAME,
            "Counter written: %u (slot %u, seq %u)",
            counter, ctx->active_slot, ctx->write_seq);

    return BL_ANTIROLLBACK_OK;
}

bl_antrollback_error_t Boot_AntiRollback_Increment(
    bl_antrollback_context_t *ctx,
    uint32_t *new_counter
)
{
    if (ctx == NULL) {
        return BL_ANTIROLLBACK_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED;
    }

    if (ctx->counter == UINT32_MAX) {
        return BL_ANTIROLLBACK_ERROR_COUNTER_FULL;
    }

    uint32_t next = ctx->counter + 1U;
    bl_antrollback_error_t result = Boot_AntiRollback_Write(ctx, next);
    if (result != BL_ANTIROLLBACK_OK) {
        return result;
    }

    if (new_counter != NULL) {
        *new_counter = next;
    }
    return BL_ANTIROLLBACK_OK;
}
