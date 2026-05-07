/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : OS Mock Implementation
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "mock_os.h"
#include <string.h>

/*==================================================================================================
*                                      TASK MOCK VARIABLES
==================================================================================================*/
Os_MockTaskType Os_MockTasks[OS_MOCK_MAX_TASKS];

/*==================================================================================================
*                                      EVENT MOCK VARIABLES
==================================================================================================*/
Os_MockEventType Os_MockEvents[OS_MOCK_MAX_EVENTS];

/*==================================================================================================
*                                      ALARM MOCK VARIABLES
==================================================================================================*/
Os_MockAlarmType Os_MockAlarms[OS_MOCK_MAX_ALARMS];

/*==================================================================================================
*                                      RESOURCE MOCK VARIABLES
==================================================================================================*/
Os_MockResourceType Os_MockResources[OS_MOCK_MAX_RESOURCES];

/*==================================================================================================
*                                      ISR MOCK VARIABLES
==================================================================================================*/
Os_MockIsrType Os_MockIsrs[OS_MOCK_MAX_ISRS];

/*==================================================================================================
*                                      COUNTER MOCK VARIABLES
==================================================================================================*/
Os_MockCounterType Os_MockCounters[OS_MOCK_MAX_COUNTERS];

/*==================================================================================================
*                                      SCHEDULE TABLE MOCK VARIABLES
==================================================================================================*/
Os_MockScheduleType Os_MockSchedules[OS_MOCK_MAX_SCHEDULES];

/*==================================================================================================
*                                      TASK MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_Reset(void)
{
    memset(Os_MockTasks, 0, sizeof(Os_MockTasks));
}

void Os_Mock_SetTaskState(uint8 taskId, uint8 state)
{
    if (taskId < OS_MOCK_MAX_TASKS)
    {
        Os_MockTasks[taskId].State = state;
        if (state == OS_MOCK_TASK_RUNNING)
        {
            Os_MockTasks[taskId].ActivationCount++;
        }
    }
}

uint8 Os_Mock_GetTaskState(uint8 taskId)
{
    if (taskId < OS_MOCK_MAX_TASKS)
    {
        return Os_MockTasks[taskId].State;
    }
    return OS_MOCK_TASK_SUSPENDED;
}

/*==================================================================================================
*                                      EVENT MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_SetEvent(uint8 eventId, uint32 eventMask)
{
    if (eventId < OS_MOCK_MAX_EVENTS)
    {
        Os_MockEvents[eventId].Set = TRUE;
        Os_MockEvents[eventId].EventMask |= eventMask;
    }
}

void Os_Mock_ClearEvent(uint8 eventId)
{
    if (eventId < OS_MOCK_MAX_EVENTS)
    {
        Os_MockEvents[eventId].Set = FALSE;
        Os_MockEvents[eventId].EventMask = 0;
    }
}

uint32 Os_Mock_GetEvent(uint8 eventId)
{
    if (eventId < OS_MOCK_MAX_EVENTS)
    {
        return Os_MockEvents[eventId].EventMask;
    }
    return 0;
}

/*==================================================================================================
*                                      ALARM MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_ResetAlarms(void)
{
    memset(Os_MockAlarms, 0, sizeof(Os_MockAlarms));
}

void Os_Mock_SetAlarm(uint8 alarmId, uint32 period)
{
    if (alarmId < OS_MOCK_MAX_ALARMS)
    {
        Os_MockAlarms[alarmId].Active = TRUE;
        Os_MockAlarms[alarmId].Period = period;
        Os_MockAlarms[alarmId].Count = 0;
    }
}

void Os_Mock_CancelAlarm(uint8 alarmId)
{
    if (alarmId < OS_MOCK_MAX_ALARMS)
    {
        Os_MockAlarms[alarmId].Active = FALSE;
    }
}

/*==================================================================================================
*                                      RESOURCE MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_ResetResources(void)
{
    memset(Os_MockResources, 0, sizeof(Os_MockResources));
}

void Os_Mock_LockResource(uint8 resId, uint8 taskId)
{
    if (resId < OS_MOCK_MAX_RESOURCES)
    {
        Os_MockResources[resId].Locked = TRUE;
        Os_MockResources[resId].OwnerTask = taskId;
    }
}

void Os_Mock_UnlockResource(uint8 resId)
{
    if (resId < OS_MOCK_MAX_RESOURCES)
    {
        Os_MockResources[resId].Locked = FALSE;
        Os_MockResources[resId].OwnerTask = 0xFF;
    }
}

boolean Os_Mock_IsResourceLocked(uint8 resId)
{
    if (resId < OS_MOCK_MAX_RESOURCES)
    {
        return Os_MockResources[resId].Locked;
    }
    return FALSE;
}

/*==================================================================================================
*                                      ISR MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_ResetIsrs(void)
{
    memset(Os_MockIsrs, 0, sizeof(Os_MockIsrs));
}

void Os_Mock_EnableIsr(uint8 isrId)
{
    if (isrId < OS_MOCK_MAX_ISRS)
    {
        Os_MockIsrs[isrId].Enabled = TRUE;
    }
}

void Os_Mock_DisableIsr(uint8 isrId)
{
    if (isrId < OS_MOCK_MAX_ISRS)
    {
        Os_MockIsrs[isrId].Enabled = FALSE;
    }
}

void Os_Mock_TriggerIsr(uint8 isrId)
{
    if (isrId < OS_MOCK_MAX_ISRS && Os_MockIsrs[isrId].Enabled)
    {
        Os_MockIsrs[isrId].CallCount++;
        if (Os_MockIsrs[isrId].Handler != NULL)
        {
            Os_MockIsrs[isrId].Handler();
        }
    }
}

/*==================================================================================================
*                                      COUNTER MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_ResetCounters(void)
{
    memset(Os_MockCounters, 0, sizeof(Os_MockCounters));
}

void Os_Mock_IncrementCounter(uint8 counterId, uint32 increment)
{
    if (counterId < OS_MOCK_MAX_COUNTERS)
    {
        Os_MockCounters[counterId].Value += increment;
        if (Os_MockCounters[counterId].Value > Os_MockCounters[counterId].MaxValue)
        {
            Os_MockCounters[counterId].Value = 0;
        }
    }
}

uint32 Os_Mock_GetCounterValue(uint8 counterId)
{
    if (counterId < OS_MOCK_MAX_COUNTERS)
    {
        return Os_MockCounters[counterId].Value;
    }
    return 0;
}

/*==================================================================================================
*                                      SCHEDULE TABLE MOCK FUNCTIONS
==================================================================================================*/
void Os_Mock_ResetSchedules(void)
{
    memset(Os_MockSchedules, 0, sizeof(Os_MockSchedules));
}

void Os_Mock_StartSchedule(uint8 scheduleId)
{
    if (scheduleId < OS_MOCK_MAX_SCHEDULES)
    {
        Os_MockSchedules[scheduleId].State = OS_MOCK_SCHEDULE_RUNNING;
        Os_MockSchedules[scheduleId].Position = 0;
    }
}

void Os_Mock_StopSchedule(uint8 scheduleId)
{
    if (scheduleId < OS_MOCK_MAX_SCHEDULES)
    {
        Os_MockSchedules[scheduleId].State = OS_MOCK_SCHEDULE_STOPPED;
    }
}
