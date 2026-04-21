/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : OS (Operating System)
* Dependencies         : FreeRTOS V10.6.x / V11.x
*
* SW Version           : 1.0.0
* Build Version        : YULETECH_OS_1.0.0
* Build Date           : 2026-04-21
* Author               : AI Agent (OS Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Os.h"
#include "Os_Internal.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define OS_INSTANCE_ID                  (0x00U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (OS_DEV_ERROR_DETECT == STD_ON)
    #define OS_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(OS_MODULE_ID, OS_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define OS_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/

/*==================================================================================================
*                                  GLOBAL VARIABLES
==================================================================================================*/
#define OS_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

Os_GlobalStateType Os_GlobalState;

#define OS_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void Os_InitAlarms(void);
static void Os_InitResources(void);
static void Os_InitTasks(AppModeType Mode);

/*==================================================================================================
*                                  FUNCTION IMPLEMENTATIONS
==================================================================================================*/
#define OS_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initialize the Operating System
 *
 * @param[in] Mode Application mode to start
 *
 * @pre None
 * @post OS initialized and scheduler started
 */
void StartOS(AppModeType Mode)
{
    Os_Internal_StartOS(Mode);
}

/**
 * @brief Internal OS startup
 */
void Os_Internal_StartOS(AppModeType Mode)
{
    uint32 i;

    memset(&Os_GlobalState, 0, sizeof(Os_GlobalState));
    Os_GlobalState.CurrentAppMode = Mode;
    Os_GlobalState.IsRunning = TRUE;

    /* Initialize resources */
    Os_InitResources();

    /* Initialize alarms */
    Os_InitAlarms();

    /* Initialize tasks based on application mode */
    Os_InitTasks(Mode);

    /* Call startup hook */
    Os_Internal_StartupHook();

    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here unless scheduler stops */
    Os_Internal_ShutdownHook(E_OS_OK);
}

/**
 * @brief Shutdown the Operating System
 *
 * @param[in] Error Error code for shutdown reason
 */
void ShutdownOS(StatusType Error)
{
    Os_Internal_ShutdownOS(Error);
}

/**
 * @brief Internal OS shutdown
 */
void Os_Internal_ShutdownOS(StatusType Error)
{
    Os_GlobalState.IsRunning = FALSE;
    Os_Internal_ShutdownHook(Error);

    /* In FreeRTOS, vTaskEndScheduler is only available on some ports */
    #ifdef vTaskEndScheduler
    vTaskEndScheduler();
    #endif

    /* Infinite loop - system halted */
    for(;;)
    {
        /* System is shut down */
    }
}

/**
 * @brief Get the active application mode
 *
 * @return AppModeType Current application mode
 */
AppModeType GetActiveApplicationMode(void)
{
    return Os_GlobalState.CurrentAppMode;
}

/*==================================================================================================
*                                  TASK MANAGEMENT
==================================================================================================*/

/**
 * @brief Activate a task
 *
 * @param[in] TaskID ID of the task to activate
 *
 * @return StatusType
 *         - E_OS_OK: Task activated successfully
 *         - E_OS_ID: Invalid task ID
 *         - E_OS_LIMIT: Too many task activations
 */
StatusType ActivateTask(TaskType TaskID)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID >= Os_GlobalState.NumTasks)
    {
        OS_DET_REPORT_ERROR(OS_SID_ACTIVATETASK, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    status = Os_Internal_ActivateTask(TaskID);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_ACTIVATETASK, status);
    }

    return status;
}

/**
 * @brief Internal activate task
 */
StatusType Os_Internal_ActivateTask(TaskType TaskID)
{
    Os_TaskConfigType* task;

    task = &Os_GlobalState.Tasks[TaskID];

    if (task->FreeRTOS_Task == NULL)
    {
        /* Task not created yet, create it */
        if (xTaskCreate(
                Os_TaskWrapper,
                "OsTask",
                configMINIMAL_STACK_SIZE * 2,
                (void*)task,
                task->Priority,
                &task->FreeRTOS_Task) != pdPASS)
        {
            return E_OS_LIMIT;
        }

        /* Create event group for extended tasks */
        if (task->IsExtended)
        {
            task->FreeRTOS_EventGroup = xEventGroupCreate();
            if (task->FreeRTOS_EventGroup == NULL)
            {
                vTaskDelete(task->FreeRTOS_Task);
                task->FreeRTOS_Task = NULL;
                return E_OS_LIMIT;
            }
        }
    }
    else
    {
        /* Resume the suspended task */
        vTaskResume(task->FreeRTOS_Task);
    }

    /* Trigger scheduler if preemptive */
    #if (configUSE_PREEMPTION == 1)
    taskYIELD();
    #endif

    return E_OS_OK;
}

/**
 * @brief Terminate the calling task
 *
 * @return StatusType
 *         - E_OS_OK: Task terminated
 *         - E_OS_RESOURCE: Task still holds resources
 */
StatusType TerminateTask(void)
{
    StatusType status;

    status = Os_Internal_TerminateTask();

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_TERMINATETASK, status);
    }

    return status;
}

/**
 * @brief Internal terminate task
 */
StatusType Os_Internal_TerminateTask(void)
{
    TaskHandle_t currentTask;
    uint32 i;

    currentTask = xTaskGetCurrentTaskHandle();

    /* Check if task holds any resources */
    for (i = 0; i < Os_GlobalState.NumResources; i++)
    {
        if (Os_GlobalState.Resources[i].OwnerTask == (TaskType)currentTask)
        {
            return E_OS_RESOURCE;
        }
    }

    /* Call post-task hook */
    Os_Internal_PostTaskHook();

    /* Suspend the task (AutoSAR tasks don't get deleted, they suspend) */
    vTaskSuspend(NULL);

    /* This line is never reached */
    return E_OS_OK;
}

/**
 * @brief Chain to another task (terminate self and activate new task)
 *
 * @param[in] TaskID ID of the task to chain to
 *
 * @return StatusType
 *         - E_OS_OK: Task chained successfully
 *         - E_OS_ID: Invalid task ID
 */
StatusType ChainTask(TaskType TaskID)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID >= Os_GlobalState.NumTasks)
    {
        OS_DET_REPORT_ERROR(OS_SID_CHAINTASK, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    /* Activate the new task first */
    status = Os_Internal_ActivateTask(TaskID);
    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_CHAINTASK, status);
        return status;
    }

    /* Then terminate self */
    status = Os_Internal_TerminateTask();

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_CHAINTASK, status);
    }

    return status;
}

/**
 * @brief Explicitly trigger rescheduling
 *
 * @return StatusType
 *         - E_OS_OK: Schedule performed
 *         - E_OS_CALLEVEL: Called from wrong context
 */
StatusType Schedule(void)
{
    taskYIELD();
    return E_OS_OK;
}

/**
 * @brief Get the ID of the current task
 *
 * @param[out] TaskID Pointer to store the task ID
 *
 * @return StatusType
 *         - E_OS_OK: Task ID retrieved
 *         - E_OS_PARAM_POINTER: Invalid pointer
 */
StatusType GetTaskID(TaskRefType TaskID)
{
    TaskHandle_t currentTask;
    uint32 i;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID == NULL)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETTASKID, E_OS_PARAM_POINTER);
        return E_OS_PARAM_POINTER;
    }
    #endif

    currentTask = xTaskGetCurrentTaskHandle();

    /* Find the task ID from FreeRTOS handle */
    for (i = 0; i < Os_GlobalState.NumTasks; i++)
    {
        if (Os_GlobalState.Tasks[i].FreeRTOS_Task == currentTask)
        {
            *TaskID = Os_GlobalState.Tasks[i].TaskID;
            return E_OS_OK;
        }
    }

    *TaskID = 0;
    return E_OS_OK;
}

/**
 * @brief Get the state of a task
 *
 * @param[in] TaskID ID of the task
 * @param[out] State Pointer to store the task state
 *
 * @return StatusType
 *         - E_OS_OK: State retrieved
 *         - E_OS_ID: Invalid task ID
 *         - E_OS_PARAM_POINTER: Invalid pointer
 */
StatusType GetTaskState(TaskType TaskID, TaskStateRefType State)
{
    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID >= Os_GlobalState.NumTasks)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETTASKSTATE, E_OS_ID);
        return E_OS_ID;
    }
    if (State == NULL)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETTASKSTATE, E_OS_PARAM_POINTER);
        return E_OS_PARAM_POINTER;
    }
    #endif

    return Os_Internal_GetTaskState(TaskID, State);
}

/**
 * @brief Internal get task state
 */
StatusType Os_Internal_GetTaskState(TaskType TaskID, TaskStateRefType State)
{
    Os_TaskConfigType* task;
    eTaskState freeRtosState;

    task = &Os_GlobalState.Tasks[TaskID];

    if (task->FreeRTOS_Task == NULL)
    {
        *State = SUSPENDED;
        return E_OS_OK;
    }

    freeRtosState = eTaskGetState(task->FreeRTOS_Task);

    switch (freeRtosState)
    {
        case eRunning:
            *State = RUNNING;
            break;
        case eReady:
            *State = READY;
            break;
        case eBlocked:
            *State = WAITING;
            break;
        case eSuspended:
            *State = SUSPENDED;
            break;
        default:
            *State = SUSPENDED;
            break;
    }

    return E_OS_OK;
}

/*==================================================================================================
*                                  INTERRUPT MANAGEMENT
==================================================================================================*/

/**
 * @brief Enable all interrupts
 */
void EnableAllInterrupts(void)
{
    portENABLE_INTERRUPTS();
}

/**
 * @brief Disable all interrupts
 */
void DisableAllInterrupts(void)
{
    portDISABLE_INTERRUPTS();
}

/**
 * @brief Resume all interrupts
 */
void ResumeAllInterrupts(void)
{
    portENABLE_INTERRUPTS();
}

/**
 * @brief Suspend all interrupts
 */
void SuspendAllInterrupts(void)
{
    portDISABLE_INTERRUPTS();
}

/**
 * @brief Resume OS interrupts (category 2)
 */
void ResumeOSInterrupts(void)
{
    portENABLE_INTERRUPTS();
}

/**
 * @brief Suspend OS interrupts (category 2)
 */
void SuspendOSInterrupts(void)
{
    portDISABLE_INTERRUPTS();
}

/*==================================================================================================
*                                  EVENT MANAGEMENT
==================================================================================================*/

/**
 * @brief Set events for a task
 *
 * @param[in] TaskID ID of the task
 * @param[in] Mask Event mask to set
 *
 * @return StatusType
 *         - E_OS_OK: Events set
 *         - E_OS_ID: Invalid task ID
 *         - E_OS_ACCESS: Task is not an extended task
 *         - E_OS_STATE: Task is in invalid state
 */
StatusType SetEvent(TaskType TaskID, EventMaskType Mask)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID >= Os_GlobalState.NumTasks)
    {
        OS_DET_REPORT_ERROR(OS_SID_SETEVENT, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    status = Os_Internal_SetEvent(TaskID, Mask);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_SETEVENT, status);
    }

    return status;
}

/**
 * @brief Internal set event
 */
StatusType Os_Internal_SetEvent(TaskType TaskID, EventMaskType Mask)
{
    Os_TaskConfigType* task;

    task = &Os_GlobalState.Tasks[TaskID];

    if (!task->IsExtended)
    {
        return E_OS_ACCESS;
    }

    if (task->FreeRTOS_EventGroup == NULL)
    {
        return E_OS_STATE;
    }

    xEventGroupSetBits(task->FreeRTOS_EventGroup, (EventBits_t)Mask);

    return E_OS_OK;
}

/**
 * @brief Clear events for the calling task
 *
 * @param[in] Mask Event mask to clear
 *
 * @return StatusType
 *         - E_OS_OK: Events cleared
 *         - E_OS_ACCESS: Task is not an extended task
 *         - E_OS_CALLEVEL: Called from ISR
 */
StatusType ClearEvent(EventMaskType Mask)
{
    StatusType status;

    status = Os_Internal_ClearEvent(Mask);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_CLEAREVENT, status);
    }

    return status;
}

/**
 * @brief Internal clear event
 */
StatusType Os_Internal_ClearEvent(EventMaskType Mask)
{
    TaskHandle_t currentTask;
    uint32 i;

    currentTask = xTaskGetCurrentTaskHandle();

    /* Find current task config */
    for (i = 0; i < Os_GlobalState.NumTasks; i++)
    {
        if (Os_GlobalState.Tasks[i].FreeRTOS_Task == currentTask)
        {
            if (!Os_GlobalState.Tasks[i].IsExtended)
            {
                return E_OS_ACCESS;
            }

            if (Os_GlobalState.Tasks[i].FreeRTOS_EventGroup == NULL)
            {
                return E_OS_STATE;
            }

            xEventGroupClearBits(
                Os_GlobalState.Tasks[i].FreeRTOS_EventGroup,
                (EventBits_t)Mask);

            return E_OS_OK;
        }
    }

    return E_OS_ACCESS;
}

/**
 * @brief Wait for events
 *
 * @param[in] Mask Event mask to wait for
 *
 * @return StatusType
 *         - E_OS_OK: Events received
 *         - E_OS_ACCESS: Task is not an extended task
 *         - E_OS_RESOURCE: Task holds resources
 */
StatusType WaitEvent(EventMaskType Mask)
{
    StatusType status;

    status = Os_Internal_WaitEvent(Mask);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_WAITEVENT, status);
    }

    return status;
}

/**
 * @brief Internal wait event
 */
StatusType Os_Internal_WaitEvent(EventMaskType Mask)
{
    TaskHandle_t currentTask;
    uint32 i;
    EventBits_t bits;

    currentTask = xTaskGetCurrentTaskHandle();

    /* Find current task and check resources */
    for (i = 0; i < Os_GlobalState.NumTasks; i++)
    {
        if (Os_GlobalState.Tasks[i].FreeRTOS_Task == currentTask)
        {
            uint32 resIdx;

            if (!Os_GlobalState.Tasks[i].IsExtended)
            {
                return E_OS_ACCESS;
            }

            /* Check if task holds resources */
            for (resIdx = 0; resIdx < Os_GlobalState.NumResources; resIdx++)
            {
                if (Os_GlobalState.Resources[resIdx].OwnerTask ==
                    Os_GlobalState.Tasks[i].TaskID)
                {
                    return E_OS_RESOURCE;
                }
            }

            if (Os_GlobalState.Tasks[i].FreeRTOS_EventGroup == NULL)
            {
                return E_OS_STATE;
            }

            /* Wait for events (no timeout - indefinite wait) */
            bits = xEventGroupWaitBits(
                Os_GlobalState.Tasks[i].FreeRTOS_EventGroup,
                (EventBits_t)Mask,
                pdFALSE,    /* Don't clear on exit */
                pdFALSE,    /* Wait for any bit */
                portMAX_DELAY);

            (void)bits;
            return E_OS_OK;
        }
    }

    return E_OS_ACCESS;
}

/**
 * @brief Get the current event state of a task
 *
 * @param[in] TaskID ID of the task
 * @param[out] Event Pointer to store the event mask
 *
 * @return StatusType
 *         - E_OS_OK: Events retrieved
 *         - E_OS_ID: Invalid task ID
 *         - E_OS_ACCESS: Task is not an extended task
 *         - E_OS_PARAM_POINTER: Invalid pointer
 */
StatusType GetEvent(TaskType TaskID, EventMaskRefType Event)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (TaskID >= Os_GlobalState.NumTasks)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETEVENT, E_OS_ID);
        return E_OS_ID;
    }
    if (Event == NULL)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETEVENT, E_OS_PARAM_POINTER);
        return E_OS_PARAM_POINTER;
    }
    #endif

    status = Os_Internal_GetEvent(TaskID, Event);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETEVENT, status);
    }

    return status;
}

/**
 * @brief Internal get event
 */
StatusType Os_Internal_GetEvent(TaskType TaskID, EventMaskRefType Event)
{
    Os_TaskConfigType* task;

    task = &Os_GlobalState.Tasks[TaskID];

    if (!task->IsExtended)
    {
        return E_OS_ACCESS;
    }

    if (task->FreeRTOS_EventGroup == NULL)
    {
        *Event = 0;
        return E_OS_OK;
    }

    *Event = (EventMaskType)xEventGroupGetBits(task->FreeRTOS_EventGroup);

    return E_OS_OK;
}

/*==================================================================================================
*                                  RESOURCE MANAGEMENT
==================================================================================================*/

/**
 * @brief Get a resource (mutex)
 *
 * @param[in] ResID ID of the resource
 *
 * @return StatusType
 *         - E_OS_OK: Resource acquired
 *         - E_OS_ID: Invalid resource ID
 *         - E_OS_ACCESS: Resource cannot be accessed by this task
 */
StatusType GetResource(ResourceType ResID)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (ResID >= Os_GlobalState.NumResources)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETRESOURCE, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    status = Os_Internal_GetResource(ResID);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETRESOURCE, status);
    }

    return status;
}

/**
 * @brief Internal get resource
 */
StatusType Os_Internal_GetResource(ResourceType ResID)
{
    Os_ResourceConfigType* resource;

    resource = &Os_GlobalState.Resources[ResID];

    if (resource->FreeRTOS_Mutex == NULL)
    {
        return E_OS_ID;
    }

    /* Take the mutex with indefinite timeout */
    if (xSemaphoreTake(resource->FreeRTOS_Mutex, portMAX_DELAY) != pdTRUE)
    {
        return E_OS_ACCESS;
    }

    resource->OwnerTask = (TaskType)xTaskGetCurrentTaskHandle();
    resource->NestCount++;

    return E_OS_OK;
}

/**
 * @brief Release a resource (mutex)
 *
 * @param[in] ResID ID of the resource
 *
 * @return StatusType
 *         - E_OS_OK: Resource released
 *         - E_OS_ID: Invalid resource ID
 *         - E_OS_NOFUNC: Resource not occupied by this task
 *         - E_OS_ACCESS: Nested resource access
 */
StatusType ReleaseResource(ResourceType ResID)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (ResID >= Os_GlobalState.NumResources)
    {
        OS_DET_REPORT_ERROR(OS_SID_RELEASERESOURCE, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    status = Os_Internal_ReleaseResource(ResID);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_RELEASERESOURCE, status);
    }

    return status;
}

/**
 * @brief Internal release resource
 */
StatusType Os_Internal_ReleaseResource(ResourceType ResID)
{
    Os_ResourceConfigType* resource;
    TaskHandle_t currentTask;

    resource = &Os_GlobalState.Resources[ResID];
    currentTask = xTaskGetCurrentTaskHandle();

    if (resource->FreeRTOS_Mutex == NULL)
    {
        return E_OS_ID;
    }

    if (resource->OwnerTask != (TaskType)currentTask)
    {
        return E_OS_NOFUNC;
    }

    resource->NestCount--;

    if (resource->NestCount == 0)
    {
        resource->OwnerTask = 0;
    }

    if (xSemaphoreGive(resource->FreeRTOS_Mutex) != pdTRUE)
    {
        return E_OS_ACCESS;
    }

    return E_OS_OK;
}

/*==================================================================================================
*                                  ALARM MANAGEMENT
==================================================================================================*/

/**
 * @brief Get alarm base information
 *
 * @param[in] AlarmID ID of the alarm
 * @param[out] Info Pointer to alarm base information
 *
 * @return StatusType
 *         - E_OS_OK: Information retrieved
 *         - E_OS_ID: Invalid alarm ID
 *         - E_OS_PARAM_POINTER: Invalid pointer
 */
StatusType GetAlarmBase(AlarmType AlarmID, AlarmBaseRefType Info)
{
    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (AlarmID >= Os_GlobalState.NumAlarms)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETALARMBASE, E_OS_ID);
        return E_OS_ID;
    }
    if (Info == NULL)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETALARMBASE, E_OS_PARAM_POINTER);
        return E_OS_PARAM_POINTER;
    }
    #endif

    Info->maxallowedvalue = portMAX_DELAY;
    Info->ticksperbase = OS_TICKS_PER_MS;
    Info->mincycle = 1;

    return E_OS_OK;
}

/**
 * @brief Get the current alarm value
 *
 * @param[in] AlarmID ID of the alarm
 * @param[out] Tick Pointer to store remaining ticks
 *
 * @return StatusType
 *         - E_OS_OK: Value retrieved
 *         - E_OS_ID: Invalid alarm ID
 *         - E_OS_NOFUNC: Alarm not active
 *         - E_OS_PARAM_POINTER: Invalid pointer
 */
StatusType GetAlarm(AlarmType AlarmID, TickRefType Tick)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (AlarmID >= Os_GlobalState.NumAlarms)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETALARM, E_OS_ID);
        return E_OS_ID;
    }
    if (Tick == NULL)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETALARM, E_OS_PARAM_POINTER);
        return E_OS_PARAM_POINTER;
    }
    #endif

    status = Os_Internal_GetAlarm(AlarmID, Tick);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_GETALARM, status);
    }

    return status;
}

/**
 * @brief Internal get alarm
 */
StatusType Os_Internal_GetAlarm(AlarmType AlarmID, TickRefType Tick)
{
    Os_AlarmConfigType* alarm;

    alarm = &Os_GlobalState.Alarms[AlarmID];

    if (alarm->State != OS_ALARM_ACTIVE)
    {
        return E_OS_NOFUNC;
    }

    *Tick = alarm->ExpiryTick;
    return E_OS_OK;
}

/**
 * @brief Set a relative alarm
 *
 * @param[in] AlarmID ID of the alarm
 * @param[in] Increment Relative tick value
 * @param[in] Cycle Cycle period (0 = one-shot)
 *
 * @return StatusType
 *         - E_OS_OK: Alarm set
 *         - E_OS_ID: Invalid alarm ID
 *         - E_OS_VALUE: Invalid parameter values
 */
StatusType SetRelAlarm(AlarmType AlarmID, TickType Increment, TickType Cycle)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (AlarmID >= Os_GlobalState.NumAlarms)
    {
        OS_DET_REPORT_ERROR(OS_SID_SETRELALARM, E_OS_ID);
        return E_OS_ID;
    }
    if (Increment == 0)
    {
        OS_DET_REPORT_ERROR(OS_SID_SETRELALARM, E_OS_VALUE);
        return E_OS_VALUE;
    }
    #endif

    status = Os_Internal_SetRelAlarm(AlarmID, Increment, Cycle);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_SETRELALARM, status);
    }

    return status;
}

/**
 * @brief Internal set relative alarm
 */
StatusType Os_Internal_SetRelAlarm(AlarmType AlarmID, TickType Increment, TickType Cycle)
{
    Os_AlarmConfigType* alarm;
    TickType periodMs;

    alarm = &Os_GlobalState.Alarms[AlarmID];

    if (alarm->FreeRTOS_Timer == NULL)
    {
        return E_OS_ID;
    }

    alarm->Increment = Increment;
    alarm->Cycle = Cycle;
    alarm->State = OS_ALARM_ACTIVE;

    /* Convert ticks to milliseconds for FreeRTOS */
    periodMs = Increment / OS_TICKS_PER_MS;
    if (periodMs == 0)
    {
        periodMs = 1;
    }

    if (xTimerChangePeriod(alarm->FreeRTOS_Timer, pdMS_TO_TICKS(periodMs), 0) != pdPASS)
    {
        return E_OS_VALUE;
    }

    if (xTimerStart(alarm->FreeRTOS_Timer, 0) != pdPASS)
    {
        return E_OS_VALUE;
    }

    return E_OS_OK;
}

/**
 * @brief Set an absolute alarm
 *
 * @param[in] AlarmID ID of the alarm
 * @param[in] Start Absolute start tick
 * @param[in] Cycle Cycle period (0 = one-shot)
 *
 * @return StatusType
 *         - E_OS_OK: Alarm set
 *         - E_OS_ID: Invalid alarm ID
 *         - E_OS_VALUE: Invalid parameter values
 */
StatusType SetAbsAlarm(AlarmType AlarmID, TickType Start, TickType Cycle)
{
    /* Absolute alarms are not directly supported by FreeRTOS timers.
     * We implement them as relative alarms from current tick count. */
    TickType currentTick;
    TickType relativeIncrement;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (AlarmID >= Os_GlobalState.NumAlarms)
    {
        OS_DET_REPORT_ERROR(OS_SID_SETABSALARM, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    currentTick = (TickType)xTaskGetTickCount();

    if (Start > currentTick)
    {
        relativeIncrement = Start - currentTick;
    }
    else
    {
        relativeIncrement = 1; /* Start immediately if time has passed */
    }

    return Os_Internal_SetRelAlarm(AlarmID, relativeIncrement, Cycle);
}

/**
 * @brief Internal set absolute alarm
 */
StatusType Os_Internal_SetAbsAlarm(AlarmType AlarmID, TickType Start, TickType Cycle)
{
    return SetAbsAlarm(AlarmID, Start, Cycle);
}

/**
 * @brief Cancel an alarm
 *
 * @param[in] AlarmID ID of the alarm
 *
 * @return StatusType
 *         - E_OS_OK: Alarm canceled
 *         - E_OS_ID: Invalid alarm ID
 *         - E_OS_NOFUNC: Alarm was not active
 */
StatusType CancelAlarm(AlarmType AlarmID)
{
    StatusType status;

    #if (OS_DEV_ERROR_DETECT == STD_ON)
    if (AlarmID >= Os_GlobalState.NumAlarms)
    {
        OS_DET_REPORT_ERROR(OS_SID_CANCELALARM, E_OS_ID);
        return E_OS_ID;
    }
    #endif

    status = Os_Internal_CancelAlarm(AlarmID);

    if (status != E_OS_OK)
    {
        OS_DET_REPORT_ERROR(OS_SID_CANCELALARM, status);
    }

    return status;
}

/**
 * @brief Internal cancel alarm
 */
StatusType Os_Internal_CancelAlarm(AlarmType AlarmID)
{
    Os_AlarmConfigType* alarm;

    alarm = &Os_GlobalState.Alarms[AlarmID];

    if (alarm->FreeRTOS_Timer == NULL)
    {
        return E_OS_ID;
    }

    if (alarm->State != OS_ALARM_ACTIVE)
    {
        return E_OS_NOFUNC;
    }

    if (xTimerStop(alarm->FreeRTOS_Timer, 0) != pdPASS)
    {
        return E_OS_STATE;
    }

    alarm->State = OS_ALARM_CANCELED;

    return E_OS_OK;
}

/*==================================================================================================
*                                  HOOK FUNCTIONS
==================================================================================================*/

/**
 * @brief Error hook - called when an OS error occurs
 *
 * @param[in] Error Error code
 */
void ErrorHook(StatusType Error)
{
    Os_Internal_ErrorHook(Error);
}

/**
 * @brief Internal error hook
 */
void Os_Internal_ErrorHook(StatusType Error)
{
    (void)Error;
    /* Default implementation - can be overridden by application */
}

/**
 * @brief Pre-task hook - called before task switch in
 */
void PreTaskHook(void)
{
    Os_Internal_PreTaskHook();
}

/**
 * @brief Internal pre-task hook
 */
void Os_Internal_PreTaskHook(void)
{
    /* Default implementation - can be overridden by application */
}

/**
 * @brief Post-task hook - called after task switch out
 */
void PostTaskHook(void)
{
    Os_Internal_PostTaskHook();
}

/**
 * @brief Internal post-task hook
 */
void Os_Internal_PostTaskHook(void)
{
    /* Default implementation - can be overridden by application */
}

/**
 * @brief Startup hook - called after OS initialization
 */
void StartupHook(void)
{
    Os_Internal_StartupHook();
}

/**
 * @brief Internal startup hook
 */
void Os_Internal_StartupHook(void)
{
    /* Default implementation - can be overridden by application */
}

/**
 * @brief Shutdown hook - called before OS shutdown
 *
 * @param[in] Error Error code for shutdown reason
 */
void ShutdownHook(StatusType Error)
{
    Os_Internal_ShutdownHook(Error);
}

/**
 * @brief Internal shutdown hook
 */
void Os_Internal_ShutdownHook(StatusType Error)
{
    (void)Error;
    /* Default implementation - can be overridden by application */
}

/*==================================================================================================
*                                  VERSION INFO
==================================================================================================*/

/**
 * @brief Get OS version information
 *
 * @param[out] versioninfo Pointer to version information structure
 */
void Os_GetVersionInfo(Std_VersionInfoType* versioninfo)
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

/*==================================================================================================
*                                  LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize resources (mutexes)
 */
static void Os_InitResources(void)
{
    uint32 i;

    for (i = 0; i < Os_GlobalState.NumResources; i++)
    {
        Os_ResourceConfigType* resource = &Os_GlobalState.Resources[i];

        resource->ResID = (ResourceType)i;
        resource->FreeRTOS_Mutex = xSemaphoreCreateMutex();
        resource->OwnerTask = 0;
        resource->NestCount = 0;
        resource->IsCeilingPriority = FALSE;
        resource->CeilingPriority = 0;
    }
}

/**
 * @brief Initialize alarms (timers)
 */
static void Os_InitAlarms(void)
{
    uint32 i;

    for (i = 0; i < Os_GlobalState.NumAlarms; i++)
    {
        Os_AlarmConfigType* alarm = &Os_GlobalState.Alarms[i];

        alarm->AlarmID = (AlarmType)i;
        alarm->State = OS_ALARM_UNUSED;
        alarm->Callback = NULL;
        alarm->FreeRTOS_Timer = xTimerCreate(
            "OsAlarm",
            pdMS_TO_TICKS(1000),
            pdFALSE, /* One-shot by default */
            (void*)alarm,
            Os_AlarmCallback);
    }
}

/**
 * @brief Initialize tasks
 */
static void Os_InitTasks(AppModeType Mode)
{
    uint32 i;
    (void)Mode;

    for (i = 0; i < Os_GlobalState.NumTasks; i++)
    {
        Os_TaskConfigType* task = &Os_GlobalState.Tasks[i];

        task->TaskID = (TaskType)i;
        task->FreeRTOS_Task = NULL;
        task->FreeRTOS_EventGroup = NULL;

        if (task->IsAutoStart)
        {
            Os_Internal_ActivateTask(task->TaskID);
        }
    }
}

/*==================================================================================================
*                                  FREERTOS CALLBACKS
==================================================================================================*/

/**
 * @brief Alarm callback wrapper for FreeRTOS timers
 */
void Os_AlarmCallback(TimerHandle_t xTimer)
{
    Os_AlarmConfigType* alarm;

    alarm = (Os_AlarmConfigType*)pvTimerGetTimerID(xTimer);

    if (alarm != NULL && alarm->Callback != NULL)
    {
        alarm->Callback();
    }

    /* Handle cyclic alarms */
    if (alarm->Cycle > 0 && alarm->State == OS_ALARM_ACTIVE)
    {
        TickType cycleMs = alarm->Cycle / OS_TICKS_PER_MS;
        if (cycleMs == 0)
        {
            cycleMs = 1;
        }

        xTimerChangePeriod(xTimer, pdMS_TO_TICKS(cycleMs), 0);
        xTimerStart(xTimer, 0);
    }
}

/**
 * @brief Task wrapper for FreeRTOS
 */
void Os_TaskWrapper(void* pvParameters)
{
    Os_TaskConfigType* task;

    task = (Os_TaskConfigType*)pvParameters;

    /* Call pre-task hook */
    Os_Internal_PreTaskHook();

    /* Execute task entry point */
    if (task->EntryPoint != NULL)
    {
        task->EntryPoint();
    }

    /* Task should not return in AutoSAR - if it does, suspend it */
    Os_Internal_TerminateTask();
}

#define OS_STOP_SEC_CODE
#include "MemMap.h"
