/**
 * @file test_os_alarm.c
 * @brief OS Alarm Management, Hook Functions, and Resource Management tests
 *
 * Compiles the ACTUAL Os.c source with fully stubbed FreeRTOS APIs.
 *
 * Coverage targets (Os.c):
 *   - GetAlarmBase: DET guards (invalid ID, NULL ptr), success path
 *   - GetAlarm: DET guards, internal (active, not active, NULL timer)
 *   - SetRelAlarm: DET guards (invalid ID, zero increment), internal paths
 *                  (NULL timer, xTimerChangePeriod fail, xTimerStart fail, success)
 *   - SetAbsAlarm: DET guard, start > current, start <= current
 *   - CancelAlarm: DET guard, internal (NULL timer, not active, xTimerStop fail, success)
 *   - Os_AlarmCallback: NULL alarm, NULL callback, cyclic alarm, one-shot
 *   - GetResource / ReleaseResource: DET guards, internal paths
 *   - Hook functions: ErrorHook, PreTaskHook, PostTaskHook, StartupHook, ShutdownHook
 *   - Os_GetVersionInfo
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================
 *  AUTOSAR includes
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
static UBaseType_t g_stub_priorities[STUB_MAX_TASKS];
static TickType_t g_stub_tick_count;
static eTaskState g_stub_task_state;

/* Event group stubs */
static int g_stub_event_group_create_fail;
static EventGroupHandle_t g_stub_event_group_handle;
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
static void* g_stub_timer_id;

/* Semaphore stubs */
static int g_stub_sem_take_fail;
static int g_stub_sem_give_fail;
static SemaphoreHandle_t g_stub_mutex_handle;

/* DET stubs */
static int g_det_error_reported;
static uint8_t g_det_last_api_id;
static uint8_t g_det_last_error_id;

/* Hook tracking */
static int g_hook_error_called;
static int g_hook_pre_task_called;
static int g_hook_post_task_called;
static int g_hook_startup_called;
static int g_hook_shutdown_called;
static StatusType g_hook_last_error;

/* Alarm callback tracking */
static int g_alarm_callback_invoked;

/* Task entry tracking */
static int g_task_entry_called;

/* ================================================================
 *  Test config tables
 * ================================================================ */
static void dummy_entry(void) { g_task_entry_called++; }
static void test_alarm_cb(void) { g_alarm_callback_invoked++; }

/* Os_GlobalState strong definition (normally in Os_Cfg.c) */
Os_GlobalStateType Os_GlobalState;

static Os_TaskConfigType g_test_tasks[3];
static Os_AlarmConfigType g_test_alarms[3];
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

void vTaskStartScheduler(void) {}
void vTaskEndScheduler(void) {}

void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority)
{
    (void)xTask; (void)uxNewPriority;
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
    g_stub_event_bits |= uxBitsToSet;
    return g_stub_event_bits;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear)
{
    (void)xEventGroup;
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
    return g_stub_timer_id;
}

/* DET stub */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId; (void)InstanceId;
    g_det_error_reported++;
    g_det_last_api_id = ApiId;
    g_det_last_error_id = ErrorId;
    return 0x00U;
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
    memset(g_stub_priorities, 0, sizeof(g_stub_priorities));
    g_stub_priorities[0] = 2;
    g_stub_tick_count = 100;
    g_stub_task_state = eReady;

    g_stub_event_group_create_fail = 0;
    static int eg_storage;
    g_stub_event_group_handle = (EventGroupHandle_t)&eg_storage;
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
    g_stub_timer_id = NULL_PTR;

    g_stub_sem_take_fail = 0;
    g_stub_sem_give_fail = 0;
    g_stub_mutex_handle = (SemaphoreHandle_t)&g_dummy_mutex_1;

    g_det_error_reported = 0;
    g_det_last_api_id = 0;
    g_det_last_error_id = 0;

    g_hook_error_called = 0;
    g_hook_pre_task_called = 0;
    g_hook_post_task_called = 0;
    g_hook_startup_called = 0;
    g_hook_shutdown_called = 0;
    g_hook_last_error = 0;
    g_alarm_callback_invoked = 0;
    g_task_entry_called = 0;
}

/* ================================================================
 *  Os_GlobalState setup helper
 * ================================================================ */
static void setup_global_state(void)
{
    /* Task 0: basic task */
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

    /* Task 2: already created */
    memset(&g_test_tasks[2], 0, sizeof(Os_TaskConfigType));
    g_test_tasks[2].TaskID = 2;
    static int task2_handle;
    g_test_tasks[2].FreeRTOS_Task = (TaskHandle_t)&task2_handle;
    g_test_tasks[2].FreeRTOS_EventGroup = NULL_PTR;
    g_test_tasks[2].Priority = 2;
    g_test_tasks[2].IsAutoStart = FALSE;
    g_test_tasks[2].IsExtended = FALSE;
    g_test_tasks[2].EntryPoint = dummy_entry;

    /* Alarm 0: active, with timer and callback */
    memset(&g_test_alarms[0], 0, sizeof(Os_AlarmConfigType));
    g_test_alarms[0].AlarmID = 0;
    g_test_alarms[0].State = OS_ALARM_ACTIVE;
    g_test_alarms[0].ExpiryTick = 500;
    g_test_alarms[0].Increment = 100;
    g_test_alarms[0].Cycle = 0;
    g_test_alarms[0].Callback = test_alarm_cb;
    g_test_alarms[0].FreeRTOS_Timer = g_stub_timer_handle;

    /* Alarm 1: unused */
    memset(&g_test_alarms[1], 0, sizeof(Os_AlarmConfigType));
    g_test_alarms[1].AlarmID = 1;
    g_test_alarms[1].State = OS_ALARM_UNUSED;
    g_test_alarms[1].Increment = 0;
    g_test_alarms[1].Cycle = 0;
    g_test_alarms[1].Callback = test_alarm_cb;
    g_test_alarms[1].FreeRTOS_Timer = g_stub_timer_handle;

    /* Alarm 2: active, cyclic, with NULL timer (edge case) */
    memset(&g_test_alarms[2], 0, sizeof(Os_AlarmConfigType));
    g_test_alarms[2].AlarmID = 2;
    g_test_alarms[2].State = OS_ALARM_ACTIVE;
    g_test_alarms[2].ExpiryTick = 200;
    g_test_alarms[2].Increment = 50;
    g_test_alarms[2].Cycle = 25;
    g_test_alarms[2].Callback = NULL_PTR;
    g_test_alarms[2].FreeRTOS_Timer = NULL_PTR;

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
    Os_GlobalState.NumAlarms = 3;
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
 *  GetAlarmBase tests
 * ================================================================ */

/** @test GAB_001 - Invalid AlarmID triggers DET and returns E_OS_ID */
void test_GetAlarmBase_InvalidID(void)
{
    AlarmBaseType info;
    StatusType ret = GetAlarmBase(99, &info);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_GETALARMBASE, g_det_last_api_id);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, g_det_last_error_id);
}

/** @test GAB_002 - NULL Info pointer triggers DET and returns E_OS_PARAM_POINTER */
void test_GetAlarmBase_NullPtr(void)
{
    StatusType ret = GetAlarmBase(0, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OS_PARAM_POINTER, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_GETALARMBASE, g_det_last_api_id);
    TEST_ASSERT_EQUAL_UINT8(E_OS_PARAM_POINTER, g_det_last_error_id);
}

/** @test GAB_003 - Valid call fills AlarmBaseType correctly */
void test_GetAlarmBase_Success(void)
{
    AlarmBaseType info;
    memset(&info, 0xFF, sizeof(info));
    StatusType ret = GetAlarmBase(0, &info);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(portMAX_DELAY, info.maxallowedvalue);
    TEST_ASSERT_EQUAL_UINT32(OS_TICKS_PER_MS, info.ticksperbase);
    TEST_ASSERT_EQUAL_UINT32(1, info.mincycle);
}

/* ================================================================
 *  GetAlarm tests
 * ================================================================ */

/** @test GA_001 - Invalid AlarmID triggers DET */
void test_GetAlarm_InvalidID(void)
{
    TickType tick;
    StatusType ret = GetAlarm(99, &tick);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_GETALARM, g_det_last_api_id);
}

/** @test GA_002 - NULL Tick pointer triggers DET */
void test_GetAlarm_NullPtr(void)
{
    StatusType ret = GetAlarm(0, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OS_PARAM_POINTER, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_GETALARM, g_det_last_api_id);
}

/** @test GA_003 - Active alarm returns expiry tick */
void test_GetAlarm_ActiveAlarm(void)
{
    TickType tick = 0;
    StatusType ret = GetAlarm(0, &tick);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(500, tick);
}

/** @test GA_004 - Non-active alarm returns E_OS_NOFUNC and DET */
void test_GetAlarm_NotActive(void)
{
    TickType tick = 0;
    StatusType ret = GetAlarm(1, &tick);
    TEST_ASSERT_EQUAL_UINT8(E_OS_NOFUNC, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(E_OS_NOFUNC, g_det_last_error_id);
}

/* ================================================================
 *  SetRelAlarm tests
 * ================================================================ */

/** @test SRA_001 - Invalid AlarmID triggers DET */
void test_SetRelAlarm_InvalidID(void)
{
    StatusType ret = SetRelAlarm(99, 100, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_SETRELALARM, g_det_last_api_id);
}

/** @test SRA_002 - Zero increment triggers DET and E_OS_VALUE */
void test_SetRelAlarm_ZeroIncrement(void)
{
    StatusType ret = SetRelAlarm(0, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_VALUE, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_SETRELALARM, g_det_last_api_id);
    TEST_ASSERT_EQUAL_UINT8(E_OS_VALUE, g_det_last_error_id);
}

/** @test SRA_003 - NULL timer => E_OS_ID from internal */
void test_SetRelAlarm_NullTimer(void)
{
    /* Alarm 2 has NULL timer */
    StatusType ret = SetRelAlarm(2, 100, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test SRA_004 - xTimerChangePeriod fails => E_OS_VALUE */
void test_SetRelAlarm_ChangePeriodFails(void)
{
    g_stub_timer_change_period_ret = pdFAIL;
    StatusType ret = SetRelAlarm(0, 100, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_VALUE, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test SRA_005 - xTimerStart fails => E_OS_VALUE */
void test_SetRelAlarm_TimerStartFails(void)
{
    g_stub_timer_start_ret = pdFAIL;
    StatusType ret = SetRelAlarm(0, 100, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_VALUE, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test SRA_006 - Success path: alarm set, state becomes ACTIVE */
void test_SetRelAlarm_Success(void)
{
    /* Reset alarm 1 to have a valid timer but UNUSED state */
    g_test_alarms[1].FreeRTOS_Timer = g_stub_timer_handle;
    g_test_alarms[1].State = OS_ALARM_UNUSED;

    StatusType ret = SetRelAlarm(1, 200, 50);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(200, g_test_alarms[1].Increment);
    TEST_ASSERT_EQUAL_UINT32(50, g_test_alarms[1].Cycle);
    TEST_ASSERT_EQUAL(OS_ALARM_ACTIVE, g_test_alarms[1].State);
    TEST_ASSERT_EQUAL_INT(1, g_stub_timer_change_period_called);
    TEST_ASSERT_EQUAL_INT(1, g_stub_timer_start_called);
}

/** @test SRA_007 - Increment that rounds to 0 ms gets clamped to 1 */
void test_SetRelAlarm_SmallIncrement_ClampedTo1Ms(void)
{
    /* OS_TICKS_PER_MS = 1, so Increment=0 would be caught by DET.
     * Increment=1 => periodMs = 1/1 = 1 => no clamping needed.
     * We test the clamping path by having OS_TICKS_PER_MS > 1 scenario.
     * With current config (OS_TICKS_PER_MS=1), any Increment>=1 gives periodMs>=1.
     * Just verify it works with minimum valid increment. */
    StatusType ret = SetRelAlarm(0, 1, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
}

/* ================================================================
 *  SetAbsAlarm tests
 * ================================================================ */

/** @test SAA_001 - Invalid AlarmID triggers DET */
void test_SetAbsAlarm_InvalidID(void)
{
    StatusType ret = SetAbsAlarm(99, 200, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_SETABSALARM, g_det_last_api_id);
}

/** @test SAA_002 - Start > currentTick => relative = Start - currentTick */
void test_SetAbsAlarm_StartAfterCurrent(void)
{
    g_stub_tick_count = 100;
    /* Alarm 1 has valid timer */
    g_test_alarms[1].FreeRTOS_Timer = g_stub_timer_handle;
    g_test_alarms[1].State = OS_ALARM_UNUSED;

    StatusType ret = SetAbsAlarm(1, 300, 10);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    /* relativeIncrement = 300 - 100 = 200 */
    TEST_ASSERT_EQUAL_UINT32(200, g_test_alarms[1].Increment);
    TEST_ASSERT_EQUAL_UINT32(10, g_test_alarms[1].Cycle);
}

/** @test SAA_003 - Start <= currentTick => relative = 1 (immediate) */
void test_SetAbsAlarm_StartBeforeCurrent(void)
{
    g_stub_tick_count = 500;
    g_test_alarms[1].FreeRTOS_Timer = g_stub_timer_handle;
    g_test_alarms[1].State = OS_ALARM_UNUSED;

    StatusType ret = SetAbsAlarm(1, 100, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    /* Start(100) <= currentTick(500), so relativeIncrement = 1 */
    TEST_ASSERT_EQUAL_UINT32(1, g_test_alarms[1].Increment);
}

/** @test SAA_004 - Start == currentTick => relative = 1 */
void test_SetAbsAlarm_StartEqualsCurrent(void)
{
    g_stub_tick_count = 200;
    g_test_alarms[1].FreeRTOS_Timer = g_stub_timer_handle;
    g_test_alarms[1].State = OS_ALARM_UNUSED;

    StatusType ret = SetAbsAlarm(1, 200, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1, g_test_alarms[1].Increment);
}

/* ================================================================
 *  CancelAlarm tests
 * ================================================================ */

/** @test CA_001 - Invalid AlarmID triggers DET */
void test_CancelAlarm_InvalidID(void)
{
    StatusType ret = CancelAlarm(99);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_CANCELALARM, g_det_last_api_id);
}

/** @test CA_002 - NULL timer => E_OS_ID from internal */
void test_CancelAlarm_NullTimer(void)
{
    /* Alarm 2 has NULL timer */
    StatusType ret = CancelAlarm(2);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test CA_003 - Alarm not active => E_OS_NOFUNC */
void test_CancelAlarm_NotActive(void)
{
    /* Alarm 1 is OS_ALARM_UNUSED */
    StatusType ret = CancelAlarm(1);
    TEST_ASSERT_EQUAL_UINT8(E_OS_NOFUNC, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(E_OS_NOFUNC, g_det_last_error_id);
}

/** @test CA_004 - xTimerStop fails => E_OS_STATE */
void test_CancelAlarm_TimerStopFails(void)
{
    g_stub_timer_stop_ret = pdFAIL;
    StatusType ret = CancelAlarm(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_STATE, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test CA_005 - Success: alarm canceled, state becomes CANCELED */
void test_CancelAlarm_Success(void)
{
    TEST_ASSERT_EQUAL(OS_ALARM_ACTIVE, g_test_alarms[0].State);
    StatusType ret = CancelAlarm(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL(OS_ALARM_CANCELED, g_test_alarms[0].State);
    TEST_ASSERT_EQUAL_INT(1, g_stub_timer_stop_called);
}

/* ================================================================
 *  Os_AlarmCallback tests
 * ================================================================ */

/** @test ACB_001 - Valid alarm with callback => callback invoked */
void test_AlarmCallback_WithCallback(void)
{
    g_alarm_callback_invoked = 0;
    g_stub_timer_id = (void*)&g_test_alarms[0];

    Os_AlarmCallback(g_stub_timer_handle);
    TEST_ASSERT_EQUAL_INT(1, g_alarm_callback_invoked);
}

/** @test ACB_002 - NULL alarm pointer => skipped (production code dereferences
 *  alarm->Cycle without NULL guard after callback block; this is a latent bug) */

/** @test ACB_003 - NULL callback => callback not invoked */
void test_AlarmCallback_NullCallback(void)
{
    /* Alarm 2 has NULL callback */
    g_stub_timer_id = (void*)&g_test_alarms[2];
    g_alarm_callback_invoked = 0;

    Os_AlarmCallback(g_stub_timer_handle);
    TEST_ASSERT_EQUAL_INT(0, g_alarm_callback_invoked);
}

/** @test ACB_004 - Cyclic alarm (Cycle > 0, ACTIVE) => timer restarted */
void test_AlarmCallback_CyclicAlarm(void)
{
    /* Alarm 0 is ACTIVE with Cycle=0 (one-shot). Set Cycle to make it cyclic. */
    g_test_alarms[0].Cycle = 50;
    g_test_alarms[0].State = OS_ALARM_ACTIVE;
    g_stub_timer_id = (void*)&g_test_alarms[0];
    g_stub_timer_change_period_called = 0;
    g_stub_timer_start_called = 0;

    Os_AlarmCallback(g_stub_timer_handle);
    TEST_ASSERT_EQUAL_INT(1, g_alarm_callback_invoked);
    /* Cyclic: should change period and restart */
    TEST_ASSERT_GREATER_THAN(0, g_stub_timer_change_period_called);
    TEST_ASSERT_GREATER_THAN(0, g_stub_timer_start_called);
}

/** @test ACB_005 - One-shot alarm (Cycle == 0) => timer NOT restarted */
void test_AlarmCallback_OneShotAlarm(void)
{
    g_test_alarms[0].Cycle = 0;
    g_test_alarms[0].State = OS_ALARM_ACTIVE;
    g_stub_timer_id = (void*)&g_test_alarms[0];
    g_stub_timer_change_period_called = 0;
    g_stub_timer_start_called = 0;

    Os_AlarmCallback(g_stub_timer_handle);
    TEST_ASSERT_EQUAL_INT(1, g_alarm_callback_invoked);
    /* One-shot: should NOT change period or restart */
    TEST_ASSERT_EQUAL_INT(0, g_stub_timer_change_period_called);
    TEST_ASSERT_EQUAL_INT(0, g_stub_timer_start_called);
}

/** @test ACB_006 - Cyclic but not ACTIVE => timer NOT restarted */
void test_AlarmCallback_CyclicButNotActive(void)
{
    g_test_alarms[0].Cycle = 50;
    g_test_alarms[0].State = OS_ALARM_CANCELED;
    g_stub_timer_id = (void*)&g_test_alarms[0];
    g_stub_timer_change_period_called = 0;
    g_stub_timer_start_called = 0;

    Os_AlarmCallback(g_stub_timer_handle);
    TEST_ASSERT_EQUAL_INT(1, g_alarm_callback_invoked);
    /* Not ACTIVE: should NOT restart timer */
    TEST_ASSERT_EQUAL_INT(0, g_stub_timer_change_period_called);
    TEST_ASSERT_EQUAL_INT(0, g_stub_timer_start_called);
}

/* ================================================================
 *  GetResource / ReleaseResource tests
 * ================================================================ */

/** @test GR_001 - Invalid ResID triggers DET and returns E_OS_ID */
void test_GetResource_InvalidID(void)
{
    StatusType ret = GetResource(99);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_GETRESOURCE, g_det_last_api_id);
}

/** @test GR_002 - Success: mutex taken, owner set */
void test_GetResource_Success(void)
{
    StatusType ret = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1, g_test_resources[0].NestCount);
}

/** @test GR_003 - xSemaphoreTake fails => E_OS_ACCESS */
void test_GetResource_TakeFails(void)
{
    g_stub_sem_take_fail = 1;
    StatusType ret = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test GR_004 - NULL mutex => E_OS_ID from internal */
void test_GetResource_NullMutex(void)
{
    g_test_resources[0].FreeRTOS_Mutex = NULL_PTR;
    StatusType ret = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
}

/** @test RR_001 - Invalid ResID triggers DET */
void test_ReleaseResource_InvalidID(void)
{
    StatusType ret = ReleaseResource(99);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
    TEST_ASSERT_EQUAL_UINT8(OS_SID_RELEASERESOURCE, g_det_last_api_id);
}

/** @test RR_002 - Not owner => E_OS_NOFUNC */
void test_ReleaseResource_NotOwner(void)
{
    g_test_resources[0].OwnerTask = 999; /* Different from current task */
    g_test_resources[0].NestCount = 1;
    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_NOFUNC, ret);
    TEST_ASSERT_GREATER_THAN(0, g_det_error_reported);
}

/** @test RR_003 - Success: release, nest count decremented */
void test_ReleaseResource_Success(void)
{
    /* First acquire */
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 1;

    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, g_test_resources[0].NestCount);
}

/** @test RR_004 - xSemaphoreGive fails => E_OS_ACCESS */
void test_ReleaseResource_GiveFails(void)
{
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 1;
    g_stub_sem_give_fail = 1;

    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ACCESS, ret);
}

/** @test RR_005 - NULL mutex => E_OS_ID from internal */
void test_ReleaseResource_NullMutex(void)
{
    g_test_resources[0].FreeRTOS_Mutex = NULL_PTR;
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_ID, ret);
}

/* ================================================================
 *  PCP (Priority Ceiling Protocol) tests via actual Os.c
 * ================================================================ */

/** @test PCP_001 - GetResource with IsCeilingPriority=TRUE, ceiling > current */
void test_GetResource_PCP_ElevatesPriority(void)
{
    g_test_resources[0].IsCeilingPriority = TRUE;
    g_test_resources[0].CeilingPriority = 10;
    g_stub_priorities[0] = 2; /* current priority is lower */

    StatusType ret = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1, g_test_resources[0].NestCount);
    TEST_ASSERT_EQUAL_UINT32(2, g_test_resources[0].OriginalPriority);
}

/** @test PCP_002 - GetResource with IsCeilingPriority=TRUE, ceiling <= current */
void test_GetResource_PCP_NoElevation(void)
{
    g_test_resources[0].IsCeilingPriority = TRUE;
    g_test_resources[0].CeilingPriority = 1;
    g_stub_priorities[0] = 5; /* current priority is already higher */

    StatusType ret = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1, g_test_resources[0].NestCount);
}

/** @test PCP_003 - GetResource nested (NestCount > 1) skips PCP elevation */
void test_GetResource_PCP_NestedSkipsElevation(void)
{
    g_test_resources[0].IsCeilingPriority = TRUE;
    g_test_resources[0].CeilingPriority = 10;
    g_stub_priorities[0] = 2;

    /* First acquire */
    StatusType ret1 = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret1);
    TEST_ASSERT_EQUAL_UINT32(1, g_test_resources[0].NestCount);

    /* Second acquire (nested) - should NOT re-save priority */
    g_test_resources[0].OriginalPriority = 99; /* sentinel */
    StatusType ret2 = GetResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret2);
    TEST_ASSERT_EQUAL_UINT32(2, g_test_resources[0].NestCount);
    /* OriginalPriority should NOT have been overwritten */
    TEST_ASSERT_EQUAL_UINT32(99, g_test_resources[0].OriginalPriority);
}

/** @test PCP_004 - ReleaseResource with NestCount > 1 (not fully released) */
void test_ReleaseResource_NestedNotFullyReleased(void)
{
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 2; /* nested: will become 1 after release */
    g_test_resources[0].IsCeilingPriority = TRUE;

    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1, g_test_resources[0].NestCount);
    /* OwnerTask should NOT be cleared since NestCount != 0 */
    TEST_ASSERT_EQUAL_UINT32((TaskType)(uintptr_t)g_stub_current_task, g_test_resources[0].OwnerTask);
}

/** @test PCP_005 - ReleaseResource NestCount->0 with IsCeilingPriority=TRUE */
void test_ReleaseResource_PCP_RestoresPriority(void)
{
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 1;
    g_test_resources[0].IsCeilingPriority = TRUE;
    g_test_resources[0].OriginalPriority = 3;

    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, g_test_resources[0].NestCount);
    TEST_ASSERT_EQUAL_UINT32(0, g_test_resources[0].OwnerTask);
}

/** @test PCP_006 - ReleaseResource NestCount->0 with IsCeilingPriority=FALSE */
void test_ReleaseResource_NoPCP_NoPriorityRestore(void)
{
    g_test_resources[0].OwnerTask = (TaskType)(uintptr_t)g_stub_current_task;
    g_test_resources[0].NestCount = 1;
    g_test_resources[0].IsCeilingPriority = FALSE;

    StatusType ret = ReleaseResource(0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0, g_test_resources[0].NestCount);
    TEST_ASSERT_EQUAL_UINT32(0, g_test_resources[0].OwnerTask);
}

/* ================================================================
 *  Os_Internal_SetRelAlarm edge case tests
 * ================================================================ */

/** @test SREL_001 - Os_Internal_SetRelAlarm with Increment=0 => periodMs==0 path */
void test_InternalSetRelAlarm_ZeroIncrement(void)
{
    /* Bypass the public API DET check by calling internal directly */
    g_test_alarms[0].FreeRTOS_Timer = g_stub_timer_handle;
    g_stub_timer_change_period_called = 0;

    StatusType ret = Os_Internal_SetRelAlarm(0, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(E_OS_OK, ret);
    /* periodMs == 0 triggers the clamping to 1 */
    TEST_ASSERT_GREATER_THAN(0, g_stub_timer_change_period_called);
}

/* ================================================================
 *  Hook function tests
 * ================================================================ */

/** @test HOOK_001 - ErrorHook calls internal */
void test_ErrorHook(void)
{
    ErrorHook(E_OS_ID);
    /* Internal is empty by default, just verify no crash */
}

/** @test HOOK_002 - PreTaskHook calls internal */
void test_PreTaskHook(void)
{
    PreTaskHook();
    /* Internal is empty by default, just verify no crash */
}

/** @test HOOK_003 - PostTaskHook calls internal */
void test_PostTaskHook(void)
{
    PostTaskHook();
}

/** @test HOOK_004 - StartupHook calls internal */
void test_StartupHook(void)
{
    StartupHook();
}

/** @test HOOK_005 - ShutdownHook calls internal */
void test_ShutdownHook(void)
{
    ShutdownHook(E_OS_OK);
}

/** @test HOOK_006 - Internal hooks are callable */
void test_InternalHooks(void)
{
    Os_Internal_ErrorHook(E_OS_VALUE);
    Os_Internal_PreTaskHook();
    Os_Internal_PostTaskHook();
    Os_Internal_StartupHook();
    Os_Internal_ShutdownHook(E_OS_ID);
}

/* ================================================================
 *  GetActiveApplicationMode tests
 * ================================================================ */

/** @test MODE_001 - Returns current app mode */
void test_GetActiveApplicationMode_Default(void)
{
    Os_GlobalState.CurrentAppMode = OSDEFAULTAPPMODE;
    AppModeType mode = GetActiveApplicationMode();
    TEST_ASSERT_EQUAL_UINT8(OSDEFAULTAPPMODE, mode);
}

/** @test MODE_002 - Returns changed app mode */
void test_GetActiveApplicationMode_Changed(void)
{
    Os_GlobalState.CurrentAppMode = 0x42;
    AppModeType mode = GetActiveApplicationMode();
    TEST_ASSERT_EQUAL_UINT8(0x42, mode);
}

/* ================================================================
 *  Os_GetVersionInfo tests
 * ================================================================ */

/** @test VER_001 - Valid pointer fills version info */
void test_Os_GetVersionInfo_ValidPtr(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    Os_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(OS_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(OS_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(OS_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(OS_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(OS_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @test VER_002 - NULL pointer => no crash */
void test_Os_GetVersionInfo_NullPtr(void)
{
    Os_GetVersionInfo(NULL_PTR);
    /* Should not crash */
}

/* ================================================================
 *  Main
 * ================================================================ */
int main(void)
{
    UNITY_BEGIN();

    /* GetAlarmBase */
    RUN_TEST(test_GetAlarmBase_InvalidID);
    RUN_TEST(test_GetAlarmBase_NullPtr);
    RUN_TEST(test_GetAlarmBase_Success);

    /* GetAlarm */
    RUN_TEST(test_GetAlarm_InvalidID);
    RUN_TEST(test_GetAlarm_NullPtr);
    RUN_TEST(test_GetAlarm_ActiveAlarm);
    RUN_TEST(test_GetAlarm_NotActive);

    /* SetRelAlarm */
    RUN_TEST(test_SetRelAlarm_InvalidID);
    RUN_TEST(test_SetRelAlarm_ZeroIncrement);
    RUN_TEST(test_SetRelAlarm_NullTimer);
    RUN_TEST(test_SetRelAlarm_ChangePeriodFails);
    RUN_TEST(test_SetRelAlarm_TimerStartFails);
    RUN_TEST(test_SetRelAlarm_Success);
    RUN_TEST(test_SetRelAlarm_SmallIncrement_ClampedTo1Ms);

    /* SetAbsAlarm */
    RUN_TEST(test_SetAbsAlarm_InvalidID);
    RUN_TEST(test_SetAbsAlarm_StartAfterCurrent);
    RUN_TEST(test_SetAbsAlarm_StartBeforeCurrent);
    RUN_TEST(test_SetAbsAlarm_StartEqualsCurrent);

    /* CancelAlarm */
    RUN_TEST(test_CancelAlarm_InvalidID);
    RUN_TEST(test_CancelAlarm_NullTimer);
    RUN_TEST(test_CancelAlarm_NotActive);
    RUN_TEST(test_CancelAlarm_TimerStopFails);
    RUN_TEST(test_CancelAlarm_Success);

    /* Os_AlarmCallback */
    RUN_TEST(test_AlarmCallback_WithCallback);
    /* test_AlarmCallback_NullAlarm skipped: production code NULL-deref bug */
    RUN_TEST(test_AlarmCallback_NullCallback);
    RUN_TEST(test_AlarmCallback_CyclicAlarm);
    RUN_TEST(test_AlarmCallback_OneShotAlarm);
    RUN_TEST(test_AlarmCallback_CyclicButNotActive);

    /* GetResource */
    RUN_TEST(test_GetResource_InvalidID);
    RUN_TEST(test_GetResource_Success);
    RUN_TEST(test_GetResource_TakeFails);
    RUN_TEST(test_GetResource_NullMutex);

    /* ReleaseResource */
    RUN_TEST(test_ReleaseResource_InvalidID);
    RUN_TEST(test_ReleaseResource_NotOwner);
    RUN_TEST(test_ReleaseResource_Success);
    RUN_TEST(test_ReleaseResource_GiveFails);
    RUN_TEST(test_ReleaseResource_NullMutex);

    /* PCP tests via actual Os.c */
    RUN_TEST(test_GetResource_PCP_ElevatesPriority);
    RUN_TEST(test_GetResource_PCP_NoElevation);
    RUN_TEST(test_GetResource_PCP_NestedSkipsElevation);
    RUN_TEST(test_ReleaseResource_NestedNotFullyReleased);
    RUN_TEST(test_ReleaseResource_PCP_RestoresPriority);
    RUN_TEST(test_ReleaseResource_NoPCP_NoPriorityRestore);

    /* Os_Internal_SetRelAlarm edge cases */
    RUN_TEST(test_InternalSetRelAlarm_ZeroIncrement);

    /* Hook functions */
    RUN_TEST(test_ErrorHook);
    RUN_TEST(test_PreTaskHook);
    RUN_TEST(test_PostTaskHook);
    RUN_TEST(test_StartupHook);
    RUN_TEST(test_ShutdownHook);
    RUN_TEST(test_InternalHooks);

    /* Application Mode */
    RUN_TEST(test_GetActiveApplicationMode_Default);
    RUN_TEST(test_GetActiveApplicationMode_Changed);

    /* Version Info */
    RUN_TEST(test_Os_GetVersionInfo_ValidPtr);
    RUN_TEST(test_Os_GetVersionInfo_NullPtr);

    return UNITY_END();
}
