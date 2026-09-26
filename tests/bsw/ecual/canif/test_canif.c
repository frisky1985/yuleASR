/**
 * @file test_canif.c
 * @brief CanIf Unit Tests
 * @req SWS_CanIf
 */

// @tests src/bsw/ecual/canif/src/CanIf.c  @tests src/bsw/ecual/canif/include/CanIf.h
#include "unity.h"
#include "CanIf.h"
#include "Can.h"
#include "PduR.h"

/* Lower-layer stubs (Can driver + PduR), signatures per Can.h / PduR.h */
static uint8 mock_DetCalls = 0;
static uint8 mock_CanWriteCalls = 0;
static uint8 mock_PduRTxConfCalls = 0;
static uint8 mock_PduRRxIndCalls = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}
Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition) {
    (void)Controller;(void)Transition; return CAN_OK;
}
Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo) {
    (void)Hth;(void)PduInfo; mock_CanWriteCalls++; return CAN_OK;
}
Std_ReturnType Can_CheckWakeup(uint8 Controller) {
    (void)Controller; return E_OK;
}
void PduR_TxConfirmation(PduIdType TxPduId, Std_ReturnType result) {
    (void)TxPduId;(void)result; mock_PduRTxConfCalls++;
}
void PduR_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;(void)PduInfoPtr; mock_PduRRxIndCalls++;
}

/* Valid static config: one controller, 4 Tx / 4 Rx PDUs (matches the
 * compile-time CANIF_NUM_TX_PDUS / CANIF_NUM_RX_PDUS guards in CanIf.c). */
static const CanIf_ControllerConfigType testControllers[CANIF_NUM_CONTROLLERS] = {
    { 0U, 500000U, 0U, CANIF_CS_STOPPED, FALSE, FALSE, TRUE, FALSE }
};
static const CanIf_TxPduConfigType testTxPdus[CANIF_NUM_TX_PDUS] = {
    { 0U, 0x100U, 0U, 0U, 0U, 8U, TRUE,  FALSE },
    { 1U, 0x200U, 0U, 0U, 0U, 4U, FALSE, FALSE },
    { 2U, 0x300U, 0U, 1U, 0U, 2U, FALSE, FALSE },
    { 3U, 0x700U, 0U, 1U, 0U, 8U, TRUE,  FALSE }
};
static const CanIf_RxPduConfigType testRxPdus[CANIF_NUM_RX_PDUS] = {
    { 0U, 0x150U, 0x7FFU, 0U, 0U, 0U, 2U, TRUE },
    { 1U, 0x250U, 0x7FFU, 0U, 0U, 0U, 4U, TRUE },
    { 2U, 0x350U, 0x7FFU, 0U, 1U, 0U, 4U, TRUE },
    { 3U, 0x600U, 0x7FFU, 0U, 1U, 0U, 8U, TRUE }
};
static const CanIf_ConfigType testConfig = {
    testControllers, CANIF_NUM_CONTROLLERS,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    testTxPdus, CANIF_NUM_TX_PDUS,
    testRxPdus, CANIF_NUM_RX_PDUS,
    TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE
};

void setUp(void) {
    CanIf_DeInit(); /* normalize driver state; may report E_UNINIT, counted then cleared below */
    mock_DetCalls = 0;
    mock_CanWriteCalls = 0;
    mock_PduRTxConfCalls = 0;
    mock_PduRRxIndCalls = 0;
}
void tearDown(void) {}

/* NOTE: runner executes in declaration order and CanIf keeps static state,
 * so uninitialized-behavior tests come before any successful CanIf_Init(). */

/** @req SWS_CanIf_00001 */
void test_CanIf_Init_NullPtr_ShouldReportDet(void) {
    CanIf_Init(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00005 */
void test_CanIf_Transmit_BeforeInit_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = CanIf_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_CanWriteCalls);
}

/** @req SWS_CanIf_00001 */
void test_CanIf_Init_ValidConfig_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
    Std_VersionInfoType info;
    CanIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(CANIF_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanIf_00001 */
void test_CanIf_Init_DoubleInit_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    CanIf_Init(&testConfig);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00005 */
void test_CanIf_Transmit_NullPdu_ShouldFail(void) {
    CanIf_Init(&testConfig);
    Std_ReturnType ret = CanIf_Transmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_CanWriteCalls);
}

/** @req SWS_CanIf_00005 */
void test_CanIf_Transmit_OnlineController_ShouldCallCanWrite(void) {
    PduInfoType pdu;
    uint8 data[8] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    CanIf_Init(&testConfig);

    /* After Init the controller is STOPPED / PDU mode OFFLINE: Tx rejected
     * by CanIf itself, Can_Write must not be reached. */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0, mock_CanWriteCalls);

    /* Start controller and bring PDU mode online: Tx must reach Can_Write */
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetControllerMode(0U, CANIF_CS_STARTED));
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetPduMode(0U, CANIF_ONLINE));
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00003 */
void test_CanIf_SetControllerMode_AfterInit_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    Std_ReturnType ret = CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00004 */
void test_CanIf_GetControllerMode_AfterInit_ShouldReturnStopped(void) {
    CanIf_ControllerModeType mode = CANIF_CS_STARTED;
    CanIf_Init(&testConfig);
    Std_ReturnType ret = CanIf_GetControllerMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(CANIF_CS_STOPPED, mode); /* default after Init */
}

/** @req SWS_CanIf_00006 */
void test_CanIf_CancelTransmit_AfterInit_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    Std_ReturnType ret = CanIf_CancelTransmit(0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_CanIf_00007 */
void test_CanIf_SetPduMode_AfterInit_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    Std_ReturnType ret = CanIf_SetPduMode(0U, CANIF_ONLINE);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00008 */
void test_CanIf_GetPduMode_AfterInit_ShouldReturnOffline(void) {
    CanIf_PduModeType mode = CANIF_ONLINE;
    CanIf_Init(&testConfig);
    Std_ReturnType ret = CanIf_GetPduMode(0U, &mode);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL(CANIF_OFFLINE, mode); /* default after Init */
}

/** @req SWS_CanIf_00009 */
void test_CanIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    CanIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(CANIF_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanIf_00009 */
void test_CanIf_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    CanIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00010 */
void test_CanIf_TxConfirmation_ShouldNotifyPduR(void) {
    CanIf_Init(&testConfig);
    /* testTxPdus[0].TxConfirmation == TRUE: PduR_TxConfirmation expected */
    CanIf_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(1, mock_PduRTxConfCalls);
    /* testTxPdus[1].TxConfirmation == FALSE: no notification */
    CanIf_TxConfirmation(1U);
    TEST_ASSERT_EQUAL(1, mock_PduRTxConfCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00011 */
void test_CanIf_RxIndication_MatchingPdu_ShouldNotifyPduR(void) {
    CanIf_Init(&testConfig);
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    mailbox.CanId = 0x150U; mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;

    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(1, mock_PduRRxIndCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00012 */
void test_CanIf_ControllerBusOff_ShouldSetStopped(void) {
    CanIf_ControllerModeType mode = CANIF_CS_STARTED;
    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    CanIf_ControllerBusOff(0U);
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_CS_STOPPED, mode);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00002 */
void test_CanIf_DeInit_AfterInit_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    CanIf_DeInit();
    CanIf_Transmit(0U, NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); /* E_UNINIT after DeInit */
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanIf_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanIf_Transmit_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_CanIf_Init_DoubleInit_ShouldReportDet);
    RUN_TEST(test_CanIf_Transmit_NullPdu_ShouldFail);
    RUN_TEST(test_CanIf_Transmit_OnlineController_ShouldCallCanWrite);
    RUN_TEST(test_CanIf_SetControllerMode_AfterInit_ShouldSucceed);
    RUN_TEST(test_CanIf_GetControllerMode_AfterInit_ShouldReturnStopped);
    RUN_TEST(test_CanIf_CancelTransmit_AfterInit_ShouldSucceed);
    RUN_TEST(test_CanIf_SetPduMode_AfterInit_ShouldSucceed);
    RUN_TEST(test_CanIf_GetPduMode_AfterInit_ShouldReturnOffline);
    RUN_TEST(test_CanIf_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_CanIf_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanIf_TxConfirmation_ShouldNotifyPduR);
    RUN_TEST(test_CanIf_RxIndication_MatchingPdu_ShouldNotifyPduR);
    RUN_TEST(test_CanIf_ControllerBusOff_ShouldSetStopped);
    RUN_TEST(test_CanIf_DeInit_AfterInit_ShouldSucceed);

    return UnityEnd();
}
