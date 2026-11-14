/**
 * @file test_os_benchmark.c
 * @brief P1 Phase 8 — OS-layer performance benchmark (host build).
 *
 * Measures, with clock_gettime(CLOCK_MONOTONIC) statistics (min/max/avg/
 * p95/p99 via perf_stats.h):
 *   1. clock call overhead               — sanity reference for all campaigns
 *   2. task context-switch time          — swapcontext() ping-pong between the
 *                                          Unity main context and a dedicated
 *                                          task context (one round trip = two
 *                                          switches; per-switch = rt / 2)
 *   3. interrupt disable/enable latency  — Mcal_DisableAllInterrupts() /
 *                                          Mcal_EnableAllInterrupts() call
 *                                          latency and the total IRQ blackout
 *                                          window per disable-enable pair
 *   4. MainFunction scheduling jitter    — 1 ms tick loop scheduled with
 *                                          nanosleep (absolute targets), real
 *                                          Com_MainFunctionRx/Tx executed
 *                                          every tick; jitter = wake-up
 *                                          overshoot past the tick deadline
 *
 * The production Com.c and Com_Cfg.h are used directly; the benchmark
 * supplies one active signal and I-PDU in osComConfig.
 *
 * @note Platform: this is a host-side benchmark (macOS/Linux). On Apple hosts
 *       Mcal_DisableAllInterrupts/Enable are user-space no-ops by design (the
 *       DAIF register is not accessible), so campaigns 3 measure the harness
 *       floor; on aarch64 non-Apple and Cortex-M targets the same binary
 *       measures the real PRIMASK/DAIF instructions.
 */

/* macOS: the deprecated ucontext routines require _XOPEN_SOURCE to be
 * defined before ANY system header is included. */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include "unity.h"
#include "Com.h"
#include "PduR.h"
#include "Mcal.h"
#include "perf_stats.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>

/* ------------------------------------------------------------------ */
/* Dependency stubs                                                    */
/* ------------------------------------------------------------------ */

static uint32_t mock_DetCalls = 0u;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    mock_DetCalls++;
    return E_OK;
}

static uint32_t mock_PduRTransmitCount = 0u;

Std_ReturnType PduR_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    mock_PduRTransmitCount++;
    return E_OK;
}

/* ------------------------------------------------------------------ */
/* Minimal Com configuration (one PENDING signal on one I-PDU) so the  */
/* scheduling-jitter campaign drives the real MainFunctions.           */
/* ------------------------------------------------------------------ */

static const Com_SignalConfigType osSignals[1] = {
    { 0U, 0U, 8U, COM_LITTLE_ENDIAN, COM_PENDING, COM_ALWAYS, 0U, 0U, 0U }
};
static const Com_IPduConfigType osIPdus[1] = {
    { 0U, 8U, FALSE, 0U, 0U, 0U, 0U }
};
static const Com_ConfigType osComConfig = { osSignals, 1U, osIPdus, 1U };

/* ------------------------------------------------------------------ */
/* Campaigns                                                           */
/* ------------------------------------------------------------------ */

static perf_stats_t g_timer_overhead;
static perf_stats_t g_task_switch;
static perf_stats_t g_irq_disable;
static perf_stats_t g_irq_enable;
static perf_stats_t g_irq_blackout;
static perf_stats_t g_mainfn_jitter;

void setUp(void)
{
    mock_DetCalls = 0u;
    mock_PduRTransmitCount = 0u;
}

void tearDown(void)
{
}

/* --- 0: clock call overhead — reference floor for every campaign --- */
void test_00_TimerCallOverhead(void)
{
    const uint32_t iterations = 5000u;
    uint32_t i;

    for (i = 0u; i < iterations; i++) {
        uint64_t t0 = perf_now_ns();
        uint64_t t1 = perf_now_ns();
        perf_record(&g_timer_overhead, t1 - t0);
    }

    perf_report("clock_gettime(CLOCK_MONOTONIC) call overhead (reference floor)", &g_timer_overhead);
    /* sanity: the clock must be usable (call cost far below 1 ms) */
    TEST_ASSERT_TRUE(perf_avg_ns(&g_timer_overhead) < 1000000ULL);
}

/* --- 1: task context-switch time (swapcontext ping-pong) ----------- */

#define OSB_TASK_STACK_SIZE (128u * 1024u)

static ucontext_t g_ctx_main;
static ucontext_t g_ctx_task;
static void* g_task_stack = NULL;

/** Task body: immediately bounce back to the main context, forever. */
static void task_ping_entry(void)
{
    for (;;) {
        (void)swapcontext(&g_ctx_task, &g_ctx_main);
    }
}

void test_10_TaskSwitchTime_swapcontext(void)
{
    const uint32_t switches = 5000u;
    uint32_t i;

    if (g_task_stack == NULL) {
        g_task_stack = malloc(OSB_TASK_STACK_SIZE);
        TEST_ASSERT_NOT_NULL(g_task_stack);
        (void)getcontext(&g_ctx_task);
        g_ctx_task.uc_stack.ss_sp = g_task_stack;
        g_ctx_task.uc_stack.ss_size = OSB_TASK_STACK_SIZE;
        g_ctx_task.uc_stack.ss_flags = 0;
        g_ctx_task.uc_link = &g_ctx_main;
        makecontext(&g_ctx_task, task_ping_entry, 0);
    }

    /* Every main-side swapcontext performs main->task->main, i.e. a full
     * round trip of two context switches. */
    for (i = 0u; i < switches; i++) {
        uint64_t t0 = perf_now_ns();
        (void)swapcontext(&g_ctx_main, &g_ctx_task);
        uint64_t t1 = perf_now_ns();
        perf_record(&g_task_switch, (t1 - t0) / 2u);
    }

    perf_report("OS task context switch (swapcontext, per-switch = round-trip/2)", &g_task_switch);
    perf_note("       (host userspace measure; on-target DWT cycle counter recommended for ECU budgets)");
    /* sanity: a userspace context switch must stay far below 100 us */
    TEST_ASSERT_TRUE(perf_avg_ns(&g_task_switch) < 100000ULL);
}

/* --- 2: interrupt disable/enable latency --------------------------- */
void test_20_IrqDisableEnableLatency(void)
{
    const uint32_t iterations = 10000u;
    uint32_t i;

    for (i = 0u; i < iterations; i++) {
        uint64_t t0 = perf_now_ns();
        Mcal_DisableAllInterrupts();
        uint64_t t1 = perf_now_ns();
        Mcal_EnableAllInterrupts();
        uint64_t t2 = perf_now_ns();

        perf_record(&g_irq_disable, t1 - t0);
        perf_record(&g_irq_enable, t2 - t1);
        perf_record(&g_irq_blackout, t2 - t0);
    }

    perf_report("Mcal_DisableAllInterrupts() call latency", &g_irq_disable);
    perf_report("Mcal_EnableAllInterrupts() call latency", &g_irq_enable);
    perf_report("interrupt blackout window (disable..enable pair)", &g_irq_blackout);
    /* sanity: the critical section must be far below 1 ms */
    TEST_ASSERT_TRUE(perf_avg_ns(&g_irq_blackout) < 1000000ULL);
}

/* --- 3: MainFunction scheduling jitter ------------------------------ */
void test_30_MainFunctionSchedulingJitter(void)
{
    const uint32_t ticks = 1000u;
    const uint64_t period_ns = 1000000u; /* 1 ms tick */
    uint64_t start;
    uint32_t i;

    Com_Init(&osComConfig);
    start = perf_now_ns();

    for (i = 1u; i <= ticks; i++) {
        uint64_t target = start + ((uint64_t)i * period_ns);
        int64_t jitter;

        /* sleep until the absolute tick deadline (nanosleep; macOS has no
         * clock_nanosleep) */
        for (;;) {
            uint64_t now = perf_now_ns();
            struct timespec ts;
            uint64_t delta;
            if (now >= target) { break; }
            delta = target - now;
            ts.tv_sec = (time_t)(delta / 1000000000ULL);
            ts.tv_nsec = (long)(delta % 1000000000ULL);
            (void)nanosleep(&ts, NULL);
        }

        jitter = (int64_t)(perf_now_ns() - target);
        if (jitter < 0) { jitter = 0; }
        perf_record(&g_mainfn_jitter, (uint64_t)jitter);

        /* real BSW scheduler load every tick */
        Com_MainFunctionRx();
        Com_MainFunctionTx();
    }

    perf_report("Com MainFunction scheduling jitter vs 1 ms tick (overshoot)", &g_mainfn_jitter);
    perf_note("       (host sleep resolution dominates; on-target this maps to the Os alarm/counter tick)");
    /* sanity: p99 overshoot below 5 ms even on a loaded CI host */
    TEST_ASSERT_TRUE(perf_pctl_ns(&g_mainfn_jitter, 99u) < 5000000ULL);
}

/* ------------------------------------------------------------------ */
/* Runner                                                              */
/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_00_TimerCallOverhead);
    RUN_TEST(test_10_TaskSwitchTime_swapcontext);
    RUN_TEST(test_20_IrqDisableEnableLatency);
    RUN_TEST(test_30_MainFunctionSchedulingJitter);

    return UNITY_END();
}
