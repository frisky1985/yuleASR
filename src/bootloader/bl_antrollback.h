/**
 * @file bl_antrollback.h
 * @brief Bootloader Anti-Rollback Counter Module
 * @version 1.1
 * @date 2026-08-12
 *
 * 抗回滚计数器 (RS-OTA-01 / GB 44496-2024, UNECE R156 对齐):
 * - 单调递增计数器, 持久化于独立 NVM 区, 只增不减
 * - 多槽位磨损均衡 (SHOULD): 写入轮转至不同槽位, 延长 NVM 寿命
 * - 槽位记录带 CRC32 完整性保护; 任一槽位损坏不影响其余槽位恢复
 * - 启动验签时: 固件版本 < 计数器 → 拒绝启动 (BL_SB_ERROR_ROLLBACK_PROTECTION)
 * - 延后递增策略 (P1-4): 新版本先 Stage (记录待确认版本, 不提升计数器),
 *   成功启动 N 次 (SetConfirmBoots 可配) 后 NotifySuccessfulBoot 才提交
 *   counter = pending (平衡安全与回滚: 确认窗口内允许回滚到旧版本/同版本重装)
 *
 * 记录格式 v3: 槽位新增 pending_counter/pending_boot_count (v2) 与
 * health_confirmed (v3, 业务健康确认标记)。v2 记录兼容读: v3 代码可恢复
 * v2 布局的 NVM (计数器/待确认状态), 此时 health_confirmed 置 false
 * (健康门控关闭语义)。v1 记录不兼容 (升级后首次初始化从 0 开始,
 * 属预期格式变更, 见 BL_ANTIROLLBACK_RECORD_VERSION)。
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
#include "bl_rollback_storage.h"   /* 跨层抗回滚存储服务接口 (共享 include 树) */

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
/* Configurator 驱动（Boot_Cfg.h 生成 OTA 安全配置）：
 * BOOT_ROLLBACK_CONFIRM_BOOTS 由 yuleASR-Configurator 生成（OtaSecurity 容器）。
 * 此处只读不写（避免先于 Boot_Cfg.h 被 include 时重定义宏）。 */
#ifdef BOOT_ROLLBACK_CONFIRM_BOOTS
#define BL_ANTIROLLBACK_DEFAULT_CONFIRM_BOOTS  BOOT_ROLLBACK_CONFIRM_BOOTS  /* 延后递增默认阈值 N */
#else
#define BL_ANTIROLLBACK_DEFAULT_CONFIRM_BOOTS  3U
#endif
#define BL_ANTIROLLBACK_MAGIC               0x41524243U  /* "ARBC" */
#define BL_ANTIROLLBACK_RECORD_VERSION      3U   /* v3: 槽位含 pending 状态 + health_confirmed */
#define BL_ANTIROLLBACK_RECORD_VERSION_V2   2U   /* v2: 槽位含 pending 状态 (无 health_confirmed, 兼容读) */

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
    uint32_t record_version;    /* 记录格式版本 (v3: 含待确认状态 + 健康确认标记) */
    uint32_t counter;           /* 已确认的抗回滚计数器 (地板) */
    uint32_t write_seq;         /* 全局写入序号 (磨损均衡/最新判定) */
    uint32_t pending_counter;   /* 待确认版本 (0 = 无待确认升级) */
    uint32_t pending_boot_count;/* 新版本成功启动次数 (达到阈值后提交) */
    uint32_t health_confirmed;  /* v3: 业务健康确认标记 (0/1; 健康门控开启时提交需此位) */
    uint32_t crc32;             /* 前 7 字段的 CRC32 (v2 记录为前 6 字段, 兼容读) */
} bl_antrollback_slot_t;

/* ============================================================================
 * 抗回滚计数器上下文
 * ============================================================================ */
typedef struct {
    uint32_t slots;             /* 磨损均衡槽位数 (<= BL_ANTIROLLBACK_MAX_SLOTS) */
    uint32_t base_address;      /* NVM 区域基址 */
    const bl_flash_driver_t *flash;  /* Flash 驱动 */
    uint32_t counter;           /* 当前已确认计数器值 (RAM 快照) */
    uint32_t active_slot;       /* 最近写入的槽位 */
    uint32_t write_seq;         /* 最近写入序号 */
    uint32_t confirm_boots;     /* 延后递增阈值 N (0 由 Init 置默认) */
    uint32_t pending_counter;   /* 待确认版本 (RAM 快照, 0 = 无) */
    uint32_t pending_boot_count;/* 新版本成功启动次数 (RAM 快照) */
    bool     health_check_enabled; /* 健康门控 (RS-OTA-05, 运行期配置, 默认关闭) */
    bool     health_confirmed;  /* 待确认版本的业务健康确认标记 (RAM 快照) */
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

/**
 * @brief 设置延后递增阈值 N (P1-4: 新版本成功启动 N 次后才提交计数器)
 * @details N=0 时恢复默认 (BL_ANTIROLLBACK_DEFAULT_CONFIRM_BOOTS);
 *          N=1 表示首次成功启动即提交 (等同于立即递增, 不推荐)。
 *          阈值不持久化 (运行期配置, 由集成层在每次启动时设置)。
 * @param ctx 上下文
 * @param boots 成功启动次数阈值 (0 = 默认)
 */
void Boot_AntiRollback_SetConfirmBoots(bl_antrollback_context_t *ctx, uint32_t boots);

/**
 * @brief 延后递增-阶段 1: 记录待确认升级版本 (不提升计数器)
 * @details 仅当 version > 当前计数器时接受; 记录 pending=version,
 *          boot_count=0 并持久化。计数器保持不动 → 确认窗口内
 *          仍可回滚到旧版本 / 同版本重装 (P1-4 平衡安全与回滚)。
 *          再次 Stage 覆盖旧 pending (以最新升级为准)。
 * @param ctx 上下文
 * @param version 新固件版本号
 * @return BL_ANTIROLLBACK_OK 成功; BL_ANTIROLLBACK_ERROR_DECREMENT_ATTEMPT
 *         版本 <= 当前计数器 (低于地板, 拒绝); 存储错误透传
 */
bl_antrollback_error_t Boot_AntiRollback_Stage(
    bl_antrollback_context_t *ctx,
    uint32_t version
);

/**
 * @brief 延后递增-阶段 2: 上报一次成功启动
 * @details 仅当 current_version == pending_counter 时计数;
 *          达到阈值 (confirm_boots) 后提交: counter = pending, 清除 pending。
 *          current_version != pending_counter (启动其他版本, 如 A/B 回退)
 *          不计数也不清除 pending (槽位可能仍保留新版本, 待下次启动确认)。
 * @param ctx 上下文
 * @param current_version 本次成功启动的固件版本
 * @return BL_ANTIROLLBACK_OK 成功; 存储错误透传
 */
bl_antrollback_error_t Boot_AntiRollback_NotifySuccessfulBoot(
    bl_antrollback_context_t *ctx,
    uint32_t current_version
);

/**
 * @brief 使能/关闭健康门控 (RS-OTA-05 P1: 主动业务健康检查钩子)
 * @details 运行期配置 (不持久化), 默认关闭 = 原 N 次制向后兼容:
 *          门控关闭时提交条件仅为 pending_boot_count >= confirm_boots;
 *          门控开启时还需 health_confirmed == true (App 业务健康确认)。
 * @param ctx 上下文
 * @param enabled TRUE=开启健康门控; FALSE=关闭 (默认)
 */
void Boot_AntiRollback_SetHealthCheckMode(bl_antrollback_context_t *ctx, bool enabled);

/**
 * @brief App 业务健康确认 (RS-OTA-05 P1)
 * @details 由 ASW 层业务健康检查 (DCM 响应/传感器有效/无功能异常) 通过后调用:
 *          - ok==true 且 version == pending_counter → 置 health_confirmed=true 并落盘;
 *            version 不匹配 → 不落盘 (防误确认); 无 pending → 无操作。
 *          - ok==false → 置 health_confirmed=false 落盘 (pending 保持,
 *            等待 watchdog/boot_attempt 超限自动回滚)。
 *          确认标记随槽位记录持久化, 跨重启恢复。
 * @param ctx 上下文
 * @param version 本次健康检查对应的固件版本
 * @param ok TRUE=业务健康; FALSE=业务异常
 * @return BL_ANTIROLLBACK_OK 成功; 存储错误透传
 */
bl_antrollback_error_t Boot_AntiRollback_ConfirmHealth(
    bl_antrollback_context_t *ctx,
    uint32_t version,
    bool ok
);

/**
 * @brief 查询当前待确认版本的业务健康确认标记 (MAY, 诊断/测试用)
 * @param ctx 上下文
 * @return TRUE=已确认健康; FALSE=未确认/无 pending/未初始化
 */
bool Boot_AntiRollback_IsHealthConfirmed(const bl_antrollback_context_t *ctx);

/**
 * @brief 查询待确认升级状态 (启动流程/诊断用)
 * @param ctx 上下文
 * @param version 输出待确认版本 (0 = 无待确认升级)
 * @param count 输出已成功启动次数 (可为 NULL)
 * @return BL_ANTIROLLBACK_OK 成功
 */
bl_antrollback_error_t Boot_AntiRollback_GetPending(
    const bl_antrollback_context_t *ctx,
    uint32_t *version,
    uint32_t *count
);

/* ============================================================================
 * 抗回滚存储服务接口实现 (方案 C: 接口抽象, 供 Boot_Update 注入使用)
 * ============================================================================ */

/**
 * @brief 获取抗回滚存储服务接口 (bl_rollback_storage_api_t 实现)
 * @details 返回静态函数表, ctx 参数为 bl_antrollback_context_t*。
 *          供集成层注入给 bsw/boot 层 Boot_Update
 *          (Boot_Update_SetAntiRollbackStorage), 使 Boot_Update 经
 *          回调访问本 NVM 计数器 — 计数器单一事实源, 分层解耦。
 *          表内函数与 bl_antrollback API 一一对应 (见 bl_antrollback.c)。
 * @return 静态接口表 (永不为 NULL)
 */
const bl_rollback_storage_api_t *Boot_AntiRollback_GetStorageApi(void);

#ifdef __cplusplus
}
#endif

#endif /* BL_ANTIROLLBACK_H */
