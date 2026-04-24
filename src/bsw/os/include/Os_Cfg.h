/******************************************************************************
 * @file    Os_Cfg.h
 * @brief   AUTOSAR OS Module Configuration
 *
 * This file contains the configuration parameters for the OS module.
 * 
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef OS_CFG_H
#define OS_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Os_Types.h"

/******************************************************************************
 * OS Configuration - General
 ******************************************************************************/

/* Number of tasks */
#define OS_CFG_NUM_TASKS                    (8U)

/* Number of resources */
#define OS_CFG_NUM_RESOURCES                (4U)

/* Number of alarms */
#define OS_CFG_NUM_ALARMS                   (4U)

/* Number of counters */
#define OS_CFG_NUM_COUNTERS                 (2U)

/* Number of schedule tables */
#define OS_CFG_NUM_SCHEDULE_TABLES          (2U)

/* Number of ISRs */
#define OS_CFG_NUM_ISRS                     (4U)

/* Number of events per task */
#define OS_CFG_NUM_EVENTS                   (8U)

/******************************************************************************
 * OS Configuration - Multi-core
 ******************************************************************************/

/* Number of cores */
#define OS_CFG_NUM_CORES                    (1U)

/* Multi-core support */
#if (OS_CFG_NUM_CORES > 1U)
    #define OS_CFG_MULTICORE_ENABLED        (STD_ON)
#else
    #define OS_CFG_MULTICORE_ENABLED        (STD_OFF)
#endif

/******************************************************************************
 * OS Configuration - Hooks
 ******************************************************************************/

/* Error hook enabled */
#define OS_CFG_USE_ERROR_HOOK               (STD_ON)

/* Pre-task hook enabled */
#define OS_CFG_USE_PRETASK_HOOK             (STD_OFF)

/* Post-task hook enabled */
#define OS_CFG_USE_POSTTASK_HOOK            (STD_OFF)

/* Startup hook enabled */
#define OS_CFG_USE_STARTUP_HOOK             (STD_ON)

/* Shutdown hook enabled */
#define OS_CFG_USE_SHUTDOWN_HOOK            (STD_ON)

/* Protection hook enabled */
#define OS_CFG_USE_PROTECTION_HOOK          (STD_ON)

/******************************************************************************
 * OS Configuration - Stack
 ******************************************************************************/

/* Stack monitoring enabled */
#define OS_CFG_USE_STACK_MONITORING         (STD_ON)

/* Stack size (in bytes) */
#define OS_CFG_STACK_SIZE                   (8192U)

/* ISR stack size */
#define OS_CFG_ISR_STACK_SIZE               (2048U)

/******************************************************************************
 * OS Configuration - Scheduling
 ******************************************************************************/

/* Scheduling policy: FULL (preemptive) or NON (non-preemptive) */
#define OS_CFG_SCHEDULING_POLICY            (FULL)

/* Extended task support */
#define OS_CFG_EXTENDED_TASKS               (STD_ON)

/* Nested resource support */
#define OS_CFG_NESTED_RESOURCES             (STD_ON)

/******************************************************************************
 * OS Configuration - Timing
 ******************************************************************************/

/* System tick frequency (Hz) */
#define OS_CFG_TICK_FREQUENCY               (1000U)

/* System tick period (ms) */
#define OS_CFG_TICK_PERIOD_MS               (1U)

/* Max allowed counter value */
#define OS_CFG_MAX_ALLOWED_VALUE            (0xFFFFFFFFU)

/* Ticks per base */
#define OS_CFG_TICKS_PER_BASE               (1U)

/* Minimum cycle */
#define OS_CFG_MIN_CYCLE                    (1U)

/******************************************************************************
 * OS Configuration - Protection
 ******************************************************************************/

/* Memory protection enabled */
#define OS_CFG_USE_MEMORY_PROTECTION        (STD_OFF)

/* Timing protection enabled */
#define OS_CFG_USE_TIMING_PROTECTION        (STD_OFF)

/******************************************************************************
 * OS Configuration - FreeRTOS Specific
 ******************************************************************************/

#ifdef FREERTOS_USED
    /* FreeRTOS specific configuration */
    #define OS_CFG_FREERTOS_USE_16_BIT_TICKS    (0)
    
    /* Task stack size in words */
    #define OS_CFG_FREERTOS_STACK_SIZE          (256)
    
    /* Maximum task name length */
    #define OS_CFG_FREERTOS_MAX_TASK_NAME_LEN   (16)
    
    /* Use mutexes for resources */
    #define OS_CFG_FREERTOS_USE_MUTEXES         (STD_ON)
    
    /* Use recursive mutexes */
    #define OS_CFG_FREERTOS_USE_RECURSIVE_MUTEXES (STD_OFF)
    
    /* Use counting semaphores */
    #define OS_CFG_FREERTOS_USE_COUNTING_SEMAPHORES (STD_ON)
    
    /* Use queue sets */
    #define OS_CFG_FREERTOS_USE_QUEUE_SETS      (STD_OFF)
#endif

/******************************************************************************
 * Task IDs (generated based on configuration)
 ******************************************************************************/

typedef enum {
    OS_TASK_ID_IDLE = 0,
    OS_TASK_ID_MAIN,
    OS_TASK_ID_1,
    OS_TASK_ID_2,
    OS_TASK_ID_3,
    OS_TASK_ID_4,
    OS_TASK_ID_5,
    OS_TASK_ID_6,
    OS_TASK_COUNT
} Os_TaskIdEnumType;

/******************************************************************************
 * Resource IDs
 ******************************************************************************/

typedef enum {
    OS_RES_ID_INTERNAL = 0,
    OS_RES_ID_1,
    OS_RES_ID_2,
    OS_RES_ID_3,
    OS_RES_COUNT
} Os_ResourceIdEnumType;

/******************************************************************************
 * Alarm IDs
 ******************************************************************************/

typedef enum {
    OS_ALARM_ID_1 = 0,
    OS_ALARM_ID_2,
    OS_ALARM_ID_3,
    OS_ALARM_ID_4,
    OS_ALARM_COUNT
} Os_AlarmIdEnumType;

/******************************************************************************
 * Counter IDs
 ******************************************************************************/

typedef enum {
    OS_COUNTER_ID_SYSTEM = 0,
    OS_COUNTER_ID_1,
    OS_COUNTER_COUNT
} Os_CounterIdEnumType;

/******************************************************************************
 * Schedule Table IDs
 ******************************************************************************/

typedef enum {
    OS_SCHEDTABLE_ID_1 = 0,
    OS_SCHEDTABLE_ID_2,
    OS_SCHEDTABLE_COUNT
} Os_ScheduleTableIdEnumType;

/******************************************************************************
 * ISR IDs
 ******************************************************************************/

typedef enum {
    OS_ISR_ID_1 = 0,
    OS_ISR_ID_2,
    OS_ISR_ID_3,
    OS_ISR_ID_4,
    OS_ISR_COUNT
} Os_IsrIdEnumType;

#ifdef __cplusplus
}
#endif

#endif /* OS_CFG_H */
