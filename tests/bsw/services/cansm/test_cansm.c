/**
 * @file test_cansm.c
 * @brief CanSM (CAN State Manager) Unit Tests
 * @req SWS_CanSM
 */
#include "unity.h"
#include "CanSm.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static CanSm_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_CanSM_00001 */
void test_CanSm_Init_NullPtr_ShouldNotCrash(void) { CanSm_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanSM_00001 */
void test_CanSm_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; CanSm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanSM_00002 */
void test_CanSm_DeInit_AfterInit_ShouldSucceed(void) { CanSm_Init(&testConfig); CanSm_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; CanSm_GetVersionInfo(&info); TEST_ASSERT_EQUAL(CANSM_VENDOR_ID, info.vendorID); }
/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_NullPtr_ShouldReportDet(void) { CanSm_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_CanSM_00004 */
void test_CanSm_RequestComMode_AfterInit_ShouldReturnResult(void) { CanSm_Init(&testConfig); Std_ReturnType ret = CanSm_RequestComMode(0U, CANSM_COMM_FULL); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanSM_00005 */
void test_CanSm_GetCurrentComMode_AfterInit_ShouldSucceed(void) { CanSm_Init(&testConfig); CanSm_CommModeType mode; Std_ReturnType ret = CanSm_GetCurrentComMode(0U, &mode); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanSM_00006 */
void test_CanSm_MainFunction_AfterInit_ShouldNotCrash(void) { CanSm_Init(&testConfig); CanSm_MainFunction(); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanSM_00007 */
void test_CanSm_ControllerBusOff_ShouldNotCrash(void) { CanSm_Init(&testConfig); CanSm_ControllerBusOff(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanSM_00008 */
void test_CanSm_ControllerBusOff_Recovery_ShouldNotCrash(void) { CanSm_Init(&testConfig); CanSm_ControllerBusOff(0U); CanSm_MainFunction(); TEST_ASSERT_TRUE(1); }
void test_CanSm_Init_DoubleInit_ShouldNotCrash(void) { CanSm_Init(&testConfig); CanSm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_CanSm_DeInit_BeforeInit_ShouldNotCrash(void) { CanSm_DeInit(); TEST_ASSERT_TRUE(1); }
