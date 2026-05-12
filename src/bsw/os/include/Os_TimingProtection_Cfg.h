/**
 * @file Os_TimingProtection_Cfg.h
 * @brief OS Timing Protection Configuration
 */

#ifndef OS_TIMINGPROTECTION_CFG_H
#define OS_TIMINGPROTECTION_CFG_H

#include "Os.h"

/* Timing Protection Enabled */
#define OS_TIMING_PROTECTION_ENABLED        STD_ON

/* Execution Time Protection */
#define OS_EXECUTION_TIME_PROTECTION        STD_ON
#define OS_MAX_TASK_EXECUTION_TIME_MS       100U    /* Max 100ms per task */
#define OS_MAX_ISR_EXECUTION_TIME_MS        50U     /* Max 50ms per ISR */

/* Arrival Time Protection */
#define OS_ARRIVAL_TIME_PROTECTION          STD_ON
#define OS_MAX_TASK_ARRIVAL_INTERVAL_MS     10U     /* Min 10ms between activations */

/* Resource Locking Time Protection */
#define OS_RESOURCE_LOCK_PROTECTION         STD_ON
#define OS_MAX_RESOURCE_LOCK_TIME_MS        50U     /* Max 50ms holding resource */

/* Interrupt Locking Time Protection */
#define OS_INTERRUPT_LOCK_PROTECTION        STD_ON
#define OS_MAX_ALL_INT_LOCK_TIME_MS         10U     /* Max 10ms all interrupts disabled */
#define OS_MAX_OS_INT_LOCK_TIME_MS          50U     /* Max 50ms OS interrupts disabled */

/* Timing Fault Actions */
typedef enum
{
    OS_TIMING_ACTION_NONE = 0,
    OS_TIMING_ACTION_TASK_KILL,
    OS_TIMING_ACTION_TASK_RESTART,
    OS_TIMING_ACTION_ERROR_HOOK,
    OS_TIMING_ACTION_PROTECTION_HOOK
} Os_TimingFaultActionType;

/* Timing Budget Configuration */
typedef struct
{
    TaskType                    TaskId;
    uint32                      ExecutionTimeBudget;    /* In microseconds */
    uint32                      ArrivalTimeMin;         /* Min interval in microseconds */
    Os_TimingFaultActionType    FaultAction;
} Os_TaskTimingBudgetType;

typedef struct
{
    ResourceType                ResourceId;
    uint32                      LockTimeBudget;         /* Max lock time in microseconds */
    Os_TimingFaultActionType    FaultAction;
} Os_ResourceTimingBudgetType;

typedef struct
{
    uint32                      AllIntLockBudget;
    uint32                      OsIntLockBudget;
    Os_TimingFaultActionType    FaultAction;
} Os_InterruptTimingBudgetType;

/* External configuration */
extern const Os_TaskTimingBudgetType Os_TaskTimingBudgets[OS_TASK_COUNT];
extern const Os_ResourceTimingBudgetType Os_ResourceTimingBudgets[OS_RESOURCE_COUNT];
extern const Os_InterruptTimingBudgetType Os_InterruptTimingBudget;

#endif /* OS_TIMINGPROTECTION_CFG_H */
