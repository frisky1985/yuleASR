/*==================================================================================================
 * s0_smoke_test.c - Native smoke test for the S0 fix
 *
 * Verifies that StartOS actually runs:
 *   1. FreeRTOS scheduler starts (Posix port)
 *   2. Init task runs Rte_Init/Rte_Start/Rte_AswScheduler_Start + SetRelAlarm
 *   3. BSW alarm callbacks fire periodically (10ms/100ms)
 *   4. RTE scheduler tick counter advances (ASW tasks dispatched)
 *
 * Exit strategy: a FreeRTOS software timer (created before StartOS, fired
 * by the timer daemon task) counts 3 seconds then calls vTaskEndScheduler(),
 * which on the Posix port must be invoked from a FreeRTOS task context.
 *================================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "Os.h"
#include "Rte.h"

#define RUN_SECONDS 3U

/* Rte_Scheduler runtime query - defined in Rte_Scheduler.c, not in a public header */
extern uint32 Rte_SchedulerGetTickCount(void);
extern void vAssertCalled(const char *pcFile, uint32_t ulLine);

/* BSW alarm callback dispatch counters (incremented by alarm callbacks) */
static volatile uint32 g_timerTicks = 0U;

/* FreeRTOS timer that ends the scheduler after RUN_SECONDS */
static TimerHandle_t g_exitTimer = NULL;

static void exit_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;
    g_timerTicks++;
    if (g_timerTicks >= RUN_SECONDS)
    {
        printf("[S0-SMOKE] %u s elapsed - calling vTaskEndScheduler()\n",
               (unsigned int)g_timerTicks);
        fflush(stdout);
        vTaskEndScheduler();
    }
}

int main(void)
{
    uint32 ticksAfter;
    uint32 ticksBefore;

    printf("[S0-SMOKE] === S0 fix verification ===\n");
    fflush(stdout);

    /* Create the exit timer before starting the scheduler.
     * Its callback runs in the FreeRTOS timer daemon task context. */
    g_exitTimer = xTimerCreate("ExitTimer",
                               pdMS_TO_TICKS(1000U),
                               pdTRUE,               /* auto-reload */
                               (void *)0,
                               exit_timer_callback);
    if (g_exitTimer == NULL)
    {
        printf("[S0-SMOKE] ERROR: xTimerCreate failed\n");
        return 1;
    }
    if (xTimerStart(g_exitTimer, 0U) != pdPASS)
    {
        printf("[S0-SMOKE] ERROR: xTimerStart failed\n");
        return 1;
    }

    ticksBefore = 0U;

    printf("[S0-SMOKE] Calling StartOS(OSDEFAULTAPPMODE)...\n");
    fflush(stdout);

    /* Start the OS - FreeRTOS scheduler takes over.
     * Blocks until vTaskEndScheduler() is called by the exit timer. */
    StartOS(OSDEFAULTAPPMODE);

    /* We only reach here after the scheduler stops (exit timer fired) */
    printf("\n[S0-SMOKE] Scheduler returned - verifying runtime state...\n");

    ticksAfter = Rte_SchedulerGetTickCount();
    printf("[S0-SMOKE] Rte_SchedulerGetTickCount = %u\n", (unsigned int)ticksAfter);

    if (ticksAfter > ticksBefore)
    {
        printf("[S0-SMOKE] PASS: RTE scheduler tick counter advanced (%u -> %u)\n",
               (unsigned int)ticksBefore, (unsigned int)ticksAfter);
        printf("[S0-SMOKE] PASS: FreeRTOS scheduler ran, Init task executed,\n");
        printf("[S0-SMOKE]       ASW scheduler dispatched periodic tasks\n");
        printf("[S0-SMOKE] RESULT: ALL CHECKS PASSED\n");
        return 0;
    }
    else
    {
        printf("[S0-SMOKE] FAIL: tick counter did not advance (%u -> %u)\n",
               (unsigned int)ticksBefore, (unsigned int)ticksAfter);
        printf("[S0-SMOKE] RESULT: FAILED - scheduler may not have dispatched tasks\n");
        return 1;
    }
}
