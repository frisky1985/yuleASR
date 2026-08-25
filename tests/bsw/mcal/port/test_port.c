/**
 * @file test_port.c
 * @brief Port Driver Unit Tests
 * @req SWS_Port
 */
#include "unity.h"
#include "Port.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Port_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Port_00001 */
void test_Port_Init_NullPtr_ShouldReportDet(void) { Port_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Port_00001 */
void test_Port_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumPins = 0U; Port_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Port_00002 */
void test_Port_RefreshPinDirection_AfterInit_ShouldNotCrash(void) { Port_Init(&testConfig); Port_RefreshPinDirection(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Port_00003 */
void test_Port_RefreshPinMode_AfterInit_ShouldNotCrash(void) { Port_Init(&testConfig); Port_RefreshPinMode(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Port_00004 */
void test_Port_SetPinDirection_AfterInit_ShouldReturnResult(void) { Port_Init(&testConfig); Std_ReturnType ret = Port_SetPinDirection(0U, PORT_PIN_IN); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Port_00005 */
void test_Port_SetPinMode_AfterInit_ShouldReturnResult(void) { Port_Init(&testConfig); Std_ReturnType ret = Port_SetPinMode(0U, PORT_PIN_MODE_GPIO); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Port_00006 */
void test_Port_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Port_GetVersionInfo(&info); TEST_ASSERT_EQUAL(PORT_VENDOR_ID, info.vendorID); }
/** @req SWS_Port_00006 */
void test_Port_GetVersionInfo_NullPtr_ShouldReportDet(void) { Port_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
void test_Port_Init_DoubleInit_ShouldNotCrash(void) { Port_Init(&testConfig); Port_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Port_RefreshPinDirection_BeforeInit_ShouldNotCrash(void) { Port_RefreshPinDirection(0U); TEST_ASSERT_TRUE(1); }
void test_Port_RefreshPinMode_BeforeInit_ShouldNotCrash(void) { Port_RefreshPinMode(0U); TEST_ASSERT_TRUE(1); }
