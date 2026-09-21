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

    return UnityEnd();
}
