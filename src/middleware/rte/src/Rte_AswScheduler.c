/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Rte, Rte_Scheduler, ASW Components
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-26
* Author               : AI Agent (RTE Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/
/* @req SHALL_RTE */


/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Rte_AswScheduler.h"
#include "Rte_Swc.h"
#include "Rte.h"
#include "Det.h"

/* Include all ASW component headers */
#include "Swc_EngineControl.h"
#include "Swc_VehicleDynamics.h"
#include "Swc_DiagnosticManager.h"
#include "Swc_CommunicationManager.h"
#include "Swc_StorageManager.h"
#include "Swc_IOControl.h"
#include "Swc_ModeManager.h"
#include "Swc_WatchdogManager.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define RTE_ASWSCHEDULER_INSTANCE_ID    (0x00U)
#define RTE_ASWSCHEDULER_MODULE_ID      (0x90U)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct {
    boolean                     isRunning;
    Rte_AswComponentStateType   componentStates[SWC_ID_COUNT];
} Rte_AswSchedulerStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Rte_AswSchedulerStateType Rte_AswSchedulerState = {
    .isRunning = FALSE,
    .componentStates = {
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED,
        ASW_STATE_UNINITIALIZED
    }
};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  COMPONENT REGISTRATION TABLE
*
* Each component entry provides:
*   - Init / Deinit / MainFunction lifecycle hooks
*   - Scheduling period and priority
*   - isMandatory flag (critical components)
*
* Each component's own MainFunction (defined in its respective Swc_Xxx.c)
* handles multi-rate dispatch internally. ModeManager has mode-switch logic
* and WatchdogManager has heartbeat mechanism.
==================================================================================================*/

/* Component registration table - single source of truth */
STATIC const Rte_AswComponentEntryType Rte_AswComponentTable[SWC_ID_COUNT] = {
    {
        .componentId   = SWC_ID_ENGINE_CONTROL,
        .componentName = "EngineControl",
        .Init          = Swc_EngineControl_Init,
        .Deinit        = Swc_EngineControl_Deinit,
        .MainFunction  = Swc_EngineControl_MainFunction,
        .periodMs      = 10U,
        .priority      = 1U,
        .isMandatory   = TRUE
    },
    {
        .componentId   = SWC_ID_VEHICLE_DYNAMICS,
        .componentName = "VehicleDynamics",
        .Init          = Swc_VehicleDynamics_Init,
        .Deinit        = Swc_VehicleDynamics_Deinit,
        .MainFunction  = Swc_VehicleDynamics_MainFunction,
        .periodMs      = 10U,
        .priority      = 2U,
        .isMandatory   = TRUE
    },
    {
        .componentId   = SWC_ID_DIAGNOSTIC_MANAGER,
        .componentName = "DiagnosticManager",
        .Init          = Swc_DiagnosticManager_Init,
        .Deinit        = Swc_DiagnosticManager_Deinit,
        .MainFunction  = Swc_DiagnosticManager_MainFunction,
        .periodMs      = 10U,
        .priority      = 4U,
        .isMandatory   = FALSE
    },
    {
        .componentId   = SWC_ID_COMMUNICATION_MANAGER,
        .componentName = "CommunicationManager",
        .Init          = Swc_CommunicationManager_Init,
        .Deinit        = Swc_CommunicationManager_Deinit,
        .MainFunction  = Swc_CommunicationManager_MainFunction,
        .periodMs      = 10U,
        .priority      = 3U,
        .isMandatory   = TRUE
    },
    {
        .componentId   = SWC_ID_STORAGE_MANAGER,
        .componentName = "StorageManager",
        .Init          = Swc_StorageManager_Init,
        .Deinit        = Swc_StorageManager_Deinit,
        .MainFunction  = Swc_StorageManager_MainFunction,
        .periodMs      = 10U,
        .priority      = 6U,
        .isMandatory   = FALSE
    },
    {
        .componentId   = SWC_ID_IO_CONTROL,
        .componentName = "IOControl",
        .Init          = Swc_IOControl_Init,
        .Deinit        = Swc_IOControl_Deinit,
        .MainFunction  = Swc_IOControl_MainFunction,
        .periodMs      = 10U,
        .priority      = 5U,
        .isMandatory   = FALSE
    },
    {
        .componentId   = SWC_ID_MODE_MANAGER,
        .componentName = "ModeManager",
        .Init          = Swc_ModeManager_Init,
        .Deinit        = Swc_ModeManager_Deinit,
        .MainFunction  = Swc_ModeManager_MainFunction,
        .periodMs      = 10U,
        .priority      = 0U,   /* Highest priority - mode changes affect all */
        .isMandatory   = TRUE
    },
    {
        .componentId   = SWC_ID_WATCHDOG_MANAGER,
        .componentName = "WatchdogManager",
        .Init          = Swc_WatchdogManager_Init,
        .Deinit        = Swc_WatchdogManager_Deinit,
        .MainFunction  = Swc_WatchdogManager_MainFunction,
        .periodMs      = 10U,
        .priority      = 7U,   /* Lowest priority - runs after all checks */
        .isMandatory   = TRUE
    }
};

/*==================================================================================================
*                                  EXTERNAL SCHEDULER API DECLARATIONS
*
* These functions are implemented in Rte_Scheduler.c and called by the ASW scheduler
* to register components as periodic tasks.
==================================================================================================*/
extern void Rte_Scheduler_Init(void);
extern void Rte_Scheduler_Start(void);
extern void Rte_Scheduler_Stop(void);
extern Std_ReturnType Rte_SchedulerCreateTask(uint8 taskId, uint8 priority,
                                              uint32 periodMs,
                                              void (*entryPoint)(void));

/*==================================================================================================
*                                  GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Starts the ASW scheduler: initializes all components and registers them
 *        as periodic tasks in the Rte_Scheduler.
 */
Rte_StatusType Rte_AswScheduler_Start(void)
{
    Rte_StatusType result = RTE_E_OK;
    uint8 i;

    if (Rte_AswSchedulerState.isRunning)
    {
        return RTE_E_OK;
    }

    /* Initialize the underlying scheduler first */
    Rte_Scheduler_Init();

    /* Initialize all components in order of priority (lowest ID first) */
    for (i = 0U; i < (uint32_t)(SWC_ID_COUNT); i++)
    {
        if (Rte_AswComponentTable[i].Init != NULL_PTR)
        {
            Rte_AswComponentTable[i].Init();
            Rte_AswSchedulerState.componentStates[i] = ASW_STATE_INITIALIZED;

            Det_ReportError(RTE_ASWSCHEDULER_MODULE_ID,
                            RTE_ASWSCHEDULER_INSTANCE_ID,
                            (uint8)(i + 1U),
                            RTE_E_OK);
        }
        else
        {
            Rte_AswSchedulerState.componentStates[i] = ASW_STATE_ERROR;
            result = RTE_E_NOK;
        }
    }

    /* Register MainFunction with external scheduler (Rte_SchedulerCreateTask) */
    for (i = 0U; i < (uint32_t)(SWC_ID_COUNT); i++)
    {
        if (Rte_AswComponentTable[i].MainFunction != NULL_PTR)
        {
            /* Register as periodic task in Rte_Scheduler */
            Std_ReturnType taskResult = Rte_SchedulerCreateTask(
                i,                                    /* taskId = componentId */
                Rte_AswComponentTable[i].priority,    /* priority */
                Rte_AswComponentTable[i].periodMs,    /* periodMs */
                Rte_AswComponentTable[i].MainFunction /* entryPoint */
            );

            if (taskResult == E_OK)
            {
                Rte_AswSchedulerState.componentStates[i] = ASW_STATE_RUNNING;
            }
            else
            {
                Rte_AswSchedulerState.componentStates[i] = ASW_STATE_ERROR;
                result = RTE_E_NOK;
            }
        }
    }

    Rte_AswSchedulerState.isRunning = TRUE;

    /* Start the scheduler tick */
    Rte_Scheduler_Start();

    return result;
}

/**
 * @brief Stops the ASW scheduler and deinitializes all components
 */
Rte_StatusType Rte_AswScheduler_Stop(void)
{
    uint8 i;

    /* Stop the scheduler */
    Rte_Scheduler_Stop();

    /* Deinitialize all components in reverse order */
    for (i = SWC_ID_COUNT; i > 0U; i--)
    {
        uint8 idx = i - 1U;
        if (Rte_AswComponentTable[idx].Deinit != NULL_PTR)
        {
            Rte_AswComponentTable[idx].Deinit();
        }
        Rte_AswSchedulerState.componentStates[idx] = ASW_STATE_STOPPED;
    }

    Rte_AswSchedulerState.isRunning = FALSE;

    return RTE_E_OK;
}

/**
 * @brief Gets a component entry by ID
 */
const Rte_AswComponentEntryType* Rte_AswScheduler_GetComponentEntry(Swc_ComponentIdType componentId)
{
    if (componentId >= SWC_ID_COUNT)
    {
        return NULL_PTR;
    }
    return &Rte_AswComponentTable[componentId];
}

/**
 * @brief Gets component runtime state
 */
Rte_StatusType Rte_AswScheduler_GetComponentState(Swc_ComponentIdType componentId,
                                                   Rte_AswComponentStateType* state)
{
    if (componentId >= SWC_ID_COUNT)
    {
        return RTE_E_INVALID;
    }

    if (state == NULL_PTR)
    {
        return RTE_E_INVALID;
    }

    *state = Rte_AswSchedulerState.componentStates[componentId];
    return RTE_E_OK;
}

/**
 * @brief Initializes a specific component by ID
 */
Rte_StatusType Rte_AswScheduler_InitComponent(Swc_ComponentIdType componentId)
{
    if (componentId >= SWC_ID_COUNT)
    {
        return RTE_E_INVALID;
    }

    if (Rte_AswComponentTable[componentId].Init != NULL_PTR)
    {
        Rte_AswComponentTable[componentId].Init();
        Rte_AswSchedulerState.componentStates[componentId] = ASW_STATE_INITIALIZED;
        return RTE_E_OK;
    }

    return RTE_E_NOK;
}

/**
 * @brief Deinitializes a specific component by ID
 */
Rte_StatusType Rte_AswScheduler_DeinitComponent(Swc_ComponentIdType componentId)
{
    if (componentId >= SWC_ID_COUNT)
    {
        return RTE_E_INVALID;
    }

    if (Rte_AswComponentTable[componentId].Deinit != NULL_PTR)
    {
        Rte_AswComponentTable[componentId].Deinit();
    }
    Rte_AswSchedulerState.componentStates[componentId] = ASW_STATE_STOPPED;

    return RTE_E_OK;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
