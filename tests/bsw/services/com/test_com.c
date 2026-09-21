/**
 * @file test_com.c
 * @brief Com (Communication Manager) Unit Tests
 * @req SWS_Com
 */

// @tests src/bsw/services/com/src/Com.c  @tests src/bsw/services/com/include/Com.h
#include "unity.h"
#include "Com.h"

/* -------------------------------------------------------------------------- */
/* DET mock: records call count plus ApiId/ErrorId of the last reported error */
/* -------------------------------------------------------------------------- */
static uint8 mock_DetCalls = 0;
static uint8 mock_DetLastApiId = 0;
static uint8 mock_DetLastErrorId = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId; (void)InstanceId;
    mock_DetCalls++;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    return E_OK;
}

/* -------------------------------------------------------------------------- */
/* PduR_Transmit recorder (implemented in stubs.c)                          */
/* -------------------------------------------------------------------------- */
extern uint32 mock_PduR_Transmit_Count;
extern PduIdType mock_PduR_Transmit_LastPduId;
extern uint16 mock_PduR_Transmit_LastSduLength;

/* -------------------------------------------------------------------------- */
/* Test configurations                                                        */
/* -------------------------------------------------------------------------- */
/* One 8-bit little-endian signal at bit 0, mapped to IPDU 0, COM_PENDING
 * transfer property (no automatic transmission on Com_SendSignal). */
static const Com_SignalConfigType testSignals_Pending[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
/* Same signal with COM_TRIGGERED transfer property: Com_SendSignal must
 * trigger PduR_Transmit for the referenced IPDU. */
static const Com_SignalConfigType testSignals_Triggered[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType testIPdus[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U }
};
static const Com_ConfigType fullConfig =
    { testSignals_Pending, 1U, testIPdus, 1U };
static const Com_ConfigType triggeredConfig =
    { testSignals_Triggered, 1U, testIPdus, 1U };
/* Empty configuration: module init succeeds but no signal/IPDU is resolvable. */
static const Com_ConfigType emptyConfig = { NULL_PTR, 0U, NULL_PTR, 0U };

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */
static void mock_reset(void) {
    mock_DetCalls = 0;
    mock_DetLastApiId = 0;
    mock_DetLastErrorId = 0;
    mock_PduR_Transmit_Count = 0U;
    mock_PduR_Transmit_LastPduId = 0;
    mock_PduR_Transmit_LastSduLength = 0;
}

/* Bring the static SUT state deterministically to COM_UNINIT. Com_Init()
 * re-initializes unconditionally, so Init+DeInit is always a safe reset. */
static void ensure_uninit(void) {
    Com_Init(&emptyConfig);
    Com_DeInit();
    mock_reset();
}

void setUp(void) { ensure_uninit(); }
void tearDown(void) {}

/** @req SWS_Com_00001 */
void test_Com_Init_NullPtr_ShouldNotCrash(void) {
    Com_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Module must still be uninitialized: DeInit on UNINIT reports E_UNINIT. */
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(2U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00001 */
void test_Com_Init_ValidConfig_ShouldSucceed(void) {
    Com_Init(&emptyConfig);
    /* Init is silent; a successful DeInit afterwards (no E_UNINIT DET)
     * proves the module reached COM_STATE_INIT. */
    mock_reset();
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00002 */
void test_Com_DeInit_AfterInit_ShouldSucceed(void) {
    Com_Init(&emptyConfig);
    mock_reset();
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Probe: the module is really UNINIT now. */
    uint8 data = 0x42;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, Com_SendSignal(0U, &data));
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_SENDSIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00003 */
void test_Com_SendSignal_BeforeInit_ShouldFail(void) {
    uint8 data = 0x42;
    uint8 ret = Com_SendSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_SENDSIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00003 */
void test_Com_SendSignal_NullData_ShouldFail(void) {
    Com_Init(&emptyConfig);
    uint8 ret = Com_SendSignal(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_SENDSIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Com_00004 */
void test_Com_ReceiveSignal_BeforeInit_ShouldFail(void) {
    uint8 data = 0;
    uint8 ret = Com_ReceiveSignal(0U, &data);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_RECEIVESIGNAL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Com_00005 */
void test_Com_SendSignalGroup_AfterInit_ShouldReturnResult(void) {
    Com_Init(&emptyConfig);
    uint8 ret = Com_SendSignalGroup(0U);
    /* No IPDU configured -> Com_TransmitIPdu() cannot resolve the PduId. */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00006 */
void test_Com_ReceiveSignalGroup_AfterInit_ShouldReturnResult(void) {
    Com_Init(&emptyConfig);
    uint8 ret = Com_ReceiveSignalGroup(0U);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00007 */
void test_Com_TriggerIPDUSend_AfterInit_ShouldReturnResult(void) {
    Com_Init(&emptyConfig);
    Std_ReturnType ret = Com_TriggerIPDUSend(0U);
    /* Unknown PduId (empty config) -> transmission impossible. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00008 */
void test_Com_TxConfirmation_ShouldNotCrash(void) {
    Com_Init(&triggeredConfig);
    /* COM_TRIGGERED signal: Com_SendSignal must trigger PduR_Transmit. */
    uint8 data = 0x42;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_SendSignal(0U, &data));
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PduR_Transmit_Count);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_PduR_Transmit_LastPduId);
    TEST_ASSERT_EQUAL_UINT16(8U, mock_PduR_Transmit_LastSduLength);
    /* TxConfirmation for a valid, initialized PduId is silent. */
    mock_reset();
    Com_TxConfirmation(0U, E_OK);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

/** @req SWS_Com_00009 */
void test_Com_RxIndication_ShouldNotCrash(void) {
    Com_Init(&fullConfig);
    uint8 data[8] = { 0x99, 0, 0, 0, 0, 0, 0, 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = data;
    pdu.SduLength = 8U;
    pdu.MetaDataPtr = NULL_PTR;
    Com_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    /* Round-trip: the received byte must be readable back via the signal. */
    uint8 out = 0;
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_OK, Com_ReceiveSignal(0U, &out));
    TEST_ASSERT_EQUAL_UINT8(0x99, out);
}

/** @req SWS_Com_00010 */
void test_Com_TriggerTransmit_AfterInit_ShouldReturnResult(void) {
    Com_Init(&fullConfig);
    uint8 buf[8] = { 0 };
    PduInfoType pdu;
    pdu.SduDataPtr = buf;
    pdu.SduLength = 0U;
    pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = Com_TriggerTransmit(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Com must provide the configured IPDU length. */
    TEST_ASSERT_EQUAL_UINT16(8U, pdu.SduLength);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_Com_Init_DoubleInit_ShouldNotCrash(void) {
    Com_Init(&emptyConfig);
    mock_reset();
    /* Com_Init() re-initializes unconditionally and silently. */
    Com_Init(&emptyConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCalls);
}

void test_Com_DeInit_BeforeInit_ShouldNotCrash(void) {
    /* setUp() already left the module in COM_UNINIT. */
    Com_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCalls);
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_ID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_UNINIT, mock_DetLastErrorId);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Com_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Com_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Com_DeInit_AfterInit_ShouldSucceed);
    RUN_TEST(test_Com_SendSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_Com_SendSignal_NullData_ShouldFail);
    RUN_TEST(test_Com_ReceiveSignal_BeforeInit_ShouldFail);
    RUN_TEST(test_Com_SendSignalGroup_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_ReceiveSignalGroup_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_TriggerIPDUSend_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_TxConfirmation_ShouldNotCrash);
    RUN_TEST(test_Com_RxIndication_ShouldNotCrash);
    RUN_TEST(test_Com_TriggerTransmit_AfterInit_ShouldReturnResult);
    RUN_TEST(test_Com_Init_DoubleInit_ShouldNotCrash);
    RUN_TEST(test_Com_DeInit_BeforeInit_ShouldNotCrash);
    return UnityEnd();
}
