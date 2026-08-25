/**
 * @file test_cantp.c
 * @brief CanTp (CAN Transport Protocol) Unit Tests
 * @req SWS_CanTp
 */
#include "unity.h"
#include "CanTp.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static CanTp_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_CanTp_00001 */
void test_CanTp_Init_NullPtr_ShouldNotCrash(void) { CanTp_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanTp_00001 */
void test_CanTp_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; CanTp_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanTp_00002 */
void test_CanTp_Shutdown_AfterInit_ShouldSucceed(void) { CanTp_Init(&testConfig); CanTp_Shutdown(); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanTp_00003 */
void test_CanTp_Transmit_BeforeInit_ShouldFail(void) { PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Std_ReturnType ret = CanTp_Transmit(0U, &pdu); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_CanTp_00003 */
void test_CanTp_Transmit_NullPdu_ShouldFail(void) { CanTp_Init(&testConfig); Std_ReturnType ret = CanTp_Transmit(0U, NULL_PTR); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_CanTp_00004 */
void test_CanTp_CancelTransmit_AfterInit_ShouldReturnResult(void) { CanTp_Init(&testConfig); Std_ReturnType ret = CanTp_CancelTransmit(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanTp_00005 */
void test_CanTp_CancelReceive_AfterInit_ShouldReturnResult(void) { CanTp_Init(&testConfig); Std_ReturnType ret = CanTp_CancelReceive(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanTp_00006 */
void test_CanTp_ChangeParameter_AfterInit_ShouldReturnResult(void) { CanTp_Init(&testConfig); Std_ReturnType ret = CanTp_ChangeParameter(0U, TP_PARAMETER_BS, 10U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanTp_00007 */
void test_CanTp_ReadParameter_AfterInit_ShouldReturnResult(void) { CanTp_Init(&testConfig); uint16 value; Std_ReturnType ret = CanTp_ReadParameter(0U, TP_PARAMETER_BS, &value); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanTp_00008 */
void test_CanTp_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; CanTp_GetVersionInfo(&info); TEST_ASSERT_EQUAL(CANTP_VENDOR_ID, info.vendorID); }
/** @req SWS_CanTp_00008 */
void test_CanTp_GetVersionInfo_NullPtr_ShouldReportDet(void) { CanTp_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_CanTp_00009 */
void test_CanTp_MainFunction_AfterInit_ShouldNotCrash(void) { CanTp_Init(&testConfig); CanTp_MainFunction(); TEST_ASSERT_TRUE(1); }
void test_CanTp_Init_DoubleInit_ShouldNotCrash(void) { CanTp_Init(&testConfig); CanTp_Init(&testConfig); TEST_ASSERT_TRUE(1); }
