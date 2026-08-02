/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Os, Rte, BswM, Com, Dcm, NvM, Dem
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Os_TaskEntries.c
 * @brief Implementations of the OS task entry points declared in Os_Cfg.c
 * @version 1.0.0
 * @date 2026-08-01
 *
 * These entry points are referenced (extern) by Os_Cfg.c task configuration
 * table but were previously never defined, which made every OS task a
 * NULL entry point and broke the whole scheduling chain at link time.
 */

#include "Os.h"
#include "Os_Internal.h"
#include "Os_Cfg.h"

#include "Rte.h"
#include "Rte_AswScheduler.h"

/* Rte_Scheduler_MainFunction is defined in Rte_Scheduler.c but not exposed
 * through a public header - declare it here for the OS task loop. */
extern void Rte_Scheduler_MainFunction(void);

/*==================================================================================================
*                                   LOCAL HELPERS
*==================================================================================================*/

/**
 * @brief Common startup sequence: RTE init/start + ASW scheduler + BSW alarms
 */
static void Os_TaskInit_StartSystem(void)
{
    /* Start RTE */
    (void)Rte_Init();
    (void)Rte_Start();

    /* Start ASW component scheduler (initializes all SW-Cs, registers
     * their MainFunctions as periodic tasks in Rte_Scheduler) */
    (void)Rte_AswScheduler_Start();

    /* Arm all BSW cyclic alarms so their MainFunctions are dispatched
     * by the OS alarm callbacks (BswM/Com/CanIf 10ms, NvM/Dem 100ms).
     * SetRelAlarm(AlarmID, Increment, Cycle) with Cycle == Increment
     * yields a periodic alarm. */
    (void)SetRelAlarm(OsAlarm_BswM_MainFunction,   OS_ALARM_PERIOD_10MS,  OS_ALARM_PERIOD_10MS);
    (void)SetRelAlarm(OsAlarm_Com_MainFunction,    OS_ALARM_PERIOD_10MS,  OS_ALARM_PERIOD_10MS);
    (void)SetRelAlarm(OsAlarm_CanIf_MainFunction,  OS_ALARM_PERIOD_10MS,  OS_ALARM_PERIOD_10MS);
    (void)SetRelAlarm(OsAlarm_Dcm_MainFunction,    OS_ALARM_PERIOD_10MS,  OS_ALARM_PERIOD_10MS);
    (void)SetRelAlarm(OsAlarm_NvM_MainFunction,    OS_ALARM_PERIOD_100MS, OS_ALARM_PERIOD_100MS);
    (void)SetRelAlarm(OsAlarm_Dem_MainFunction,    OS_ALARM_PERIOD_100MS, OS_ALARM_PERIOD_100MS);
}

/*==================================================================================================
*                                   TASK ENTRY POINTS
*==================================================================================================*/

/**
 * @brief Init task: brings up the full runtime (RTE + ASW + BSW alarms),
 *        then parks forever (the 10ms/50ms/100ms tasks do the periodic work).
 */
void OsTask_Init_Entry(void)
{
    Os_TaskInit_StartSystem();

    /* Signal init complete so other tasks waiting on this event may run */
    (void)SetEvent(OsTask_Diagnostic, OS_EVENT_INIT_COMPLETE);

    for (;;)
    {
        /* Park: periodic work is done by cyclic tasks + alarm callbacks */
        (void)Schedule();
    }
}

/**
 * @brief 10ms cyclic task: drives the RTE/ASW scheduler every 10ms.
 *
 * Rte_Scheduler_MainFunction() dispatches ASW component MainFunctions
 * according to their registered periods (10/20/50/100ms). In the native
 * host build the FreeRTOS Posix port provides real 10ms ticks.
 */
void OsTask_10ms_Entry(void)
{
    for (;;)
    {
        Rte_Scheduler_MainFunction();
        (void)vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

/**
 * @brief 50ms cyclic task: placeholder for slower periodic work.
 */
void OsTask_50ms_Entry(void)
{
    for (;;)
    {
        /* Reserved for 50ms BSW/ASW processing */
        (void)vTaskDelay(pdMS_TO_TICKS(50U));
    }
}

/**
 * @brief 100ms cyclic task: placeholder for slow periodic work.
 */
void OsTask_100ms_Entry(void)
{
    for (;;)
    {
        /* Reserved for 100ms BSW/ASW processing */
        (void)vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

/**
 * @brief Background task: lowest priority, runs when nothing else is ready.
 */
void OsTask_Background_Entry(void)
{
    for (;;)
    {
        /* Idle / background diagnostics */
        (void)vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

/**
 * @brief COM RX task: driven by the CAN message received event.
 */
void OsTask_ComMainFunctionRx_Entry(void)
{
    EventMaskType events = 0U;

    for (;;)
    {
        (void)WaitEvent(OS_EVENT_CAN_MESSAGE_RECEIVED);
        (void)GetEvent(OsTask_ComMainFunctionRx, &events);
        if ((events & OS_EVENT_CAN_MESSAGE_RECEIVED) != 0U)
        {
            /* Com_MainFunctionRx is dispatched via the 10ms alarm;
             * this task handles event-driven RX processing. */
            (void)ClearEvent(OS_EVENT_CAN_MESSAGE_RECEIVED);
        }
    }
}

/**
 * @brief COM TX task: driven by transmit trigger events.
 */
void OsTask_ComMainFunctionTx_Entry(void)
{
    for (;;)
    {
        /* TX processing is driven by the 10ms alarm (Com_MainFunctionTx);
         * keep the task alive for event-driven TX. */
        (void)vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

/**
 * @brief Diagnostic task: handles diagnostic requests/events.
 */
void OsTask_Diagnostic_Entry(void)
{
    EventMaskType events = 0U;

    for (;;)
    {
        (void)WaitEvent(OS_EVENT_INIT_COMPLETE | OS_EVENT_DIAGNOSTIC_REQUEST);
        (void)GetEvent(OsTask_Diagnostic, &events);

        if ((events & OS_EVENT_DIAGNOSTIC_REQUEST) != 0U)
        {
            /* Dcm_MainFunction is dispatched via the 10ms alarm */
            (void)ClearEvent(OS_EVENT_DIAGNOSTIC_REQUEST);
        }

        if ((events & OS_EVENT_INIT_COMPLETE) != 0U)
        {
            /* Init handshake received - stay alive for diagnostics */
            (void)ClearEvent(OS_EVENT_INIT_COMPLETE);
        }
    }
}

#define OS_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
*==================================================================================================*/
