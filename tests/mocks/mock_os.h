/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : OS Mock Header
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef MOCK_OS_H
#define MOCK_OS_H

#include "Std_Types.h"

/*==================================================================================================
*                                      TASK MOCK
==================================================================================================*/
#define OS_MOCK_MAX_TASKS   32

typedef enum {
    OS_MOCK_TASK_SUSPENDED = 0,
    OS_MOCK_TASK_READY,
    OS_MOCK_TASK_RUNNING,
    OS_MOCK_TASK_WAITING
} Os_MockTaskStateType;

typedef struct {
    boolean Active;
    uint8 State;
    uint8 Priority;
    uint32 ActivationCount;
    uint32 ContextSwitches;
} Os_MockTaskType;

extern Os_MockTaskType Os_MockTasks[OS_MOCK_MAX_TASKS];

void Os_Mock_Reset(void);
void Os_Mock_SetTaskState(uint8 taskId, uint8 state);
uint8 Os_Mock_GetTaskState(uint8 taskId);

/*==================================================================================================
*                                      EVENT MOCK
==================================================================================================*/
#define OS_MOCK_MAX_EVENTS  64

typedef struct {
    boolean Set;
    uint8 OwnerTask;
    uint32 EventMask;
} Os_MockEventType;

extern Os_MockEventType Os_MockEvents[OS_MOCK_MAX_EVENTS];

void Os_Mock_SetEvent(uint8 eventId, uint32 eventMask);
void Os_Mock_ClearEvent(uint8 eventId);
uint32 Os_Mock_GetEvent(uint8 eventId);

/*==================================================================================================
*                                      ALARM MOCK
==================================================================================================*/
#define OS_MOCK_MAX_ALARMS  16

typedef struct {
    boolean Active;
    uint32 Period;
    uint32 Count;
    uint8 CallbackTask;
} Os_MockAlarmType;

extern Os_MockAlarmType Os_MockAlarms[OS_MOCK_MAX_ALARMS];

void Os_Mock_ResetAlarms(void);
void Os_Mock_SetAlarm(uint8 alarmId, uint32 period);
void Os_Mock_CancelAlarm(uint8 alarmId);

/*==================================================================================================
*                                      RESOURCE MOCK
==================================================================================================*/
#define OS_MOCK_MAX_RESOURCES   16

typedef struct {
    boolean Locked;
    uint8 OwnerTask;
    uint8 PriorityCeiling;
} Os_MockResourceType;

extern Os_MockResourceType Os_MockResources[OS_MOCK_MAX_RESOURCES];

void Os_Mock_ResetResources(void);
void Os_Mock_LockResource(uint8 resId, uint8 taskId);
void Os_Mock_UnlockResource(uint8 resId);
boolean Os_Mock_IsResourceLocked(uint8 resId);

/*==================================================================================================
*                                      ISR MOCK
==================================================================================================*/
#define OS_MOCK_MAX_ISRS    16

typedef struct {
    boolean Enabled;
    uint8 Priority;
    uint32 CallCount;
    void (*Handler)(void);
} Os_MockIsrType;

extern Os_MockIsrType Os_MockIsrs[OS_MOCK_MAX_ISRS];

void Os_Mock_ResetIsrs(void);
void Os_Mock_EnableIsr(uint8 isrId);
void Os_Mock_DisableIsr(uint8 isrId);
void Os_Mock_TriggerIsr(uint8 isrId);

/*==================================================================================================
*                                      COUNTER MOCK
==================================================================================================*/
#define OS_MOCK_MAX_COUNTERS    8

typedef struct {
    uint32 Value;
    uint32 MaxValue;
    uint32 TickPeriod;
} Os_MockCounterType;

extern Os_MockCounterType Os_MockCounters[OS_MOCK_MAX_COUNTERS];

void Os_Mock_ResetCounters(void);
void Os_Mock_IncrementCounter(uint8 counterId, uint32 increment);
uint32 Os_Mock_GetCounterValue(uint8 counterId);

/*==================================================================================================
*                                      SCHEDULE TABLE MOCK
==================================================================================================*/
#define OS_MOCK_MAX_SCHEDULES   8

typedef enum {
    OS_MOCK_SCHEDULE_STOPPED = 0,
    OS_MOCK_SCHEDULE_NEXT,
    OS_MOCK_SCHEDULE_WAITING,
    OS_MOCK_SCHEDULE_RUNNING
} Os_MockScheduleStateType;

typedef struct {
    uint8 State;
    uint32 Duration;
    uint32 Position;
} Os_MockScheduleType;

extern Os_MockScheduleType Os_MockSchedules[OS_MOCK_MAX_SCHEDULES];

void Os_Mock_ResetSchedules(void);
void Os_Mock_StartSchedule(uint8 scheduleId);
void Os_Mock_StopSchedule(uint8 scheduleId);

#endif /* MOCK_OS_H */
