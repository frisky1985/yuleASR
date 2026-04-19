/**
 * @file Swc_WatchdogManager.h
 * @brief Watchdog Manager Software Component Header
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Watchdog supervision and alive monitoring at application layer
 */

#ifndef SWC_WATCHDOGMANAGER_H
#define SWC_WATCHDOGMANAGER_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Swc.h"
#include "Std_Types.h"

/*==================================================================================================
*                                    COMPONENT TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Watchdog supervision status
 */
typedef enum {
    WDG_STATUS_OK = 0,
    WDG_STATUS_EXPIRED,
    WDG_STATUS_STOPPED,
    WDG_STATUS_FAULT
} Swc_WatchdogStatusType;

/**
 * @brief Alive supervision state
 */
typedef enum {
    ALIVE_STATE_CORRECT = 0,
    ALIVE_STATE_INCORRECT,
    ALIVE_STATE_EXPIRED,
    ALIVE_STATE_DEACTIVATED
} Swc_AliveStateType;

/**
 * @brief Supervised entity configuration
 */
typedef struct {
    uint8 entityId;
    uint16 aliveTimeout;          /* ms */
    uint16 expectedAliveIndications;
    uint16 minCorrectIndications;
    uint16 maxCorrectIndications;
    boolean isActive;
} Swc_SupervisedEntityConfigType;

/**
 * @brief Supervised entity status
 */
typedef struct {
    uint8 entityId;
    Swc_AliveStateType state;
    uint16 aliveIndications;
    uint16 missedIndications;
    uint32 lastAliveTime;
    uint32 supervisionCycle;
} Swc_SupervisedEntityStatusType;

/**
 * @brief Watchdog manager configuration
 */
typedef struct {
    uint16 initialTimeout;        /* ms */
    uint16 windowTimeout;         /* ms */
    boolean windowModeEnabled;
    boolean fastModeEnabled;
} Swc_WatchdogManagerConfigType;

/**
 * @brief Watchdog manager status
 */
typedef struct {
    Swc_WatchdogStatusType watchdogStatus;
    uint8 numSupervisedEntities;
    uint32 globalSupervisionCycle;
    boolean isInitialized;
} Swc_WatchdogManagerStatusType;

/*==================================================================================================
*                                    RUNNABLE IDS
==================================================================================================*/
#define RTE_RUNNABLE_WatchdogManager_Init            0x71
#define RTE_RUNNABLE_WatchdogManager_10ms            0x72
#define RTE_RUNNABLE_WatchdogManager_Trigger         0x73

/*==================================================================================================
*                                    PORT IDS
==================================================================================================*/
#define SWC_WATCHDOGMANAGER_PORT_WDG_STATUS_P        0x71
#define SWC_WATCHDOGMANAGER_PORT_ENTITY_STATUS_P     0x72
#define SWC_WATCHDOGMANAGER_PORT_ALIVE_INDICATION_R  0x73
#define SWC_WATCHDOGMANAGER_PORT_WDG_TRIGGER_P       0x74

/*==================================================================================================
*                                    COMPONENT API
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Watchdog Manager component
 */
extern void Swc_WatchdogManager_Init(void);

/**
 * @brief 10ms cyclic runnable - watchdog supervision
 */
extern void Swc_WatchdogManager_10ms(void);

/**
 * @brief Watchdog trigger runnable
 */
extern void Swc_WatchdogManager_Trigger(void);

/**
 * @brief Checks in supervised entity
 * @param entityId Entity ID
 * @return RTE status
 */
extern Rte_StatusType Swc_WatchdogManager_CheckpointReached(uint8 entityId);

/**
 * @brief Registers supervised entity
 * @param config Entity configuration
 * @return RTE status
 */
extern Rte_StatusType Swc_WatchdogManager_RegisterEntity(
    const Swc_SupervisedEntityConfigType* config);

/**
 * @brief Unregisters supervised entity
 * @param entityId Entity ID
 * @return RTE status
 */
extern Rte_StatusType Swc_WatchdogManager_UnregisterEntity(uint8 entityId);

/**
 * @brief Gets entity status
 * @param entityId Entity ID
 * @param status Pointer to store status
 * @return RTE status
 */
extern Rte_StatusType Swc_WatchdogManager_GetEntityStatus(
    uint8 entityId,
    Swc_SupervisedEntityStatusType* status);

/**
 * @brief Sets entity active state
 * @param entityId Entity ID
 * @param active TRUE to activate, FALSE to deactivate
 * @return RTE status
 */
extern Rte_StatusType Swc_WatchdogManager_SetEntityActive(uint8 entityId, boolean active);

/**
 * @brief Gets watchdog manager status
 * @param status Pointer to store status
 * @return RTE status
 */
extern Rte_StatusType Swc_WatchdogManager_GetStatus(Swc_WatchdogManagerStatusType* status);

/**
 * @brief Performs global status check
 * @return TRUE if all entities are correct
 */
extern boolean Swc_WatchdogManager_IsGlobalStatusCorrect(void);

/**
 * @brief Handles watchdog expiration
 */
extern void Swc_WatchdogManager_HandleExpiration(void);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE INTERFACE MACROS
==================================================================================================*/
#define Rte_Write_WatchdogStatus(data) \
    Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_STATUS_P(data)

#define Rte_Write_EntityStatus(data) \
    Rte_Write_SWC_WATCHDOGMANAGER_PORT_ENTITY_STATUS_P(data)

#define Rte_Read_AliveIndication(data) \
    Rte_Read_SWC_WATCHDOGMANAGER_PORT_ALIVE_INDICATION_R(data)

#define Rte_Write_WatchdogTrigger(data) \
    Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_TRIGGER_P(data)

#endif /* SWC_WATCHDOGMANAGER_H */
