/**
 * @file test_e2e_qualification.c
 * @brief E2E 安全通信合格性测试 — 完整保护链验收
 * @details 覆盖 E2E Profile 1 完整端到端保护+校验链路、CRC 错误注入检测、
 *          计数器翻转场景，确保 E2E 模块满足 SWR-002 合格性验收标准。
 *
 * AUTOSAR Standard: R22-11
 * ASIL Level: D
 * Target: SWR-002 (Safety & Security — E2E error injection detection)
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

#include "E2E.h"
#include "E2E_P01.h"
#include "Crc.h"

/*==================================================================================================
 *                                    Test Constants
 *================================================================================================*/
#define TEST_DATA_LENGTH        16U
#define TEST_DATAID_P01         0xA5A5U
#define TEST_WRAP_COUNT         255U    /* Full rollover: 0→1→…→15→0 */

/*==================================================================================================
 *                                    Test Fixtures
 *================================================================================================*/
static E2E_P01ConfigType         g_config;
static E2E_P01ProtectStateType   g_protectState;
static E2E_P01CheckStateType     g_checkState;
static uint8                     g_data[TEST_DATA_LENGTH];

/*==================================================================================================
 *                                    Setup / Teardown
 *================================================================================================*/
static int setup(void **state)
{
    (void)state;

    /* Profile 1 配置 */
    memset(&g_config, 0, sizeof(g_config));
    g_config.DataID               = TEST_DATAID_P01;
    g_config.DataLength           = TEST_DATA_LENGTH;
    g_config.DataIDMode           = E2E_P01_DATAID_BOTH;
    g_config.CounterOffset        = 1U;
    g_config.CRCOffset            = 0U;
    g_config.DataIDNibbleOffset   = 2U;

    /* 清空状态 */
    memset(&g_protectState, 0, sizeof(g_protectState));
    memset(&g_checkState, 0, sizeof(g_checkState));
    g_checkState.MaxDeltaCounterInit = 3U;

    /* 清空数据缓冲区 */
    memset(g_data, 0, sizeof(g_data));

    return 0;
}

static int teardown(void **state)
{
    (void)state;
    return 0;
}

/*==================================================================================================
 *          Test 1: Profile 1 完整保护+校验 (test_e2e_profile1_end_to_end)
 *================================================================================================*/
static void test_e2e_profile1_end_to_end(void **state)
{
    (void)state;
    Std_ReturnType ret;

    /* ---- 1. E2E_Init / E2E_DeInit ---- */
    ret = E2E_Init(NULL);
    assert_int_equal(ret, E_OK);

    ret = E2E_DeInit();
    assert_int_equal(ret, E_OK);

    /* ---- 2. 保护数据 ---- */
    ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
    assert_int_equal(ret, E_OK);

    /* 验证 CRC 被写入（CRC offset 字节应非零） */
    {
        uint8 crcSum = 0U;
        for (uint16 i = 0U; i < 2U; i++)
        {
            crcSum |= g_data[g_config.CRCOffset + i];
        }
        assert_true(crcSum != 0U);
    }

    /* 验证计数器被写入 */
    {
        uint8 counter = (g_data[g_config.CounterOffset] >> 4U) & 0x0FU;
        assert_int_equal(counter, 0U);  /* 初始计数器值 */
    }

    /* ---- 3. 校验数据（首次应为 INITIAL） ---- */
    ret = E2E_P01Check(&g_config, &g_checkState, g_data);
    assert_int_equal(ret, E_OK);

    /* 首次检查：状态应为 INITIAL */
    assert_int_equal(g_checkState.Status, E2E_P_INITIAL);

    /* ---- 4. 连续保护+校验（应返回 OK） ---- */
    for (uint16 i = 0U; i < 5U; i++)
    {
        memset(g_data, 0, sizeof(g_data));
        ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
        assert_int_equal(ret, E_OK);

        ret = E2E_P01Check(&g_config, &g_checkState, g_data);
        assert_int_equal(ret, E_OK);
    }

    /* 连续 OK 后状态应为 OK */
    assert_int_equal(g_checkState.Status, E2E_P_OK);

    /* ---- 5. 状态机映射验证 ---- */
    {
        E2E_SMStateType smState;
        boolean         error;

        E2E_P01MapStatusToSM(E2E_P_OK, &smState, &error);
        assert_int_equal(smState, E2E_SM_VALID);
        assert_false(error);

        E2E_P01MapStatusToSM(E2E_P_INITIAL, &smState, &error);
        assert_int_equal(smState, E2E_SM_NODATA);
        assert_false(error);

        E2E_P01MapStatusToSM(E2E_P_SYNC, &smState, &error);
        assert_int_equal(smState, E2E_SM_INIT);
        assert_false(error);
    }

    /* ---- 6. 重复检测 ---- */
    /* 用相同数据再校验一次 => REPEATED */
    {
        E2E_P01CheckStateType repeatState;
        memset(&repeatState, 0, sizeof(repeatState));
        repeatState.MaxDeltaCounterInit = 3U;

        ret = E2E_P01Check(&g_config, &repeatState, g_data);
        assert_int_equal(ret, E_OK);

        /* 再次校验相同数据 */
        ret = E2E_P01Check(&g_config, &repeatState, g_data);
        assert_int_equal(ret, E_OK);
        assert_int_equal(repeatState.Status, E2E_P_REPEATED);
    }
}

/*==================================================================================================
 *          Test 2: CRC 错误注入检测 (test_e2e_crc_error_detection)
 *================================================================================================*/
static void test_e2e_crc_error_detection(void **state)
{
    (void)state;
    Std_ReturnType ret;
    uint8          savedByte;

    /* ---- 1. 保护数据 ---- */
    ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
    assert_int_equal(ret, E_OK);

    /* ---- 2. 校验通过 ---- */
    E2E_P01Check(&g_config, &g_checkState, g_data);

    /* ---- 3. 注入数据损坏 ---- */
    savedByte = g_data[5U];
    g_data[5U] ^= (uint8)0xFF;

    /* ---- 4. 校验应检测到 CRC 错误 ---- */
    {
        E2E_P01CheckStateType errState;
        memset(&errState, 0, sizeof(errState));
        errState.MaxDeltaCounterInit = 3U;

        ret = E2E_P01Check(&g_config, &errState, g_data);
        assert_int_equal(ret, E_OK);
        assert_int_equal(errState.Status, E2E_P_WRONGCRC);
    }

    /* 恢复并验证 */
    g_data[5U] = savedByte;

    /* ---- 5. 不同位置多次错误注入 ---- */
    for (uint16 offset = 3U; offset < TEST_DATA_LENGTH - 2U; offset += 4U)
    {
        uint8 saved = g_data[offset];
        g_data[offset] ^= (uint8)0xAA;

        {
            E2E_P01CheckStateType injState;
            memset(&injState, 0, sizeof(injState));
            injState.MaxDeltaCounterInit = 3U;

            ret = E2E_P01Check(&g_config, &injState, g_data);
            assert_int_equal(ret, E_OK);
            assert_int_equal(injState.Status, E2E_P_WRONGCRC);
        }

        g_data[offset] = saved;
    }

    /* ---- 6. 空指针参数校验 ---- */
    {
        ret = E2E_P01Protect(NULL, &g_protectState, g_data);
        assert_int_equal(ret, E_NOT_OK);

        ret = E2E_P01Protect(&g_config, NULL, g_data);
        assert_int_equal(ret, E_NOT_OK);

        ret = E2E_P01Protect(&g_config, &g_protectState, NULL);
        assert_int_equal(ret, E_NOT_OK);

        ret = E2E_P01Check(NULL, &g_checkState, g_data);
        assert_int_equal(ret, E_NOT_OK);

        ret = E2E_P01Check(&g_config, NULL, g_data);
        assert_int_equal(ret, E_NOT_OK);

        ret = E2E_P01Check(&g_config, &g_checkState, NULL);
        assert_int_equal(ret, E_NOT_OK);
    }
}

/*==================================================================================================
 *          Test 3: 计数器翻转场景 (test_e2e_counter_rollover)
 *================================================================================================*/
static void test_e2e_counter_rollover(void **state)
{
    (void)state;
    Std_ReturnType ret;
    uint8          prevCounter;

    /* ---- 1. 初始保护: 计数器从 0 开始 ---- */
    ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
    assert_int_equal(ret, E_OK);

    prevCounter = g_protectState.Counter;
    assert_int_equal(prevCounter, 0U);

    /* ---- 2. 正常递增至接近翻转 ---- */
    for (uint16 i = 1U; i <= 14U; i++)
    {
        memset(g_data, 0, sizeof(g_data));
        ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
        assert_int_equal(ret, E_OK);
        assert_int_equal(g_protectState.Counter, (uint8)i);
    }

    /* 现在计数器 == 14 */
    assert_int_equal(g_protectState.Counter, 14U);

    /* ---- 3. 14 → 15 ---- */
    memset(g_data, 0, sizeof(g_data));
    ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
    assert_int_equal(ret, E_OK);
    assert_int_equal(g_protectState.Counter, 15U);

    /* ---- 4. 15 → 0（翻转） ---- */
    memset(g_data, 0, sizeof(g_data));
    ret = E2E_P01Protect(&g_config, &g_protectState, g_data);
    assert_int_equal(ret, E_OK);
    assert_int_equal(g_protectState.Counter, 0U);

    /* ---- 5. 翻转后校验链路是否正常 ---- */
    {
        E2E_P01CheckStateType rollState;
        memset(&rollState, 0, sizeof(rollState));
        rollState.MaxDeltaCounterInit = 3U;

        /* 先发送一条建立基线 */
        memset(g_data, 0, sizeof(g_data));
        E2E_P01Protect(&g_config, &g_protectState, g_data);
        ret = E2E_P01Check(&g_config, &rollState, g_data);
        assert_int_equal(ret, E_OK);

        /* 再发一条连续数据 */
        memset(g_data, 0, sizeof(g_data));
        E2E_P01Protect(&g_config, &g_protectState, g_data);
        ret = E2E_P01Check(&g_config, &rollState, g_data);
        assert_int_equal(ret, E_OK);

        /* 翻转后连续 OK */
        assert_true((rollState.Status == E2E_P_OK) ||
                    (rollState.Status == E2E_P_OKSOMELOST));
    }

    /* ---- 6. 丢失检测：大幅跳跃后应触发 SYNC ---- */
    {
        E2E_P01CheckStateType lostState;
        memset(&lostState, 0, sizeof(lostState));
        lostState.MaxDeltaCounterInit = 3U;

        memset(g_data, 0, sizeof(g_data));
        E2E_P01Protect(&g_config, &g_protectState, g_data);
        ret = E2E_P01Check(&g_config, &lostState, g_data);
        assert_int_equal(ret, E_OK);

        /* 大幅跳跃 */
        g_protectState.Counter = (g_protectState.Counter + 10U) & 0x0FU;

        memset(g_data, 0, sizeof(g_data));
        E2E_P01Protect(&g_config, &g_protectState, g_data);
        ret = E2E_P01Check(&g_config, &lostState, g_data);
        assert_int_equal(ret, E_OK);
        assert_int_equal(lostState.Status, E2E_P_SYNC);
    }
}

/*==================================================================================================
 *                                      Main Test Suite
 *================================================================================================*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(
            test_e2e_profile1_end_to_end, setup, teardown),
        cmocka_unit_test_setup_teardown(
            test_e2e_crc_error_detection, setup, teardown),
        cmocka_unit_test_setup_teardown(
            test_e2e_counter_rollover, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
