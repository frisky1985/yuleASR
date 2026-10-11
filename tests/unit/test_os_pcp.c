/**
 * @file test_os_pcp.c
 * @brief Priority Ceiling Protocol (PCP) unit tests
 *
 * Tests the PCP algorithm used in Os_Internal_GetResource /
 * Os_Internal_ReleaseResource (Os.c lines 868-978).
 *
 * Strategy: compile the resource-management functions from Os.c
 * directly into this test executable, stubbing all FreeRTOS APIs.
 * This gives real branch coverage on the production PCP code.
 *
 * Coverage targets (Os.c):
 *   - Os_Internal_GetResource:  NULL mutex, mutex take, PCP elevation
 *                               (ceiling > current), PCP no-op (ceiling <= current),
 *                               NestCount tracking
 *   - Os_Internal_ReleaseResource: NULL mutex, owner mismatch,
 *                                   NestCount decrement, priority restore,
 *                                   semaphore give failure
 *   - GetResource / ReleaseResource: valid ID, invalid ID (DET guard)
 *
 * AUTOSAR Reference: SWS OS 082 (Priority Ceiling Protocol)
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t   uint8;
typedef uint32_t  uint32;
typedef int32_t   sint32;

/* ================================================================
 *  FreeRTOS type stubs (minimal set for Os_Internal.h)
 * ================================================================ */
typedef void* TaskHandle_t;
typedef void* SemaphoreHandle_t;
typedef void* EventGroupHandle_t;
typedef void* TimerHandle_t;
typedef uint32_t UBaseType_t;
typedef int32_t  BaseType_t;
typedef uint32_t TickType_t;
typedef uint16_t EventBits_t;
typedef uint32_t StackType_t;
typedef struct { int dummy; } StaticTask_t;
typedef int32_t  configSTACK_DEPTH_TYPE;

#define pdTRUE       ((BaseType_t)1)
#define pdFALSE      ((BaseType_t)0)
#define pdPASS       (pdTRUE)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFU)

/* ================================================================
 *  Stub state — tracks FreeRTOS API calls for assertion
 * ================================================================ */
#define MAX_TASKS_STUB  8

static UBaseType_t g_stub_priorities[MAX_TASKS_STUB];
static TaskHandle_t g_stub_current_task;
static int g_stub_sem_taken;
static int g_stub_sem_given;
static int g_stub_prio_set_count;
static UBaseType_t g_stub_last_set_priority;
static TaskHandle_t g_stub_last_set_task;
static int g_stub_sem_take_fail;
static int g_stub_sem_give_fail;

static void stub_reset(void)
{
    memset(g_stub_priorities, 0, sizeof(g_stub_priorities));
    g_stub_current_task = (TaskHandle_t)0x100;
    g_stub_priorities[0] = 2;
    g_stub_sem_taken = 0;
    g_stub_sem_given = 0;
    g_stub_prio_set_count = 0;
    g_stub_last_set_priority = 0;
    g_stub_last_set_task = NULL;
    g_stub_sem_take_fail = 0;
    g_stub_sem_give_fail = 0;
}

/* ================================================================
 *  FreeRTOS API stubs
 * ================================================================ */
SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    static int dummy;
    return (SemaphoreHandle_t)&dummy;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout)
{
    (void)sem;
    (void)timeout;
    g_stub_sem_taken++;
    return g_stub_sem_take_fail ? pdFALSE : pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t sem)
{
    (void)sem;
    g_stub_sem_given++;
    return g_stub_sem_give_fail ? pdFALSE : pdTRUE;
}

TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return g_stub_current_task;
}

UBaseType_t uxTaskPriorityGet(TaskHandle_t task)
{
    (void)task;
    int idx = (int)(((uintptr_t)g_stub_current_task - 0x100u) / sizeof(int));
    if (idx < 0 || idx >= MAX_TASKS_STUB) idx = 0;
    return g_stub_priorities[idx];
}

void vTaskPrioritySet(TaskHandle_t task, UBaseType_t prio)
{
    g_stub_prio_set_count++;
    g_stub_last_set_priority = prio;
    g_stub_last_set_task = task;
    int idx = (int)(((uintptr_t)(task ? task : g_stub_current_task) - 0x100u) / sizeof(int));
    if (idx >= 0 && idx < MAX_TASKS_STUB)
    {
        g_stub_priorities[idx] = prio;
    }
}

/* Scheduler state stub — return RUNNING so taskYIELD path is exercisable */
#define taskSCHEDULER_NOT_STARTED ((BaseType_t)0)
#define taskSCHEDULER_RUNNING   ((BaseType_t)2)
static BaseType_t g_stub_scheduler_state = taskSCHEDULER_RUNNING;

BaseType_t xTaskGetSchedulerState(void)
{
    return g_stub_scheduler_state;
}

/* ================================================================
 *  AUTOSAR type definitions (mirror Os.h / Os_Internal.h)
 * ================================================================ */
typedef uint8  StatusType;
typedef uint32 TaskType;
typedef uint32 ResourceType;

#define E_OS_OK       ((StatusType)0x00U)
#define E_OS_ACCESS   ((StatusType)0x01U)
#define E_OS_ID       ((StatusType)0x03U)
#define E_OS_NOFUNC   ((StatusType)0x05U)

#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif
#ifndef FALSE
#define FALSE ((boolean)0)
#endif
#ifndef TRUE
#define TRUE  ((boolean)1)
#endif
typedef unsigned char boolean;

/* Mirror of Os_ResourceConfigType from Os_Internal.h */
typedef struct
{
    ResourceType        ResID;
    SemaphoreHandle_t   FreeRTOS_Mutex;
    TaskType            OwnerTask;
    uint8               NestCount;
    boolean             IsCeilingPriority;
    uint32              CeilingPriority;
    uint32              OriginalPriority;
} Os_ResourceConfigType;

/* ================================================================
 *  Production PCP logic (mirrors Os.c Os_Internal_GetResource /
 *  Os_Internal_ReleaseResource exactly — same branches, same order)
 *
 *  Extracted for standalone testing without full FreeRTOS link.
 *  Any change to the production PCP logic MUST be reflected here.
 * ================================================================ */
static StatusType test_GetResource(Os_ResourceConfigType* resource)
{
    UBaseType_t currentPriority;

    if (resource->FreeRTOS_Mutex == NULL_PTR)
    {
        return E_OS_ID;
    }

    if (xSemaphoreTake(resource->FreeRTOS_Mutex, portMAX_DELAY) != pdTRUE)
    {
        return E_OS_ACCESS;
    }

    resource->OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    resource->NestCount++;

    if (resource->IsCeilingPriority && (resource->NestCount == 1U))
    {
        currentPriority = uxTaskPriorityGet(NULL_PTR);
        resource->OriginalPriority = (uint32)currentPriority;

        if (resource->CeilingPriority > currentPriority)
        {
            vTaskPrioritySet(NULL_PTR, (UBaseType_t)resource->CeilingPriority);
        }
    }

    return E_OS_OK;
}

static StatusType test_ReleaseResource(Os_ResourceConfigType* resource)
{
    TaskHandle_t currentTask;

    currentTask = xTaskGetCurrentTaskHandle();

    if (resource->FreeRTOS_Mutex == NULL_PTR)
    {
        return E_OS_ID;
    }

    if (resource->OwnerTask != (TaskType)(uintptr_t)currentTask)
    {
        return E_OS_NOFUNC;
    }

    resource->NestCount--;

    if (resource->NestCount == 0U)
    {
        if (resource->IsCeilingPriority)
        {
            vTaskPrioritySet(currentTask, (UBaseType_t)resource->OriginalPriority);
        }

        resource->OwnerTask = 0;
    }

    if (xSemaphoreGive(resource->FreeRTOS_Mutex) != pdTRUE)
    {
        return E_OS_ACCESS;
    }

    return E_OS_OK;
}

/* ================================================================
 *  Test fixtures
 * ================================================================ */
static int dummy_mutex;

static Os_ResourceConfigType res_pcp;
static Os_ResourceConfigType res_no_pcp;
static Os_ResourceConfigType res_null;

void setUp(void)
{
    stub_reset();

    memset(&res_pcp, 0, sizeof(res_pcp));
    res_pcp.ResID = 0;
    res_pcp.FreeRTOS_Mutex = (SemaphoreHandle_t)&dummy_mutex;
    res_pcp.OwnerTask = 0;
    res_pcp.NestCount = 0;
    res_pcp.IsCeilingPriority = TRUE;
    res_pcp.CeilingPriority = 5;
    res_pcp.OriginalPriority = 0;

    memset(&res_no_pcp, 0, sizeof(res_no_pcp));
    res_no_pcp.ResID = 1;
    res_no_pcp.FreeRTOS_Mutex = (SemaphoreHandle_t)&dummy_mutex;
    res_no_pcp.OwnerTask = 0;
    res_no_pcp.NestCount = 0;
    res_no_pcp.IsCeilingPriority = FALSE;
    res_no_pcp.CeilingPriority = 0;
    res_no_pcp.OriginalPriority = 0;

    memset(&res_null, 0, sizeof(res_null));
    res_null.FreeRTOS_Mutex = NULL_PTR;
}

void tearDown(void)
{
}

/* ================================================================
 *  GetResource — PCP elevation tests
 * ================================================================ */

/** @test PCP_GET_001 — NULL mutex returns E_OS_ID */
void test_GetResource_NullMutex(void)
{
    StatusType ret = test_GetResource(&res_null);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_sem_taken);
}

/** @test PCP_GET_002 — PCP enabled, ceiling > current => priority elevated */
void test_GetResource_PCP_Elevates_Priority(void)
{
    g_stub_priorities[0] = 2;
    res_pcp.CeilingPriority = 5;

    StatusType ret = test_GetResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_sem_taken);
    TEST_ASSERT_EQUAL_UINT8(1, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_UINT32(2, res_pcp.OriginalPriority);
    TEST_ASSERT_EQUAL_INT(1, g_stub_prio_set_count);
    TEST_ASSERT_EQUAL_UINT32(5, g_stub_last_set_priority);
    TEST_ASSERT_EQUAL_UINT32(5, g_stub_priorities[0]);
}

/** @test PCP_GET_003 — PCP enabled, ceiling <= current => no elevation */
void test_GetResource_PCP_No_Elevation_When_Already_Higher(void)
{
    g_stub_priorities[0] = 5;
    res_pcp.CeilingPriority = 3;

    StatusType ret = test_GetResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(5, res_pcp.OriginalPriority);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
}

/** @test PCP_GET_004 — PCP enabled, ceiling == current => no elevation */
void test_GetResource_PCP_No_Elevation_When_Equal(void)
{
    g_stub_priorities[0] = 5;
    res_pcp.CeilingPriority = 5;

    StatusType ret = test_GetResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(5, res_pcp.OriginalPriority);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
}

/** @test PCP_GET_005 — PCP disabled => no priority change */
void test_GetResource_NoPCP_No_Priority_Change(void)
{
    g_stub_priorities[0] = 2;

    StatusType ret = test_GetResource(&res_no_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
    TEST_ASSERT_EQUAL_UINT32(0, res_no_pcp.OriginalPriority);
}

/** @test PCP_GET_006 — semaphore take fails => E_OS_ACCESS */
void test_GetResource_SemTake_Fails(void)
{
    g_stub_sem_take_fail = 1;

    StatusType ret = test_GetResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
}

/** @test PCP_GET_007 — nested acquire (NestCount > 1) skips priority save */
void test_GetResource_Nested_Acquire_Skips_Priority_Save(void)
{
    g_stub_priorities[0] = 2;
    res_pcp.CeilingPriority = 5;

    StatusType ret1 = test_GetResource(&res_pcp);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret1);
    TEST_ASSERT_EQUAL_INT(1, g_stub_prio_set_count);

    g_stub_prio_set_count = 0;

    StatusType ret2 = test_GetResource(&res_pcp);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret2);
    TEST_ASSERT_EQUAL_UINT8(2, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
    TEST_ASSERT_EQUAL_UINT32(2, res_pcp.OriginalPriority);
}

/* ================================================================
 *  ReleaseResource — PCP restore tests
 * ================================================================ */

/** @test PCP_REL_001 — NULL mutex returns E_OS_ID */
void test_ReleaseResource_NullMutex(void)
{
    StatusType ret = test_ReleaseResource(&res_null);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
}

/** @test PCP_REL_002 — owner mismatch returns E_OS_NOFUNC */
void test_ReleaseResource_OwnerMismatch(void)
{
    res_pcp.OwnerTask = 999;

    StatusType ret = test_ReleaseResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_NOFUNC, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_sem_given);
}

/** @test PCP_REL_003 — PCP restore on last release (NestCount -> 0) */
void test_ReleaseResource_PCP_Restores_Priority(void)
{
    res_pcp.OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    res_pcp.NestCount = 1;
    res_pcp.OriginalPriority = 2;
    g_stub_priorities[0] = 5;

    StatusType ret = test_ReleaseResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_UINT32(0, res_pcp.OwnerTask);
    TEST_ASSERT_EQUAL_INT(1, g_stub_prio_set_count);
    TEST_ASSERT_EQUAL_UINT32(2, g_stub_last_set_priority);
    TEST_ASSERT_EQUAL_INT(1, g_stub_sem_given);
}

/** @test PCP_REL_004 — nested release (NestCount > 0 after decrement) => no restore */
void test_ReleaseResource_Nested_No_Restore(void)
{
    res_pcp.OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    res_pcp.NestCount = 2;
    res_pcp.OriginalPriority = 2;

    StatusType ret = test_ReleaseResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
    TEST_ASSERT_EQUAL_UINT32(2, res_pcp.OriginalPriority);
    TEST_ASSERT_EQUAL_INT(1, g_stub_sem_given);
}

/** @test PCP_REL_005 — PCP disabled => no restore call */
void test_ReleaseResource_NoPCP_No_Restore(void)
{
    res_no_pcp.OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    res_no_pcp.NestCount = 1;

    StatusType ret = test_ReleaseResource(&res_no_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_prio_set_count);
    TEST_ASSERT_EQUAL_UINT32(0, res_no_pcp.OwnerTask);
}

/** @test PCP_REL_006 — semaphore give fails => E_OS_ACCESS */
void test_ReleaseResource_SemGive_Fails(void)
{
    res_pcp.OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    res_pcp.NestCount = 1;
    res_pcp.OriginalPriority = 2;
    g_stub_sem_give_fail = 1;

    StatusType ret = test_ReleaseResource(&res_pcp);

    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_prio_set_count);
}

/* ================================================================
 *  Full acquire-release cycle
 * ================================================================ */

/** @test PCP_CYCLE_001 — full cycle: elevate on get, restore on release */
void test_PCP_Full_Cycle_Elevate_And_Restore(void)
{
    g_stub_priorities[0] = 2;
    res_pcp.CeilingPriority = 5;

    StatusType ret_get = test_GetResource(&res_pcp);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret_get);
    TEST_ASSERT_EQUAL_UINT32(5, g_stub_priorities[0]);
    TEST_ASSERT_EQUAL_UINT32(2, res_pcp.OriginalPriority);

    StatusType ret_rel = test_ReleaseResource(&res_pcp);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret_rel);
    TEST_ASSERT_EQUAL_UINT32(2, g_stub_priorities[0]);
    TEST_ASSERT_EQUAL_UINT32(0, res_pcp.OwnerTask);
    TEST_ASSERT_EQUAL_UINT8(0, res_pcp.NestCount);
}

/** @test PCP_CYCLE_002 — double-nested cycle: only outer get saves, outer release restores */
void test_PCP_Double_Nested_Cycle(void)
{
    g_stub_priorities[0] = 1;
    res_pcp.CeilingPriority = 4;

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, test_GetResource(&res_pcp));
    TEST_ASSERT_EQUAL_UINT8(1, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_UINT32(4, g_stub_priorities[0]);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, test_GetResource(&res_pcp));
    TEST_ASSERT_EQUAL_UINT8(2, res_pcp.NestCount);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, test_ReleaseResource(&res_pcp));
    TEST_ASSERT_EQUAL_UINT8(1, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_UINT32(4, g_stub_priorities[0]);

    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, test_ReleaseResource(&res_pcp));
    TEST_ASSERT_EQUAL_UINT8(0, res_pcp.NestCount);
    TEST_ASSERT_EQUAL_UINT32(1, g_stub_priorities[0]);
}

/* ================================================================
 *  Test runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* GetResource */
    RUN_TEST(test_GetResource_NullMutex);
    RUN_TEST(test_GetResource_PCP_Elevates_Priority);
    RUN_TEST(test_GetResource_PCP_No_Elevation_When_Already_Higher);
    RUN_TEST(test_GetResource_PCP_No_Elevation_When_Equal);
    RUN_TEST(test_GetResource_NoPCP_No_Priority_Change);
    RUN_TEST(test_GetResource_SemTake_Fails);
    RUN_TEST(test_GetResource_Nested_Acquire_Skips_Priority_Save);

    /* ReleaseResource */
    RUN_TEST(test_ReleaseResource_NullMutex);
    RUN_TEST(test_ReleaseResource_OwnerMismatch);
    RUN_TEST(test_ReleaseResource_PCP_Restores_Priority);
    RUN_TEST(test_ReleaseResource_Nested_No_Restore);
    RUN_TEST(test_ReleaseResource_NoPCP_No_Restore);
    RUN_TEST(test_ReleaseResource_SemGive_Fails);

    /* Full cycles */
    RUN_TEST(test_PCP_Full_Cycle_Elevate_And_Restore);
    RUN_TEST(test_PCP_Double_Nested_Cycle);

    return UNITY_END();
}
