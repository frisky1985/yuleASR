/**
 * @file test_boot_update_confirm.c
 * @brief Unit tests for Boot_Update user confirm flow (RS-OTA-02)
 * @version 1.0
 * @date 2026-08-12
 *
 * 覆盖: 未确认阻止写入 / 用户确认放行 / 用户拒绝 / 超时自动取消 /
 *       无时间源不超时 / 状态机终态复位。
 * 使用 RAM 模拟 Flash 的 Boot_Flash stub, 不依赖真实硬件。
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "boot_platform.h"   /* host stub: boolean/uint8/... + NULL_PTR */
#include "Std_Types.h"
#include "Boot_Update.h"
#include "Boot_Flash.h"
#include "Boot_Loader.h"
#include "bl_rollback_storage.h"

/* ---- RAM-backed Boot_Flash stub ---- */

#define TEST_FLASH_SIZE   (4U * 1024U * 1024U)

static uint8_t g_test_flash[TEST_FLASH_SIZE];

/* 故障注入: 目标槽 (BOOT_APP_SLOT_A_ADDR) 擦除失败 (BIB 擦除不受影响) */
static int g_fail_erase_slot = 0;

Boot_Result Boot_Flash_Init(void) { return BOOT_OK; }

Boot_Result Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
    if ((g_fail_erase_slot != 0) && (addr == BOOT_APP_SLOT_A_ADDR)) {
        return BOOT_E_FLASH_ERASE;
    }
    for (uint32_t i = 0U; i < size; i++) {
        if ((addr + i) < TEST_FLASH_SIZE) {
            g_test_flash[addr + i] = 0xFFU;
        }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Write(uint32_t dst, const uint8_t *src, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++) {
        if ((dst + i) < TEST_FLASH_SIZE) {
            g_test_flash[dst + i] = src[i];
        }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Read(uint32_t src, uint8_t *dst, uint32_t len)
{
    memcpy(dst, &g_test_flash[src], len);
    return BOOT_OK;
}

Boot_Result Boot_Flash_SetProtection(uint32_t a, uint32_t s, boolean p)
{
    (void)a; (void)s; (void)p;
    return BOOT_OK;
}

/* ---- Mock monotonic tick ---- */

static uint64_t g_tick_ms = 0U;

static uint64_t mock_tick_ms(void)
{
    return g_tick_ms;
}

/* ---- Test macros (与 test_bootloader.c 同款) ---- */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * Test Implementations
 * ============================================================================ */

/* GIVEN 未请求用户确认 / WHEN Prepare / THEN 拒绝 (未确认不得开始升级写入) */
static int test_confirm_not_requested_blocks(void)
{
    printf("  Testing unconfirmed upgrade is blocked...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();

    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_IDLE);
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_CONFIRM_PENDING);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_IDLE);

    printf("  PASSED\n");
    return 0;
}

/* GIVEN 确认后正常流程 / WHEN Prepare+WriteBlock+Finalize / THEN 全部放行, 状态复位 */
static int test_confirm_granted_flow(void)
{
    printf("  Testing confirmed upgrade flow...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();

    /* 初始化 BIB 区域: 擦除态 0xFF 会被当成 anti_rollback_counter=0xFFFFFFFF,
     * Finalize 的版本检查将误判回滚 → 先清零 */
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_PENDING);

    /* 用户确认 */
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_GRANTED);

    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);

    uint8_t block[64];
    memset(block, 0xA5, sizeof(block));
    TEST_ASSERT_EQ(Boot_Update_WriteBlock(block, 0U, sizeof(block)), BOOT_OK);

    TEST_ASSERT_EQ(Boot_Update_Finalize(BOOT_IMAGE_APP, 0x100U), BOOT_OK);
    /* 升级完成 → 一次性确认授权复位 */
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_IDLE);

    printf("  PASSED\n");
    return 0;
}

/* GIVEN 用户拒绝 / WHEN 后续写入 / THEN 全部拒绝 */
static int test_confirm_denied(void)
{
    printf("  Testing user-denied upgrade...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(FALSE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_DENIED);

    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_CONFIRM_DENIED);

    /* 拒绝为终态: 重新请求不改变状态 (防误覆盖用户决策) */
    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_DENIED);

    /* 中止后重新发起确认流程 */
    TEST_ASSERT_EQ(Boot_Update_Abort(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_IDLE);
    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_PENDING);
    (void)Boot_Update_Abort();

    printf("  PASSED\n");
    return 0;
}

/* GIVEN 等待确认中 / WHEN 超时 (默认 30s) / THEN 自动取消, 写入返回 TIMEOUT */
static int test_confirm_timeout(void)
{
    printf("  Testing confirm timeout auto-cancel...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);

    /* 超时前: 仍处于等待 (返回 PENDING) */
    g_tick_ms = 10000U;
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_CONFIRM_PENDING);

    /* 超过 BOOT_USER_CONFIRM_TIMEOUT_MS (30000) → 自动取消 */
    g_tick_ms = (uint64_t)BOOT_USER_CONFIRM_TIMEOUT_MS + 1U;
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_TIMEOUT);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_TIMEOUT);

    /* 超时后确认决策无效 */
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_E_PARAM);

    printf("  PASSED\n");
    return 0;
}

/* GIVEN 无时间源 / WHEN 等待确认 / THEN 不超时, 等待显式确认 */
static int test_confirm_no_timesource(void)
{
    printf("  Testing confirm without time source...\n");

    (void)Boot_Update_SetTimeSource(NULL_PTR);
    (void)Boot_Update_Abort();

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetConfirmState(), BOOT_CONFIRM_PENDING);

    /* 无时间源: PENDING 不因时间流逝取消 */
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_CONFIRM_PENDING);

    /* 显式确认后放行 */
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Abort(), BOOT_OK);

    printf("  PASSED\n");
    return 0;
}

/* GIVEN 非 PENDING 状态 / WHEN ConfirmUserDecision / THEN BOOT_E_PARAM */
static int test_confirm_state_errors(void)
{
    printf("  Testing confirm state-machine error paths...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();

    /* IDLE 态确认决策 → PARAM */
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_E_PARAM);

    /* GRANTED 态重复决策 → PARAM */
    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_E_PARAM);
    (void)Boot_Update_Abort();

    /* 拒绝后 WriteBlock (无活动上下文) → NOT_INIT (门控之前的状态检查) */
    uint8_t block[16];
    memset(block, 0x11, sizeof(block));
    TEST_ASSERT_EQ(Boot_Update_WriteBlock(block, 0U, sizeof(block)), BOOT_E_NOT_INIT);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 抗回滚延后递增测试 (RS-OTA-01 / P1-4)
 * ============================================================================ */

/* 与 Boot_Update.c 一致的 BIB 校验和 (测试构造 BIB 用) */
static uint32_t test_bib_crc(const Boot_InfoBlock *bib)
{
    const uint8_t *bytes = (const uint8_t *)bib;
    uint32_t sum = 0U;
    uint32_t len = sizeof(Boot_InfoBlock) - sizeof(bib->crc32);
    for (uint32_t i = 0U; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}

/* 执行一次完整的 确认→Prepare→WriteBlock→Finalize 流程 */
static Boot_Result run_upgrade(uint32_t version)
{
    uint8_t block[64];
    memset(block, 0xA5, sizeof(block));
    (void)Boot_Update_Abort();
    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_WriteBlock(block, 0U, sizeof(block)), BOOT_OK);
    return Boot_Update_Finalize(BOOT_IMAGE_APP, version);
}

static int test_deferred_antrollback(void)
{
    printf("  Testing deferred anti-rollback increment (P1-4)...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();
    /* 清零 BIB 区: counter=0, pending=0 (擦除态 0xFF 会误判) */
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));

    Boot_Update_SetRollbackConfirmBoots(2U);   /* 阈值 N=2 */
    uint32_t rc = 0U;
    Boot_InfoBlock bib;

    /* GIVEN 确认流程 / WHEN Finalize(0x100) / THEN 计数器不动, 记录 pending */
    TEST_ASSERT_EQ(run_upgrade(0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&rc), BOOT_OK);
    TEST_ASSERT_EQ(rc, 0U);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_counter, 0x100U);
    TEST_ASSERT_EQ(bib.pending_boot_count, 0U);

    /* GIVEN pending 0x100 / WHEN 成功启动 1 次 (N-1) / THEN 仍未提交 */
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&rc), BOOT_OK);
    TEST_ASSERT_EQ(rc, 0U);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_boot_count, 1U);

    /* 确认窗口内回滚成功 (P1-4 核心): 地板未提升, 旧版本 0x90 可安装 */
    TEST_ASSERT_EQ(run_upgrade(0x90U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&rc), BOOT_OK);
    TEST_ASSERT_EQ(rc, 0U);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_counter, 0x90U);   /* 新升级覆盖 pending */
    TEST_ASSERT_EQ(bib.pending_boot_count, 0U);

    /* 确认窗口内同版本重装: Finalize(0x100) 再次放行, pending 重置 */
    TEST_ASSERT_EQ(run_upgrade(0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_counter, 0x100U);
    TEST_ASSERT_EQ(bib.pending_boot_count, 0U);

    /* WHEN 再成功启动 2 次 (共 N=2) / THEN 提交: counter=0x100, pending 清除 */
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&rc), BOOT_OK);
    TEST_ASSERT_EQ(rc, 0x100U);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_counter, 0U);

    /* GIVEN 地板 0x100 / WHEN Finalize 同版本 / THEN 拒绝 (地板已提交) */
    TEST_ASSERT_EQ(run_upgrade(0x100U), BOOT_E_VERSION);
    (void)Boot_Update_Abort();

    /* GIVEN 地板 0x100 / WHEN Finalize 旧版本 0x90 / THEN 拒绝 (防回滚) */
    TEST_ASSERT_EQ(run_upgrade(0x90U), BOOT_E_VERSION);
    (void)Boot_Update_Abort();

    /* GIVEN pending 0x200 / WHEN 启动其他版本 0x50 / THEN 不计数不清除 */
    TEST_ASSERT_EQ(run_upgrade(0x200U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x50U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_counter, 0x200U);
    TEST_ASSERT_EQ(bib.pending_boot_count, 0U);

    /* GIVEN N=1 / WHEN Finalize(0x300) + 1 次成功启动 / THEN 立即提交 */
    Boot_Update_SetRollbackConfirmBoots(1U);
    TEST_ASSERT_EQ(run_upgrade(0x300U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x300U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&rc), BOOT_OK);
    TEST_ASSERT_EQ(rc, 0x300U);

    /* 非法 pending (旧格式 reserved 残留 0xFF): 视为无待确认, 不提交垃圾值 */
    {
        Boot_InfoBlock raw;
        memset(&raw, 0, sizeof(raw));
        raw.magic = 0x30424942U;
        raw.max_boot_attempts = 5U;
        raw.anti_rollback_counter = 0x300U;
        raw.pending_counter = 0xFFFFFFFFU;
        raw.pending_boot_count = 0xFFFFFFFFU;
        raw.crc32 = test_bib_crc(&raw);
        TEST_ASSERT_EQ(Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)&raw, sizeof(raw)),
                       BOOT_OK);
        TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x300U), BOOT_OK);
        TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(&rc), BOOT_OK);
        TEST_ASSERT_EQ(rc, 0x300U);   /* 未提交垃圾 pending */
    }

    /* NULL 参数 */
    TEST_ASSERT_EQ(Boot_Update_GetRollbackCounter(NULL_PTR), BOOT_E_PARAM);

    Boot_Update_SetRollbackConfirmBoots(0U);   /* 恢复默认 */
    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 掉电保护测试 (P2 / RS-OTA-06): BIB update_state 生命周期 + 启动决策回退
 * ============================================================================ */

/* GIVEN 确认流程 / WHEN Prepare / THEN 擦除前落盘 DOWNLOADING;
 *    Finalize 全部成功 → IDLE; 中断 (DOWNLOADING) → 启动决策回退活动槽 */
static int test_update_state_lifecycle(void)
{
    printf("  Testing BIB update_state lifecycle (DOWNLOADING/PENDING/IDLE)...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));

    Boot_InfoBlock bib;

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);

    /* Prepare → BIB(update_state=DOWNLOADING) 落盘 (擦除前) */
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ((int)bib.update_state, (int)BOOT_UPDATE_DOWNLOADING);

    /* GIVEN 擦除/写入中途掉电重启 (status 已选中 Slot B + DOWNLOADING)
     * WHEN 启动决策 / THEN 按 marker 启动活动槽 B (旧固件照常运行)。
     * 语义 (P1-1 裁决 2026-08-13): OTA 只写非活动槽 (Prepare 防护性拒绝
     * 活动槽写入), 故 DOWNLOADING 时 marker 指向的活动槽必然完好 —
     * 不再强制回退 Slot A (旧实现会指向正在被擦写的目标槽 A, 击穿保护)。 */
    bib.magic = 0x30424942U;   /* 'BIB0' (Boot_Loader load_bib 需合法魔数) */
    bib.status = 0x03U;   /* 0x01 active + 0x02 slot-B 选中 → 活动槽 B */
    bib.crc32 = test_bib_crc(&bib);
    TEST_ASSERT_EQ(Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)&bib, sizeof(bib)),
                   BOOT_OK);
    Boot_Decision dec = Boot_Loader_ResolveBootTarget();
    TEST_ASSERT_EQ((int)dec.last_error, (int)BOOT_OK);
    TEST_ASSERT_EQ((int)dec.target, (int)BOOT_IMAGE_APP);      /* 启动活动槽 B */
    TEST_ASSERT_EQ(dec.target_addr, BOOT_APP_SLOT_B_ADDR);

    /* 对照: IDLE + 选中 Slot B → 正常切换 Slot B (无 DOWNLOADING 时行为不变) */
    bib.update_state = BOOT_UPDATE_IDLE;
    bib.crc32 = test_bib_crc(&bib);
    TEST_ASSERT_EQ(Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)&bib, sizeof(bib)),
                   BOOT_OK);
    dec = Boot_Loader_ResolveBootTarget();
    TEST_ASSERT_EQ((int)dec.target, (int)BOOT_IMAGE_APP);      /* 正常切 Slot B */
    TEST_ASSERT_EQ(dec.target_addr, BOOT_APP_SLOT_B_ADDR);

    /* Finalize 全部成功 → BIB(update_state=IDLE) (状态机无残留) */
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));
    (void)Boot_Update_Abort();   /* 复位一次性确认授权, 重新发起完整升级流程 */
    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);
    uint8_t block[64];
    memset(block, 0xA5, sizeof(block));
    TEST_ASSERT_EQ(Boot_Update_WriteBlock(block, 0U, sizeof(block)), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Finalize(BOOT_IMAGE_APP, 0x100U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ((int)bib.update_state, (int)BOOT_UPDATE_IDLE);

    printf("  PASSED\n");
    return 0;
}

/* GIVEN Prepare 已落盘 DOWNLOADING / WHEN 目标槽擦除失败 / THEN
 *    Prepare 返回错误且 BIB 恢复 IDLE (不留残留状态) */
static int test_update_state_erase_failure_restore(void)
{
    printf("  Testing Prepare erase-failure restores IDLE...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));

    g_fail_erase_slot = 1;   /* 注入: 目标槽擦除失败 */

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_FLASH_ERASE);

    g_fail_erase_slot = 0;

    Boot_InfoBlock bib;
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ((int)bib.update_state, (int)BOOT_UPDATE_IDLE);   /* 恢复 IDLE */

    /* 后续正常流程不受影响: 授权保留 (Prepare 失败不消耗用户确认), 直接重试 */
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);

    printf("  PASSED\n");
    return 0;
}

/* GIVEN BIB update_state=DOWNLOADING / WHEN NotifyBootSuccess (旧 BIB 模式)
 * THEN pending 视为无效: 不计数不提交; 旧 BIB (读为 0=IDLE) 行为不变 */
static int test_update_state_downloading_invalidates_pending(void)
{
    printf("  Testing DOWNLOADING invalidates BIB pending...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();

    Boot_InfoBlock bib;
    memset(&bib, 0, sizeof(bib));
    bib.magic = 0x30424942U;   /* 'BIB0' */
    bib.max_boot_attempts = 5U;
    bib.anti_rollback_counter = 0x300U;
    bib.pending_counter = 0x400U;
    bib.pending_boot_count = 0U;
    bib.update_state = BOOT_UPDATE_DOWNLOADING;
    bib.crc32 = test_bib_crc(&bib);
    TEST_ASSERT_EQ(Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)&bib, sizeof(bib)),
                   BOOT_OK);

    /* 未注入模式: DOWNLOADING 下的 pending 不计数、不提交 */
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x400U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_boot_count, 0U);
    TEST_ASSERT_EQ(bib.anti_rollback_counter, 0x300U);

    /* 旧 BIB 兼容: update_state 读为 0=IDLE → 原行为不变 (计数) */
    bib.update_state = BOOT_UPDATE_IDLE;
    bib.crc32 = test_bib_crc(&bib);
    TEST_ASSERT_EQ(Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)&bib, sizeof(bib)),
                   BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_NotifyBootSuccess(0x400U), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)&bib, sizeof(bib)), BOOT_OK);
    TEST_ASSERT_EQ(bib.pending_boot_count, 1U);
    TEST_ASSERT_EQ(bib.anti_rollback_counter, 0x300U);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * 健康确认封装测试 (P1 / RS-OTA-05): Boot_Update_ConfirmBusinessHealth 转发
 * ============================================================================ */

/* Mock 存储接口: 记录 set_health_check_mode / confirm_health 调用 */
static int g_inject_calls = 0;
static int g_inject_mode = 0;
static uint32_t g_inject_confirm_version = 0U;
static int g_inject_confirm_ok = 0;
static void *g_mock_ctx = (void *)0x1;   /* 任意非 NULL 上下文 */

static bl_rollback_storage_error_t mock_read_counter(void *ctx, uint32_t *c)
{ (void)ctx; *c = 0U; return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_write_counter(void *ctx, uint32_t c)
{ (void)ctx; (void)c; return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_increment(void *ctx, uint32_t *c)
{ (void)ctx; if (c != NULL) { *c = 1U; } return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_set_confirm_boots(void *ctx, uint32_t n)
{ (void)ctx; (void)n; return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_stage(void *ctx, uint32_t v)
{ (void)ctx; (void)v; return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_notify_boot(void *ctx, uint32_t v)
{ (void)ctx; (void)v; return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_get_pending(void *ctx, uint32_t *v, uint32_t *c)
{ (void)ctx; *v = 0x7U; if (c != NULL) { *c = 0U; } return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_set_health_mode(void *ctx, bool enabled)
{ (void)ctx; g_inject_mode = enabled ? 1 : 0; g_inject_calls++; return BL_ROLLBACK_STORAGE_OK; }
static bl_rollback_storage_error_t mock_confirm_health(void *ctx, uint32_t version, bool ok)
{ (void)ctx; g_inject_confirm_version = version; g_inject_confirm_ok = ok ? 1 : 0;
  g_inject_calls++; return BL_ROLLBACK_STORAGE_OK; }

static const bl_rollback_storage_api_t mock_storage_api = {
    .read_counter           = mock_read_counter,
    .write_counter          = mock_write_counter,
    .increment              = mock_increment,
    .set_confirm_boots      = mock_set_confirm_boots,
    .stage                  = mock_stage,
    .notify_successful_boot = mock_notify_boot,
    .get_pending            = mock_get_pending,
    .set_health_check_mode  = mock_set_health_mode,
    .confirm_health         = mock_confirm_health
};

/* GIVEN 注入存储接口 / WHEN Boot_Update_SetHealthCheckMode +
 * Boot_Update_ConfirmBusinessHealth / THEN 转发到存储实现; 未注入时无操作 */
static int test_confirm_business_health_forward(void)
{
    printf("  Testing Boot_Update health forwarding (RS-OTA-05)...\n");

    (void)Boot_Update_Abort();

    /* 未注入: 无操作兼容 (旧 BIB 模式无健康门控) */
    TEST_ASSERT_EQ(Boot_Update_ConfirmBusinessHealth(TRUE), BOOT_OK);
    Boot_Update_SetHealthCheckMode(TRUE);   /* 无操作, 不崩溃 */

    g_inject_calls = 0;
    Boot_Update_SetAntiRollbackStorage(&mock_storage_api, g_mock_ctx);

    /* SetHealthCheckMode → set_health_check_mode 转发 */
    Boot_Update_SetHealthCheckMode(TRUE);
    TEST_ASSERT_EQ(g_inject_calls, 1);
    TEST_ASSERT_EQ(g_inject_mode, 1);
    Boot_Update_SetHealthCheckMode(FALSE);
    TEST_ASSERT_EQ(g_inject_calls, 2);
    TEST_ASSERT_EQ(g_inject_mode, 0);

    /* ConfirmBusinessHealth: 从 get_pending 取版本 (mock: 0x7) 转发 confirm_health */
    TEST_ASSERT_EQ(Boot_Update_ConfirmBusinessHealth(FALSE), BOOT_OK);
    TEST_ASSERT_EQ(g_inject_calls, 3);
    TEST_ASSERT_EQ(g_inject_confirm_version, 0x7U);
    TEST_ASSERT_EQ(g_inject_confirm_ok, 0);

    TEST_ASSERT_EQ(Boot_Update_ConfirmBusinessHealth(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(g_inject_calls, 4);
    TEST_ASSERT_EQ(g_inject_confirm_ok, 1);

    /* 解除注入: 恢复无操作 */
    Boot_Update_SetAntiRollbackStorage(NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQ(Boot_Update_ConfirmBusinessHealth(TRUE), BOOT_OK);

    printf("  PASSED\n");
    return 0;
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

static int run_test(int (*test_func)(void), const char *name)
{
    printf("Running %s...\n", name);
    tests_run++;
    if (test_func() == 0) {
        tests_passed++;
        return 0;
    }
    return -1;
}

/* GIVEN BIB 有效且当前活动槽 A / WHEN Prepare 目标 = 活动槽 A
 * THEN 防护性拒绝 (BOOT_E_PARAM) — OTA 只写非活动槽约定 (P1-1 裁决) */
static int test_prepare_rejects_active_slot(void)
{
    printf("  Testing Prepare rejects active slot write...\n");

    (void)Boot_Update_SetTimeSource(mock_tick_ms);
    g_tick_ms = 0U;
    (void)Boot_Update_Abort();
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));

    /* BIB 有效: 活动槽 A (status 无 slot-B 位) */
    Boot_InfoBlock bib;
    memset(&bib, 0, sizeof(bib));
    bib.magic = 0x30424942U;
    bib.status = 0x01U;   /* active, 未选 Slot B → 活动槽 A */
    bib.crc32 = test_bib_crc(&bib);
    TEST_ASSERT_EQ(Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)&bib, sizeof(bib)),
                   BOOT_OK);

    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);

    /* 写活动槽 A → 拒绝 */
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP),
                   BOOT_E_PARAM);

    /* 写非活动槽 B → 放行 */
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_B_ADDR, BOOT_IMAGE_APP), BOOT_OK);
    (void)Boot_Update_Abort();

    /* 对照: BIB 无效 (magic 不符, 首次启动) → 放行 (无既有固件可保护) */
    memset(&g_test_flash[BOOT_BIB_ADDR], 0x00, sizeof(Boot_InfoBlock));
    (void)Boot_Update_Abort();
    TEST_ASSERT_EQ(Boot_Update_RequestUserConfirm(), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_ConfirmUserDecision(TRUE), BOOT_OK);
    TEST_ASSERT_EQ(Boot_Update_Prepare(BOOT_APP_SLOT_A_ADDR, BOOT_IMAGE_APP), BOOT_OK);

    printf("  PASSED\n");
    return 0;
}

int main(void)
{
    printf("============================================\n");
    printf("Boot Update User Confirm Tests (RS-OTA-02)\n");
    printf("============================================\n\n");

    memset(g_test_flash, 0xFF, sizeof(g_test_flash));

    run_test(test_confirm_not_requested_blocks, "Confirm Not Requested Blocks Upgrade");
    run_test(test_confirm_granted_flow, "Confirmed Upgrade Flow");
    run_test(test_confirm_denied, "User Denied Upgrade");
    run_test(test_confirm_timeout, "Confirm Timeout Auto-Cancel");
    run_test(test_confirm_no_timesource, "Confirm Without Time Source");
    run_test(test_confirm_state_errors, "Confirm State-Machine Errors");
    run_test(test_deferred_antrollback, "Deferred Anti-Rollback Increment (P1-4)");
    run_test(test_update_state_lifecycle, "BIB Update State Lifecycle (RS-OTA-06)");
    run_test(test_update_state_erase_failure_restore, "Prepare Erase-Failure Restores IDLE");
    run_test(test_update_state_downloading_invalidates_pending, "DOWNLOADING Invalidates Pending");
    run_test(test_confirm_business_health_forward, "Health Forwarding via Injected Storage");
    run_test(test_prepare_rejects_active_slot, "Prepare Rejects Active Slot Write (P1-1)");

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
