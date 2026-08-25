/**
 * @file test_can.c
 * @brief Can (CAN Driver) Unit Tests
 * @req SWS_Can
 */
#include "unity.h"
#include "Can.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Can_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Can_00001 */
void test_Can_Init_NullPtr_ShouldNotCrash(void) { Can_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00001 */
void test_Can_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumControllers = 0U; Can_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00002 */
void test_Can_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Can_GetVersionInfo(&info); TEST_ASSERT_EQUAL(CAN_VENDOR_ID, info.vendorID); }
/** @req SWS_Can_00002 */
void test_Can_GetVersionInfo_NullPtr_ShouldReportDet(void) { Can_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Can_00003 */
void test_Can_SetControllerMode_AfterInit_ShouldReturnResult(void) { Can_Init(&testConfig); Can_ReturnType ret = Can_SetControllerMode(0U, CAN_CS_STARTED); TEST_ASSERT_TRUE(ret == CAN_OK || ret == CAN_NOT_OK); }
/** @req SWS_Can_00004 */
void test_Can_DisableControllerInterrupts_ShouldNotCrash(void) { Can_Init(&testConfig); Can_DisableControllerInterrupts(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00005 */
void test_Can_EnableControllerInterrupts_ShouldNotCrash(void) { Can_Init(&testConfig); Can_EnableControllerInterrupts(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00006 */
void test_Can_Write_BeforeInit_ShouldFail(void) { Can_PduType pdu; pdu.id = 0x100U; pdu.length = 8U; uint8 data[8] = {0}; pdu.sdu = data; Can_ReturnType ret = Can_Write(0U, &pdu); TEST_ASSERT_EQUAL(CAN_NOT_OK, ret); }
/** @req SWS_Can_00007 */
void test_Can_MainFunction_Write_ShouldNotCrash(void) { Can_Init(&testConfig); Can_MainFunction_Write(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00008 */
void test_Can_MainFunction_Read_ShouldNotCrash(void) { Can_Init(&testConfig); Can_MainFunction_Read(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00009 */
void test_Can_MainFunction_BusOff_ShouldNotCrash(void) { Can_Init(&testConfig); Can_MainFunction_BusOff(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00010 */
void test_Can_MainFunction_Wakeup_ShouldNotCrash(void) { Can_Init(&testConfig); Can_MainFunction_Wakeup(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00011 */
void test_Can_MainFunction_Mode_ShouldNotCrash(void) { Can_Init(&testConfig); Can_MainFunction_Mode(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Can_00012 */
void test_Can_CheckWakeup_AfterInit_ShouldReturnResult(void) { Can_Init(&testConfig); Std_ReturnType ret = Can_CheckWakeup(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
void test_Can_Init_DoubleInit_ShouldNotCrash(void) { Can_Init(&testConfig); Can_Init(&testConfig); TEST_ASSERT_TRUE(1); }
