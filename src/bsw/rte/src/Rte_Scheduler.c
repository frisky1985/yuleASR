/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Runtime Environment)
* Dependencies         : Rte, OS
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (RTE Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte.h"
#include "Rte_Type.h"
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_SCHEDULER_INSTANCE_ID       (0x00U)

/* Maximum number of tasks */
#define RTE_MAX_TASKS                   (8U)

/* Maximum number of events per task */
#define RTE_MAX_EVENTS_PER_TASK         (16U)

/* Scheduler tick period in ms */
#define RTE_SCHEDULER_TICK_MS           (1U)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* Task state */
typedef enum
{
    RTE_TASK_SUSPENDED = 0,
    RTE_TASK_READY,
    RTE_TASK_RUNNING,
    RTE_TASK_WAITING
} Rte_TaskStateType;

/* Task type */
typedef enum
{
    RTE_TASK_BASIC = 0,
    RTE_TASK_EXTENDED
} Rte_TaskType;

/* Task control block */
typedef struct
{
    uint8 TaskId;
    uint8 Priority;
    Rte_TaskStateType State;
    Rte_TaskType Type;
    uint32 PeriodMs;
    uint32 Timer;
    uint32 EventMask;
    uint32 WaitedEvents;
    void (*EntryPoint)(void);
    boolean IsActive;
} Rte_TaskControlBlockType;

/* Event control block */
typedef struct
{
    uint8 EventId;
    uint8 TaskId;
    boolean IsSet;
    uint32 EventMask;
} Rte_EventControlBlockType;

/* Scheduler state */
typedef struct
{
    boolean IsInitialized;
    boolean IsRunning;
    uint8 NumTasks;
    uint8 CurrentTask;
    uint32 TickCounter;
    Rte_TaskControlBlockType Tasks[RTE_MAX_TASKS];
    Rte_EventControlBlockType Events[RTE_MAX_EVENTS_PER_TASK];
} Rte_SchedulerStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Rte_SchedulerStateType Rte_SchedulerState;

#define RTE_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Rte_SchedulerDispatch(void);
STATIC void Rte_SchedulerSelectNextTask(void);
STATIC uint8 Rte_SchedulerFindHighestPriorityReadyTask(void);
STATIC void Rte_SchedulerUpdateTaskTimers(void);
STATIC Std_ReturnType Rte_SchedulerActivateTask(uint8 taskId);
STATIC void Rte_SchedulerTerminateTask(uint8 taskId);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find highest priority ready task
 */
STATIC uint8 Rte_SchedulerFindHighestPriorityReadyTask(void)
{
    uint8 highestPriorityTask = 0xFFU;
    uint8 highestPriority = 0xFFU;
    uint8 i;

    for (i = 0U; i < Rte_SchedulerState.NumTasks; i++)
    {
        if (Rte_SchedulerState.Tasks[i].IsActive &&
            (Rte_SchedulerState.Tasks[i].State == RTE_TASK_READY))
        {
            if (Rte_SchedulerState.Tasks[i].Priority < highestPriority)
            {
                highestPriority = Rte_SchedulerState.Tasks[i].Priority;
                highestPriorityTask = i;
            }
        }
    }

    return highestPriorityTask;
}

/**
 * @brief   Update task timers
 */
STATIC void Rte_SchedulerUpdateTaskTimers(void)
{
    uint8 i;

    for (i = 0U; i < Rte_SchedulerState.NumTasks; i++)
    {
        if (Rte_SchedulerState.Tasks[i].IsActive &&
            (Rte_SchedulerState.Tasks[i].PeriodMs > 0U))
        {
            if (Rte_SchedulerState.Tasks[i].Timer == 0U)
            {
                /* Task period expired - make it ready */
                if (Rte_SchedulerState.Tasks[i].State == RTE_TASK_SUSPENDED)
                {
                    Rte_SchedulerState.Tasks[i].State = RTE_TASK_READY;
                }
                Rte_SchedulerState.Tasks[i].Timer = Rte_SchedulerState.Tasks[i].PeriodMs;
            }
            else
            {
                Rte_SchedulerState.Tasks[i].Timer--;
            }
        }
    }
}

/**
 * @brief   Select next task to run
 */
STATIC void Rte_SchedulerSelectNextTask(void)
{
    uint8 nextTask = Rte_SchedulerFindHighestPriorityReadyTask();

    if (nextTask != 0xFFU)
    {
        /* Preempt current task if new task has higher priority */
        if (Rte_SchedulerState.CurrentTask != 0xFFU)
        {
            if (Rte_SchedulerState.Tasks[nextTask].Priority <
                Rte_SchedulerState.Tasks[Rte_SchedulerState.CurrentTask].Priority)
            {
                /* Preempt current task */
                Rte_SchedulerState.Tasks[Rte_SchedulerState.CurrentTask].State = RTE_TASK_READY;
            }
        }

        Rte_SchedulerState.CurrentTask = nextTask;
        Rte_SchedulerState.Tasks[nextTask].State = RTE_TASK_RUNNING;
    }
}

/**
 * @brief   Dispatch tasks
 */
STATIC void Rte_SchedulerDispatch(void)
{
    if (Rte_SchedulerState.CurrentTask != 0xFFU)
    {
        Rte_TaskControlBlockType* taskPtr = &Rte_SchedulerState.Tasks[Rte_SchedulerState.CurrentTask];

        if (taskPtr->EntryPoint != NULL_PTR)
        {
            taskPtr->EntryPoint();
        }

        /* Task completed - suspend or make ready for next period */
        if (taskPtr->PeriodMs > 0U)
        {
            taskPtr->State = RTE_TASK_SUSPENDED;
        }
        else
        {
            taskPtr->State = RTE_TASK_READY;
        }
    }
}

/**
 * @brief   Activate a task
 */
STATIC Std_ReturnType Rte_SchedulerActivateTask(uint8 taskId)
{
    Std_ReturnType result = E_NOT_OK;

    if (taskId < Rte_SchedulerState.NumTasks)
    {
        if (Rte_SchedulerState.Tasks[taskId].IsActive)
        {
            Rte_SchedulerState.Tasks[taskId].State = RTE_TASK_READY;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Terminate a task
 */
STATIC void Rte_SchedulerTerminateTask(uint8 taskId)
{
    if (taskId < Rte_SchedulerState.NumTasks)
    {
        Rte_SchedulerState.Tasks[taskId].State = RTE_TASK_SUSPENDED;

        if (Rte_SchedulerState.CurrentTask == taskId)
        {
            Rte_SchedulerState.CurrentTask = 0xFFU;
        }
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes RTE Scheduler
 */
void Rte_Scheduler_Init(void)
{
    uint8 i;

    Rte_SchedulerState.IsInitialized = FALSE;
    Rte_SchedulerState.IsRunning = FALSE;
    Rte_SchedulerState.NumTasks = 0U;
    Rte_SchedulerState.CurrentTask = 0xFFU;
    Rte_SchedulerState.TickCounter = 0U;

    /* Initialize tasks */
    for (i = 0U; i < RTE_MAX_TASKS; i++)
    {
        Rte_SchedulerState.Tasks[i].TaskId = i;
        Rte_SchedulerState.Tasks[i].Priority = 0xFFU;
        Rte_SchedulerState.Tasks[i].State = RTE_TASK_SUSPENDED;
        Rte_SchedulerState.Tasks[i].Type = RTE_TASK_BASIC;
        Rte_SchedulerState.Tasks[i].PeriodMs = 0U;
        Rte_SchedulerState.Tasks[i].Timer = 0U;
        Rte_SchedulerState.Tasks[i].EventMask = 0U;
        Rte_SchedulerState.Tasks[i].WaitedEvents = 0U;
        Rte_SchedulerState.Tasks[i].EntryPoint = NULL_PTR;
        Rte_SchedulerState.Tasks[i].IsActive = FALSE;
    }

    /* Initialize events */
    for (i = 0U; i < RTE_MAX_EVENTS_PER_TASK; i++)
    {
        Rte_SchedulerState.Events[i].EventId = i;
        Rte_SchedulerState.Events[i].TaskId = 0xFFU;
        Rte_SchedulerState.Events[i].IsSet = FALSE;
        Rte_SchedulerState.Events[i].EventMask = 0U;
    }

    Rte_SchedulerState.IsInitialized = TRUE;
}

/**
 * @brief   Starts RTE Scheduler
 */
void Rte_Scheduler_Start(void)
{
    if (Rte_SchedulerState.IsInitialized)
    {
        Rte_SchedulerState.IsRunning = TRUE;
    }
}

/**
 * @brief   Stops RTE Scheduler
 */
void Rte_Scheduler_Stop(void)
{
    if (Rte_SchedulerState.IsInitialized)
    {
        Rte_SchedulerState.IsRunning = FALSE;
    }
}

/**
 * @brief   Creates a task
 */
Std_ReturnType Rte_SchedulerCreateTask(uint8 taskId, uint8 priority, uint32 periodMs,
                                       void (*entryPoint)(void))
{
    Std_ReturnType result = E_NOT_OK;

    if (Rte_SchedulerState.IsInitialized && (taskId < RTE_MAX_TASKS))
    {
        if (!Rte_SchedulerState.Tasks[taskId].IsActive)
        {
            Rte_SchedulerState.Tasks[taskId].Priority = priority;
            Rte_SchedulerState.Tasks[taskId].PeriodMs = periodMs;
            Rte_SchedulerState.Tasks[taskId].Timer = periodMs;
            Rte_SchedulerState.Tasks[taskId].EntryPoint = entryPoint;
            Rte_SchedulerState.Tasks[taskId].IsActive = TRUE;
            Rte_SchedulerState.Tasks[taskId].State = RTE_TASK_SUSPENDED;

            if (taskId >= Rte_SchedulerState.NumTasks)
            {
                Rte_SchedulerState.NumTasks = taskId + 1U;
            }

            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Activates a task
 */
Std_ReturnType Rte_SchedulerActivateTask(uint8 taskId)
{
    return Rte_SchedulerActivateTask(taskId);
}

/**
 * @brief   Terminates current task
 */
void Rte_SchedulerTerminateTask(void)
{
    if (Rte_SchedulerState.CurrentTask != 0xFFU)
    {
        Rte_SchedulerTerminateTask(Rte_SchedulerState.CurrentTask);
    }
}

/**
 * @brief   Waits for events
 */
Rte_StatusType Rte_WaitForEvent(Rte_InstanceHandleType instance, Rte_EventType eventMask, uint32 timeout)
{
    Rte_StatusType result = RTE_E_OK;

    (void)instance;
    (void)eventMask;
    (void)timeout;

    /* Event waiting implementation would go here */
    /* For now, return OK */

    return result;
}

/**
 * @brief   Sets an event
 */
Rte_StatusType Rte_SetEvent(Rte_InstanceHandleType instance, Rte_EventType event)
{
    Rte_StatusType result = RTE_E_OK;

    (void)instance;
    (void)event;

    /* Event setting implementation would go here */
    /* For now, return OK */

    return result;
}

/**
 * @brief   Clears an event
 */
Rte_StatusType Rte_ClearEvent(Rte_InstanceHandleType instance, Rte_EventType event)
{
    Rte_StatusType result = RTE_E_OK;

    (void)instance;
    (void)event;

    /* Event clearing implementation would go here */
    /* For now, return OK */

    return result;
}

/**
 * @brief   Scheduler tick handler
 */
void Rte_SchedulerTick(void)
{
    if (Rte_SchedulerState.IsInitialized && Rte_SchedulerState.IsRunning)
    {
        /* Increment tick counter */
        Rte_SchedulerState.TickCounter++;

        /* Update task timers */
        Rte_SchedulerUpdateTaskTimers();

        /* Select next task */
        Rte_SchedulerSelectNextTask();

        /* Dispatch task */
        Rte_SchedulerDispatch();
    }
}

/**
 * @brief   Main function for scheduler processing
 */
void Rte_Scheduler_MainFunction(void)
{
    if (Rte_SchedulerState.IsInitialized && Rte_SchedulerState.IsRunning)
    {
        /* Process scheduler tick */
        Rte_SchedulerTick();
    }
}

/**
 * @brief   Gets scheduler status
 */
boolean Rte_SchedulerIsRunning(void)
{
    return Rte_SchedulerState.IsRunning;
}

/**
 * @brief   Gets current task ID
 */
uint8 Rte_SchedulerGetCurrentTask(void)
{
    return Rte_SchedulerState.CurrentTask;
}

/**
 * @brief   Gets tick counter
 */
uint32 Rte_SchedulerGetTickCount(void)
{
    return Rte_SchedulerState.TickCounter;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
