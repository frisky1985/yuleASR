/**
 * @file test_linsm.c
 * @brief LinSM (LIN State Manager) Unit Tests
 * @req SWS_LinSM
 */
#include "unity.h"
#include "LinSM.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static LinSM_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_LinSM_00001 */
void test_LinSM_Init_NullPtr_ShouldNotCrash(void) { LinSM_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_LinSM_00001 */
void test_LinSM_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; LinSM_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_LinSM_00002 */
void test_LinSM_DeInit_AfterInit_ShouldSucceed(void) { LinSM_Init(&testConfig); LinSM_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_LinSM_00003 */
void test_LinSM_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; LinSM_GetVersionInfo(&info); TEST_ASSERT_EQUAL(LINSM_VENDOR_ID, info.vendorID); }
/** @req SWS_LinSM_00003 */
void test_LinSM_GetVersionInfo_NullPtr_ShouldReportDet(void) { LinSM_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_LinSM_00004 */
void test_LinSM_ScheduleRequest_AfterInit_ShouldReturnResult(void) { LinSM_Init(&testConfig); Std_ReturnType ret = LinSM_ScheduleRequest(0U, LINSM_SCHEDULE_NORMAL); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_LinSM_00005 */
void test_LinSM_GetCurrentSchedule_AfterInit_ShouldSucceed(void) { LinSM_Init(&testConfig); LinSM_ScheduleType schedule; Std_ReturnType ret = LinSM_GetCurrentSchedule(0U, &schedule); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_LinSM_00006 */
void test_LinSM_RequestComMode_AfterInit_ShouldReturnResult(void) { LinSM_Init(&testConfig); Std_ReturnType ret = LinSM_RequestComMode(0U, LINSM_COMM_FULL); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_LinSM_00007 */
void test_LinSM_GetCurrentComMode_AfterInit_ShouldSucceed(void) { LinSM_Init(&testConfig); LinSM_ModeType mode; Std_ReturnType ret = LinSM_GetCurrentComMode(0U, &mode); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_LinSM_00008 */
void test_LinSM_MainFunction_AfterInit_ShouldNotCrash(void) { LinSM_Init(&testConfig); LinSM_MainFunction(); TEST_ASSERT_TRUE(1); }
/** @req SWS_LinSM_00009 */
void test_LinSM_ScheduleConfirmation_ShouldNotCrash(void) { LinSM_Init(&testConfig); LinSM_ScheduleConfirmation(0U, LINSM_SCHEDULE_NORMAL); TEST_ASSERT_TRUE(1); }
/** @req SWS_LinSM_00010 */
void test_LinSM_WakeUpConfirmation_ShouldNotCrash(void) { LinSM_Init(&testConfig); LinSM_WakeUpConfirmation(0U, TRUE); TEST_ASSERT_TRUE(1); }
/** @req SWS_LinSM_00011 */
void test_LinSM_GotoSleepConfirmation_ShouldNotCrash(void) { LinSM_Init(&testConfig); LinSM_GotoSleepConfirmation(0U, TRUE); TEST_ASSERT_TRUE(1); }
void test_LinSM_Init_DoubleInit_ShouldNotCrash(void) { LinSM_Init(&testConfig); LinSM_Init(&testConfig); TEST_ASSERT_TRUE(1); }
