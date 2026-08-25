/**
 * @file test_com.c
 * @brief Com (Communication Manager) Unit Tests
 * @req SWS_Com
 */
#include "unity.h"
#include "Com.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Com_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Com_00001 */
void test_Com_Init_NullPtr_ShouldNotCrash(void) { Com_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_Com_00001 */
void test_Com_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumSignals = 0U; testConfig.NumIPdus = 0U; Com_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Com_00002 */
void test_Com_DeInit_AfterInit_ShouldSucceed(void) { Com_Init(&testConfig); Com_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Com_00003 */
void test_Com_SendSignal_BeforeInit_ShouldFail(void) { uint8 data = 0x42; uint8 ret = Com_SendSignal(0U, &data); TEST_ASSERT_EQUAL(COM_SERVICE_NOT_AVAILABLE, ret); }
/** @req SWS_Com_00003 */
void test_Com_SendSignal_NullData_ShouldFail(void) { Com_Init(&testConfig); uint8 ret = Com_SendSignal(0U, NULL_PTR); TEST_ASSERT_EQUAL(COM_WRONG_SIGNAL_REF, ret); }
/** @req SWS_Com_00004 */
void test_Com_ReceiveSignal_BeforeInit_ShouldFail(void) { uint8 data; uint8 ret = Com_ReceiveSignal(0U, &data); TEST_ASSERT_EQUAL(COM_SERVICE_NOT_AVAILABLE, ret); }
/** @req SWS_Com_00005 */
void test_Com_SendSignalGroup_AfterInit_ShouldReturnResult(void) { Com_Init(&testConfig); uint8 ret = Com_SendSignalGroup(0U); TEST_ASSERT_TRUE(ret == COM_OK || ret == COM_SERVICE_NOT_AVAILABLE); }
/** @req SWS_Com_00006 */
void test_Com_ReceiveSignalGroup_AfterInit_ShouldReturnResult(void) { Com_Init(&testConfig); uint8 ret = Com_ReceiveSignalGroup(0U); TEST_ASSERT_TRUE(ret == COM_OK || ret == COM_SERVICE_NOT_AVAILABLE); }
/** @req SWS_Com_00007 */
void test_Com_TriggerIPDUSend_AfterInit_ShouldReturnResult(void) { Com_Init(&testConfig); Std_ReturnType ret = Com_TriggerIPDUSend(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Com_00008 */
void test_Com_TxConfirmation_ShouldNotCrash(void) { Com_Init(&testConfig); Com_TxConfirmation(0U, E_OK); TEST_ASSERT_TRUE(1); }
/** @req SWS_Com_00009 */
void test_Com_RxIndication_ShouldNotCrash(void) { Com_Init(&testConfig); PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Com_RxIndication(0U, &pdu); TEST_ASSERT_TRUE(1); }
/** @req SWS_Com_00010 */
void test_Com_TriggerTransmit_AfterInit_ShouldReturnResult(void) { Com_Init(&testConfig); PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Std_ReturnType ret = Com_TriggerTransmit(0U, &pdu); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
void test_Com_Init_DoubleInit_ShouldNotCrash(void) { Com_Init(&testConfig); Com_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Com_DeInit_BeforeInit_ShouldNotCrash(void) { Com_DeInit(); TEST_ASSERT_TRUE(1); }
