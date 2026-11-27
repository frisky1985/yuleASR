/**
 * @file test_e2e_profiles_p02.c
 * @brief E2E Profile 2 (Dual Path / CRC8) unit tests
 * @details Covers E2E_P02Protect / E2E_P02Check / E2E_P02MapStatusToSM
 *          against the production implementation in src/bsw/services/e2e.
 *
 *          Profile 2 protects an 8-bit counter + 1-byte CRC8 over the data.
 *          The CRC field is excluded from its own input (zeroed before
 *          calculation in Protect, and via a local copy in Check because
 *          Data is const) — mirroring E2E_P04. This keeps protect -> check
 *          round-trips stable so the state machine can be exercised directly.
 */

#include <string.h>
#include "unity.h"
#include "E2E.h"
#include "E2E_P02.h"

/* 8-byte frame: counter at [0], CRC8 at [1], data-ID/path nibble at [2],
 * payload at [3..7]. */
#define P02_LEN      8U
#define P02_CNT_OFF  0U
#define P02_CRC_OFF  1U
#define P02_NIB_OFF  2U

static const E2E_P02ConfigType cfgDual = {
    0x1234U,      /* DataID            */
    P02_LEN,      /* DataLength        */
    P02_CNT_OFF,  /* CounterOffset     */
    P02_CRC_OFF,  /* CRCOffset         */
    P02_NIB_OFF,  /* DataIDNibbleOffset*/
    TRUE          /* DualPathEnabled   */
};

static const E2E_P02ConfigType cfgSingle = {
    0x1234U,
    P02_LEN,
    P02_CNT_OFF,
    P02_CRC_OFF,
    P02_NIB_OFF,
    FALSE
};

static E2E_P02ProtectStateType ps;
static E2E_P02CheckStateType   cs;
static uint8 frame[P02_LEN];

void setUp(void)
{
    memset(&ps, 0, sizeof(ps));
    memset(&cs, 0, sizeof(cs));
    memset(frame, 0, sizeof(frame));
}

void tearDown(void)
{
}

/* Produce a valid protected frame carrying the requested counter/path. */
static void makeFrame(const E2E_P02ConfigType* cfg, uint8 counter, uint8 pathId)
{
    memset(frame, 0, sizeof(frame));
    frame[3] = 0xA5U;
    frame[7] = 0x5AU;
    ps.Counter = counter;
    ps.PathId  = pathId;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Protect(cfg, &ps, frame));
}

static void test_P02_Protect_NullArgs_ShouldReturnNotOk(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Protect(NULL_PTR, &ps, frame));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Protect(&cfgDual, NULL_PTR, frame));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Protect(&cfgDual, &ps, NULL_PTR));
}

static void test_P02_Check_NullArgs_ShouldReturnNotOk(void)
{
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Check(NULL_PTR, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Check(&cfgDual, NULL_PTR, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Check(&cfgDual, &cs, NULL_PTR, 0U));
}

static void test_P02_Check_InvalidPathId_ShouldReturnNotOk(void)
{
    makeFrame(&cfgDual, 1U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P02Check(&cfgDual, &cs, frame, 2U));
}

static void test_P02_Protect_SinglePath_DoesNotTouchPath(void)
{
    ps.Counter = 7U;
    ps.PathId  = 0U;
    frame[3] = 0xA5U;

    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Protect(&cfgSingle, &ps, frame));

    /* Counter written, then incremented */
    TEST_ASSERT_EQUAL_UINT8(7U, frame[P02_CNT_OFF]);
    TEST_ASSERT_EQUAL_UINT8(8U, ps.Counter);

    /* Single path: path nibble untouched, PathId not toggled, CRC written */
    TEST_ASSERT_EQUAL_UINT8(0U, frame[P02_NIB_OFF]);
    TEST_ASSERT_EQUAL_UINT8(0U, ps.PathId);
    TEST_ASSERT_TRUE(frame[P02_CRC_OFF] != 0U);
}

static void test_P02_Protect_DualPath_TogglesPathIdAndNibble(void)
{
    ps.Counter = 1U;
    ps.PathId  = 0U;
    frame[3] = 0xA5U;

    /* First transmission on path 0: high nibble of byte 2 == 0x0 */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Protect(&cfgDual, &ps, frame));
    TEST_ASSERT_EQUAL_UINT8(0x00U, frame[P02_NIB_OFF] & 0xF0U);
    TEST_ASSERT_EQUAL_UINT8(1U, ps.PathId);   /* toggled 0 -> 1 */

    /* Second transmission on path 1: high nibble == 0x1 */
    ps.Counter = 2U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Protect(&cfgDual, &ps, frame));
    TEST_ASSERT_EQUAL_UINT8(0x10U, frame[P02_NIB_OFF] & 0xF0U);
    TEST_ASSERT_EQUAL_UINT8(0U, ps.PathId);   /* toggled 1 -> 0 */
}

static void test_P02_ProtectCheck_InitialThenOk(void)
{
    cs.MaxDeltaCounterInit = 3U;

    makeFrame(&cfgSingle, 5U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgSingle, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(1U, cs.WaitForFirstData);
    TEST_ASSERT_EQUAL_UINT8(5U, cs.LastValidCounter);
    TEST_ASSERT_EQUAL_UINT8(0U, cs.PathStatus[0]);

    /* Counter +1 -> OK */
    makeFrame(&cfgSingle, 6U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgSingle, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(6U, cs.LastValidCounter);
    TEST_ASSERT_EQUAL_UINT8(0U, cs.PathStatus[0]);
}

static void test_P02_Check_StatusSequence(void)
{
    cs.MaxDeltaCounterInit = 3U;

    makeFrame(&cfgDual, 10U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_INITIAL, cs.Status);   /* WaitForFirstData==0 */

    /* Same frame again -> REPEATED (delta == 0) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_REPEATED, cs.Status);

    /* +1 -> OK (delta == 1) */
    makeFrame(&cfgDual, 11U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OK, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(11U, cs.LastValidCounter);

    /* +3 within tolerance -> OKSOMELOST (1 < delta <= MaxDeltaCounterInit) */
    makeFrame(&cfgDual, 14U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_OKSOMELOST, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(14U, cs.LastValidCounter);

    /* Large jump beyond tolerance -> SYNC (delta > MaxDeltaCounterInit) */
    makeFrame(&cfgDual, 20U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_SYNC, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(20U, cs.LastValidCounter);

    /* Counter went backwards -> WRONGSEQUENCE (delta < 0) */
    makeFrame(&cfgDual, 5U, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 0U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGSEQUENCE, cs.Status);
}

static void test_P02_Check_CorruptedPayload_ShouldReportWrongCrc(void)
{
    makeFrame(&cfgDual, 10U, 1U);
    frame[3] ^= 0xFFU;   /* corrupt a payload byte */

    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P02Check(&cfgDual, &cs, frame, 1U));
    TEST_ASSERT_EQUAL_UINT8(E2E_P_WRONGCRC, cs.Status);
    TEST_ASSERT_EQUAL_UINT8(1U, cs.PathStatus[1]);   /* error flagged on path 1 */
}

static void test_P02_MapStatusToSM_AllBranches(void)
{
    E2E_SMStateType sm;
    boolean error;

    sm = (E2E_SMStateType)0xFF; error = TRUE;
    E2E_P02MapStatusToSM(E2E_P_OK, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(error);

    E2E_P02MapStatusToSM(E2E_P_OKSOMELOST, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_VALID, sm);
    TEST_ASSERT_FALSE(error);

    E2E_P02MapStatusToSM(E2E_P_WRONGCRC, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    E2E_P02MapStatusToSM(E2E_P_WRONGSEQUENCE, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    E2E_P02MapStatusToSM(E2E_P_REPEATED, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_TRUE(error);

    E2E_P02MapStatusToSM(E2E_P_SYNC, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INIT, sm);
    TEST_ASSERT_FALSE(error);

    E2E_P02MapStatusToSM(E2E_P_INITIAL, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_NODATA, sm);
    TEST_ASSERT_FALSE(error);

    /* Unknown status falls into the default branch (INVALID, no error) */
    E2E_P02MapStatusToSM((E2E_PCheckStatusType)0x7F, &sm, &error);
    TEST_ASSERT_EQUAL_UINT8(E2E_SM_INVALID, sm);
    TEST_ASSERT_FALSE(error);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_P02_Protect_NullArgs_ShouldReturnNotOk);
    RUN_TEST(test_P02_Check_NullArgs_ShouldReturnNotOk);
    RUN_TEST(test_P02_Check_InvalidPathId_ShouldReturnNotOk);
    RUN_TEST(test_P02_Protect_SinglePath_DoesNotTouchPath);
    RUN_TEST(test_P02_Protect_DualPath_TogglesPathIdAndNibble);
    RUN_TEST(test_P02_ProtectCheck_InitialThenOk);
    RUN_TEST(test_P02_Check_StatusSequence);
    RUN_TEST(test_P02_Check_CorruptedPayload_ShouldReportWrongCrc);
    RUN_TEST(test_P02_MapStatusToSM_AllBranches);
    return UnityEnd();
}
