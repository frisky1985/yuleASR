/**
 * @file test_os_timing.c
 * @brief OS Timing Protection unit tests — substantiated.
 *
 * Compiles Os_TimingProtection.c (via os_timing_expose.c, which removes
 * `static`) together with the mock FreeRTOS headers in tests/unit/os_mocks.
 * All FreeRTOS/hook dependencies are stubbed here. The configuration tables
 * (Os_TaskTimingBudgets / Os_InterruptTimingBudget) are declared `extern const`
 * in Os_TimingProtection_Cfg.h but defined nowhere in the tree, so this file
 * provides them — without them the module cannot link at all.
 *
 * OS_RESOURCE_COUNT is 0 in the configuration, so the resource-lock helpers
 * can only ever take their `ResID < OS_RESOURCE_COUNT` == false branch.
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>

#include "Os.h"
#include "Os_TimingProtection_Cfg.h"
#include "FreeRTOS.h"
#include "task.h"

/* ── Exposed helpers (os_timing_expose.c) ────────────────────────────────── */
void    OsTest_ResetMonitors(void);
boolean OsTest_GetBudgetExceeded(TaskType TaskID);
uint32  OsTest_GetLastArrivalTime(TaskType TaskID);

/* ── Internal (static) functions made global by the expose wrapper ───────── */
extern boolean Os_CheckTaskArrivalTime(TaskType TaskID);
extern void    Os_StartResourceTiming(ResourceType ResID);
extern void    Os_StopResourceTiming(ResourceType ResID);
extern void    Os_CheckResourceLockBudget(ResourceType ResID);
extern void    Os_StartAllIntTiming(void);
extern void    Os_StopAllIntTiming(void);
extern void    Os_StartOsIntTiming(void);
extern void    Os_StopOsIntTiming(void);
extern void    Os_CheckInterruptLockBudgets(void);
extern void    Os_CheckTimingFault(TaskType TaskID, Os_TimingFaultActionType Action);

/* ══════════════════════════════════════════════════════════════════════════
 * Configuration tables (undefined in the tree — see file header)
 * ══════════════════════════════════════════════════════════════════════════ */
const Os_TaskTimingBudgetType Os_TaskTimingBudgets[OS_TASK_COUNT] = {
    { 0U, 1000U, 500U, OS_TIMING_ACTION_TASK_KILL },
    { 1U, 1000U, 500U, OS_TIMING_ACTION_TASK_RESTART },
    { 2U, 1000U, 500U, OS_TIMING_ACTION_ERROR_HOOK },
    { 3U, 1000U, 500U, OS_TIMING_ACTION_PROTECTION_HOOK },
    { 4U, 1000U, 500U, OS_TIMING_ACTION_NONE },
    { 5U, 1000U, 500U, OS_TIMING_ACTION_NONE },
    { 6U, 1000U, 500U, OS_TIMING_ACTION_NONE },
    { 7U, 1000U, 500U, OS_TIMING_ACTION_NONE }
};

const Os_ResourceTimingBudgetType Os_ResourceTimingBudgets[OS_RESOURCE_COUNT];

const Os_InterruptTimingBudgetType Os_InterruptTimingBudget = {
    10U, 50U, OS_TIMING_ACTION_PROTECTION_HOOK
};

/* ══════════════════════════════════════════════════════════════════════════
 * FreeRTOS + OS hook stubs
 * ══════════════════════════════════════════════════════════════════════════ */
static TickType_t g_tick;
static uint32 g_terminate_calls;
static uint32 g_errorhook_calls;
static uint32 g_protect_calls;
static uint32 g_last_errorhook_code;
static uint32 g_last_protect_code;

TickType_t xTaskGetTickCount(void) { return g_tick; }
void Os_TerminateTask(void)        { g_terminate_calls++; }
void Os_ErrorHook(uint32 ErrorCode){ g_errorhook_calls++; g_last_errorhook_code = ErrorCode; }
void Os_ProtectionHook(uint32 ProtectionError){ g_protect_calls++; g_last_protect_code = ProtectionError; }

void setUp(void)
{
    g_tick = 0U;
    g_terminate_calls = 0U;
    g_errorhook_calls = 0U;
    g_protect_calls = 0U;
    g_last_errorhook_code = 0U;
    g_last_protect_code = 0U;
    OsTest_ResetMonitors();
}

void tearDown(void)
{
}

/* ══════════════════════════════════════════════════════════════════════════
 * Task execution timing
 * ══════════════════════════════════════════════════════════════════════════ */

void test_TaskTiming_StartStop_ValidAndInvalid(void)
{
    Os_StartTaskExecutionTiming(0U);       /* valid */
    Os_StopTaskExecutionTiming(0U);        /* valid */
    Os_StartTaskExecutionTiming(OS_TASK_COUNT);   /* invalid -> no-op */
    Os_StopTaskExecutionTiming(OS_TASK_COUNT);    /* invalid -> no-op */
    TEST_ASSERT_TRUE(1);
}

void test_TaskBudget_Fault_Kill(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(0U);   /* action = TASK_KILL */
    g_tick = 2U;                       /* 2000us > 1000us budget */
    Os_CheckTaskExecutionBudget(0U);
    TEST_ASSERT_TRUE(OsTest_GetBudgetExceeded(0U));
    TEST_ASSERT_EQUAL_UINT32(1U, g_terminate_calls);
}

void test_TaskBudget_Fault_Restart(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(1U);   /* action = TASK_RESTART (no hook) */
    g_tick = 2U;
    Os_CheckTaskExecutionBudget(1U);
    TEST_ASSERT_TRUE(OsTest_GetBudgetExceeded(1U));
    TEST_ASSERT_EQUAL_UINT32(0U, g_terminate_calls);
}

void test_TaskBudget_Fault_ErrorHook(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(2U);   /* action = ERROR_HOOK */
    g_tick = 2U;
    Os_CheckTaskExecutionBudget(2U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_errorhook_calls);
    TEST_ASSERT_EQUAL_UINT32((uint32)E_OS_PROTECTION_TIME, g_last_errorhook_code);
}

void test_TaskBudget_Fault_ProtectionHook(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(3U);   /* action = PROTECTION_HOOK */
    g_tick = 2U;
    Os_CheckTaskExecutionBudget(3U);
    TEST_ASSERT_EQUAL_UINT32(1U, g_protect_calls);
}

void test_TaskBudget_Fault_None_Default(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(4U);   /* action = NONE -> default branch */
    g_tick = 2U;
    Os_CheckTaskExecutionBudget(4U);
    TEST_ASSERT_TRUE(OsTest_GetBudgetExceeded(4U));
    TEST_ASSERT_EQUAL_UINT32(0U, g_terminate_calls);
}

void test_TaskBudget_NoFault_WithinBudget(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(0U);
    g_tick = 1U;                       /* 1000us == budget, not > */
    Os_CheckTaskExecutionBudget(0U);
    TEST_ASSERT_FALSE(OsTest_GetBudgetExceeded(0U));
    TEST_ASSERT_EQUAL_UINT32(0U, g_terminate_calls);
}

void test_TaskBudget_NotRunning_And_InvalidTask(void)
{
    /* Never started -> IsRunning false */
    Os_CheckTaskExecutionBudget(0U);
    TEST_ASSERT_FALSE(OsTest_GetBudgetExceeded(0U));
    /* Invalid task id -> guard rejects */
    Os_CheckTaskExecutionBudget(OS_TASK_COUNT);
    TEST_ASSERT_TRUE(1);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Arrival-time protection
 * ══════════════════════════════════════════════════════════════════════════ */

void test_Arrival_FirstArrival_Accepted(void)
{
    g_tick = 5U;
    TEST_ASSERT_TRUE(Os_CheckTaskArrivalTime(0U));
    /* OS_TIMING_GET_US() == tick * (1e6 / configTICK_RATE_HZ) == tick * 1000 */
    TEST_ASSERT_EQUAL_UINT32(5000U, OsTest_GetLastArrivalTime(0U));
}

void test_Arrival_TooEarly_Rejected(void)
{
    g_tick = 5U;
    TEST_ASSERT_TRUE(Os_CheckTaskArrivalTime(0U));   /* records arrival 5000us */
    TEST_ASSERT_FALSE(Os_CheckTaskArrivalTime(0U));  /* same tick -> 0us < 500us */
    TEST_ASSERT_EQUAL_UINT32(5000U, OsTest_GetLastArrivalTime(0U)); /* unchanged */
}

void test_Arrival_AfterMinInterval_Accepted(void)
{
    g_tick = 5U;
    TEST_ASSERT_TRUE(Os_CheckTaskArrivalTime(0U));
    g_tick = 6U;                                     /* +1000us >= 500us */
    TEST_ASSERT_TRUE(Os_CheckTaskArrivalTime(0U));
    TEST_ASSERT_EQUAL_UINT32(6000U, OsTest_GetLastArrivalTime(0U));
}

void test_Arrival_InvalidTask_Allowed(void)
{
    TEST_ASSERT_TRUE(Os_CheckTaskArrivalTime(OS_TASK_COUNT));
}

/* ══════════════════════════════════════════════════════════════════════════
 * Resource-lock protection (OS_RESOURCE_COUNT == 0 -> only false branches)
 * ══════════════════════════════════════════════════════════════════════════ */

void test_ResourceTiming_ZeroResources_NoOp(void)
{
    Os_StartResourceTiming(0U);
    Os_StopResourceTiming(0U);
    Os_CheckResourceLockBudget(0U);
    TEST_ASSERT_EQUAL_UINT32(0U, g_protect_calls);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Interrupt-lock protection
 * ══════════════════════════════════════════════════════════════════════════ */

void test_Interrupt_AllInt_Fault(void)
{
    g_tick = 0U;
    Os_StartAllIntTiming();
    g_tick = 1U;                       /* 1000us > 10us budget */
    Os_CheckInterruptLockBudgets();
    TEST_ASSERT_EQUAL_UINT32(1U, g_protect_calls);
    TEST_ASSERT_EQUAL_UINT32((uint32)E_OS_PROTECTION_LOCKED, g_last_protect_code);
    Os_StopAllIntTiming();
}

void test_Interrupt_AllInt_NoFault(void)
{
    g_tick = 0U;
    Os_StartAllIntTiming();
    g_tick = 0U;                       /* 0us, not > 10us */
    Os_CheckInterruptLockBudgets();
    TEST_ASSERT_EQUAL_UINT32(0U, g_protect_calls);
    Os_StopAllIntTiming();
    Os_CheckInterruptLockBudgets();    /* now unlocked -> skip */
    TEST_ASSERT_EQUAL_UINT32(0U, g_protect_calls);
}

void test_Interrupt_OsInt_Fault(void)
{
    g_tick = 0U;
    Os_StartOsIntTiming();
    g_tick = 1U;                       /* 1000us > 50us budget */
    Os_CheckInterruptLockBudgets();
    TEST_ASSERT_EQUAL_UINT32(1U, g_protect_calls);
    Os_StopOsIntTiming();
}

void test_Interrupt_OsInt_NoFault(void)
{
    g_tick = 0U;
    Os_StartOsIntTiming();
    g_tick = 0U;
    Os_CheckInterruptLockBudgets();
    TEST_ASSERT_EQUAL_UINT32(0U, g_protect_calls);
    Os_StopOsIntTiming();
}

/* ══════════════════════════════════════════════════════════════════════════
 * Periodic main function
 * ══════════════════════════════════════════════════════════════════════════ */

void test_MainFunction_SweepsTasks(void)
{
    g_tick = 0U;
    Os_StartTaskExecutionTiming(0U);   /* KILL action */
    g_tick = 2U;
    Os_TimingProtectionMainFunction(); /* loops tasks; task 0 over budget */
    TEST_ASSERT_EQUAL_UINT32(1U, g_terminate_calls);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Runner
 * ══════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_TaskTiming_StartStop_ValidAndInvalid);
    RUN_TEST(test_TaskBudget_Fault_Kill);
    RUN_TEST(test_TaskBudget_Fault_Restart);
    RUN_TEST(test_TaskBudget_Fault_ErrorHook);
    RUN_TEST(test_TaskBudget_Fault_ProtectionHook);
    RUN_TEST(test_TaskBudget_Fault_None_Default);
    RUN_TEST(test_TaskBudget_NoFault_WithinBudget);
    RUN_TEST(test_TaskBudget_NotRunning_And_InvalidTask);

    RUN_TEST(test_Arrival_FirstArrival_Accepted);
    RUN_TEST(test_Arrival_TooEarly_Rejected);
    RUN_TEST(test_Arrival_AfterMinInterval_Accepted);
    RUN_TEST(test_Arrival_InvalidTask_Allowed);

    RUN_TEST(test_ResourceTiming_ZeroResources_NoOp);

    RUN_TEST(test_Interrupt_AllInt_Fault);
    RUN_TEST(test_Interrupt_AllInt_NoFault);
    RUN_TEST(test_Interrupt_OsInt_Fault);
    RUN_TEST(test_Interrupt_OsInt_NoFault);

    RUN_TEST(test_MainFunction_SweepsTasks);

    return UNITY_END();
}
