/**
 * @file Os_Internal.h
 * @brief AutoSAR OS Internal Header - FreeRTOS Integration
 * @version 1.0.0
 * @date 2026-04-21
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef OS_INTERNAL_H
#define OS_INTERNAL_H

#include "Os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"

/*==================================================================================================
*                                    CONFIGURATION
==================================================================================================*/
#define OS_MAX_TASKS                    (32U)
#define OS_MAX_ALARMS                   (16U)
#define OS_MAX_RESOURCES                (16U)
#define OS_MAX_EVENTS                   (32U)
#define OS_TICKS_PER_MS                 (1U)

/*==================================================================================================
*                                    INTERNAL TYPES
==================================================================================================*/
typedef enum
{
    OS_ALARM_UNUSED = 0,
    OS_ALARM_ACTIVE,
    OS_ALARM_CANCELED
} Os_AlarmStateType;

typedef struct
{
    AlarmType           AlarmID;
    TickType            Increment;
    TickType            Cycle;
    TickType            ExpiryTick;
    Os_AlarmStateType   State;
    void (*Callback)(void);
    TimerHandle_t       FreeRTOS_Timer;
} Os_AlarmConfigType;

typedef struct
{
    TaskType            TaskID;
    TaskHandle_t        FreeRTOS_Task;
    EventGroupHandle_t  FreeRTOS_EventGroup;
    uint32              Priority;
    boolean             IsAutoStart;
    boolean             IsExtended;
    void (*EntryPoint)(void);
} Os_TaskConfigType;

typedef struct
{
    ResourceType        ResID;
    SemaphoreHandle_t   FreeRTOS_Mutex;
    TaskType            OwnerTask;
    uint8               NestCount;
    boolean             IsCeilingPriority;
    uint32              CeilingPriority;
} Os_ResourceConfigType;

typedef struct
{
    boolean             IsRunning;
    AppModeType         CurrentAppMode;
    uint32              NumTasks;
    uint32              NumAlarms;
    uint32              NumResources;
    Os_TaskConfigType*  Tasks;
    Os_AlarmConfigType* Alarms;
    Os_ResourceConfigType* Resources;
} Os_GlobalStateType;

/*==================================================================================================
*                                    EXTERNAL VARIABLES
==================================================================================================*/
extern Os_GlobalStateType Os_GlobalState;

/*==================================================================================================
*                                    INTERNAL FUNCTIONS
==================================================================================================*/
StatusType Os_Internal_ActivateTask(TaskType TaskID);
StatusType Os_Internal_TerminateTask(void);
StatusType Os_Internal_ChainTask(TaskType TaskID);
StatusType Os_Internal_GetTaskState(TaskType TaskID, TaskStateRefType State);

StatusType Os_Internal_SetEvent(TaskType TaskID, EventMaskType Mask);
StatusType Os_Internal_ClearEvent(EventMaskType Mask);
StatusType Os_Internal_WaitEvent(EventMaskType Mask);
StatusType Os_Internal_GetEvent(TaskType TaskID, EventMaskRefType Event);

StatusType Os_Internal_GetResource(ResourceType ResID);
StatusType Os_Internal_ReleaseResource(ResourceType ResID);

StatusType Os_Internal_SetRelAlarm(AlarmType AlarmID, TickType Increment, TickType Cycle);
StatusType Os_Internal_SetAbsAlarm(AlarmType AlarmID, TickType Start, TickType Cycle);
StatusType Os_Internal_CancelAlarm(AlarmType AlarmID);
StatusType Os_Internal_GetAlarm(AlarmType AlarmID, TickRefType Tick);

void Os_Internal_StartOS(AppModeType Mode);
void Os_Internal_ShutdownOS(StatusType Error);

void Os_Internal_ErrorHook(StatusType Error);
void Os_Internal_PreTaskHook(void);
void Os_Internal_PostTaskHook(void);
void Os_Internal_StartupHook(void);
void Os_Internal_ShutdownHook(StatusType Error);

/* Timer callback wrapper */
void Os_AlarmCallback(TimerHandle_t xTimer);

/* Task wrapper for FreeRTOS */
void Os_TaskWrapper(void* pvParameters);

#endif /* OS_INTERNAL_H */
