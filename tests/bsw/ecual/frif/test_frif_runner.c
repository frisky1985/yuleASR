/**
 * @file test_frif_runner.c
 * @brief Unity runner for the substantiated FrIf unit tests.
 *
 * FrIf has no DeInit API and is once-only per process — Phase 1 tests run
 * before the first successful FrIf_Init(); Phase 2 performs that init; all
 * later tests assume the module is initialised. Tests tagged "...Hack" use
 * frif_test_hacks.c accessors to reach FrIf's internal state.
 */
#include "unity.h"

extern void setUp(void);
extern void tearDown(void);

/* Test configuration (lives in test_frif.c — shared TU scope because FrIf.c
 * is compiled via frif_test_hacks.c, but test_frif.c + runner are a second
 * TU that #includes FrIf.h only). We need the test config pointer for
 * hack-init tests below. */

/* FrIf lifecycle & version */
extern void test_FrIf_Init_NullPtr_ShouldReportInvConfig(void);
extern void test_FrIf_Init_ValidConfig_ShouldSucceed(void);
extern void test_FrIf_Init_DoubleInit_ShouldReportAlreadyInitialized(void);
extern void test_FrIf_GetVersionInfo_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetVersionInfo_ValidPtr_ShouldFillVersionInfo(void);

/* Controller init */
extern void test_FrIf_ControllerInit_Uninit_ShouldReportUninit(void);
extern void test_FrIf_ControllerInit_InvalidCtrl_ShouldReportInvCtrlIdx(void);

/* Timers: SetAbsoluteTimer */
extern void test_FrIf_SetAbsoluteTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SetAbsoluteTimer_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_SetAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx(void);
extern void test_FrIf_SetAbsoluteTimer_ValidCall_ShouldSucceedAndArmTimer(void);
extern void test_FrIf_SetAbsoluteTimer_AllTimers_ShouldSucceed(void);

/* Timers: SetRelativeTimer */
extern void test_FrIf_SetRelativeTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SetRelativeTimer_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_SetRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx(void);
extern void test_FrIf_SetRelativeTimer_AllTimers_ShouldSucceed(void);

/* Timers: CancelAbsoluteTimer */
extern void test_FrIf_CancelAbsoluteTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_CancelAbsoluteTimer_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_CancelAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx(void);
extern void test_FrIf_CancelAbsoluteTimer_ValidCall_ShouldSucceed(void);

/* Timers: CancelRelativeTimer */
extern void test_FrIf_CancelRelativeTimer_Uninit_ShouldReportUninit(void);
extern void test_FrIf_CancelRelativeTimer_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_CancelRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx(void);
extern void test_FrIf_CancelRelativeTimer_ValidCall_ShouldSucceed(void);

/* Transmit */
extern void test_FrIf_Transmit_Uninit_ShouldReportUninit(void);
extern void test_FrIf_Transmit_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_Transmit_InvalidLpdu_ShouldReportInvLpduIdx(void);
extern void test_FrIf_Transmit_ControllerNotActive_ShouldReturnNotOk(void);
extern void test_FrIf_Transmit_UnconfiguredLpdu_ShouldReturnNotOk(void);
extern void test_FrIf_Transmit_ControllerActive_ShouldReturnOk(void);
extern void test_FrIf_Transmit_CtrlIdxOutOfRange_ShouldReturnNotOk(void);

/* POC status */
extern void test_FrIf_GetPOCStatus_Uninit_ShouldReportUninit(void);
extern void test_FrIf_GetPOCStatus_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_GetPOCStatus_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetPOCStatus_ValidCall_ShouldReturnCurrentMode(void);

/* Global time */
extern void test_FrIf_GetGlobalTime_Uninit_ShouldReportUninit(void);
extern void test_FrIf_GetGlobalTime_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_GetGlobalTime_NullPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetGlobalTime_NullMacrotickPtr_ShouldReportInvPointer(void);
extern void test_FrIf_GetGlobalTime_ValidCall_ShouldSucceed(void);

/* Controller mode transitions */
extern void test_FrIf_AllowColdstart_Uninit_ShouldReportUninit(void);
extern void test_FrIf_AllowColdstart_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_AllowColdstart_ValidCall_ShouldSetColdstartMode(void);

extern void test_FrIf_HaltCommunication_Uninit_ShouldReportUninit(void);
extern void test_FrIf_HaltCommunication_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_HaltCommunication_ValidCall_ShouldSetHaltMode(void);

extern void test_FrIf_AbortCommunication_Uninit_ShouldReportUninit(void);
extern void test_FrIf_AbortCommunication_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_AbortCommunication_ValidCall_ShouldSetStandbyMode(void);

extern void test_FrIf_SendWUP_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SendWUP_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_SendWUP_ValidCall_ShouldSetWakeupMode(void);

extern void test_FrIf_SetWakeupChannel_Uninit_ShouldReportUninit(void);
extern void test_FrIf_SetWakeupChannel_InvalidCtrl_ShouldReportInvCtrlIdx(void);
extern void test_FrIf_SetWakeupChannel_InvalidChannel_ShouldReportInvChnl(void);
extern void test_FrIf_SetWakeupChannel_ValidCall_ShouldSucceed(void);

/* MainFunction */
extern void test_FrIf_MainFunction_Uninit_ShouldReturnSilently(void);
extern void test_FrIf_MainFunction_ValidCall_ShouldKeepStateStable(void);
extern void test_FrIf_MainFunction_StartupMode_Hack(void);
extern void test_FrIf_MainFunction_HaltMode_ShouldProcess(void);
extern void test_FrIf_MainFunction_ColdstartMode_Hack(void);
extern void test_FrIf_MainFunction_NormalActiveMode_Hack(void);
extern void test_FrIf_MainFunction_NormalPassiveMode_Hack(void);
extern void test_FrIf_MainFunction_NormalActiveMode_ShouldProcess(void);

/* ControllerInit post-init success path */
extern void test_FrIf_ControllerInit_ValidCall_ShouldSetReadyMode(void);

int main(void) {
    UnityBegin();

    /* ------------------------------------------------------------------
     * Phase 1 — uninitialised contract. MUST run before first Init.
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
    RUN_TEST(test_FrIf_HaltCommunication_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_AbortCommunication_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_SendWUP_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_SetWakeupChannel_Uninit_ShouldReportUninit);
    RUN_TEST(test_FrIf_MainFunction_Uninit_ShouldReturnSilently);

    /* ------------------------------------------------------------------
     * Phase 2 — initialisation. ValidConfig performs the *only* first
     * successful FrIf_Init() of this process. DoubleInit verifies
     * subsequent calls are rejected. GetVersionInfo works any time.
     * ------------------------------------------------------------------ */
    RUN_TEST(test_FrIf_Init_NullPtr_ShouldReportInvConfig);
    RUN_TEST(test_FrIf_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_FrIf_Init_DoubleInit_ShouldReportAlreadyInitialized);
    RUN_TEST(test_FrIf_GetVersionInfo_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetVersionInfo_ValidPtr_ShouldFillVersionInfo);

    /* ------------------------------------------------------------------
     * Phase 3 — initialised behaviour. All the error/valid paths.
     * ------------------------------------------------------------------ */

    /* Controller */
    RUN_TEST(test_FrIf_ControllerInit_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_ControllerInit_ValidCall_ShouldSetReadyMode);

    /* Timers */
    RUN_TEST(test_FrIf_SetAbsoluteTimer_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_SetAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx);
    RUN_TEST(test_FrIf_SetAbsoluteTimer_ValidCall_ShouldSucceedAndArmTimer);
    RUN_TEST(test_FrIf_SetAbsoluteTimer_AllTimers_ShouldSucceed);

    RUN_TEST(test_FrIf_SetRelativeTimer_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_SetRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx);
    RUN_TEST(test_FrIf_SetRelativeTimer_AllTimers_ShouldSucceed);

    RUN_TEST(test_FrIf_CancelAbsoluteTimer_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_CancelAbsoluteTimer_InvalidTimer_ShouldReportInvTimerIdx);
    RUN_TEST(test_FrIf_CancelAbsoluteTimer_ValidCall_ShouldSucceed);

    RUN_TEST(test_FrIf_CancelRelativeTimer_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_CancelRelativeTimer_InvalidTimer_ShouldReportInvTimerIdx);
    RUN_TEST(test_FrIf_CancelRelativeTimer_ValidCall_ShouldSucceed);

    /* Transmit */
    RUN_TEST(test_FrIf_Transmit_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_Transmit_InvalidLpdu_ShouldReportInvLpduIdx);
    RUN_TEST(test_FrIf_Transmit_UnconfiguredLpdu_ShouldReturnNotOk);
    RUN_TEST(test_FrIf_Transmit_ControllerNotActive_ShouldReturnNotOk);
    RUN_TEST(test_FrIf_Transmit_ControllerActive_ShouldReturnOk);
    RUN_TEST(test_FrIf_Transmit_CtrlIdxOutOfRange_ShouldReturnNotOk);

    /* POC / GlobalTime */
    RUN_TEST(test_FrIf_GetPOCStatus_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_GetPOCStatus_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetPOCStatus_ValidCall_ShouldReturnCurrentMode);

    RUN_TEST(test_FrIf_GetGlobalTime_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_GetGlobalTime_NullPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetGlobalTime_NullMacrotickPtr_ShouldReportInvPointer);
    RUN_TEST(test_FrIf_GetGlobalTime_ValidCall_ShouldSucceed);

    /* Controller mode transitions */
    RUN_TEST(test_FrIf_AllowColdstart_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_AllowColdstart_ValidCall_ShouldSetColdstartMode);

    RUN_TEST(test_FrIf_HaltCommunication_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_HaltCommunication_ValidCall_ShouldSetHaltMode);

    RUN_TEST(test_FrIf_AbortCommunication_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_AbortCommunication_ValidCall_ShouldSetStandbyMode);

    RUN_TEST(test_FrIf_SendWUP_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_SendWUP_ValidCall_ShouldSetWakeupMode);

    RUN_TEST(test_FrIf_SetWakeupChannel_InvalidCtrl_ShouldReportInvCtrlIdx);
    RUN_TEST(test_FrIf_SetWakeupChannel_InvalidChannel_ShouldReportInvChnl);
    RUN_TEST(test_FrIf_SetWakeupChannel_ValidCall_ShouldSucceed);

    /* MainFunction */
    RUN_TEST(test_FrIf_MainFunction_ValidCall_ShouldKeepStateStable);
    RUN_TEST(test_FrIf_MainFunction_HaltMode_ShouldProcess);
    RUN_TEST(test_FrIf_MainFunction_StartupMode_Hack);
    RUN_TEST(test_FrIf_MainFunction_ColdstartMode_Hack);
    RUN_TEST(test_FrIf_MainFunction_NormalActiveMode_Hack);
    RUN_TEST(test_FrIf_MainFunction_NormalPassiveMode_Hack);
    RUN_TEST(test_FrIf_MainFunction_NormalActiveMode_ShouldProcess);

    return UnityEnd();
}
