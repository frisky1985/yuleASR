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
static Can_ReturnType mock_CanWriteReturn = CAN_OK;
static uint8 mock_CanWriteLastData[8];
static uint8 mock_CanWriteLastDlc = 0U;
static uint8 mock_PduRTxConfCalls = 0;
static uint8 mock_PduRRxIndCalls = 0;
static Can_ReturnType mock_CanSetCtrlModeReturn = CAN_OK;
static Std_ReturnType mock_CanCheckWakeupReturn = E_OK;

/* CanTrcv stub: CanTrcv.h is intentionally not included, the mode type is
 * mirrored with the same constants/values as CanTrcv_TrcvModeType. */
typedef enum {
    CANTRCV_TRCVMODE_NORMAL = 0u,
    CANTRCV_TRCVMODE_STANDBY = 1u,
    CANTRCV_TRCVMODE_SLEEP = 2u
} CanTrcv_TrcvModeType;

static uint8 mock_CanTrcvCalls = 0;
static uint8 mock_CanTrcvLastTransceiver = 0xFFU;
static CanTrcv_TrcvModeType mock_CanTrcvLastMode = CANTRCV_TRCVMODE_NORMAL;
static Std_ReturnType mock_CanTrcvReturn = E_OK;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}
Can_ReturnType Can_SetControllerMode(uint8 Controller, Can_ControllerStateType Transition) {
    (void)Controller;(void)Transition; return mock_CanSetCtrlModeReturn;
}
Can_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType* PduInfo) {
    uint8 i;
    uint8 len;
    (void)Hth;
    mock_CanWriteCalls++;
    if ((PduInfo != NULL) && (PduInfo->SduPtr != NULL)) {
        len = PduInfo->CanDlc;
        if (len > 8U) { len = 8U; }
        for (i = 0U; i < len; i++) {
            mock_CanWriteLastData[i] = PduInfo->SduPtr[i];
        }
        mock_CanWriteLastDlc = PduInfo->CanDlc;
    }
    return mock_CanWriteReturn;
}
Std_ReturnType Can_CheckWakeup(uint8 Controller) {
    (void)Controller; return mock_CanCheckWakeupReturn;
}
Std_ReturnType CanTrcv_SetOpMode(uint8 Transceiver, CanTrcv_TrcvModeType OpMode) {
    mock_CanTrcvCalls++;
    mock_CanTrcvLastTransceiver = Transceiver;
    mock_CanTrcvLastMode = OpMode;
    return mock_CanTrcvReturn;
}

/* Can driver error-state getter stubs (new AD3 APIs) */
static Std_ReturnType mock_CanGetErrState_Return = E_OK;
static Can_ErrorStateType mock_CanGetErrState_Value = CAN_ERRORSTATE_ACTIVE;
static uint8 mock_CanGetRxCnt_Value = 0U;
static uint8 mock_CanGetTxCnt_Value = 0U;
static uint8 mock_CanGetErrCalls = 0U;

Std_ReturnType Can_GetControllerErrorState(uint8 Controller, Can_ErrorStateType* ErrorStatePtr) {
    (void)Controller;
    mock_CanGetErrCalls++;
    if (mock_CanGetErrState_Return == E_OK) {
        *ErrorStatePtr = mock_CanGetErrState_Value;
    }
    return mock_CanGetErrState_Return;
}
Std_ReturnType Can_GetControllerRxErrorCounter(uint8 Controller, uint8* RxErrorCounterPtr) {
    (void)Controller;
    mock_CanGetErrCalls++;
    *RxErrorCounterPtr = mock_CanGetRxCnt_Value;
    return E_OK;
}
Std_ReturnType Can_GetControllerTxErrorCounter(uint8 Controller, uint8* TxErrorCounterPtr) {
    (void)Controller;
    mock_CanGetErrCalls++;
    *TxErrorCounterPtr = mock_CanGetTxCnt_Value;
    return E_OK;
}

/* CanSM indication stubs: capture calls so tests can assert forwarding. */
static uint8 mock_CanSM_CheckWufCalls = 0U;
static uint8 mock_CanSM_CheckWufLastTrcv = 0xFFU;
static uint8 mock_CanSM_ClearWufCalls = 0U;
static uint8 mock_CanSM_ClearWufLastTrcv = 0xFFU;
static uint8 mock_CanSM_TrcvModeCalls = 0U;
static uint8 mock_CanSM_TrcvModeLastTrcv = 0xFFU;
static CanIf_TransceiverModeType mock_CanSM_TrcvModeLastMode = CANIF_TRCV_MODE_NORMAL;
static uint8 mock_CanSM_CtrlModeCalls = 0U;
static uint8 mock_CanSM_CtrlModeLastCtrl = 0xFFU;
static CanIf_ControllerModeType mock_CanSM_CtrlModeLastMode = CANIF_CS_STOPPED;

void CanSM_CheckTransceiverWakeFlagIndication(uint8 NetworkHandle) {
    mock_CanSM_CheckWufCalls++;
    mock_CanSM_CheckWufLastTrcv = NetworkHandle;
}
Std_ReturnType CanSM_ClearTrcvWufFlagIndication(uint8 NetworkHandle) {
    mock_CanSM_ClearWufCalls++;
    mock_CanSM_ClearWufLastTrcv = NetworkHandle;
    return E_OK;
}
void CanSM_TransceiverModeIndication(uint8 NetworkHandle, CanIf_TransceiverModeType TransceiverMode) {
    mock_CanSM_TrcvModeCalls++;
    mock_CanSM_TrcvModeLastTrcv = NetworkHandle;
    mock_CanSM_TrcvModeLastMode = TransceiverMode;
}
void CanSM_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode) {
    mock_CanSM_CtrlModeCalls++;
    mock_CanSM_CtrlModeLastCtrl = ControllerId;
    mock_CanSM_CtrlModeLastMode = ControllerMode;
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
    { 0U, 0x100U, 0U, 0U, 0U, 8U, TRUE,  FALSE, FALSE },
    { 1U, 0x200U, 0U, 0U, 0U, 4U, FALSE, FALSE, FALSE },
    { 2U, 0x300U, 0U, 1U, 0U, 2U, FALSE, TRUE,  FALSE }, /* UserType: trigger-transmit capable */
    { 3U, 0x700U, 0U, 1U, 0U, 8U, TRUE,  FALSE, FALSE }
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
/* Same tables, but with Tx/Rx notification-status APIs enabled. */
static const CanIf_ConfigType testConfigNotify = {
    testControllers, CANIF_NUM_CONTROLLERS,
    NULL_PTR, 0U,
    NULL_PTR, 0U,
    testTxPdus, CANIF_NUM_TX_PDUS,
    testRxPdus, CANIF_NUM_RX_PDUS,
    TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, TRUE
};

void setUp(void) {
    uint8 i;
    CanIf_DeInit(); /* normalize driver state; may report E_UNINIT, counted then cleared below */
    mock_DetCalls = 0;
    mock_CanWriteCalls = 0;
    mock_CanWriteReturn = CAN_OK;
    mock_CanWriteLastDlc = 0U;
    for (i = 0U; i < 8U; i++) { mock_CanWriteLastData[i] = 0U; }
    mock_PduRTxConfCalls = 0;
    mock_PduRRxIndCalls = 0;
    mock_CanTrcvCalls = 0;
    mock_CanTrcvLastTransceiver = 0xFFU;
    mock_CanTrcvLastMode = CANTRCV_TRCVMODE_NORMAL;
    mock_CanTrcvReturn = E_OK;
    mock_CanGetErrState_Return = E_OK;
    mock_CanGetErrState_Value = CAN_ERRORSTATE_ACTIVE;
    mock_CanGetRxCnt_Value = 0U;
    mock_CanGetTxCnt_Value = 0U;
    mock_CanGetErrCalls = 0U;
    mock_CanSM_CheckWufCalls = 0U;
    mock_CanSM_CheckWufLastTrcv = 0xFFU;
    mock_CanSM_ClearWufCalls = 0U;
    mock_CanSM_ClearWufLastTrcv = 0xFFU;
    mock_CanSM_TrcvModeCalls = 0U;
    mock_CanSM_TrcvModeLastTrcv = 0xFFU;
    mock_CanSM_TrcvModeLastMode = CANIF_TRCV_MODE_NORMAL;
    mock_CanSM_CtrlModeCalls = 0U;
    mock_CanSM_CtrlModeLastCtrl = 0xFFU;
    mock_CanSM_CtrlModeLastMode = CANIF_CS_STOPPED;
    mock_CanSetCtrlModeReturn = CAN_OK;
    mock_CanCheckWakeupReturn = E_OK;
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

/* --- Before-init guards for the AD3 APIs (must run before CanIf_Init) --- */

void test_CanIf_GetControllerErrorState_BeforeInit_ShouldFail(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_ACTIVE;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerErrorState(0U, &state));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_CanGetErrCalls); /* must not reach the Can driver */
}

void test_CanIf_ReadTxNotifStatus_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadTxNotifStatus(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ReadRxNotifStatus_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadRxNotifStatus(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_TriggerTransmit_BeforeInit_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[2] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(2U, &pdu));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ConfirmPnAvailability_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_ConfirmPnAvailability(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_CheckTrcvWakeFlag_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckTrcvWakeFlag(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ClearTrcvWufFlag_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_ClearTrcvWufFlag(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetPnWakeupFilter_BeforeInit_ShouldFail(void) {
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x1FFU, 0x7FFU, TRUE };
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_TxQueueMainFunction_BeforeInit_ShouldReportDet(void) {
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
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

/** @req SWS_CanIf_00016 */
void test_CanIf_SetTrcvMode_Standby_AfterInit_ShouldSucceed(void) {
    CanIf_TransceiverModeType mode = CANIF_TRCV_MODE_NORMAL;
    CanIf_Init(&testConfig);

    TEST_ASSERT_EQUAL(E_OK, CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_STANDBY));
    TEST_ASSERT_EQUAL(1, mock_CanTrcvCalls);
    TEST_ASSERT_EQUAL(0, mock_CanTrcvLastTransceiver);
    TEST_ASSERT_EQUAL(CANTRCV_TRCVMODE_STANDBY, mock_CanTrcvLastMode);

    TEST_ASSERT_EQUAL(E_OK, CanIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_TRCV_MODE_STANDBY, mode);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00016 */
void test_CanIf_SetTrcvMode_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_STANDBY);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_CanTrcvCalls);
}

/** @req SWS_CanIf_00016 */
void test_CanIf_SetTrcvMode_InvalidTrcvId_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;

    Std_ReturnType ret = CanIf_SetTrcvMode(CANIF_NUM_TRANSCEIVERS, CANIF_TRCV_MODE_NORMAL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_CanTrcvCalls);
}

/** @req SWS_CanIf_00017 */
void test_CanIf_SetTrcvMode_TrcvError_ShouldKeepPreviousMode(void) {
    CanIf_TransceiverModeType mode = CANIF_TRCV_MODE_NORMAL;
    CanIf_Init(&testConfig);

    TEST_ASSERT_EQUAL(E_OK, CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_STANDBY));

    mock_CanTrcvReturn = E_NOT_OK;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_SLEEP));

    TEST_ASSERT_EQUAL(E_OK, CanIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_TRCV_MODE_STANDBY, mode); /* unchanged */
}

/** @req SWS_CanIf_00022 */
void test_CanIf_CheckValidation_ShouldSucceedOncePerWakeup(void) {
    CanIf_Init(&testConfig);

    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckValidation(0U));
    TEST_ASSERT_EQUAL(E_OK, CanIf_CheckWakeup(0U));
    TEST_ASSERT_EQUAL(E_OK, CanIf_CheckValidation(0U));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckValidation(0U)); /* flag consumed */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00023 */
void test_CanIf_GetTxConfirmationState_AfterTransmitAndConfirm(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(CANIF_TXCONF_NONE, CanIf_GetTxConfirmationState(0U));

    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);

    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(CANIF_TXCONF_PENDING, CanIf_GetTxConfirmationState(0U));

    CanIf_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(CANIF_TXCONF_CONFIRMED, CanIf_GetTxConfirmationState(0U));
    TEST_ASSERT_EQUAL(1, mock_PduRTxConfCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_CanIf_00023 */
void test_CanIf_GetTxConfirmationState_InvalidPduId_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;

    TEST_ASSERT_EQUAL(CANIF_TXCONF_NONE, CanIf_GetTxConfirmationState(CANIF_NUM_TX_PDUS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- AD3: error-state delegation to the Can driver --- */

void test_CanIf_GetControllerErrorState_ShouldDelegateToCan(void) {
    Can_ErrorStateType state = CAN_ERRORSTATE_ACTIVE;
    CanIf_Init(&testConfig);
    mock_CanGetErrState_Value = CAN_ERRORSTATE_PASSIVE;

    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerErrorState(0U, &state));
    TEST_ASSERT_EQUAL(CAN_ERRORSTATE_PASSIVE, state);
    TEST_ASSERT_EQUAL(1, mock_CanGetErrCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetControllerRxErrorCounter_ShouldDelegateToCan(void) {
    uint8 counter = 0U;
    CanIf_Init(&testConfig);
    mock_CanGetRxCnt_Value = 0x55U;

    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerRxErrorCounter(0U, &counter));
    TEST_ASSERT_EQUAL(0x55U, counter);
    TEST_ASSERT_EQUAL(1, mock_CanGetErrCalls);
}

void test_CanIf_GetControllerTxErrorCounter_ShouldDelegateToCan(void) {
    uint8 counter = 0U;
    CanIf_Init(&testConfig);
    mock_CanGetTxCnt_Value = 0x77U;

    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerTxErrorCounter(0U, &counter));
    TEST_ASSERT_EQUAL(0x77U, counter);
    TEST_ASSERT_EQUAL(1, mock_CanGetErrCalls);
}

void test_CanIf_GetControllerErrorState_NullPtr_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerErrorState(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
    TEST_ASSERT_EQUAL(0, mock_CanGetErrCalls); /* guarded in CanIf, not delegated */
}

void test_CanIf_GetControllerRxErrorCounter_InvalidController_ShouldReportDet(void) {
    uint8 counter = 0U;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerRxErrorCounter(CANIF_NUM_CONTROLLERS, &counter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- AD3: notification status, read-and-clear, gated by config --- */

void test_CanIf_ReadTxNotifStatus_DisabledApi_ShouldReportDet(void) {
    CanIf_Init(&testConfig); /* ReadTxPduNotifyStatusApi == FALSE here */
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadTxNotifStatus(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ReadTxNotifStatus_ShouldSetAndClearOnRead(void) {
    CanIf_Init(&testConfigNotify); /* notify-status APIs enabled */
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadTxNotifStatus(0U));

    CanIf_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(CANIF_TX_RX_NOTIFICATION, CanIf_ReadTxNotifStatus(0U));
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadTxNotifStatus(0U)); /* consumed */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ReadRxNotifStatus_DisabledApi_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadRxNotifStatus(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ReadRxNotifStatus_ShouldSetAndClearOnRead(void) {
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    mailbox.CanId = 0x150U; mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;

    CanIf_Init(&testConfigNotify);
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadRxNotifStatus(0U));

    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(CANIF_TX_RX_NOTIFICATION, CanIf_ReadRxNotifStatus(0U));
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadRxNotifStatus(0U)); /* consumed */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/* --- AD3: trigger transmit --- */

void test_CanIf_TriggerTransmit_NotConfiguredPdu_ShouldFail(void) {
    PduInfoType txPdu;
    PduInfoType trigPdu;
    uint8 txData[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8 trigBuf[8] = {0U};

    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);

    txPdu.SduDataPtr = txData; txPdu.SduLength = 8U; txPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &txPdu)); /* PDU0 UserType == FALSE */

    trigPdu.SduDataPtr = trigBuf; trigPdu.SduLength = 8U; trigPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(0U, &trigPdu));
}

void test_CanIf_TriggerTransmit_NoCachedData_ShouldFail(void) {
    PduInfoType trigPdu;
    uint8 trigBuf[8] = {0U};

    CanIf_Init(&testConfig);
    trigPdu.SduDataPtr = trigBuf; trigPdu.SduLength = 8U; trigPdu.MetaDataPtr = NULL_PTR;
    /* PDU2 is trigger-transmit capable but never transmitted */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(2U, &trigPdu));
}

void test_CanIf_TriggerTransmit_ShouldFillCachedDataWithMinLength(void) {
    PduInfoType txPdu;
    PduInfoType trigPdu;
    uint8 txData[8] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    uint8 trigBuf[8] = {0U};

    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);

    /* PDU2 configured Length == 2: cache must hold min(8, 2) = 2 bytes */
    txPdu.SduDataPtr = txData; txPdu.SduLength = 8U; txPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(2U, &txPdu));

    trigPdu.SduDataPtr = trigBuf; trigPdu.SduLength = 8U; trigPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanIf_TriggerTransmit(2U, &trigPdu));
    TEST_ASSERT_EQUAL(2U, trigPdu.SduLength); /* min(requested 8, configured 2, cached 2) */
    TEST_ASSERT_EQUAL(0x11U, trigBuf[0]);
    TEST_ASSERT_EQUAL(0x22U, trigBuf[1]);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_TriggerTransmit_ShouldTruncateToCachedLength(void) {
    PduInfoType txPdu;
    PduInfoType trigPdu;
    uint8 shortData[1] = {0x42U};
    uint8 trigBuf[8] = {0U};

    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);

    /* Cache holds only 1 byte although the PDU is configured for 2 */
    txPdu.SduDataPtr = shortData; txPdu.SduLength = 1U; txPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(2U, &txPdu));

    trigPdu.SduDataPtr = trigBuf; trigPdu.SduLength = 2U; trigPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_OK, CanIf_TriggerTransmit(2U, &trigPdu));
    TEST_ASSERT_EQUAL(1U, trigPdu.SduLength); /* min(requested 2, configured 2, cached 1) */
    TEST_ASSERT_EQUAL(0x42U, trigBuf[0]);
}

void test_CanIf_TriggerTransmit_NullBuffer_ShouldReportDet(void) {
    PduInfoType trigPdu;
    uint8 trigBuf[2] = {0U};

    CanIf_Init(&testConfig);
    mock_DetCalls = 0;

    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(2U, NULL_PTR));
    trigPdu.SduDataPtr = NULL_PTR; trigPdu.SduLength = 2U; trigPdu.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(2U, &trigPdu));
    TEST_ASSERT_EQUAL(2, mock_DetCalls);
    (void)trigBuf;
}

/* --- AD3: PN availability / transceiver wake flags (CanIf-level state) --- */

void test_CanIf_ConfirmPnAvailability_ValidTrcv_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_ConfirmPnAvailability(0U));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ConfirmPnAvailability_InvalidTrcv_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_ConfirmPnAvailability(CANIF_NUM_TRANSCEIVERS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_CheckTrcvWakeFlag_ValidTrcv_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_CheckTrcvWakeFlag(0U));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_CheckTrcvWakeFlag_InvalidTrcv_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckTrcvWakeFlag(CANIF_NUM_TRANSCEIVERS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ClearTrcvWufFlag_ValidTrcv_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_ClearTrcvWufFlag(0U));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/* --- AD3: indication forwarding to CanSM --- */

void test_CanIf_CheckTrcvWakeFlagIndication_ShouldNotifyCanSM(void) {
    CanIf_Init(&testConfig);
    CanIf_CheckTrcvWakeFlag(0U); /* latch pending flag */

    CanIf_CheckTrcvWakeFlagIndication(0U);
    TEST_ASSERT_EQUAL(1, mock_CanSM_CheckWufCalls);
    TEST_ASSERT_EQUAL(0U, mock_CanSM_CheckWufLastTrcv);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ClearTrcvWufFlagIndication_ShouldNotifyCanSM(void) {
    CanIf_Init(&testConfig);
    CanIf_ClearTrcvWufFlag(0U);

    CanIf_ClearTrcvWufFlagIndication(0U);
    TEST_ASSERT_EQUAL(1, mock_CanSM_ClearWufCalls);
    TEST_ASSERT_EQUAL(0U, mock_CanSM_ClearWufLastTrcv);
}

void test_CanIf_TrcvModeIndication_ShouldUpdateAndNotifyCanSM(void) {
    CanIf_TransceiverModeType mode = CANIF_TRCV_MODE_NORMAL;
    CanIf_Init(&testConfig);

    CanIf_TrcvModeIndication(0U, CANIF_TRCV_MODE_STANDBY);
    TEST_ASSERT_EQUAL(1, mock_CanSM_TrcvModeCalls);
    TEST_ASSERT_EQUAL(0U, mock_CanSM_TrcvModeLastTrcv);
    TEST_ASSERT_EQUAL(CANIF_TRCV_MODE_STANDBY, mock_CanSM_TrcvModeLastMode);
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetTrcvMode(0U, &mode)); /* cached mode updated */
    TEST_ASSERT_EQUAL(CANIF_TRCV_MODE_STANDBY, mode);
}

void test_CanIf_ControllerModeIndication_ShouldNotifyCanSM(void) {
    CanIf_Init(&testConfig);

    CanIf_ControllerModeIndication(0U, CANIF_CS_STARTED);
    TEST_ASSERT_EQUAL(1, mock_CanSM_CtrlModeCalls);
    TEST_ASSERT_EQUAL(0U, mock_CanSM_CtrlModeLastCtrl);
    TEST_ASSERT_EQUAL(CANIF_CS_STARTED, mock_CanSM_CtrlModeLastMode);
}

/* --- Phase 4: PN wakeup filtering (CanIf_SetPnWakeupFilter + RxIndication gate) --- */

static void startOnlineTx(void) {
    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);
}

void test_CanIf_SetPnWakeupFilter_ValidRange_ShouldSucceed(void) {
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x1FFU, 0x7FFU, TRUE };
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetPnWakeupFilter_InvalidTrcv_ShouldReportDet(void) {
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x1FFU, 0x7FFU, TRUE };
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetPnWakeupFilter(CANIF_NUM_TRANSCEIVERS, &filter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetPnWakeupFilter_NullPtr_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetPnWakeupFilter(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetPnWakeupFilter_InvertedRange_ShouldReportDet(void) {
    CanIf_PnWakeupFilterType filter = { 0x300U, 0x100U, 0x7FFU, TRUE };
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_RxIndication_PnFilter_MatchingId_ShouldPass(void) {
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x1FFU, 0x7FFU, TRUE };
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    mailbox.CanId = 0x150U; mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(1, mock_PduRRxIndCalls); /* 0x150 is inside [0x100, 0x1FF] */
}

void test_CanIf_RxIndication_PnFilter_NonMatchingId_ShouldDrop(void) {
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x1FFU, 0x7FFU, TRUE };
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    mailbox.CanId = 0x250U; mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(0, mock_PduRRxIndCalls); /* 0x250 outside [0x100, 0x1FF] */
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_RxIndication_PnFilter_MaskAppliedToRangeAndFrame(void) {
    /* Mask 0x700 narrows both range bounds and received IDs:
     * range [0x100,0x100]&0x700 = [0x100,0x100]; 0x150&0x700=0x100 passes,
     * 0x250&0x700=0x200 drops. */
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x100U, 0x700U, TRUE };
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    mailbox.CanId = 0x150U;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(1, mock_PduRRxIndCalls);
    mailbox.CanId = 0x250U;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(1, mock_PduRRxIndCalls); /* masked out */
}

void test_CanIf_RxIndication_NoPnFilter_ShouldPassAll(void) {
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    CanIf_Init(&testConfig);
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    mailbox.CanId = 0x150U;
    CanIf_RxIndication(&mailbox, &pdu);
    mailbox.CanId = 0x250U;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(2, mock_PduRRxIndCalls); /* no filter: everything passes */
}

void test_CanIf_DeInit_ShouldClearPnFilter(void) {
    CanIf_PnWakeupFilterType filter = { 0x100U, 0x1FFU, 0x7FFU, TRUE };
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetPnWakeupFilter(0U, &filter));
    CanIf_DeInit();
    CanIf_Init(&testConfig); /* fresh init: filter storage must be cleared */
    mailbox.CanId = 0x250U; mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(1, mock_PduRRxIndCalls); /* previously filtered ID passes again */
}

/* --- Phase 4: Hoh-bucketed Rx dispatch lookup table --- */

static void rxDispatchOnce(uint16 Hoh, uint32 CanId) {
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0xAAU, 0xBBU};
    mailbox.CanId = CanId; mailbox.Hoh = Hoh; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_RxIndication(&mailbox, &pdu);
}

void test_CanIf_RxLookup_DispatchesEachHohBucket(void) {
    CanIf_Init(&testConfig);
    rxDispatchOnce(0U, 0x150U); /* Hoh 0 bucket, entry 0 */
    rxDispatchOnce(0U, 0x250U); /* Hoh 0 bucket, entry 1 */
    rxDispatchOnce(1U, 0x350U); /* Hoh 1 bucket, entry 0 */
    rxDispatchOnce(1U, 0x600U); /* Hoh 1 bucket, entry 1 */
    TEST_ASSERT_EQUAL(4, mock_PduRRxIndCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_RxIndication_UnknownCanId_ShouldDrop(void) {
    CanIf_Init(&testConfig);
    rxDispatchOnce(0U, 0x999U);
    TEST_ASSERT_EQUAL(0, mock_PduRRxIndCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_RxIndication_UnknownHoh_ShouldDrop(void) {
    CanIf_Init(&testConfig);
    rxDispatchOnce(0x42U, 0x150U);   /* Hoh beyond configured, inside table range */
    rxDispatchOnce(0xFFFEU, 0x150U); /* Hoh beyond table: linear fallback path */
    TEST_ASSERT_EQUAL(0, mock_PduRRxIndCalls);
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_RxLookup_Equivalence_WithLinearScan(void) {
    /* Lookup-table dispatch must accept exactly the (Hoh, CanId) pairs the
     * linear scan accepted, including out-of-table Hoh values. */
    static const struct {
        uint16 Hoh;
        uint32 CanId;
        boolean ExpectDispatch;
    } cases[] = {
        { 0U,      0x150U, TRUE  },
        { 0U,      0x250U, TRUE  },
        { 1U,      0x350U, TRUE  },
        { 1U,      0x600U, TRUE  },
        { 0U,      0x151U, FALSE },
        { 1U,      0x601U, FALSE },
        { 2U,      0x150U, FALSE },
        { 0xFFFEU, 0x150U, FALSE }
    };
    int dispatched = 0;
    CanIf_Init(&testConfig);
    for (uint8 i = 0U; i < (sizeof(cases) / sizeof(cases[0])); i++) {
        rxDispatchOnce(cases[i].Hoh, cases[i].CanId);
        if (cases[i].ExpectDispatch == TRUE) {
            dispatched++;
        }
        /* Unity build lacks *_MESSAGE variants; the loop index identifies a mismatch */
        TEST_ASSERT_EQUAL(dispatched, mock_PduRRxIndCalls);
    }
}

/* --- Phase 4: Tx retry queue (CAN_BUSY buffering) --- */

void test_CanIf_Transmit_CanBusy_ShouldEnqueueForRetry(void) {
    PduInfoType pdu;
    uint8 data[8] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu)); /* buffered, not dropped */
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls);

    CanIf_TxQueueMainFunction(); /* hardware still busy */
    TEST_ASSERT_EQUAL(2, mock_CanWriteCalls);

    mock_CanWriteReturn = CAN_OK;
    CanIf_TxQueueMainFunction(); /* queued frame is retransmitted */
    TEST_ASSERT_EQUAL(3, mock_CanWriteCalls);
    TEST_ASSERT_EQUAL(8U, mock_CanWriteLastDlc);
    TEST_ASSERT_EQUAL(0x11U, mock_CanWriteLastData[0]);
    TEST_ASSERT_EQUAL(0x88U, mock_CanWriteLastData[7]);
    TEST_ASSERT_EQUAL(CANIF_TXCONF_PENDING, CanIf_GetTxConfirmationState(0U));

    CanIf_TxQueueMainFunction(); /* queue empty: no further writes */
    TEST_ASSERT_EQUAL(3, mock_CanWriteCalls);
}

void test_CanIf_TxQueue_ShouldDrainInFifoOrder(void) {
    PduInfoType pdu0;
    PduInfoType pdu1;
    uint8 d0[8] = {0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U};
    uint8 d1[4] = {0x21U, 0x22U, 0x23U, 0x24U};
    pdu0.SduDataPtr = d0; pdu0.SduLength = 8U; pdu0.MetaDataPtr = NULL_PTR;
    pdu1.SduDataPtr = d1; pdu1.SduLength = 4U; pdu1.MetaDataPtr = NULL_PTR;
    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu0));
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(1U, &pdu1));
    TEST_ASSERT_EQUAL(2, mock_CanWriteCalls);

    mock_CanWriteReturn = CAN_OK;
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(4, mock_CanWriteCalls); /* both queued frames drained */
    /* FIFO order: the second queued frame (4 bytes, 0x21..) written last */
    TEST_ASSERT_EQUAL(4U, mock_CanWriteLastDlc);
    TEST_ASSERT_EQUAL(0x21U, mock_CanWriteLastData[0]);
    TEST_ASSERT_EQUAL(0x24U, mock_CanWriteLastData[3]);
    TEST_ASSERT_EQUAL(CANIF_TXCONF_PENDING, CanIf_GetTxConfirmationState(1U));
}

void test_CanIf_TxQueue_Full_ShouldReturnNotOk(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    for (uint8 i = 0U; i < CANIF_TX_QUEUE_DEPTH; i++) {
        TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu));
    }
    TEST_ASSERT_EQUAL(CANIF_TX_QUEUE_DEPTH, mock_CanWriteCalls);

    /* Queue full: the frame is rejected instead of silently dropped */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL((int)CANIF_TX_QUEUE_DEPTH + 1, mock_CanWriteCalls);

    mock_CanWriteReturn = CAN_OK;
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL((int)CANIF_TX_QUEUE_DEPTH * 2 + 1, mock_CanWriteCalls); /* exactly the buffered 16 drained */
}

void test_CanIf_Transmit_OversizedFrame_ShouldNotBeBuffered(void) {
    PduInfoType pdu;
    uint8 data[12] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 12U; pdu.MetaDataPtr = NULL_PTR;
    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu)); /* exceeds queue entry size */
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls); /* only the rejected Can_Write attempt */
}

void test_CanIf_ControllerBusOff_ShouldFlushTxQueue(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls);

    CanIf_ControllerBusOff(0U); /* queued frame must not outlive the bus-off */
    mock_CanWriteReturn = CAN_OK;
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls);
}

void test_CanIf_DeInit_ShouldFlushTxQueue(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls);

    CanIf_DeInit();
    CanIf_Init(&testConfig);
    mock_CanWriteReturn = CAN_OK;
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls); /* queue did not survive DeInit */
}

/* --- AD3: MetaData <-> CAN ID conversion --- */

void test_CanIf_CanIdToMetaData_ExtendedId_ShouldPackBigEndian(void) {
    uint8 meta[4] = {0U};
    TEST_ASSERT_EQUAL(E_OK, CanIf_CanIdToMetaData(0x1FFFFFFFU, meta));
    TEST_ASSERT_EQUAL(0xFFU, meta[0]);
    TEST_ASSERT_EQUAL(0xFFU, meta[1]);
    TEST_ASSERT_EQUAL(0xFFU, meta[2]);
    TEST_ASSERT_EQUAL(0xFFU, meta[3]);
}

void test_CanIf_CanIdToMetaData_StandardId_ShouldPackBigEndian(void) {
    uint8 meta[4] = {0U};
    TEST_ASSERT_EQUAL(E_OK, CanIf_CanIdToMetaData(0x123U, meta));
    TEST_ASSERT_EQUAL(0x00U, meta[0]);
    TEST_ASSERT_EQUAL(0x00U, meta[1]);
    TEST_ASSERT_EQUAL(0x09U, meta[2]); /* bits 12..5 */
    TEST_ASSERT_EQUAL(0x23U, meta[3]); /* bits 7..0 */
}

void test_CanIf_CanIdToMetaData_NullPtr_ShouldReportDet(void) {
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CanIdToMetaData(0x123U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_MetaDataToCanId_NullPtr_ShouldReportDet(void) {
    uint8 meta[4] = {0U};
    uint32 canId = 0U;
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_MetaDataToCanId(NULL_PTR, &canId));
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_MetaDataToCanId(meta, NULL_PTR));
    TEST_ASSERT_EQUAL(2, mock_DetCalls);
}

void test_CanIf_MetaData_RoundTrip_ShouldPreserveCanId(void) {
    uint8 meta[4] = {0U};
    uint32 canId = 0U;
    const uint32 testIds[3] = {0x00000000U, 0x00000123U, 0x1FFFFFFFU};

    for (uint8 i = 0U; i < 3U; i++) {
        TEST_ASSERT_EQUAL(E_OK, CanIf_CanIdToMetaData(testIds[i], meta));
        TEST_ASSERT_EQUAL(E_OK, CanIf_MetaDataToCanId(meta, &canId));
        TEST_ASSERT_EQUAL(testIds[i], canId);
    }
}

/* --- Coverage expansion: SetControllerMode branches --- */

void test_CanIf_SetControllerMode_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetControllerMode(0U, CANIF_CS_STARTED));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetControllerMode_InvalidController_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetControllerMode(CANIF_NUM_CONTROLLERS, CANIF_CS_STARTED));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetControllerMode_Stopped_ShouldSucceed(void) {
    CanIf_ControllerModeType mode = CANIF_CS_STARTED;
    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetControllerMode(0U, CANIF_CS_STOPPED));
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_CS_STOPPED, mode);
}

void test_CanIf_SetControllerMode_Sleep_ShouldSucceed(void) {
    CanIf_ControllerModeType mode = CANIF_CS_STARTED;
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetControllerMode(0U, CANIF_CS_SLEEP));
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_CS_SLEEP, mode);
}

void test_CanIf_SetControllerMode_InvalidMode_ShouldFail(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetControllerMode(0U, (CanIf_ControllerModeType)0xFFU));
}

/* --- Coverage expansion: GetControllerMode error paths --- */

void test_CanIf_GetControllerMode_BeforeInit_ShouldFail(void) {
    CanIf_ControllerModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetControllerMode_InvalidController_ShouldFail(void) {
    CanIf_ControllerModeType mode;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerMode(CANIF_NUM_CONTROLLERS, &mode));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetControllerMode_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerMode(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: Transmit error paths --- */

void test_CanIf_Transmit_InvalidTxPduId_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(CANIF_NUM_TX_PDUS, &pdu));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_Transmit_ControllerNotStarted_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_Init(&testConfig);
    /* Controller is STOPPED after init */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0, mock_CanWriteCalls);
}

void test_CanIf_Transmit_PduModeOffline_ShouldFail(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    /* PDU mode is OFFLINE after init */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0, mock_CanWriteCalls);
}

void test_CanIf_Transmit_CanNotOk_ShouldReturnNotOk(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);
    /* CAN_NOT_OK is neither CAN_OK nor CAN_BUSY -> line 473 */
    mock_CanWriteReturn = CAN_NOT_OK;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu));
}

/* --- Coverage expansion: CancelTransmit error paths --- */

void test_CanIf_CancelTransmit_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CancelTransmit(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_CancelTransmit_InvalidTxPduId_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CancelTransmit(CANIF_NUM_TX_PDUS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: SetPduMode error paths --- */

void test_CanIf_SetPduMode_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetPduMode(0U, CANIF_ONLINE));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetPduMode_InvalidController_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetPduMode(CANIF_NUM_CONTROLLERS, CANIF_ONLINE));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: GetPduMode error paths --- */

void test_CanIf_GetPduMode_BeforeInit_ShouldFail(void) {
    CanIf_PduModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetPduMode(0U, &mode));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetPduMode_InvalidController_ShouldFail(void) {
    CanIf_PduModeType mode;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetPduMode(CANIF_NUM_CONTROLLERS, &mode));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetPduMode_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetPduMode(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: SetDynamicTxId all paths --- */

void test_CanIf_SetDynamicTxId_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetDynamicTxId(0U, 0x100U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetDynamicTxId_InvalidTxPduId_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetDynamicTxId(CANIF_NUM_TX_PDUS, 0x100U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetDynamicTxId_Valid_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetDynamicTxId(0U, 0x123U));
    TEST_ASSERT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: SetTrcvMode NORMAL and SLEEP --- */

void test_CanIf_SetTrcvMode_Normal_ShouldSucceed(void) {
    CanIf_TransceiverModeType mode = CANIF_TRCV_MODE_STANDBY;
    CanIf_Init(&testConfig);
    (void)CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_STANDBY);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_NORMAL));
    TEST_ASSERT_EQUAL(CANTRCV_TRCVMODE_NORMAL, mock_CanTrcvLastMode);
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_TRCV_MODE_NORMAL, mode);
}

void test_CanIf_SetTrcvMode_Sleep_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetTrcvMode(0U, CANIF_TRCV_MODE_SLEEP));
    TEST_ASSERT_EQUAL(CANTRCV_TRCVMODE_SLEEP, mock_CanTrcvLastMode);
}

void test_CanIf_SetTrcvMode_InvalidMode_ShouldFail(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetTrcvMode(0U, (CanIf_TransceiverModeType)0xFFU));
    TEST_ASSERT_EQUAL(0, mock_CanTrcvCalls);
}

/* --- Coverage expansion: GetTrcvMode error paths --- */

void test_CanIf_GetTrcvMode_BeforeInit_ShouldFail(void) {
    CanIf_TransceiverModeType mode;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetTrcvMode_InvalidTrcv_ShouldFail(void) {
    CanIf_TransceiverModeType mode;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetTrcvMode(CANIF_NUM_TRANSCEIVERS, &mode));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetTrcvMode_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetTrcvMode(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: GetTrcvWakeupReason all paths --- */

void test_CanIf_GetTrcvWakeupReason_BeforeInit_ShouldFail(void) {
    CanIf_TrcvWakeupReasonType reason;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetTrcvWakeupReason(0U, &reason));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetTrcvWakeupReason_InvalidTrcv_ShouldFail(void) {
    CanIf_TrcvWakeupReasonType reason;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetTrcvWakeupReason(CANIF_NUM_TRANSCEIVERS, &reason));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetTrcvWakeupReason_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetTrcvWakeupReason(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetTrcvWakeupReason_Valid_ShouldReturnNotSupported(void) {
    CanIf_TrcvWakeupReasonType reason = (CanIf_TrcvWakeupReasonType)0xFFU;
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetTrcvWakeupReason(0U, &reason));
    TEST_ASSERT_EQUAL(CANIF_TRCV_WU_NOT_SUPPORTED, reason);
}

/* --- Coverage expansion: SetTrcvWakeupMode all paths --- */

void test_CanIf_SetTrcvWakeupMode_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetTrcvWakeupMode(0U, (CanIf_TrcvWakeupModeType)0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetTrcvWakeupMode_InvalidTrcv_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetTrcvWakeupMode(CANIF_NUM_TRANSCEIVERS, (CanIf_TrcvWakeupModeType)0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetTrcvWakeupMode_Valid_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetTrcvWakeupMode(0U, (CanIf_TrcvWakeupModeType)0U));
}

/* --- Coverage expansion: SetBaudrate all paths --- */

void test_CanIf_SetBaudrate_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetBaudrate(0U, 500U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetBaudrate_InvalidController_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetBaudrate(CANIF_NUM_CONTROLLERS, 500U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_SetBaudrate_Valid_ShouldSucceed(void) {
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_SetBaudrate(0U, 250U));
}

/* --- Coverage expansion: GetBaudrate all paths --- */

void test_CanIf_GetBaudrate_BeforeInit_ShouldFail(void) {
    uint16 baudrate;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetBaudrate(0U, &baudrate));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetBaudrate_InvalidController_ShouldFail(void) {
    uint16 baudrate;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetBaudrate(CANIF_NUM_CONTROLLERS, &baudrate));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetBaudrate_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetBaudrate(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetBaudrate_Valid_ShouldReturnDefault(void) {
    uint16 baudrate = 0U;
    CanIf_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetBaudrate(0U, &baudrate));
    TEST_ASSERT_EQUAL(CANIF_DEFAULT_BAUDRATE, baudrate);
}

/* --- Coverage expansion: DeInit before init --- */

void test_CanIf_DeInit_BeforeInit_ShouldReportDet(void) {
    /* setUp already calls DeInit, so driver is uninit here */
    CanIf_DeInit();
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: CheckWakeup / CheckValidation before init --- */

void test_CanIf_CheckWakeup_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckWakeup(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_CheckValidation_BeforeInit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckValidation(0U));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: GetControllerErrorState invalid controller --- */

void test_CanIf_GetControllerErrorState_InvalidController_ShouldFail(void) {
    Can_ErrorStateType state;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerErrorState(CANIF_NUM_CONTROLLERS, &state));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: GetControllerRxErrorCounter before init --- */

void test_CanIf_GetControllerRxErrorCounter_BeforeInit_ShouldFail(void) {
    uint8 counter;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerRxErrorCounter(0U, &counter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetControllerRxErrorCounter_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerRxErrorCounter(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: GetControllerTxErrorCounter error paths --- */

void test_CanIf_GetControllerTxErrorCounter_BeforeInit_ShouldFail(void) {
    uint8 counter;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerTxErrorCounter(0U, &counter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetControllerTxErrorCounter_InvalidController_ShouldFail(void) {
    uint8 counter;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerTxErrorCounter(CANIF_NUM_CONTROLLERS, &counter));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_GetControllerTxErrorCounter_NullPtr_ShouldFail(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_GetControllerTxErrorCounter(0U, NULL_PTR));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: ReadTxNotifStatus / ReadRxNotifStatus invalid PDU --- */

void test_CanIf_ReadTxNotifStatus_InvalidTxPduId_ShouldFail(void) {
    CanIf_Init(&testConfigNotify);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadTxNotifStatus(CANIF_NUM_TX_PDUS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_CanIf_ReadRxNotifStatus_InvalidRxPduId_ShouldFail(void) {
    CanIf_Init(&testConfigNotify);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(CANIF_NO_NOTIFICATION, CanIf_ReadRxNotifStatus(CANIF_NUM_RX_PDUS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: TriggerTransmit invalid TxPduId --- */

void test_CanIf_TriggerTransmit_InvalidTxPduId_ShouldFail(void) {
    PduInfoType trigPdu;
    uint8 buf[8] = {0U};
    trigPdu.SduDataPtr = buf; trigPdu.SduLength = 8U; trigPdu.MetaDataPtr = NULL_PTR;
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(CANIF_NUM_TX_PDUS, &trigPdu));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* --- Coverage expansion: indication callbacks before init / invalid index --- */

void test_CanIf_TxConfirmation_BeforeInit_ShouldReturnSilently(void) {
    CanIf_TxConfirmation(0U);
    TEST_ASSERT_EQUAL(0, mock_PduRTxConfCalls);
}

void test_CanIf_RxIndication_BeforeInit_ShouldReturnSilently(void) {
    Can_HwType mailbox;
    PduInfoType pdu;
    uint8 data[2] = {0U};
    mailbox.CanId = 0x150U; mailbox.Hoh = 0U; mailbox.ControllerId = 0U;
    pdu.SduDataPtr = data; pdu.SduLength = 2U; pdu.MetaDataPtr = NULL_PTR;
    CanIf_RxIndication(&mailbox, &pdu);
    TEST_ASSERT_EQUAL(0, mock_PduRRxIndCalls);
}

void test_CanIf_ControllerBusOff_BeforeInit_ShouldReturnSilently(void) {
    CanIf_ControllerBusOff(0U);
    /* no crash, no DET (silent return) */
}

void test_CanIf_CheckTrcvWakeFlagIndication_BeforeInit_ShouldReturnSilently(void) {
    CanIf_CheckTrcvWakeFlagIndication(0U);
    TEST_ASSERT_EQUAL(0, mock_CanSM_CheckWufCalls);
}

void test_CanIf_CheckTrcvWakeFlagIndication_InvalidTrcv_ShouldNotNotifyCanSM(void) {
    CanIf_Init(&testConfig);
    CanIf_CheckTrcvWakeFlagIndication(CANIF_NUM_TRANSCEIVERS);
    TEST_ASSERT_EQUAL(0, mock_CanSM_CheckWufCalls);
}

void test_CanIf_ClearTrcvWufFlagIndication_BeforeInit_ShouldReturnSilently(void) {
    CanIf_ClearTrcvWufFlagIndication(0U);
    TEST_ASSERT_EQUAL(0, mock_CanSM_ClearWufCalls);
}

void test_CanIf_ClearTrcvWufFlagIndication_InvalidTrcv_ShouldNotNotifyCanSM(void) {
    CanIf_Init(&testConfig);
    CanIf_ClearTrcvWufFlagIndication(CANIF_NUM_TRANSCEIVERS);
    TEST_ASSERT_EQUAL(0, mock_CanSM_ClearWufCalls);
}

void test_CanIf_TrcvModeIndication_BeforeInit_ShouldReturnSilently(void) {
    CanIf_TrcvModeIndication(0U, CANIF_TRCV_MODE_NORMAL);
    TEST_ASSERT_EQUAL(0, mock_CanSM_TrcvModeCalls);
}

void test_CanIf_TrcvModeIndication_InvalidTrcv_ShouldNotNotifyCanSM(void) {
    CanIf_Init(&testConfig);
    CanIf_TrcvModeIndication(CANIF_NUM_TRANSCEIVERS, CANIF_TRCV_MODE_NORMAL);
    TEST_ASSERT_EQUAL(0, mock_CanSM_TrcvModeCalls);
}

void test_CanIf_ControllerModeIndication_BeforeInit_ShouldReturnSilently(void) {
    CanIf_ControllerModeIndication(0U, CANIF_CS_STARTED);
    TEST_ASSERT_EQUAL(0, mock_CanSM_CtrlModeCalls);
}

void test_CanIf_ControllerModeIndication_InvalidController_ShouldNotNotifyCanSM(void) {
    CanIf_Init(&testConfig);
    CanIf_ControllerModeIndication(CANIF_NUM_CONTROLLERS, CANIF_CS_STARTED);
    TEST_ASSERT_EQUAL(0, mock_CanSM_CtrlModeCalls);
}

/* ── Coverage gaps: SetControllerMode Can driver error paths ── */

void test_CanIf_SetControllerMode_Start_CanDriverFails_ShouldKeepPrevious(void) {
    CanIf_Init(&testConfig);
    /* Default is STOPPED; driver rejects STARTED → mode stays STOPPED */
    mock_CanSetCtrlModeReturn = CAN_NOT_OK;
    CanIf_ControllerModeType mode = CANIF_CS_STARTED;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetControllerMode(0U, CANIF_CS_STARTED));
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_CS_STOPPED, mode);
}

void test_CanIf_SetControllerMode_Stop_CanDriverFails_ShouldKeepPrevious(void) {
    CanIf_Init(&testConfig);
    /* First successfully start */
    mock_CanSetCtrlModeReturn = CAN_OK;
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    mock_CanSetCtrlModeReturn = CAN_NOT_OK;
    CanIf_ControllerModeType mode = CANIF_CS_STOPPED;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetControllerMode(0U, CANIF_CS_STOPPED));
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_CS_STARTED, mode); /* unchanged */
}

void test_CanIf_SetControllerMode_Sleep_CanDriverFails_ShouldKeepPrevious(void) {
    CanIf_Init(&testConfig);
    mock_CanSetCtrlModeReturn = CAN_NOT_OK;
    CanIf_ControllerModeType mode = CANIF_CS_STARTED;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_SetControllerMode(0U, CANIF_CS_SLEEP));
    TEST_ASSERT_EQUAL(E_OK, CanIf_GetControllerMode(0U, &mode));
    TEST_ASSERT_EQUAL(CANIF_CS_STOPPED, mode);
}

/* ── Coverage gap: CheckWakeup all controllers fail ── */

void test_CanIf_CheckWakeup_AllControllersFail_ShouldReturnNotOk(void) {
    CanIf_Init(&testConfig);
    mock_CanCheckWakeupReturn = E_NOT_OK;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_CheckWakeup(0U));
}

/* ── Coverage gaps: CacheTxData else branch + TxQueueEnqueue NULL data ── */

void test_CanIf_Transmit_ZeroLength_ShouldInvalidateTriggerCache(void) {
    PduInfoType pdu;
    uint8 data[8] = {0x11U};
    pdu.SduDataPtr = data; pdu.SduLength = 0U; pdu.MetaDataPtr = NULL_PTR;

    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);

    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu));
    /* TriggerTransmit must fail because cache is invalidated (cacheLen == 0
     * leads to the else branch in CanIf_CacheTxData). */
    PduInfoType trig;
    uint8 buf[8] = {0U};
    trig.SduDataPtr = buf; trig.SduLength = 8U; trig.MetaDataPtr = NULL_PTR;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_TriggerTransmit(0U, &trig));
}

void test_CanIf_Transmit_CanBusyNullData_ShouldFailQueueing(void) {
    PduInfoType pdu;
    pdu.SduDataPtr = NULL_PTR; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    CanIf_Init(&testConfig);
    (void)CanIf_SetControllerMode(0U, CANIF_CS_STARTED);
    (void)CanIf_SetPduMode(0U, CANIF_ONLINE);

    mock_CanWriteReturn = CAN_BUSY;
    /* CanIf_TxQueueEnqueue: (SduDataPtr==NULL && SduLength>0) → E_NOT_OK */
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_Transmit(0U, &pdu));
}

/* ── Coverage gap: GetTxConfirmationState BeforeInit ── */

void test_CanIf_GetTxConfirmationState_BeforeInit_ShouldFail(void) {
    /* setUp 调了 DeInit → DriverInitialized == FALSE */
    CanIf_TxConfirmationStateType st = CanIf_GetTxConfirmationState(0U);
    TEST_ASSERT_EQUAL(CANIF_TXCONF_NONE, st);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* ── Coverage gap: ClearTrcvWufFlag Invalid transceiver ── */

void test_CanIf_ClearTrcvWufFlag_InvalidTrcv_ShouldReportDet(void) {
    CanIf_Init(&testConfig);
    mock_DetCalls = 0;
    TEST_ASSERT_EQUAL(E_NOT_OK, CanIf_ClearTrcvWufFlag(CANIF_NUM_TRANSCEIVERS));
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/* ── Coverage gap: TxQueueMainFunction drain on CAN_BUSY (break path) ── */

void test_CanIf_TxQueueMainFunction_Busy_DoesNotDrain(void) {
    PduInfoType pdu;
    uint8 d0[8] = {0x11U};
    pdu.SduDataPtr = d0; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    startOnlineTx();

    mock_CanWriteReturn = CAN_BUSY;
    TEST_ASSERT_EQUAL(E_OK, CanIf_Transmit(0U, &pdu)); /* queued */
    TEST_ASSERT_EQUAL(1, mock_CanWriteCalls);

    /* MainFunction but CAN_BUSY → break, queue stays full */
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(2, mock_CanWriteCalls); /* one retry, still BUSY → break */

    /* Final drain */
    mock_CanWriteReturn = CAN_OK;
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(3, mock_CanWriteCalls); /* successful retry */

    /* Queue drained: further MainFunction should not call Can_Write */
    CanIf_TxQueueMainFunction();
    TEST_ASSERT_EQUAL(3, mock_CanWriteCalls);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanIf_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanIf_Transmit_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_GetControllerErrorState_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_ReadTxNotifStatus_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_ReadRxNotifStatus_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_TriggerTransmit_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_ConfirmPnAvailability_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_CheckTrcvWakeFlag_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_ClearTrcvWufFlag_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_SetPnWakeupFilter_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_TxQueueMainFunction_BeforeInit_ShouldReportDet);
    RUN_TEST(test_CanIf_GetTxConfirmationState_BeforeInit_ShouldFail);
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
    RUN_TEST(test_CanIf_SetTrcvMode_Standby_AfterInit_ShouldSucceed);
    RUN_TEST(test_CanIf_SetTrcvMode_BeforeInit_ShouldFail);
    RUN_TEST(test_CanIf_SetTrcvMode_InvalidTrcvId_ShouldReportDet);
    RUN_TEST(test_CanIf_SetTrcvMode_TrcvError_ShouldKeepPreviousMode);
    RUN_TEST(test_CanIf_CheckValidation_ShouldSucceedOncePerWakeup);
    RUN_TEST(test_CanIf_GetTxConfirmationState_AfterTransmitAndConfirm);
    RUN_TEST(test_CanIf_GetTxConfirmationState_InvalidPduId_ShouldReportDet);
    RUN_TEST(test_CanIf_GetControllerErrorState_ShouldDelegateToCan);
    RUN_TEST(test_CanIf_GetControllerRxErrorCounter_ShouldDelegateToCan);
    RUN_TEST(test_CanIf_GetControllerTxErrorCounter_ShouldDelegateToCan);
    RUN_TEST(test_CanIf_GetControllerErrorState_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanIf_GetControllerRxErrorCounter_InvalidController_ShouldReportDet);
    RUN_TEST(test_CanIf_ReadTxNotifStatus_DisabledApi_ShouldReportDet);
    RUN_TEST(test_CanIf_ReadTxNotifStatus_ShouldSetAndClearOnRead);
    RUN_TEST(test_CanIf_ReadRxNotifStatus_DisabledApi_ShouldReportDet);
    RUN_TEST(test_CanIf_ReadRxNotifStatus_ShouldSetAndClearOnRead);
    RUN_TEST(test_CanIf_TriggerTransmit_NotConfiguredPdu_ShouldFail);
    RUN_TEST(test_CanIf_TriggerTransmit_NoCachedData_ShouldFail);
    RUN_TEST(test_CanIf_TriggerTransmit_ShouldFillCachedDataWithMinLength);
    RUN_TEST(test_CanIf_TriggerTransmit_ShouldTruncateToCachedLength);
    RUN_TEST(test_CanIf_TriggerTransmit_NullBuffer_ShouldReportDet);
    RUN_TEST(test_CanIf_ConfirmPnAvailability_ValidTrcv_ShouldSucceed);
    RUN_TEST(test_CanIf_ConfirmPnAvailability_InvalidTrcv_ShouldReportDet);
    RUN_TEST(test_CanIf_CheckTrcvWakeFlag_ValidTrcv_ShouldSucceed);
    RUN_TEST(test_CanIf_CheckTrcvWakeFlag_InvalidTrcv_ShouldReportDet);
    RUN_TEST(test_CanIf_ClearTrcvWufFlag_ValidTrcv_ShouldSucceed);
    RUN_TEST(test_CanIf_CheckTrcvWakeFlagIndication_ShouldNotifyCanSM);
    RUN_TEST(test_CanIf_ClearTrcvWufFlagIndication_ShouldNotifyCanSM);
    RUN_TEST(test_CanIf_TrcvModeIndication_ShouldUpdateAndNotifyCanSM);
    RUN_TEST(test_CanIf_ControllerModeIndication_ShouldNotifyCanSM);
    RUN_TEST(test_CanIf_CanIdToMetaData_ExtendedId_ShouldPackBigEndian);
    RUN_TEST(test_CanIf_CanIdToMetaData_StandardId_ShouldPackBigEndian);
    RUN_TEST(test_CanIf_CanIdToMetaData_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanIf_MetaDataToCanId_NullPtr_ShouldReportDet);
    RUN_TEST(test_CanIf_MetaData_RoundTrip_ShouldPreserveCanId);
        RUN_TEST(test_CanIf_SetPnWakeupFilter_ValidRange_ShouldSucceed);
        RUN_TEST(test_CanIf_SetPnWakeupFilter_InvalidTrcv_ShouldReportDet);
        RUN_TEST(test_CanIf_SetPnWakeupFilter_NullPtr_ShouldReportDet);
        RUN_TEST(test_CanIf_SetPnWakeupFilter_InvertedRange_ShouldReportDet);
        RUN_TEST(test_CanIf_RxIndication_PnFilter_MatchingId_ShouldPass);
        RUN_TEST(test_CanIf_RxIndication_PnFilter_NonMatchingId_ShouldDrop);
        RUN_TEST(test_CanIf_RxIndication_PnFilter_MaskAppliedToRangeAndFrame);
        RUN_TEST(test_CanIf_RxIndication_NoPnFilter_ShouldPassAll);
        RUN_TEST(test_CanIf_DeInit_ShouldClearPnFilter);
        RUN_TEST(test_CanIf_RxLookup_DispatchesEachHohBucket);
        RUN_TEST(test_CanIf_RxIndication_UnknownCanId_ShouldDrop);
        RUN_TEST(test_CanIf_RxIndication_UnknownHoh_ShouldDrop);
        RUN_TEST(test_CanIf_RxLookup_Equivalence_WithLinearScan);
        RUN_TEST(test_CanIf_Transmit_CanBusy_ShouldEnqueueForRetry);
        RUN_TEST(test_CanIf_TxQueue_ShouldDrainInFifoOrder);
        RUN_TEST(test_CanIf_TxQueue_Full_ShouldReturnNotOk);
        RUN_TEST(test_CanIf_Transmit_OversizedFrame_ShouldNotBeBuffered);
        RUN_TEST(test_CanIf_ControllerBusOff_ShouldFlushTxQueue);
        RUN_TEST(test_CanIf_DeInit_ShouldFlushTxQueue);

        /* Coverage expansion tests */
        RUN_TEST(test_CanIf_SetControllerMode_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_SetControllerMode_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_SetControllerMode_Stopped_ShouldSucceed);
        RUN_TEST(test_CanIf_SetControllerMode_Sleep_ShouldSucceed);
        RUN_TEST(test_CanIf_SetControllerMode_InvalidMode_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerMode_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerMode_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerMode_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_Transmit_InvalidTxPduId_ShouldFail);
        RUN_TEST(test_CanIf_Transmit_ControllerNotStarted_ShouldFail);
        RUN_TEST(test_CanIf_Transmit_PduModeOffline_ShouldFail);
        RUN_TEST(test_CanIf_Transmit_CanNotOk_ShouldReturnNotOk);
        RUN_TEST(test_CanIf_CancelTransmit_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_CancelTransmit_InvalidTxPduId_ShouldFail);
        RUN_TEST(test_CanIf_SetPduMode_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_SetPduMode_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_GetPduMode_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetPduMode_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_GetPduMode_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_SetDynamicTxId_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_SetDynamicTxId_InvalidTxPduId_ShouldFail);
        RUN_TEST(test_CanIf_SetDynamicTxId_Valid_ShouldSucceed);
        RUN_TEST(test_CanIf_SetTrcvMode_Normal_ShouldSucceed);
        RUN_TEST(test_CanIf_SetTrcvMode_Sleep_ShouldSucceed);
        RUN_TEST(test_CanIf_SetTrcvMode_InvalidMode_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvMode_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvMode_InvalidTrcv_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvMode_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvWakeupReason_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvWakeupReason_InvalidTrcv_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvWakeupReason_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_GetTrcvWakeupReason_Valid_ShouldReturnNotSupported);
        RUN_TEST(test_CanIf_SetTrcvWakeupMode_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_SetTrcvWakeupMode_InvalidTrcv_ShouldFail);
        RUN_TEST(test_CanIf_SetTrcvWakeupMode_Valid_ShouldSucceed);
        RUN_TEST(test_CanIf_SetBaudrate_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_SetBaudrate_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_SetBaudrate_Valid_ShouldSucceed);
        RUN_TEST(test_CanIf_GetBaudrate_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetBaudrate_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_GetBaudrate_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_GetBaudrate_Valid_ShouldReturnDefault);
        RUN_TEST(test_CanIf_DeInit_BeforeInit_ShouldReportDet);
        RUN_TEST(test_CanIf_CheckWakeup_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_CheckValidation_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerErrorState_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerRxErrorCounter_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerRxErrorCounter_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerTxErrorCounter_BeforeInit_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerTxErrorCounter_InvalidController_ShouldFail);
        RUN_TEST(test_CanIf_GetControllerTxErrorCounter_NullPtr_ShouldFail);
        RUN_TEST(test_CanIf_ReadTxNotifStatus_InvalidTxPduId_ShouldFail);
        RUN_TEST(test_CanIf_ReadRxNotifStatus_InvalidRxPduId_ShouldFail);
        RUN_TEST(test_CanIf_TriggerTransmit_InvalidTxPduId_ShouldFail);
        RUN_TEST(test_CanIf_TxConfirmation_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_RxIndication_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_ControllerBusOff_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_CheckTrcvWakeFlagIndication_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_CheckTrcvWakeFlagIndication_InvalidTrcv_ShouldNotNotifyCanSM);
        RUN_TEST(test_CanIf_ClearTrcvWufFlagIndication_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_ClearTrcvWufFlagIndication_InvalidTrcv_ShouldNotNotifyCanSM);
        RUN_TEST(test_CanIf_TrcvModeIndication_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_TrcvModeIndication_InvalidTrcv_ShouldNotNotifyCanSM);
        RUN_TEST(test_CanIf_ControllerModeIndication_BeforeInit_ShouldReturnSilently);
        RUN_TEST(test_CanIf_ControllerModeIndication_InvalidController_ShouldNotNotifyCanSM);

        /* ── NEW: coverage expansion for uncovered branches ── */
        RUN_TEST(test_CanIf_SetControllerMode_Start_CanDriverFails_ShouldKeepPrevious);
        RUN_TEST(test_CanIf_SetControllerMode_Stop_CanDriverFails_ShouldKeepPrevious);
        RUN_TEST(test_CanIf_SetControllerMode_Sleep_CanDriverFails_ShouldKeepPrevious);
        RUN_TEST(test_CanIf_CheckWakeup_AllControllersFail_ShouldReturnNotOk);
        RUN_TEST(test_CanIf_Transmit_ZeroLength_ShouldInvalidateTriggerCache);
        RUN_TEST(test_CanIf_Transmit_CanBusyNullData_ShouldFailQueueing);
        RUN_TEST(test_CanIf_ClearTrcvWufFlag_InvalidTrcv_ShouldReportDet);
        RUN_TEST(test_CanIf_TxQueueMainFunction_Busy_DoesNotDrain);

    return UnityEnd();
}
