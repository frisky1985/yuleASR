/* test_os_timing_coverage.c — OS timing protection coverage driver (ASIL-D)
 *
 * Exercises the real src/bsw/os/src/Os_TimingProtection.c implementation
 * (task execution / arrival / resource-lock / interrupt-lock budgets)
 * using a host stub for the FreeRTOS tick source (asil_stubs.c).
 */
#include <stdio.h>
#include <string.h>
#include "Os.h"
#include "Os_TimingProtection_Cfg.h"
#include "FreeRTOS.h"
#include "task.h"
#include "asil_stubs.h"

static int t_pass = 0;
static int t_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { t_pass++; } else { t_fail++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } } while (0)

int main(void)
{
    printf("=== OS Timing Protection Coverage Driver ===\n");

    /* Task execution timing — normal + exceeded budgets */
    asil_rtos_tick = 0U;
    Os_StartTaskExecutionTiming(0U);
    asil_rtos_tick = 1000000U;  /* > 100ms budget */
    Os_CheckTaskExecutionBudget(0U);
    Os_StopTaskExecutionTiming(0U);

    /* Out-of-range task id path */
    Os_StartTaskExecutionTiming(0xFFU);
    Os_CheckTaskExecutionBudget(0xFFU);
    Os_StopTaskExecutionTiming(0xFFU);

    /* Arrival-time protection — normal + too-fast */
    asil_rtos_tick = 0U;
    Os_StartTaskExecutionTiming(1U);
    Os_CheckTaskArrivalTime(1U);
    asil_rtos_tick = 1000U;  /* 1ms < 10ms min interval -> too fast */
    Os_CheckTaskArrivalTime(1U);
    Os_StopTaskExecutionTiming(1U);

    /* Resource lock timing */
    Os_StartResourceTiming(0U);
    asil_rtos_tick = 60000U;  /* > 50ms lock budget */
    Os_CheckResourceLockBudget(0U);
    Os_StopResourceTiming(0U);

    /* Interrupt lock timing — all interrupts + OS interrupts */
    Os_StartAllIntTiming();
    asil_rtos_tick = 20000U;  /* > 10ms budget */
    Os_CheckInterruptLockBudgets();
    Os_StopAllIntTiming();

    Os_StartOsIntTiming();
    asil_rtos_tick = 60000U;  /* > 50ms budget */
    Os_CheckInterruptLockBudgets();
    Os_StopOsIntTiming();

    /* Main function */
    Os_TimingProtectionMainFunction();

    /* Normal-path re-run with small ticks (no exceed) */
    asil_rtos_tick = 0U;
    Os_StartTaskExecutionTiming(2U);
    asil_rtos_tick = 10U;
    Os_CheckTaskExecutionBudget(2U);
    Os_StopTaskExecutionTiming(2U);
    Os_StartResourceTiming(1U);
    asil_rtos_tick = 20U;
    Os_CheckResourceLockBudget(1U);
    Os_StopResourceTiming(1U);
    Os_StartAllIntTiming();
    asil_rtos_tick = 30U;
    Os_CheckInterruptLockBudgets();
    Os_StopAllIntTiming();

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
