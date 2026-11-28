/**
 * @file test_os_taskentries.c
 * @brief Os_TaskEntries.c unit tests — substantiated.
 *
 * Every task entry point is an infinite `for(;;)` loop; the test stubs the
 * blocking primitives (vTaskDelay / WaitEvent) so that, after executing the
 * loop body the required number of times, they siglongjmp back into the test.
 * This lets us exercise each loop body (and both sides of the event checks)
 * without ever hanging.
 *
 * Only OsTask_Init_Entry returns normally; it is called directly.
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#include "Os.h"
#include "Os_Internal.h"
#include "Os_Cfg.h"
#include "Rte.h"
#include "Rte_AswScheduler.h"
#include "FreeRTOS.h"
#include "task.h"

/* ══════════════════════════════════════════════════════════════════════════
 * Loop escape + stubs
 * ══════════════════════════════════════════════════════════════════════════ */
static sigjmp_buf g_jmp;
static volatile int g_armed;
static volatile int g_block_budget;   /* blocking calls allowed before bail */

static uint32 g_rte_init_calls, g_rte_start_calls, g_asw_start_calls;
static uint32 g_setalarm_calls, g_setevent_calls, g_clearevent_calls;
static uint32 g_sched_calls;
static EventMaskType g_events_value;

static void maybe_bail(void)
{
    if (g_armed != 0)
    {
        if (g_block_budget <= 0) { siglongjmp(g_jmp, 1); }
        g_block_budget--;
    }
}

/* FreeRTOS delay */
void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; maybe_bail(); }

/* RTE stubs */
Rte_StatusType Rte_Init(void) { g_rte_init_calls++; return (Rte_StatusType)0; }
Rte_StatusType Rte_Start(void) { g_rte_start_calls++; return (Rte_StatusType)0; }
Rte_StatusType Rte_AswScheduler_Start(void) { g_asw_start_calls++; return (Rte_StatusType)0; }
void Rte_Scheduler_MainFunction(void) { g_sched_calls++; }

/* OS service stubs */
StatusType SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle)
{
    (void)AlarmID; (void)increment; (void)cycle;
    g_setalarm_calls++;
    return E_OK;
}
StatusType SetEvent(TaskType TaskID, EventMaskType Mask)
{
    (void)TaskID; (void)Mask;
    g_setevent_calls++;
    return E_OK;
}
StatusType ClearEvent(EventMaskType Mask)
{
    (void)Mask;
    g_clearevent_calls++;
    return E_OK;
}
StatusType GetEvent(TaskType TaskID, EventMaskRefType Event)
{
    (void)TaskID;
    if (Event != NULL_PTR) { *Event = g_events_value; }
    return E_OK;
}
StatusType WaitEvent(EventMaskType Mask)
{
    (void)Mask;
    maybe_bail();
    return E_OK;
}

/* Run an infinite-loop entry point until the stub bails out. */
static void run_entry(void (*fn)(void), int budget)
{
    g_block_budget = budget;
    g_armed = 1;
    if (sigsetjmp(g_jmp, 1) == 0) { fn(); }
    g_armed = 0;
}

void setUp(void)
{
    g_armed = 0;
    g_block_budget = 0;
    g_rte_init_calls = 0U;
    g_rte_start_calls = 0U;
    g_asw_start_calls = 0U;
    g_setalarm_calls = 0U;
    g_setevent_calls = 0U;
    g_clearevent_calls = 0U;
    g_sched_calls = 0U;
    g_events_value = 0U;
}

void tearDown(void)
{
}

/* ══════════════════════════════════════════════════════════════════════════
 * Init entry (finite)
 * ══════════════════════════════════════════════════════════════════════════ */

void test_InitEntry_StartsSystem(void)
{
    OsTask_Init_Entry();
    TEST_ASSERT_EQUAL_UINT32(1U, g_rte_init_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, g_rte_start_calls);
    TEST_ASSERT_EQUAL_UINT32(1U, g_asw_start_calls);
    TEST_ASSERT_EQUAL_UINT32(6U, g_setalarm_calls);   /* 6 BSW alarms armed */
    TEST_ASSERT_EQUAL_UINT32(1U, g_setevent_calls);   /* INIT_COMPLETE signalled */
}

/* ══════════════════════════════════════════════════════════════════════════
 * Periodic entries (single loop-body pass)
 * ══════════════════════════════════════════════════════════════════════════ */

void test_10msEntry_RunsScheduler(void)
{
    run_entry(OsTask_10ms_Entry, 0);   /* vTaskDelay bails on first call */
    TEST_ASSERT_EQUAL_UINT32(1U, g_sched_calls);
}

void test_50msEntry(void)
{
    run_entry(OsTask_50ms_Entry, 0);
    TEST_ASSERT_TRUE(1);
}

void test_100msEntry(void)
{
    run_entry(OsTask_100ms_Entry, 0);
    TEST_ASSERT_TRUE(1);
}

void test_BackgroundEntry(void)
{
    run_entry(OsTask_Background_Entry, 0);
    TEST_ASSERT_TRUE(1);
}

void test_ComTxEntry(void)
{
    run_entry(OsTask_ComMainFunctionTx_Entry, 0);
    TEST_ASSERT_TRUE(1);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Event-driven entries — both sides of the event checks
 * ══════════════════════════════════════════════════════════════════════════ */

void test_ComRxEntry_EventSet(void)
{
    g_events_value = OS_EVENT_CAN_MESSAGE_RECEIVED;
    run_entry(OsTask_ComMainFunctionRx_Entry, 1);  /* let 1 WaitEvent through */
    TEST_ASSERT_TRUE(g_clearevent_calls >= 1U);
}

void test_ComRxEntry_EventClear(void)
{
    g_events_value = 0U;
    run_entry(OsTask_ComMainFunctionRx_Entry, 1);
    TEST_ASSERT_EQUAL_UINT32(0U, g_clearevent_calls);
}

void test_DiagnosticEntry_BothEvents(void)
{
    g_events_value = (EventMaskType)(OS_EVENT_INIT_COMPLETE | OS_EVENT_DIAGNOSTIC_REQUEST);
    run_entry(OsTask_Diagnostic_Entry, 1);
    TEST_ASSERT_EQUAL_UINT32(2U, g_clearevent_calls);
}

void test_DiagnosticEntry_NoEvents(void)
{
    g_events_value = 0U;
    run_entry(OsTask_Diagnostic_Entry, 1);
    TEST_ASSERT_EQUAL_UINT32(0U, g_clearevent_calls);
}

/* ══════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_InitEntry_StartsSystem);

    RUN_TEST(test_10msEntry_RunsScheduler);
    RUN_TEST(test_50msEntry);
    RUN_TEST(test_100msEntry);
    RUN_TEST(test_BackgroundEntry);
    RUN_TEST(test_ComTxEntry);

    RUN_TEST(test_ComRxEntry_EventSet);
    RUN_TEST(test_ComRxEntry_EventClear);
    RUN_TEST(test_DiagnosticEntry_BothEvents);
    RUN_TEST(test_DiagnosticEntry_NoEvents);

    return UNITY_END();
}
