/**
 * @file test_frif_runner.c
 * @brief Unity runner for the substantiated FrIf unit tests.
 */
#include "unity.h"

extern void setUp(void);
extern void tearDown(void);

/* FrIf lifecycle & version */
extern void test_FrIf_Init_NullPtr_ShouldReportInvConfig(void);
extern void test_FrIf_Init_ValidConfig_ShouldSucceed(void);
extern void test_FrIf_Init_DoubleInit_ShouldReportAlreadyInitialized(void);
extern void test_FrIf_GetVersionInfo_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetVersionInfo_ValidPtr_ShouldFillVersionInfo(void);
/* Controller */
extern void test_FrIf_ControllerInit_Uninit_ShouldReportUninit(void);
extern void test_FrIf_ControllerInit_InvalidCtrl_ShouldReportInvCtrlIdx(void);
/* Timers */
extern void test_FrIf_SetAbsoluteTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SetAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx(void);
extern void test_FrIf_SetAbsoluteTimer_ValidCall_ShouldSucceedAndArmTimer(void);
extern void test_FrIf_SetRelativeTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SetRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx(void);
extern void test_FrIf_CancelAbsoluteTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_CancelAbsoluteTimer_ValidCall_ShouldSucceed(void);
extern void test_FrIf_CancelRelativeTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_CancelRelativeTimer_ValidCall_ShouldSucceed(void);
/* Transmit */
extern void test_FrIf_Transmit_Uninit_ShouldReportUninit(void);
extern void test_FrIf_Transmit_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_Transmit_InvalidLpdu_ShouldReportInvLpduIdx(void);
extern void test_FrIf_Transmit_ControllerNotActive_ShouldReturnNotOk(void);
/* POC status / global time */
extern void test_FrIf_GetPOCStatus_Uninit_ShouldReportUninit(void);
extern void test_FrIf_GetPOCStatus_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetPOCStatus_ValidCall_ShouldReturnCurrentMode(void);
extern void test_FrIf_GetGlobalTime_Uninit_ShouldReportUninit(void);
extern void test_FrIf_GetGlobalTime_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetGlobalTime_ValidCall_ShouldSucceed(void);
/* Controller mode transitions */
extern void test_FrIf_AllowColdstart_Uninit_ShouldReportUninit(void);
extern void test_FrIf_AllowColdstart_ValidCall_ShouldSetColdstartMode(void);
extern void test_FrIf_HaltCommunication_ValidCall_ShouldSetHaltMode(void);
extern void test_FrIf_AbortCommunication_ValidCall_ShouldSetStandbyMode(void);
extern void test_FrIf_SendWUP_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SendWUP_ValidCall_ShouldSetWakeupMode(void);
extern void test_FrIf_SetWakeupChannel_InvalidChannel_ShouldReportInvChnl(void);
extern void test_FrIf_SetWakeupChannel_ValidCall_ShouldSucceed(void);
/* MainFunction */
extern void test_FrIf_MainFunction_Uninit_ShouldReturnSilently(void);
extern void test_FrIf_MainFunction_ValidCall_ShouldKeepStateStable(void);

int main(void) {
    UnityBegin();

    /* ------------------------------------------------------------------
     * Phase 1 — uninitialised contract.
     *
     * FrIf_Init() is once-only per process (SWS_FrIf) and FrIf has no
     * DeInit, so a module instance can never return to the uninitialised
     * state. These cases therefore have to run before the first
     * FrIf_Init() of this process, and setUp() only resets the DET mock.
     * ------------------------------------------------------------------ */
    RUN_TEST(test_FrIf_ControllerInit_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_SetAbsoluteTimer_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_SetRelativeTimer_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_CancelAbsoluteTimer_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_CancelRelativeTimer_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_Transmit_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_GetPOCStatus_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_GetGlobalTime_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_AllowColdstart_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_SendWUP_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_MainFunction_Uninit_ShouldReturnSilently);

    /* ------------------------------------------------------------------
     * Phase 2 — initialisation. ValidConfig performs the first (and only)
     * successful FrIf_Init() of this process; every later case runs with
     * the module already initialised, so it must not re-init.
     * ------------------------------------------------------------------ */
    RUN_TEST(test_FrIf_Init_NullPtr_ShouldReportInvConfig);
    RUN_TEST(test_FrIf_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_FrIf_Init_DoubleInit_ShouldReportAlreadyInitialized);
    RUN_TEST(test_FrIf_GetVersionInfo_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetVersionInfo_ValidPtr_ShouldFillVersionInfo);

    /* ------------------------------------------------------------------
     * Phase 3 — initialised behaviour
     * ------------------------------------------------------------------ */
    RUN_TEST(test_FrIf_ControllerInit_InvalidCtrl_ShouldReportInvCtrlIdx);

    RUN_TEST(test_FrIf_SetAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx);
    RUN_TEST(test_FrIf_SetAbsoluteTimer_ValidCall_ShouldSucceedAndArmTimer);
    RUN_TEST(test_FrIf_SetRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx);
    RUN_TEST(test_FrIf_CancelAbsoluteTimer_ValidCall_ShouldSucceed);
    RUN_TEST(test_FrIf_CancelRelativeTimer_ValidCall_ShouldSucceed);

    RUN_TEST(test_FrIf_Transmit_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_Transmit_InvalidLpdu_ShouldReportInvLpduIdx);
    RUN_TEST(test_FrIf_Transmit_ControllerNotActive_ShouldReturnNotOk);

    RUN_TEST(test_FrIf_GetPOCStatus_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetPOCStatus_ValidCall_ShouldReturnCurrentMode);
    RUN_TEST(test_FrIf_GetGlobalTime_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetGlobalTime_ValidCall_ShouldSucceed);

    RUN_TEST(test_FrIf_AllowColdstart_ValidCall_ShouldSetColdstartMode);
    RUN_TEST(test_FrIf_HaltCommunication_ValidCall_ShouldSetHaltMode);
    RUN_TEST(test_FrIf_AbortCommunication_ValidCall_ShouldSetStandbyMode);
    RUN_TEST(test_FrIf_SendWUP_ValidCall_ShouldSetWakeupMode);
    RUN_TEST(test_FrIf_SetWakeupChannel_InvalidChannel_ShouldReportInvChnl);
    RUN_TEST(test_FrIf_SetWakeupChannel_ValidCall_ShouldSucceed);

    RUN_TEST(test_FrIf_MainFunction_ValidCall_ShouldKeepStateStable);

    return UnityEnd();
}
