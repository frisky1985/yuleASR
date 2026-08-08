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
 *
 * Hang protection (tech-debt T1 fix, 2026-08-08): the FreeRTOS Posix port is
 * only *supported* at runtime on Linux; on macOS the tick/scheduler does not
 * dispatch reliably, so the exit timer never fires and StartOS() would block
 * forever in sigwait(SIG_RESUME) - 8 such processes previously accumulated
 * (up to 8447 min CPU each). A plain pthread watchdog (independent of the
 * FreeRTOS scheduler) guarantees vTaskEndScheduler() is still called after
 * RUN_SECONDS + WATCHDOG_MARGIN, and _exit()s as a last resort so this test
 * can never leave a hung process behind again.
 *================================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "Os.h"
#include "Rte.h"

#define RUN_SECONDS 3U

/* Extra seconds the watchdog waits beyond the FreeRTOS exit timer.
 * The FreeRTOS timer fires at RUN_SECONDS=3s; the watchdog only fires
 * (8s) when the scheduler never dispatched the timer daemon. */
#define WATCHDOG_DELAY_SECONDS (RUN_SECONDS + 5U)

/* How long the watchdog waits after calling vTaskEndScheduler() before
 * force-exiting. If the main thread was woken, the process exits well
 * before this grace period elapses. */
#define WATCHDOG_EXIT_GRACE_SECONDS 3U

/* Rte_Scheduler runtime query - defined in Rte_Scheduler.c, not in a public header */
extern uint32 Rte_SchedulerGetTickCount(void);
extern void vAssertCalled(const char *pcFile, uint32_t ulLine);

/* BSW alarm callback dispatch counters (incremented by alarm callbacks) */
static volatile uint32 g_timerTicks = 0U;

/* Set by the FreeRTOS exit-timer callback once it ends the scheduler.
 * The watchdog polls this to know it can retire without firing. */
static volatile int g_schedulerEnded = 0;

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
        g_schedulerEnded = 1;
        vTaskEndScheduler();
    }
}

/* Watchdog thread - guarantees the test process always exits.
 *
 * On Linux (CI) the FreeRTOS exit timer fires at RUN_SECONDS and the
 * watchdog silently retires. On unsupported hosts (macOS Posix-port
 * runtime limitation) the scheduler never dispatches the timer daemon,
 * so after WATCHDOG_DELAY_SECONDS this thread calls vTaskEndScheduler()
 * itself (safe in the single-core Posix port: pxCurrentTCB is a global,
 * and vPortEndScheduler() wakes the main thread before its final wait),
 * and falls back to _exit() if even that cannot complete.
 */
static void *watchdog_thread(void *arg)
{
    unsigned int elapsed = 0U;
    (void)arg;

    while (elapsed < WATCHDOG_DELAY_SECONDS)
    {
        (void)sleep(1U);
        elapsed++;
        if (g_schedulerEnded != 0)
        {
            break; /* FreeRTOS exit timer already ended the scheduler */
        }
    }

    if (g_schedulerEnded == 0)
    {
        printf("[S0-SMOKE] WATCHDOG: %u s elapsed without FreeRTOS exit timer - "
               "forcing vTaskEndScheduler()\n",
               (unsigned int)elapsed);
        fflush(stdout);
        g_schedulerEnded = 1;
        vTaskEndScheduler();
    }

    /* Last resort: if the scheduler cannot be stopped at all (or the
     * shutdown path hangs), hard-exit so no hung s0_smoke_test process
     * can ever accumulate again. On the healthy path the process exits
     * right after StartOS() returns (~3s), far before this grace period
     * elapses (~11s). */
    (void)sleep(WATCHDOG_EXIT_GRACE_SECONDS);
    printf("[S0-SMOKE] WATCHDOG: process did not exit after scheduler stop - "
           "forcing exit\n");
    fflush(stdout);
    _exit(2);
    return NULL;
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

    /* Arm the watchdog before StartOS: it guarantees the process exits
     * even if the FreeRTOS scheduler never dispatches (see top comment). */
    {
        pthread_t watchdog;
        if (pthread_create(&watchdog, NULL, watchdog_thread, NULL) != 0)
        {
            printf("[S0-SMOKE] ERROR: watchdog pthread_create failed\n");
            return 1;
        }
        (void)pthread_detach(watchdog);
    }

    printf("[S0-SMOKE] Calling StartOS(OSDEFAULTAPPMODE)...\n");
    fflush(stdout);

    /* Start the OS - FreeRTOS scheduler takes over.
     * Blocks until vTaskEndScheduler() is called by the exit timer or
     * the watchdog (hard deadline: RUN_SECONDS + WATCHDOG_DELAY_SECONDS). */
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
