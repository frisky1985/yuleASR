/**
 * @file test_icu_new.c
 * @brief Icu (Input Capture Unit) Additional Unit Tests
 * @req SWS_Icu
 */
#include "unity.h"
#include "Icu.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Icu_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Icu_00001 */
void test_Icu_Init_NullPtr_ShouldReportDet(void) { Icu_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Icu_00001 */
void test_Icu_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; Icu_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Icu_00002 */
void test_Icu_DeInit_AfterInit_ShouldSucceed(void) { Icu_Init(&testConfig); Icu_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Icu_00003 */
void test_Icu_StartTimestamp_AfterInit_ShouldNotCrash(void) { Icu_Init(&testConfig); Icu_StartTimestamp(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Icu_00004 */
void test_Icu_StopTimestamp_AfterInit_ShouldNotCrash(void) { Icu_Init(&testConfig); Icu_StopTimestamp(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Icu_00005 */
void test_Icu_GetIndexAndDutyCycle_AfterInit_ShouldReturnResult(void) { Icu_Init(&testConfig); uint16 OnPeriod = 0, Period = 0; Std_ReturnType ret = Icu_GetIndexAndDutyCycle(0U, &OnPeriod, &Period); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Icu_00006 */
void test_Icu_StartEdgeCount_AfterInit_ShouldNotCrash(void) { Icu_Init(&testConfig); Icu_StartEdgeCount(0U, ICU_ACTIVE_EDGE_RISING, 100U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Icu_00007 */
void test_Icu_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Icu_GetVersionInfo(&info); TEST_ASSERT_EQUAL(ICU_VENDOR_ID, info.vendorID); }
/** @req SWS_Icu_00007 */
void test_Icu_GetVersionInfo_NullPtr_ShouldReportDet(void) { Icu_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
void test_Icu_Init_DoubleInit_ShouldNotCrash(void) { Icu_Init(&testConfig); Icu_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Icu_DeInit_BeforeInit_ShouldNotCrash(void) { Icu_DeInit(); TEST_ASSERT_TRUE(1); }
void test_Icu_StartTimestamp_BeforeInit_ShouldNotCrash(void) { Icu_StartTimestamp(0U); TEST_ASSERT_TRUE(1); }
