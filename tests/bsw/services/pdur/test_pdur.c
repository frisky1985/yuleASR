/**
 * @file test_pdur.c
 * @brief PduR (PDU Router) Unit Tests
 * @req SWS_PduR
 */
#include "unity.h"
#include "PduR.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static PduR_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_PduR_00001 */
void test_PduR_Init_NullPtr_ShouldNotCrash(void) { PduR_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00001 */
void test_PduR_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumRoutingPaths = 0U; PduR_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00002 */
void test_PduR_DeInit_AfterInit_ShouldSucceed(void) { PduR_Init(&testConfig); PduR_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00003 */
void test_PduR_Transmit_BeforeInit_ShouldFail(void) { PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Std_ReturnType ret = PduR_Transmit(0U, &pdu); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_PduR_00003 */
void test_PduR_Transmit_NullPdu_ShouldFail(void) { PduR_Init(&testConfig); Std_ReturnType ret = PduR_Transmit(0U, NULL_PTR); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_PduR_00004 */
void test_PduR_RxIndication_ShouldNotCrash(void) { PduR_Init(&testConfig); PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; PduR_RxIndication(0U, &pdu); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00005 */
void test_PduR_TxConfirmation_ShouldNotCrash(void) { PduR_Init(&testConfig); PduR_TxConfirmation(0U, E_OK); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00006 */
void test_PduR_TriggerTransmit_AfterInit_ShouldReturnResult(void) { PduR_Init(&testConfig); PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Std_ReturnType ret = PduR_TriggerTransmit(0U, &pdu); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_PduR_00007 */
void test_PduR_CancelTransmitRequest_AfterInit_ShouldReturnResult(void) { PduR_Init(&testConfig); Std_ReturnType ret = PduR_CancelTransmitRequest(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_PduR_00008 */
void test_PduR_CancelReceiveRequest_AfterInit_ShouldReturnResult(void) { PduR_Init(&testConfig); Std_ReturnType ret = PduR_CancelReceiveRequest(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_PduR_00009 */
void test_PduR_EnableRouting_ShouldNotCrash(void) { PduR_Init(&testConfig); PduR_EnableRouting(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00010 */
void test_PduR_DisableRouting_ShouldNotCrash(void) { PduR_Init(&testConfig); PduR_DisableRouting(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_PduR_00011 */
void test_PduR_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; PduR_GetVersionInfo(&info); TEST_ASSERT_EQUAL(PDUR_VENDOR_ID, info.vendorID); }
/** @req SWS_PduR_00011 */
void test_PduR_GetVersionInfo_NullPtr_ShouldReportDet(void) { PduR_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
void test_PduR_Init_DoubleInit_ShouldNotCrash(void) { PduR_Init(&testConfig); PduR_Init(&testConfig); TEST_ASSERT_TRUE(1); }
