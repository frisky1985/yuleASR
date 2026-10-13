/**
 * @file test_linif.c
 * @brief LinIf (LIN Interface) Unit Tests
 * @version 2.0.0
 * @date 2026-09-26
 */

// @tests src/bsw/ecual/linif/src/LinIf.c  @tests src/bsw/ecual/linif/src/LinIf_Lcfg.c  @tests src/bsw/ecual/linif/include/LinIf.h

#include "unity.h"
#include "LinIf.h"
#include "Lin.h"
#include "LinTrcv.h"
#include "Dio.h"
#include <string.h>

/* Det SID/error literals still private to LinIf.c (not exported in the
 * header). LINIF_E_PARAM_POINTER / LINIF_E_PARAM_CHANNEL / LINIF_E_PARAM_VALUE
 * and the 0x20-0x29 SID block come from LinIf.h. */
#define LINIF_SID_INIT          (0x00U)
#define LINIF_SID_TRANSMIT      (0x02U)
#define LINIF_SID_RX_INDICATION (0x03U)
#define LINIF_SID_SCHEDULE      (0x05U)
#define LINIF_SID_WAKEUP        (0x09U)
#define LINIF_SID_GOTOSLEEP     (0x0AU)
#define LINIF_SID_SCHEDREQ      (0x0BU)
#define LINIF_SID_RX_IND        (0x03U)
#define LINIF_SID_MAINFUNC      (0x04U)
#define LINIF_E_UNINIT          (0x20U)
#define LINIF_E_PARAM_PDU       (0x30U)
#define LINIF_E_PARAM_SCHEDULE  (0x40U)

/* ---------- Det mock ---------- */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---------- MCAL Lin mock ---------- */
#define MOCK_LOG_DEPTH (8U)

static uint8 mock_SendCount = 0U;
static Lin_ChannelType mock_SendChannel[MOCK_LOG_DEPTH];
static Lin_PduType mock_SendPdu[MOCK_LOG_DEPTH];
static uint8 mock_SendData[MOCK_LOG_DEPTH][LINIF_MAX_FRAME_LENGTH];
static uint8 mock_GoToSleepCount = 0U;
static uint8 mock_WakeUpCount = 0U;
static Std_ReturnType mock_LinRetVal = E_OK;

Std_ReturnType Lin_SendFrame(Lin_ChannelType Channel, const Lin_PduType* PduInfoPtr) {
    if (mock_SendCount < MOCK_LOG_DEPTH) {
        mock_SendChannel[mock_SendCount] = Channel;
        mock_SendPdu[mock_SendCount] = *PduInfoPtr;
        (void)memset(mock_SendData[mock_SendCount], 0, LINIF_MAX_FRAME_LENGTH);
        if (PduInfoPtr->SduPtr != NULL_PTR) {
            (void)memcpy(mock_SendData[mock_SendCount], PduInfoPtr->SduPtr, PduInfoPtr->Length);
        }
    }
    mock_SendCount++;
    return mock_LinRetVal;
}

Std_ReturnType Lin_WakeUp(Lin_ChannelType Channel) {
    (void)Channel;
    mock_WakeUpCount++;
    return mock_LinRetVal;
}

Std_ReturnType Lin_GoToSleep(Lin_ChannelType Channel) {
    (void)Channel;
    mock_GoToSleepCount++;
    return mock_LinRetVal;
}

/* ---------- LinTrcv driver support: host-safe DIO + EcuM mocks ----------
 * The real LinTrcv driver (ecual_lintrcv library) is linked into this test;
 * its external dependencies are satisfied here so the delegation paths run
 * against production code. */
static Dio_LevelType mock_DioLevel[MOCK_LOG_DEPTH];

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId) {
    return (ChannelId < MOCK_LOG_DEPTH) ? mock_DioLevel[ChannelId] : STD_LOW;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level) {
    if (ChannelId < MOCK_LOG_DEPTH) {
        mock_DioLevel[ChannelId] = Level;
    }
}

static uint32 mock_EcuM_WakeupEventCount = 0U;
static uint32 mock_EcuM_LastWakeupSource = 0U;

void EcuM_SetWakeupEvent(uint32 wakeupSource) {
    mock_EcuM_WakeupEventCount++;
    mock_EcuM_LastWakeupSource = wakeupSource;
}

/* One TJA1021 channel on DIO pin 0 (EN), 1 (NWake), 2 (NERR); all transition
 * delays zero so mode changes do not busy-wait on the host. */
static const LinTrcv_ChannelConfigType mock_TrcvChannelCfg = {
    0U,                    /* ChannelId */
    LINTRCV_TJA1021,       /* HwType */
    LINTRCV_CTRL_DIO,      /* CtrlIf */
    0U,                    /* EnPinDio */
    0xFFFFU,               /* TxDPinDio (not managed) */
    1U,                    /* NwadrsPinDio */
    2U,                    /* NerrPinDio */
    TRUE,                  /* WakeupByBusEnabled */
    TRUE,                  /* WakeupByPinEnabled */
    0U,                    /* WakeupSourceRef (0 => no EcuM notification) */
    0U,                    /* SpiChannel */
    0U,                    /* SpiDevice */
    0U, 0U, 0U, 0U,        /* mode transition delays */
    LINTRCV_OPMODE_NORMAL  /* InitialMode */
};

static const LinTrcv_ConfigType mock_TrcvConfig = {
    1U,
    &mock_TrcvChannelCfg,
    TRUE,
    TRUE,
    TRUE
};

static void linifTest_InitLinTrcv(void) {
    LinTrcv_Init(&mock_TrcvConfig);
}

/* ---------- LinIf test config holding a diagnostic frame ---------- */
static const LinIf_FrameConfigType mock_DiagFrames[1] = {
    { 0U, 0x3CU, 8U, LINIF_DIAGNOSTIC_FRAME, TRUE }
};

static const LinIf_ChannelConfigType mock_DiagChannel = {
    0U, 1U, 0U, mock_DiagFrames, NULL_PTR
};

static const LinIf_ConfigType mock_DiagConfig = {
    1U, &mock_DiagChannel, 0U, NULL_PTR
};

/* ---------- Upper layer hook mocks (override LinIf weak defaults) ---------- */
static uint8 mock_TxConfCount = 0U;
static uint8 mock_TxConfChannel = 0xFFU;
static uint8 mock_TxConfPduId = 0xFFU;
static uint8 mock_SchedConfCount = 0U;
static uint8 mock_SchedConfChannel = 0xFFU;
static uint8 mock_SchedConfSchedule = 0xFFU;
static uint8 mock_RxCallbackCount = 0U;
static LinIf_PduType mock_RxCallbackPdu;

void LinIf_TxConfirmation(uint8 Channel, uint8 LinTxPduId) {
    mock_TxConfCount++;
    mock_TxConfChannel = Channel;
    mock_TxConfPduId = LinTxPduId;
}

void LinIf_ScheduleRequestConfirmation(uint8 Channel, uint8 ScheduleIndex) {
    mock_SchedConfCount++;
    mock_SchedConfChannel = Channel;
    mock_SchedConfSchedule = ScheduleIndex;
}

void LinIf_RxCallback(uint8 Channel, const LinIf_PduType* PduInfoPtr) {
    (void)Channel;
    mock_RxCallbackCount++;
    mock_RxCallbackPdu = *PduInfoPtr;
}

static void mock_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
    mock_SendCount = 0U;
    mock_GoToSleepCount = 0U;
    mock_WakeUpCount = 0U;
    mock_LinRetVal = E_OK;
    mock_TxConfCount = 0U;
    mock_TxConfChannel = 0xFFU;
    mock_TxConfPduId = 0xFFU;
    mock_SchedConfCount = 0U;
    mock_SchedConfChannel = 0xFFU;
    mock_SchedConfSchedule = 0xFFU;
    mock_RxCallbackCount = 0U;
    mock_EcuM_WakeupEventCount = 0U;
    mock_EcuM_LastWakeupSource = 0U;
    (void)memset(&mock_RxCallbackPdu, 0, sizeof(mock_RxCallbackPdu));
    (void)memset(mock_SendPdu, 0, sizeof(mock_SendPdu));
    (void)memset(mock_SendChannel, 0, sizeof(mock_SendChannel));
    (void)memset(mock_SendData, 0, sizeof(mock_SendData));
    (void)memset(mock_DioLevel, 0, sizeof(mock_DioLevel));
}

static void linif_RunTicks(uint16 ticks) {
    uint16 i;
    for (i = 0U; i < ticks; i++) {
        LinIf_MainFunction();
    }
}

void setUp(void) { mock_Reset(); }

void tearDown(void) {
}

/* NOTE: LinIf keeps static state across tests and the runner executes in
 * declaration order, so uninitialized-behavior tests come first. */

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_NullPtr_ShouldReportDet(void) {
    LinIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LINIF_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_BeforeInit_ShouldReportUninit(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;
    Std_ReturnType ret = LinIf_Transmit(0U, &pdu);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(LINIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_SetSchedule_BeforeInit_ShouldReportUninit(void) {
    Std_ReturnType ret = LinIf_SetSchedule(LINIF_Normal);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(LINIF_SID_SCHEDULE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_MainFunction_BeforeInit_ShouldReturnSilently(void) {
    LinIf_MainFunction(); /* must not crash, must not report */
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(0U, mock_SendCount);
}

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_ValidConfig_ShouldActivateModule(void) {
    PduInfoType pdu;
    uint8 data[8] = {0xAAU, 0xBBU, 0xCCU, 0xDDU, 0U, 0U, 0U, 0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    /* Module must be operational after Init: Transmit accepted */
    TEST_ASSERT_EQUAL(E_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00001 */
void test_LinIf_Init_DoubleInit_ShouldStayOperational(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_NullPdu_ShouldReportDet(void) {
    LinIf_Init(&LinIf_Config);
    Std_ReturnType ret = LinIf_Transmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL(LINIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_OutOfRangePduId_ShouldReportParamPdu(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    /* LinIf_Config maps only TxPduId 0 and 1 */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_Transmit(2U, &pdu));
    TEST_ASSERT_EQUAL(LINIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_PDU, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_DlcMismatch_ShouldReportParamPdu(void) {
    PduInfoType pdu;
    uint8 data[4] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 4U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    /* Frame 0 is configured with Dlc 8 */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_PDU, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_SetSchedule_ValidSchedule_ShouldSucceed(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetSchedule(LINIF_Normal));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00004 */
void test_LinIf_SetSchedule_InvalidSchedule_ShouldReportParamSchedule(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetSchedule(0x7FU));
    TEST_ASSERT_EQUAL(LINIF_SID_SCHEDULE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_SCHEDULE, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_MainFunction_ShouldSendFramesAtConfiguredDelays(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);

    /* Normal schedule: entry0 @5ms (0x3C), entry1 @10ms (0x3D), entry2 @10ms (0x3E) */
    linif_RunTicks(4U);
    TEST_ASSERT_EQUAL(0U, mock_SendCount);

    linif_RunTicks(1U); /* tick 5 */
    TEST_ASSERT_EQUAL(1U, mock_SendCount);
    TEST_ASSERT_EQUAL(0U, mock_SendChannel[0]);
    TEST_ASSERT_EQUAL(0x3CU, mock_SendPdu[0].Pid);
    TEST_ASSERT_EQUAL(8U, mock_SendPdu[0].Length);
    TEST_ASSERT_EQUAL(LIN_MASTER_RESPONSE, mock_SendPdu[0].FrameResponse);
    TEST_ASSERT_EQUAL(LIN_CLASSIC_CS, mock_SendPdu[0].ChecksumType);

    linif_RunTicks(10U); /* tick 15 */
    TEST_ASSERT_EQUAL(2U, mock_SendCount);
    TEST_ASSERT_EQUAL(0x3DU, mock_SendPdu[1].Pid);
    TEST_ASSERT_EQUAL(8U, mock_SendPdu[1].Length);
    TEST_ASSERT_EQUAL(LIN_SLAVE_RESPONSE, mock_SendPdu[1].FrameResponse);
    TEST_ASSERT_EQUAL(LIN_CLASSIC_CS, mock_SendPdu[1].ChecksumType); /* 0x3D = diagnostic PID */
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00006 */
void test_LinIf_MainFunction_ShouldCycleScheduleEntries(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);

    linif_RunTicks(25U); /* ticks 5 / 15 / 25 */
    TEST_ASSERT_EQUAL(3U, mock_SendCount);
    TEST_ASSERT_EQUAL(0x3EU, mock_SendPdu[2].Pid);
    TEST_ASSERT_EQUAL(4U, mock_SendPdu[2].Length);
    TEST_ASSERT_EQUAL(LIN_FRAMETYPE_EVENT_TRIGGERED, mock_SendPdu[2].FrameType);
    TEST_ASSERT_EQUAL(LIN_MASTER_RESPONSE, mock_SendPdu[2].FrameResponse);
    TEST_ASSERT_EQUAL(LIN_ENHANCED_CS, mock_SendPdu[2].ChecksumType);

    linif_RunTicks(5U); /* tick 30: back to entry 0 -> 0x3C again */
    TEST_ASSERT_EQUAL(4U, mock_SendCount);
    TEST_ASSERT_EQUAL(0x3CU, mock_SendPdu[3].Pid);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_ShouldPublishBufferAtNextSchedulePoint(void) {
    PduInfoType pdu;
    uint8 data[8] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);
    TEST_ASSERT_EQUAL(E_OK, LinIf_Transmit(0U, &pdu));

    linif_RunTicks(5U);
    TEST_ASSERT_EQUAL(1U, mock_SendCount);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, mock_SendData[0], 8U);
    TEST_ASSERT_EQUAL(1U, mock_TxConfCount);
    TEST_ASSERT_EQUAL(0U, mock_TxConfChannel);
    TEST_ASSERT_EQUAL(0U, mock_TxConfPduId);
}

/** @req SWS_LinIf_00003 */
void test_LinIf_Transmit_EventTriggeredPdu_ShouldKeepConfiguredPid(void) {
    PduInfoType pdu;
    uint8 data[4] = {0xA1U, 0xB2U, 0xC3U, 0xD4U};
    pdu.SduDataPtr = data; pdu.SduLength = 4U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);
    TEST_ASSERT_EQUAL(E_OK, LinIf_Transmit(1U, &pdu));

    linif_RunTicks(25U); /* tick 25 sends the event triggered frame */
    TEST_ASSERT_EQUAL(3U, mock_SendCount);
    TEST_ASSERT_EQUAL(0x3EU, mock_SendPdu[2].Pid);
    TEST_ASSERT_EQUAL(4U, mock_SendPdu[2].Length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, mock_SendData[2], 4U);
    TEST_ASSERT_EQUAL(1U, mock_TxConfCount);
    TEST_ASSERT_EQUAL(1U, mock_TxConfPduId);
}

/** @req SWS_LinIf_00008 */
void test_LinIf_ScheduleRequest_ValidSchedule_ShouldSwitchAndConfirm(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);
    linif_RunTicks(5U);
    TEST_ASSERT_EQUAL(1U, mock_SendCount);

    TEST_ASSERT_EQUAL(E_OK, LinIf_ScheduleRequest(0U, LINIF_SCHEDULE_DIAG_REQUEST));
    TEST_ASSERT_EQUAL(0U, mock_SchedConfCount);

    LinIf_MainFunction(); /* switch takes effect at next tick, restart at entry 0 */
    TEST_ASSERT_EQUAL(1U, mock_SchedConfCount);
    TEST_ASSERT_EQUAL(0U, mock_SchedConfChannel);
    TEST_ASSERT_EQUAL(LINIF_SCHEDULE_DIAG_REQUEST, mock_SchedConfSchedule);

    linif_RunTicks(19U);
    TEST_ASSERT_EQUAL(1U, mock_SendCount); /* diag entry is 20 ms away */

    linif_RunTicks(1U); /* tick 20 of the diag schedule */
    TEST_ASSERT_EQUAL(2U, mock_SendCount);
    TEST_ASSERT_EQUAL(0x3CU, mock_SendPdu[1].Pid);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00008 */
void test_LinIf_ScheduleRequest_InvalidSchedule_ShouldReportParamSchedule(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_ScheduleRequest(0U, 0x7FU));
    TEST_ASSERT_EQUAL(LINIF_SID_SCHEDREQ, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_SCHEDULE, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00008 */
void test_LinIf_ScheduleRequest_InvalidChannel_ShouldReportParamChannel(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_ScheduleRequest(3U, LINIF_Normal));
    TEST_ASSERT_EQUAL(LINIF_SID_SCHEDREQ, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00009 @req SWS_LinIf_00010 */
void test_LinIf_GotoSleep_ShouldStopScheduleAndWakeUpShouldResume(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);

    TEST_ASSERT_EQUAL(E_OK, LinIf_GotoSleep(0U));
    TEST_ASSERT_EQUAL(1U, mock_GoToSleepCount);

    linif_RunTicks(10U);
    TEST_ASSERT_EQUAL(0U, mock_SendCount); /* channel asleep: no bus traffic */

    TEST_ASSERT_EQUAL(E_OK, LinIf_WakeUp(0U));
    TEST_ASSERT_EQUAL(1U, mock_WakeUpCount);

    linif_RunTicks(5U);
    TEST_ASSERT_EQUAL(1U, mock_SendCount);
    TEST_ASSERT_EQUAL(0x3CU, mock_SendPdu[0].Pid);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00009 */
void test_LinIf_WakeUp_InvalidChannel_ShouldReportParamChannel(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_WakeUp(5U));
    TEST_ASSERT_EQUAL(LINIF_SID_WAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00009 @req SWS_LinIf_00010 */
void test_LinIf_WakeUp_GotoSleep_BeforeInit_ShouldReportUninit(void) {
    LinIf_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_WakeUp(0U));
    TEST_ASSERT_EQUAL(LINIF_SID_WAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GotoSleep(0U));
    TEST_ASSERT_EQUAL(LINIF_SID_GOTOSLEEP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00005 */
void test_LinIf_RxIndication_ShouldDispatchToHook(void) {
    LinIf_PduType rxPdu;
    uint8 data[8] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    rxPdu.Id = 0x3DU; rxPdu.Dlc = 8U; rxPdu.DataPtr = data;

    LinIf_Init(&LinIf_Config);
    LinIf_RxIndication(0U, &rxPdu);
    TEST_ASSERT_EQUAL(1U, mock_RxCallbackCount);
    TEST_ASSERT_EQUAL(0x3DU, mock_RxCallbackPdu.Id);

    /* Unknown PID must not reach the upper layer */
    rxPdu.Id = 0x55U;
    LinIf_RxIndication(0U, &rxPdu);
    TEST_ASSERT_EQUAL(1U, mock_RxCallbackCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00005 */
void test_LinIf_RxIndication_NullPtr_ShouldReportDet(void) {
    LinIf_Init(&LinIf_Config);
    LinIf_RxIndication(0U, NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LINIF_SID_RX_INDICATION, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00002 */
void test_LinIf_DeInit_AfterInit_ShouldDeactivateModule(void) {
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    LinIf_DeInit();

    /* After DeInit the module is uninitialized again: Transmit must fail */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    /* ... and the scheduler must not drive the bus any more */
    linif_RunTicks(10U);
    TEST_ASSERT_EQUAL(0U, mock_SendCount);
}

/** @req SWS_LinIf_00007 */
void test_LinIf_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    LinIf_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(LINIF_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(LINIF_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(LINIF_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(LINIF_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(LINIF_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_LinIf_00007 */
void test_LinIf_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    LinIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00679 @req SWS_LinIf_00680 */
void test_LinIf_SetGetTrcvMode_ShouldDelegateToLinTrcv(void) {
    LinIf_TrcvModeType mode = LINIF_TRCV_MODE_NORMAL;

    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();

    TEST_ASSERT_EQUAL(E_OK, LinIf_SetTrcvMode(0U, LINIF_TRCV_MODE_SLEEP));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINIF_TRCV_MODE_SLEEP, mode);

    TEST_ASSERT_EQUAL(E_OK, LinIf_SetTrcvMode(0U, LINIF_TRCV_MODE_STANDBY));
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINIF_TRCV_MODE_STANDBY, mode);

    TEST_ASSERT_EQUAL(E_OK, LinIf_SetTrcvMode(0U, LINIF_TRCV_MODE_NORMAL));
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINIF_TRCV_MODE_NORMAL, mode);

    /* The mode must be visible on the real driver as well */
    {
        LinTrcv_OpmodeType trcvMode = LINTRCV_OPMODE_SLEEP;
        TEST_ASSERT_EQUAL(E_OK, LinTrcv_GetOpMode(0U, &trcvMode));
        TEST_ASSERT_EQUAL(LINTRCV_OPMODE_NORMAL, trcvMode);
    }
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00679 */
void test_LinIf_SetTrcvMode_InvalidChannel_ShouldReportParamChannel(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetTrcvMode(9U, LINIF_TRCV_MODE_NORMAL));
    TEST_ASSERT_EQUAL(LINIF_SID_SETTRCVMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00679 */
void test_LinIf_SetTrcvMode_InvalidMode_ShouldReportParamValue(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetTrcvMode(0U, (LinIf_TrcvModeType)0x7FU));
    TEST_ASSERT_EQUAL(LINIF_SID_SETTRCVMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_VALUE, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00680 */
void test_LinIf_GetTrcvMode_NullPtr_ShouldReportParamPointer(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetTrcvMode(0U, NULL_PTR));
    TEST_ASSERT_EQUAL(LINIF_SID_GETTRCVMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00681 */
void test_LinIf_SetTrcvWakeupMode_ShouldAcceptValidModes(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();

    TEST_ASSERT_EQUAL(E_OK, LinIf_SetTrcvWakeupMode(0U, LINIF_TRCV_WU_ENABLE));
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetTrcvWakeupMode(0U, LINIF_TRCV_WU_DISABLE));
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetTrcvWakeupMode(0U, LINIF_TRCV_WU_CLEAR));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00681 */
void test_LinIf_SetTrcvWakeupMode_InvalidParams_ShouldReportDet(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();

    /* LinTrcv exposes no SetWakeupMode API: invalid values are rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetTrcvWakeupMode(0U, (LinIf_TrcvWakeupModeType)0x55U));
    TEST_ASSERT_EQUAL(LINIF_SID_SETTRCVWAKEUPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_VALUE, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetTrcvWakeupMode(9U, LINIF_TRCV_WU_ENABLE));
    TEST_ASSERT_EQUAL(LINIF_SID_SETTRCVWAKEUPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00682 */
void test_LinIf_GetTrcvWakeupReason_ShouldMapLinTrcvReason(void) {
    LinIf_TrcvWakeupReasonType reason = LINIF_TRCV_WU_ERROR;

    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();

    /* LinTrcv_Init records the reset wake-up reason */
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetTrcvWakeupReason(0U, &reason));
    TEST_ASSERT_EQUAL(LINIF_TRCV_WU_RESET, reason);

    /* A bus wake-up notification updates the delegated reason */
    LinTrcv_Cbk_WakeupByBus(0U);
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetTrcvWakeupReason(0U, &reason));
    TEST_ASSERT_EQUAL(LINIF_TRCV_WU_BY_BUS, reason);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00682 */
void test_LinIf_GetTrcvWakeupReason_NullPtr_ShouldReportParamPointer(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetTrcvWakeupReason(0U, NULL_PTR));
    TEST_ASSERT_EQUAL(LINIF_SID_GETTRCVWAKEUPREASON, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00683 */
void test_LinIf_CheckWakeup_ShouldDelegateToLinTrcv(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();

    /* No wake-up pending yet */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_CheckWakeup(0U));

    LinTrcv_Cbk_WakeupByBus(0U);
    TEST_ASSERT_EQUAL(E_OK, LinIf_CheckWakeup(0U));
    /* The pending flag is consumed by the first check */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_CheckWakeup(0U));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00683 */
void test_LinIf_CheckWakeup_InvalidChannel_ShouldReportParamChannel(void) {
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_CheckWakeup(9U));
    TEST_ASSERT_EQUAL(LINIF_SID_CHECKWAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00679 @req SWS_LinIf_00580 @req SWS_LinIf_00581
 *  @req SWS_LinIf_00582 @req SWS_LinIf_00583 */
void test_LinIf_TrcvAndNodeConfig_BeforeInit_ShouldReportUninit(void) {
    LinIf_TrcvModeType mode = LINIF_TRCV_MODE_NORMAL;
    LinIf_TrcvWakeupReasonType reason = LINIF_TRCV_WU_ERROR;
    uint8 nad = 0U;
    uint8 pidBuf[4] = {0U};
    uint8 numFrames = 0U;

    LinIf_DeInit();

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetTrcvMode(0U, LINIF_TRCV_MODE_NORMAL));
    TEST_ASSERT_EQUAL(LINIF_SID_SETTRCVMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetTrcvMode(0U, &mode));
    TEST_ASSERT_EQUAL(LINIF_SID_GETTRCVMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetTrcvWakeupMode(0U, LINIF_TRCV_WU_ENABLE));
    TEST_ASSERT_EQUAL(LINIF_SID_SETTRCVWAKEUPMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetTrcvWakeupReason(0U, &reason));
    TEST_ASSERT_EQUAL(LINIF_SID_GETTRCVWAKEUPREASON, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_CheckWakeup(0U));
    TEST_ASSERT_EQUAL(LINIF_SID_CHECKWAKEUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetConfiguredNAD(0U, 0x2EU));
    TEST_ASSERT_EQUAL(LINIF_SID_SETCONFIGUREDNAD, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetConfiguredNAD(0U, &nad));
    TEST_ASSERT_EQUAL(LINIF_SID_GETCONFIGUREDNAD, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetPIDTable(0U, pidBuf, 1U));
    TEST_ASSERT_EQUAL(LINIF_SID_SETPIDTABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetPIDTable(0U, pidBuf, &numFrames));
    TEST_ASSERT_EQUAL(LINIF_SID_GETPIDTABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00580 @req SWS_LinIf_00581 */
void test_LinIf_ConfiguredNAD_DefaultAndRoundTrip(void) {
    uint8 nad = 0U;

    LinIf_Init(&LinIf_Config);

    /* Default NAD after Init is 0x60 */
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetConfiguredNAD(0U, &nad));
    TEST_ASSERT_EQUAL(0x60U, nad);

    TEST_ASSERT_EQUAL(E_OK, LinIf_SetConfiguredNAD(0U, 0x2EU));
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetConfiguredNAD(0U, &nad));
    TEST_ASSERT_EQUAL(0x2EU, nad);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00580 @req SWS_LinIf_00581 */
void test_LinIf_ConfiguredNAD_InvalidParams_ShouldReportDet(void) {
    uint8 nad = 0U;

    LinIf_Init(&LinIf_Config);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetConfiguredNAD(9U, 0x2EU));
    TEST_ASSERT_EQUAL(LINIF_SID_SETCONFIGUREDNAD, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetConfiguredNAD(9U, &nad));
    TEST_ASSERT_EQUAL(LINIF_SID_GETCONFIGUREDNAD, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetConfiguredNAD(0U, NULL_PTR));
    TEST_ASSERT_EQUAL(LINIF_SID_GETCONFIGUREDNAD, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00582 @req SWS_LinIf_00583 */
void test_LinIf_PidTable_DefaultFromConfigAndRoundTrip(void) {
    uint8 buf[LINIF_MAX_FRAMES] = {0U};
    uint8 numFrames = 0U;
    static const uint8 newTable[3] = {0x11U, 0x22U, 0x33U};

    LinIf_Init(&LinIf_Config);

    /* Default PID table is copied from the configured frames (0x3C/0x3D/0x3E) */
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetPIDTable(0U, buf, &numFrames));
    TEST_ASSERT_EQUAL(3U, numFrames);
    TEST_ASSERT_EQUAL_HEX8(0x3CU, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0x3DU, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0x3EU, buf[2]);

    TEST_ASSERT_EQUAL(E_OK, LinIf_SetPIDTable(0U, newTable, 3U));
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetPIDTable(0U, buf, &numFrames));
    TEST_ASSERT_EQUAL(3U, numFrames);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(newTable, buf, 3U);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00582 @req SWS_LinIf_00583 */
void test_LinIf_SetPIDTable_OversizedAndNullPtr_ShouldReportDet(void) {
    uint8 buf[LINIF_MAX_FRAMES] = {0U};
    uint8 numFrames = 0U;
    static const uint8 fourPids[4] = {0x11U, 0x22U, 0x33U, 0x44U};

    LinIf_Init(&LinIf_Config);

    /* Channel 0 configures only 3 frames */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetPIDTable(0U, fourPids, 4U));
    TEST_ASSERT_EQUAL(LINIF_SID_SETPIDTABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_VALUE, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetPIDTable(0U, NULL_PTR, 1U));
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetPIDTable(9U, fourPids, 1U));
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetPIDTable(0U, NULL_PTR, &numFrames));
    TEST_ASSERT_EQUAL(LINIF_SID_GETPIDTABLE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetPIDTable(0U, buf, NULL_PTR));
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);

    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetPIDTable(9U, buf, &numFrames));
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** @req SWS_LinIf_00002 */
void test_LinIf_DeInit_ShouldRestoreDefaultNadAndPidTable(void) {
    uint8 nad = 0U;
    uint8 buf[LINIF_MAX_FRAMES] = {0U};
    uint8 numFrames = 0U;
    static const uint8 customTable[2] = {0x55U, 0x66U};

    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetConfiguredNAD(0U, 0x99U));
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetPIDTable(0U, customTable, 2U));

    LinIf_DeInit();
    LinIf_Init(&LinIf_Config);

    /* NAD and PID table are back to the configuration defaults */
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetConfiguredNAD(0U, &nad));
    TEST_ASSERT_EQUAL(0x60U, nad);
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetPIDTable(0U, buf, &numFrames));
    TEST_ASSERT_EQUAL(3U, numFrames);
    TEST_ASSERT_EQUAL_HEX8(0x3CU, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0x3DU, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0x3EU, buf[2]);
}

/** @req SWS_LinIf_00585 */
void test_LinIf_IsSupportTpTransmit_ShouldReflectDiagnosticFrames(void) {
    /* Shipped config holds no LINIF_DIAGNOSTIC_FRAME-typed frame */
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(FALSE, LinIf_IsSupportTpTransmit(0U));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    /* A config with a diagnostic frame supports Tp transmission */
    LinIf_Init(&mock_DiagConfig);
    TEST_ASSERT_EQUAL(TRUE, LinIf_IsSupportTpTransmit(0U));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);

    /* Unknown channel reports FALSE without Det */
    TEST_ASSERT_EQUAL(FALSE, LinIf_IsSupportTpTransmit(9U));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** @req SWS_LinIf_00585 */
void test_LinIf_IsSupportTpTransmit_BeforeInit_ShouldReturnFalse(void) {
    LinIf_DeInit();
    TEST_ASSERT_EQUAL(FALSE, LinIf_IsSupportTpTransmit(0U));
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/* ===== New tests for additional branch coverage ===== */

/** WakeUp returns E_NOT_OK when Lin_WakeUp fails (HW failure path). */
void test_LinIf_WakeUp_HwFailure_ShouldReturnNotOk(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);

    mock_LinRetVal = E_NOT_OK;
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_WakeUp(0U));
    TEST_ASSERT_EQUAL(1U, mock_WakeUpCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** GotoSleep returns E_NOT_OK when Lin_GoToSleep fails (HW failure path). */
void test_LinIf_GotoSleep_HwFailure_ShouldReturnNotOk(void) {
    LinIf_Init(&LinIf_Config);

    mock_LinRetVal = E_NOT_OK;
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GotoSleep(0U));
    TEST_ASSERT_EQUAL(1U, mock_GoToSleepCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** RxIndication with DataPtr=NULL: skips memcpy but still calls RxCallback. */
void test_LinIf_RxIndication_NullDataPtr_ShouldSkipCopy(void) {
    LinIf_PduType rxPdu;
    rxPdu.Id = 0x3DU; rxPdu.Dlc = 8U; rxPdu.DataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    LinIf_RxIndication(0U, &rxPdu);
    TEST_ASSERT_EQUAL(1U, mock_RxCallbackCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** RxIndication with Dlc > LINIF_MAX_FRAME_LENGTH: skips copy, calls callback. */
void test_LinIf_RxIndication_DlcTooLarge_ShouldSkipCopy(void) {
    LinIf_PduType rxPdu;
    uint8 data[16] = {0U};
    rxPdu.Id = 0x3DU; rxPdu.Dlc = 16U; rxPdu.DataPtr = data;

    LinIf_Init(&LinIf_Config);
    LinIf_RxIndication(0U, &rxPdu);
    TEST_ASSERT_EQUAL(1U, mock_RxCallbackCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** RxIndication with invalid channel: returns silently. */
void test_LinIf_RxIndication_InvalidChannel_ShouldReturnSilently(void) {
    LinIf_PduType rxPdu;
    uint8 data[8] = {0U};
    rxPdu.Id = 0x3DU; rxPdu.Dlc = 8U; rxPdu.DataPtr = data;

    LinIf_Init(&LinIf_Config);
    LinIf_RxIndication(9U, &rxPdu);
    TEST_ASSERT_EQUAL(0U, mock_RxCallbackCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** MainFunction skips channel in SLEEP state (state != ONLINE branch). */
void test_LinIf_MainFunction_SleepState_ShouldNotSend(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);

    /* Go to sleep: state becomes LINIF_CHANNEL_SLEEP */
    (void)LinIf_GotoSleep(0U);

    linif_RunTicks(20U);
    TEST_ASSERT_EQUAL(0U, mock_SendCount);
}

/** Transmit with SduDataPtr=NULL: reports LINIF_E_PARAM_PDU. */
void test_LinIf_Transmit_NullSduDataPtr_ShouldReportDet(void) {
    PduInfoType pdu;
    pdu.SduDataPtr = NULL_PTR; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_Transmit(0U, &pdu));
    TEST_ASSERT_EQUAL(LINIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_PDU, mock_DetLastErrorId);
}

/** GotoSleep with invalid channel: reports LINIF_E_PARAM_CHANNEL. */
void test_LinIf_GotoSleep_InvalidChannel_ShouldReportParamChannel(void) {
    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GotoSleep(5U));
    TEST_ASSERT_EQUAL(LINIF_SID_GOTOSLEEP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

/** SetConfiguredNAD with NULL configPtr (uninit): reports LINIF_E_UNINIT. */
void test_LinIf_SetConfiguredNAD_Uninit_ShouldReportUninit(void) {
    LinIf_DeInit();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_SetConfiguredNAD(0U, 0x2EU));
    TEST_ASSERT_EQUAL(LINIF_SID_SETCONFIGUREDNAD, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_UNINIT, mock_DetLastErrorId);
}

/** SetPIDTable with zero frames: boundary condition (0 <= NumFrames). */
void test_LinIf_SetPIDTable_ZeroFrames_ShouldSucceed(void) {
    uint8 buf[LINIF_MAX_FRAMES] = {0U};
    uint8 numFrames = 0xFFU;
    static const uint8 dummyPid = 0x11U;

    LinIf_Init(&LinIf_Config);
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetPIDTable(0U, &dummyPid, 0U));
    TEST_ASSERT_EQUAL(E_OK, LinIf_GetPIDTable(0U, buf, &numFrames));
    TEST_ASSERT_EQUAL(0U, numFrames);
}

/** Init with config holding NULL Channels: exercises else branch in Init. */
void test_LinIf_Init_NullChannels_ShouldInitGracefully(void) {
    static const LinIf_ConfigType nullChCfg = { 1U, NULL_PTR, 0U, NULL_PTR };
    PduInfoType pdu;
    uint8 data[8] = {0U};
    pdu.SduDataPtr = data; pdu.SduLength = 8U; pdu.MetaDataPtr = NULL_PTR;

    LinIf_Init(&nullChCfg);
    /* Module is initialized but channel config is unreachable */
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_Transmit(0U, &pdu));
}

/** RxIndication before init (configPtr NULL): reports LINIF_E_PARAM_POINTER. */
void test_LinIf_RxIndication_BeforeInit_ShouldReportDet(void) {
    LinIf_PduType rxPdu;
    uint8 data[8] = {0U};
    rxPdu.Id = 0x3DU; rxPdu.Dlc = 8U; rxPdu.DataPtr = data;

    LinIf_DeInit();
    LinIf_RxIndication(0U, &rxPdu);
    TEST_ASSERT_EQUAL(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(LINIF_SID_RX_IND, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** SetSchedule transitions channel from INIT to ONLINE state. */
void test_LinIf_SetSchedule_ShouldTransitionToOnline(void) {
    LinIf_Init(&LinIf_Config);
    /* After Init, state = LINIF_CHANNEL_INIT */
    TEST_ASSERT_EQUAL(E_OK, LinIf_SetSchedule(LINIF_Normal));
    /* After SetSchedule, state should be LINIF_CHANNEL_ONLINE */
    /* Verify by running MainFunction: ONLINE channels process schedules */
    linif_RunTicks(5U);
    TEST_ASSERT_EQUAL(1U, mock_SendCount);
    TEST_ASSERT_EQUAL(0U, mock_DetCallCount);
}

/** MainFunction before init: returns silently (configPtr NULL). */
void test_LinIf_MainFunction_DeInit_ShouldNotSend(void) {
    LinIf_Init(&LinIf_Config);
    (void)LinIf_SetSchedule(LINIF_Normal);
    linif_RunTicks(5U);
    TEST_ASSERT_EQUAL(1U, mock_SendCount);

    LinIf_DeInit();
    linif_RunTicks(10U);
    /* No additional sends after DeInit */
    TEST_ASSERT_EQUAL(1U, mock_SendCount);
}

/** GetTrcvMode with invalid channel: reports LINIF_E_PARAM_CHANNEL. */
void test_LinIf_GetTrcvMode_InvalidChannel_ShouldReportParamChannel(void) {
    LinIf_TrcvModeType mode = LINIF_TRCV_MODE_NORMAL;
    LinIf_Init(&LinIf_Config);
    linifTest_InitLinTrcv();
    TEST_ASSERT_EQUAL(E_NOT_OK, LinIf_GetTrcvMode(9U, &mode));
    TEST_ASSERT_EQUAL(LINIF_SID_GETTRCVMODE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL(LINIF_E_PARAM_CHANNEL, mock_DetLastErrorId);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_LinIf_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_LinIf_Transmit_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_LinIf_SetSchedule_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_LinIf_MainFunction_BeforeInit_ShouldReturnSilently);
    RUN_TEST(test_LinIf_Init_ValidConfig_ShouldActivateModule);
    RUN_TEST(test_LinIf_Init_DoubleInit_ShouldStayOperational);
    RUN_TEST(test_LinIf_Transmit_NullPdu_ShouldReportDet);
    RUN_TEST(test_LinIf_Transmit_OutOfRangePduId_ShouldReportParamPdu);
    RUN_TEST(test_LinIf_Transmit_DlcMismatch_ShouldReportParamPdu);
    RUN_TEST(test_LinIf_SetSchedule_ValidSchedule_ShouldSucceed);
    RUN_TEST(test_LinIf_SetSchedule_InvalidSchedule_ShouldReportParamSchedule);
    RUN_TEST(test_LinIf_MainFunction_ShouldSendFramesAtConfiguredDelays);
    RUN_TEST(test_LinIf_MainFunction_ShouldCycleScheduleEntries);
    RUN_TEST(test_LinIf_Transmit_ShouldPublishBufferAtNextSchedulePoint);
    RUN_TEST(test_LinIf_Transmit_EventTriggeredPdu_ShouldKeepConfiguredPid);
    RUN_TEST(test_LinIf_ScheduleRequest_ValidSchedule_ShouldSwitchAndConfirm);
    RUN_TEST(test_LinIf_ScheduleRequest_InvalidSchedule_ShouldReportParamSchedule);
    RUN_TEST(test_LinIf_ScheduleRequest_InvalidChannel_ShouldReportParamChannel);
    RUN_TEST(test_LinIf_GotoSleep_ShouldStopScheduleAndWakeUpShouldResume);
    RUN_TEST(test_LinIf_WakeUp_InvalidChannel_ShouldReportParamChannel);
    RUN_TEST(test_LinIf_WakeUp_GotoSleep_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_LinIf_RxIndication_ShouldDispatchToHook);
    RUN_TEST(test_LinIf_RxIndication_NullPtr_ShouldReportDet);
    RUN_TEST(test_LinIf_DeInit_AfterInit_ShouldDeactivateModule);
    RUN_TEST(test_LinIf_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_LinIf_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_LinIf_SetGetTrcvMode_ShouldDelegateToLinTrcv);
    RUN_TEST(test_LinIf_SetTrcvMode_InvalidChannel_ShouldReportParamChannel);
    RUN_TEST(test_LinIf_SetTrcvMode_InvalidMode_ShouldReportParamValue);
    RUN_TEST(test_LinIf_GetTrcvMode_NullPtr_ShouldReportParamPointer);
    RUN_TEST(test_LinIf_SetTrcvWakeupMode_ShouldAcceptValidModes);
    RUN_TEST(test_LinIf_SetTrcvWakeupMode_InvalidParams_ShouldReportDet);
    RUN_TEST(test_LinIf_GetTrcvWakeupReason_ShouldMapLinTrcvReason);
    RUN_TEST(test_LinIf_GetTrcvWakeupReason_NullPtr_ShouldReportParamPointer);
    RUN_TEST(test_LinIf_CheckWakeup_ShouldDelegateToLinTrcv);
    RUN_TEST(test_LinIf_CheckWakeup_InvalidChannel_ShouldReportParamChannel);
    RUN_TEST(test_LinIf_TrcvAndNodeConfig_BeforeInit_ShouldReportUninit);
    RUN_TEST(test_LinIf_ConfiguredNAD_DefaultAndRoundTrip);
    RUN_TEST(test_LinIf_ConfiguredNAD_InvalidParams_ShouldReportDet);
    RUN_TEST(test_LinIf_PidTable_DefaultFromConfigAndRoundTrip);
    RUN_TEST(test_LinIf_SetPIDTable_OversizedAndNullPtr_ShouldReportDet);
    RUN_TEST(test_LinIf_DeInit_ShouldRestoreDefaultNadAndPidTable);
    RUN_TEST(test_LinIf_IsSupportTpTransmit_ShouldReflectDiagnosticFrames);
    RUN_TEST(test_LinIf_IsSupportTpTransmit_BeforeInit_ShouldReturnFalse);

    /* New tests for additional branch coverage */
    RUN_TEST(test_LinIf_WakeUp_HwFailure_ShouldReturnNotOk);
    RUN_TEST(test_LinIf_GotoSleep_HwFailure_ShouldReturnNotOk);
    RUN_TEST(test_LinIf_RxIndication_NullDataPtr_ShouldSkipCopy);
    RUN_TEST(test_LinIf_RxIndication_DlcTooLarge_ShouldSkipCopy);
    RUN_TEST(test_LinIf_RxIndication_InvalidChannel_ShouldReturnSilently);
    RUN_TEST(test_LinIf_MainFunction_SleepState_ShouldNotSend);
    RUN_TEST(test_LinIf_Transmit_NullSduDataPtr_ShouldReportDet);
    RUN_TEST(test_LinIf_GotoSleep_InvalidChannel_ShouldReportParamChannel);
    RUN_TEST(test_LinIf_SetConfiguredNAD_Uninit_ShouldReportUninit);
    RUN_TEST(test_LinIf_SetPIDTable_ZeroFrames_ShouldSucceed);
    RUN_TEST(test_LinIf_Init_NullChannels_ShouldInitGracefully);
    RUN_TEST(test_LinIf_RxIndication_BeforeInit_ShouldReportDet);
    RUN_TEST(test_LinIf_SetSchedule_ShouldTransitionToOnline);
    RUN_TEST(test_LinIf_MainFunction_DeInit_ShouldNotSend);
    RUN_TEST(test_LinIf_GetTrcvMode_InvalidChannel_ShouldReportParamChannel);

    return UnityEnd();
}
