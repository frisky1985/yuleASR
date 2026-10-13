/**
 * @file test_os_event.c
 * @brief OS Event Management unit tests
 *
 * Compiles the ACTUAL Os.c source with fully stubbed FreeRTOS APIs.
 * Covers all branches in SetEvent, ClearEvent, WaitEvent, GetEvent.
 *
 * Coverage targets (Os.c):
 *   - SetEvent: DET guard (invalid ID), not extended, NULL event group, success
 *   - ClearEvent: not extended, NULL event group, found+clear, not found
 *   - WaitEvent: not extended, holds resource, NULL event group, success, not found
 *   - GetEvent: DET guards, not extended, NULL event group, success
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>

#include "Os.h"
#include "Os_Internal.h"
#include "Os_Cfg.h"

/* ================================================================
 *  Stub state
 * ================================================================ */
static TaskHandle_t g_stub_current_task;
static UBaseType_t g_stub_priorities[4];
static EventBits_t g_stub_event_bits;
static int g_stub_set_bits_called;
static int g_stub_clear_bits_called;
static int g_stub_get_bits_called;
static int g_stub_event_group_create_fail;
static EventGroupHandle_t g_stub_event_group_handle;

/* DET */
static int g_det_error_reported;
static uint8_t g_det_last_api_id;
static uint8_t g_det_last_error_id;

/* ================================================================
 *  Test config tables
 * ================================================================ */
/* Os_GlobalState strong definition (normally in Os_Cfg.c) */
Os_GlobalStateType Os_GlobalState;

static Os_TaskConfigType g_test_tasks[4];
static Os_AlarmConfigType g_test_alarms[1];
static Os_ResourceConfigType g_test_resources[2];
static int g_dummy_mutex;

/* ================================================================
 *  FreeRTOS API stubs (only what event management needs)
 * ================================================================ */

/* Minimal task stubs */
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return g_stub_current_task; }
void vTaskPrioritySet(TaskHandle_t x, UBaseType_t p) { (void)x;(void)p; }
UBaseType_t uxTaskPriorityGet(TaskHandle_t x) { (void)x; return g_stub_priorities[0]; }

/* Unused but required by Os.c linker */
BaseType_t xTaskCreate(void (*a)(void*), const char *b, configSTACK_DEPTH_TYPE c,
                       void *d, UBaseType_t e, TaskHandle_t *f)
{ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f; return pdPASS; }
void vTaskDelete(TaskHandle_t x) { (void)x; }
void vTaskSuspend(TaskHandle_t x) { (void)x; }
void vTaskResume(TaskHandle_t x) { (void)x; }
eTaskState eTaskGetState(TaskHandle_t x) { (void)x; return eReady; }
void vTaskStartScheduler(void) {}
void vTaskEndScheduler(void) {}
BaseType_t xTaskGetSchedulerState(void) { return taskSCHEDULER_RUNNING; }
TickType_t xTaskGetTickCount(void) { return 0; }
void vTaskDelay(TickType_t x) { (void)x; }

/* Semaphore stubs */
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (SemaphoreHandle_t)&g_dummy_mutex; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t t) { (void)s;(void)t; return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t s) { (void)s; return pdTRUE; }

/* Event group stubs */
EventGroupHandle_t xEventGroupCreate(void)
{
    if (g_stub_event_group_create_fail) return NULL_PTR;
    return g_stub_event_group_handle;
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t x, EventBits_t bits)
{
    (void)x; g_stub_set_bits_called++; g_stub_event_bits |= bits;
    return g_stub_event_bits;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t x, EventBits_t bits)
{
    (void)x; g_stub_clear_bits_called++; g_stub_event_bits &= ~bits;
    return g_stub_event_bits;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t x, EventBits_t wait,
                                 BaseType_t clear, BaseType_t all, TickType_t ticks)
{
    (void)x;(void)wait;(void)clear;(void)all;(void)ticks;
    return g_stub_event_bits;
}

EventBits_t xEventGroupGetBits(EventGroupHandle_t x)
{
    (void)x; g_stub_get_bits_called++; return g_stub_event_bits;
}

/* Timer stubs */
TimerHandle_t xTimerCreate(const char *a, TickType_t b, UBaseType_t c,
                           void *d, void (*e)(TimerHandle_t))
{ (void)a;(void)b;(void)c;(void)d;(void)e; return NULL; }
BaseType_t xTimerStart(TimerHandle_t t, TickType_t w) { (void)t;(void)w; return pdPASS; }
BaseType_t xTimerStop(TimerHandle_t t, TickType_t w) { (void)t;(void)w; return pdPASS; }
BaseType_t xTimerChangePeriod(TimerHandle_t t, TickType_t p, TickType_t w)
{ (void)t;(void)p;(void)w; return pdPASS; }
void *pvTimerGetTimerID(TimerHandle_t t) { (void)t; return NULL; }

/* DET stub */
Std_ReturnType Det_ReportError(uint16 m, uint8 i, uint8 a, uint8 e)
{ (void)m;(void)i; g_det_error_reported++; g_det_last_api_id=a; g_det_last_error_id=e; return 0; }

/* ================================================================
 *  Setup helpers
 * ================================================================ */
static void stub_reset(void)
{
    g_stub_current_task = (TaskHandle_t)0x200;
    memset(g_stub_priorities, 0, sizeof(g_stub_priorities));
    g_stub_event_bits = 0;
    g_stub_set_bits_called = 0;
    g_stub_clear_bits_called = 0;
    g_stub_get_bits_called = 0;
    g_stub_event_group_create_fail = 0;
    static int eg_storage;
    g_stub_event_group_handle = (EventGroupHandle_t)&eg_storage;
    g_det_error_reported = 0;
    g_det_last_api_id = 0;
    g_det_last_error_id = 0;
}

static void setup_tasks(void)
{
    /* Task 0: basic (not extended) */
    memset(&g_test_tasks[0], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[0].TaskID = 0;
    g_test_tasks[0].FreeRTOS_Task = NULL_PTR;
    g_test_tasks[0].IsExtended = FALSE;

    /* Task 1: extended, with event group */
    memset(&g_test_tasks[1], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[1].TaskID = 1;
    static int eg1;
    g_test_tasks[1].FreeRTOS_Task = NULL_PTR;
    g_test_tasks[1].FreeRTOS_EventGroup = (EventGroupHandle_t)&eg1;
    g_test_tasks[1].IsExtended = TRUE;

    /* Task 2: extended, NULL event group */
    memset(&g_test_tasks[2], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[2].TaskID = 2;
    g_test_tasks[2].FreeRTOS_Task = NULL_PTR;
    g_test_tasks[2].FreeRTOS_EventGroup = NULL_PTR;
    g_test_tasks[2].IsExtended = TRUE;

    /* Task 3: extended, current task (for ClearEvent/WaitEvent) */
    memset(&g_test_tasks[3], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[3].TaskID = 3;
    static int eg3;
    g_test_tasks[3].FreeRTOS_Task = (TaskHandle_t)0x200; /* matches g_stub_current_task */
    g_test_tasks[3].FreeRTOS_EventGroup = (EventGroupHandle_t)&eg3;
    g_test_tasks[3].IsExtended = TRUE;

    Os_GlobalState.NumTasks = 4;
    Os_GlobalState.NumAlarms = 0;
    Os_GlobalState.NumResources = 2;
    Os_GlobalState.Tasks = g_test_tasks;
    Os_GlobalState.Alarms = g_test_alarms;
    Os_GlobalState.Resources = g_test_resources;

    /* Resources (for WaitEvent resource check) */
    memset(&g_test_resources[0], 0, sizeof(Os_ResourceConfigType));
    g_test_resources[0].OwnerTask = 0;
    g_test_resources[0].NestCount = 0;
    memset(&g_test_resources[1], 0, sizeof(Os_ResourceConfigType));
    g_test_resources[1].OwnerTask = 0;
    g_test_resources[1].NestCount = 0;
}

void setUp(void) { stub_reset(); setup_tasks(); }
void tearDown(void) {}

/* ================================================================
 *  SetEvent tests
 * ================================================================ */

/** @test SET_001 - Invalid TaskID => E_OS_ID */
void test_SetEvent_InvalidID(void)
{
    StatusType ret = SetEvent(99, 0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test SET_002 - Not extended task => E_OS_ACCESS */
void test_SetEvent_NotExtended(void)
{
    StatusType ret = SetEvent(0, 0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/** @test SET_003 - Extended task, NULL event group => E_OS_STATE */
void test_SetEvent_NullEventGroup(void)
{
    StatusType ret = SetEvent(2, 0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_STATE, ret);
}

/** @test SET_004 - Extended task, valid event group => E_OS_OK */
void test_SetEvent_Success(void)
{
    StatusType ret = SetEvent(1, 0x05);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_set_bits_called);
    TEST_ASSERT_EQUAL_HEX32(0x05, g_stub_event_bits & 0x05);
}

/** @test SET_005 - DET error reported on non-OK return */
void test_SetEvent_DET_OnError(void)
{
    SetEvent(0, 0x01); /* E_OS_ACCESS */
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_SETEVENT, g_det_last_api_id);
}

/* ================================================================
 *  ClearEvent tests
 * ================================================================ */

/** @test CLR_001 - Current task not extended => E_OS_ACCESS */
void test_ClearEvent_NotExtended(void)
{
    /* Current task handle (0x200) doesn't match any task in table,
     * so the for-loop falls through => E_OS_ACCESS */
    /* First, set current task to task 0 (not extended) */
    g_test_tasks[0].FreeRTOS_Task = g_stub_current_task;
    StatusType ret = ClearEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/** @test CLR_002 - Extended task, NULL event group => E_OS_STATE */
void test_ClearEvent_NullEventGroup(void)
{
    g_test_tasks[2].FreeRTOS_Task = g_stub_current_task;
    StatusType ret = ClearEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_STATE, ret);
}

/** @test CLR_003 - Extended task, valid event group => E_OS_OK */
void test_ClearEvent_Success(void)
{
    g_stub_event_bits = 0x0F;
    /* Task 3 is the current task (FreeRTOS_Task == 0x200) */
    StatusType ret = ClearEvent(0x03);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_clear_bits_called);
    TEST_ASSERT_EQUAL_HEX32(0x00, g_stub_event_bits & 0x03);
}

/** @test CLR_004 - Current task not found in table => E_OS_ACCESS */
void test_ClearEvent_TaskNotFound(void)
{
    /* Make sure no task matches current handle */
    for (int i = 0; i < 4; i++) {
        g_test_tasks[i].FreeRTOS_Task = NULL_PTR;
    }
    StatusType ret = ClearEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/* ================================================================
 *  WaitEvent tests
 * ================================================================ */

/** @test WAIT_001 - Current task not extended => E_OS_ACCESS */
void test_WaitEvent_NotExtended(void)
{
    g_test_tasks[0].FreeRTOS_Task = g_stub_current_task;
    StatusType ret = WaitEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/** @test WAIT_002 - Task holds resource => E_OS_RESOURCE */
void test_WaitEvent_HoldsResource(void)
{
    /* Task 3 is current task, set resource owner to task 3's TaskID */
    g_test_resources[0].OwnerTask = g_test_tasks[3].TaskID;
    g_test_resources[0].NestCount = 1;
    StatusType ret = WaitEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_RESOURCE, ret);
}

/** @test WAIT_003 - Extended task, NULL event group => E_OS_STATE */
void test_WaitEvent_NullEventGroup(void)
{
    g_test_tasks[2].FreeRTOS_Task = g_stub_current_task;
    StatusType ret = WaitEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_STATE, ret);
}

/** @test WAIT_004 - Extended task, valid => E_OS_OK */
void test_WaitEvent_Success(void)
{
    /* Task 3 is current task, extended, with event group */
    StatusType ret = WaitEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
}

/** @test WAIT_005 - Current task not found => E_OS_ACCESS */
void test_WaitEvent_TaskNotFound(void)
{
    for (int i = 0; i < 4; i++) {
        g_test_tasks[i].FreeRTOS_Task = NULL_PTR;
    }
    StatusType ret = WaitEvent(0x01);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/** @test WAIT_006 - DET reported on error */
void test_WaitEvent_DET_OnError(void)
{
    g_test_tasks[0].FreeRTOS_Task = g_stub_current_task;
    WaitEvent(0x01); /* E_OS_ACCESS */
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_WAITEVENT, g_det_last_api_id);
}

/* ================================================================
 *  GetEvent tests
 * ================================================================ */

/** @test GET_001 - Invalid TaskID => E_OS_ID */
void test_GetEvent_InvalidID(void)
{
    EventMaskType event;
    StatusType ret = GetEvent(99, &event);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
}

/** @test GET_002 - NULL Event pointer => E_OS_PARAM_POINTER */
void test_GetEvent_NullPointer(void)
{
    StatusType ret = GetEvent(0, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OS_PARAM_POINTER, ret);
}

/** @test GET_003 - Not extended task => E_OS_ACCESS */
void test_GetEvent_NotExtended(void)
{
    EventMaskType event;
    StatusType ret = GetEvent(0, &event);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/** @test GET_004 - Extended task, NULL event group => returns 0 */
void test_GetEvent_NullEventGroup_ReturnsZero(void)
{
    EventMaskType event = 0xFF;
    StatusType ret = GetEvent(2, &event);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, event);
}

/** @test GET_005 - Extended task, valid event group => returns bits */
void test_GetEvent_Success(void)
{
    g_stub_event_bits = 0xAB;
    EventMaskType event = 0;
    StatusType ret = GetEvent(1, &event);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0xAB, event);
    TEST_ASSERT_EQUAL_INT(1, g_stub_get_bits_called);
}

/** @test GET_006 - DET reported on non-OK */
void test_GetEvent_DET_OnError(void)
{
    EventMaskType event;
    GetEvent(0, &event); /* E_OS_ACCESS */
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_GETEVENT, g_det_last_api_id);
}

/* ================================================================
 *  Test runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* SetEvent */
    RUN_TEST(test_SetEvent_InvalidID);
    RUN_TEST(test_SetEvent_NotExtended);
    RUN_TEST(test_SetEvent_NullEventGroup);
    RUN_TEST(test_SetEvent_Success);
    RUN_TEST(test_SetEvent_DET_OnError);

    /* ClearEvent */
    RUN_TEST(test_ClearEvent_NotExtended);
    RUN_TEST(test_ClearEvent_NullEventGroup);
    RUN_TEST(test_ClearEvent_Success);
    RUN_TEST(test_ClearEvent_TaskNotFound);

    /* WaitEvent */
    RUN_TEST(test_WaitEvent_NotExtended);
    RUN_TEST(test_WaitEvent_HoldsResource);
    RUN_TEST(test_WaitEvent_NullEventGroup);
    RUN_TEST(test_WaitEvent_Success);
    RUN_TEST(test_WaitEvent_TaskNotFound);
    RUN_TEST(test_WaitEvent_DET_OnError);

    /* GetEvent */
    RUN_TEST(test_GetEvent_InvalidID);
    RUN_TEST(test_GetEvent_NullPointer);
    RUN_TEST(test_GetEvent_NotExtended);
    RUN_TEST(test_GetEvent_NullEventGroup_ReturnsZero);
    RUN_TEST(test_GetEvent_Success);
    RUN_TEST(test_GetEvent_DET_OnError);

    return UNITY_END();
}
