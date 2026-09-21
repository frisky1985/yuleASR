/**
 * @file test_ethtsyn.c
 * @brief EthTSyn Unit Tests — substantiated against the production EthTSyn
 *        service stub (src/bsw/services/ethtsyn/src/EthTSyn.c).
 * @version 2.0.0
 * @date 2026-09-17
 */

// @tests src/bsw/services/ethtsyn/src/EthTSyn.c  @tests src/bsw/services/ethtsyn/include/EthTSyn.h

#include "unity.h"

/* The vendored Unity lacks 64-bit asserts; provide a compatible shim. */
#define TEST_ASSERT_EQUAL_UINT64(expected, actual) \
    TEST_ASSERT_TRUE((expected) == (actual))
#include "EthTSyn.h"
#include "Det.h"
#include <string.h>

/* ---------------------------------------------------------------------------
 * Det mock
 * ------------------------------------------------------------------------- */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint32 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    TEST_ASSERT_EQUAL_UINT16(ETHTSYN_MODULE_ID, ModuleId);
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */
static EthTSyn_ConfigType test_config;
static boolean ethtsyn_initialized = FALSE;

static void test_init_default(void) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.domainNumber = 0U;
    test_config.masterOnly = FALSE;
    test_config.logSyncInterval = 3U;
    test_config.logAnnounceInterval = 3U;
    test_config.logPdelayReqInterval = 3U;
    test_config.priority1 = 128U;
    test_config.priority2 = 128U;
    test_config.clockClass = 248U;
    test_config.clockAccuracy = 0xFEU;
    test_config.offsetScaledLogVariance = 0x4100U;
    mock_Det_Reset();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_Init(&test_config));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    ethtsyn_initialized = TRUE;
}

static void test_deinit(void) {
    if (ethtsyn_initialized) {
        EthTSyn_DeInit();
        ethtsyn_initialized = FALSE;
    }
}

void setUp(void) {
    test_deinit();
    mock_Det_Reset();
}

void tearDown(void) {
    test_deinit();
}

/* ---------------------------------------------------------------------------
 * EthTSyn_Init / EthTSyn_DeInit
 * ------------------------------------------------------------------------- */

/** @req SWS_EthTSyn_00001 */
void test_EthTSyn_Init_NullConfig_ShouldReportDetAndFail(void) {
    Std_ReturnType ret = EthTSyn_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00001 */
void test_EthTSyn_Init_ValidConfig_ShouldSucceedAndZeroClock(void) {
    EthTSyn_TimestampType ts = { 0xFFFFU, 0xFFFFFFFFU };
    test_init_default();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_GetTime(&ts));
    TEST_ASSERT_EQUAL_UINT64(0U, ts.seconds);
    TEST_ASSERT_EQUAL_UINT32(0U, ts.nanoseconds);
}

/** @req SWS_EthTSyn_00001 */
void test_EthTSyn_Init_DoubleInit_ShouldReportDetAndFail(void) {
    test_init_default();
    mock_Det_Reset();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_Init(&test_config));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00002 */
void test_EthTSyn_DeInit_AfterInit_ShouldDeinitializeModule(void) {
    EthTSyn_TimestampType ts = { 0U, 0U };
    test_init_default();
    EthTSyn_DeInit();
    ethtsyn_initialized = FALSE;
    mock_Det_Reset();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_GetTime(&ts));
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/* ---------------------------------------------------------------------------
 * EthTSyn_MainFunction
 * ------------------------------------------------------------------------- */

/** @req SWS_EthTSyn_00003 */
void test_EthTSyn_MainFunction_Uninitialized_ShouldReturnSilently(void) {
    EthTSyn_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_EthTSyn_00003 */
void test_EthTSyn_MainFunction_Initialized_ShouldRunWithoutError(void) {
    test_init_default();
    EthTSyn_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * EthTSyn_GetTime / EthTSyn_SetTime
 * ------------------------------------------------------------------------- */

/** @req SWS_EthTSyn_00004 */
void test_EthTSyn_GetTime_Uninitialized_ShouldReportDetAndFail(void) {
    EthTSyn_TimestampType ts = { 0U, 0U };
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_GetTime(&ts));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00004 */
void test_EthTSyn_GetTime_NullPointer_ShouldReportDetAndFail(void) {
    test_init_default();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_GetTime(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00004 @req SWS_EthTSyn_00005 */
void test_EthTSyn_SetTimeGetTime_ShouldRoundTrip(void) {
    EthTSyn_TimestampType write_ts = { 123456789ULL, 987654321U };
    EthTSyn_TimestampType read_ts = { 0U, 0U };
    test_init_default();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_SetTime(&write_ts));
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_GetTime(&read_ts));
    TEST_ASSERT_EQUAL_UINT64(write_ts.seconds, read_ts.seconds);
    TEST_ASSERT_EQUAL_UINT32(write_ts.nanoseconds, read_ts.nanoseconds);
}

/** @req SWS_EthTSyn_00005 */
void test_EthTSyn_SetTime_Uninitialized_ShouldReportDetAndFail(void) {
    EthTSyn_TimestampType ts = { 1U, 1U };
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_SetTime(&ts));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(2U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00005 */
void test_EthTSyn_SetTime_NullPointer_ShouldReportDetAndFail(void) {
    test_init_default();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_SetTime(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(2U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00001 @req SWS_EthTSyn_00005 */
void test_EthTSyn_ReInit_ShouldResetClockToZero(void) {
    EthTSyn_TimestampType ts = { 42U, 42U };
    EthTSyn_TimestampType read_ts = { 0U, 0U };
    test_init_default();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_SetTime(&ts));
    test_deinit();
    test_init_default();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_GetTime(&read_ts));
    TEST_ASSERT_EQUAL_UINT64(0U, read_ts.seconds);
    TEST_ASSERT_EQUAL_UINT32(0U, read_ts.nanoseconds);
}

/* ---------------------------------------------------------------------------
 * EthTSyn_AdjustRate
 * ------------------------------------------------------------------------- */

/** @req SWS_EthTSyn_00006 */
void test_EthTSyn_AdjustRate_Uninitialized_ShouldReportDetAndFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_AdjustRate(1, 1));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(3U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00006 */
void test_EthTSyn_AdjustRate_ZeroDenominator_ShouldReportDetAndFail(void) {
    test_init_default();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_AdjustRate(1, 0));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(3U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00006 */
void test_EthTSyn_AdjustRate_ValidArgs_ShouldSucceed(void) {
    test_init_default();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_AdjustRate(1, 1000));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * EthTSyn_GetPortState
 * ------------------------------------------------------------------------- */

/** @req SWS_EthTSyn_00007 */
void test_EthTSyn_GetPortState_Uninitialized_ShouldReportDetAndFail(void) {
    EthTSyn_PortStateType state = ETHTSYN_PORT_INIT;
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_GetPortState(0U, &state));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(4U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00007 */
void test_EthTSyn_GetPortState_NullPointer_ShouldReportDetAndFail(void) {
    test_init_default();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_GetPortState(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(4U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EthTSyn_00007 */
void test_EthTSyn_GetPortState_ValidCall_ShouldReturnListening(void) {
    EthTSyn_PortStateType state = ETHTSYN_PORT_INIT;
    test_init_default();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_GetPortState(0U, &state));
    TEST_ASSERT_EQUAL(ETHTSYN_PORT_LISTENING, state);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * EthTSyn_GetClockIdentity — NOTE: production code performs no init check.
 * ------------------------------------------------------------------------- */

/** @req SWS_EthTSyn_00008 */
void test_EthTSyn_GetClockIdentity_NullPointer_ShouldReportDetAndFail(void) {
    EthTSyn_ClockIdentityType identity = { { 0U } };
    test_init_default();
    TEST_ASSERT_EQUAL(E_NOT_OK, EthTSyn_GetClockIdentity(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(5U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
    (void)identity;
}

/** @req SWS_EthTSyn_00008 */
void test_EthTSyn_GetClockIdentity_UninitializedValidPointer_ShouldSucceed(void) {
    /* Production GetClockIdentity has no init guard — documented quirk. */
    EthTSyn_ClockIdentityType identity = { { 0U } };
    mock_Det_Reset();
    TEST_ASSERT_EQUAL(E_OK, EthTSyn_GetClockIdentity(&identity));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * Runner
 * ------------------------------------------------------------------------- */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_EthTSyn_Init_NullConfig_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_Init_ValidConfig_ShouldSucceedAndZeroClock);
    RUN_TEST(test_EthTSyn_Init_DoubleInit_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_DeInit_AfterInit_ShouldDeinitializeModule);
    RUN_TEST(test_EthTSyn_MainFunction_Uninitialized_ShouldReturnSilently);
    RUN_TEST(test_EthTSyn_MainFunction_Initialized_ShouldRunWithoutError);
    RUN_TEST(test_EthTSyn_GetTime_Uninitialized_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_GetTime_NullPointer_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_SetTimeGetTime_ShouldRoundTrip);
    RUN_TEST(test_EthTSyn_SetTime_Uninitialized_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_SetTime_NullPointer_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_ReInit_ShouldResetClockToZero);
    RUN_TEST(test_EthTSyn_AdjustRate_Uninitialized_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_AdjustRate_ZeroDenominator_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_AdjustRate_ValidArgs_ShouldSucceed);
    RUN_TEST(test_EthTSyn_GetPortState_Uninitialized_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_GetPortState_NullPointer_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_GetPortState_ValidCall_ShouldReturnListening);
    RUN_TEST(test_EthTSyn_GetClockIdentity_NullPointer_ShouldReportDetAndFail);
    RUN_TEST(test_EthTSyn_GetClockIdentity_UninitializedValidPointer_ShouldSucceed);

    return UNITY_END();
}
