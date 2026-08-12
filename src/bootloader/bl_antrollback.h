/**
 * @file bl_antrollback.h
 * @brief Bootloader Anti-Rollback Counter Module
 * @version 1.0
 * @date 2026-08-12
 *
 * 抗回滚计数器 (RS-OTA-01 / GB 44496-2024, UNECE R156 对齐):
 * - 单调递增计数器, 持久化于独立 NVM 区, 只增不减
 * - 多槽位磨损均衡 (SHOULD): 写入轮转至不同槽位, 延长 NVM 寿命
 * - 槽位记录带 CRC32 完整性保护; 任一槽位损坏不影响其余槽位恢复
 * - 启动验签时: 固件版本 < 计数器 → 拒绝启动 (BL_SB_ERROR_ROLLBACK_PROTECTION)
 *
 * 生产集成要求: base_address 需按 Flash 扇区对齐, NVM 区域大小应 >=
 * slots * 扇区大小 (每个槽位一个扇区), 以保证擦除/写入的硬件正确性。
 * ASIL-D Safety Level
 */

#ifndef BL_ANTIROLLBACK_H
#define BL_ANTIROLLBACK_H

#include <stdint.h>
#include <stdbool.h>
#include "bl_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define BL_ANTIROLLBACK_MAJOR_VERSION       1
#define BL_ANTIROLLBACK_MINOR_VERSION       0
#define BL_ANTIROLLBACK_PATCH_VERSION       0

/* ============================================================================
 * 配置常量
 * ============================================================================ */
#define BL_ANTIROLLBACK_MAX_SLOTS           8U   /* 磨损均衡槽位数上限 */
#define BL_ANTIROLLBACK_DEFAULT_SLOTS       4U   /* 默认槽位数 */
#define BL_ANTIROLLBACK_MAGIC               0x41524243U  /* "ARBC" */
#define BL_ANTIROLLBACK_RECORD_VERSION      1U

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    BL_ANTIROLLBACK_OK = 0,
    BL_ANTIROLLBACK_ERROR_INVALID_PARAM = -1,      /* 非法参数 */
    BL_ANTIROLLBACK_ERROR_NOT_INITIALIZED = -2,    /* 未初始化 */
    BL_ANTIROLLBACK_ERROR_STORAGE_ERROR = -3,      /* NVM 读写/擦除失败 */
    BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT = -4,  /* 检测到写入值低于当前值 (回滚攻击) */
    BL_ANTIROLLBACK_ERROR_INVALID_RECORD = -5,     /* NVM 记录损坏 (魔数/CRC 不符) */
    BL_ANTIROLLBACK_ERROR_COUNTER_FULL = -6        /* 计数器达到最大值 (UINT32_MAX) */
} bl_antrollback_error_t;

/* ============================================================================
 * 槽位记录结构 (每槽一份, 独立 CRC 保护)
 * ============================================================================ */
typedef struct {
    uint32_t magic;             /* 魔数 BL_ANTIROLLBACK_MAGIC */
    uint32_t record_version;    /* 记录格式版本 */
    uint32_t counter;           /* 计数器值 */
    uint32_t write_seq;         /* 全局写入序号 (磨损均衡/最新判定) */
    uint32_t crc32;             /* 前 4 字段的 CRC32 */
} bl_antrollback_slot_t;

/* ============================================================================
 * 抗回滚计数器上下文
 * ============================================================================ */
typedef struct {
    uint32_t slots;             /* 磨损均衡槽位数 (<= BL_ANTIROLLBACK_MAX_SLOTS) */
    uint32_t base_address;      /* NVM 区域基址 */
    const bl_flash_driver_t *flash;  /* Flash 驱动 */
    uint32_t counter;           /* 当前计数器值 (RAM 快照) */
    uint32_t active_slot;       /* 最近写入的槽位 */
    uint32_t write_seq;         /* 最近写入序号 */
    bool     initialized;       /* 初始化标志 */
} bl_antrollback_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化抗回滚计数器
 * @details 扫描全部槽位, 以最大写入序号(次为计数器值)的合法记录恢复当前值;
 *          全部槽位无效时从 0 开始。
 * @param ctx 上下文
 * @param flash Flash 驱动 (NVM 访问)
 * @param base_address NVM 区域基址 (需扇区对齐)
 * @param slots 磨损均衡槽位数 (1..BL_ANTIROLLBACK_MAX_SLOTS; 0 取默认)
 * @return BL_ANTIROLLBACK_OK 成功
 */
bl_antrollback_error_t Boot_AntiRollback_Init(
    bl_antrollback_context_t *ctx,
    const bl_flash_driver_t *flash,
    uint32_t base_address,
    uint32_t slots
);

/**
 * @brief 反初始化抗回滚计数器
 * @param ctx 上下文
 */
void Boot_AntiRollback_Deinit(bl_antrollback_context_t *ctx);

/**
 * @brief 读取当前计数器值
 * @param ctx 上下文
 * @param counter 输出计数器值
 * @return BL_ANTIROLLBACK_OK 成功
 */
bl_antrollback_error_t Boot_AntiRollback_Read(
    const bl_antrollback_context_t *ctx,
    uint32_t *counter
);

/**
 * @brief 写入计数器值 (单调递增)
 * @details 值低于当前值 → BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT (防回滚攻击);
 *          等于当前值允许 (幂等重写, 用于磨损均衡)。
 *          写入轮转到下一槽位: 擦除 → 编程 → 读回校验。
 * @param ctx 上下文
 * @param counter 新计数器值
 * @return BL_ANTIROLLBACK_OK 成功
 */
bl_antrollback_error_t Boot_AntiRollback_Write(
    bl_antrollback_context_t *ctx,
    uint32_t counter
);

/**
 * @brief 计数器递增 (当前值 + 1, 原子语义由调用方保证单线程)
 * @param ctx 上下文
 * @param new_counter 输出递增后的值 (可为 NULL)
 * @return BL_ANTIROLLBACK_OK 成功
 */
bl_antrollback_error_t Boot_AntiRollback_Increment(
    bl_antrollback_context_t *ctx,
    uint32_t *new_counter
);

#ifdef __cplusplus
}
#endif

#endif /* BL_ANTIROLLBACK_H */
