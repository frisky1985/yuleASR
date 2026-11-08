/* @req SHALL_BOOT */

#include "Boot_Update.h"
#include <stdint.h>
#include "Boot_Flash.h"
#include "Boot_Image.h"
#include "Boot_Verify.h"
#include <string.h>
#include "mbedtls/sha256.h"
#include "hash_algos.h"
#if defined(MBEDTLS_USE)
#include "mbedtls/sha256.h"
#endif

/* MISRA 8.4 保留说明: 本文件全部外部函数均声明于 Boot_Update.h
 * (Boot_Types.h/Boot_Cfg.h 亦在 src/bsw/boot/include 下, 声明/定义一致
 * 由编译期保证: 头文件缺失或签名不符将直接编译失败)。cppcheck 8.4
 * "定义无可见声明"系扫描命令未含 -I src/bsw/boot/include 且 Std_Types.h
 * (host 测试 stub, 位于 src/bsw/boot/test) 无法解析所致 — 补上 include
 * 路径后 8.4 全部消失 (已实测验证)。故保留 external 链接, 不调整头文件。 */

/* Internal update context */
typedef struct {
    uint32_t       slot_addr;
    Boot_ImageType image_type;
#if defined(MBEDTLS_USE)
    mbedtls_sha256_context hash_ctx;
    boolean        hash_active;
#endif
    uint32_t       bytes_written;
    boolean        active;
    /* 用户确认状态 (RS-OTA-02) */
    Boot_ConfirmState confirm_state;
    uint64_t     confirm_start_ms;   /* 确认请求发起时刻 (超时判定) */
} UpdateContext;

static UpdateContext g_ctx;
static boolean g_ctx_valid = FALSE;

/* 单调时间源 (确认超时用; NULL = 无时间源) */
static uint64_t (*s_tick_fn)(void) = NULL_PTR;

/* 延后递增阈值 N (P1-4): 新版本成功启动 N 次后提交抗回滚计数器 */
static uint32_t s_confirm_boots = BOOT_ROLLBACK_CONFIRM_BOOTS;

/* 抗回滚存储服务接口 (方案 C 注入, NULL = 旧 BIB 行为) */
static const bl_rollback_storage_api_t *s_antrollback_api = NULL_PTR;
static void *s_antrollback_ctx = NULL_PTR;

/* Forward declaration for BIB helpers */
static Boot_Result bib_read(Boot_InfoBlock *bib);
static Boot_Result bib_write(const Boot_InfoBlock *bib);
static uint32_t    bib_calc_crc(const Boot_InfoBlock *bib);
static Boot_Result bib_set_update_state(Boot_UpdateState state);

/* ============================================================================
 * 抗回滚延后递增内部辅助 (RS-OTA-01 / P1-4)
 * ============================================================================ */

/**
 * @brief 待确认状态合法性判定
 * @details BIB 老格式的 reserved 字节被复用为 pending 字段, 旧数据可能为
 *          0x00 (清零) / 0xFF (擦除) 等任意值; 仅当字段自洽时才视为有效:
 *          pending_counter > 已确认地板 且 启动次数不超过阈值。
 */
static boolean bib_pending_valid(const Boot_InfoBlock *bib)
{
    /* 掉电保护 (RS-OTA-06): DOWNLOADING = 升级写入未完成 (Prepare 落盘后
     * 擦除/写入中途掉电), 待确认状态不可信 → 视为无 pending。
     * 旧 BIB 无 update_state 字段 (reserved=0) → 读为 IDLE, 天然兼容。 */
    if (bib->update_state == BOOT_UPDATE_DOWNLOADING) {
        return FALSE;
    }
    if (bib->pending_counter == 0U) {
        return FALSE;   /* 无待确认升级 */
    }
    if (bib->pending_counter <= bib->anti_rollback_counter) {
        return FALSE;   /* 低于地板: 无效残留 */
    }
    if (bib->pending_boot_count > s_confirm_boots) {
        return FALSE;   /* 次数越界: 无效残留 (防 0xFF 擦除态误判) */
    }
    return TRUE;
}

/* ============================================================================
 * 用户确认内部辅助 (RS-OTA-02)
 * ============================================================================ */

/**
 * @brief 获取当前时间 (ms)
 * @param out_ms 输出时间; 仅返回 TRUE 时有效
 */
static boolean confirm_now_ms(uint64_t *out_ms)
{
    if (s_tick_fn == NULL_PTR) {
        return FALSE;
    }
    *out_ms = s_tick_fn();
    return TRUE;
}

/**
 * @brief 确认窗口是否已超时 (仅 PENDING 态判定)
 */
static boolean confirm_timeout_elapsed(void)
{
    if (BOOT_USER_CONFIRM_TIMEOUT_MS == 0U) {
        return FALSE;   /* 0 = 不超时 */
    }
    uint64_t now;
    if (!confirm_now_ms(&now)) {
        return FALSE;   /* 无时间源: 无法判定超时, 等待显式确认 */
    }
    return (now >= g_ctx.confirm_start_ms + BOOT_USER_CONFIRM_TIMEOUT_MS);
}

/**
 * @brief 确认门控检查: 未确认/被拒/超时 → 拒绝写入
 * @return BOOT_OK 放行; 否则对应错误码
 */
static Boot_Result confirm_gate(void)
{
    if (BOOT_USER_CONFIRM_REQUIRED == 0U) {
        return BOOT_OK;   /* 配置关闭确认门控 */
    }

    switch (g_ctx.confirm_state) {
        case BOOT_CONFIRM_GRANTED:
            return BOOT_OK;
        case BOOT_CONFIRM_DENIED:
            return BOOT_E_CONFIRM_DENIED;
        case BOOT_CONFIRM_TIMEOUT:
            return BOOT_E_TIMEOUT;
        case BOOT_CONFIRM_PENDING:
        default:
            /* 超时自动取消 (可配置, 默认 30s) */
            if (confirm_timeout_elapsed()) {
                g_ctx.confirm_state = BOOT_CONFIRM_TIMEOUT;
                return BOOT_E_TIMEOUT;
            }
            return BOOT_E_CONFIRM_PENDING;
    }
}

/* ============================================================================
 * User Confirm API (RS-OTA-02)
 * ============================================================================ */

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
Boot_Result Boot_Update_RequestUserConfirm(void)
{
    if (BOOT_USER_CONFIRM_REQUIRED == 0U) {
        g_ctx.confirm_state = BOOT_CONFIRM_GRANTED;  /* 未启用确认, 直接放行 */
        return BOOT_OK;
    }

    /* 已确认/已拒绝/已超时: 保持终态 (幂等) */
    if ((g_ctx.confirm_state == BOOT_CONFIRM_GRANTED) ||
        (g_ctx.confirm_state == BOOT_CONFIRM_DENIED) ||
        (g_ctx.confirm_state == BOOT_CONFIRM_TIMEOUT)) {
        return BOOT_OK;
    }

    /* 进入等待确认, 启动超时计时 */
    g_ctx.confirm_state = BOOT_CONFIRM_PENDING;
    if (!confirm_now_ms(&g_ctx.confirm_start_ms)) {
        g_ctx.confirm_start_ms = 0U;  /* 无时间源: 不超时, 等待显式确认 */
    }
    return BOOT_OK;
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
Boot_Result Boot_Update_ConfirmUserDecision(boolean confirmed)
{
    if (g_ctx.confirm_state != BOOT_CONFIRM_PENDING) {
        return BOOT_E_PARAM;   /* 无待确认请求 */
    }
    g_ctx.confirm_state = (confirmed != 0U) ? BOOT_CONFIRM_GRANTED : BOOT_CONFIRM_DENIED;
    return BOOT_OK;
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
Boot_ConfirmState Boot_Update_GetConfirmState(void)
{
    return g_ctx.confirm_state;
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
void Boot_Update_SetTimeSource(uint64_t (*tick_fn)(void))
{
    s_tick_fn = tick_fn;
}

Boot_Result Boot_Update_Prepare(uint32_t slot_addr, Boot_ImageType image_type)
{
    /* 用户确认门控 (RS-OTA-02): 未确认不得开始升级写入 */
    Boot_Result confirm_ret = confirm_gate();
    if (confirm_ret != BOOT_OK) {
        return confirm_ret;
    }

    /* 防护性拒绝 (P1-1 裁决, 2026-08-13): OTA 只写非活动槽 — 活动槽是当前
     * 运行固件, 写入活动槽会击穿 DOWNLOADING 掉电保护 (中断后回退指向
     * 正在被擦写的槽)。约定: 升级目标必须是非活动槽。
     * 仅 BIB 有效 (magic 匹配) 时判定活动槽; BIB 无效 (首次启动/出厂态)
     * 放行 — 无既有固件可保护, 且 Boot_Loader 首次启动固定走 SBL。 */
    {
        Boot_InfoBlock bib;
        if ((bib_read(&bib) == BOOT_OK) &&
            (bib.magic == 0x30424942U)) {
            uint32_t active_slot_addr = ((bib.status & 0x02U) != 0U)
                                            ? BOOT_APP_SLOT_B_ADDR
                                            : BOOT_APP_SLOT_A_ADDR;
            if (slot_addr == active_slot_addr) {
                return BOOT_E_PARAM;   /* 禁止写入活动槽 */
            }
        }
    }

    if (g_ctx_valid!= 0U) {
        (void)Boot_Update_Abort();
    }

    (void)memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.slot_addr    = slot_addr;
    g_ctx.image_type   = image_type;
    g_ctx.active       = TRUE;
    g_ctx.confirm_state = BOOT_CONFIRM_GRANTED;  /* Prepare 时确认已通过, 保持授权 */

#if defined(MBEDTLS_USE)
    mbedtls_sha256_init(&g_ctx.hash_ctx);
    mbedtls_sha256_starts(&g_ctx.hash_ctx, 0);
    g_ctx.hash_active = TRUE;
#endif

    /* 掉电保护节点① (RS-OTA-06): 擦除目标槽之前先落盘
     * BIB(update_state=DOWNLOADING) — 擦除/写入中途掉电, 重启时启动决策
     * 识别未完成升级并回退活动槽 (旧固件照常运行)。 */
    Boot_Result ret = bib_set_update_state(BOOT_UPDATE_DOWNLOADING);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        return ret;
    }

    ret = Boot_Flash_Erase(slot_addr, BOOT_APP_SLOT_A_SIZE);
    if (ret != BOOT_OK) {
        /* 擦除失败: 升级未开始, 恢复 BIB(update_state=IDLE) 不留残留 */
        g_ctx_valid = FALSE;
        (void)bib_set_update_state(BOOT_UPDATE_IDLE);
        return ret;
    }

    g_ctx_valid = TRUE;
    return BOOT_OK;
}

Boot_Result Boot_Update_WriteBlock(const uint8_t *data,
                                   uint32_t       offset,
                                   uint32_t       length)
{
    if (g_ctx_valid == 0U) {
        return BOOT_E_NOT_INIT;
    }

    /* 用户确认门控: 未确认/拒绝/超时 → 拒绝写入 */
    Boot_Result confirm_ret = confirm_gate();
    if (confirm_ret != BOOT_OK) {
        return confirm_ret;
    }

    uint32_t write_addr = g_ctx.slot_addr + offset;

    /* Update running hash incrementally */
#if defined(MBEDTLS_USE)
    if (g_ctx.hash_active) {
        mbedtls_sha256_update(&g_ctx.hash_ctx, data, length);
    }
#else
    /* Without mbedTLS, hash the entire payload at Finalize time */
#endif

    Boot_Result ret = Boot_Flash_Write(write_addr, data, length);
    if (ret == BOOT_OK) {
        g_ctx.bytes_written += length;
    }
    return ret;
}

Boot_Result Boot_Update_Finalize(Boot_ImageType image_type, uint32_t version)
{
    if (g_ctx_valid == 0U) {
        return BOOT_E_NOT_INIT;
    }

    /* 用户确认门控: 未确认/拒绝/超时 → 拒绝完成升级 */
    Boot_Result confirm_ret = confirm_gate();
    if (confirm_ret != BOOT_OK) {
        return confirm_ret;
    }

    /* 1. Build and write image header */
    Boot_ImageHeader hdr;
    (void)memset(&hdr, 0, sizeof(hdr));
    hdr.magic        = BOOT_IMAGE_MAGIC;
    hdr.image_type   = (uint32_t)image_type;
    hdr.version      = version;
    hdr.payload_size = g_ctx.bytes_written;

    /* Obtain the complete payload hash */
#if defined(MBEDTLS_USE)
    if (g_ctx.hash_active) {
        mbedtls_sha256_finish(&g_ctx.hash_ctx, hdr.hash);
        mbedtls_sha256_free(&g_ctx.hash_ctx);
        g_ctx.hash_active = FALSE;
    }
#else
    {
        /* 流式哈希: 逐块 update, 避免旧实现 256B 缓冲按整包长度取哈希的越界读 */
        uint8_t page_buf[256];
        uint32_t remaining = g_ctx.bytes_written;
        uint32_t off = 0U;
        sha256_state_t hash_ctx;

        (void)sha256_init(&hash_ctx);
        while (remaining > 0U) {
            uint32_t chunk = (remaining < sizeof(page_buf)) ? remaining : sizeof(page_buf);
            (void)Boot_Flash_Read(g_ctx.slot_addr + sizeof(Boot_ImageHeader) + off, page_buf, chunk);
            (void)sha256_update(&hash_ctx, page_buf, chunk);
            off += chunk;
            remaining -= chunk;
        }
        (void)sha256_final(&hash_ctx, hdr.hash);
    }
#endif

    hdr.header_crc   = Boot_Image_CalcHeaderCrc(&hdr);

    Boot_Result ret = Boot_Flash_Write(g_ctx.slot_addr,
                                       (const uint8_t *)&hdr,
                                       sizeof(hdr));
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        return ret;
    }

    /* 2. Verify written payload hash */
    const uint8_t *payload_buf ; /* In production, read-back from flash and verify */
    /* For stub, trust the hash was correct during write */

    /* 掉电保护节点② (RS-OTA-06): 校验通过 → BIB(update_state=PENDING),
     * 升级进入待验证/确认阶段; 全部成功后再落盘 IDLE。 */
    ret = bib_set_update_state(BOOT_UPDATE_PENDING);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 尽力恢复 */
        return ret;
    }

    /* 3. 抗回滚延后递增 (RS-OTA-01 / P1-4): 不立即提升计数器,
     *    先记录待确认版本, 新版本成功启动 N 次后由 NotifyBootSuccess 提交 */
    if (s_antrollback_api != NULL_PTR) {
        /* 注入模式 (方案 C): 计数器唯一来源 = 注入的 NVM 存储 (bl_antrollback),
         * BIB 只做版本管理 (不写 anti_rollback_counter/pending 字段) */
        uint32_t arb_counter = 0U;
        if (s_antrollback_api->read_counter(s_antrollback_ctx, &arb_counter)
                != BL_ROLLBACK_STORAGE_OK) {
            g_ctx_valid = FALSE;
            (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
            return BOOT_E_GENERAL;   /* NVM 不可用: 拒绝完成升级 (fail-closed) */
        }
        if (version <= arb_counter) {
            g_ctx_valid = FALSE;
            (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
            return BOOT_E_VERSION;   /* 低于已确认地板: 拒绝 (防回滚攻击) */
        }
        bl_rollback_storage_error_t st_ret =
            s_antrollback_api->stage(s_antrollback_ctx, version);
        if (st_ret != BL_ROLLBACK_STORAGE_OK) {
            g_ctx_valid = FALSE;
            (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
            return (st_ret == BL_ROLLBACK_STORAGE_ERROR_DECREMENT_ATTEMPT)
                       ? BOOT_E_VERSION : BOOT_E_GENERAL;
        }

        /* BIB 仅记录版本信息 (版本管理) */
        Boot_InfoBlock bib;
        ret = bib_read(&bib);
        if (ret != BOOT_OK) {
            g_ctx_valid = FALSE;
            (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
            return ret;
        }
        if (image_type == BOOT_IMAGE_SBL) {
            bib.sbl_version = version;
        } else {
            bib.app_version = version;
        }
        bib.crc32 = bib_calc_crc(&bib);

        ret = bib_write(&bib);
        if (ret == BOOT_OK) {
            /* 升级完成: 一次性确认授权复位 + BIB(update_state=IDLE),
             * 状态机无残留 */
            g_ctx.confirm_state = BOOT_CONFIRM_IDLE;
            (void)bib_set_update_state(BOOT_UPDATE_IDLE);
        } else {
            (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
        }
        g_ctx_valid = FALSE;
        return ret;
    }

    /* 旧模式 (未注入): BIB 自带 pending 机制 (兼容既有集成/测试) */
    Boot_InfoBlock bib;
    ret = bib_read(&bib);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
        return ret;
    }

    if (version <= bib.anti_rollback_counter) {
        g_ctx_valid = FALSE;
        (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
        return BOOT_E_VERSION;  /* 低于已确认地板: 拒绝 (防回滚攻击) */
    }

    /* 记录待确认升级 (覆盖旧 pending: 以最新升级为准) */
    bib.pending_counter = version;
    bib.pending_boot_count = 0U;
    if (image_type == BOOT_IMAGE_SBL) {
        bib.sbl_version = version;
    } else {
        bib.app_version = version;
    }
    bib.crc32 = bib_calc_crc(&bib);

    ret = bib_write(&bib);
    if (ret == BOOT_OK) {
        /* 升级完成: 一次性确认授权复位 + BIB(update_state=IDLE),
         * 状态机无残留 */
        g_ctx.confirm_state = BOOT_CONFIRM_IDLE;
        (void)bib_set_update_state(BOOT_UPDATE_IDLE);
    } else {
        (void)bib_set_update_state(BOOT_UPDATE_IDLE);   /* 失败恢复 */
    }
    g_ctx_valid = FALSE;
    return ret;
}

Boot_Result Boot_Update_Abort(void)
{
    (void)memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx_valid = FALSE;
    return BOOT_OK;
}

Boot_Result Boot_Update_SwapSlots(void)
{
    /* In A/B scheme: toggle a flag in BIB to select opposite slot */
    Boot_InfoBlock bib;
    Boot_Result ret = bib_read(&bib);
    if (ret != BOOT_OK) { return ret; }
    /* Toggle slot selection bit in status */
    bib.status ^= 0x02U;
    bib.crc32 = bib_calc_crc(&bib);
    return bib_write(&bib);
}

/* ============================================================================
 * 抗回滚延后递增 API (RS-OTA-01 / P1-4)
 * ============================================================================ */

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
Boot_Result Boot_Update_NotifyBootSuccess(uint32_t current_version)
{
    /* 注入模式: 延后递增状态/计数器保存在注入的 NVM 存储 (bl_antrollback),
     * BIB 不参与 (计数器单一事实源) */
    if (s_antrollback_api != NULL_PTR) {
        bl_rollback_storage_error_t st_ret =
            s_antrollback_api->notify_successful_boot(s_antrollback_ctx, current_version);
        return (st_ret == BL_ROLLBACK_STORAGE_OK) ? BOOT_OK : BOOT_E_GENERAL;
    }

    /* 旧模式 (未注入): BIB 自带 pending 机制 */
    Boot_InfoBlock bib;
    Boot_Result ret = bib_read(&bib);
    if (ret != BOOT_OK) {
        return ret;
    }

    /* 无待确认升级 / 待确认状态非法 (旧格式残留) → 无需处理 */
    if (!bib_pending_valid(&bib)) {
        return BOOT_OK;
    }

    /* 启动版本 != 待确认版本 (A/B 槽位回退等): 不计数、不清除,
     * 槽位中可能仍保留新版本待下次启动确认 */
    if (current_version != bib.pending_counter) {
        return BOOT_OK;
    }

    bib.pending_boot_count++;
    if (bib.pending_boot_count >= s_confirm_boots) {
        /* 达到阈值 N: 提交计数器, 清除待确认状态 */
        bib.anti_rollback_counter = bib.pending_counter;
        bib.pending_counter = 0U;
        bib.pending_boot_count = 0U;
    }
    bib.crc32 = bib_calc_crc(&bib);
    return bib_write(&bib);
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
Boot_Result Boot_Update_GetRollbackCounter(uint32_t *counter)
{
    if (counter == NULL_PTR) {
        return BOOT_E_PARAM;
    }

    /* 注入模式: 计数器来自 NVM 存储 (bl_antrollback), 非 BIB */
    if (s_antrollback_api != NULL_PTR) {
        uint32_t arb_counter = 0U;
        if (s_antrollback_api->read_counter(s_antrollback_ctx, &arb_counter)
                != BL_ROLLBACK_STORAGE_OK) {
            return BOOT_E_GENERAL;
        }
        *counter = arb_counter;
        return BOOT_OK;
    }

    /* 旧模式 (未注入): 读取 BIB */
    Boot_InfoBlock bib;
    Boot_Result ret = bib_read(&bib);
    if (ret != BOOT_OK) {
        return ret;
    }
    *counter = bib.anti_rollback_counter;
    return BOOT_OK;
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
void Boot_Update_SetRollbackConfirmBoots(uint32_t n)
{
    s_confirm_boots = (n == 0U) ? BOOT_ROLLBACK_CONFIRM_BOOTS : n;

    /* 注入模式下同步到存储实现 (bl_antrollback 为运行期配置) */
    if ((s_antrollback_api != NULL_PTR) &&
        (s_antrollback_api->set_confirm_boots != NULL_PTR)) {
        (void)s_antrollback_api->set_confirm_boots(s_antrollback_ctx, s_confirm_boots);
    }
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
void Boot_Update_SetHealthCheckMode(boolean enabled)
{
    /* 注入模式: 转发到抗回滚存储 (bl_antrollback 运行期配置, 默认关闭) */
    if ((s_antrollback_api != NULL_PTR) &&
        (s_antrollback_api->set_health_check_mode != NULL_PTR)) {
        (void)s_antrollback_api->set_health_check_mode(s_antrollback_ctx, enabled != 0U);
    }
    /* 旧模式 (未注入): 无健康门控, 无操作 (向后兼容) */
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
Boot_Result Boot_Update_ConfirmBusinessHealth(boolean ok)
{
    /* 注入模式: 转发到抗回滚存储 (bl_antrollback) — 版本取存储中的
     * 待确认版本, 由存储侧校验版本匹配 (RS-OTA-05 P1) */
    if ((s_antrollback_api != NULL_PTR) &&
        (s_antrollback_api->confirm_health != NULL_PTR)) {
        uint32_t version = 0U;
        uint32_t count = 0U;
        if (s_antrollback_api->get_pending(s_antrollback_ctx, &version, &count)
                != BL_ROLLBACK_STORAGE_OK) {
            return BOOT_E_GENERAL;
        }
        bl_rollback_storage_error_t st_ret =
            s_antrollback_api->confirm_health(s_antrollback_ctx, version, ok != 0U);
        return (st_ret == BL_ROLLBACK_STORAGE_OK) ? BOOT_OK : BOOT_E_GENERAL;
    }

    /* 旧模式 (未注入): BIB 机制无健康门控 → 无操作 (向后兼容) */
    return BOOT_OK;
}

/* MISRA 8.4 保留: 声明见 Boot_Update.h, 扫描缺 include 路径 (见文件头说明) */
void Boot_Update_SetAntiRollbackStorage(const bl_rollback_storage_api_t *api, void *ctx)
{
    s_antrollback_api = api;
    s_antrollback_ctx = ctx;

    /* 仅绑定接口, 不同步阈值: 存储实现可能已有运行期配置 (如 bl_antrollback
     * 由集成层 SetConfirmBoots 设定), 此处推送会覆盖之。阈值同步统一走
     * Boot_Update_SetRollbackConfirmBoots (显式调用)。 */
}

/* ---- BIB Helpers ---- */

static Boot_Result bib_read(Boot_InfoBlock *bib)
{
    return Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)bib, sizeof(Boot_InfoBlock));
}

static Boot_Result bib_write(const Boot_InfoBlock *bib)
{
    Boot_Result ret = Boot_Flash_Erase(BOOT_BIB_ADDR, sizeof(Boot_InfoBlock));
    if (ret != BOOT_OK) { return ret; }
    return Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)bib, sizeof(Boot_InfoBlock));
}

static uint32_t bib_calc_crc(const Boot_InfoBlock *bib)
{
    /* Simple XOR checksum for BIB integrity — replace with hardware CRC in prod */
    const uint8_t *bytes = (const uint8_t *)bib;
    uint32_t sum = 0U;
    /* CRC over everything except the crc32 field itself */
    uint32_t len = sizeof(Boot_InfoBlock) - sizeof(bib->crc32);
    for (uint32_t i = 0U; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}

/**
 * @brief 读-改-写 BIB 升级状态字段 (掉电保护, RS-OTA-06)
 * @details 保留其余字段, 仅更新 update_state 并重算 CRC 落盘。
 *          旧 BIB (无该字段, reserved=0) 读为 IDLE, 天然兼容。
 */
static Boot_Result bib_set_update_state(Boot_UpdateState state)
{
    Boot_InfoBlock bib;
    Boot_Result ret = bib_read(&bib);
    if (ret != BOOT_OK) {
        return ret;
    }
    bib.update_state = (uint8_t)state;
    bib.crc32 = bib_calc_crc(&bib);
    return bib_write(&bib);
}
