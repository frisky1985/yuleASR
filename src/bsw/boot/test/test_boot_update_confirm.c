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

/* ---- RAM-backed Boot_Flash stub ---- */

#define TEST_FLASH_SIZE   (4U * 1024U * 1024U)

static uint8_t g_test_flash[TEST_FLASH_SIZE];

Boot_Result Boot_Flash_Init(void) { return BOOT_OK; }

Boot_Result Boot_Flash_Erase(uint32_t addr, uint32_t size)
{
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

    printf("\n============================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("============================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
