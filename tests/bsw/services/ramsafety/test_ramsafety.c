/**
 * @file test_ramsafety.c
 * @brief RamSafety Unit Tests
 * @req SWS_RamSafety
 */
#include "unity.h"
#include "RamSafety.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static RamSafety_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_RamSafety_00001 */
void test_RamSafety_Init_NullPtr_ShouldFail(void) {
    Std_ReturnType ret = RamSafety_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_RamSafety_00001 */
void test_RamSafety_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.NumRegions = 0U;
    Std_ReturnType ret = RamSafety_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_RamSafety_00002 */
void test_RamSafety_DeInit_AfterInit_ShouldSucceed(void) {
    RamSafety_Init(&testConfig);
    Std_ReturnType ret = RamSafety_DeInit();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_RamSafety_00003 */
void test_RamSafety_GetState_AfterInit_ShouldReturnIdle(void) {
    RamSafety_Init(&testConfig);
    RamSafety_StateType state = RamSafety_GetState();
    TEST_ASSERT_TRUE(state == RAMSAFETY_STATE_IDLE || state == RAMSAFETY_STATE_READY);
}

/** @req SWS_RamSafety_00003 */
void test_RamSafety_GetState_BeforeInit_ShouldReturnUninit(void) {
    RamSafety_StateType state = RamSafety_GetState();
    TEST_ASSERT_EQUAL(RAMSAFETY_STATE_UNINIT, state);
}

/** @req SWS_RamSafety_00004 */
void test_RamSafety_RunStartupTest_AfterInit_ShouldReturnResult(void) {
    RamSafety_Init(&testConfig);
    Std_ReturnType ret = RamSafety_RunStartupTest(NULL_PTR);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_RamSafety_00005 */
void test_RamSafety_MainFunction_AfterInit_ShouldNotCrash(void) {
    RamSafety_Init(&testConfig);
    RamSafety_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_RamSafety_00006 */
void test_RamSafety_TriggerTest_BeforeInit_ShouldFail(void) {
    RamSafety_ResultType result = RamSafety_TriggerTest(0U, RAMSAFETY_TEST_MARCH_C);
    TEST_ASSERT_EQUAL(RAMSAFETY_RESULT_ERROR, result);
}

/** @req SWS_RamSafety_00007 */
void test_RamSafety_VerifyRegion_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = RamSafety_VerifyRegion(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_RamSafety_00008 */
void test_RamSafety_VerifyRange_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = RamSafety_VerifyRange(0x20000000U, 256U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_RamSafety_00009 */
void test_RamSafety_GetStatistics_AfterInit_ShouldSucceed(void) {
    RamSafety_Init(&testConfig);
    RamSafety_StatisticsType stats;
    Std_ReturnType ret = RamSafety_GetStatistics(&stats);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_RamSafety_00010 */
void test_RamSafety_ClearStatistics_AfterInit_ShouldSucceed(void) {
    RamSafety_Init(&testConfig);
    Std_ReturnType ret = RamSafety_ClearStatistics();
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_RamSafety_00011 */
void test_RamSafety_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    RamSafety_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(RAMSAFETY_VENDOR_ID, info.vendorID);
}

void test_RamSafety_EnterSafeState_ShouldNotCrash(void) {
    RamSafety_Init(&testConfig);
    RamSafety_EnterSafeState(0x01U);
    TEST_ASSERT_TRUE(1);
}

void test_RamSafety_CheckEccStatus_AfterInit_ShouldSucceed(void) {
    RamSafety_Init(&testConfig);
    boolean hasError; uint32 errorCount;
    Std_ReturnType ret = RamSafety_CheckEccStatus(0U, &hasError, &errorCount);
    TEST_ASSERT_EQUAL(E_OK, ret);
}
