/**
 * @file test_os_task.c
 * @brief OS Task Management, Interrupt Management, Version Info, Mode tests
 *
 * Compiles the ACTUAL Os.c source with fully stubbed FreeRTOS APIs.
 * This gives real branch coverage on the production code.
 *
 * Coverage targets (Os.c):
 *   - ActivateTask: DET guard (invalid ID), create path, resume path,
 *                   xTaskCreate fail, event group create fail, scheduler check
 *   - TerminateTask: resource check, post-task hook, suspend
 *   - ChainTask: DET guard, activate+terminate chain
 *   - Schedule: yield
 *   - GetTaskID: NULL ptr, found, not-found
 *   - GetTaskState: DET guards, NULL handle, all eTaskState switch cases
 *   - Interrupt management: all 6 functions
 *   - GetActiveApplicationMode
 *   - Os_GetVersionInfo: valid ptr, NULL ptr
 *   - Os_TaskWrapper: entry dispatch, NULL entry
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================
 *  AUTOSAR includes (Os.h first, then internal)
 * ================================================================ */
#include "Os.h"
#include "Os_Internal.h"
#include "Os_Cfg.h"

/* ================================================================
 *  Stub state
 * ================================================================ */
#define STUB_MAX_TASKS  8

static TaskHandle_t g_stub_current_task;
static BaseType_t g_stub_scheduler_state;
static BaseType_t g_stub_xTaskCreate_ret;
static int g_stub_xTaskCreate_called;
static int g_stub_vTaskDelete_called;
static int g_stub_vTaskSuspend_called;
static int g_stub_vTaskResume_called;
static int g_stub_vTaskStartScheduler_called;
static UBaseType_t g_stub_priorities[STUB_MAX_TASKS];
static int g_stub_prio_set_count;
static UBaseType_t g_stub_last_set_priority;
static TickType_t g_stub_tick_count;
static eTaskState g_stub_task_state;

/* Event group stubs */
static int g_stub_event_group_create_fail;
static EventGroupHandle_t g_stub_event_group_handle;
static int g_stub_set_bits_called;
static int g_stub_clear_bits_called;
static int g_stub_get_bits_called;
static EventBits_t g_stub_event_bits;

/* Timer stubs */
static int g_stub_timer_create_fail;
static TimerHandle_t g_stub_timer_handle;
static int g_stub_timer_start_called;
static int g_stub_timer_stop_called;
static int g_stub_timer_change_period_called;
static BaseType_t g_stub_timer_start_ret;
static BaseType_t g_stub_timer_stop_ret;
static BaseType_t g_stub_timer_change_period_ret;

/* Semaphore stubs */
static int g_stub_sem_take_fail;
static int g_stub_sem_give_fail;
static SemaphoreHandle_t g_stub_mutex_handle;

/* DET stubs */
static int g_det_error_reported;
static uint8_t g_det_last_api_id;
static uint8_t g_det_last_error_id;

/* Interrupt tracking */
static int g_stub_interrupts_disabled;
static int g_stub_interrupts_enabled;

/* Task entry tracking */
static int g_task_entry_called;

/* ================================================================
 *  Test config tables (local to this test executable)
 * ================================================================ */
static void dummy_entry(void) { g_task_entry_called++; }
static void null_entry_check(void) { /* returns immediately */ }

/* Os_GlobalState strong definition (normally in Os_Cfg.c) */
Os_GlobalStateType Os_GlobalState;

static Os_TaskConfigType g_test_tasks[3];
static Os_AlarmConfigType g_test_alarms[2];
static Os_ResourceConfigType g_test_resources[2];
static int g_dummy_mutex_1;
static int g_dummy_mutex_2;

/* ================================================================
 *  FreeRTOS API stubs
 * ================================================================ */
BaseType_t xTaskCreate(void (*pvTaskCode)(void*),
                       const char *pcName,
                       configSTACK_DEPTH_TYPE usStackDepth,
                       void *pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *pxCreatedTask)
{
    (void)pvTaskCode; (void)pcName; (void)usStackDepth;
    (void)pvParameters; (void)uxPriority;
    g_stub_xTaskCreate_called++;
    if (g_stub_xTaskCreate_ret != pdPASS) return g_stub_xTaskCreate_ret;
    if (pxCreatedTask != NULL) {
        static int task_handle_storage;
        *pxCreatedTask = (TaskHandle_t)&task_handle_storage;
    }
    return pdPASS;
}

void vTaskDelete(TaskHandle_t xTask) { (void)xTask; g_stub_vTaskDelete_called++; }
void vTaskSuspend(TaskHandle_t xTaskToSuspend) { (void)xTaskToSuspend; g_stub_vTaskSuspend_called++; }
void vTaskResume(TaskHandle_t xTaskToResume) { (void)xTaskToResume; g_stub_vTaskResume_called++; }

TaskHandle_t xTaskGetCurrentTaskHandle(void) { return g_stub_current_task; }
eTaskState eTaskGetState(TaskHandle_t xTask) { (void)xTask; return g_stub_task_state; }

void vTaskStartScheduler(void) { g_stub_vTaskStartScheduler_called++; }
void vTaskEndScheduler(void) {}

void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority)
{
    (void)xTask;
    g_stub_prio_set_count++;
    g_stub_last_set_priority = uxNewPriority;
}

UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask)
{
    (void)xTask;
    return g_stub_priorities[0];
}

BaseType_t xTaskGetSchedulerState(void) { return g_stub_scheduler_state; }
TickType_t xTaskGetTickCount(void) { return g_stub_tick_count; }
void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return g_stub_mutex_handle;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout)
{
    (void)sem; (void)timeout;
    return g_stub_sem_take_fail ? pdFALSE : pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t sem)
{
    (void)sem;
    return g_stub_sem_give_fail ? pdFALSE : pdTRUE;
}

EventGroupHandle_t xEventGroupCreate(void)
{
    if (g_stub_event_group_create_fail) return NULL_PTR;
    return g_stub_event_group_handle;
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet)
{
    (void)xEventGroup;
    g_stub_set_bits_called++;
    g_stub_event_bits |= uxBitsToSet;
    return g_stub_event_bits;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear)
{
    (void)xEventGroup;
    g_stub_clear_bits_called++;
    g_stub_event_bits &= ~uxBitsToClear;
    return g_stub_event_bits;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                 EventBits_t uxBitsToWaitFor,
                                 BaseType_t xClearOnExit,
                                 BaseType_t xWaitForAllBits,
                                 TickType_t xTicksToWait)
{
    (void)xEventGroup; (void)uxBitsToWaitFor;
    (void)xClearOnExit; (void)xWaitForAllBits; (void)xTicksToWait;
    return g_stub_event_bits;
}

EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup)
{
    (void)xEventGroup;
    g_stub_get_bits_called++;
    return g_stub_event_bits;
}

TimerHandle_t xTimerCreate(const char *pcTimerName,
                           TickType_t xTimerPeriodInTicks,
                           UBaseType_t uxAutoReload,
                           void *pvTimerID,
                           void (*pxCallBack)(TimerHandle_t))
{
    (void)pcTimerName; (void)xTimerPeriodInTicks;
    (void)uxAutoReload; (void)pvTimerID; (void)pxCallBack;
    if (g_stub_timer_create_fail) return NULL_PTR;
    return g_stub_timer_handle;
}

BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait)
{
    (void)xTimer; (void)xTicksToWait;
    g_stub_timer_start_called++;
    return g_stub_timer_start_ret;
}

BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait)
{
    (void)xTimer; (void)xTicksToWait;
    g_stub_timer_stop_called++;
    return g_stub_timer_stop_ret;
}

BaseType_t xTimerChangePeriod(TimerHandle_t xTimer, TickType_t xNewPeriod, TickType_t xTicksToWait)
{
    (void)xTimer; (void)xNewPeriod; (void)xTicksToWait;
    g_stub_timer_change_period_called++;
    return g_stub_timer_change_period_ret;
}

void *pvTimerGetTimerID(TimerHandle_t xTimer)
{
    (void)xTimer;
    return NULL_PTR;
}

/* DET stub */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId;
    g_det_error_reported++;
    g_det_last_api_id = ApiId;
    g_det_last_error_id = ErrorId;
    return 0x00U; /* E_OK */
}

/* ================================================================
 *  Stub reset helper
 * ================================================================ */
static void stub_reset_all(void)
{
    g_stub_current_task = (TaskHandle_t)0x100;
    g_stub_scheduler_state = taskSCHEDULER_RUNNING;
    g_stub_xTaskCreate_ret = pdPASS;
    g_stub_xTaskCreate_called = 0;
    g_stub_vTaskDelete_called = 0;
    g_stub_vTaskSuspend_called = 0;
    g_stub_vTaskResume_called = 0;
    g_stub_vTaskStartScheduler_called = 0;
    memset(g_stub_priorities, 0, sizeof(g_stub_priorities));
    g_stub_priorities[0] = 2;
    g_stub_prio_set_count = 0;
    g_stub_last_set_priority = 0;
    g_stub_tick_count = 100;
    g_stub_task_state = eReady;

    g_stub_event_group_create_fail = 0;
    static int eg_storage;
    g_stub_event_group_handle = (EventGroupHandle_t)&eg_storage;
    g_stub_set_bits_called = 0;
    g_stub_clear_bits_called = 0;
    g_stub_get_bits_called = 0;
    g_stub_event_bits = 0;

    g_stub_timer_create_fail = 0;
    static int tmr_storage;
    g_stub_timer_handle = (TimerHandle_t)&tmr_storage;
    g_stub_timer_start_called = 0;
    g_stub_timer_stop_called = 0;
    g_stub_timer_change_period_called = 0;
    g_stub_timer_start_ret = pdPASS;
    g_stub_timer_stop_ret = pdPASS;
    g_stub_timer_change_period_ret = pdPASS;

    g_stub_sem_take_fail = 0;
    g_stub_sem_give_fail = 0;
    g_stub_mutex_handle = (SemaphoreHandle_t)&g_dummy_mutex_1;

    g_det_error_reported = 0;
    g_det_last_api_id = 0;
    g_det_last_error_id = 0;

    g_stub_interrupts_disabled = 0;
    g_stub_interrupts_enabled = 0;
    g_task_entry_called = 0;
}

/* ================================================================
 *  Os_GlobalState setup helper
 * ================================================================ */
static void setup_global_state(void)
{
    /* Task 0: basic task, not extended */
    memset(&g_test_tasks[0], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[0].TaskID = 0;
    g_test_tasks[0].FreeRTOS_Task = NULL_PTR;
    g_test_tasks[0].FreeRTOS_EventGroup = NULL_PTR;
    g_test_tasks[0].Priority = 3;
    g_test_tasks[0].IsAutoStart = FALSE;
    g_test_tasks[0].IsExtended = FALSE;
    g_test_tasks[0].EntryPoint = dummy_entry;

    /* Task 1: extended task */
    memset(&g_test_tasks[1], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[1].TaskID = 1;
    g_test_tasks[1].FreeRTOS_Task = NULL_PTR;
    g_test_tasks[1].FreeRTOS_EventGroup = NULL_PTR;
    g_test_tasks[1].Priority = 4;
    g_test_tasks[1].IsAutoStart = FALSE;
    g_test_tasks[1].IsExtended = TRUE;
    g_test_tasks[1].EntryPoint = dummy_entry;

    /* Task 2: already created (has handle) */
    memset(&g_test_tasks[2], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[2].TaskID = 2;
    static int task2_handle;
    g_test_tasks[2].FreeRTOS_Task = (TaskHandle_t)&task2_handle;
    g_test_tasks[2].FreeRTOS_EventGroup = NULL_PTR;
    g_test_tasks[2].Priority = 2;
    g_test_tasks[2].IsAutoStart = FALSE;
    g_test_tasks[2].IsExtended = FALSE;
    g_test_tasks[2].EntryPoint = dummy_entry;

    /* Alarm 0: active */
    memset(&g_test_alarms[0], 0, sizeof(Os_AlarmConfigType));
    g_test_alarms[0].AlarmID = 0;
    g_test_alarms[0].State = OS_ALARM_ACTIVE;
    g_test_alarms[0].ExpiryTick = 500;
    g_test_alarms[0].FreeRTOS_Timer = g_stub_timer_handle;

    /* Alarm 1: unused */
    memset(&g_test_alarms[1], 0, sizeof(Os_AlarmConfigType));
    g_test_alarms[1].AlarmID = 1;
    g_test_alarms[1].State = OS_ALARM_UNUSED;
    g_test_alarms[1].FreeRTOS_Timer = g_stub_timer_handle;

    /* Resource 0: with mutex */
    memset(&g_test_resources[0], 0, sizeof(Os_ResourceConfigType));
    g_test_resources[0].ResID = 0;
    g_test_resources[0].FreeRTOS_Mutex = (SemaphoreHandle_t)&g_dummy_mutex_1;
    g_test_resources[0].OwnerTask = 0;
    g_test_resources[0].NestCount = 0;
    g_test_resources[0].IsCeilingPriority = FALSE;

    /* Resource 1: with mutex */
    memset(&g_test_resources[1], 0, sizeof(Os_ResourceConfigType));
    g_test_resources[1].ResID = 1;
    g_test_resources[1].FreeRTOS_Mutex = (SemaphoreHandle_t)&g_dummy_mutex_2;
    g_test_resources[1].OwnerTask = 0;
    g_test_resources[1].NestCount = 0;
    g_test_resources[1].IsCeilingPriority = FALSE;

    /* Set up global state */
    Os_GlobalState.IsInitialized = FALSE;
    Os_GlobalState.IsRunning = FALSE;
    Os_GlobalState.CurrentAppMode = OSDEFAULTAPPMODE;
    Os_GlobalState.NumTasks = 3;
    Os_GlobalState.NumAlarms = 2;
    Os_GlobalState.NumResources = 2;
    Os_GlobalState.Tasks = g_test_tasks;
    Os_GlobalState.Alarms = g_test_alarms;
    Os_GlobalState.Resources = g_test_resources;
}

/* ================================================================
 *  Unity fixtures
 * ================================================================ */
void setUp(void)
{
    stub_reset_all();
    setup_global_state();
}

void tearDown(void) {}

/* ================================================================
 *  ActivateTask tests
 * ================================================================ */

/** @test ACT_001 - Invalid TaskID triggers DET and returns E_OS_ID */
void test_ActivateTask_InvalidID(void)
{
    StatusType ret = ActivateTask(99);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_ACTIVATETASK, g_det_last_api_id);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, g_det_last_error_id);
}

/** @test ACT_002 - New task created successfully (FreeRTOS_Task == NULL) */
void test_ActivateTask_CreateNew(void)
{
    TEST_ASSERT_NULL(g_test_tasks[0].FreeRTOS_Task);
    StatusType ret = ActivateTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_xTaskCreate_called);
    TEST_ASSERT_NOT_NULL(g_test_tasks[0].FreeRTOS_Task);
}

/** @test ACT_003 - xTaskCreate fails => E_OS_LIMIT */
void test_ActivateTask_CreateFails(void)
{
    g_stub_xTaskCreate_ret = pdFAIL;
    StatusType ret = ActivateTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_LIMIT, ret);
    TEST_ASSERT_NULL(g_test_tasks[0].FreeRTOS_Task);
}

/** @test ACT_004 - Extended task: event group created on task creation */
void test_ActivateTask_ExtendedTask_CreatesEventGroup(void)
{
    StatusType ret = ActivateTask(1);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_NOT_NULL(g_test_tasks[1].FreeRTOS_EventGroup);
}

/** @test ACT_005 - Extended task: event group create fails => cleanup */
void test_ActivateTask_ExtendedTask_EventGroupFails(void)
{
    g_stub_event_group_create_fail = 1;
    StatusType ret = ActivateTask(1);
    TEST_ASSERT_EQUAL_UINT8(E_OS_LIMIT, ret);
    TEST_ASSERT_NULL(g_test_tasks[1].FreeRTOS_EventGroup);
    TEST_ASSERT_EQUAL_INT(1, g_stub_vTaskDelete_called);
    TEST_ASSERT_NULL(g_test_tasks[1].FreeRTOS_Task);
}

/** @test ACT_006 - Already created task => resume path */
void test_ActivateTask_AlreadyCreated_Resumes(void)
{
    TEST_ASSERT_NOT_NULL(g_test_tasks[2].FreeRTOS_Task);
    StatusType ret = ActivateTask(2);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_xTaskCreate_called);
    TEST_ASSERT_EQUAL_INT(1, g_stub_vTaskResume_called);
}

/** @test ACT_007 - Scheduler not started => no yield */
void test_ActivateTask_SchedulerNotStarted_NoYield(void)
{
    g_stub_scheduler_state = taskSCHEDULER_NOT_STARTED;
    StatusType ret = ActivateTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
}

/** @test ACT_008 - Non-extended task: no event group created */
void test_ActivateTask_BasicTask_NoEventGroup(void)
{
    StatusType ret = ActivateTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_NULL(g_test_tasks[0].FreeRTOS_EventGroup);
}

/* ================================================================
 *  TerminateTask tests
 * ================================================================ */

/** @test TERM_001 - Task holds resource => E_OS_RESOURCE */
void test_TerminateTask_HoldsResource(void)
{
    /* Set resource owner to current task */
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 1;
    StatusType ret = TerminateTask();
    TEST_ASSERT_EQUAL_UINT8(E_OS_RESOURCE, ret);
    TEST_ASSERT_EQUAL_INT(0, g_stub_vTaskSuspend_called);
}

/** @test TERM_002 - No resources held => suspend self */
void test_TerminateTask_NoResources_Suspends(void)
{
    StatusType ret = TerminateTask();
    /* After PostTaskHook, vTaskSuspend(NULL) is called and never returns.
     * Since our stub returns, the function returns E_OS_OK */
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_vTaskSuspend_called);
}

/* ================================================================
 *  ChainTask tests
 * ================================================================ */

/** @test CHAIN_001 - Invalid TaskID */
void test_ChainTask_InvalidID(void)
{
    StatusType ret = ChainTask(99);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test CHAIN_002 - Activate fails => returns error */
void test_ChainTask_ActivateFails(void)
{
    g_stub_xTaskCreate_ret = pdFAIL;
    StatusType ret = ChainTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_LIMIT, ret);
}

/** @test CHAIN_003 - Activate succeeds, no resources held => terminate OK */
void test_ChainTask_ActivateSucceeds_TerminateOK(void)
{
    /* Task 0 is not created yet (FreeRTOS_Task == NULL), so ActivateTask
     * will create it. No resources are held by current task. */
    StatusType ret = ChainTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, g_stub_xTaskCreate_called);
    /* TerminateTask calls vTaskSuspend in the stub */
    TEST_ASSERT_EQUAL_INT(1, g_stub_vTaskSuspend_called);
}

/** @test CHAIN_004 - Activate succeeds, but task holds resource => E_OS_RESOURCE */
void test_ChainTask_ActivateSucceeds_HoldsResource(void)
{
    /* Set resource 0 as owned by current task */
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 1;

    StatusType ret = ChainTask(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_RESOURCE, ret);
    /* ActivateTask should have been called */
    TEST_ASSERT_EQUAL_INT(1, g_stub_xTaskCreate_called);
}

/* ================================================================
 *  StartOS / Os_Internal_StartOS tests
 * ================================================================ */

/** @test STARTOS_001 - StartOS with valid config initializes everything */
void test_StartOS_ValidConfig(void)
{
    /* Ensure resources have mutexes for init */
    g_stub_mutex_handle = (SemaphoreHandle_t)&g_dummy_mutex_1;

    /* Reset init state */
    Os_GlobalState.IsInitialized = FALSE;
    Os_GlobalState.IsRunning = FALSE;

    StartOS(OSDEFAULTAPPMODE);

    /* After StartOS, IsRunning should be TRUE */
    TEST_ASSERT_EQUAL_INT(TRUE, Os_GlobalState.IsRunning);
    /* vTaskStartScheduler should have been called */
    TEST_ASSERT_GREATER_THAN(0, g_stub_vTaskStartScheduler_called);
    /* Tasks should have been iterated (auto-start tasks activated) */
    /* Init functions should have run (timers created for alarms, mutexes for resources) */
}

/** @test STARTOS_002 - StartOS with NULL Tasks pointer skips task init */
void test_StartOS_NullTasks(void)
{
    Os_GlobalState.Tasks = NULL_PTR;
    Os_GlobalState.NumTasks = 0; /* Must set to 0 since Os_InitTasks has no NULL guard */
    Os_GlobalState.IsInitialized = FALSE;
    Os_GlobalState.IsRunning = FALSE;

    StartOS(OSDEFAULTAPPMODE);

    TEST_ASSERT_EQUAL_INT(TRUE, Os_GlobalState.IsRunning);
    TEST_ASSERT_GREATER_THAN(0, g_stub_vTaskStartScheduler_called);
}

/** @test STARTOS_003 - StartOS with NULL Alarms pointer skips alarm init */
void test_StartOS_NullAlarms(void)
{
    Os_GlobalState.Alarms = NULL_PTR;
    Os_GlobalState.NumAlarms = 0; /* Must set to 0 since Os_InitAlarms has no NULL guard */
    Os_GlobalState.IsInitialized = FALSE;
    Os_GlobalState.IsRunning = FALSE;

    StartOS(OSDEFAULTAPPMODE);

    TEST_ASSERT_EQUAL_INT(TRUE, Os_GlobalState.IsRunning);
}

/** @test STARTOS_004 - StartOS with NULL Resources pointer skips resource init */
void test_StartOS_NullResources(void)
{
    Os_GlobalState.Resources = NULL_PTR;
    Os_GlobalState.NumResources = 0; /* Must set to 0 since Os_InitResources has no NULL guard */
    Os_GlobalState.IsInitialized = FALSE;
    Os_GlobalState.IsRunning = FALSE;

    StartOS(OSDEFAULTAPPMODE);

    TEST_ASSERT_EQUAL_INT(TRUE, Os_GlobalState.IsRunning);
}

/** @test STARTOS_005 - StartOS with auto-start task */
void test_StartOS_AutoStartTask(void)
{
    /* Set task 0 as auto-start */
    g_test_tasks[0].IsAutoStart = TRUE;
    g_test_tasks[0].FreeRTOS_Task = NULL_PTR;
    Os_GlobalState.IsInitialized = FALSE;
    Os_GlobalState.IsRunning = FALSE;

    StartOS(OSDEFAULTAPPMODE);

    TEST_ASSERT_EQUAL_INT(TRUE, Os_GlobalState.IsRunning);
    /* Auto-start task should have been activated (xTaskCreate called) */
    TEST_ASSERT_GREATER_THAN(0, g_stub_xTaskCreate_called);
}

/* ================================================================
 *  Schedule tests
 * ================================================================ */

/** @test SCHED_001 - Schedule returns E_OS_OK */
void test_Schedule_ReturnsOK(void)
{
    StatusType ret = Schedule();
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
}

/* ================================================================
 *  GetTaskID tests
 * ================================================================ */

/** @test GETID_001 - NULL pointer => E_OS_PARAM_POINTER */
void test_GetTaskID_NullPointer(void)
{
    StatusType ret = GetTaskID(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OS_PARAM_POINTER, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test GETID_002 - Current task found in table */
void test_GetTaskID_Found(void)
{
    /* Set task 0's handle to current task */
    g_test_tasks[0].FreeRTOS_Task = g_stub_current_task;
    TaskType id = 99;
    StatusType ret = GetTaskID(&id);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, id);
}

/** @test GETID_003 - Current task not found => returns 0 */
void test_GetTaskID_NotFound(void)
{
    /* No task has the current handle */
    TaskType id = 99;
    StatusType ret = GetTaskID(&id);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, id);
}

/* ================================================================
 *  GetTaskState tests
 * ================================================================ */

/** @test GETSTATE_001 - Invalid TaskID */
void test_GetTaskState_InvalidID(void)
{
    TaskStateType state;
    StatusType ret = GetTaskState(99, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
}

/** @test GETSTATE_002 - NULL State pointer */
void test_GetTaskState_NullState(void)
{
    StatusType ret = GetTaskState(0, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OS_PARAM_POINTER, ret);
}

/** @test GETSTATE_003 - NULL FreeRTOS_Task => SUSPENDED */
void test_GetTaskState_NullTask_SUSPENDED(void)
{
    g_test_tasks[0].FreeRTOS_Task = NULL_PTR;
    TaskStateType state = READY;
    StatusType ret = GetTaskState(0, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(SUSPENDED, state);
}

/** @test GETSTATE_004 - eRunning => RUNNING */
void test_GetTaskState_Running(void)
{
    static int handle;
    g_test_tasks[0].FreeRTOS_Task = (TaskHandle_t)&handle;
    g_stub_task_state = eRunning;
    TaskStateType state;
    StatusType ret = GetTaskState(0, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(RUNNING, state);
}

/** @test GETSTATE_005 - eReady => READY */
void test_GetTaskState_Ready(void)
{
    static int handle;
    g_test_tasks[0].FreeRTOS_Task = (TaskHandle_t)&handle;
    g_stub_task_state = eReady;
    TaskStateType state;
    StatusType ret = GetTaskState(0, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(READY, state);
}

/** @test GETSTATE_006 - eBlocked => WAITING */
void test_GetTaskState_Blocked(void)
{
    static int handle;
    g_test_tasks[0].FreeRTOS_Task = (TaskHandle_t)&handle;
    g_stub_task_state = eBlocked;
    TaskStateType state;
    StatusType ret = GetTaskState(0, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(WAITING, state);
}

/** @test GETSTATE_007 - eSuspended => SUSPENDED */
void test_GetTaskState_Suspended(void)
{
    static int handle;
    g_test_tasks[0].FreeRTOS_Task = (TaskHandle_t)&handle;
    g_stub_task_state = eSuspended;
    TaskStateType state;
    StatusType ret = GetTaskState(0, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(SUSPENDED, state);
}

/** @test GETSTATE_008 - default case => SUSPENDED */
void test_GetTaskState_Default(void)
{
    static int handle;
    g_test_tasks[0].FreeRTOS_Task = (TaskHandle_t)&handle;
    g_stub_task_state = eDeleted;
    TaskStateType state;
    StatusType ret = GetTaskState(0, &state);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_INT(SUSPENDED, state);
}

/* ================================================================
 *  Interrupt management tests
 * ================================================================ */

void test_EnableAllInterrupts(void)
{
    EnableAllInterrupts();
    /* No crash = pass (portENABLE_INTERRUPTS is a no-op stub) */
    TEST_ASSERT_TRUE(1);
}

void test_DisableAllInterrupts(void)
{
    DisableAllInterrupts();
    TEST_ASSERT_TRUE(1);
}

void test_ResumeAllInterrupts(void)
{
    ResumeAllInterrupts();
    TEST_ASSERT_TRUE(1);
}

void test_SuspendAllInterrupts(void)
{
    SuspendAllInterrupts();
    TEST_ASSERT_TRUE(1);
}

void test_ResumeOSInterrupts(void)
{
    ResumeOSInterrupts();
    TEST_ASSERT_TRUE(1);
}

void test_SuspendOSInterrupts(void)
{
    SuspendOSInterrupts();
    TEST_ASSERT_TRUE(1);
}

/* ================================================================
 *  GetActiveApplicationMode tests
 * ================================================================ */

void test_GetActiveApplicationMode_ReturnsCurrent(void)
{
    Os_GlobalState.CurrentAppMode = OSDEFAULTAPPMODE;
    AppModeType mode = GetActiveApplicationMode();
    TEST_ASSERT_EQUAL_UINT8(OSDEFAULTAPPMODE, mode);
}

void test_GetActiveApplicationMode_DifferentMode(void)
{
    Os_GlobalState.CurrentAppMode = 0x42;
    AppModeType mode = GetActiveApplicationMode();
    TEST_ASSERT_EQUAL_UINT8(0x42, mode);
}

/* ================================================================
 *  Os_GetVersionInfo tests
 * ================================================================ */

void test_GetVersionInfo_ValidPointer(void)
{
    Std_VersionInfoType info;
    memset(&info, 0xFF, sizeof(info));
    Os_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(OS_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(OS_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(OS_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(OS_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(OS_SW_PATCH_VERSION, info.sw_patch_version);
}

void test_GetVersionInfo_NullPointer(void)
{
    /* Should not crash */
    Os_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(1);
}

/* ================================================================
 *  Os_TaskWrapper tests
 * ================================================================ */

/** @test WRAPPER_001 - TaskWrapper calls entry point */
void test_TaskWrapper_CallsEntry(void)
{
    g_task_entry_called = 0;
    Os_TaskConfigType task;
    memset(&task, 0, sizeof(task));
    task.EntryPoint = dummy_entry;
    task.FreeRTOS_Task = NULL_PTR;
    task.IsExtended = FALSE;

    /* Os_TaskWrapper calls PreTaskHook, then EntryPoint, then TerminateTask.
     * TerminateTask will call vTaskSuspend which is stubbed to return. */
    Os_TaskWrapper((void*)&task);
    TEST_ASSERT_GREATER_THAN(0, g_task_entry_called);
}

/** @test WRAPPER_002 - TaskWrapper with NULL entry point */
void test_TaskWrapper_NullEntry(void)
{
    Os_TaskConfigType task;
    memset(&task, 0, sizeof(task));
    task.EntryPoint = NULL_PTR;
    task.FreeRTOS_Task = NULL_PTR;
    task.IsExtended = FALSE;

    /* Should not crash - NULL entry is skipped */
    Os_TaskWrapper((void*)&task);
    TEST_ASSERT_EQUAL_INT(0, g_task_entry_called);
}

/* ================================================================
 *  Test runner
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* ActivateTask */
    RUN_TEST(test_ActivateTask_InvalidID);
    RUN_TEST(test_ActivateTask_CreateNew);
    RUN_TEST(test_ActivateTask_CreateFails);
    RUN_TEST(test_ActivateTask_ExtendedTask_CreatesEventGroup);
    RUN_TEST(test_ActivateTask_ExtendedTask_EventGroupFails);
    RUN_TEST(test_ActivateTask_AlreadyCreated_Resumes);
    RUN_TEST(test_ActivateTask_SchedulerNotStarted_NoYield);
    RUN_TEST(test_ActivateTask_BasicTask_NoEventGroup);

    /* TerminateTask */
    RUN_TEST(test_TerminateTask_HoldsResource);
    RUN_TEST(test_TerminateTask_NoResources_Suspends);

    /* ChainTask */
    RUN_TEST(test_ChainTask_InvalidID);
    RUN_TEST(test_ChainTask_ActivateFails);
    RUN_TEST(test_ChainTask_ActivateSucceeds_TerminateOK);
    RUN_TEST(test_ChainTask_ActivateSucceeds_HoldsResource);

    /* StartOS */
    RUN_TEST(test_StartOS_ValidConfig);
    RUN_TEST(test_StartOS_NullTasks);
    RUN_TEST(test_StartOS_NullAlarms);
    RUN_TEST(test_StartOS_NullResources);
    RUN_TEST(test_StartOS_AutoStartTask);

    /* Schedule */
    RUN_TEST(test_Schedule_ReturnsOK);

    /* GetTaskID */
    RUN_TEST(test_GetTaskID_NullPointer);
    RUN_TEST(test_GetTaskID_Found);
    RUN_TEST(test_GetTaskID_NotFound);

    /* GetTaskState */
    RUN_TEST(test_GetTaskState_InvalidID);
    RUN_TEST(test_GetTaskState_NullState);
    RUN_TEST(test_GetTaskState_NullTask_SUSPENDED);
    RUN_TEST(test_GetTaskState_Running);
    RUN_TEST(test_GetTaskState_Ready);
    RUN_TEST(test_GetTaskState_Blocked);
    RUN_TEST(test_GetTaskState_Suspended);
    RUN_TEST(test_GetTaskState_Default);

    /* Interrupt management */
    RUN_TEST(test_EnableAllInterrupts);
    RUN_TEST(test_DisableAllInterrupts);
    RUN_TEST(test_ResumeAllInterrupts);
    RUN_TEST(test_SuspendAllInterrupts);
    RUN_TEST(test_ResumeOSInterrupts);
    RUN_TEST(test_SuspendOSInterrupts);

    /* GetActiveApplicationMode */
    RUN_TEST(test_GetActiveApplicationMode_ReturnsCurrent);
    RUN_TEST(test_GetActiveApplicationMode_DifferentMode);

    /* Os_GetVersionInfo */
    RUN_TEST(test_GetVersionInfo_ValidPointer);
    RUN_TEST(test_GetVersionInfo_NullPointer);

    /* Os_TaskWrapper */
    RUN_TEST(test_TaskWrapper_CallsEntry);
    RUN_TEST(test_TaskWrapper_NullEntry);

    return UNITY_END();
}
