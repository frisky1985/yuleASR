/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Os_TimingProtection.c
 * @brief OS Timing Protection Implementation
 * @details Implements execution time, arrival time, and resource locking protection
 */

#include "Os.h"
#include "Os_TimingProtection_Cfg.h"
#include "FreeRTOS.h"
#include "task.h"

#if (OS_TIMING_PROTECTION_ENABLED == STD_ON)

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define OS_TIMING_GET_US()      ((uint32)(xTaskGetTickCount() * (1000000U / configTICK_RATE_HZ)))

/*******************************************************************************
 * Local Types
 ******************************************************************************/
typedef struct
{
    uint32              StartTime;
    uint32              LastArrivalTime;
    boolean             IsRunning;
    boolean             BudgetExceeded;
} Os_TimingMonitorType;

typedef struct
{
    uint32              LockStartTime;
    uint32              LockDuration;
    boolean             IsLocked;
} Os_ResourceTimingMonitorType;

typedef struct
{
    uint32              AllIntLockStart;
    uint32              OsIntLockStart;
    boolean             AllIntLocked;
    boolean             OsIntLocked;
} Os_InterruptTimingMonitorType;

/*******************************************************************************
 * Local Variables
 ******************************************************************************/
static Os_TimingMonitorType Os_TaskMonitors[OS_TASK_COUNT];
static Os_ResourceTimingMonitorType Os_ResourceMonitors[OS_RESOURCE_COUNT];
static Os_InterruptTimingMonitorType Os_InterruptMonitor;

/*******************************************************************************
 * Local Functions
 ******************************************************************************/
static void Os_CheckTimingFault(TaskType TaskID, Os_TimingFaultActionType Action);
static void Os_CheckResourceTimingFault(ResourceType ResID);
static void Os_CheckInterruptTimingFault(void);

/*******************************************************************************
 * Task Timing Protection
 ******************************************************************************/

void Os_StartTaskExecutionTiming(TaskType TaskID)
{
    #if (OS_EXECUTION_TIME_PROTECTION == STD_ON)
    if (TaskID < OS_TASK_COUNT)
    {
        Os_TaskMonitors[TaskID].StartTime = OS_TIMING_GET_US();
        Os_TaskMonitors[TaskID].IsRunning = TRUE;
        Os_TaskMonitors[TaskID].BudgetExceeded = FALSE;
    }
    #endif
}

void Os_StopTaskExecutionTiming(TaskType TaskID)
{
    #if (OS_EXECUTION_TIME_PROTECTION == STD_ON)
    if (TaskID < OS_TASK_COUNT)
    {
        Os_TaskMonitors[TaskID].IsRunning = FALSE;
    }
    #endif
}

void Os_CheckTaskExecutionBudget(TaskType TaskID)
{
    #if (OS_EXECUTION_TIME_PROTECTION == STD_ON)
    uint32 ElapsedTime;
    
    if ((TaskID < OS_TASK_COUNT) && Os_TaskMonitors[TaskID].IsRunning)
    {
        ElapsedTime = OS_TIMING_GET_US() - Os_TaskMonitors[TaskID].StartTime;
        
        if (ElapsedTime > Os_TaskTimingBudgets[TaskID].ExecutionTimeBudget)
        {
            Os_TaskMonitors[TaskID].BudgetExceeded = TRUE;
            Os_CheckTimingFault(TaskID, Os_TaskTimingBudgets[TaskID].FaultAction);
        }
    }
    #endif
}

/*******************************************************************************
 * Arrival Time Protection
 ******************************************************************************/

boolean Os_CheckTaskArrivalTime(TaskType TaskID)
{
    #if (OS_ARRIVAL_TIME_PROTECTION == STD_ON)
    uint32 CurrentTime;
    uint32 TimeSinceLastArrival;
    
    if (TaskID >= OS_TASK_COUNT)
    {
        return TRUE; /* Allow by default */
    }
    
    CurrentTime = OS_TIMING_GET_US();
    
    if (Os_TaskMonitors[TaskID].LastArrivalTime > 0U)
    {
        TimeSinceLastArrival = CurrentTime - Os_TaskMonitors[TaskID].LastArrivalTime;
        
        if (TimeSinceLastArrival < Os_TaskTimingBudgets[TaskID].ArrivalTimeMin)
        {
            /* Arrived too early - reject activation */
            return FALSE;
        }
    }
    
    Os_TaskMonitors[TaskID].LastArrivalTime = CurrentTime;
    return TRUE;
    #else
    (void)TaskID;
    return TRUE;
    #endif
}

/*******************************************************************************
 * Resource Timing Protection
 ******************************************************************************/

void Os_StartResourceTiming(ResourceType ResID)
{
    #if (OS_RESOURCE_LOCK_PROTECTION == STD_ON)
    if (ResID < OS_RESOURCE_COUNT)
    {
        Os_ResourceMonitors[ResID].LockStartTime = OS_TIMING_GET_US();
        Os_ResourceMonitors[ResID].IsLocked = TRUE;
    }
    #endif
}

void Os_StopResourceTiming(ResourceType ResID)
{
    #if (OS_RESOURCE_LOCK_PROTECTION == STD_ON)
    if (ResID < OS_RESOURCE_COUNT)
    {
        Os_ResourceMonitors[ResID].IsLocked = FALSE;
    }
    #endif
}

void Os_CheckResourceLockBudget(ResourceType ResID)
{
    #if (OS_RESOURCE_LOCK_PROTECTION == STD_ON)
    uint32 LockDuration;
    
    if ((ResID < OS_RESOURCE_COUNT) && Os_ResourceMonitors[ResID].IsLocked)
    {
        LockDuration = OS_TIMING_GET_US() - Os_ResourceMonitors[ResID].LockStartTime;
        
        if (LockDuration > Os_ResourceTimingBudgets[ResID].LockTimeBudget)
        {
            Os_CheckResourceTimingFault(ResID);
        }
    }
    #endif
}

/*******************************************************************************
 * Interrupt Timing Protection
 ******************************************************************************/

void Os_StartAllIntTiming(void)
{
    #if (OS_INTERRUPT_LOCK_PROTECTION == STD_ON)
    Os_InterruptMonitor.AllIntLockStart = OS_TIMING_GET_US();
    Os_InterruptMonitor.AllIntLocked = TRUE;
    #endif
}

void Os_StopAllIntTiming(void)
{
    #if (OS_INTERRUPT_LOCK_PROTECTION == STD_ON)
    Os_InterruptMonitor.AllIntLocked = FALSE;
    #endif
}

void Os_StartOsIntTiming(void)
{
    #if (OS_INTERRUPT_LOCK_PROTECTION == STD_ON)
    Os_InterruptMonitor.OsIntLockStart = OS_TIMING_GET_US();
    Os_InterruptMonitor.OsIntLocked = TRUE;
    #endif
}

void Os_StopOsIntTiming(void)
{
    #if (OS_INTERRUPT_LOCK_PROTECTION == STD_ON)
    Os_InterruptMonitor.OsIntLocked = FALSE;
    #endif
}

void Os_CheckInterruptLockBudgets(void)
{
    #if (OS_INTERRUPT_LOCK_PROTECTION == STD_ON)
    uint32 LockDuration;
    
    /* Check all interrupt lock */
    if (Os_InterruptMonitor.AllIntLocked)
    {
        LockDuration = OS_TIMING_GET_US() - Os_InterruptMonitor.AllIntLockStart;
        
        if (LockDuration > Os_InterruptTimingBudget.AllIntLockBudget)
        {
            Os_CheckInterruptTimingFault();
        }
    }
    
    /* Check OS interrupt lock */
    if (Os_InterruptMonitor.OsIntLocked)
    {
        LockDuration = OS_TIMING_GET_US() - Os_InterruptMonitor.OsIntLockStart;
        
        if (LockDuration > Os_InterruptTimingBudget.OsIntLockBudget)
        {
            Os_CheckInterruptTimingFault();
        }
    }
    #endif
}

/*******************************************************************************
 * Fault Handling
 ******************************************************************************/

static void Os_CheckTimingFault(TaskType TaskID, Os_TimingFaultActionType Action)
{
    switch (Action)
    {
        case OS_TIMING_ACTION_TASK_KILL:
            Os_TerminateTask();
            break;
            
        case OS_TIMING_ACTION_TASK_RESTART:
            /* Restart task - implementation specific */
            break;
            
        case OS_TIMING_ACTION_ERROR_HOOK:
            Os_ErrorHook(E_OS_PROTECTION_TIME);
            break;
            
        case OS_TIMING_ACTION_PROTECTION_HOOK:
            Os_ProtectionHook(E_OS_PROTECTION_TIME);
            break;
            
        default:
            break;
    }
}

static void Os_CheckResourceTimingFault(ResourceType ResID)
{
    (void)ResID;
    Os_ProtectionHook(E_OS_PROTECTION_LOCKED);
}

static void Os_CheckInterruptTimingFault(void)
{
    Os_ProtectionHook(E_OS_PROTECTION_LOCKED);
}

/*******************************************************************************
 * Main Function (called periodically)
 ******************************************************************************/

void Os_TimingProtectionMainFunction(void)
{
    uint8 i;
    
    #if (OS_EXECUTION_TIME_PROTECTION == STD_ON)
    for (i = 0U; i < OS_TASK_COUNT; i++)
    {
        Os_CheckTaskExecutionBudget(i);
    }
    #endif
    
    #if (OS_RESOURCE_LOCK_PROTECTION == STD_ON)
    for (i = 0U; i < OS_RESOURCE_COUNT; i++)
    {
        Os_CheckResourceLockBudget(i);
    }
    #endif
    
    #if (OS_INTERRUPT_LOCK_PROTECTION == STD_ON)
    Os_CheckInterruptLockBudgets();
    #endif
}

#endif /* OS_TIMING_PROTECTION_ENABLED */
