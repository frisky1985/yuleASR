/**
 * @file test_e2e_profiles_p04.c
 * @brief E2E Profile 4 (CRC32) unit tests
 * @details Covers E2E_P04Protect / E2E_P04Check / E2E_P04MapStatusToSM
 *          against the production implementation in src/bsw/services/e2e.
 *
 *          Profile 4 is self-contained (local CRC32 table) and, unlike
 *          Profile 2, zeroes the CRC field before calculating, so protect ->
 *          check round-trips are stable and can be asserted directly.
 */

#include <string.h>
#include "unity.h"
#include "E2E.h"
#include "E2E_P04.h"

/* 16-byte frame: counter at [0..1] (big-endian), CRC32 at [2..5], payload 6..15 */
#define P04_LEN     16U
#define P04_CNT_OFF 0U
#define P04_CRC_OFF 2U

static const E2E_P04ConfigType cfgId = {
    0x11223344U,   /* DataID */
    P04_LEN,       /* DataLength */
    P04_CNT_OFF,   /* CounterOffset */
    P04_CRC_OFF,   /* CRCOffset */
    TRUE           /* IncludeDataID */
};

static const E2E_P04ConfigType cfgNoId = {
    0x11223344U,
    P04_LEN,
    P04_CNT_OFF,
    P04_CRC_OFF,
    FALSE
};

static E2E_P04ProtectStateType ps;
static E2E_P04CheckStateType   cs;
static uint8 frame[P04_LEN];

void setUp(void)
{
    memset(&ps, 0, sizeof(ps));
    memset(&cs, 0, sizeof(cs));
    memset(frame, 0, sizeof(frame));
}

void tearDown(void)
{
}

/* Produce a valid protected frame carrying the requested counter value. */
static void makeFrame(const E2E_P04ConfigType* cfg, uint16 counter)
{
    memset(frame, 0, sizeof(frame));
    frame[6]  = 0xA5U;
    frame[15] = 0x5AU;
    ps.Counter = counter;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Protect(cfg, &ps, frame));
}

static void test_P04_Protect_NullArgs_ShouldReturnNotOk(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P04Protect(NULL_PTR, &ps, frame));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P04Protect(&cfgId, NULL_PTR, frame));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P04Protect(&cfgId, &ps, NULL_PTR));
}

static void test_P04_Check_NullArgs_ShouldReturnNotOk(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P04Check(NULL_PTR, &cs, frame));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P04Check(&cfgId, NULL_PTR, frame));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P04Check(&cfgId, &cs, NULL_PTR));
}

static void test_P04_Protect_WritesCounterAndCrc_ThenCheckInitial(void)
{
    ps.Counter = 0x0102U;
    frame[6] = 0xA5U;

    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Protect(&cfgId, &ps, frame));

    /* 16-bit counter, big-endian */
    TEST_ASSERT_EQUAL_UINT8(0x01U, frame[0]);
    TEST_ASSERT_EQUAL_UINT8(0x02U, frame[1]);
    TEST_ASSERT_EQUAL_UINT16(0x0103U, ps.Counter);

    /* First check reports INITIAL and latches the counter */
    cs.MaxDeltaCounterInit = 3U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(1U, cs.WaitForFirstData);
    TEST_ASSERT_EQUAL_UINT16(0x0102U, cs.LastValidCounter);
}

static void test_P04_ProtectCheck_NoDataId_Roundtrip(void)
{
    makeFrame(&cfgNoId, 7U);
    cs.MaxDeltaCounterInit = 3U;

    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgNoId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, cs.Status);
    TEST_ASSERT_EQUAL_UINT16(7U, cs.LastValidCounter);

    makeFrame(&cfgNoId, 8U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgNoId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_OK, cs.Status);
    TEST_ASSERT_EQUAL_UINT16(8U, cs.LastValidCounter);
}

static void test_P04_Check_StatusSequence(void)
{
    cs.MaxDeltaCounterInit = 3U;

    makeFrame(&cfgId, 10U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, cs.Status);

    /* Same frame again -> repeated */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_REPEATED, cs.Status);

    /* +1 -> OK */
    makeFrame(&cfgId, 11U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_OK, cs.Status);
    TEST_ASSERT_EQUAL_UINT16(11U, cs.LastValidCounter);

    /* +3 (within tolerance) -> OKSOMELOST */
    makeFrame(&cfgId, 14U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_OKSOMELOST, cs.Status);
    TEST_ASSERT_EQUAL_UINT16(14U, cs.LastValidCounter);

    /* Large jump -> SYNC */
    makeFrame(&cfgId, 30U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_SYNC, cs.Status);
    TEST_ASSERT_EQUAL_UINT16(30U, cs.LastValidCounter);

    /* Counter went backwards -> WRONGSEQUENCE */
    makeFrame(&cfgId, 5U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_WRONGSEQUENCE, cs.Status);
}

static void test_P04_Check_CorruptedPayload_ShouldReportWrongCrc(void)
{
    makeFrame(&cfgId, 10U);
    frame[6] ^= 0xFFU;

    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P04Check(&cfgId, &cs, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_WRONGCRC, cs.Status);
}

static void test_P04_MapStatusToSM_AllCases(void)
{
    E2E_SMStateType sm;
    boolean error;

    sm = (E2E_SMStateType)0xFF; error = TRUE;
    E2E_P04MapStatusToSM(E2E_P_OK, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(error);

    E2E_P04MapStatusToSM(E2E_P_OKSOMELOST, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(error);

    E2E_P04MapStatusToSM(E2E_P_WRONGCRC, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    E2E_P04MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    E2E_P04MapStatusToSM(E2E_P_REPEATED, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    E2E_P04MapStatusToSM(E2E_P_SYNC, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INIT, sm);
    TEST_ASSERT_FALSE(error);

    E2E_P04MapStatusToSM(E2E_P_INITIAL, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_NODATA, sm);
    TEST_ASSERT_FALSE(error);

    /* Unknown status falls into the default branch */
    E2E_P04MapStatusToSM((E2E_PCheckStatusType)0x7F, &sm, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, sm);
    TEST_ASSERT_FALSE(error);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_P04_Protect_NullArgs_ShouldReturnNotOk);
    RUN_TEST(test_P04_Check_NullArgs_ShouldReturnNotOk);
    RUN_TEST(test_P04_Protect_WritesCounterAndCrc_ThenCheckInitial);
    RUN_TEST(test_P04_ProtectCheck_NoDataId_Roundtrip);
    RUN_TEST(test_P04_Check_StatusSequence);
    RUN_TEST(test_P04_Check_CorruptedPayload_ShouldReportWrongCrc);
    RUN_TEST(test_P04_MapStatusToSM_AllCases);
    return UnityEnd();
}
