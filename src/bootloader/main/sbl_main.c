/**
 * @file sbl_main.c
 * @brief SBL 集成层 — 生产启动入口实现 (Step 1 骨架)
 * @version 1.0
 * @date 2026-08-12
 *
 * 完整链路 (见 sbl_main.h 头注释):
 *   bl_antrollback (NVM) → bl_secure_boot_config.anti_rollback_counter
 *   → Boot_Update 注入接口 → bl_secure_boot_verify → 启动/回滚/拒绝
 *   → Boot_AntiRollback_NotifySuccessfulBoot (延后递增提交)
 *
 * 生产替换点 (注释标注): flash 驱动 / CSM-KeyM / App 镜像来源 /
 * jump_to_app / enter_recovery / 回滚分区管理器。
 *
 * UNECE R156 / GB 44496-2024 对齐, ASIL-D Safety Level
 */

#include <string.h>
#include "sbl_main.h"
#include "Boot_Update.h"
#include "../common/log/dds_log.h"

/* ============================================================================
 * 内部宏和常量
 * ============================================================================ */
#define SBL_MAIN_MODULE_NAME        "SBL_MAIN"
#define SBL_MAIN_LOG_LEVEL          DDS_LOG_LEVEL_INFO

#define SBL_MAIN_DEFAULT_MAX_ATTEMPTS          3U
#define SBL_MAIN_DEFAULT_MAX_CONSEC_FAILURES   2U

/* ============================================================================
 * API函数实现
 * ============================================================================ */

sbl_main_error_t sbl_main_load_rollback_counter(sbl_main_context_t *ctx,
                                                uint32_t *counter)
{
    if ((ctx == NULL) || (counter == NULL)) {
        return SBL_MAIN_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return SBL_MAIN_ERROR_NOT_INITIALIZED;
    }

    /* 从 NVM 读取已确认计数器 (单一事实源; 0 = 禁用) */
    if (Boot_AntiRollback_Read(&ctx->antrollback, counter) != BL_ANTIROLLBACK_OK) {
        return SBL_MAIN_ERROR_COUNTER_READ;
    }
    ctx->counter = *counter;
    return SBL_MAIN_OK;
}

sbl_main_error_t sbl_main_connect_antrollback(sbl_main_context_t *ctx)
{
    if (ctx == NULL) {
        return SBL_MAIN_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return SBL_MAIN_ERROR_NOT_INITIALIZED;
    }

    /* 注入抗回滚存储接口 → Boot_Update (bsw/boot 层):
     * Boot_Update 经 bl_rollback_storage_api_t 回调访问本 NVM 计数器,
     * 不直接依赖 bootloader 层; BIB 不再重复存计数器 (只做版本管理)。 */
    uint32_t confirm_boots = ctx->antrollback.confirm_boots;   /* 先取, 防注入覆盖 */
    Boot_Update_SetAntiRollbackStorage(Boot_AntiRollback_GetStorageApi(),
                                       &ctx->antrollback);

    /* 同步延后递增阈值 (bl_antrollback 运行期配置) */
    Boot_Update_SetRollbackConfirmBoots(confirm_boots);

    DDS_LOG(SBL_MAIN_LOG_LEVEL, SBL_MAIN_MODULE_NAME,
            "Anti-rollback storage injected into Boot_Update (confirm_boots=%u)",
            ctx->antrollback.confirm_boots);

    return SBL_MAIN_OK;
}

sbl_main_error_t sbl_main_init(sbl_main_context_t *ctx, const sbl_main_config_t *cfg)
{
    if ((ctx == NULL) || (cfg == NULL)) {
        return SBL_MAIN_ERROR_INVALID_PARAM;
    }
    if ((cfg->flash == NULL) || (cfg->flash->read == NULL) ||
        (cfg->flash->erase == NULL) || (cfg->flash->program == NULL)) {
        return SBL_MAIN_ERROR_INVALID_PARAM;   /* 无可用 NVM 访问 */
    }
    if ((cfg->app_image == NULL) || (cfg->app_image_size == 0U)) {
        return SBL_MAIN_ERROR_NO_APP;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->config = *cfg;   /* 固化配置副本 */

    /* ① 初始化抗回滚计数器: 扫描槽位, 从 NVM 恢复计数器/待确认状态 */
    if (Boot_AntiRollback_Init(&ctx->antrollback, cfg->flash,
                               cfg->antrollback_base,
                               cfg->antrollback_slots) != BL_ANTIROLLBACK_OK) {
        return SBL_MAIN_ERROR_ANTIROLLBACK_INIT;
    }
    Boot_AntiRollback_SetConfirmBoots(&ctx->antrollback, cfg->confirm_boots);

    /* ② 读计数器 → 填安全启动配置 (0 = 禁用抗回滚) */
    if (Boot_AntiRollback_Read(&ctx->antrollback, &ctx->counter)
            != BL_ANTIROLLBACK_OK) {
        return SBL_MAIN_ERROR_COUNTER_READ;
    }

    bl_secure_boot_config_t *sb = &ctx->sb_config;
    memset(sb, 0, sizeof(*sb));
    sb->verify_signature    = cfg->verify_signature;
    sb->verify_hash         = cfg->verify_hash;
    sb->verify_version      = cfg->verify_version;
    sb->verify_cert_chain   = cfg->verify_cert_chain;
    sb->verify_cert_validity = cfg->verify_cert_validity;
    sb->max_boot_attempts   = (cfg->rollback_max_boot_attempts != 0U)
                                  ? cfg->rollback_max_boot_attempts
                                  : SBL_MAIN_DEFAULT_MAX_ATTEMPTS;
    sb->min_firmware_version = 0U;
    sb->root_ca_key_slot    = cfg->root_ca_key_slot;
    sb->oem_key_slot        = cfg->oem_key_slot;
    /* 抗回滚计数器: 从 NVM 装载 (0 = 禁用); sbl_main_boot 启动前会再装载一次 */
    sb->anti_rollback_counter = ctx->counter;

    /* ③ 安全启动模块初始化 */
    if (bl_secure_boot_init(&ctx->secure_boot, sb,
                            cfg->csm_ctx, cfg->keym_ctx) != BL_SB_OK) {
        return SBL_MAIN_ERROR_SECURE_BOOT_INIT;
    }

    /* ④ 回滚管理器初始化 (生产替换点: 注入真实分区管理器,
     * 此处骨架传 NULL — 回滚仅记录/切换意图, 分区切换由平台接管) */
    bl_rollback_config_t rb_cfg;
    memset(&rb_cfg, 0, sizeof(rb_cfg));
    rb_cfg.max_boot_attempts         = (cfg->rollback_max_boot_attempts != 0U)
                                           ? cfg->rollback_max_boot_attempts
                                           : SBL_MAIN_DEFAULT_MAX_ATTEMPTS;
    rb_cfg.max_consecutive_failures  = (cfg->rollback_max_consecutive_failures != 0U)
                                           ? cfg->rollback_max_consecutive_failures
                                           : SBL_MAIN_DEFAULT_MAX_CONSEC_FAILURES;
    rb_cfg.auto_rollback_enabled     = cfg->rollback_auto_enabled;
    rb_cfg.preserve_history          = true;
    if (bl_rollback_init(&ctx->rollback, &rb_cfg, NULL) != BL_ROLLBACK_OK) {
        return SBL_MAIN_ERROR_ROLLBACK_FAILED;
    }

    ctx->initialized = true;

    DDS_LOG(SBL_MAIN_LOG_LEVEL, SBL_MAIN_MODULE_NAME,
            "SBL integration layer initialized: arb_counter=%u (0=disabled)",
            ctx->counter);

    return SBL_MAIN_OK;
}

void sbl_main_deinit(sbl_main_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    /* 解除 Boot_Update 注入 (恢复 BIB 旧行为), 避免悬挂指针 */
    Boot_Update_SetAntiRollbackStorage(NULL_PTR, NULL_PTR);

    bl_rollback_deinit(&ctx->rollback);
    bl_secure_boot_deinit(&ctx->secure_boot);
    Boot_AntiRollback_Deinit(&ctx->antrollback);
    ctx->initialized = false;

    DDS_LOG(SBL_MAIN_LOG_LEVEL, SBL_MAIN_MODULE_NAME,
            "SBL integration layer deinitialized");
}

sbl_main_error_t sbl_main_boot(sbl_main_context_t *ctx)
{
    if (ctx == NULL) {
        return SBL_MAIN_ERROR_INVALID_PARAM;
    }
    if (!ctx->initialized) {
        return SBL_MAIN_ERROR_NOT_INITIALIZED;
    }

    /* ① 从 NVM 装载抗回滚计数器 (启动时最新值) */
    uint32_t counter = 0U;
    if (Boot_AntiRollback_Read(&ctx->antrollback, &counter) != BL_ANTIROLLBACK_OK) {
        return SBL_MAIN_ERROR_COUNTER_READ;
    }
    ctx->counter = counter;

    /* ② 填安全启动配置: 0 = 禁用抗回滚 (与 bl_secure_boot 语义一致) */
    ctx->secure_boot.config.anti_rollback_counter = counter;

    /* ③ 注入抗回滚存储接口 → Boot_Update (集成层接线点) */
    sbl_main_error_t conn_ret = sbl_main_connect_antrollback(ctx);
    if (conn_ret != SBL_MAIN_OK) {
        return conn_ret;
    }

    /* ④ 安全启动验签 (App 固件) */
    bl_secure_boot_error_t sb_result = bl_secure_boot_verify(
        &ctx->secure_boot, ctx->config.app_image, ctx->config.app_image_size);

    if (sb_result == BL_SB_OK) {
        /* ⑤a 验签通过: 延后递增提交 — 上报本次成功启动 (达到阈值 N 后
         *     计数器才提升; 确认窗口内允许回滚/同版本重装)。
         *     生产: 亦可延后到 App 健康检查通过后再上报 (平台策略)。 */
        bl_antrollback_error_t notify_ret = Boot_AntiRollback_NotifySuccessfulBoot(
            &ctx->antrollback, ctx->config.app_version);
        if (notify_ret != BL_ANTIROLLBACK_OK) {
            /* 计数失败不阻塞启动, 仅记录 (下次启动仍受计数器保护) */
            DDS_LOG(DDS_LOG_LEVEL_WARN, SBL_MAIN_MODULE_NAME,
                    "NotifySuccessfulBoot failed (%d) — boot proceeds",
                    (int)notify_ret);
        }

        if (ctx->config.on_boot_result != NULL) {
            ctx->config.on_boot_result(SBL_MAIN_RESULT_APP_STARTED);
        }

        /* ⑤b 跳转 App (生产: Boot_Loader_Jump, 不返回; 测试: mock 返回) */
        if (ctx->config.jump_to_app != NULL) {
            ctx->config.jump_to_app(ctx->config.app_entry);
        }
        return SBL_MAIN_OK;
    }

    /* ⑤c 验签失败: 记录失败并进入回滚/拒绝流程 */
    DDS_LOG(DDS_LOG_LEVEL_ERROR, SBL_MAIN_MODULE_NAME,
            "Secure boot verification failed (%d) for version 0x%08X, "
            "counter=%u", (int)sb_result, ctx->config.app_version, counter);

    (void)bl_rollback_record_boot_result(&ctx->rollback, BL_BOOT_RESULT_FAILURE);

    if (ctx->config.rollback_auto_enabled) {
        bool need_rollback = false;
        bl_rollback_error_t rb_ret = bl_rollback_check_needed(&ctx->rollback,
                                                              &need_rollback,
                                                              NULL);
        if ((rb_ret == BL_ROLLBACK_OK) && need_rollback) {
            bl_rollback_error_t ex_ret = bl_rollback_execute(
                &ctx->rollback, BL_ROLLBACK_REASON_SIGNATURE_INVALID);
            if (ex_ret != BL_ROLLBACK_OK) {
                DDS_LOG(DDS_LOG_LEVEL_ERROR, SBL_MAIN_MODULE_NAME,
                        "Rollback execute failed (%d)", (int)ex_ret);
                return SBL_MAIN_ERROR_ROLLBACK_FAILED;
            }
            if (ctx->config.on_boot_result != NULL) {
                ctx->config.on_boot_result(SBL_MAIN_RESULT_ROLLBACK);
            }
            /* 生产: 回滚后跳转旧版本 (由平台在 on_boot_result 后接管) */
            return SBL_MAIN_ERROR_VERIFY_FAILED;
        }
    }

    /* 拒绝启动: 进入恢复模式 (生产: Boot_Loader_EnterRecovery) */
    if (ctx->config.enter_recovery != NULL) {
        ctx->config.enter_recovery();
    }
    if (ctx->config.on_boot_result != NULL) {
        ctx->config.on_boot_result(SBL_MAIN_RESULT_APP_REJECTED);
    }
    return SBL_MAIN_ERROR_VERIFY_FAILED;
}

const char *sbl_main_error_to_string(sbl_main_error_t err)
{
    switch (err) {
        case SBL_MAIN_OK:                    return "OK";
        case SBL_MAIN_ERROR_INVALID_PARAM:   return "INVALID_PARAM";
        case SBL_MAIN_ERROR_ANTIROLLBACK_INIT: return "ANTIROLLBACK_INIT";
        case SBL_MAIN_ERROR_COUNTER_READ:    return "COUNTER_READ";
        case SBL_MAIN_ERROR_SECURE_BOOT_INIT:return "SECURE_BOOT_INIT";
        case SBL_MAIN_ERROR_VERIFY_FAILED:   return "VERIFY_FAILED";
        case SBL_MAIN_ERROR_ROLLBACK_FAILED: return "ROLLBACK_FAILED";
        case SBL_MAIN_ERROR_NOT_INITIALIZED: return "NOT_INITIALIZED";
        case SBL_MAIN_ERROR_NOTIFY_FAILED:   return "NOTIFY_FAILED";
        case SBL_MAIN_ERROR_NO_APP:          return "NO_APP";
        default:                             return "UNKNOWN";
    }
}
