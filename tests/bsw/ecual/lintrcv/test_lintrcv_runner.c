/**
 * @file test_lintrcv_runner.c
 * @brief Unity runner for the substantiated LinTrcv unit tests.
 *
 * The test file only provides setUp/tearDown and the test_LinTrcv_* functions,
 * so this runner supplies main() and the RUN_TEST list. setUp() leaves the
 * module uninitialized and tearDown() DeInits it, so the tests are order
 * independent and can run in declaration order.
 *
 * @tests src/bsw/ecual/lintrcv/src/LinTrcv.c
 */
#include "unity.h"

extern void setUp(void);
extern void tearDown(void);

/* Lifecycle & version */
extern void test_LinTrcv_Init_NullPtr_ShouldReportError(void);
extern void test_LinTrcv_Init_ValidConfig_ShouldSucceed(void);
extern void test_LinTrcv_Init_DoubleInit_ShouldSucceed(void);
extern void test_LinTrcv_DeInit_Uninit_ShouldBeSilentNoOp(void);
extern void test_LinTrcv_DeInit_ValidCall_ShouldResetState(void);
extern void test_LinTrcv_GetVersionInfo_NullPtr_ShouldReportError(void);
extern void test_LinTrcv_GetVersionInfo_ValidPtr_ShouldReturnVersion(void);

/* Operation mode */
extern void test_LinTrcv_SetOpMode_Uninit_ShouldReportError(void);
extern void test_LinTrcv_SetOpMode_InvalidChannel_ShouldReportError(void);
extern void test_LinTrcv_SetOpMode_InvalidOpMode_ShouldReportError(void);
extern void test_LinTrcv_SetOpMode_ValidCall_ShouldDriveEnPin(void);
extern void test_LinTrcv_GetOpMode_Uninit_ShouldReportError(void);
extern void test_LinTrcv_GetOpMode_InvalidChannel_ShouldReportError(void);
extern void test_LinTrcv_GetOpMode_NullPtr_ShouldReportError(void);
extern void test_LinTrcv_GetOpMode_ValidCall_ShouldReturnMode(void);

/* Wake-up reason */
extern void test_LinTrcv_GetBusWuReason_Uninit_ShouldReportError(void);
extern void test_LinTrcv_GetBusWuReason_InvalidChannel_ShouldReportError(void);
extern void test_LinTrcv_GetBusWuReason_ValidCall_ShouldReturnReason(void);

/* Main function & wake-up detection */
extern void test_LinTrcv_MainFunction_Uninit_ShouldBeSilentNoOp(void);
extern void test_LinTrcv_MainFunction_ValidCall_ShouldSucceed(void);
extern void test_LinTrcv_MainFunction_PinWakeup_ShouldSetReasonByPin(void);
extern void test_LinTrcv_CheckWakeup_NoEvent_ShouldReturnNotOk(void);
extern void test_LinTrcv_Cbk_WakeupByBus_ShouldSetPendingEvent(void);

/* Additional branch-coverage tests */
extern void test_LinTrcv_SetOpMode_StandbyMode_ShouldDriveEnLow(void);
extern void test_LinTrcv_SetOpMode_TJA1028_ShouldSucceed(void);
extern void test_LinTrcv_SetOpMode_Generic_ShouldSucceed(void);
extern void test_LinTrcv_GetOpMode_TJA1028_ShouldReturnMode(void);
extern void test_LinTrcv_GetOpMode_Generic_ShouldReturnMode(void);
extern void test_LinTrcv_Wakeup_Uninit_ShouldReportError(void);
extern void test_LinTrcv_Wakeup_InvalidChannel_ShouldReportError(void);
extern void test_LinTrcv_Wakeup_FromSleep_ShouldTransitionToNormal(void);
extern void test_LinTrcv_CheckWakeup_Uninit_ShouldReportError(void);
extern void test_LinTrcv_CheckWakeup_InvalidChannel_ShouldReportError(void);
extern void test_LinTrcv_CheckWakeup_WithWakeupSource_ShouldNotifyEcuM(void);
extern void test_LinTrcv_Cbk_WakeupByBus_Uninit_ShouldReturnSilently(void);
extern void test_LinTrcv_Cbk_WakeupByBus_InvalidChannel_ShouldReportError(void);
extern void test_LinTrcv_Cbk_WakeupByBus_WithWakeupSource_ShouldNotifyEcuM(void);
extern void test_LinTrcv_GetBusWuReason_NullPtr_ShouldReportError(void);
extern void test_LinTrcv_GetBusWuReason_AfterSysErr_ShouldReturnSysErr(void);
extern void test_LinTrcv_Init_StandbyInitialMode_ShouldEnterStandby(void);
extern void test_LinTrcv_Init_SleepInitialMode_ShouldEnterSleep(void);
extern void test_LinTrcv_MainFunction_PinWakeupWithEcuM_ShouldNotify(void);

/* Second round of branch-coverage tests */
extern void test_LinTrcv_SetOpMode_WithDelay_ShouldExerciseDelayLoop(void);
extern void test_LinTrcv_Wakeup_WithDelay_ShouldExerciseSleepToNormalDelay(void);
extern void test_LinTrcv_CheckWakeup_NoBusWakeupEnabled_ShouldSkipBusCheck(void);
extern void test_LinTrcv_CheckWakeup_NoPinWakeupEnabled_ShouldSkipPinCheck(void);
extern void test_LinTrcv_CheckWakeup_NoNerrPin_ShouldSkipErrorCheck(void);
extern void test_LinTrcv_MainFunction_SleepModeBusWakeup_ShouldDetect(void);
extern void test_LinTrcv_MainFunction_SleepModeNoWakeup_ShouldNotDetect(void);
extern void test_LinTrcv_GetOpMode_AfterStandby_ShouldReturnStandby(void);
extern void test_LinTrcv_GetOpMode_AfterSleep_ShouldReturnSleep(void);
extern void test_LinTrcv_SetOpMode_StandbyNoPinWakeup_ShouldSucceed(void);
extern void test_LinTrcv_MainFunction_NoPinWakeupEnabled_ShouldSkipPinCheck(void);
extern void test_LinTrcv_CheckWakeup_NWakeInactive_ShouldNotDetectPinWakeup(void);
extern void test_LinTrcv_Init_InvalidInitialMode_ShouldDefaultToNormal(void);
extern void test_LinTrcv_MainFunction_BusWakeupNoEcuM_ShouldNotNotify(void);

int main(void) {
    UNITY_BEGIN();

    /* Lifecycle & version */
    RUN_TEST(test_LinTrcv_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_LinTrcv_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_LinTrcv_Init_DoubleInit_ShouldSucceed);
    RUN_TEST(test_LinTrcv_DeInit_Uninit_ShouldBeSilentNoOp);
    RUN_TEST(test_LinTrcv_DeInit_ValidCall_ShouldResetState);
    RUN_TEST(test_LinTrcv_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetVersionInfo_ValidPtr_ShouldReturnVersion);

    /* Operation mode */
    RUN_TEST(test_LinTrcv_SetOpMode_Uninit_ShouldReportError);
    RUN_TEST(test_LinTrcv_SetOpMode_InvalidChannel_ShouldReportError);
    RUN_TEST(test_LinTrcv_SetOpMode_InvalidOpMode_ShouldReportError);
    RUN_TEST(test_LinTrcv_SetOpMode_ValidCall_ShouldDriveEnPin);
    RUN_TEST(test_LinTrcv_GetOpMode_Uninit_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetOpMode_InvalidChannel_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetOpMode_NullPtr_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetOpMode_ValidCall_ShouldReturnMode);

    /* Wake-up reason */
    RUN_TEST(test_LinTrcv_GetBusWuReason_Uninit_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetBusWuReason_InvalidChannel_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetBusWuReason_ValidCall_ShouldReturnReason);

    /* Main function & wake-up detection */
    RUN_TEST(test_LinTrcv_MainFunction_Uninit_ShouldBeSilentNoOp);
    RUN_TEST(test_LinTrcv_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_LinTrcv_MainFunction_PinWakeup_ShouldSetReasonByPin);
    RUN_TEST(test_LinTrcv_CheckWakeup_NoEvent_ShouldReturnNotOk);
    RUN_TEST(test_LinTrcv_Cbk_WakeupByBus_ShouldSetPendingEvent);

    /* Additional branch-coverage tests */
    RUN_TEST(test_LinTrcv_SetOpMode_StandbyMode_ShouldDriveEnLow);
    RUN_TEST(test_LinTrcv_SetOpMode_TJA1028_ShouldSucceed);
    RUN_TEST(test_LinTrcv_SetOpMode_Generic_ShouldSucceed);
    RUN_TEST(test_LinTrcv_GetOpMode_TJA1028_ShouldReturnMode);
    RUN_TEST(test_LinTrcv_GetOpMode_Generic_ShouldReturnMode);
    RUN_TEST(test_LinTrcv_Wakeup_Uninit_ShouldReportError);
    RUN_TEST(test_LinTrcv_Wakeup_InvalidChannel_ShouldReportError);
    RUN_TEST(test_LinTrcv_Wakeup_FromSleep_ShouldTransitionToNormal);
    RUN_TEST(test_LinTrcv_CheckWakeup_Uninit_ShouldReportError);
    RUN_TEST(test_LinTrcv_CheckWakeup_InvalidChannel_ShouldReportError);
    RUN_TEST(test_LinTrcv_CheckWakeup_WithWakeupSource_ShouldNotifyEcuM);
    RUN_TEST(test_LinTrcv_Cbk_WakeupByBus_Uninit_ShouldReturnSilently);
    RUN_TEST(test_LinTrcv_Cbk_WakeupByBus_InvalidChannel_ShouldReportError);
    RUN_TEST(test_LinTrcv_Cbk_WakeupByBus_WithWakeupSource_ShouldNotifyEcuM);
    RUN_TEST(test_LinTrcv_GetBusWuReason_NullPtr_ShouldReportError);
    RUN_TEST(test_LinTrcv_GetBusWuReason_AfterSysErr_ShouldReturnSysErr);
    RUN_TEST(test_LinTrcv_Init_StandbyInitialMode_ShouldEnterStandby);
    RUN_TEST(test_LinTrcv_Init_SleepInitialMode_ShouldEnterSleep);
    RUN_TEST(test_LinTrcv_MainFunction_PinWakeupWithEcuM_ShouldNotify);

    /* Second round of branch-coverage tests */
    RUN_TEST(test_LinTrcv_SetOpMode_WithDelay_ShouldExerciseDelayLoop);
    RUN_TEST(test_LinTrcv_Wakeup_WithDelay_ShouldExerciseSleepToNormalDelay);
    RUN_TEST(test_LinTrcv_CheckWakeup_NoBusWakeupEnabled_ShouldSkipBusCheck);
    RUN_TEST(test_LinTrcv_CheckWakeup_NoPinWakeupEnabled_ShouldSkipPinCheck);
    RUN_TEST(test_LinTrcv_CheckWakeup_NoNerrPin_ShouldSkipErrorCheck);
    RUN_TEST(test_LinTrcv_MainFunction_SleepModeBusWakeup_ShouldDetect);
    RUN_TEST(test_LinTrcv_MainFunction_SleepModeNoWakeup_ShouldNotDetect);
    RUN_TEST(test_LinTrcv_GetOpMode_AfterStandby_ShouldReturnStandby);
    RUN_TEST(test_LinTrcv_GetOpMode_AfterSleep_ShouldReturnSleep);
    RUN_TEST(test_LinTrcv_SetOpMode_StandbyNoPinWakeup_ShouldSucceed);
    RUN_TEST(test_LinTrcv_MainFunction_NoPinWakeupEnabled_ShouldSkipPinCheck);
    RUN_TEST(test_LinTrcv_CheckWakeup_NWakeInactive_ShouldNotDetectPinWakeup);
    RUN_TEST(test_LinTrcv_Init_InvalidInitialMode_ShouldDefaultToNormal);
    RUN_TEST(test_LinTrcv_MainFunction_BusWakeupNoEcuM_ShouldNotNotify);

    return UnityEnd();
}
