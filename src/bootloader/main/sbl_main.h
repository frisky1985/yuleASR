/**
 * @file sbl_main.h
 * @brief SBL 集成层 — 生产启动入口 (抗回滚「已建未接线」修复)
 * @version 1.0
 * @date 2026-08-12
 *
 * 串起完整启动链路 (Step 1 骨架, 生产替换点以注释标注):
 *   ① 初始化 bl_antrollback — 从 NVM 装载单调计数器 (磨损均衡+CRC+延后递增)
 *   ② 读计数器 → 填入 bl_secure_boot_config_t.anti_rollback_counter (0=禁用)
 *   ③ 注入抗回滚存储接口 → Boot_Update (bsw/boot 层经回调访问, 分层解耦)
 *   ④ bl_secure_boot_init + verify — 验签 App 固件
 *   ⑤ 验签通过 → Boot_AntiRollback_NotifySuccessfulBoot (延后递增提交) → 启动 App;
 *      失败 → 记录回滚信息并 bl_rollback 回滚 / 拒绝 (恢复模式)
 *
 * 修复背景: bl_antrollback 此前 0 个生产调用者; bl_secure_boot 的
 * anti_rollback_counter 是调用者静态值而非 NVM 装载; Boot_Update 自建 BIB
 * pending 机制与 bl_antrollback 重复。本集成层 + bl_rollback_storage 接口
 * 将三层接通, 计数器唯一事实源 = bl_antrollback NVM。
 *
 * 可测性: 本层不依赖真实硬件 — flash 驱动、跳转/恢复回调均由配置注入
 * (测试用 mock; 生产替换为真实驱动与 Boot_Loader_Jump)。
 *
 * UNECE R156 / GB 44496-2024 对齐, ASIL-D Safety Level
 */

#ifndef SBL_MAIN_H
#define SBL_MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "bl_antrollback.h"
#include "bl_secure_boot.h"
#include "bl_rollback.h"
#include "bl_rollback_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 版本信息
 * ============================================================================ */
#define SBL_MAIN_MAJOR_VERSION       1
#define SBL_MAIN_MINOR_VERSION       0
#define SBL_MAIN_PATCH_VERSION       0

/* ============================================================================
 * 错误码定义
 * ============================================================================ */
typedef enum {
    SBL_MAIN_OK = 0,
    SBL_MAIN_ERROR_INVALID_PARAM = -1,        /* 非法参数 */
    SBL_MAIN_ERROR_ANTIROLLBACK_INIT = -2,    /* 抗回滚 NVM 初始化失败 */
    SBL_MAIN_ERROR_COUNTER_READ = -3,         /* 计数器读取失败 */
    SBL_MAIN_ERROR_SECURE_BOOT_INIT = -4,     /* 安全启动模块初始化失败 */
    SBL_MAIN_ERROR_VERIFY_FAILED = -5,        /* 验签失败 (含回滚保护拒绝) */
    SBL_MAIN_ERROR_ROLLBACK_FAILED = -6,      /* 回滚执行失败 */
    SBL_MAIN_ERROR_NOT_INITIALIZED = -7,      /* 未初始化 */
    SBL_MAIN_ERROR_NOTIFY_FAILED = -8,        /* 延后递增上报失败 */
    SBL_MAIN_ERROR_NO_APP = -9                /* 无有效 App 固件镜像 */
} sbl_main_error_t;

/* ============================================================================
 * 启动结果 (供上层记录/诊断/审计)
 * ============================================================================ */
typedef enum {
    SBL_MAIN_RESULT_APP_STARTED = 0,   /* 验签通过, 已跳转 App */
    SBL_MAIN_RESULT_APP_REJECTED,      /* 验签失败, 拒绝启动 */
    SBL_MAIN_RESULT_ROLLBACK,          /* 验签失败, 触发回滚 */
    SBL_MAIN_RESULT_RECOVERY           /* 无有效 App, 进入恢复模式 */
} sbl_main_result_t;

/* ============================================================================
 * 集成层配置 (全部依赖注入, 无硬件直连)
 * ============================================================================ */
typedef struct {
    /* ── NVM 抗回滚存储 ───────────────────────────────────────────────
     * 生产替换点: flash 传入真实驱动 (S32K312 MCAL Flash 适配),
     * base 按扇区对齐; 测试用 mock 驱动。 */
    const bl_flash_driver_t *flash;
    uint32_t antrollback_base;        /* 抗回滚 NVM 区基址 (扇区对齐) */
    uint32_t antrollback_slots;       /* 磨损均衡槽位数 (0 = 默认 4) */
    uint32_t confirm_boots;           /* 延后递增阈值 N (0 = 默认 3) */

    /* ── 安全启动验证选项 (生产: 全部使能 + 真实密钥槽) ── */
    bool verify_signature;
    bool verify_hash;
    bool verify_version;
    bool verify_cert_chain;
    bool verify_cert_validity;
    uint8_t root_ca_key_slot;
    uint8_t oem_key_slot;

    /* ── CSM/KeyM 上下文 (crypto_stack; 生产注入, 测试 csm_init(NULL)) ── */
    void *csm_ctx;
    void *keym_ctx;

    /* ── App 固件镜像 (生产: flash 地址映射; 测试: RAM buffer) ── */
    const uint8_t *app_image;         /* 含安全启动头部 */
    uint32_t app_image_size;
    uint32_t app_version;             /* 本次启动版本 (NotifySuccessfulBoot/回滚判定) */
    uint32_t app_entry;               /* App 入口地址 (跳转目标) */

    /* ── 回滚管理器配置 (0 取默认: 3 次尝试 / 2 次连续失败) ── */
    uint32_t rollback_max_boot_attempts;
    uint32_t rollback_max_consecutive_failures;
    bool     rollback_auto_enabled;

    /* ── 平台回调 (生产替换点) ─────────────────────────────────────
     * jump_to_app:   验签通过后跳转 App (生产: Boot_Loader_Jump)
     * enter_recovery: 无有效 App / 拒绝后进入恢复模式 (生产: Boot_Loader_EnterRecovery)
     * on_boot_result: 启动结果上报 (生产: 升级日志/审计) */
    void (*jump_to_app)(uint32_t entry);
    void (*enter_recovery)(void);
    void (*on_boot_result)(sbl_main_result_t result);
} sbl_main_config_t;

/* ============================================================================
 * 集成层上下文
 * ============================================================================ */
typedef struct {
    sbl_main_config_t config;         /* 配置副本 (init 时固化) */
    bl_antrollback_context_t antrollback;   /* 抗回滚计数器 (NVM 事实源) */
    bl_secure_boot_context_t secure_boot;   /* 安全启动验证 */
    bl_secure_boot_config_t  sb_config;     /* 安全启动配置 (计数器装载后填充) */
    bl_rollback_manager_t    rollback;      /* 回滚管理器 */
    uint32_t counter;                       /* 启动时从 NVM 装载的计数器 (0=禁用) */
    bool initialized;
} sbl_main_context_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化集成层
 * @details ① 初始化 bl_antrollback (从 NVM 恢复计数器/待确认状态);
 *          ② 读取计数器并填入 bl_secure_boot_config_t.anti_rollback_counter
 *             (0 = 禁用抗回滚, 与 bl_secure_boot 语义一致);
 *          ③ bl_secure_boot_init; ④ bl_rollback_init。
 *          不执行验签 (验签在 sbl_main_boot)。
 * @param ctx 集成层上下文
 * @param cfg 配置 (依赖注入: flash/镜像/回调/CSM)
 * @return SBL_MAIN_OK 成功
 */
sbl_main_error_t sbl_main_init(sbl_main_context_t *ctx, const sbl_main_config_t *cfg);

/**
 * @brief 反初始化集成层
 * @param ctx 集成层上下文
 */
void sbl_main_deinit(sbl_main_context_t *ctx);

/**
 * @brief 从 NVM 读取抗回滚计数器 (启动/诊断用)
 * @param ctx 集成层上下文
 * @param counter 输出计数器值 (0 = 禁用)
 * @return SBL_MAIN_OK 成功
 */
sbl_main_error_t sbl_main_load_rollback_counter(sbl_main_context_t *ctx,
                                                uint32_t *counter);

/**
 * @brief 注入抗回滚存储接口 → Boot_Update (bsw/boot 层)
 * @details 集成层接线点 (方案 C): 将 bl_antrollback 实现注入
 *          Boot_Update_SetAntiRollbackStorage, 使 Boot_Update 经
 *          bl_rollback_storage_api_t 回调访问本 NVM 计数器 —
 *          BIB 不再重复存计数器。sbl_main_boot 内部自动调用;
 *          单独暴露便于测试/诊断。
 * @param ctx 集成层上下文
 * @return SBL_MAIN_OK 成功
 */
sbl_main_error_t sbl_main_connect_antrollback(sbl_main_context_t *ctx);

/**
 * @brief 执行完整启动链路
 * @details ① 装载计数器; ② 填 secure_boot 配置; ③ 注入 Boot_Update 接口;
 *          ④ bl_secure_boot_verify 验签 App 固件;
 *          ⑤ 通过 → NotifySuccessfulBoot (延后递增提交) + 跳转 App (回调);
 *             失败 → bl_rollback 回滚 / 拒绝 (恢复模式回调)。
 * @param ctx 集成层上下文
 * @return SBL_MAIN_OK 验签通过 (跳转回调返回后; 生产跳转不返回)
 */
sbl_main_error_t sbl_main_boot(sbl_main_context_t *ctx);

/**
 * @brief 错误码转字符串 (诊断/日志)
 * @param err 错误码
 * @return 字符串描述
 */
const char *sbl_main_error_to_string(sbl_main_error_t err);

#ifdef __cplusplus
}
#endif

#endif /* SBL_MAIN_H */
