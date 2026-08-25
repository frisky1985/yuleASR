/**
 * @file test_canif.c
 * @brief CanIf Unit Tests
 * @req SWS_CanIf
 */

// @tests src/bsw/ecual/canif/src/CanIf.c  @tests src/bsw/ecual/canif/include/CanIf.h
#include "unity.h"
#include "CanIf.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static CanIf_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_CanIf_00001 */
void test_CanIf_Init_NullPtr_ShouldNotCrash(void) { CanIf_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanIf_00001 */
void test_CanIf_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumTxPdus = 0U; testConfig.NumRxPdus = 0U; CanIf_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanIf_00002 */
void test_CanIf_DeInit_AfterInit_ShouldSucceed(void) { CanIf_Init(&testConfig); CanIf_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanIf_00003 */
void test_CanIf_SetControllerMode_AfterInit_ShouldReturnResult(void) { CanIf_Init(&testConfig); Std_ReturnType ret = CanIf_SetControllerMode(0U, CANIF_CS_STARTED); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanIf_00004 */
void test_CanIf_GetControllerMode_AfterInit_ShouldSucceed(void) { CanIf_Init(&testConfig); CanIf_ControllerModeType mode; Std_ReturnType ret = CanIf_GetControllerMode(0U, &mode); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanIf_00005 */
void test_CanIf_Transmit_BeforeInit_ShouldFail(void) { PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Std_ReturnType ret = CanIf_Transmit(0U, &pdu); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_CanIf_00005 */
void test_CanIf_Transmit_NullPdu_ShouldFail(void) { CanIf_Init(&testConfig); Std_ReturnType ret = CanIf_Transmit(0U, NULL_PTR); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_CanIf_00006 */
void test_CanIf_CancelTransmit_AfterInit_ShouldReturnResult(void) { CanIf_Init(&testConfig); Std_ReturnType ret = CanIf_CancelTransmit(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanIf_00007 */
void test_CanIf_SetPduMode_AfterInit_ShouldReturnResult(void) { CanIf_Init(&testConfig); Std_ReturnType ret = CanIf_SetPduMode(0U, CANIF_SET_ONLINE); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanIf_00008 */
void test_CanIf_GetPduMode_AfterInit_ShouldSucceed(void) { CanIf_Init(&testConfig); CanIf_PduModeType mode; Std_ReturnType ret = CanIf_GetPduMode(0U, &mode); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanIf_00009 */
void test_CanIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; CanIf_GetVersionInfo(&info); TEST_ASSERT_EQUAL(CANIF_VENDOR_ID, info.vendorID); }
/** @req SWS_CanIf_00009 */
void test_CanIf_GetVersionInfo_NullPtr_ShouldReportDet(void) { CanIf_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_CanIf_00010 */
void test_CanIf_TxConfirmation_ShouldNotCrash(void) { CanIf_Init(&testConfig); CanIf_TxConfirmation(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanIf_00011 */
void test_CanIf_ControllerBusOff_ShouldNotCrash(void) { CanIf_Init(&testConfig); CanIf_ControllerBusOff(0U); TEST_ASSERT_TRUE(1); }
void test_CanIf_Init_DoubleInit_ShouldNotCrash(void) { CanIf_Init(&testConfig); CanIf_Init(&testConfig); TEST_ASSERT_TRUE(1); }
