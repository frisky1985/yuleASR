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
 * @file Rte_AswScheduler.h
 * @brief ASW Component Scheduler Registration header
 * @version 1.0.0
 * @date 2026-05-26
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Runtime Environment (RTE)
 * Purpose: ASW component registration and scheduler integration
 */

#ifndef RTE_ASWSCHEDULER_H
#define RTE_ASWSCHEDULER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Type.h"

/*==================================================================================================
*                                    ASW COMPONENT IDS
==================================================================================================*/

/** @brief ASW Component IDs corresponding to Rte_Cfg.h definitions */
typedef enum {
    SWC_ID_ENGINE_CONTROL         = 0U,  /* RTE_COMPONENT_SWC_ECU_MANAGER */
    SWC_ID_VEHICLE_DYNAMICS       = 1U,  /* RTE_COMPONENT_SWC_DIAGNOSTIC */
    SWC_ID_DIAGNOSTIC_MANAGER     = 2U,  /* RTE_COMPONENT_SWC_COMMUNICATION */
    SWC_ID_COMMUNICATION_MANAGER  = 3U,  /* RTE_COMPONENT_SWC_STORAGE */
    SWC_ID_STORAGE_MANAGER        = 4U,  /* RTE_COMPONENT_SWC_IO_CONTROL */
    SWC_ID_IO_CONTROL             = 5U,  /* RTE_COMPONENT_SWC_MODE_MANAGER */
    SWC_ID_MODE_MANAGER           = 6U,  /* RTE_COMPONENT_SWC_WATCHDOG */
    SWC_ID_WATCHDOG_MANAGER       = 7U,  /* RTE_COMPONENT_SWC_NVM_MANAGER */
    SWC_ID_COUNT                  = 8U,
    SWC_ID_INVALID                = 0xFFU
} Swc_ComponentIdType;

/*==================================================================================================
*                                    ASW COMPONENT ENTRY TYPE
==================================================================================================*/

/**
 * @brief ASW component entry with lifecycle function pointers
 */
typedef struct {
    Swc_ComponentIdType componentId;     /**< Component ID */
    const char*         componentName;   /**< Component name string */
    void                (*Init)(void);   /**< Component initialization */
    void                (*Deinit)(void); /**< Component deinitialization */
    void                (*MainFunction)(void); /**< Main cyclic entry for scheduler */
    uint32              periodMs;        /**< Base scheduling period in ms */
    uint8               priority;        /**< Scheduling priority (lower = higher) */
    boolean             isMandatory;     /**< Component is mandatory for system operation */
} Rte_AswComponentEntryType;

/*==================================================================================================
*                                    ASW COMPONENT STATE
==================================================================================================*/

/** @brief ASW component runtime state */
typedef enum {
    ASW_STATE_UNINITIALIZED = 0U,
    ASW_STATE_INITIALIZED,
    ASW_STATE_RUNNING,
    ASW_STATE_STOPPED,
    ASW_STATE_ERROR
} Rte_AswComponentStateType;

/*==================================================================================================
*                                    ASW SCHEDULER API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Starts the ASW scheduler - registers all components and enables scheduling
 * @return RTE_E_OK on success, RTE_E_NOK on failure
 */
extern Rte_StatusType Rte_AswScheduler_Start(void);

/**
 * @brief Stops the ASW scheduler and deinitializes all components
 * @return RTE_E_OK on success
 */
extern Rte_StatusType Rte_AswScheduler_Stop(void);

/**
 * @brief Gets a component entry by ID
 * @param componentId Component ID
 * @return Pointer to component entry, or NULL if not found
 */
extern const Rte_AswComponentEntryType* Rte_AswScheduler_GetComponentEntry(Swc_ComponentIdType componentId);

/**
 * @brief Gets component runtime state
 * @param componentId Component ID
 * @param state Pointer to store state
 * @return RTE_E_OK on success
 */
extern Rte_StatusType Rte_AswScheduler_GetComponentState(Swc_ComponentIdType componentId,
                                                          Rte_AswComponentStateType* state);

/**
 * @brief Initializes a specific component by ID
 * @param componentId Component ID
 * @return RTE_E_OK on success
 */
extern Rte_StatusType Rte_AswScheduler_InitComponent(Swc_ComponentIdType componentId);

/**
 * @brief Deinitializes a specific component by ID
 * @param componentId Component ID
 * @return RTE_E_OK on success
 */
extern Rte_StatusType Rte_AswScheduler_DeinitComponent(Swc_ComponentIdType componentId);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

#endif /* RTE_ASWSCHEDULER_H */
