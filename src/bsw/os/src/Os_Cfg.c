/**
 * @file Os_Cfg.c
 * @brief AutoSAR OS Configuration - Task and Resource Tables
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Operating System (OS)
 * Layer: OS Layer
 * Target: FreeRTOS V10.6.x / V11.x
 */

/*==================================================================================================
*                                         INCLUDE FILES
==================================================================================================*/
#include "Os.h"
#include "Os_Internal.h"
#include "Os_Cfg.h"
#include "Compiler.h"
#include "MemMap.h"

/* BSW MainFunction headers */
#include "BswM.h"
#include "Com.h"
#include "CanIf.h"
#include "Dcm.h"
#include "NvM.h"
#include "Dem.h"

/*==================================================================================================
*                                    TASK ENTRY POINT DECLARATIONS
==================================================================================================*/
extern void OsTask_Init_Entry(void);
extern void OsTask_10ms_Entry(void);
extern void OsTask_50ms_Entry(void);
extern void OsTask_100ms_Entry(void);
extern void OsTask_Background_Entry(void);
extern void OsTask_ComMainFunctionRx_Entry(void);
extern void OsTask_ComMainFunctionTx_Entry(void);
extern void OsTask_Diagnostic_Entry(void);

/*==================================================================================================
*                                    ALARM CALLBACK DECLARATIONS
==================================================================================================*/
STATIC void OsAlarm_BswM_MainFunction_Callback(void);
STATIC void OsAlarm_Com_MainFunction_Callback(void);
STATIC void OsAlarm_CanIf_MainFunction_Callback(void);
STATIC void OsAlarm_Dcm_MainFunction_Callback(void);
STATIC void OsAlarm_NvM_MainFunction_Callback(void);
STATIC void OsAlarm_Dem_MainFunction_Callback(void);

/*==================================================================================================
*                                    TASK CONFIGURATION TABLE
==================================================================================================*/
#define OS_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

STATIC Os_TaskConfigType Os_TaskConfigs[OS_CFG_NUM_TASKS] =
{
    /* Task 0: Init Task */
    {
        /* TaskID            */ OsTask_Init,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_CRITICAL,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_Init_Entry
    },

    /* Task 1: 10ms Cyclic Task */
    {
        /* TaskID            */ OsTask_10ms,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_HIGH,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_10ms_Entry
    },

    /* Task 2: 50ms Cyclic Task */
    {
        /* TaskID            */ OsTask_50ms,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_NORMAL,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_50ms_Entry
    },

    /* Task 3: 100ms Cyclic Task */
    {
        /* TaskID            */ OsTask_100ms,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_LOW,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_100ms_Entry
    },

    /* Task 4: Background Task */
    {
        /* TaskID            */ OsTask_Background,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_IDLE,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_Background_Entry
    },

    /* Task 5: COM MainFunctionRx Task */
    {
        /* TaskID            */ OsTask_ComMainFunctionRx,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_NORMAL,
        /* IsAutoStart       */ FALSE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_ComMainFunctionRx_Entry
    },

    /* Task 6: COM MainFunctionTx Task */
    {
        /* TaskID            */ OsTask_ComMainFunctionTx,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_NORMAL,
        /* IsAutoStart       */ FALSE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_ComMainFunctionTx_Entry
    },

    /* Task 7: Diagnostic Task */
    {
        /* TaskID            */ OsTask_Diagnostic,
        /* FreeRTOS_Task     */ NULL,
        /* FreeRTOS_EventGroup */ NULL,
        /* Priority          */ OS_TASK_PRIORITY_HIGH,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ TRUE,
        /* EntryPoint        */ OsTask_Diagnostic_Entry
    }
};

#define OS_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    ALARM CONFIGURATION TABLE
==================================================================================================*/
#define OS_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

STATIC Os_AlarmConfigType Os_AlarmConfigs[OS_CFG_NUM_ALARMS] =
{
    /* Alarm 0: BswM_MainFunction - 10ms */
    {
        /* AlarmID           */ OsAlarm_BswM_MainFunction,
        /* Increment         */ OS_ALARM_PERIOD_10MS,
        /* Cycle             */ OS_ALARM_PERIOD_10MS,
        /* ExpiryTick        */ 0U,
        /* State             */ OS_ALARM_UNUSED,
        /* Callback          */ OsAlarm_BswM_MainFunction_Callback,
        /* FreeRTOS_Timer    */ NULL
    },

    /* Alarm 1: Com_MainFunctionRx/Tx - 10ms */
    {
        /* AlarmID           */ OsAlarm_Com_MainFunction,
        /* Increment         */ OS_ALARM_PERIOD_10MS,
        /* Cycle             */ OS_ALARM_PERIOD_10MS,
        /* ExpiryTick        */ 0U,
        /* State             */ OS_ALARM_UNUSED,
        /* Callback          */ OsAlarm_Com_MainFunction_Callback,
        /* FreeRTOS_Timer    */ NULL
    },

    /* Alarm 2: CanIf_MainFunction - 10ms */
    {
        /* AlarmID           */ OsAlarm_CanIf_MainFunction,
        /* Increment         */ OS_ALARM_PERIOD_10MS,
        /* Cycle             */ OS_ALARM_PERIOD_10MS,
        /* ExpiryTick        */ 0U,
        /* State             */ OS_ALARM_UNUSED,
        /* Callback          */ OsAlarm_CanIf_MainFunction_Callback,
        /* FreeRTOS_Timer    */ NULL
    },

    /* Alarm 3: Dcm_MainFunction - 10ms */
    {
        /* AlarmID           */ OsAlarm_Dcm_MainFunction,
        /* Increment         */ OS_ALARM_PERIOD_10MS,
        /* Cycle             */ OS_ALARM_PERIOD_10MS,
        /* ExpiryTick        */ 0U,
        /* State             */ OS_ALARM_UNUSED,
        /* Callback          */ OsAlarm_Dcm_MainFunction_Callback,
        /* FreeRTOS_Timer    */ NULL
    },

    /* Alarm 4: NvM_MainFunction - 100ms */
    {
        /* AlarmID           */ OsAlarm_NvM_MainFunction,
        /* Increment         */ OS_ALARM_PERIOD_100MS,
        /* Cycle             */ OS_ALARM_PERIOD_100MS,
        /* ExpiryTick        */ 0U,
        /* State             */ OS_ALARM_UNUSED,
        /* Callback          */ OsAlarm_NvM_MainFunction_Callback,
        /* FreeRTOS_Timer    */ NULL
    },

    /* Alarm 5: Dem_MainFunction - 100ms */
    {
        /* AlarmID           */ OsAlarm_Dem_MainFunction,
        /* Increment         */ OS_ALARM_PERIOD_100MS,
        /* Cycle             */ OS_ALARM_PERIOD_100MS,
        /* ExpiryTick        */ 0U,
        /* State             */ OS_ALARM_UNUSED,
        /* Callback          */ OsAlarm_Dem_MainFunction_Callback,
        /* FreeRTOS_Timer    */ NULL
    }
};

#define OS_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    RESOURCE CONFIGURATION TABLE
==================================================================================================*/
#define OS_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

STATIC Os_ResourceConfigType Os_ResourceConfigs[OS_CFG_NUM_RESOURCES] =
{
    /* Resource 0: CAN Controller */
    {
        /* ResID             */ OsRes_CanController,
        /* FreeRTOS_Mutex    */ NULL,
        /* OwnerTask         */ 0U,
        /* NestCount         */ 0U,
        /* IsCeilingPriority */ FALSE,
        /* CeilingPriority   */ 0U
    },

    /* Resource 1: NvM Block */
    {
        /* ResID             */ OsRes_NvMBlock,
        /* FreeRTOS_Mutex    */ NULL,
        /* OwnerTask         */ 0U,
        /* NestCount         */ 0U,
        /* IsCeilingPriority */ FALSE,
        /* CeilingPriority   */ 0U
    },

    /* Resource 2: COM Buffer */
    {
        /* ResID             */ OsRes_ComBuffer,
        /* FreeRTOS_Mutex    */ NULL,
        /* OwnerTask         */ 0U,
        /* NestCount         */ 0U,
        /* IsCeilingPriority */ FALSE,
        /* CeilingPriority   */ 0U
    },

    /* Resource 3: Diagnostic */
    {
        /* ResID             */ OsRes_Diagnostic,
        /* FreeRTOS_Mutex    */ NULL,
        /* OwnerTask         */ 0U,
        /* NestCount         */ 0U,
        /* IsCeilingPriority */ FALSE,
        /* CeilingPriority   */ 0U
    }
};

#define OS_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL STATE INITIALIZATION
==================================================================================================*/
#define OS_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

Os_GlobalStateType Os_GlobalState =
{
    /* IsRunning         */ FALSE,
    /* CurrentAppMode    */ OSDEFAULTAPPMODE,
    /* NumTasks          */ OS_CFG_NUM_TASKS,
    /* NumAlarms         */ OS_CFG_NUM_ALARMS,
    /* NumResources      */ OS_CFG_NUM_RESOURCES,
    /* Tasks             */ Os_TaskConfigs,
    /* Alarms            */ Os_AlarmConfigs,
    /* Resources         */ Os_ResourceConfigs
};

#define OS_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    ALARM CALLBACK FUNCTIONS
==================================================================================================*/
#define OS_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Unified alarm callback dispatcher
 *
 * Called by the OS alarm callback wrapper. Dispatches to the appropriate
 * BSW MainFunction based on the alarm ID.
 *
 * @param[in] AlarmID ID of the expired alarm
 */
void Os_Callback_Alarm(AlarmType AlarmID)
{
    switch (AlarmID)
    {
        case OsAlarm_BswM_MainFunction:
            BswM_MainFunction();
            break;

        case OsAlarm_Com_MainFunction:
            Com_MainFunctionRx();
            Com_MainFunctionTx();
            break;

        case OsAlarm_CanIf_MainFunction:
            /* CanIf_MainFunction not implemented in this project */
            break;

        case OsAlarm_Dcm_MainFunction:
            Dcm_MainFunction();
            break;

        case OsAlarm_NvM_MainFunction:
            NvM_MainFunction();
            break;

        case OsAlarm_Dem_MainFunction:
            Dem_MainFunction();
            break;

        default:
            /* Unknown alarm ID - no action */
            break;
    }
}

/**
 * @brief Alarm callback for BswM_MainFunction (10ms)
 */
STATIC void OsAlarm_BswM_MainFunction_Callback(void)
{
    Os_Callback_Alarm(OsAlarm_BswM_MainFunction);
}

/**
 * @brief Alarm callback for Com_MainFunction (10ms)
 */
STATIC void OsAlarm_Com_MainFunction_Callback(void)
{
    Os_Callback_Alarm(OsAlarm_Com_MainFunction);
}

/**
 * @brief Alarm callback for CanIf_MainFunction (10ms)
 */
STATIC void OsAlarm_CanIf_MainFunction_Callback(void)
{
    Os_Callback_Alarm(OsAlarm_CanIf_MainFunction);
}

/**
 * @brief Alarm callback for Dcm_MainFunction (10ms)
 */
STATIC void OsAlarm_Dcm_MainFunction_Callback(void)
{
    Os_Callback_Alarm(OsAlarm_Dcm_MainFunction);
}

/**
 * @brief Alarm callback for NvM_MainFunction (100ms)
 */
STATIC void OsAlarm_NvM_MainFunction_Callback(void)
{
    Os_Callback_Alarm(OsAlarm_NvM_MainFunction);
}

/**
 * @brief Alarm callback for Dem_MainFunction (100ms)
 */
STATIC void OsAlarm_Dem_MainFunction_Callback(void)
{
    Os_Callback_Alarm(OsAlarm_Dem_MainFunction);
}

#define OS_STOP_SEC_CODE
#include "MemMap.h"
