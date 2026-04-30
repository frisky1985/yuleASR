/******************************************************************************
 * @file Os.c
 * @brief AutoSAR OS to FreeRTOS Adaptation Layer Implementation
 * @details This file implements the AutoSAR OS standard API using FreeRTOS
 *          primitives. It provides a mapping layer between AutoSAR OS
 *          concepts and FreeRTOS functionality.
 *
 *          Mapping Summary:
 *          - AutoSAR Task      -> FreeRTOS Task (xTaskCreate/vTaskResume/vTaskDelete)
 *          - AutoSAR Alarm     -> FreeRTOS Timer (xTimerCreate/xTimerStart/xTimerStop)
 *          - AutoSAR Event     -> FreeRTOS EventGroup (xEventGroupCreate/xEventGroupSetBits)
 *          - AutoSAR Resource  -> FreeRTOS Mutex (xSemaphoreCreateMutex/xSemaphoreTake)
 *          - AutoSAR Scheduler -> FreeRTOS Scheduler (vTaskStartScheduler)
 *
 * @author YuleTech
 * @version 1.0.0
 * @date 2026-04-30
 *
 * @copyright Copyright (c) 2026 YuleTech
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "Os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"

/*******************************************************************************
 * Compiler Switches
 ******************************************************************************/
#if (OS_STATUS_LEVEL == OS_STATUS_EXTENDED)
    #define OS_EXTENDED_ERROR_CHECK  (1)
#else
    #define OS_EXTENDED_ERROR_CHECK  (0)
#endif

/*******************************************************************************
 * Version Info
 ******************************************************************************/
#define OS_C_SW_MAJOR_VERSION   (1U)
#define OS_C_SW_MINOR_VERSION   (0U)
#define OS_C_SW_PATCH_VERSION   (0U)

/*******************************************************************************
 * Internal Data Structures
 ******************************************************************************/

/**
 * @brief Internal Task Control Block
 * @details Maps AutoSAR TaskType to FreeRTOS TaskHandle_t
 *          and stores additional AutoSAR specific task information
 */
typedef struct
{
    TaskHandle_t        xTaskHandle;        /* FreeRTOS task handle */
    TaskType            taskId;             /* AutoSAR task ID */
    EventGroupHandle_t  xEventGroup;        /* FreeRTOS event group for this task */
    boolean             isActivated;        /* Task has been activated */
    boolean             isSuspended;        /* Task is suspended (AutoSAR state) */
    uint8               activationCount;    /* Current activation count */
    uint8               maxActivations;     /* Maximum allowed activations */
    void                (*entryPoint)(void); /* Task entry function */
    uint32              stackSize;          /* Stack size in bytes */
    uint8               priority;           /* Task priority */
    const char          *name;              /* Task name for debugging */
} Os_TaskControlBlockType;

/**
 * @brief Internal Alarm Control Block
 * @details Maps AutoSAR AlarmType to FreeRTOS Timer
 *          and stores alarm callback information
 */
typedef struct
{
    TimerHandle_t       xTimerHandle;       /* FreeRTOS timer handle */
    AlarmType           alarmId;            /* AutoSAR alarm ID */
    boolean             isActive;           /* Alarm is currently active */
    TickType            increment;          /* Relative start offset */
    TickType            cycle;              /* Cycle period (0 = one-shot) */
    void                (*callback)(void);  /* Alarm callback function */
    TaskType            taskId;             /* Associated task for event setting */
    EventMaskType       eventMask;          /* Events to set when alarm expires */
    boolean             isRelative;         /* TRUE = relative, FALSE = absolute */
} Os_AlarmControlBlockType;

/**
 * @brief Internal Resource Control Block
 * @details Maps AutoSAR ResourceType to FreeRTOS Mutex
 *          and implements priority ceiling protocol
 */
typedef struct
{
    SemaphoreHandle_t   xMutexHandle;       /* FreeRTOS mutex handle */
    ResourceType        resourceId;         /* AutoSAR resource ID */
    boolean             isOccupied;         /* Resource is currently held */
    TaskType            ownerTask;          /* Task currently holding resource */
    uint8               priorityCeiling;    /* Priority ceiling for this resource */
    uint8               savedPriority;      /* Saved priority for restoration */
} Os_ResourceControlBlockType;

/**
 * @brief Internal Schedule Table Control Block
 * @details Implements AutoSAR Schedule Tables using FreeRTOS timers
 */
typedef struct
{
    TimerHandle_t       xTimerHandle;       /* FreeRTOS timer handle */
    ScheduleTableType   scheduleTableId;    /* AutoSAR schedule table ID */
    ScheduleTableStatusType status;         /* Current status */
    TickType            offset;             /* Start offset */
    boolean             isPeriodic;         /* Periodic schedule table */
    void                (*callback)(void);  /* Expiry point callback */
} Os_ScheduleTableControlBlockType;

/**
 * @brief Internal Counter Control Block
 * @details Implements AutoSAR Counters
 */
typedef struct
{
    CounterType         counterId;          /* AutoSAR counter ID */
    TickType            currentValue;       /* Current counter value */
    TickType            maxValue;           /* Maximum allowed value */
    TickType            ticksPerBase;       /* Ticks per base */
    TickType            minCycle;           /* Minimum cycle */
    boolean             isSoftwareCounter;  /* TRUE = software counter */
} Os_CounterControlBlockType;

/**
 * @brief OS Global State
 * @details Tracks OS initialization and runtime state
 */
typedef struct
{
    boolean             isInitialized;      /* OS initialized */
    boolean             isRunning;          /* Scheduler running */
    AppModeType         activeAppMode;      /* Current application mode */
    TaskType            runningTask;        /* Currently running task */
    uint32              interruptNesting;   /* Interrupt nesting level */
    uint32              resourceNesting;    /* Resource nesting level */
    CoreIdType          coreId;             /* Current core ID */
} Os_GlobalStateType;

/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/* OS Global State */
static Os_GlobalStateType Os_GlobalState;

/* Task Control Blocks */
static Os_TaskControlBlockType Os_TaskTable[OS_TASK_COUNT];

/* Alarm Control Blocks */
static Os_AlarmControlBlockType Os_AlarmTable[OS_ALARM_COUNT];

/* Resource Control Blocks */
static Os_ResourceControlBlockType Os_ResourceTable[OS_RESOURCE_COUNT];

/* Schedule Table Control Blocks */
static Os_ScheduleTableControlBlockType Os_ScheduleTableTable[OS_SCHEDULE_TABLE_COUNT];

/* Counter Control Blocks */
static Os_CounterControlBlockType Os_CounterTable[OS_COUNTER_COUNT];

/* Resource Nesting Stack (for nested resource access) */
static ResourceType Os_ResourceStack[OS_RESOURCE_MAX_NESTING];
static uint8 Os_ResourceStackTop = 0U;

/* Interrupt Nesting Counters */
static uint32 Os_SuspendAllCount = 0U;
static uint32 Os_SuspendOSCount = 0U;

/*******************************************************************************
 * Static Function Prototypes
 ******************************************************************************/
static void Os_TaskWrapper(void *pvParameters);
static void Os_AlarmCallback(TimerHandle_t xTimer);
static void Os_ScheduleTableCallback(TimerHandle_t xTimer);
static Os_TaskControlBlockType* Os_GetTaskControlBlock(TaskType taskId);
static Os_AlarmControlBlockType* Os_GetAlarmControlBlock(AlarmType alarmId);
static Os_ResourceControlBlockType* Os_GetResourceControlBlock(ResourceType resId);
static Os_ScheduleTableControlBlockType* Os_GetScheduleTableControlBlock(ScheduleTableType stId);
static void Os_SetError(StatusType error, uint8 serviceId);
static void Os_CallErrorHook(StatusType error);

/*******************************************************************************
 * FreeRTOS Callback Functions
 ******************************************************************************/

/**
 * @brief Task wrapper function
 * @details Wraps the user task entry point to handle AutoSAR specific behavior
 *          including task termination and hook function calls
 */
static void Os_TaskWrapper(void *pvParameters)
{
    Os_TaskControlBlockType *pTcb;
    TaskType taskId;

    /* Get task ID from parameters */
    taskId = (TaskType)((uintptr_t)pvParameters);
    pTcb = Os_GetTaskControlBlock(taskId);

    if (pTcb == NULL)
    {
        /* Invalid task ID - should never happen */
        Os_CallErrorHook(E_OS_ID);
        vTaskDelete(NULL);
        return;
    }

    /* Mark task as activated */
    pTcb->isActivated = TRUE;
    pTcb->isSuspended = FALSE;
    pTcb->activationCount++;
    Os_GlobalState.runningTask = taskId;

    /* Call Pre-Task Hook if enabled */
    #if (OS_USE_PRETASK_HOOK == STD_ON)
    Os_PreTaskHook();
    #endif

    /* Call the actual task entry point */
    if (pTcb->entryPoint != NULL)
    {
        pTcb->entryPoint();
    }

    /* Task entry point returned without TerminateTask */
    /* Call Post-Task Hook if enabled */
    #if (OS_USE_POSTTASK_HOOK == STD_ON)
    Os_PostTaskHook();
    #endif

    /* In AutoSAR, tasks should not return - they must call TerminateTask() */
    /* If they return anyway, we clean up */
    pTcb->isActivated = FALSE;
    pTcb->activationCount--;
    Os_GlobalState.runningTask = (TaskType)0;

    /* Delete the FreeRTOS task */
    vTaskDelete(NULL);

    /* Should never reach here */
    for (;;)
    {
        /* Infinite loop for safety */
    }
}

/**
 * @brief Alarm callback function
 * @details Called when a FreeRTOS timer (mapped to AutoSAR alarm) expires
 */
static void Os_AlarmCallback(TimerHandle_t xTimer)
{
    Os_AlarmControlBlockType *pAlarm;
    uint32 i;

    /* Find the alarm control block */
    for (i = 0U; i < OS_ALARM_COUNT; i++)
    {
        if (Os_AlarmTable[i].xTimerHandle == xTimer)
        {
            pAlarm = &Os_AlarmTable[i];

            /* Call alarm callback if configured */
            if (pAlarm->callback != NULL)
            {
                pAlarm->callback();
            }

            /* Set events if configured */
            if (pAlarm->eventMask != 0U && pAlarm->taskId < OS_TASK_COUNT)
            {
                (void)Os_SetEvent(pAlarm->taskId, pAlarm->eventMask);
            }

            /* Handle one-shot alarms - mark as inactive */
            if (pAlarm->cycle == 0U)
            {
                pAlarm->isActive = FALSE;
            }

            break;
        }
    }
}

/**
 * @brief Schedule table callback function
 * @details Called when a schedule table expiry point is reached
 */
static void Os_ScheduleTableCallback(TimerHandle_t xTimer)
{
    Os_ScheduleTableControlBlockType *pSt;
    uint32 i;

    /* Find the schedule table control block */
    for (i = 0U; i < OS_SCHEDULE_TABLE_COUNT; i++)
    {
        if (Os_ScheduleTableTable[i].xTimerHandle == xTimer)
        {
            pSt = &Os_ScheduleTableTable[i];

            /* Call expiry point callback if configured */
            if (pSt->callback != NULL)
            {
                pSt->callback();
            }

            break;
        }
    }
}

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

/**
 * @brief Get task control block by task ID
 * @param taskId AutoSAR task ID
 * @return Pointer to task control block or NULL if invalid
 */
static Os_TaskControlBlockType* Os_GetTaskControlBlock(TaskType taskId)
{
    if (taskId >= OS_TASK_COUNT)
    {
        return NULL;
    }
    return &Os_TaskTable[taskId];
}

/**
 * @brief Get alarm control block by alarm ID
 * @param alarmId AutoSAR alarm ID
 * @return Pointer to alarm control block or NULL if invalid
 */
static Os_AlarmControlBlockType* Os_GetAlarmControlBlock(AlarmType alarmId)
{
    if (alarmId >= OS_ALARM_COUNT)
    {
        return NULL;
    }
    return &Os_AlarmTable[alarmId];
}

/**
 * @brief Get resource control block by resource ID
 * @param resId AutoSAR resource ID
 * @return Pointer to resource control block or NULL if invalid
 */
static Os_ResourceControlBlockType* Os_GetResourceControlBlock(ResourceType resId)
{
    if (resId >= OS_RESOURCE_COUNT && resId != RES_SCHEDULER)
    {
        return NULL;
    }
    return &Os_ResourceTable[resId];
}

/**
 * @brief Get schedule table control block by ID
 * @param stId AutoSAR schedule table ID
 * @return Pointer to schedule table control block or NULL if invalid
 */
static Os_ScheduleTableControlBlockType* Os_GetScheduleTableControlBlock(ScheduleTableType stId)
{
    if (stId >= OS_SCHEDULE_TABLE_COUNT)
    {
        return NULL;
    }
    return &Os_ScheduleTableTable[stId];
}

/**
 * @brief Set error information for error hook
 * @param error Error code
 * @param serviceId Service ID where error occurred
 */
static void Os_SetError(StatusType error, uint8 serviceId)
{
    /* Store error information for debugging/analysis */
    (void)error;
    (void)serviceId;

    /* In extended status mode, we could store this in a global error log */
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    /* Error logging could be implemented here */
    #endif
}

/**
 * @brief Call error hook function
 * @param error Error code to pass to hook
 */
static void Os_CallErrorHook(StatusType error)
{
    #if (OS_USE_ERROR_HOOK == STD_ON)
    Os_ErrorHook(error);
    #else
    (void)error;
    #endif
}

/*******************************************************************************
 * Hook Functions (Weak Defaults - Can be overridden by user)
 ******************************************************************************/

/* Error Hook */
#if (OS_USE_ERROR_HOOK == STD_ON)
__attribute__((weak)) void Os_ErrorHook(StatusType error)
{
    (void)error;
    /* Default: do nothing. User can override this function. */
}
#endif

/* Pre-Task Hook */
#if (OS_USE_PRETASK_HOOK == STD_ON)
__attribute__((weak)) void Os_PreTaskHook(void)
{
    /* Default: do nothing. User can override this function. */
}
#endif

/* Post-Task Hook */
#if (OS_USE_POSTTASK_HOOK == STD_ON)
__attribute__((weak)) void Os_PostTaskHook(void)
{
    /* Default: do nothing. User can override this function. */
}
#endif

/* Startup Hook */
#if (OS_USE_STARTUP_HOOK == STD_ON)
__attribute__((weak)) void Os_StartupHook(void)
{
    /* Default: do nothing. User can override this function. */
}
#endif

/* Shutdown Hook */
#if (OS_USE_SHUTDOWN_HOOK == STD_ON)
__attribute__((weak)) void Os_ShutdownHook(StatusType error)
{
    (void)error;
    /* Default: do nothing. User can override this function. */
}
#endif

/* Protection Hook */
#if (OS_USE_PROTECTION_HOOK == STD_ON)
__attribute__((weak)) ProtectionReturnType Os_ProtectionHook(StatusType fatalError)
{
    (void)fatalError;
    /* Default: shutdown on protection error */
    return PRO_SHUTDOWN;
}
#endif

/* Idle Hook */
#if (OS_USE_IDLE_HOOK == STD_ON)
__attribute__((weak)) void Os_IdleHook(void)
{
    /* Default: do nothing. User can override this function. */
    /* Called repeatedly by the idle task */
}
#endif

/*******************************************************************************
 * Task Management Implementation
 ******************************************************************************/

/**
 * @brief Create a task
 * @details Maps AutoSAR Os_CreateTask to FreeRTOS xTaskCreate
 *          Creates the FreeRTOS task but does not activate it
 *
 * @param TaskID AutoSAR task identifier
 * @return StatusType E_OK if successful, error code otherwise
 */
StatusType Os_CreateTask(TaskType TaskID)
{
    Os_TaskControlBlockType *pTcb;
    BaseType_t result;

    /* Extended error checking */
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_CREATE_TASK);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Os_GlobalState.isRunning)
    {
        /* Cannot create tasks after OS is started */
        Os_SetError(E_OS_CALLEVEL, OS_SID_CREATE_TASK);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }
    #endif

    pTcb = Os_GetTaskControlBlock(TaskID);
    if (pTcb == NULL)
    {
        return E_OS_ID;
    }

    /* Check if task is already created */
    if (pTcb->xTaskHandle != NULL)
    {
        return E_OS_STATE;
    }

    /* Create event group for this task */
    pTcb->xEventGroup = xEventGroupCreate();
    if (pTcb->xEventGroup == NULL)
    {
        return E_OS_LIMIT;
    }

    /* Create the FreeRTOS task (initially suspended) */
    result = xTaskCreate(
        Os_TaskWrapper,                         /* Task function */
        pTcb->name != NULL ? pTcb->name : "OsTask", /* Task name */
        pTcb->stackSize / sizeof(StackType_t),  /* Stack size (in words) */
        (void *)((uintptr_t)TaskID),            /* Task parameter (pass TaskID) */
        pTcb->priority,                         /* Priority */
        &pTcb->xTaskHandle                      /* Task handle */
    );

    if (result != pdPASS)
    {
        /* Task creation failed */
        vEventGroupDelete(pTcb->xEventGroup);
        pTcb->xEventGroup = NULL;
        return E_OS_LIMIT;
    }

    /* Suspend the task initially (AutoSAR tasks start in suspended state) */
    vTaskSuspend(pTcb->xTaskHandle);
    pTcb->isSuspended = TRUE;
    pTcb->isActivated = FALSE;
    pTcb->activationCount = 0U;

    return E_OK;
}

/**
 * @brief Activate a task
 * @details Maps AutoSAR Os_ActivateTask to FreeRTOS vTaskResume
 *          Transitions a task from suspended to ready state
 *
 * @param TaskID AutoSAR task identifier
 * @return StatusType E_OK if successful, error code otherwise
 */
StatusType Os_ActivateTask(TaskType TaskID)
{
    Os_TaskControlBlockType *pTcb;
    TaskHandle_t currentTask;

    /* Extended error checking */
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_ACTIVATE_TASK);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (!Os_GlobalState.isRunning)
    {
        /* OS not started yet */
        Os_SetError(E_OS_CALLEVEL, OS_SID_ACTIVATE_TASK);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }
    #endif

    pTcb = Os_GetTaskControlBlock(TaskID);
    if (pTcb == NULL)
    {
        return E_OS_ID;
    }

    /* Check if task has been created */
    if (pTcb->xTaskHandle == NULL)
    {
        #if (OS_EXTENDED_ERROR_CHECK == 1)
        Os_SetError(E_OS_ID, OS_SID_ACTIVATE_TASK);
        Os_CallErrorHook(E_OS_ID);
        #endif
        return E_OS_ID;
    }

    /* Check activation limit */
    if (pTcb->activationCount >= pTcb->maxActivations)
    {
        #if (OS_EXTENDED_ERROR_CHECK == 1)
        Os_SetError(E_OS_LIMIT, OS_SID_ACTIVATE_TASK);
        Os_CallErrorHook(E_OS_LIMIT);
        #endif
        return E_OS_LIMIT;
    }

    /* Get current task to determine if we need to yield */
    currentTask = xTaskGetCurrentTaskHandle();

    /* Resume the FreeRTOS task */
    vTaskResume(pTcb->xTaskHandle);

    /* Check if we need to yield (higher priority task was activated) */
    if (pTcb->priority > uxTaskPriorityGet(currentTask))
    {
        /* Request context switch if in task context */
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            taskYIELD();
        }
    }

    return E_OK;
}

/**
 * @brief Terminate the calling task
 * @details Maps AutoSAR Os_TerminateTask to FreeRTOS vTaskDelete
 *          The calling task terminates and control goes to the next ready task
 *
 * @return StatusType E_OK if successful (typically doesn't return)
 */
StatusType Os_TerminateTask(void)
{
    TaskHandle_t currentHandle;
    Os_TaskControlBlockType *pTcb;
    uint32 i;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (!Os_GlobalState.isRunning)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_TERMINATE_TASK);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }
    #endif

    /* Get current task handle */
    currentHandle = xTaskGetCurrentTaskHandle();

    /* Find the TCB for current task */
    pTcb = NULL;
    for (i = 0U; i < OS_TASK_COUNT; i++)
    {
        if (Os_TaskTable[i].xTaskHandle == currentHandle)
        {
            pTcb = &Os_TaskTable[i];
            break;
        }
    }

    if (pTcb != NULL)
    {
        /* Call Post-Task Hook */
        #if (OS_USE_POSTTASK_HOOK == STD_ON)
        Os_PostTaskHook();
        #endif

        /* Update TCB */
        pTcb->isActivated = FALSE;
        pTcb->isSuspended = TRUE;
        if (pTcb->activationCount > 0U)
        {
            pTcb->activationCount--;
        }
        Os_GlobalState.runningTask = (TaskType)0;

        /* Delete the FreeRTOS task (this function never returns) */
        vTaskDelete(NULL);
    }

    /* Should never reach here if task was found and deleted */
    return E_OK;
}

/**
 * @brief Chain task - terminate current and activate another
 * @details Terminates the calling task and activates another task
 *          If chaining to same task, it's equivalent to restarting
 *
 * @param TaskID Task to activate after current terminates
 * @return StatusType E_OK or error code
 */
StatusType Os_ChainTask(TaskType TaskID)
{
    StatusType status;

    /* Extended error checking */
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_CHAIN_TASK);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    /* Check if resources are still held */
    if (Os_GlobalState.resourceNesting > 0U)
    {
        Os_SetError(E_OS_RESOURCE, OS_SID_CHAIN_TASK);
        Os_CallErrorHook(E_OS_RESOURCE);
        return E_OS_RESOURCE;
    }
    #endif

    /* Activate the new task first (before terminating current) */
    status = Os_ActivateTask(TaskID);
    if (status != E_OK)
    {
        return status;
    }

    /* Now terminate the current task */
    return Os_TerminateTask();
}

/**
 * @brief Get the identifier of the running task
 * @details Returns the AutoSAR task ID of the currently running task
 *
 * @param TaskRef Reference to store the task ID
 * @return StatusType E_OK or error code
 */
StatusType Os_GetTaskID(TaskRefType TaskRef)
{
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskRef == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_TASK_ID);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    *TaskRef = Os_GlobalState.runningTask;
    return E_OK;
}

/**
 * @brief Get the state of a task
 * @details Returns the current state (RUNNING, WAITING, READY, SUSPENDED)
 *          of the specified task
 *
 * @param TaskID Task to query
 * @param State Reference to store the state
 * @return StatusType E_OK or error code
 */
StatusType Os_GetTaskState(TaskType TaskID, TaskStateRefType State)
{
    Os_TaskControlBlockType *pTcb;
    TaskHandle_t currentHandle;
    eTaskState freeRtosState;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_TASK_STATE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (State == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_TASK_STATE);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    pTcb = Os_GetTaskControlBlock(TaskID);
    if (pTcb == NULL || pTcb->xTaskHandle == NULL)
    {
        *State = INVALID_TASK;
        return E_OS_ID;
    }

    /* Get FreeRTOS task state */
    freeRtosState = eTaskGetState(pTcb->xTaskHandle);
    currentHandle = xTaskGetCurrentTaskHandle();

    /* Map FreeRTOS state to AutoSAR state */
    if (pTcb->xTaskHandle == currentHandle)
    {
        *State = RUNNING;
    }
    else
    {
        switch (freeRtosState)
        {
            case eRunning:
                *State = RUNNING;
                break;
            case eReady:
                *State = READY;
                break;
            case eBlocked:
                /* Task is waiting for event or resource */
                *State = WAITING;
                break;
            case eSuspended:
                *State = SUSPENDED;
                break;
            case eDeleted:
                *State = INVALID_TASK;
                break;
            default:
                *State = INVALID_TASK;
                break;
        }
    }

    return E_OK;
}

/**
 * @brief Request a schedule point
 * @details In full preemptive scheduling, this allows rescheduling
 *          if a higher priority task is ready
 *
 * @return StatusType E_OK or error code
 */
StatusType Os_Schedule(void)
{
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (!Os_GlobalState.isRunning)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_SCHEDULE);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }

    /* Schedule can only be called from task level */
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_SCHEDULE);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }

    /* Resources must not be held when calling Schedule */
    if (Os_GlobalState.resourceNesting > 0U)
    {
        Os_SetError(E_OS_RESOURCE, OS_SID_SCHEDULE);
        Os_CallErrorHook(E_OS_RESOURCE);
        return E_OS_RESOURCE;
    }
    #endif

    /* Request context switch */
    taskYIELD();

    return E_OK;
}

/*******************************************************************************
 * Event Management Implementation
 ******************************************************************************/

/**
 * @brief Set events for a task
 * @details Uses FreeRTOS Event Groups to implement AutoSAR events
 *          Sets the specified event bits for the target task
 *
 * @param TaskID Task to set events for
 * @param Mask Event mask to set
 * @return StatusType E_OK or error code
 */
StatusType Os_SetEvent(TaskType TaskID, EventMaskType Mask)
{
    Os_TaskControlBlockType *pTcb;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_SET_EVENT);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Mask == 0U)
    {
        Os_SetError(E_OS_VALUE, OS_SID_SET_EVENT);
        Os_CallErrorHook(E_OS_VALUE);
        return E_OS_VALUE;
    }
    #endif

    pTcb = Os_GetTaskControlBlock(TaskID);
    if (pTcb == NULL)
    {
        return E_OS_ID;
    }

    if (pTcb->xEventGroup == NULL)
    {
        return E_OS_STATE;
    }

    /* Set event bits in the task's event group */
    xEventGroupSetBits(pTcb->xEventGroup, (EventBits_t)Mask);

    return E_OK;
}

/**
 * @brief Wait for events
 * @details Blocks the calling task until one or more of the specified
 *          events are set. Uses FreeRTOS Event Groups with timeout support.
 *
 * @param Mask Events to wait for
 * @return StatusType E_OK or error code
 */
StatusType Os_WaitEvent(EventMaskType Mask)
{
    TaskHandle_t currentHandle;
    Os_TaskControlBlockType *pTcb;
    uint32 i;
    EventBits_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (!Os_GlobalState.isRunning)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_WAIT_EVENT);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }

    /* Must be called from task level, not ISR */
    if (xPortIsInsideInterrupt() == pdTRUE)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_WAIT_EVENT);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }
    #endif

    /* Get current task handle */
    currentHandle = xTaskGetCurrentTaskHandle();

    /* Find the TCB for current task */
    pTcb = NULL;
    for (i = 0U; i < OS_TASK_COUNT; i++)
    {
        if (Os_TaskTable[i].xTaskHandle == currentHandle)
        {
            pTcb = &Os_TaskTable[i];
            break;
        }
    }

    if (pTcb == NULL || pTcb->xEventGroup == NULL)
    {
        return E_OS_ACCESS;
    }

    /* Wait for events (indefinite wait - no timeout in AutoSAR standard) */
    result = xEventGroupWaitBits(
        pTcb->xEventGroup,          /* Event group */
        (EventBits_t)Mask,          /* Bits to wait for */
        pdFALSE,                    /* Don't clear on exit (must use ClearEvent) */
        pdFALSE,                    /* Wait for any bit (not all) */
        portMAX_DELAY               /* Wait indefinitely */
    );

    /* We always return E_OK since we wait indefinitely */
    (void)result;
    return E_OK;
}

/**
 * @brief Clear events
 * @details Clears the specified event bits for the calling task
 *
 * @param Mask Events to clear
 * @return StatusType E_OK or error code
 */
StatusType Os_ClearEvent(EventMaskType Mask)
{
    TaskHandle_t currentHandle;
    Os_TaskControlBlockType *pTcb;
    uint32 i;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (!Os_GlobalState.isRunning)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_CLEAR_EVENT);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }

    /* Must be called from task level */
    if (xPortIsInsideInterrupt() == pdTRUE)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_CLEAR_EVENT);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }
    #endif

    /* Get current task handle */
    currentHandle = xTaskGetCurrentTaskHandle();

    /* Find the TCB for current task */
    pTcb = NULL;
    for (i = 0U; i < OS_TASK_COUNT; i++)
    {
        if (Os_TaskTable[i].xTaskHandle == currentHandle)
        {
            pTcb = &Os_TaskTable[i];
            break;
        }
    }

    if (pTcb == NULL || pTcb->xEventGroup == NULL)
    {
        return E_OS_ACCESS;
    }

    /* Clear event bits */
    xEventGroupClearBits(pTcb->xEventGroup, (EventBits_t)Mask);

    return E_OK;
}

/**
 * @brief Get events for a task
 * @details Returns the current event mask for the specified task
 *          without modifying the events
 *
 * @param TaskID Task to query
 * @param Event Reference to store the event mask
 * @return StatusType E_OK or error code
 */
StatusType Os_GetEvent(TaskType TaskID, EventMaskRefType Event)
{
    Os_TaskControlBlockType *pTcb;
    EventBits_t eventBits;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_EVENT);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Event == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_EVENT);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    pTcb = Os_GetTaskControlBlock(TaskID);
    if (pTcb == NULL)
    {
        return E_OS_ID;
    }

    if (pTcb->xEventGroup == NULL)
    {
        return E_OS_STATE;
    }

    /* Get current event bits */
    eventBits = xEventGroupGetBits(pTcb->xEventGroup);
    *Event = (EventMaskType)eventBits;

    return E_OK;
}

/*******************************************************************************
 * Alarm Management Implementation
 ******************************************************************************/

/**
 * @brief Set a relative alarm
 * @details Creates/starts a FreeRTOS timer with a relative offset
 *          Timer can be one-shot (cycle=0) or periodic
 *
 * @param AlarmID Alarm to set
 * @param increment Relative offset from now (in ticks)
 * @param cycle Cycle time (0 = one-shot)
 * @return StatusType E_OK or error code
 */
StatusType Os_SetRelAlarm(AlarmType AlarmID, TickType increment, TickType cycle)
{
    Os_AlarmControlBlockType *pAlarm;
    TickType period;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (AlarmID >= OS_ALARM_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_SET_REL_ALARM);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (increment == 0U)
    {
        Os_SetError(E_OS_VALUE, OS_SID_SET_REL_ALARM);
        Os_CallErrorHook(E_OS_VALUE);
        return E_OS_VALUE;
    }
    #endif

    pAlarm = Os_GetAlarmControlBlock(AlarmID);
    if (pAlarm == NULL)
    {
        return E_OS_ID;
    }

    /* Check if alarm is already active */
    if (pAlarm->isActive)
    {
        return E_OS_STATE;
    }

    /* Determine timer period (0 for one-shot, cycle for periodic) */
    period = (cycle == 0U) ? 0U : pdMS_TO_TICKS(cycle * OS_TICK_MS);

    /* Create timer if not already created */
    if (pAlarm->xTimerHandle == NULL)
    {
        pAlarm->xTimerHandle = xTimerCreate(
            "OsAlarm",                          /* Timer name */
            pdMS_TO_TICKS(increment * OS_TICK_MS), /* Initial delay */
            (period == 0U) ? pdFALSE : pdTRUE,  /* Auto-reload */
            (void *)pAlarm,                     /* Timer ID */
            Os_AlarmCallback                    /* Callback function */
        );

        if (pAlarm->xTimerHandle == NULL)
        {
            return E_OS_LIMIT;
        }
    }
    else
    {
        /* Change timer period */
        result = xTimerChangePeriod(pAlarm->xTimerHandle,
                                    pdMS_TO_TICKS(increment * OS_TICK_MS),
                                    portMAX_DELAY);
        if (result != pdPASS)
        {
            return E_OS_STATE;
        }
    }

    /* Start the timer */
    result = xTimerStart(pAlarm->xTimerHandle, portMAX_DELAY);
    if (result != pdPASS)
    {
        return E_OS_STATE;
    }

    /* Update alarm state */
    pAlarm->isActive = TRUE;
    pAlarm->increment = increment;
    pAlarm->cycle = cycle;
    pAlarm->isRelative = TRUE;

    return E_OK;
}

/**
 * @brief Set an absolute alarm
 * @details Similar to relative alarm but starts at an absolute counter value
 *          Note: In FreeRTOS implementation, we simulate absolute alarms
 *          by converting to relative timing based on current counter value
 *
 * @param AlarmID Alarm to set
 * @param start Absolute start value
 * @param cycle Cycle time (0 = one-shot)
 * @return StatusType E_OK or error code
 */
StatusType Os_SetAbsAlarm(AlarmType AlarmID, TickType start, TickType cycle)
{
    Os_AlarmControlBlockType *pAlarm;
    TickType currentTicks;
    TickType relativeTicks;
    TickType period;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (AlarmID >= OS_ALARM_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_SET_ABS_ALARM);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (start == 0U)
    {
        Os_SetError(E_OS_VALUE, OS_SID_SET_ABS_ALARM);
        Os_CallErrorHook(E_OS_VALUE);
        return E_OS_VALUE;
    }
    #endif

    pAlarm = Os_GetAlarmControlBlock(AlarmID);
    if (pAlarm == NULL)
    {
        return E_OS_ID;
    }

    /* Check if alarm is already active */
    if (pAlarm->isActive)
    {
        return E_OS_STATE;
    }

    /* Get current tick count */
    currentTicks = xTaskGetTickCount() / OS_TICK_MS;

    /* Calculate relative offset from absolute start */
    if (start > currentTicks)
    {
        relativeTicks = start - currentTicks;
    }
    else
    {
        /* Start time has passed - use minimal delay */
        relativeTicks = 1U;
    }

    /* Determine timer period */
    period = (cycle == 0U) ? 0U : pdMS_TO_TICKS(cycle * OS_TICK_MS);

    /* Create timer if not already created */
    if (pAlarm->xTimerHandle == NULL)
    {
        pAlarm->xTimerHandle = xTimerCreate(
            "OsAlarm",
            pdMS_TO_TICKS(relativeTicks * OS_TICK_MS),
            (period == 0U) ? pdFALSE : pdTRUE,
            (void *)pAlarm,
            Os_AlarmCallback
        );

        if (pAlarm->xTimerHandle == NULL)
        {
            return E_OS_LIMIT;
        }
    }
    else
    {
        result = xTimerChangePeriod(pAlarm->xTimerHandle,
                                    pdMS_TO_TICKS(relativeTicks * OS_TICK_MS),
                                    portMAX_DELAY);
        if (result != pdPASS)
        {
            return E_OS_STATE;
        }
    }

    /* Start the timer */
    result = xTimerStart(pAlarm->xTimerHandle, portMAX_DELAY);
    if (result != pdPASS)
    {
        return E_OS_STATE;
    }

    /* Update alarm state */
    pAlarm->isActive = TRUE;
    pAlarm->increment = relativeTicks;
    pAlarm->cycle = cycle;
    pAlarm->isRelative = FALSE;

    return E_OK;
}

/**
 * @brief Cancel an alarm
 * @details Stops the FreeRTOS timer associated with the alarm
 *
 * @param AlarmID Alarm to cancel
 * @return StatusType E_OK or error code
 */
StatusType Os_CancelAlarm(AlarmType AlarmID)
{
    Os_AlarmControlBlockType *pAlarm;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (AlarmID >= OS_ALARM_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_CANCEL_ALARM);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }
    #endif

    pAlarm = Os_GetAlarmControlBlock(AlarmID);
    if (pAlarm == NULL)
    {
        return E_OS_ID;
    }

    /* Check if alarm is active */
    if (!pAlarm->isActive)
    {
        return E_OS_NOFUNC;
    }

    /* Stop the timer */
    if (pAlarm->xTimerHandle != NULL)
    {
        result = xTimerStop(pAlarm->xTimerHandle, portMAX_DELAY);
        if (result != pdPASS)
        {
            return E_OS_STATE;
        }
    }

    pAlarm->isActive = FALSE;

    return E_OK;
}

/**
 * @brief Get remaining ticks until alarm expires
 * @details Returns the remaining time until an active alarm expires
 *
 * @param AlarmID Alarm to query
 * @param Tick Reference to store remaining ticks
 * @return StatusType E_OK or error code
 */
StatusType Os_GetAlarm(AlarmType AlarmID, TickRefType Tick)
{
    Os_AlarmControlBlockType *pAlarm;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (AlarmID >= OS_ALARM_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_ALARM);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Tick == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_ALARM);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    pAlarm = Os_GetAlarmControlBlock(AlarmID);
    if (pAlarm == NULL)
    {
        return E_OS_ID;
    }

    /* Check if alarm is active */
    if (!pAlarm->isActive)
    {
        return E_OS_NOFUNC;
    }

    /* Get remaining time from FreeRTOS timer */
    if (pAlarm->xTimerHandle != NULL)
    {
        *Tick = (TickType)(xTimerGetExpiryTime(pAlarm->xTimerHandle) - xTaskGetTickCount());
        *Tick = *Tick / OS_TICK_MS; /* Convert to OS ticks */
    }
    else
    {
        *Tick = 0U;
    }

    return E_OK;
}

/**
 * @brief Get alarm base characteristics
 * @details Returns the characteristics of the counter driving the alarm
 *
 * @param AlarmID Alarm to query
 * @param Info Reference to store alarm base info
 * @return StatusType E_OK or error code
 */
StatusType Os_GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info)
{
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (AlarmID >= OS_ALARM_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_ALARM_BASE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Info == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_ALARM_BASE);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    /* Fill in alarm base characteristics based on configuration */
    /* maxallowedvalue: maximum counter value (32-bit) */
    Info->maxallowedvalue = 0xFFFFFFFFU;

    /* ticksperbase: number of ticks per base unit */
    Info->ticksperbase = (TickType)OS_TICKS_PER_SECOND;

    /* mincycle: minimum cycle time */
    Info->mincycle = 1U;

    return E_OK;
}

/*******************************************************************************
 * Resource Management Implementation
 ******************************************************************************/

/**
 * @brief Get a resource
 * @details Uses FreeRTOS mutex to implement AutoSAR resources
 *          Implements priority ceiling protocol
 *
 * @param ResID Resource to get
 * @return StatusType E_OK or error code
 */
StatusType Os_GetResource(ResourceType ResID)
{
    Os_ResourceControlBlockType *pRes;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ResID >= OS_RESOURCE_COUNT && ResID != RES_SCHEDULER)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_RESOURCE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    /* Cannot be called from ISR */
    if (xPortIsInsideInterrupt() == pdTRUE)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_GET_RESOURCE);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }

    /* Check nesting limit */
    if (Os_ResourceStackTop >= OS_RESOURCE_MAX_NESTING)
    {
        Os_SetError(E_OS_LIMIT, OS_SID_GET_RESOURCE);
        Os_CallErrorHook(E_OS_LIMIT);
        return E_OS_LIMIT;
    }
    #endif

    /* Handle RES_SCHEDULER (scheduler resource) specially */
    if (ResID == RES_SCHEDULER)
    {
        /* Disable scheduler/preemption */
        vTaskSuspendAll();
        Os_ResourceStack[Os_ResourceStackTop] = ResID;
        Os_ResourceStackTop++;
        Os_GlobalState.resourceNesting++;
        return E_OK;
    }

    pRes = Os_GetResourceControlBlock(ResID);
    if (pRes == NULL)
    {
        return E_OS_ID;
    }

    /* Check if resource is already occupied by this task */
    if (pRes->isOccupied && pRes->ownerTask == Os_GlobalState.runningTask)
    {
        return E_OS_ACCESS;
    }

    /* Create mutex if not already created */
    if (pRes->xMutexHandle == NULL)
    {
        pRes->xMutexHandle = xSemaphoreCreateMutex();
        if (pRes->xMutexHandle == NULL)
        {
            return E_OS_LIMIT;
        }
    }

    /* Take the mutex (blocking) */
    result = xSemaphoreTake(pRes->xMutexHandle, portMAX_DELAY);
    if (result != pdPASS)
    {
        return E_OS_ACCESS;
    }

    /* Update resource state */
    pRes->isOccupied = TRUE;
    pRes->ownerTask = Os_GlobalState.runningTask;

    /* Push to resource stack */
    Os_ResourceStack[Os_ResourceStackTop] = ResID;
    Os_ResourceStackTop++;
    Os_GlobalState.resourceNesting++;

    return E_OK;
}

/**
 * @brief Release a resource
 * @details Releases a previously acquired resource
 *          Resources must be released in LIFO order
 *
 * @param ResID Resource to release
 * @return StatusType E_OK or error code
 */
StatusType Os_ReleaseResource(ResourceType ResID)
{
    Os_ResourceControlBlockType *pRes;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ResID >= OS_RESOURCE_COUNT && ResID != RES_SCHEDULER)
    {
        Os_SetError(E_OS_ID, OS_SID_RELEASE_RESOURCE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    /* Cannot be called from ISR */
    if (xPortIsInsideInterrupt() == pdTRUE)
    {
        Os_SetError(E_OS_CALLEVEL, OS_SID_RELEASE_RESOURCE);
        Os_CallErrorHook(E_OS_CALLEVEL);
        return E_OS_CALLEVEL;
    }

    /* Check LIFO order - resource must be at top of stack */
    if (Os_ResourceStackTop == 0U || Os_ResourceStack[Os_ResourceStackTop - 1] != ResID)
    {
        Os_SetError(E_OS_NOFUNC, OS_SID_RELEASE_RESOURCE);
        Os_CallErrorHook(E_OS_NOFUNC);
        return E_OS_NOFUNC;
    }
    #endif

    /* Handle RES_SCHEDULER specially */
    if (ResID == RES_SCHEDULER)
    {
        xTaskResumeAll();
        Os_ResourceStackTop--;
        Os_GlobalState.resourceNesting--;
        return E_OK;
    }

    pRes = Os_GetResourceControlBlock(ResID);
    if (pRes == NULL)
    {
        return E_OS_ID;
    }

    /* Check if resource is held by current task */
    if (!pRes->isOccupied || pRes->ownerTask != Os_GlobalState.runningTask)
    {
        return E_OS_NOFUNC;
    }

    /* Release the mutex */
    if (pRes->xMutexHandle != NULL)
    {
        result = xSemaphoreGive(pRes->xMutexHandle);
        if (result != pdPASS)
        {
            return E_OS_STATE;
        }
    }

    /* Update resource state */
    pRes->isOccupied = FALSE;
    pRes->ownerTask = (TaskType)0;

    /* Pop from resource stack */
    Os_ResourceStackTop--;
    Os_GlobalState.resourceNesting--;

    return E_OK;
}

/*******************************************************************************
 * Schedule Table Management Implementation
 ******************************************************************************/

/**
 * @brief Start a schedule table relative to current counter value
 * @param ScheduleTableID Schedule table to start
 * @param Offset Relative offset from now
 * @return StatusType E_OK or error code
 */
StatusType Os_StartScheduleTableRel(ScheduleTableType ScheduleTableID, TickType Offset)
{
    Os_ScheduleTableControlBlockType *pSt;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ScheduleTableID >= OS_SCHEDULE_TABLE_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_START_SCHEDULE_TABLE_REL);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Offset == 0U)
    {
        Os_SetError(E_OS_VALUE, OS_SID_START_SCHEDULE_TABLE_REL);
        Os_CallErrorHook(E_OS_VALUE);
        return E_OS_VALUE;
    }
    #endif

    pSt = Os_GetScheduleTableControlBlock(ScheduleTableID);
    if (pSt == NULL)
    {
        return E_OS_ID;
    }

    /* Check if already running */
    if (pSt->status == SCHEDULETABLE_RUNNING ||
        pSt->status == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)
    {
        return E_OS_STATE;
    }

    /* Create timer if not already created */
    if (pSt->xTimerHandle == NULL)
    {
        pSt->xTimerHandle = xTimerCreate(
            "OsScheduleTable",
            pdMS_TO_TICKS(Offset * OS_TICK_MS),
            pdFALSE,  /* One-shot initially */
            (void *)pSt,
            Os_ScheduleTableCallback
        );

        if (pSt->xTimerHandle == NULL)
        {
            return E_OS_LIMIT;
        }
    }

    /* Start the timer */
    result = xTimerStart(pSt->xTimerHandle, portMAX_DELAY);
    if (result != pdPASS)
    {
        return E_OS_STATE;
    }

    pSt->status = SCHEDULETABLE_RUNNING;
    pSt->offset = Offset;

    return E_OK;
}

/**
 * @brief Start a schedule table absolute (at specific counter value)
 * @param ScheduleTableID Schedule table to start
 * @param Start Absolute start value
 * @return StatusType E_OK or error code
 */
StatusType Os_StartScheduleTableAbs(ScheduleTableType ScheduleTableID, TickType Start)
{
    Os_ScheduleTableControlBlockType *pSt;
    TickType currentTicks;
    TickType relativeTicks;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ScheduleTableID >= OS_SCHEDULE_TABLE_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_START_SCHEDULE_TABLE_ABS);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }
    #endif

    pSt = Os_GetScheduleTableControlBlock(ScheduleTableID);
    if (pSt == NULL)
    {
        return E_OS_ID;
    }

    /* Check if already running */
    if (pSt->status == SCHEDULETABLE_RUNNING ||
        pSt->status == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)
    {
        return E_OS_STATE;
    }

    /* Calculate relative offset */
    currentTicks = xTaskGetTickCount() / OS_TICK_MS;
    if (Start > currentTicks)
    {
        relativeTicks = Start - currentTicks;
    }
    else
    {
        relativeTicks = 1U;
    }

    /* Create timer if not already created */
    if (pSt->xTimerHandle == NULL)
    {
        pSt->xTimerHandle = xTimerCreate(
            "OsScheduleTable",
            pdMS_TO_TICKS(relativeTicks * OS_TICK_MS),
            pdFALSE,
            (void *)pSt,
            Os_ScheduleTableCallback
        );

        if (pSt->xTimerHandle == NULL)
        {
            return E_OS_LIMIT;
        }
    }

    result = xTimerStart(pSt->xTimerHandle, portMAX_DELAY);
    if (result != pdPASS)
    {
        return E_OS_STATE;
    }

    pSt->status = SCHEDULETABLE_RUNNING;

    return E_OK;
}

/**
 * @brief Stop a schedule table
 * @param ScheduleTableID Schedule table to stop
 * @return StatusType E_OK or error code
 */
StatusType Os_StopScheduleTable(ScheduleTableType ScheduleTableID)
{
    Os_ScheduleTableControlBlockType *pSt;
    BaseType_t result;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ScheduleTableID >= OS_SCHEDULE_TABLE_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_STOP_SCHEDULE_TABLE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }
    #endif

    pSt = Os_GetScheduleTableControlBlock(ScheduleTableID);
    if (pSt == NULL)
    {
        return E_OS_ID;
    }

    /* Can only stop running schedule tables */
    if (pSt->status != SCHEDULETABLE_RUNNING &&
        pSt->status != SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)
    {
        return E_OS_NOFUNC;
    }

    if (pSt->xTimerHandle != NULL)
    {
        result = xTimerStop(pSt->xTimerHandle, portMAX_DELAY);
        if (result != pdPASS)
        {
            return E_OS_STATE;
        }
    }

    pSt->status = SCHEDULETABLE_STOPPED;

    return E_OK;
}

/**
 * @brief Chain schedule tables (switch from one to another)
 * @param ScheduleTableID_From Current schedule table
 * @param ScheduleTableID_To Schedule table to switch to
 * @return StatusType E_OK or error code
 */
StatusType Os_NextScheduleTable(ScheduleTableType ScheduleTableID_From,
                                 ScheduleTableType ScheduleTableID_To)
{
    Os_ScheduleTableControlBlockType *pStFrom;
    Os_ScheduleTableControlBlockType *pStTo;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ScheduleTableID_From >= OS_SCHEDULE_TABLE_COUNT ||
        ScheduleTableID_To >= OS_SCHEDULE_TABLE_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_NEXT_SCHEDULE_TABLE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (ScheduleTableID_From == ScheduleTableID_To)
    {
        Os_SetError(E_OS_VALUE, OS_SID_NEXT_SCHEDULE_TABLE);
        Os_CallErrorHook(E_OS_VALUE);
        return E_OS_VALUE;
    }
    #endif

    pStFrom = Os_GetScheduleTableControlBlock(ScheduleTableID_From);
    pStTo = Os_GetScheduleTableControlBlock(ScheduleTableID_To);

    if (pStFrom == NULL || pStTo == NULL)
    {
        return E_OS_ID;
    }

    /* From schedule table must be running */
    if (pStFrom->status != SCHEDULETABLE_RUNNING &&
        pStFrom->status != SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)
    {
        return E_OS_NOFUNC;
    }

    /* To schedule table must be stopped */
    if (pStTo->status != SCHEDULETABLE_STOPPED)
    {
        return E_OS_STATE;
    }

    /* Mark for chaining - actual switch happens when From expires */
    pStFrom->status = SCHEDULETABLE_NEXT;

    return E_OK;
}

/**
 * @brief Get schedule table status
 * @param ScheduleTableID Schedule table to query
 * @param ScheduleStatus Reference to store status
 * @return StatusType E_OK or error code
 */
StatusType Os_GetScheduleTableStatus(ScheduleTableType ScheduleTableID,
                                      ScheduleTableStatusRefType ScheduleStatus)
{
    Os_ScheduleTableControlBlockType *pSt;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (ScheduleTableID >= OS_SCHEDULE_TABLE_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_SCHEDULE_TABLE_STATUS);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (ScheduleStatus == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_SCHEDULE_TABLE_STATUS);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    pSt = Os_GetScheduleTableControlBlock(ScheduleTableID);
    if (pSt == NULL)
    {
        return E_OS_ID;
    }

    *ScheduleStatus = pSt->status;

    return E_OK;
}

/*******************************************************************************
 * System Control Implementation
 ******************************************************************************/

/**
 * @brief Start the OS
 * @details Initializes the OS and starts the FreeRTOS scheduler
 *          This function never returns
 *
 * @param Mode Application mode
 */
void Os_StartOS(AppModeType Mode)
{
    uint32 i;

    /* Prevent re-entry */
    if (Os_GlobalState.isRunning)
    {
        return;
    }

    /* Initialize global state */
    Os_GlobalState.activeAppMode = Mode;
    Os_GlobalState.coreId = OS_CORE_ID_0;
    Os_GlobalState.isInitialized = TRUE;

    /* Initialize task table */
    for (i = 0U; i < OS_TASK_COUNT; i++)
    {
        Os_TaskTable[i].xTaskHandle = NULL;
        Os_TaskTable[i].xEventGroup = NULL;
        Os_TaskTable[i].isActivated = FALSE;
        Os_TaskTable[i].isSuspended = TRUE;
        Os_TaskTable[i].activationCount = 0U;
    }

    /* Initialize alarm table */
    for (i = 0U; i < OS_ALARM_COUNT; i++)
    {
        Os_AlarmTable[i].xTimerHandle = NULL;
        Os_AlarmTable[i].isActive = FALSE;
    }

    /* Initialize resource table */
    for (i = 0U; i < OS_RESOURCE_COUNT; i++)
    {
        Os_ResourceTable[i].xMutexHandle = NULL;
        Os_ResourceTable[i].isOccupied = FALSE;
    }

    /* Initialize schedule table table */
    for (i = 0U; i < OS_SCHEDULE_TABLE_COUNT; i++)
    {
        Os_ScheduleTableTable[i].xTimerHandle = NULL;
        Os_ScheduleTableTable[i].status = SCHEDULETABLE_STOPPED;
    }

    /* Call Startup Hook */
    #if (OS_USE_STARTUP_HOOK == STD_ON)
    Os_StartupHook();
    #endif

    /* Mark OS as running */
    Os_GlobalState.isRunning = TRUE;

    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here - scheduler runs forever */
    /* If we get here, scheduler failed to start */
    for (;;)
    {
        /* Infinite loop for safety */
    }
}

/**
 * @brief Shutdown the OS
 * @details Shuts down the operating system and calls shutdown hook
 *
 * @param Error Error code for shutdown
 */
void Os_ShutdownOS(StatusType Error)
{
    /* Call Shutdown Hook */
    #if (OS_USE_SHUTDOWN_HOOK == STD_ON)
    Os_ShutdownHook(Error);
    #else
    (void)Error;
    #endif

    /* Disable interrupts */
    portDISABLE_INTERRUPTS();

    /* Stop the scheduler */
    vTaskEndScheduler();

    /* Mark OS as not running */
    Os_GlobalState.isRunning = FALSE;

    /* Infinite loop - we should never return from here */
    for (;;)
    {
        /* Infinite loop for safety */
    }
}

/**
 * @brief Get OS version info
 * @param versioninfo Reference to store version info
 */
void Os_GetVersionInfo(Std_VersionInfoType *versioninfo)
{
    if (versioninfo != NULL)
    {
        versioninfo->vendorID = OS_VENDOR_ID;
        versioninfo->moduleID = OS_MODULE_ID;
        versioninfo->sw_major_version = OS_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = OS_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = OS_SW_PATCH_VERSION;
    }
}

/*******************************************************************************
 * Interrupt Management Implementation
 ******************************************************************************/

/**
 * @brief Disable all interrupts
 * @details Disables all maskable interrupts
 */
void Os_DisableAllInterrupts(void)
{
    portDISABLE_INTERRUPTS();
}

/**
 * @brief Enable all interrupts
 * @details Enables all maskable interrupts
 */
void Os_EnableAllInterrupts(void)
{
    portENABLE_INTERRUPTS();
}

/**
 * @brief Suspend all interrupts (nestable)
 * @details Suspends all interrupts with nesting counter
 */
void Os_SuspendAllInterrupts(void)
{
    vTaskSuspendAll();
    Os_SuspendAllCount++;
}

/**
 * @brief Resume all interrupts (nestable)
 * @details Resumes all interrupts when nesting counter reaches 0
 */
void Os_ResumeAllInterrupts(void)
{
    if (Os_SuspendAllCount > 0U)
    {
        Os_SuspendAllCount--;
        if (Os_SuspendAllCount == 0U)
        {
            xTaskResumeAll();
        }
    }
}

/**
 * @brief Suspend OS interrupts (category 2 only)
 * @details Suspends OS category 2 interrupts
 */
void Os_SuspendOSInterrupts(void)
{
    /* For simplicity, we use the same mechanism as SuspendAllInterrupts */
    /* In a full implementation, this would distinguish between categories */
    vTaskSuspendAll();
    Os_SuspendOSCount++;
}

/**
 * @brief Resume OS interrupts (category 2 only)
 * @details Resumes OS category 2 interrupts
 */
void Os_ResumeOSInterrupts(void)
{
    if (Os_SuspendOSCount > 0U)
    {
        Os_SuspendOSCount--;
        if (Os_SuspendOSCount == 0U)
        {
            xTaskResumeAll();
        }
    }
}

/**
 * @brief Disable interrupt source
 * @param ISRID ISR to disable
 * @return StatusType E_OK or error code
 */
StatusType Os_DisableInterruptSource(ISRType ISRID)
{
    /* Platform specific implementation */
    (void)ISRID;
    return E_OK;
}

/**
 * @brief Enable interrupt source
 * @param ISRID ISR to enable
 * @return StatusType E_OK or error code
 */
StatusType Os_EnableInterruptSource(ISRType ISRID)
{
    /* Platform specific implementation */
    (void)ISRID;
    return E_OK;
}

/**
 * @brief Clear pending interrupt
 * @param ISRID ISR to clear
 * @return StatusType E_OK or error code
 */
StatusType Os_ClearPendingInterrupt(ISRType ISRID)
{
    /* Platform specific implementation */
    (void)ISRID;
    return E_OK;
}

/*******************************************************************************
 * Counter Management Implementation
 ******************************************************************************/

/**
 * @brief Increment a counter (for software counters)
 * @param CounterID Counter to increment
 * @return StatusType E_OK or error code
 */
StatusType Os_IncrementCounter(CounterType CounterID)
{
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (CounterID >= OS_COUNTER_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_INCREMENT_COUNTER);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }
    #endif

    if (CounterID < OS_COUNTER_COUNT)
    {
        Os_CounterTable[CounterID].currentValue++;
        return E_OK;
    }

    return E_OS_ID;
}

/**
 * @brief Get current counter value
 * @param CounterID Counter to query
 * @param Value Reference to store value
 * @return StatusType E_OK or error code
 */
StatusType Os_GetCounterValue(CounterType CounterID, TickRefType Value)
{
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (CounterID >= OS_COUNTER_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_COUNTER_VALUE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Value == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_COUNTER_VALUE);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    if (CounterID < OS_COUNTER_COUNT)
    {
        if (Os_CounterTable[CounterID].isSoftwareCounter)
        {
            *Value = Os_CounterTable[CounterID].currentValue;
        }
        else
        {
            /* Hardware counter - use FreeRTOS tick count */
            *Value = (TickType)(xTaskGetTickCount() / OS_TICK_MS);
        }
        return E_OK;
    }

    return E_OS_ID;
}

/**
 * @brief Get elapsed counter value
 * @param CounterID Counter to query
 * @param Value Reference to store value (in/out)
 * @param ElapsedValue Reference to store elapsed value
 * @return StatusType E_OK or error code
 */
StatusType Os_GetElapsedCounterValue(CounterType CounterID,
                                      TickRefType Value,
                                      TickRefType ElapsedValue)
{
    TickType currentValue;
    StatusType status;

    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (CounterID >= OS_COUNTER_COUNT)
    {
        Os_SetError(E_OS_ID, OS_SID_GET_ELAPSED_COUNTER_VALUE);
        Os_CallErrorHook(E_OS_ID);
        return E_OS_ID;
    }

    if (Value == NULL || ElapsedValue == NULL)
    {
        Os_SetError(E_OS_ILLEGAL_ADDRESS, OS_SID_GET_ELAPSED_COUNTER_VALUE);
        Os_CallErrorHook(E_OS_ILLEGAL_ADDRESS);
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    status = Os_GetCounterValue(CounterID, &currentValue);
    if (status != E_OK)
    {
        return status;
    }

    /* Calculate elapsed ticks */
    if (currentValue >= *Value)
    {
        *ElapsedValue = currentValue - *Value;
    }
    else
    {
        /* Counter wrap-around */
        *ElapsedValue = (0xFFFFFFFFU - *Value) + currentValue + 1U;
    }

    /* Update reference value */
    *Value = currentValue;

    return E_OK;
}

/*******************************************************************************
 * Multi-Core Implementation
 ******************************************************************************/

/**
 * @brief Get core ID
 * @return CoreIdType Current core ID
 */
CoreIdType Os_GetCoreID(void)
{
    #if (OS_MULTICORE_ENABLED == STD_ON)
    /* Platform specific multi-core implementation */
    return Os_GlobalState.coreId;
    #else
    return OS_CORE_ID_0;
    #endif
}

/**
 * @brief Get number of cores
 * @return uint32 Number of cores
 */
uint32 Os_GetNumberOfActivatedCores(void)
{
    return (uint32)OS_CORE_COUNT;
}

/**
 * @brief Start a core
 * @param CoreID Core to start
 * @param Status Reference to store status
 */
void Os_StartCore(CoreIdType CoreID, StatusType *Status)
{
    #if (OS_MULTICORE_ENABLED == STD_ON)
    /* Platform specific multi-core implementation */
    (void)CoreID;
    if (Status != NULL)
    {
        *Status = E_OK;
    }
    #else
    (void)CoreID;
    if (Status != NULL)
    {
        *Status = E_OS_CORE;
    }
    #endif
}

/**
 * @brief Start non-autosar cores
 * @param CoreID Core to start
 * @param Status Reference to store status
 */
void Os_StartNonAutosarCore(CoreIdType CoreID, StatusType *Status)
{
    (void)CoreID;
    if (Status != NULL)
    {
        *Status = E_OS_CORE;
    }
}

/**
 * @brief Get active application mode
 * @return AppModeType Current application mode
 */
AppModeType Os_GetActiveApplicationMode(void)
{
    return Os_GlobalState.activeAppMode;
}

/*******************************************************************************
 * Spinlock Implementation (for multicore)
 ******************************************************************************/

/**
 * @brief Get a spinlock
 * @param SpinlockId Spinlock to get
 * @return StatusType E_OK or error code
 */
StatusType Os_GetSpinlock(SpinlockIdType SpinlockId)
{
    #if (OS_MULTICORE_ENABLED == STD_ON)
    (void)SpinlockId;
    /* Platform specific spinlock implementation */
    return E_OK;
    #else
    (void)SpinlockId;
    return E_OS_CORE;
    #endif
}

/**
 * @brief Release a spinlock
 * @param SpinlockId Spinlock to release
 * @return StatusType E_OK or error code
 */
StatusType Os_ReleaseSpinlock(SpinlockIdType SpinlockId)
{
    #if (OS_MULTICORE_ENABLED == STD_ON)
    (void)SpinlockId;
    /* Platform specific spinlock implementation */
    return E_OK;
    #else
    (void)SpinlockId;
    return E_OS_CORE;
    #endif
}

/**
 * @brief Try to get spinlock (non-blocking)
 * @param SpinlockId Spinlock to try
 * @param Success Reference to store result
 * @return StatusType E_OK or error code
 */
StatusType Os_TryToGetSpinlock(SpinlockIdType SpinlockId,
                                TryToGetSpinlockRefType Success)
{
    #if (OS_MULTICORE_ENABLED == STD_ON)
    (void)SpinlockId;
    if (Success != NULL)
    {
        *Success = TRYTOGETSPINLOCK_SUCCESS;
    }
    return E_OK;
    #else
    (void)SpinlockId;
    if (Success != NULL)
    {
        *Success = TRYTOGETSPINLOCK_NOSUCCESS;
    }
    return E_OS_CORE;
    #endif
}

/*******************************************************************************
 * Timing Protection Implementation
 ******************************************************************************/

/**
 * @brief Get task execution time
 * @param TaskID Task to query
 * @param Value Reference to store execution time
 * @return StatusType E_OK or error code
 */
StatusType Os_GetTaskExecutionTime(TaskType TaskID, TimeInMicrosecondsType *Value)
{
    #if (OS_EXTENDED_ERROR_CHECK == 1)
    if (TaskID >= OS_TASK_COUNT)
    {
        return E_OS_ID;
    }

    if (Value == NULL)
    {
        return E_OS_ILLEGAL_ADDRESS;
    }
    #endif

    (void)TaskID;
    if (Value != NULL)
    {
        *Value = 0U;  /* Not implemented in this version */
    }

    return E_OS_NOFUNC;  /* Feature not available */
}

/*******************************************************************************
 * Control Implementation
 ******************************************************************************/

/**
 * @brief Control idle behavior
 * @param CoreID Core to control
 * @param IdleMode Idle mode
 * @return StatusType E_OK or error code
 */
StatusType Os_ControlIdle(CoreIdType CoreID, IdleModeType IdleMode)
{
    (void)CoreID;
    (void)IdleMode;
    return E_OK;
}

/*******************************************************************************
 * Trusted Function Implementation
 ******************************************************************************/

/**
 * @brief Call a trusted function
 * @param FunctionIndex Function to call
 * @param FunctionParams Parameters
 * @return StatusType E_OK or error code
 */
StatusType Os_CallTrustedFunction(TrustedFunctionIndexType FunctionIndex,
                                   TrustedFunctionParameterRefType FunctionParams)
{
    (void)FunctionIndex;
    (void)FunctionParams;
    return E_OS_NOFUNC;  /* Trusted functions not implemented in this version */
}

/*******************************************************************************
 * FreeRTOS Hooks (required by FreeRTOS)
 ******************************************************************************/

/**
 * @brief FreeRTOS Idle Hook
 * @details Called by FreeRTOS idle task
 */
void vApplicationIdleHook(void)
{
    #if (OS_USE_IDLE_HOOK == STD_ON)
    Os_IdleHook();
    #endif
}

/**
 * @brief FreeRTOS Tick Hook
 * @details Called at each tick interrupt
 */
void vApplicationTickHook(void)
{
    /* Can be used for timing protection implementation */
}

/**
 * @brief FreeRTOS Stack Overflow Hook
 * @details Called when a stack overflow is detected
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    /* Stack overflow detected - call protection hook or shutdown */
    #if (OS_USE_PROTECTION_HOOK == STD_ON)
    (void)Os_ProtectionHook(E_OS_STACKFAULT);
    #endif

    /* Shut down the system */
    Os_ShutdownOS(E_OS_STACKFAULT);
}

/**
 * @brief FreeRTOS Malloc Failed Hook
 * @details Called when memory allocation fails
 */
void vApplicationMallocFailedHook(void)
{
    /* Memory allocation failed */
    #if (OS_USE_PROTECTION_HOOK == STD_ON)
    (void)Os_ProtectionHook(E_OS_PROTECTION_MEMORY);
    #endif

    /* Shut down the system */
    Os_ShutdownOS(E_OS_PROTECTION_MEMORY);
}

/*******************************************************************************
 * End of File
 ******************************************************************************/
