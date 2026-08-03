/*==================================================================================================
 * os_test_cfg.c - Test Os_Cfg for QEMU M33 Os-layer verification
 *
 * Replaces the production Os_Cfg.c (which pulls in Rte/Dem/BswM/Com... and
 * the whole BSW stack). This test config defines exactly two tasks:
 *   - OsTask_TestA: 500 ms cyclic (prints "A:<tick>")
 *   - OsTask_TestB: 1000 ms cyclic (prints "B:<tick>:<round>", PASS after 3)
 *
 * Os_GlobalState is a strong definition here, overriding the weak one in
 * Os.c, exactly like the production Os_Cfg.c does. The rest of the AUTOSAR
 * Os wrapper (Os.c) is linked verbatim - that is what we are verifying.
 *================================================================================================*/
#include "Os.h"
#include "Os_Internal.h"
#include "Os_Cfg.h"
#include "MemMap.h"

/* Task entry points (implemented in os_test_main.c) */
extern void OsTask_TestA_Entry(void);
extern void OsTask_TestB_Entry(void);

/*==================================================================================================
 *                                    TASK CONFIGURATION TABLE
 *================================================================================================*/
#define OS_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

static Os_TaskConfigType Os_TestTaskConfigs[2] =
{
    /* Task 0: TestA - 500 ms cyclic */
    {
        /* TaskID            */ 0U,
        /* FreeRTOS_Task     */ NULL_PTR,
        /* FreeRTOS_EventGroup */ NULL_PTR,
        /* Priority          */ OS_TASK_PRIORITY_NORMAL,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_TestA_Entry
    },

    /* Task 1: TestB - 1000 ms cyclic */
    {
        /* TaskID            */ 1U,
        /* FreeRTOS_Task     */ NULL_PTR,
        /* FreeRTOS_EventGroup */ NULL_PTR,
        /* Priority          */ OS_TASK_PRIORITY_NORMAL,
        /* IsAutoStart       */ TRUE,
        /* IsExtended        */ FALSE,
        /* EntryPoint        */ OsTask_TestB_Entry
    }
};

#define OS_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL OS STATE (strong)
==================================================================================================*/
#define OS_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

Os_GlobalStateType Os_GlobalState =
{
    /* IsInitialized   */ FALSE,
    /* IsRunning       */ FALSE,
    /* CurrentAppMode  */ OSDEFAULTAPPMODE,
    /* NumTasks        */ 2U,
    /* NumAlarms       */ 0U,
    /* NumResources    */ 0U,
    /* Tasks           */ Os_TestTaskConfigs,
    /* Alarms          */ NULL_PTR,
    /* Resources       */ NULL_PTR
};

#define OS_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
