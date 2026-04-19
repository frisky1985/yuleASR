/**
 * @file Swc_WatchdogManager.c
 * @brief Watchdog Manager Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Watchdog supervision and alive monitoring at application layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_WatchdogManager.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_WATCHDOGMANAGER_MODULE_ID       0x87
#define SWC_WATCHDOGMANAGER_INSTANCE_ID     0x00

/* Maximum supervised entities */
#define WDG_MAX_ENTITIES                    16

/* Watchdog timeout in ms */
#define WDG_TIMEOUT_MS                      100

/* Alive indication window */
#define WDG_ALIVE_WINDOW_MS                 50

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_SupervisedEntityConfigType config;
    Swc_SupervisedEntityStatusType status;
    boolean isRegistered;
} Swc_SupervisedEntityType;

typedef struct {
    Swc_WatchdogManagerConfigType config;
    Swc_WatchdogManagerStatusType status;
    Swc_SupervisedEntityType entities[WDG_MAX_ENTITIES];
    uint32 lastTriggerTime;
    boolean isInitialized;
} Swc_WatchdogManagerInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_WatchdogManagerInternalType swcWatchdogManager = {
    .config = {
        .initialTimeout = WDG_TIMEOUT_MS,
        .windowTimeout = WDG_ALIVE_WINDOW_MS,
        .windowModeEnabled = TRUE,
        .fastModeEnabled = FALSE
    },
    .status = {
        .watchdogStatus = WDG_STATUS_STOPPED,
        .numSupervisedEntities = 0,
        .globalSupervisionCycle = 0,
        .isInitialized = FALSE
    },
    .entities = {{0}},
    .lastTriggerTime = 0,
    .isInitialized = FALSE
};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Swc_WatchdogManager_SuperviseEntities(void);
STATIC void Swc_WatchdogManager_UpdateAliveCounters(void);
STATIC void Swc_WatchdogManager_CheckTimeouts(void);
STATIC sint16 Swc_WatchdogManager_FindEntity(uint8 entityId);
STATIC void Swc_WatchdogManager_TriggerHardwareWatchdog(void);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Supervises all registered entities
 */
STATIC void Swc_WatchdogManager_SuperviseEntities(void)
{
    uint8 i;
    uint32 currentTime = Rte_GetTime();

    for (i = 0; i < WDG_MAX_ENTITIES; i++) {
        if (swcWatchdogManager.entities[i].isRegistered &&
            swcWatchdogManager.entities[i].config.isActive) {

            Swc_SupervisedEntityStatusType* status = &swcWatchdogManager.entities[i].status;

            /* Check if entity has provided alive indication */
            if ((currentTime - status->lastAliveTime) > swcWatchdogManager.entities[i].config.aliveTimeout) {
                /* Timeout - entity missed deadline */
                status->missedIndications++;

                if (status->missedIndications > 3) {
                    status->state = ALIVE_STATE_EXPIRED;
                    swcWatchdogManager.status.watchdogStatus = WDG_STATUS_EXPIRED;
                }
            }

            /* Check alive indication count */
            if (status->aliveIndications < swcWatchdogManager.entities[i].config.minCorrectIndications) {
                status->state = ALIVE_STATE_INCORRECT;
            } else if (status->aliveIndications > swcWatchdogManager.entities[i].config.maxCorrectIndications) {
                status->state = ALIVE_STATE_INCORRECT;
            } else {
                if (status->state != ALIVE_STATE_EXPIRED) {
                    status->state = ALIVE_STATE_CORRECT;
                }
            }

            /* Reset alive counter for next supervision cycle */
            status->aliveIndications = 0;
            status->supervisionCycle = swcWatchdogManager.status.globalSupervisionCycle;
        }
    }
}

/**
 * @brief Updates alive counters from RTE
 */
STATIC void Swc_WatchdogManager_UpdateAliveCounters(void)
{
    uint8 i;
    uint8 entityId;

    /* Read alive indications from RTE */
    if (Rte_Read_AliveIndication(&entityId) == RTE_E_OK) {
        sint16 entityIndex = Swc_WatchdogManager_FindEntity(entityId);

        if (entityIndex >= 0) {
            if (swcWatchdogManager.entities[entityIndex].config.isActive) {
                swcWatchdogManager.entities[entityIndex].status.aliveIndications++;
                swcWatchdogManager.entities[entityIndex].status.lastAliveTime = Rte_GetTime();
            }
        }
    }

    /* Also check for checkpoint reached calls */
    for (i = 0; i < WDG_MAX_ENTITIES; i++) {
        if (swcWatchdogManager.entities[i].isRegistered &&
            swcWatchdogManager.entities[i].config.isActive) {
            /* Alive indications are updated via CheckpointReached API */
        }
    }
}

/**
 * @brief Checks for watchdog timeouts
 */
STATIC void Swc_WatchdogManager_CheckTimeouts(void)
{
    uint32 currentTime = Rte_GetTime();

    /* Check if watchdog needs to be triggered */
    if ((currentTime - swcWatchdogManager.lastTriggerTime) > WDG_TIMEOUT_MS) {
        /* Check if all entities are correct before triggering */
        if (Swc_WatchdogManager_IsGlobalStatusCorrect()) {
            Swc_WatchdogManager_TriggerHardwareWatchdog();
        } else {
            /* Don't trigger - allow watchdog to expire */
            swcWatchdogManager.status.watchdogStatus = WDG_STATUS_EXPIRED;
        }
    }
}

/**
 * @brief Finds entity by ID
 */
STATIC sint16 Swc_WatchdogManager_FindEntity(uint8 entityId)
{
    uint8 i;

    for (i = 0; i < WDG_MAX_ENTITIES; i++) {
        if (swcWatchdogManager.entities[i].isRegistered &&
            swcWatchdogManager.entities[i].config.entityId == entityId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Triggers hardware watchdog
 */
STATIC void Swc_WatchdogManager_TriggerHardwareWatchdog(void)
{
    uint8 triggerValue = 0x01;

    /* Write trigger via RTE to Wdg driver */
    (void)Rte_Write_WatchdogTrigger(&triggerValue);

    swcWatchdogManager.lastTriggerTime = Rte_GetTime();
    swcWatchdogManager.status.watchdogStatus = WDG_STATUS_OK;
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Watchdog Manager component
 */
void Swc_WatchdogManager_Init(void)
{
    uint8 i;

    if (swcWatchdogManager.isInitialized) {
        return;
    }

    /* Initialize configuration */
    swcWatchdogManager.config.initialTimeout = WDG_TIMEOUT_MS;
    swcWatchdogManager.config.windowTimeout = WDG_ALIVE_WINDOW_MS;
    swcWatchdogManager.config.windowModeEnabled = TRUE;
    swcWatchdogManager.config.fastModeEnabled = FALSE;

    /* Initialize status */
    swcWatchdogManager.status.watchdogStatus = WDG_STATUS_STOPPED;
    swcWatchdogManager.status.numSupervisedEntities = 0;
    swcWatchdogManager.status.globalSupervisionCycle = 0;
    swcWatchdogManager.status.isInitialized = FALSE;

    /* Initialize entities */
    for (i = 0; i < WDG_MAX_ENTITIES; i++) {
        swcWatchdogManager.entities[i].config.entityId = 0;
        swcWatchdogManager.entities[i].config.aliveTimeout = 0;
        swcWatchdogManager.entities[i].config.expectedAliveIndications = 0;
        swcWatchdogManager.entities[i].config.minCorrectIndications = 0;
        swcWatchdogManager.entities[i].config.maxCorrectIndications = 0;
        swcWatchdogManager.entities[i].config.isActive = FALSE;

        swcWatchdogManager.entities[i].status.entityId = 0;
        swcWatchdogManager.entities[i].status.state = ALIVE_STATE_DEACTIVATED;
        swcWatchdogManager.entities[i].status.aliveIndications = 0;
        swcWatchdogManager.entities[i].status.missedIndications = 0;
        swcWatchdogManager.entities[i].status.lastAliveTime = 0;
        swcWatchdogManager.entities[i].status.supervisionCycle = 0;

        swcWatchdogManager.entities[i].isRegistered = FALSE;
    }

    swcWatchdogManager.lastTriggerTime = 0;
    swcWatchdogManager.isInitialized = TRUE;
    swcWatchdogManager.status.isInitialized = TRUE;

    Det_ReportError(SWC_WATCHDOGMANAGER_MODULE_ID, SWC_WATCHDOGMANAGER_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 10ms cyclic runnable
 */
void Swc_WatchdogManager_10ms(void)
{
    if (!swcWatchdogManager.isInitialized) {
        return;
    }

    /* Update alive counters */
    Swc_WatchdogManager_UpdateAliveCounters();

    /* Check timeouts */
    Swc_WatchdogManager_CheckTimeouts();

    /* Write status via RTE */
    (void)Rte_Write_WatchdogStatus(&swcWatchdogManager.status.watchdogStatus);
}

/**
 * @brief Watchdog trigger runnable
 */
void Swc_WatchdogManager_Trigger(void)
{
    if (!swcWatchdogManager.isInitialized) {
        return;
    }

    /* Supervise entities */
    Swc_WatchdogManager_SuperviseEntities();

    /* Increment global supervision cycle */
    swcWatchdogManager.status.globalSupervisionCycle++;

    /* Check if we can trigger watchdog */
    if (Swc_WatchdogManager_IsGlobalStatusCorrect()) {
        Swc_WatchdogManager_TriggerHardwareWatchdog();
    }
}

/**
 * @brief Checks in supervised entity (checkpoint reached)
 */
Rte_StatusType Swc_WatchdogManager_CheckpointReached(uint8 entityId)
{
    sint16 entityIndex;

    if (!swcWatchdogManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    entityIndex = Swc_WatchdogManager_FindEntity(entityId);

    if (entityIndex < 0) {
        return RTE_E_INVALID;
    }

    if (!swcWatchdogManager.entities[entityIndex].config.isActive) {
        return RTE_E_NOT_ACTIVE;
    }

    /* Update alive indication */
    swcWatchdogManager.entities[entityIndex].status.aliveIndications++;
    swcWatchdogManager.entities[entityIndex].status.lastAliveTime = Rte_GetTime();

    return RTE_E_OK;
}

/**
 * @brief Registers supervised entity
 */
Rte_StatusType Swc_WatchdogManager_RegisterEntity(const Swc_SupervisedEntityConfigType* config)
{
    sint16 entityIndex;

    if (config == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcWatchdogManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    /* Check if entity already registered */
    entityIndex = Swc_WatchdogManager_FindEntity(config->entityId);

    if (entityIndex >= 0) {
        return RTE_E_INVALID;  /* Already registered */
    }

    /* Find free slot */
    for (entityIndex = 0; entityIndex < WDG_MAX_ENTITIES; entityIndex++) {
        if (!swcWatchdogManager.entities[entityIndex].isRegistered) {
            break;
        }
    }

    if (entityIndex >= WDG_MAX_ENTITIES) {
        return RTE_E_LIMIT;  /* No free slots */
    }

    /* Register entity */
    swcWatchdogManager.entities[entityIndex].config = *config;
    swcWatchdogManager.entities[entityIndex].status.entityId = config->entityId;
    swcWatchdogManager.entities[entityIndex].status.state = ALIVE_STATE_CORRECT;
    swcWatchdogManager.entities[entityIndex].isRegistered = TRUE;

    swcWatchdogManager.status.numSupervisedEntities++;

    return RTE_E_OK;
}

/**
 * @brief Unregisters supervised entity
 */
Rte_StatusType Swc_WatchdogManager_UnregisterEntity(uint8 entityId)
{
    sint16 entityIndex;

    if (!swcWatchdogManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    entityIndex = Swc_WatchdogManager_FindEntity(entityId);

    if (entityIndex < 0) {
        return RTE_E_INVALID;
    }

    /* Unregister entity */
    swcWatchdogManager.entities[entityIndex].isRegistered = FALSE;
    swcWatchdogManager.entities[entityIndex].config.isActive = FALSE;
    swcWatchdogManager.entities[entityIndex].status.state = ALIVE_STATE_DEACTIVATED;

    if (swcWatchdogManager.status.numSupervisedEntities > 0) {
        swcWatchdogManager.status.numSupervisedEntities--;
    }

    return RTE_E_OK;
}

/**
 * @brief Gets entity status
 */
Rte_StatusType Swc_WatchdogManager_GetEntityStatus(uint8 entityId,
                                                    Swc_SupervisedEntityStatusType* status)
{
    sint16 entityIndex;

    if (status == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcWatchdogManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    entityIndex = Swc_WatchdogManager_FindEntity(entityId);

    if (entityIndex < 0) {
        return RTE_E_INVALID;
    }

    *status = swcWatchdogManager.entities[entityIndex].status;
    return RTE_E_OK;
}

/**
 * @brief Sets entity active state
 */
Rte_StatusType Swc_WatchdogManager_SetEntityActive(uint8 entityId, boolean active)
{
    sint16 entityIndex;

    if (!swcWatchdogManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    entityIndex = Swc_WatchdogManager_FindEntity(entityId);

    if (entityIndex < 0) {
        return RTE_E_INVALID;
    }

    swcWatchdogManager.entities[entityIndex].config.isActive = active;

    if (active) {
        swcWatchdogManager.entities[entityIndex].status.state = ALIVE_STATE_CORRECT;
    } else {
        swcWatchdogManager.entities[entityIndex].status.state = ALIVE_STATE_DEACTIVATED;
    }

    return RTE_E_OK;
}

/**
 * @brief Gets watchdog manager status
 */
Rte_StatusType Swc_WatchdogManager_GetStatus(Swc_WatchdogManagerStatusType* status)
{
    if (status == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcWatchdogManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *status = swcWatchdogManager.status;
    return RTE_E_OK;
}

/**
 * @brief Checks if global status is correct
 */
boolean Swc_WatchdogManager_IsGlobalStatusCorrect(void)
{
    uint8 i;

    for (i = 0; i < WDG_MAX_ENTITIES; i++) {
        if (swcWatchdogManager.entities[i].isRegistered &&
            swcWatchdogManager.entities[i].config.isActive) {
            if (swcWatchdogManager.entities[i].status.state != ALIVE_STATE_CORRECT) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * @brief Handles watchdog expiration
 */
void Swc_WatchdogManager_HandleExpiration(void)
{
    /* This function is called when the hardware watchdog expires */
    /* In a real implementation, this would handle safe state transition */

    swcWatchdogManager.status.watchdogStatus = WDG_STATUS_EXPIRED;

    /* Report error */
    Det_ReportError(SWC_WATCHDOGMANAGER_MODULE_ID, SWC_WATCHDOGMANAGER_INSTANCE_ID,
                    0x50, RTE_E_NOT_OK);

    /* Write status via RTE */
    (void)Rte_Write_WatchdogStatus(&swcWatchdogManager.status.watchdogStatus);
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
