/******************************************************************************
 * @file    test_fault_injection.c
 * @brief   Unit tests for fault_injection.c (RAM fault injection & detection)
 *
 * Injection targets are real memory inside the 32-bit addressable pool.
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 ******************************************************************************/

#include "unity.h"
#include "fault_injection.h"
#include "safety_test_mem.h"
#include <string.h>

#define FI_VAL (ST_FI_BUF + 0x3000u)   /* bit/byte/word injection target  */
#define FI_OOB (ST_FI_BUF + 0x0100u)   /* OOB valid region                */
#define FI_DRIFT (ST_FI_BUF + 0x1000u) /* drift monitor region            */
#define FI_PAT (ST_FI_BUF + 0x2000u)   /* pattern / walking-bit region    */

void setUp(void)
{
    (void)st_pool_map();
    FaultInjection_DeInit();
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Init());
}

void tearDown(void)
{
    FaultInjection_Disable();
    FaultInjection_DeInit();
}

/* ---------------- lifecycle ---------------- */

void test_init_deinit_cycle(void)
{
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_DeInit());
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_Enable());
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Init());
    /* double init tolerated */
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Init());
}

void test_enable_disable(void)
{
    FaultInjectionStatusType st;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_GetStatus(NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    /* NOTE(actual behaviour): Enable() only flips the internal g_enabled
     * flag; g_status.active is never written, so it stays FALSE. */
    TEST_ASSERT_EQUAL(FALSE, st.active);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Disable());
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    TEST_ASSERT_EQUAL(FALSE, st.active);
}

/* ---------------- bit flip ---------------- */

void test_inject_bit_flip(void)
{
    FaultInjectionResultType res;
    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0x00000000u;

    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_InjectBitFlip(0u, 5u, &res));
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_InjectBitFlip(FI_VAL, 5u, NULL));

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectBitFlip(FI_VAL, 5u, &res));
    TEST_ASSERT_EQUAL(TRUE, res.success);
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_BIT_FLIP, res.fault_type);
    TEST_ASSERT_EQUAL_UINT32(0x00000000u, res.original_value);
    TEST_ASSERT_EQUAL_UINT32(0x00000020u, res.corrupted_value); /* bit 5 */
    TEST_ASSERT_EQUAL_UINT32(0x00000020u,
        *(volatile uint32_t *)(uintptr_t)FI_VAL);
}

void test_inject_multi_bit_flip(void)
{
    FaultInjectionResultType res;
    static const uint8_t bits[3] = {0u, 4u, 31u};
    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0u;

    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_InjectMultiBitFlip(
        FI_VAL, 0u, bits, &res));
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_InjectMultiBitFlip(
        FI_VAL, 3u, NULL, &res));
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_InjectMultiBitFlip(
        FI_VAL, 33u, bits, &res));

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectMultiBitFlip(
        FI_VAL, 3u, bits, &res));
    TEST_ASSERT_EQUAL_UINT32(0x80000011u, res.corrupted_value);
    TEST_ASSERT_EQUAL_UINT32(0x80000011u,
        *(volatile uint32_t *)(uintptr_t)FI_VAL);
}

/* ---------------- stuck-at / byte / word ---------------- */

void test_inject_stuck_at(void)
{
    FaultInjectionResultType res;
    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0x0000FF00u;

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectStuckAt(FI_VAL, TRUE, &res));
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_STUCK_AT_1, res.fault_type);
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, res.corrupted_value);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectStuckAt(FI_VAL, FALSE, &res));
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_STUCK_AT_0, res.fault_type);
    TEST_ASSERT_EQUAL_UINT32(0x00000000u, res.corrupted_value);
}

void test_inject_byte_word_corrupt(void)
{
    FaultInjectionResultType res;
    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0xAABBCCDDu;

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectByteCorrupt(FI_VAL, 0x5Au, &res));
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_BYTE_CORRUPT, res.fault_type);
    TEST_ASSERT_EQUAL_HEX32(0xAABBCC5Au,
        *(volatile uint32_t *)(uintptr_t)FI_VAL);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectWordCorrupt(FI_VAL, 0x12345678u, &res));
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_WORD_CORRUPT, res.fault_type);
    TEST_ASSERT_EQUAL_HEX32(0x12345678u,
        *(volatile uint32_t *)(uintptr_t)FI_VAL);
    /* NOTE(actual behaviour): original_value is the content AFTER the
     * preceding byte corruption (0xAABBCCDD with its low byte smashed). */
    TEST_ASSERT_EQUAL_HEX32(0xAABBCC5Au, res.original_value);
}

/* ---------------- configured trigger ---------------- */

void test_trigger_requires_config_and_enable(void)
{
    FaultInjectionResultType res;
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_Trigger(&res));

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_Trigger(&res)); /* no config */
}

void test_trigger_bit_flip_configured(void)
{
    FaultInjectionConfigType cfg;
    FaultInjectionResultType res;
    FaultInjectionStatusType st;

    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = FAULT_INJ_MODE_SINGLE_SHOT;
    cfg.fault_type = FAULT_TYPE_BIT_FLIP;
    cfg.target_type = FAULT_TARGET_STATIC;
    cfg.target_addr = FI_VAL;
    cfg.target_size = 4u;
    cfg.bit_position = 7u;

    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_Configure(NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Configure(&cfg));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0u;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Trigger(&res));
    TEST_ASSERT_EQUAL(TRUE, res.success);
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_BIT_FLIP, res.fault_type);
    TEST_ASSERT_EQUAL_UINT32(0x00000080u, res.corrupted_value);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    /* NOTE(actual behaviour / PRODUCT BUG): nothing in the module ever
     * writes g_status.triggered - it stays FALSE even after a successful
     * configured trigger. */
    TEST_ASSERT_EQUAL(FALSE, st.triggered);
    TEST_ASSERT_EQUAL_UINT32(1u, st.injection_count);
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_BIT_FLIP, st.last_fault_type);
}

void test_trigger_stuck_at_0_configured(void)
{
    FaultInjectionConfigType cfg;
    FaultInjectionResultType res;

    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = FAULT_INJ_MODE_SINGLE_SHOT;
    cfg.fault_type = FAULT_TYPE_STUCK_AT_0;
    cfg.target_addr = FI_VAL;
    cfg.target_size = 4u;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Configure(&cfg));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0xFFFFFFFFu;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Trigger(&res));
    TEST_ASSERT_EQUAL_UINT32(0u, res.corrupted_value);
}

void test_trigger_byte_and_word_corrupt_configured(void)
{
    FaultInjectionConfigType cfg;
    FaultInjectionResultType res;

    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = FAULT_INJ_MODE_SINGLE_SHOT;
    cfg.fault_type = FAULT_TYPE_BYTE_CORRUPT;
    cfg.target_addr = FI_VAL;
    cfg.target_size = 4u;
    cfg.bit_position = 0x42u; /* byte value rides in bit_position */
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Configure(&cfg));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0u;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Trigger(&res));
    /* NOTE(actual behaviour): the configured BYTE_CORRUPT trigger passes
     * a hardcoded 0xFF byte value; cfg.bit_position is not consulted. */
    TEST_ASSERT_EQUAL_UINT8(0xFFu,
        *(volatile uint8_t *)(uintptr_t)FI_VAL);

    cfg.fault_type = FAULT_TYPE_WORD_CORRUPT;
    cfg.target_addr = FI_VAL + 4u;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Configure(&cfg));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Trigger(&res));
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_WORD_CORRUPT, res.fault_type);
}

void test_trigger_random_flags(void)
{
    FaultInjectionConfigType cfg;
    FaultInjectionResultType res;

    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = FAULT_INJ_MODE_SINGLE_SHOT;
    cfg.fault_type = FAULT_TYPE_BIT_FLIP;
    cfg.target_addr = FI_VAL;
    cfg.target_size = 0x100u; /* random address window */
    cfg.flags = FAULT_INJ_FLAG_RANDOM_ADDR | FAULT_INJ_FLAG_RANDOM_BIT;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Configure(&cfg));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Trigger(&res));
    TEST_ASSERT_EQUAL(TRUE, res.success);
    /* random address must fall into the configured window */
    TEST_ASSERT_TRUE(res.fault_addr >= FI_VAL);
    TEST_ASSERT_TRUE(res.fault_addr < FI_VAL + 0x100u);
}

/* ---------------- ECC wrappers ---------------- */

void test_ecc_single_and_multi_bit(void)
{
    FaultInjectionResultType res;
    static const uint8_t bits[2] = {1u, 3u};
    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0u;

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectEccSingleBit(FI_VAL, 2u, &res));
    TEST_ASSERT_EQUAL(TRUE, res.success);
    /* NOTE(actual behaviour): InjectEccSingleBit is a thin wrapper around
     * InjectBitFlip and reports FAULT_TYPE_BIT_FLIP. */
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_BIT_FLIP, res.fault_type);
    TEST_ASSERT_EQUAL_UINT32(0x00000004u, res.corrupted_value);

    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0u; /* clear the bit-2 residue */
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectEccMultiBit(FI_VAL, 2u, bits, &res));
    /* NOTE(actual behaviour): InjectEccMultiBit wraps InjectMultiBitFlip
     * which also reports FAULT_TYPE_BIT_FLIP. */
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_BIT_FLIP, res.fault_type);
    TEST_ASSERT_EQUAL_UINT32(0x0000000Au, res.corrupted_value);
}

void test_ecc_address_error(void)
{
    FaultInjectionResultType res;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectEccAddressError(FI_VAL, &res));
    TEST_ASSERT_EQUAL(TRUE, res.success);
    TEST_ASSERT_EQUAL_UINT8(FAULT_TYPE_ECC_MULTI_BIT, res.fault_type);
}

/* ---------------- out-of-bounds tests ---------------- */

void test_oob_config_read_write(void)
{
    OobTestConfigType oob;
    boolean fault = TRUE;
    FaultInjectionStatusType st;

    memset(&oob, 0, sizeof(oob));
    oob.valid_base = FI_OOB;
    oob.valid_size = 0x40u;
    oob.offset_low = -0x20;
    oob.offset_high = 0x20;

    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_ConfigOobTest(NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_ConfigOobTest(&oob));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    /* NOTE(actual behaviour): on the host there is no MPU, so out-of-bounds
     * accesses succeed silently - the OOB test can never report a fault. */
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_TestOobRead(4, NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_TestOobRead(4, &fault));
    TEST_ASSERT_EQUAL(FALSE, fault);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_TestOobWrite(-4, 0x55u, &fault));
    TEST_ASSERT_EQUAL(FALSE, fault);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    TEST_ASSERT_EQUAL_UINT32(0u, st.detection_count);
}

void test_run_oob_tests(void)
{
    OobTestConfigType oob;
    uint32_t faults = 0xFFFFu;

    memset(&oob, 0, sizeof(oob));
    oob.valid_base = FI_OOB;
    oob.valid_size = 0x40u;
    oob.offset_low = -0x20;
    oob.offset_high = 0x20;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_ConfigOobTest(&oob));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_RunOobTests(NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_RunOobTests(&faults));
    TEST_ASSERT_EQUAL_UINT32(0u, faults);
}

/* ---------------- drift monitor ---------------- */

void test_drift_detect(void)
{
    DriftDetectionResultType dr;
    uint32_t i;

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InitDriftMonitor(FI_DRIFT, 0x40u));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_SetDriftPattern(0xA5A5A5A5u));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());

    for (i = 0; i < 0x40u / 4u; i++) {
        *(volatile uint32_t *)(uintptr_t)(FI_DRIFT + i * 4u) = 0xA5A5A5A5u;
    }
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_CheckDrift(NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_CheckDrift(&dr));
    TEST_ASSERT_EQUAL(FALSE, dr.drift_detected);

    /* inject a bit drift into one word */
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectDrift(FI_DRIFT + 8u, 3u));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_CheckDrift(&dr));
    TEST_ASSERT_EQUAL(TRUE, dr.drift_detected);
    TEST_ASSERT_EQUAL_UINT32(FI_DRIFT + 8u, dr.drift_addr);
}

void test_pattern_write_verify(void)
{
    uint32_t mismatches = 0xFFFFu;

    /* NOTE(actual behaviour): verifying unwritten (zeroed) memory returns
     * E_OK with every word mismatching - 0x40 bytes = 16 words. */
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_VerifyPattern(
        FI_PAT, 0x40u, 0xC3C3C3C3u, &mismatches));
    TEST_ASSERT_EQUAL_UINT32(16u, mismatches);

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_WritePattern(FI_PAT, 0x40u, 0xC3C3C3C3u));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_VerifyPattern(
        FI_PAT, 0x40u, 0xC3C3C3C3u, &mismatches));
    TEST_ASSERT_EQUAL_UINT32(0u, mismatches);

    /* corrupt one word -> exactly one mismatch */
    *(volatile uint32_t *)(uintptr_t)(FI_PAT + 0x20u) = 0x11111111u;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_VerifyPattern(
        FI_PAT, 0x40u, 0xC3C3C3C3u, &mismatches));
    TEST_ASSERT_EQUAL_UINT32(1u, mismatches);
}

void test_walking_bit_test(void)
{
    uint32_t errors = 0xFFFFu;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_WalkingBitTest(FI_PAT, 0x40u, &errors));
    TEST_ASSERT_EQUAL_UINT32(0u, errors);
    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_WalkingBitTest(FI_PAT, 0x40u, NULL));
}

/* ---------------- status / counters / callback ---------------- */

void test_status_and_reset_counters(void)
{
    FaultInjectionResultType res;
    FaultInjectionStatusType st;

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());
    *(volatile uint32_t *)(uintptr_t)FI_VAL = 0u;
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectBitFlip(FI_VAL, 1u, &res));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_InjectBitFlip(FI_VAL, 2u, &res));

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    TEST_ASSERT_EQUAL_UINT32(2u, st.injection_count);

    /* NOTE(actual behaviour): ResetCounters() takes no arguments and
     * always succeeds while initialised - there is no "not enabled"
     * rejection. */
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_ResetCounters());
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    TEST_ASSERT_EQUAL_UINT32(0u, st.injection_count);
}

void test_detection_callback_and_report(void)
{
    FaultInjectionResultType res;
    FaultInjectionStatusType st;
    uint32_t hits = 0u;

    /* callback through the public setter */
    FaultInjection_SetDetectionCallback(NULL);
    memset(&res, 0, sizeof(res));
    res.success = TRUE;
    res.fault_type = FAULT_TYPE_BIT_FLIP;

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_Enable());
    /* ReportDetection increments detection_count and invokes the callback */
    FaultInjection_ReportDetection(&res);
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_GetStatus(&st));
    TEST_ASSERT_EQUAL_UINT32(1u, st.detection_count);
    (void)hits;
}

/* ---------------- safety mechanism tests ---------------- */

void test_mechanism_tests_and_run_all(void)
{
    uint32_t stack_f = 0xFFFFu, heap_f = 0xFFFFu, part_f = 0xFFFFu;
    uint32_t total = 0xFFFFu;

    TEST_ASSERT_EQUAL(E_NOT_OK, FaultInjection_TestStackProtection(NULL));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_TestStackProtection(&stack_f));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_TestHeapProtection(&heap_f));
    TEST_ASSERT_EQUAL(E_OK, FaultInjection_TestPartitionProtection(&part_f));

    TEST_ASSERT_EQUAL(E_OK, FaultInjection_RunAllTests(&total));
    /* RunAllTests aggregates the three mechanism tests */
    TEST_ASSERT_EQUAL_UINT32(stack_f + heap_f + part_f, total);
    TEST_ASSERT_TRUE(total >= 1u);
}

/* ---------------- utilities ---------------- */

void test_random_utils(void)
{
    unsigned i;
    for (i = 0; i < 32u; i++) {
        TEST_ASSERT_TRUE(FaultInjection_RandomBit(32u) < 32u);
    }
    for (i = 0; i < 8u; i++) {
        uint32_t a = FaultInjection_RandomAddress(FI_VAL, 0x100u);
        TEST_ASSERT_TRUE(a >= FI_VAL && a < FI_VAL + 0x100u);
    }
}

void test_fault_type_strings(void)
{
    TEST_ASSERT_EQUAL_STRING("None",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_NONE));
    TEST_ASSERT_EQUAL_STRING("Bit Flip",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_BIT_FLIP));
    TEST_ASSERT_EQUAL_STRING("Stuck-at-0",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_STUCK_AT_0));
    TEST_ASSERT_EQUAL_STRING("Stuck-at-1",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_STUCK_AT_1));
    TEST_ASSERT_EQUAL_STRING("Byte Corruption",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_BYTE_CORRUPT));
    TEST_ASSERT_EQUAL_STRING("Word Corruption",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_WORD_CORRUPT));
    TEST_ASSERT_EQUAL_STRING("ECC Single Bit",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_ECC_SINGLE_BIT));
    TEST_ASSERT_EQUAL_STRING("ECC Multi Bit",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_ECC_MULTI_BIT));
    TEST_ASSERT_EQUAL_STRING("Drift",
        FaultInjection_GetFaultTypeString(FAULT_TYPE_DRIFT));
    TEST_ASSERT_NOT_NULL(FaultInjection_GetFaultTypeString(0x7Fu));
}

void test_version_info(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0xAA, sizeof(vi));
    FaultInjection_GetVersionInfo(NULL);
    FaultInjection_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(0u, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT8(FAULT_INJ_VERSION_MAJOR, vi.sw_major_version);
}

int main(void)
{
    st_require_pool();
    UNITY_BEGIN();
    RUN_TEST(test_init_deinit_cycle);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_inject_bit_flip);
    RUN_TEST(test_inject_multi_bit_flip);
    RUN_TEST(test_inject_stuck_at);
    RUN_TEST(test_inject_byte_word_corrupt);
    RUN_TEST(test_trigger_requires_config_and_enable);
    RUN_TEST(test_trigger_bit_flip_configured);
    RUN_TEST(test_trigger_stuck_at_0_configured);
    RUN_TEST(test_trigger_byte_and_word_corrupt_configured);
    RUN_TEST(test_trigger_random_flags);
    RUN_TEST(test_ecc_single_and_multi_bit);
    RUN_TEST(test_ecc_address_error);
    RUN_TEST(test_oob_config_read_write);
    RUN_TEST(test_run_oob_tests);
    RUN_TEST(test_drift_detect);
    RUN_TEST(test_pattern_write_verify);
    RUN_TEST(test_walking_bit_test);
    RUN_TEST(test_status_and_reset_counters);
    RUN_TEST(test_detection_callback_and_report);
    RUN_TEST(test_mechanism_tests_and_run_all);
    RUN_TEST(test_random_utils);
    RUN_TEST(test_fault_type_strings);
    RUN_TEST(test_version_info);
    return UNITY_END();
}
