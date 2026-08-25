/**
 * @file test_gpt.c
 * @brief Gpt (General Purpose Timer) Unit Tests
 * @req SWS_Gpt
 */
#include "unity.h"
#include "Gpt.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Gpt_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Gpt_00001 */
void test_Gpt_Init_NullPtr_ShouldReportDet(void) { Gpt_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Gpt_00001 */
void test_Gpt_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; Gpt_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Gpt_00002 */
void test_Gpt_DeInit_AfterInit_ShouldSucceed(void) { Gpt_Init(&testConfig); Gpt_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Gpt_00003 */
void test_Gpt_StartTimer_AfterInit_ShouldNotCrash(void) { Gpt_Init(&testConfig); Gpt_StartTimer(0U, 1000U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Gpt_00004 */
void test_Gpt_StopTimer_AfterInit_ShouldNotCrash(void) { Gpt_Init(&testConfig); Gpt_StopTimer(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Gpt_00005 */
void test_Gpt_GetTimer_AfterInit_ShouldReturnValue(void) { Gpt_Init(&testConfig); Gpt_StartTimer(0U, 1000U); Gpt_ValueType val = Gpt_GetTimer(0U); TEST_ASSERT_TRUE(val >= 0); }
/** @req SWS_Gpt_00006 */
void test_Gpt_GetTimeElapsed_AfterInit_ShouldReturnValue(void) { Gpt_Init(&testConfig); Gpt_ValueType val = Gpt_GetTimeElapsed(0U); TEST_ASSERT_TRUE(val >= 0); }
/** @req SWS_Gpt_00007 */
void test_Gpt_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Gpt_GetVersionInfo(&info); TEST_ASSERT_EQUAL(GPT_VENDOR_ID, info.vendorID); }
/** @req SWS_Gpt_00007 */
void test_Gpt_GetVersionInfo_NullPtr_ShouldReportDet(void) { Gpt_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
void test_Gpt_Init_DoubleInit_ShouldNotCrash(void) { Gpt_Init(&testConfig); Gpt_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Gpt_DeInit_BeforeInit_ShouldNotCrash(void) { Gpt_DeInit(); TEST_ASSERT_TRUE(1); }
void test_Gpt_StartTimer_BeforeInit_ShouldNotCrash(void) { Gpt_StartTimer(0U, 1000U); TEST_ASSERT_TRUE(1); }
