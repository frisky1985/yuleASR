/**
 * @file Os_Cfg.h
 * @brief AutoSAR OS Configuration Header
 * @version 1.0.0
 * @date 2026-04-21
 */

#ifndef OS_CFG_H
#define OS_CFG_H

#include "Std_Types.h"

/*==================================================================================================
*                                    GENERAL CONFIGURATION
==================================================================================================*/
#define OS_CFG_DEV_ERROR_DETECT         (STD_ON)
#define OS_CFG_USE_GET_SERVICE_ID       (STD_ON)
#define OS_CFG_USE_PARAMETER_ACCESS     (STD_ON)
#define OS_CFG_PROTECTION_HOOK          (STD_OFF)
#define OS_CFG_STACK_MONITORING         (STD_ON)
#define OS_CFG_STARTUP_HOOK             (STD_ON)
#define OS_CFG_SHUTDOWN_HOOK            (STD_ON)
#define OS_CFG_ERROR_HOOK               (STD_ON)
#define OS_CFG_PRETASK_HOOK             (STD_OFF)
#define OS_CFG_POSTTASK_HOOK            (STD_OFF)

/*==================================================================================================
*                                    TASK CONFIGURATION
==================================================================================================*/
#define OS_CFG_NUM_TASKS                (8U)

/* Task IDs */
#define OsTask_Init                     (0U)
#define OsTask_10ms                     (1U)
#define OsTask_50ms                     (2U)
#define OsTask_100ms                    (3U)
#define OsTask_Background               (4U)
#define OsTask_ComMainFunctionRx        (5U)
#define OsTask_ComMainFunctionTx        (6U)
#define OsTask_Diagnostic               (7U)

/* Task priorities (higher number = higher priority) */
#define OS_TASK_PRIORITY_IDLE           (1U)
#define OS_TASK_PRIORITY_LOW            (2U)
#define OS_TASK_PRIORITY_NORMAL         (3U)
#define OS_TASK_PRIORITY_HIGH           (4U)
#define OS_TASK_PRIORITY_CRITICAL       (5U)

/*==================================================================================================
*                                    ALARM CONFIGURATION
==================================================================================================*/
#define OS_CFG_NUM_ALARMS               (6U)

/* Alarm IDs */
#define OsAlarm_10ms                    (0U)
#define OsAlarm_50ms                    (1U)
#define OsAlarm_100ms                   (2U)
#define OsAlarm_ComRx                   (3U)
#define OsAlarm_ComTx                   (4U)
#define OsAlarm_Diagnostic              (5U)

/*==================================================================================================
*                                    RESOURCE CONFIGURATION
==================================================================================================*/
#define OS_CFG_NUM_RESOURCES            (4U)

/* Resource IDs */
#define OsRes_CanController             (0U)
#define OsRes_NvMBlock                  (1U)
#define OsRes_ComBuffer                 (2U)
#define OsRes_Diagnostic                (3U)

/*==================================================================================================
*                                    EVENT CONFIGURATION
==================================================================================================*/
#define OS_EVENT_INIT_COMPLETE          ((EventMaskType)0x00000001U)
#define OS_EVENT_CAN_MESSAGE_RECEIVED   ((EventMaskType)0x00000002U)
#define OS_EVENT_NVM_WRITE_COMPLETE     ((EventMaskType)0x00000004U)
#define OS_EVENT_DIAGNOSTIC_REQUEST     ((EventMaskType)0x00000008U)
#define OS_EVENT_MODE_SWITCH            ((EventMaskType)0x00000010U)

/*==================================================================================================
*                                    COUNTER CONFIGURATION
==================================================================================================*/
#define OS_CFG_MAIN_COUNTER_TICKS_PER_MS  (1U)
#define OS_CFG_MAIN_COUNTER_MAX_ALLOWED   (0xFFFFFFFFU)
#define OS_CFG_MAIN_COUNTER_MIN_CYCLE     (1U)

/*==================================================================================================
*                                    STACK CONFIGURATION
==================================================================================================*/
#define OS_CFG_TASK_STACK_SIZE          (4096U)
#define OS_CFG_ISR_STACK_SIZE           (2048U)

#endif /* OS_CFG_H */
