/******************************************************************************
 * @file Os_Cfg.h
 * @brief AutoSAR OS Configuration Header
 * @details Configuration parameters for the AutoSAR OS implementation
 *          based on FreeRTOS adaptation.
 *
 * @author YuleTech
 * @version 1.0.0
 * @date 2026-04-30
 ******************************************************************************/

#ifndef OS_CFG_H
#define OS_CFG_H

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "Std_Types.h"

/*******************************************************************************
 * Version Info
 ******************************************************************************/
#define OS_CFG_SW_MAJOR_VERSION     (1U)
#define OS_CFG_SW_MINOR_VERSION     (0U)
#define OS_CFG_SW_PATCH_VERSION     (0U)

/*******************************************************************************
 * Configuration: General
 ******************************************************************************/

/* OS Status Level */
#define OS_STATUS_LEVEL             (OS_STATUS_EXTENDED)

#define OS_STATUS_STANDARD          (0U)
#define OS_STATUS_EXTENDED          (1U)

/* Hook Configuration */
#define OS_USE_ERROR_HOOK           (STD_ON)
#define OS_USE_PRETASK_HOOK         (STD_ON)
#define OS_USE_POSTTASK_HOOK        (STD_ON)
#define OS_USE_STARTUP_HOOK         (STD_ON)
#define OS_USE_SHUTDOWN_HOOK        (STD_ON)
#define OS_USE_PROTECTION_HOOK      (STD_OFF)
#define OS_USE_IDLE_HOOK            (STD_ON)

/* Scheduler Configuration */
#define OS_SCHEDULER_TYPE           (OS_SCHEDULER_FULL_PREEMPTIVE)

#define OS_SCHEDULER_NON_PREEMPTIVE         (0U)
#define OS_SCHEDULER_FULL_PREEMPTIVE        (1U)
#define OS_SCHEDULER_MIXED_PREEMPTIVE       (2U)

/* Tick Configuration */
#define OS_TICK_MS                  (1U)    /* Tick period in milliseconds */
#define OS_TICKS_PER_SECOND         (1000U / OS_TICK_MS)

/*******************************************************************************
 * Configuration: Task
 ******************************************************************************/
#define OS_TASK_COUNT               (16U)   /* Maximum number of tasks */
#define OS_TASK_MAX_PRIORITY        (32U)   /* Maximum task priority */
#define OS_TASK_NAME_LEN            (32U)   /* Max task name length */

/*******************************************************************************
 * Configuration: Events
 ******************************************************************************/
#define OS_EVENT_COUNT              (32U)   /* Maximum number of events per task */
#define OS_EVENT_MASK_ALL           (0xFFFFFFFFU)

/*******************************************************************************
 * Configuration: Alarm
 ******************************************************************************/
#define OS_ALARM_COUNT              (16U)   /* Maximum number of alarms */
#define OS_COUNTER_COUNT            (8U)    /* Maximum number of counters */

/*******************************************************************************
 * Configuration: Schedule Table
 ******************************************************************************/
#define OS_SCHEDULE_TABLE_COUNT     (8U)    /* Maximum number of schedule tables */
#define OS_SCHEDULE_TABLE_MAX_EXPIRY_POINTS (16U)

/*******************************************************************************
 * Configuration: Resources
 ******************************************************************************/
#define OS_RESOURCE_COUNT           (16U)   /* Maximum number of resources */
#define OS_RESOURCE_MAX_NESTING     (8U)    /* Maximum resource nesting level */

/*******************************************************************************
 * Configuration: ISR
 ******************************************************************************/
#define OS_ISR_COUNT                (16U)   /* Maximum number of ISRs */
#define OS_ISR_MAX_PRIORITY         (255U)

/*******************************************************************************
 * Configuration: Multicore
 ******************************************************************************/
#define OS_MULTICORE_ENABLED        (STD_OFF)
#define OS_CORE_COUNT               (1U)    /* Number of CPU cores */

/*******************************************************************************
 * Configuration: Spinlock (for multicore)
 ******************************************************************************/
#define OS_SPINLOCK_COUNT           (8U)    /* Maximum number of spinlocks */

/*******************************************************************************
 * Configuration: Stack
 ******************************************************************************/
#define OS_STACK_SIZE_TASK_DEFAULT  (1024U) /* Default task stack size in bytes */
#define OS_STACK_SIZE_ISR_DEFAULT   (512U)  /* Default ISR stack size in bytes */
#define OS_STACK_MONITORING         (STD_ON)

/*******************************************************************************
 * Configuration: Timing Protection
 ******************************************************************************/
#define OS_TIMING_PROTECTION        (STD_OFF)
#define OS_EXECUTION_TIME_MONITORING (STD_OFF)

/*******************************************************************************
 * Configuration: Memory Protection
 ******************************************************************************/
#define OS_MEMORY_PROTECTION        (STD_OFF)

/*******************************************************************************
 * Configuration: IOC
 ******************************************************************************/
#define OS_IOC_ENABLED              (STD_OFF)
#define OS_IOC_BUFFER_COUNT         (8U)

/*******************************************************************************
 * Application Mode Configuration
 ******************************************************************************/
typedef uint8 AppModeType;

#define OSDEFAULTAPPMODE            ((AppModeType)0x01U)
#define OSAPPMODE_1                 ((AppModeType)0x02U)
#define OSAPPMODE_2                 ((AppModeType)0x04U)
#define OSAPPMODE_3                 ((AppModeType)0x08U)

/*******************************************************************************
 * Trusted Function Configuration
 ******************************************************************************/
typedef uint16 TrustedFunctionIndexType;
typedef void *TrustedFunctionParameterRefType;

/*******************************************************************************
 * Multi-Core Types
 ******************************************************************************/
typedef uint16 CoreIdType;
#define OS_CORE_ID_0    ((CoreIdType)0U)
#define OS_CORE_ID_1    ((CoreIdType)1U)
#define OS_CORE_ID_2    ((CoreIdType)2U)
#define OS_CORE_ID_3    ((CoreIdType)3U)
#define OS_CORE_ID_MASTER   OS_CORE_ID_0

/*******************************************************************************
 * Tick Type Configuration
 ******************************************************************************/
/* AutoSAR OS uses 32-bit tick type for compatibility */
typedef uint32 TickType;
typedef TickType *TickRefType;

/* Time in microseconds type for timing protection */
typedef uint32 TimeInMicrosecondsType;

/*******************************************************************************
 * Error Codes Configuration
 ******************************************************************************/
/* Extended error checking enabled */
#define OS_USE_GET_SERVICE_ID       (STD_ON)
#define OS_USE_PARAMETER_ACCESS     (STD_ON)

/*******************************************************************************
 * FreeRTOS Specific Configuration
 ******************************************************************************/

/* FreeRTOS adaptation layer settings */
#define OS_FREERTOS_VERSION         ((1006000U)) /* FreeRTOS V10.6.x */
#define OS_FREERTOS_USE_STATIC_ALLOC    (STD_ON)
#define OS_FREERTOS_USE_TRACE_HOOKS     (STD_OFF)
#define OS_FREERTOS_CHECK_FOR_STACK_OVERFLOW (1U)
#define OS_FREERTOS_USE_MUTEXES         (STD_ON)
#define OS_FREERTOS_USE_RECURSIVE_MUTEXES (STD_ON)
#define OS_FREERTOS_QUEUE_REGISTRY_SIZE (16U)

/*******************************************************************************
 * ISR Category Configuration
 ******************************************************************************/
/* Category 1 ISRs - No OS services allowed */
/* Category 2 ISRs - Full OS API available */

/*******************************************************************************
 * Internal Task IDs (Configuration generated)
 * These are defined in Os_Lcfg.c, but declared here for reference
 ******************************************************************************/
/* Example task IDs - actual values generated by configuration tool */
extern const TaskType Task_1ms;
extern const TaskType Task_10ms;
extern const TaskType Task_100ms;
extern const TaskType Task_Background;

/*******************************************************************************
 * Internal Alarm IDs (Configuration generated)
 ******************************************************************************/
extern const AlarmType Alarm_1ms;
extern const AlarmType Alarm_10ms;
extern const AlarmType Alarm_100ms;

/*******************************************************************************
 * Internal Resource IDs (Configuration generated)
 ******************************************************************************/
extern const ResourceType Resource_OsTaskComm;
extern const ResourceType Resource_OsTaskDiag;

/*******************************************************************************
 * Internal Schedule Table IDs (Configuration generated)
 ******************************************************************************/
extern const ScheduleTableType ScheduleTable_Main;

/*******************************************************************************
 * Internal Counter IDs (Configuration generated)
 ******************************************************************************/
extern const CounterType Counter_System;

/*******************************************************************************
 * ISR Prototypes (Configuration generated)
 ******************************************************************************/
/* ISRs are defined in configuration and generated code */

/*******************************************************************************
 * Task Entry Points (User Implementation)
 ******************************************************************************/
/* These functions must be implemented by the user/application */

/* Example task entry points */
extern void Os_Task_1ms(void);
extern void Os_Task_10ms(void);
extern void Os_Task_100ms(void);
extern void Os_Task_Background(void);

/* ISR entry points */
extern void Os_ISR_CanRx(void);
extern void Os_ISR_Timer(void);

/* Alarm callbacks */
extern void Os_AlarmCallback_1ms(void);
extern void Os_AlarmCallback_10ms(void);

/*******************************************************************************
 * OS Configuration Check
 ******************************************************************************/
#if (OS_STATUS_LEVEL != OS_STATUS_STANDARD) && (OS_STATUS_LEVEL != OS_STATUS_EXTENDED)
    #error "Invalid OS_STATUS_LEVEL configuration"
#endif

#if (OS_CORE_COUNT > 1) && (OS_MULTICORE_ENABLED == STD_OFF)
    #error "Multicore configuration mismatch: OS_MULTICORE_ENABLED must be STD_ON for OS_CORE_COUNT > 1"
#endif

#if (OS_TICK_MS == 0)
    #error "OS_TICK_MS must be greater than 0"
#endif

#endif /* OS_CFG_H */
