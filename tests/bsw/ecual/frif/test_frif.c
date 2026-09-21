/**
 * @file test_frif.c
 * @brief FrIf Unit Tests — substantiated against src/bsw/ecual/frif (ecual_frif)
 * @version 1.0.0
 * @date 2026-08-25
 *
 * DET configuration: FRIF_DEV_ERROR_DETECT == STD_ON (FrIf_Cfg.h), so every
 * API-misuse branch reports via Det_ReportError; assertions below verify the
 * exact ApiId/ErrorId pairs.
 *
 * Host-run note: FlexRay controller hardware is not present on the host, but
 * FrIf keeps all observable state in RAM (controller mode, timers, LPDU table),
 * so hardware-driven scenarios are verified via API-level state assertions.
 */

// @tests src/bsw/ecual/frif/src/FrIf.c  @tests src/bsw/ecual/frif/include/FrIf.h

#include "unity.h"
#include "FrIf.h"

/* Mock Det_ReportError (inline in this file — mock_det.c is NOT linked) */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* ---------------------------------------------------------------- */
/* Test configuration: one controller, one static-segment LPDU       */
/* (FrIf_Init() copies this into the internal LPDU state table.)     */
/* ---------------------------------------------------------------- */
static FrIf_ControllerConfigType testCtrl = {
    /* CtrlIdx */           0U,
    /* FrCtrlIdx */         0U,
    /* FrTrcvIdx */         0U,
    /* ClusterIdx */        0U,
    /* WakeupSupport */     TRUE,
    /* ColdstartSupport */  TRUE
};

static FrIf_LpduConfigType testLpdu = {
    /* LpduIdx */          0U,
    /* CtrlIdx */          0U,
    /* SlotId */           1U,
    /* Cycle */            0U,
    /* CycleRepetition */  1U,
    /* CycleOffset */      0U,
    /* Channel */          0U,
    /* PayloadLength */    8U,
    /* DynamicSegment */   FALSE
};

static FrIf_ConfigType testConfig = {
    /* Controllers */      &testCtrl,
    /* NumControllers */   1U,
    /* Lpdus */            &testLpdu,
    /* NumLpdus */         1U,
    /* DevErrorDetect */   TRUE,
    /* VersionInfoApi */   TRUE,
    /* ClstStartupActive */    TRUE,
    /* ClstWakeupActive */     TRUE,
    /* FrIfGetWupRxStatusSupport */    TRUE,
    /* FrIfGetSyncFrameListSupport */  TRUE,
    /* FrIfGetClockCorrectionSupport */ TRUE
};

/* Init with the valid test config. */
static void test_FrIf_InitValid(void) {
    FrIf_Init(&testConfig);
}

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_FrIf_00001 */
void test_FrIf_Init_NullPtr_ShouldReportInvConfig(void) {
    FrIf_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_CONFIG, mock_DetLastErrorId);
    /* Init refused: Transmit afterwards must still report FRIF_E_UNINIT */
    mock_Det_Reset();
    {
        PduInfoType info = { NULL_PTR, 0U };
        Std_ReturnType ret = FrIf_Transmit(0U, &info);
        TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
        TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
    }
}

/** @req SWS_FrIf_00001 */
void test_FrIf_Init_ValidConfig_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    /* Initialized: no DET complaints and API calls succeed */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    ret = FrIf_ControllerInit(0U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00001 */
void test_FrIf_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    test_FrIf_InitValid();
    mock_Det_Reset();
    FrIf_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00002 */
void test_FrIf_GetVersionInfo_NullPtr_ShouldReportInvPointer(void) {
    FrIf_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00002 */
void test_FrIf_GetVersionInfo_ValidPtr_ShouldFillVersionInfo(void) {
    Std_VersionInfoType vi;
    vi.vendorID = 0U;
    vi.moduleID = 0U;
    vi.sw_major_version = 0U;
    vi.sw_minor_version = 0U;
    vi.sw_patch_version = 0U;

    FrIf_GetVersionInfo(&vi);

    TEST_ASSERT_EQUAL_UINT16((uint16)FRIF_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16((uint16)FRIF_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SW_PATCH_VERSION, vi.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00004 */
void test_FrIf_ControllerInit_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_ControllerInit(0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_CONTROLLERINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00004 */
void test_FrIf_ControllerInit_InvalidCtrl_ShouldReportInvCtrlIdx(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_ControllerInit((uint8)FRIF_NUM_CONTROLLERS);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_CONTROLLERINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_CTRL_IDX, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00005 */
void test_FrIf_SetAbsoluteTimer_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_SetAbsoluteTimer(0U, 0U, 0U, 0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_SETABSOLUTETIMER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00005 */
void test_FrIf_SetAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_SetAbsoluteTimer(0U, 4U, 0U, 0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_SETABSOLUTETIMER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_TIMER_IDX, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00005 */
void test_FrIf_SetAbsoluteTimer_ValidCall_ShouldSucceedAndArmTimer(void) {
    Std_ReturnType ret;
    FrIf_POCStatusType poc;

    test_FrIf_InitValid();
    ret = FrIf_SetAbsoluteTimer(0U, 0U, 2U, 100U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);

    /* Timer-visible effect: MainFunction must not report anything and the
     * controller stays in its current mode (host-verifiable timer loop). */
    FrIf_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);

    /* Cancel the armed timer again */
    ret = FrIf_CancelAbsoluteTimer(0U, 0U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);

    /* POC status reflects controller mode (STANDBY after Init) */
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_STANDBY, poc.State);
}

/** @req SWS_FrIf_00006 */
void test_FrIf_SetRelativeTimer_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_SetRelativeTimer(0U, 0U, 0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_SETRELATIVETIMER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00006 */
void test_FrIf_SetRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_SetRelativeTimer(0U, 4U, 0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_SETRELATIVETIMER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_TIMER_IDX, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00007 */
void test_FrIf_CancelAbsoluteTimer_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_CancelAbsoluteTimer(0U, 0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_CANCELABSOLUTETIMER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00007 */
void test_FrIf_CancelAbsoluteTimer_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    (void)FrIf_SetAbsoluteTimer(0U, 1U, 0U, 10U);
    ret = FrIf_CancelAbsoluteTimer(0U, 1U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00008 */
void test_FrIf_CancelRelativeTimer_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_CancelRelativeTimer(0U, 0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_CANCELRELATIVETIMER, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00008 */
void test_FrIf_CancelRelativeTimer_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    (void)FrIf_SetRelativeTimer(0U, 2U, 10U);
    ret = FrIf_CancelRelativeTimer(0U, 2U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00009 */
void test_FrIf_Transmit_Uninit_ShouldReportUninit(void) {
    PduInfoType info = { NULL_PTR, 0U };
    Std_ReturnType ret = FrIf_Transmit(0U, &info);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00009 */
void test_FrIf_Transmit_NullPtr_ShouldReportInvPointer(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_Transmit(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00009 */
void test_FrIf_Transmit_InvalidLpdu_ShouldReportInvLpduIdx(void) {
    PduInfoType info = { NULL_PTR, 0U };
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_Transmit((PduIdType)FRIF_NUM_LPDUS, &info);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_TRANSMIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_LPDU_IDX, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00009 */
void test_FrIf_Transmit_ControllerNotActive_ShouldReturnNotOk(void) {
    uint8 sdu[8] = { 0U };
    PduInfoType info = { sdu, 8U };
    Std_ReturnType ret;
    test_FrIf_InitValid();
    /* LPDU 0 is configured but controller stays in STANDBY (not NORMAL_ACTIVE):
     * hardware "not ready" equivalent — Transmit must refuse without DET. */
    ret = FrIf_Transmit(0U, &info);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00010 */
void test_FrIf_GetPOCStatus_Uninit_ShouldReportUninit(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_GETPOCSTATUS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00010 */
void test_FrIf_GetPOCStatus_NullPtr_ShouldReportInvPointer(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_GetPOCStatus(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_GETPOCSTATUS, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00010 */
void test_FrIf_GetPOCStatus_ValidCall_ShouldReturnCurrentMode(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    (void)FrIf_ControllerInit(0U);              /* mode -> READY */
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_READY, poc.State);
    TEST_ASSERT_EQUAL_UINT8(0U, poc.SubState);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00011 */
void test_FrIf_GetGlobalTime_Uninit_ShouldReportUninit(void) {
    uint8 cycle = 0U;
    uint16 mt = 0U;
    Std_ReturnType ret = FrIf_GetGlobalTime(0U, &cycle, &mt);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_GETGLOBALTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00011 */
void test_FrIf_GetGlobalTime_NullPtr_ShouldReportInvPointer(void) {
    uint16 mt = 0U;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_GetGlobalTime(0U, NULL_PTR, &mt);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_GETGLOBALTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00011 */
void test_FrIf_GetGlobalTime_ValidCall_ShouldSucceed(void) {
    uint8 cycle = 0xFFU;
    uint16 mt = 0xFFFFU;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_GetGlobalTime(0U, &cycle, &mt);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    /* Host stub returns zeroed global time */
    TEST_ASSERT_EQUAL_UINT8(0U, cycle);
    TEST_ASSERT_EQUAL_UINT16(0U, mt);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00012 */
void test_FrIf_AllowColdstart_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_AllowColdstart(0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_ALLOWCOLDSTART, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00012 */
void test_FrIf_AllowColdstart_ValidCall_ShouldSetColdstartMode(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    /* FRIF_COLDSTART_SUPPORT == STD_ON in FrIf_Cfg.h */
    ret = FrIf_AllowColdstart(0U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_COLDSTART, poc.State);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00013 */
void test_FrIf_HaltCommunication_ValidCall_ShouldSetHaltMode(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_HaltCommunication(0U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_HALT, poc.State);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00014 */
void test_FrIf_AbortCommunication_ValidCall_ShouldSetStandbyMode(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    (void)FrIf_ControllerInit(0U);  /* leave STANDBY first */
    ret = FrIf_AbortCommunication(0U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_STANDBY, poc.State);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00015 */
void test_FrIf_SendWUP_Uninit_ShouldReportUninit(void) {
    Std_ReturnType ret = FrIf_SendWUP(0U);
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_WAKEUPCTRL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00015 */
void test_FrIf_SendWUP_ValidCall_ShouldSetWakeupMode(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    /* FRIF_WAKEUP_SUPPORT == STD_ON in FrIf_Cfg.h */
    ret = FrIf_SendWUP(0U);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_WAKEUP, poc.State);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00016 */
void test_FrIf_SetWakeupChannel_InvalidChannel_ShouldReportInvChnl(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    ret = FrIf_SetWakeupChannel(0U, (FrIf_ChannelType)(FRIF_CHANNEL_AB + 1U));
    TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_SID_SETWAKEUPCHANNEL, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_E_INV_CHNL, mock_DetLastErrorId);
}

/** @req SWS_FrIf_00016 */
void test_FrIf_SetWakeupChannel_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_FrIf_InitValid();
    /* FRIF_WAKEUP_SUPPORT == STD_ON in FrIf_Cfg.h */
    ret = FrIf_SetWakeupChannel(0U, (FrIf_ChannelType)FRIF_CHANNEL_B);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FrIf_00003 */
void test_FrIf_MainFunction_Uninit_ShouldReturnSilently(void) {
    /* Uninitialized MainFunction is a silent no-op by design (no DET route) */
    FrIf_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* And the module must still be unusable afterwards */
    {
        PduInfoType info = { NULL_PTR, 0U };
        Std_ReturnType ret = FrIf_Transmit(0U, &info);
        TEST_ASSERT_EQUAL_INT(E_NOT_OK, ret);
    }
}

/** @req SWS_FrIf_00003 */
void test_FrIf_MainFunction_ValidCall_ShouldKeepStateStable(void) {
    FrIf_POCStatusType poc;
    Std_ReturnType ret;
    test_FrIf_InitValid();
    (void)FrIf_ControllerInit(0U);
    (void)FrIf_SetAbsoluteTimer(0U, 0U, 1U, 100U);
    (void)FrIf_SetRelativeTimer(0U, 0U, 100U);

    FrIf_MainFunction();
    FrIf_MainFunction();

    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    ret = FrIf_GetPOCStatus(0U, &poc);
    TEST_ASSERT_EQUAL_INT(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8((uint8)FRIF_MODE_READY, poc.State);
}
