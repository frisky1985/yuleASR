/**
 * @file test_e2e.c
 * @brief E2E Unit Tests (substantiated against production E2E.c / E2E_P01.c)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @note The original test file exercised fabricated module-level APIs
 *       (E2E_Protect / E2E_Check / E2E_GetVersionInfo / E2E_RegisterProfile /
 *       E2E_MainFunction and an E2E_ConfigType) which do not exist in
 *       src/bsw/services/e2e. They are remapped onto the real production API:
 *         - E2E_Protect*          -> E2E_P01Protect()
 *         - E2E_Check*            -> E2E_P01Check()
 *         - E2E_GetVersionInfo*   -> E2E_P01MapStatusToSM()
 *         - E2E_RegisterProfile*  -> E2E_P01Check()/E2E_P01MapStatusToSM() roundtrip
 *         - E2E_MainFunction*     -> E2E_DeInit()/re-init lifecycle
 *       Observed SUT behaviour that differs from the old expectations:
 *         - E2E_Init(NULL) returns E2E_E_INPUTERR_NULL (no DET call; the SUT
 *           never calls Det_ReportError).
 *         - E2E_Init has NO double-init guard: a second init returns E_OK.
 *         - E2E_P01Protect/E2E_P01Check return E_NOT_OK on NULL arguments
 *           without any DET report.
 */

#include <string.h>
#include "unity.h"
#include "E2E.h"
#include "E2E_P01.h"

/* E2E_Init takes an opaque non-NULL pointer; content is not dereferenced */
static uint8 e2eDummyConfig;

/* Shared Profile 1 configuration: 4-byte frame, counter byte 0, CRC byte 1 */
static const E2E_P01ConfigType testConfig = {
    0x1234U,                    /* DataID */
    4U,                         /* DataLength */
    E2E_P01_DATAID_BOTH,        /* DataIDMode */
    0U,                         /* CounterOffset */
    1U,                         /* CRCOffset */
    0U                          /* DataIDNibbleOffset */
};

static E2E_P01ProtectStateType protectState;
static E2E_P01CheckStateType checkState;
static uint8 frame[4];

void setUp(void) {
    memset(&protectState, 0, sizeof(protectState));
    memset(&checkState, 0, sizeof(checkState));
    memset(frame, 0, sizeof(frame));
}

void tearDown(void) {
}

/* Helper: produce a protected frame with the given counter value */
static void test_E2E_ProtectWithCounter(uint8 counter) {
    protectState.Counter = counter;
    frame[0] = 0xF0U;   /* upper nibble must be preserved */
    frame[1] = 0x00U;
    frame[2] = 0xAAU;
    frame[3] = 0x55U;
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Protect(&testConfig, &protectState, frame));
}

/** @req SWS_E2E_00001 */
void test_E2E_Init_NullPtr_ShouldReturnInputErrNull(void) {
    TEST_ASSERT_EQUAL_UINT8(E2E_E_INPUTERR_NULL, E2E_Init(NULL_PTR));
}

/** @req SWS_E2E_00001 */
void test_E2E_Init_ValidConfig_ShouldSucceed(void) {
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_Init(&e2eDummyConfig));
    /* clean up for the other tests */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_DeInit());
}

/** @req SWS_E2E_00001 */
void test_E2E_Init_DoubleInit_ShouldSucceed(void) {
    /* SUT has no double-init guard: both calls return E_OK */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_Init(&e2eDummyConfig));
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_Init(&e2eDummyConfig));
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_DeInit());
}

/** @req SWS_E2E_00002 (was: MainFunction lifecycle) */
void test_E2E_DeInit_AlwaysSucceedsAndAllowsReInit(void) {
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_DeInit());
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_Init(&e2eDummyConfig));
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_DeInit());
}

/** @req SWS_E2E_00003 (was: Protect_Uninit/NullPtr) */
void test_E2E_P01Protect_NullArgs_ShouldReturnNotOk(void) {
    uint8 data[4] = {0U, 0U, 0xAAU, 0x55U};
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P01Protect(NULL_PTR, &protectState, data));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P01Protect(&testConfig, NULL_PTR, data));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P01Protect(&testConfig, &protectState, NULL_PTR));
}

/** @req SWS_E2E_00003 (was: Protect_ValidData) */
void test_E2E_P01Protect_WritesCounterAndCrc(void) {
    test_E2E_ProtectWithCounter(5U);
    /* counter stored in low nibble of byte 0, upper nibble preserved */
    TEST_ASSERT_EQUAL_UINT8(0xF5U, frame[0]);
    /* CRC written at CRCOffset */
    TEST_ASSERT_FALSE(frame[1] == 0x00U);
    /* payload untouched */
    TEST_ASSERT_EQUAL_UINT8(0xAAU, frame[2]);
    TEST_ASSERT_EQUAL_UINT8(0x55U, frame[3]);
    /* counter incremented after protection */
    TEST_ASSERT_EQUAL_UINT8(6U, protectState.Counter);
}

/** @req SWS_E2E_00003 */
void test_E2E_P01Protect_CounterWrapsAtMax(void) {
    test_E2E_ProtectWithCounter(E2E_P01_COUNTER_MAX);   /* 14 */
    TEST_ASSERT_EQUAL_UINT8((0xF0U | E2E_P01_COUNTER_MAX), frame[0]);
    /* 14 + 1 wraps back to 0 */
    TEST_ASSERT_EQUAL_UINT8(0U, protectState.Counter);
}

/** @req SWS_E2E_00004 (was: Check_Uninit/NullPtr) */
void test_E2E_P01Check_NullArgs_ShouldReturnNotOk(void) {
    uint8 data[4] = {0U, 0U, 0xAAU, 0x55U};
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P01Check(NULL_PTR, &checkState, data));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P01Check(&testConfig, NULL_PTR, data));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, E2E_P01Check(&testConfig, &checkState, NULL_PTR));
}

/** @req SWS_E2E_00004 (was: Check_ValidData) */
void test_E2E_P01Check_RoundtripInitialThenOk(void) {
    /* First valid frame -> E2E_P_INITIAL */
    test_E2E_ProtectWithCounter(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, checkState.Status);
    TEST_ASSERT_EQUAL_UINT8(1U, checkState.WaitForFirstData);

    /* Consecutive frame (delta == 1) -> E2E_P_OK */
    test_E2E_ProtectWithCounter(1U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_OK, checkState.Status);
    TEST_ASSERT_EQUAL_UINT8(1U, checkState.LastValidCounter);
}

/** @req SWS_E2E_00004 */
void test_E2E_P01Check_RepeatedFrame_ShouldReportRepeated(void) {
    test_E2E_ProtectWithCounter(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, checkState.Status);

    /* Same frame again (delta == 0) -> E2E_P_REPEATED */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_REPEATED, checkState.Status);
}

/** @req SWS_E2E_00004 */
void test_E2E_P01Check_CorruptedCrc_ShouldReportWrongCrc(void) {
    test_E2E_ProtectWithCounter(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, checkState.Status);

    frame[1] ^= 0xFFU;  /* corrupt CRC byte */
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_WRONGCRC, checkState.Status);
    /* LastValidCounter unchanged by CRC failure */
    TEST_ASSERT_EQUAL_UINT8(0U, checkState.LastValidCounter);
}

/** @req SWS_E2E_00004 */
void test_E2E_P01Check_DecreasedCounter_ShouldReportWrongSequence(void) {
    test_E2E_ProtectWithCounter(1U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, checkState.Status);

    /* counter goes backwards (delta < 0) -> E2E_P_WRONGSEQUENCE */
    test_E2E_ProtectWithCounter(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_WRONGSEQUENCE, checkState.Status);
}

/** @req SWS_E2E_00004 */
void test_E2E_P01Check_MissingFramesWithinTolerance_ShouldReportOkSomeLost(void) {
    checkState.MaxDeltaCounterInit = 3U;

    test_E2E_ProtectWithCounter(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, checkState.Status);

    /* counter jumped by 2 (<= MaxDeltaCounterInit) -> E2E_P_OKSOMELOST */
    test_E2E_ProtectWithCounter(2U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_OKSOMELOST, checkState.Status);
    TEST_ASSERT_EQUAL_UINT8(2U, checkState.LastValidCounter);
}

/** @req SWS_E2E_00004 */
void test_E2E_P01Check_TooManyLostFrames_ShouldReportSync(void) {
    checkState.MaxDeltaCounterInit = 2U;

    test_E2E_ProtectWithCounter(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_INITIAL, checkState.Status);

    /* counter jumped by 5 (> MaxDeltaCounterInit) -> E2E_P_SYNC */
    test_E2E_ProtectWithCounter(5U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, E2E_P01Check(&testConfig, &checkState, frame));
    TEST_ASSERT_EQUAL_UINT32(E2E_P_SYNC, checkState.Status);
    TEST_ASSERT_EQUAL_UINT8(5U, checkState.LastValidCounter);
}

/** @req SWS_E2E_00005 (was: GetVersionInfo) */
void test_E2E_P01MapStatusToSM_Ok_ShouldMapToValid(void) {
    E2E_SMStateType smState = E2E_SM_INVALID;
    boolean error = TRUE;
    E2E_P01MapStatusToSM(E2E_P_OK, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_VALID, smState);
    TEST_ASSERT_FALSE(error);

    E2E_P01MapStatusToSM(E2E_P_OKSOMELOST, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_VALID, smState);
    TEST_ASSERT_FALSE(error);
}

/** @req SWS_E2E_00005 (was: GetVersionInfo) */
void test_E2E_P01MapStatusToSM_Errors_ShouldMapToInvalid(void) {
    E2E_SMStateType smState = E2E_SM_VALID;
    boolean error = FALSE;
    E2E_P01MapStatusToSM(E2E_P_WRONGCRC, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, smState);
    TEST_ASSERT_TRUE(error);

    E2E_P01MapStatusToSM(E2E_P_REPEATED, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, smState);
    TEST_ASSERT_TRUE(error);

    E2E_P01MapStatusToSM(E2E_P_WRONGSEQUENCE, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INVALID, smState);
    TEST_ASSERT_TRUE(error);
}

/** @req SWS_E2E_00005 (was: RegisterProfile) */
void test_E2E_P01MapStatusToSM_SyncAndInitial_ShouldMapToInitStates(void) {
    E2E_SMStateType smState = E2E_SM_VALID;
    boolean error = TRUE;

    E2E_P01MapStatusToSM(E2E_P_SYNC, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_INIT, smState);
    TEST_ASSERT_FALSE(error);

    E2E_P01MapStatusToSM(E2E_P_INITIAL, &smState, &error);
    TEST_ASSERT_EQUAL_UINT32(E2E_SM_NODATA, smState);
    TEST_ASSERT_FALSE(error);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_E2E_Init_NullPtr_ShouldReturnInputErrNull);
    RUN_TEST(test_E2E_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_E2E_Init_DoubleInit_ShouldSucceed);
    RUN_TEST(test_E2E_DeInit_AlwaysSucceedsAndAllowsReInit);
    RUN_TEST(test_E2E_P01Protect_NullArgs_ShouldReturnNotOk);
    RUN_TEST(test_E2E_P01Protect_WritesCounterAndCrc);
    RUN_TEST(test_E2E_P01Protect_CounterWrapsAtMax);
    RUN_TEST(test_E2E_P01Check_NullArgs_ShouldReturnNotOk);
    RUN_TEST(test_E2E_P01Check_RoundtripInitialThenOk);
    RUN_TEST(test_E2E_P01Check_RepeatedFrame_ShouldReportRepeated);
    RUN_TEST(test_E2E_P01Check_CorruptedCrc_ShouldReportWrongCrc);
    RUN_TEST(test_E2E_P01Check_DecreasedCounter_ShouldReportWrongSequence);
    RUN_TEST(test_E2E_P01Check_MissingFramesWithinTolerance_ShouldReportOkSomeLost);
    RUN_TEST(test_E2E_P01Check_TooManyLostFrames_ShouldReportSync);
    RUN_TEST(test_E2E_P01MapStatusToSM_Ok_ShouldMapToValid);
    RUN_TEST(test_E2E_P01MapStatusToSM_Errors_ShouldMapToInvalid);
    RUN_TEST(test_E2E_P01MapStatusToSM_SyncAndInitial_ShouldMapToInitStates);
    return UnityEnd();
}
