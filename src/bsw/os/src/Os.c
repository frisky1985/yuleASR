/******************************************************************************
 * @file    Os.c
 * @brief   AUTOSAR OS Module Implementation
 *
 * AUTOSAR R22-11 compliant OS implementation with FreeRTOS support.
 * This module provides standard OS services including task management,
 * resource management, event handling, and alarm management.
 *
 * @version 1.0.0
 * @date    2024
 ******************************************************************************/

/* AUTOSAR OS Includes */
#include "Os.h"
#include "Os_Cfg.h"
#include "Os_Types.h"

/* Platform Abstraction */
#include "autosar_types.h"

#ifdef FREERTOS_USED
/* FreeRTOS Headers - Using paths from CMakeLists.txt */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "timers.h"
#endif

/******************************************************************************
 * Private Definitions
 ******************************************************************************/

/* OS State Machine */
typedef enum {
    OS_STATE_UNINITIALIZED = 0,
    OS_STATE_INITIALIZED,
    OS_STATE_STARTED,
    OS_STATE_SHUTDOWN
} Os_StateType;

/* Task Control Block */
typedef struct {
    TaskType taskID;
    TaskEntryType entryPoint;
    PriorityType priority;
    uint32_t stackSize;
    EventMaskType events;
    EventMaskType waitMask;
    TaskStateType state;
#ifdef FREERTOS_USED
    TaskHandle_t handle;
    StaticTask_t staticTask;
    StackType_t* stackBuffer;
#endif
} Os_TaskControlBlockType;

/* Resource Control Block */
typedef struct {
    ResourceType resourceID;
    PriorityType priorityCeiling;
    TaskType ownerTask;
    uint8_t nestCount;
    boolean locked;
#ifdef FREERTOS_USED
    SemaphoreHandle_t mutex;
    StaticSemaphore_t staticMutex;
#endif
} Os_ResourceControlBlockType;

/* Alarm Control Block */
typedef struct {
    AlarmType alarmID;
    TickType period;
    TickType cycle;
    boolean active;
    AlarmCallbackType callback;
#ifdef FREERTOS_USED
    TimerHandle_t timer;
    StaticTimer_t staticTimer;
#endif
} Os_AlarmControlBlockType;

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static Os_StateType os_state = OS_STATE_UNINITIALIZED;
static AppModeType os_activeMode = OSDEFAULTAPPMODE;
static CoreIdType os_coreID = 0U;

/* Task Management */
static Os_TaskControlBlockType os_tasks[OS_CFG_NUM_TASKS];
static uint32_t os_taskCount = 0U;

/* Resource Management */
static Os_ResourceControlBlockType os_resources[OS_CFG_NUM_RESOURCES];
static uint32_t os_resourceCount = 0U;

/* Alarm Management */
static Os_AlarmControlBlockType os_alarms[OS_CFG_NUM_ALARMS];
static uint32_t os_alarmCount = 0U;

/* Current running task */
static TaskType os_currentTask = INVALID_TASK;

/* System Counter */
static volatile TickType os_systemCounter = 0U;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static void Os_InitTasks(void);
static void Os_InitResources(void);
static void Os_InitAlarms(void);
static void Os_IdleTask(void* param);

#ifdef FREERTOS_USED
static void Os_FreeRTOS_TaskWrapper(void* param);
static void Os_FreeRTOS_AlarmCallback(TimerHandle_t xTimer);
#endif

/******************************************************************************
 * Internal Functions
 ******************************************************************************/

/**
 * @brief Initialize all configured tasks
 */
static void Os_InitTasks(void)
{
    uint32_t i;
    
    for (i = 0U; i < OS_CFG_NUM_TASKS; i++) {
        os_tasks[i].taskID = INVALID_TASK;
        os_tasks[i].state = SUSPENDED;
        os_tasks[i].events = 0U;
        os_tasks[i].waitMask = 0U;
        os_tasks[i].handle = NULL;
        os_tasks[i].stackBuffer = NULL;
    }
    
    os_taskCount = 0U;
    
    /* Create idle task - Task ID 0 is reserved for idle */
    os_tasks[0].taskID = 0U;
    os_tasks[0].priority = 0U; /* Lowest priority */
    os_tasks[0].stackSize = configMINIMAL_STACK_SIZE;
    os_tasks[0].state = READY;
    os_tasks[0].entryPoint = NULL;
    
#ifdef FREERTOS_USED
    /* Allocate stack for idle task */
    os_tasks[0].stackBuffer = (StackType_t*)pvPortMalloc(
        os_tasks[0].stackSize * sizeof(StackType_t));
    
    if (os_tasks[0].stackBuffer != NULL) {
        os_tasks[0].handle = xTaskCreateStatic(
            Os_IdleTask,
            "IDLE",
            os_tasks[0].stackSize,
            NULL,
            tskIDLE_PRIORITY,
            os_tasks[0].stackBuffer,
            &os_tasks[0].staticTask);
    }
#endif
    
    os_taskCount = 1U;
}

/**
 * @brief Initialize all configured resources
 */
static void Os_InitResources(void)
{
    uint32_t i;
    
    for (i = 0U; i < OS_CFG_NUM_RESOURCES; i++) {
        os_resources[i].resourceID = INVALID_RESOURCE;
        os_resources[i].ownerTask = INVALID_TASK;
        os_resources[i].nestCount = 0U;
        os_resources[i].locked = FALSE;
        os_resources[i].mutex = NULL;
    }
    
    os_resourceCount = 0U;
    
    /* Resource 0 is reserved (RES_SCHEDULER) */
    os_resources[0].resourceID = RES_SCHEDULER;
    os_resources[0].priorityCeiling = 0xFFU; /* Highest priority */
    os_resourceCount = 1U;
    
#ifdef FREERTOS_USED
    /* Create mutexes for resources if enabled */
    for (i = 1U; i < OS_CFG_NUM_RESOURCES; i++) {
        os_resources[i].resourceID = i;
        os_resources[i].priorityCeiling = 0U;
        
#if (OS_CFG_FREERTOS_USE_MUTEXES == STD_ON)
        os_resources[i].mutex = xSemaphoreCreateMutexStatic(
            &os_resources[i].staticMutex);
#endif
    }
    os_resourceCount = OS_CFG_NUM_RESOURCES;
#endif
}

/**
 * @brief Initialize all configured alarms
 */
static void Os_InitAlarms(void)
{
    uint32_t i;
    
    for (i = 0U; i < OS_CFG_NUM_ALARMS; i++) {
        os_alarms[i].alarmID = INVALID_ALARM;
        os_alarms[i].active = FALSE;
        os_alarms[i].callback = NULL;
        os_alarms[i].timer = NULL;
    }
    
    os_alarmCount = 0U;
}

/**
 * @brief Idle task function
 */
static void Os_IdleTask(void* param)
{
    (void)param;
    
    for (;;) {
        /* Idle task - wait for interrupts */
#ifdef FREERTOS_USED
        /* Yield to allow other tasks to run */
        taskYIELD();
#else
        /* In bare-metal, just loop */
        __asm volatile ("nop");
#endif
    }
}

#ifdef FREERTOS_USED
/**
 * @brief FreeRTOS task wrapper for AUTOSAR tasks
 */
static void Os_FreeRTOS_TaskWrapper(void* param)
{
    Os_TaskControlBlockType* tcb = (Os_TaskControlBlockType*)param;
    
    if (tcb != NULL && tcb->entryPoint != NULL) {
        /* Call the actual task function */
        tcb->entryPoint();
    }
    
    /* Task should not return - terminate if it does */
    (void)TerminateTask();
}

/**
 * @brief FreeRTOS timer callback for alarms
 */
static void Os_FreeRTOS_AlarmCallback(TimerHandle_t xTimer)
{
    uint32_t i;
    
    for (i = 0U; i < OS_CFG_NUM_ALARMS; i++) {
        if (os_alarms[i].timer == xTimer) {
            if (os_alarms[i].callback != NULL) {
                os_alarms[i].callback();
            }
            break;
        }
    }
}
#endif /* FREERTOS_USED */

/******************************************************************************
 * Public API - Task Management
 ******************************************************************************/

/**
 * @brief Activate a task
 */
StatusType ActivateTask(TaskType TaskID)
{
    StatusType status = E_OS_OK;
    
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    if (TaskID >= OS_CFG_NUM_TASKS) {
        return E_OS_ID;
    }
    
    if (os_tasks[TaskID].taskID == INVALID_TASK) {
        return E_OS_ID;
    }
    
#ifdef FREERTOS_USED
    /* Resume the task if suspended */
    if (os_tasks[TaskID].handle != NULL) {
        if (os_tasks[TaskID].state == SUSPENDED) {
            vTaskResume(os_tasks[TaskID].handle);
            os_tasks[TaskID].state = READY;
        }
    }
#else
    /* Bare-metal implementation */
    os_tasks[TaskID].state = READY;
#endif
    
    return status;
}

/**
 * @brief Terminate the calling task
 */
StatusType TerminateTask(void)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    /* Get current task ID */
    TaskType currentTask;
    if (GetTaskID(&currentTask) != E_OS_OK) {
        return E_OS_STATE;
    }
    
    if (currentTask >= OS_CFG_NUM_TASKS) {
        return E_OS_ID;
    }
    
    /* Mark task as suspended */
    os_tasks[currentTask].state = SUSPENDED;
    
#ifdef FREERTOS_USED
    /* Delete the FreeRTOS task */
    if (os_tasks[currentTask].handle != NULL) {
        vTaskDelete(os_tasks[currentTask].handle);
        os_tasks[currentTask].handle = NULL;
    }
#else
    /* In bare-metal, this is a fatal error as we can't delete */
    return E_OS_STATE;
#endif
    
    /* Should not reach here in FreeRTOS - task is deleted */
    return E_OS_OK;
}

/**
 * @brief Chain execution to another task
 */
StatusType ChainTask(TaskType TaskID)
{
    StatusType status;
    
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    /* Activate the new task first */
    status = ActivateTask(TaskID);
    if (status != E_OS_OK) {
        return status;
    }
    
    /* Then terminate current task */
    return TerminateTask();
}

/**
 * @brief Schedule the next ready task
 */
StatusType Schedule(void)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
#ifdef FREERTOS_USED
    /* Yield to allow scheduler to run */
    taskYIELD();
#else
    /* Bare-metal: manual task switch would go here */
#endif
    
    return E_OS_OK;
}

/**
 * @brief Get the ID of the current task
 */
StatusType GetTaskID(TaskRefType TaskID)
{
    if (TaskID == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
#ifdef FREERTOS_USED
    TaskHandle_t currentHandle = xTaskGetCurrentTaskHandle();
    
    /* Find task with matching handle */
    for (uint32_t i = 0U; i < OS_CFG_NUM_TASKS; i++) {
        if (os_tasks[i].handle == currentHandle) {
            *TaskID = i;
            return E_OS_OK;
        }
    }
    
    *TaskID = INVALID_TASK;
    return E_OS_NOFUNC;
#else
    *TaskID = os_currentTask;
    return E_OS_OK;
#endif
}

/**
 * @brief Get the state of a task
 */
StatusType GetTaskState(TaskType TaskID, TaskStateRefType State)
{
    if (State == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
    if (TaskID >= OS_CFG_NUM_TASKS) {
        return E_OS_ID;
    }
    
    if (os_tasks[TaskID].taskID == INVALID_TASK) {
        return E_OS_ID;
    }
    
    *State = os_tasks[TaskID].state;
    
    return E_OS_OK;
}

/******************************************************************************
 * Public API - ISR Management
 ******************************************************************************/

void EnableAllInterrupts(void)
{
#ifdef FREERTOS_USED
    taskENABLE_INTERRUPTS();
#else
    /* Bare-metal: enable interrupts at CPU level */
    __asm volatile ("cpsie i" ::: "memory");
#endif
}

void DisableAllInterrupts(void)
{
#ifdef FREERTOS_USED
    taskDISABLE_INTERRUPTS();
#else
    /* Bare-metal: disable interrupts at CPU level */
    __asm volatile ("cpsid i" ::: "memory");
#endif
}

void ResumeOSInterrupts(void)
{
#ifdef FREERTOS_USED
    /* Set base priority to allow OS-level interrupts */
    taskEXIT_CRITICAL();
#else
    /* Platform specific */
#endif
}

void SuspendOSInterrupts(void)
{
#ifdef FREERTOS_USED
    /* Set base priority to block OS-level interrupts */
    taskENTER_CRITICAL();
#else
    /* Platform specific */
#endif
}

void ResumeAllInterrupts(void)
{
#ifdef FREERTOS_USED
    taskEXIT_CRITICAL();
#else
    EnableAllInterrupts();
#endif
}

void SuspendAllInterrupts(void)
{
#ifdef FREERTOS_USED
    taskENTER_CRITICAL();
#else
    DisableAllInterrupts();
#endif
}

/******************************************************************************
 * Public API - Resource Management
 ******************************************************************************/

StatusType GetResource(ResourceType ResID)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    if (ResID >= OS_CFG_NUM_RESOURCES) {
        return E_OS_ID;
    }
    
    if (os_resources[ResID].resourceID == INVALID_RESOURCE) {
        return E_OS_ID;
    }
    
    /* Check if already locked by another task */
    if (os_resources[ResID].locked && 
        os_resources[ResID].ownerTask != os_currentTask) {
        return E_OS_ACCESS;
    }
    
#ifdef FREERTOS_USED
    /* Take mutex if using FreeRTOS */
    if (os_resources[ResID].mutex != NULL) {
        if (xSemaphoreTake(os_resources[ResID].mutex, portMAX_DELAY) != pdTRUE) {
            return E_OS_ACCESS;
        }
    }
#endif
    
    /* Mark resource as locked */
    os_resources[ResID].locked = TRUE;
    os_resources[ResID].ownerTask = os_currentTask;
    os_resources[ResID].nestCount++;
    
    return E_OS_OK;
}

StatusType ReleaseResource(ResourceType ResID)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    if (ResID >= OS_CFG_NUM_RESOURCES) {
        return E_OS_ID;
    }
    
    if (!os_resources[ResID].locked) {
        return E_OS_NOFUNC;
    }
    
    if (os_resources[ResID].ownerTask != os_currentTask) {
        return E_OS_ACCESS;
    }
    
    /* Decrement nesting count */
    if (os_resources[ResID].nestCount > 0U) {
        os_resources[ResID].nestCount--;
    }
    
    /* Unlock if nesting count is 0 */
    if (os_resources[ResID].nestCount == 0U) {
        os_resources[ResID].locked = FALSE;
        os_resources[ResID].ownerTask = INVALID_TASK;
        
#ifdef FREERTOS_USED
        /* Release mutex */
        if (os_resources[ResID].mutex != NULL) {
            xSemaphoreGive(os_resources[ResID].mutex);
        }
#endif
    }
    
    return E_OS_OK;
}

/******************************************************************************
 * Public API - Event Management
 ******************************************************************************/

StatusType SetEvent(TaskType TaskID, EventMaskType Mask)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    if (TaskID >= OS_CFG_NUM_TASKS) {
        return E_OS_ID;
    }
    
    if (os_tasks[TaskID].taskID == INVALID_TASK) {
        return E_OS_ID;
    }
    
    /* Set the events */
    os_tasks[TaskID].events |= Mask;
    
    /* Check if task is waiting for these events */
    if ((os_tasks[TaskID].waitMask & Mask) != 0U) {
        os_tasks[TaskID].waitMask = 0U;
        os_tasks[TaskID].state = READY;
        
#ifdef FREERTOS_USED
        /* Resume the task */
        if (os_tasks[TaskID].handle != NULL) {
            vTaskResume(os_tasks[TaskID].handle);
        }
#endif
    }
    
    return E_OS_OK;
}

StatusType ClearEvent(EventMaskType Mask)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    TaskType currentTask;
    if (GetTaskID(&currentTask) != E_OS_OK) {
        return E_OS_STATE;
    }
    
    os_tasks[currentTask].events &= ~Mask;
    
    return E_OS_OK;
}

StatusType GetEvent(TaskType TaskID, EventMaskRefType Event)
{
    if (Event == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
    if (TaskID >= OS_CFG_NUM_TASKS) {
        return E_OS_ID;
    }
    
    if (os_tasks[TaskID].taskID == INVALID_TASK) {
        return E_OS_ID;
    }
    
    *Event = os_tasks[TaskID].events;
    
    return E_OS_OK;
}

StatusType WaitEvent(EventMaskType Mask)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    TaskType currentTask;
    if (GetTaskID(&currentTask) != E_OS_OK) {
        return E_OS_STATE;
    }
    
    /* Check if events are already set */
    if ((os_tasks[currentTask].events & Mask) != 0U) {
        return E_OS_OK;
    }
    
    /* Set wait mask and change state */
    os_tasks[currentTask].waitMask = Mask;
    os_tasks[currentTask].state = WAITING;
    
#ifdef FREERTOS_USED
    /* Suspend the task until events are set */
    vTaskSuspend(os_tasks[currentTask].handle);
#else
    /* Bare-metal: busy wait (not recommended) */
    while ((os_tasks[currentTask].events & Mask) == 0U) {
        /* Wait for events */
    }
#endif
    
    return E_OS_OK;
}

/******************************************************************************
 * Public API - Alarm Management
 ******************************************************************************/

StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info)
{
    if (Info == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
    if (AlarmID >= OS_CFG_NUM_ALARMS) {
        return E_OS_ID;
    }
    
    Info->maxallowedvalue = OS_CFG_MAX_ALLOWED_VALUE;
    Info->ticksperbase = OS_CFG_TICKS_PER_BASE;
    Info->mincycle = OS_CFG_MIN_CYCLE;
    
    return E_OS_OK;
}

StatusType GetAlarm(AlarmType AlarmID, TickRefType Tick)
{
    if (Tick == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
    if (AlarmID >= OS_CFG_NUM_ALARMS) {
        return E_OS_ID;
    }
    
    if (os_alarms[AlarmID].alarmID == INVALID_ALARM) {
        return E_OS_NOFUNC;
    }
    
#ifdef FREERTOS_USED
    if (os_alarms[AlarmID].timer != NULL) {
        *Tick = (TickType)xTimerGetPeriod(os_alarms[AlarmID].timer);
    } else {
        *Tick = 0U;
    }
#else
    *Tick = os_alarms[AlarmID].period;
#endif
    
    return E_OS_OK;
}

StatusType SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    if (AlarmID >= OS_CFG_NUM_ALARMS) {
        return E_OS_ID;
    }
    
    if (os_alarms[AlarmID].alarmID == INVALID_ALARM) {
        return E_OS_ID;
    }
    
    if (os_alarms[AlarmID].active) {
        return E_OS_STATE;
    }
    
    os_alarms[AlarmID].period = increment;
    os_alarms[AlarmID].cycle = cycle;
    os_alarms[AlarmID].active = TRUE;
    
#ifdef FREERTOS_USED
    if (os_alarms[AlarmID].timer != NULL) {
        /* Set timer period and start */
        xTimerChangePeriod(os_alarms[AlarmID].timer, 
                           pdMS_TO_TICKS(increment), 
                           portMAX_DELAY);
        xTimerStart(os_alarms[AlarmID].timer, portMAX_DELAY);
    }
#endif
    
    return E_OS_OK;
}

StatusType SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle)
{
    /* For FreeRTOS, absolute alarms are treated as relative from now */
    (void)start;
    return SetRelAlarm(AlarmID, start - os_systemCounter, cycle);
}

StatusType CancelAlarm(AlarmType AlarmID)
{
    if (os_state != OS_STATE_STARTED) {
        return E_OS_CALLEVEL;
    }
    
    if (AlarmID >= OS_CFG_NUM_ALARMS) {
        return E_OS_ID;
    }
    
    if (os_alarms[AlarmID].alarmID == INVALID_ALARM) {
        return E_OS_ID;
    }
    
    os_alarms[AlarmID].active = FALSE;
    
#ifdef FREERTOS_USED
    if (os_alarms[AlarmID].timer != NULL) {
        xTimerStop(os_alarms[AlarmID].timer, portMAX_DELAY);
    }
#endif
    
    return E_OS_OK;
}

/******************************************************************************
 * Public API - OS Control
 ******************************************************************************/

AppModeType GetActiveApplicationMode(void)
{
    return os_activeMode;
}

void StartOS(AppModeType Mode)
{
    /* Initialize OS port layer first */
    if (Os_Port_Init() != E_OK) {
        /* Port init failed - fatal error */
        ShutdownOS(E_OS_SYS_FATAL);
        return;
    }
    
    os_activeMode = Mode;
    os_state = OS_STATE_INITIALIZED;
    
    /* Initialize OS components */
    Os_InitTasks();
    Os_InitResources();
    Os_InitAlarms();
    
    /* Set core ID */
    os_coreID = GetCoreID();
    
    /* Call startup hook if enabled */
#if (OS_CFG_USE_STARTUP_HOOK == STD_ON)
    StartupHook();
#endif
    
    os_state = OS_STATE_STARTED;
    
    /* Start the OS scheduler (never returns on success) */
    if (Os_Port_StartOS() != E_OK) {
        /* Scheduler failed to start */
        ShutdownOS(E_OS_SYS_FATAL);
    }
}

void ShutdownOS(StatusType Error)
{
    os_state = OS_STATE_SHUTDOWN;
    
    /* Call shutdown hook if enabled */
#if (OS_CFG_USE_SHUTDOWN_HOOK == STD_ON)
    ShutdownHook(Error);
#endif
    
    /* Disable interrupts */
    DisableAllInterrupts();
    
    /* Halt the system */
    for (;;) {
        /* Wait for watchdog or reset */
        __asm volatile ("nop");
    }
}

/******************************************************************************
 * Public API - Schedule Tables
 ******************************************************************************/

StatusType StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset)
{
    (void)ScheduleTableID;
    (void)Offset;
    /* TODO: Implement schedule table support */
    return E_OS_NOFUNC;
}

StatusType StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start)
{
    (void)ScheduleTableID;
    (void)Start;
    /* TODO: Implement schedule table support */
    return E_OS_NOFUNC;
}

StatusType StopScheduleTable(ScheduleTableType ScheduleTableID)
{
    (void)ScheduleTableID;
    /* TODO: Implement schedule table support */
    return E_OS_NOFUNC;
}

StatusType GetScheduleTableStatus(ScheduleTableType ScheduleTableID, 
                                   ScheduleTableStatusRefType ScheduleStatus)
{
    (void)ScheduleTableID;
    if (ScheduleStatus != NULL) {
        *ScheduleStatus = SCHEDULETABLE_STOPPED;
    }
    return E_OS_NOFUNC;
}

/******************************************************************************
 * Public API - Counter
 ******************************************************************************/

StatusType IncrementCounter(CounterType CounterID)
{
    (void)CounterID;
    if (CounterID == OS_COUNTER_ID_SYSTEM) {
        os_systemCounter++;
        return E_OS_OK;
    }
    return E_OS_ID;
}

StatusType GetCounterValue(CounterType CounterID, TickRefType Value)
{
    if (Value == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
    if (CounterID == OS_COUNTER_ID_SYSTEM) {
#ifdef FREERTOS_USED
        *Value = (TickType)xTaskGetTickCount();
#else
        *Value = os_systemCounter;
#endif
        return E_OS_OK;
    }
    
    return E_OS_ID;
}

StatusType GetElapsedValue(CounterType CounterID, TickRefType Value)
{
    if (Value == NULL) {
        return E_OS_PARAM_POINTER;
    }
    
    if (CounterID == OS_COUNTER_ID_SYSTEM) {
        TickType current;
#ifdef FREERTOS_USED
        current = (TickType)xTaskGetTickCount();
#else
        current = os_systemCounter;
#endif
        TickType elapsed = current - *Value;
        *Value = current;
        return E_OS_OK;
    }
    
    return E_OS_ID;
}

/******************************************************************************
 * Public API - Multi-core
 ******************************************************************************/

CoreIdType GetCoreID(void)
{
#ifdef FREERTOS_USED
    /* Use port layer to get core ID */
    return (CoreIdType)Os_Port_GetCoreID();
#else
    /* Single core - always return 0 */
    return 0U;
#endif
}

CoreNumType GetNumberOfActivatedCores(void)
{
    return OS_CFG_NUM_CORES;
}

void StartCore(CoreIdType CoreID, StatusType* Status)
{
    (void)CoreID;
    if (Status != NULL) {
        *Status = E_OS_CORE;
    }
}

void StartNonAutosarCore(CoreIdType CoreID, StatusType* Status)
{
    (void)CoreID;
    if (Status != NULL) {
        *Status = E_OS_CORE;
    }
}

/******************************************************************************
 * Hook Functions - Weak implementations
 ******************************************************************************/

#if (OS_CFG_USE_ERROR_HOOK == STD_ON)
__attribute__((weak)) void ErrorHook(StatusType Error)
{
    (void)Error;
    /* Application should override this */
}
#endif

#if (OS_CFG_USE_PRETASK_HOOK == STD_ON)
__attribute__((weak)) void PreTaskHook(void)
{
    /* Application should override this */
}
#endif

#if (OS_CFG_USE_POSTTASK_HOOK == STD_ON)
__attribute__((weak)) void PostTaskHook(void)
{
    /* Application should override this */
}
#endif

#if (OS_CFG_USE_STARTUP_HOOK == STD_ON)
__attribute__((weak)) void StartupHook(void)
{
    /* Application should override this */
}
#endif

#if (OS_CFG_USE_SHUTDOWN_HOOK == STD_ON)
__attribute__((weak)) void ShutdownHook(StatusType Error)
{
    (void)Error;
    /* Application should override this */
}
#endif

#if (OS_CFG_USE_PROTECTION_HOOK == STD_ON)
__attribute__((weak)) ProtectionReturnType ProtectionHook(StatusType FatalError)
{
    (void)FatalError;
    return E_OS_PROTECTION_FAULT;
}
#endif
