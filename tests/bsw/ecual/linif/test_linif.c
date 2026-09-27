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
#include <string.h>

/* Det SID/error literals are private to LinIf.c (not exported in the header) */
#define LINIF_SID_INIT          (0x00U)
#define LINIF_SID_TRANSMIT      (0x02U)
#define LINIF_SID_RX_INDICATION (0x03U)
#define LINIF_SID_SCHEDULE      (0x05U)
#define LINIF_SID_WAKEUP        (0x09U)
#define LINIF_SID_GOTOSLEEP     (0x0AU)
#define LINIF_SID_SCHEDREQ      (0x0BU)
#define LINIF_E_PARAM_POINTER   (0x10U)
#define LINIF_E_UNINIT          (0x20U)
#define LINIF_E_PARAM_PDU       (0x30U)
#define LINIF_E_PARAM_SCHEDULE  (0x40U)
#define LINIF_E_PARAM_CHANNEL   (0x50U)

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
    (void)memset(&mock_RxCallbackPdu, 0, sizeof(mock_RxCallbackPdu));
    (void)memset(mock_SendPdu, 0, sizeof(mock_SendPdu));
    (void)memset(mock_SendChannel, 0, sizeof(mock_SendChannel));
    (void)memset(mock_SendData, 0, sizeof(mock_SendData));
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

    return UnityEnd();
}
