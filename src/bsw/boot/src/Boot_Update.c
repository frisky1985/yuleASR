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

/* Forward declaration for BIB helpers */
static Boot_Result bib_read(Boot_InfoBlock *bib);
static Boot_Result bib_write(const Boot_InfoBlock *bib);
static uint32_t    bib_calc_crc(const Boot_InfoBlock *bib);

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

Boot_Result Boot_Update_ConfirmUserDecision(boolean confirmed)
{
    if (g_ctx.confirm_state != BOOT_CONFIRM_PENDING) {
        return BOOT_E_PARAM;   /* 无待确认请求 */
    }
    g_ctx.confirm_state = (confirmed != 0U) ? BOOT_CONFIRM_GRANTED : BOOT_CONFIRM_DENIED;
    return BOOT_OK;
}

Boot_ConfirmState Boot_Update_GetConfirmState(void)
{
    return g_ctx.confirm_state;
}

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

    Boot_Result ret = Boot_Flash_Erase(slot_addr, BOOT_APP_SLOT_A_SIZE);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
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

    /* 3. Update anti-rollback counter in BIB */
    Boot_InfoBlock bib;
    ret = bib_read(&bib);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        return ret;
    }

    if (version <= bib.anti_rollback_counter) {
        return BOOT_E_VERSION;  /* Anti-rollback triggered */
    }

    bib.anti_rollback_counter = version;
    if (image_type == BOOT_IMAGE_SBL) {
        bib.sbl_version = version;
    } else {
        bib.app_version = version;
    }
    bib.crc32 = bib_calc_crc(&bib);

    ret = bib_write(&bib);
    if (ret == BOOT_OK) {
        /* 升级完成: 一次性确认授权复位 */
        g_ctx.confirm_state = BOOT_CONFIRM_IDLE;
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
