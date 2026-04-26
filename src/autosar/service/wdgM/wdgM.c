/******************************************************************************
 * @file    wdgM.c
 * @brief   Watchdog Manager (WdgM) Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/service/wdgM/wdgM.h"
#include "autosar/service/wdgM/wdgM_Cfg.h"
#include "autosar/service/wdgM/wdgIf.h"
#include <string.h>

/******************************************************************************
 * Module Internal Constants
 ******************************************************************************/
#define WDGM_CYCLE_COUNTER_MAX          0xFFFFU
#define WDGM_INVALID_TIMESTAMP          0xFFFFFFFFU
#define WDGM_MAINFUNCTION_PERIOD_MS     10U
#define WDGM_IMMEDIATE_RESET_THRESHOLD  1U
#define WDGM_MAX_FAILED_CYCLES          3U

/******************************************************************************
 * Module Variables
 ******************************************************************************/
/* External configuration pointer */
const WdgM_ConfigType *WdgM_ConfigPtr = NULL;

/* Module status */
WdgM_ModuleStatusType WdgM_ModuleStatus;

/* SE status array */
WdgM_SupervisedEntityStatusType WdgM_SEStatuses[WDGM_MAX_SUPERVISED_ENTITIES];

/* Runtime data for alive supervision */
static WdgM_AliveSupervisionStatusType WdgM_AliveStatus[WDGM_MAX_SUPERVISED_ENTITIES][4U];

/* Runtime data for deadline supervision */
static WdgM_DeadlineSupervisionStatusType WdgM_DeadlineStatus[WDGM_MAX_SUPERVISED_ENTITIES][2U];

/* Current mode configuration pointer */
static const WdgM_ModeConfigType *WdgM_CurrentModeConfig = NULL;

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static void WdgM_UpdateGlobalStatus(void);
static void WdgM_CheckAliveSupervision(WdgM_SupervisedEntityIdType seId);
static void WdgM_CheckDeadlineSupervision(WdgM_SupervisedEntityIdType seId);
static void WdgM_CheckLogicalSupervision(WdgM_SupervisedEntityIdType seId);
static Std_ReturnType WdgM_ValidateCheckpointTransition(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType fromCp,
    WdgM_CheckpointIdType toCp
);
static void WdgM_TransitionSEStatus(
    WdgM_SupervisedEntityIdType seId,
    WdgM_LocalStatusType newStatus
);
static void WdgM_TriggerWatchdog(void);
static void WdgM_ReportProductionError(uint8 errorCode, WdgM_SupervisedEntityIdType seId);
static const WdgM_SupervisedEntityConfigType* WdgM_GetSEConfig(
    WdgM_SupervisedEntityIdType seId
);
static void WdgM_ClearSEStatus(WdgM_SupervisedEntityIdType seId);
static void WdgM_ProcessSESupervision(WdgM_SupervisedEntityIdType seId);

/******************************************************************************
 * Internal Function Implementations
 ******************************************************************************/

/**
 * @brief Get SE configuration from current mode
 */
static const WdgM_SupervisedEntityConfigType* WdgM_GetSEConfig(
    WdgM_SupervisedEntityIdType seId)
{
    const WdgM_SupervisedEntityConfigType *seConfig = NULL;
    uint8 i;

    if ((WdgM_CurrentModeConfig != NULL) && (seId < WDGM_MAX_SUPERVISED_ENTITIES)) {
        for (i = 0U; i < WdgM_CurrentModeConfig->numSEs; i++) {
            if (WdgM_CurrentModeConfig->seConfigs[i].seId == seId) {
                seConfig = &WdgM_CurrentModeConfig->seConfigs[i];
                break;
            }
        }
    }

    return seConfig;
}

/**
 * @brief Clear SE status for mode transition
 */
static void WdgM_ClearSEStatus(WdgM_SupervisedEntityIdType seId)
{
    uint8 i;

    if (seId < WDGM_MAX_SUPERVISED_ENTITIES) {
        WdgM_SEStatuses[seId].localStatus = WDGM_LOCAL_STATUS_OK;
        WdgM_SEStatuses[seId].failureCounter = 0U;
        WdgM_SEStatuses[seId].consecutiveFailures = 0U;
        
        /* Clear alive supervision status */
        for (i = 0U; i < 4U; i++) {
            WdgM_AliveStatus[seId][i].cycleCounter = 0U;
            WdgM_AliveStatus[seId][i].aliveIndications = 0U;
            WdgM_AliveStatus[seId][i].checkpointReachedThisCycle = FALSE;
        }
        
        /* Clear deadline supervision status */
        for (i = 0U; i < 2U; i++) {
            WdgM_DeadlineStatus[seId][i].startTime = WDGM_INVALID_TIMESTAMP;
            WdgM_DeadlineStatus[seId][i].startReached = FALSE;
            WdgM_DeadlineStatus[seId][i].endReached = FALSE;
        }
        
        /* Clear logical supervision status */
        WdgM_SEStatuses[seId].logicalStatus.lastCheckpoint = WDGM_INVALID_CP_ID;
        WdgM_SEStatuses[seId].logicalStatus.lastCheckpointValid = FALSE;
    }
}

/**
 * @brief Update global status based on all SE statuses
 */
static void WdgM_UpdateGlobalStatus(void)
{
    WdgM_GlobalStatusType oldStatus = WdgM_ModuleStatus.globalStatus;
    WdgM_GlobalStatusType newStatus = WDGM_GLOBAL_STATUS_OK;
    boolean hasFailed = FALSE;
    boolean hasExpired = FALSE;
    WdgM_SupervisedEntityIdType firstExpired = WDGM_INVALID_SE_ID;
    uint8 i;

    /* Check all SE statuses */
    for (i = 0U; i < WDGM_MAX_SUPERVISED_ENTITIES; i++) {
        if (WdgM_SEStatuses[i].isActive) {
            switch (WdgM_SEStatuses[i].localStatus) {
                case WDGM_LOCAL_STATUS_FAILED:
                    hasFailed = TRUE;
                    break;
                    
                case WDGM_LOCAL_STATUS_EXPIRED:
                    hasExpired = TRUE;
                    if (firstExpired == WDGM_INVALID_SE_ID) {
                        firstExpired = i;
                    }
                    break;
                    
                case WDGM_LOCAL_STATUS_OK:
                case WDGM_LOCAL_STATUS_DEACTIVATED:
                default:
                    /* No action needed */
                    break;
            }
        }
    }

    /* Determine global status */
    if (hasExpired) {
        newStatus = WDGM_GLOBAL_STATUS_EXPIRED;
    } else if (hasFailed) {
        newStatus = WDGM_GLOBAL_STATUS_FAILED;
    } else {
        newStatus = WDGM_GLOBAL_STATUS_OK;
    }

    /* Update global status */
    WdgM_ModuleStatus.globalStatus = newStatus;
    WdgM_ModuleStatus.firstExpiredSEId = firstExpired;

    /* Call callback if status changed */
    if ((newStatus != oldStatus) && (WdgM_ConfigPtr != NULL)) {
        if (WdgM_ConfigPtr->globalStatusCallback != NULL) {
            WdgM_ConfigPtr->globalStatusCallback(newStatus, oldStatus);
        }
        
        /* Call application callback */
        WdgM_GlobalStatusCallback(newStatus, oldStatus);
    }

    /* Handle expired status */
    if (newStatus == WDGM_GLOBAL_STATUS_EXPIRED) {
        WdgM_ModuleStatus.failedCyclesCounter++;
        
        /* Check if immediate reset is needed */
        if ((WDGM_IMMEDIATE_RESET_ENABLED == STD_ON) ||
            (WdgM_ModuleStatus.failedCyclesCounter >= WDGM_MAX_FAILED_CYCLES)) {
            WdgM_PerformReset();
        }
    } else {
        WdgM_ModuleStatus.failedCyclesCounter = 0U;
    }
}

/**
 * @brief Transition SE local status
 */
static void WdgM_TransitionSEStatus(
    WdgM_SupervisedEntityIdType seId,
    WdgM_LocalStatusType newStatus)
{
    WdgM_LocalStatusType oldStatus;

    if (seId < WDGM_MAX_SUPERVISED_ENTITIES) {
        oldStatus = WdgM_SEStatuses[seId].localStatus;
        
        if (newStatus != oldStatus) {
            WdgM_SEStatuses[seId].localStatus = newStatus;
            
            /* Call callback if registered */
            if ((WdgM_ConfigPtr != NULL) && 
                (WdgM_ConfigPtr->localStatusCallback != NULL)) {
                WdgM_ConfigPtr->localStatusCallback(seId, newStatus, oldStatus);
            }
            
            /* Call application callback */
            WdgM_LocalStatusCallback(seId, newStatus, oldStatus);
        }
    }
}

/**
 * @brief Check alive supervision for an SE
 */
static void WdgM_CheckAliveSupervision(WdgM_SupervisedEntityIdType seId)
{
    const WdgM_SupervisedEntityConfigType *seConfig;
    WdgM_AliveSupervisionStatusType *aliveStatus;
    const WdgM_AliveSupervisionConfigType *asConfig;
    uint16 expectedIndications;
    uint16 actualIndications;
    boolean supervisionFailed = FALSE;
    uint8 i;

    seConfig = WdgM_GetSEConfig(seId);
    if ((seConfig == NULL) || (!seConfig->isActive)) {
        return;
    }

    /* Check each alive supervision */
    for (i = 0U; i < seConfig->numAliveSupervisions; i++) {
        asConfig = &seConfig->aliveSupervisions[i];
        aliveStatus = &WdgM_AliveStatus[seId][i];
        
        /* Increment cycle counter */
        aliveStatus->cycleCounter++;
        
        /* Check if supervision cycle completed */
        if (aliveStatus->cycleCounter >= asConfig->supervisionCycle) {
            expectedIndications = asConfig->expectedAliveIndications;
            actualIndications = aliveStatus->aliveIndications;
            
            /* Check if alive indications are within expected range */
            if ((actualIndications < (expectedIndications - asConfig->minMargin)) ||
                (actualIndications > (expectedIndications + asConfig->maxMargin))) {
                supervisionFailed = TRUE;
            }
            
            /* Reset counters for next cycle */
            aliveStatus->cycleCounter = 0U;
            aliveStatus->aliveIndications = 0U;
        }
    }

    /* Handle supervision failure */
    if (supervisionFailed) {
        WdgM_SEStatuses[seId].failureCounter++;
        WdgM_SEStatuses[seId].consecutiveFailures++;
        
        /* Check if tolerance exceeded */
        if (WdgM_SEStatuses[seId].failureCounter > seConfig->failureTolerance) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_EXPIRED);
            WdgM_ReportProductionError(WDGM_E_ALIVE_SUPERVISION, seId);
        } else if (WdgM_SEStatuses[seId].localStatus == WDGM_LOCAL_STATUS_OK) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_FAILED);
        }
    } else {
        /* Reset consecutive failures on success */
        WdgM_SEStatuses[seId].consecutiveFailures = 0U;
        
        /* Transition back to OK if in FAILED state */
        if (WdgM_SEStatuses[seId].localStatus == WDGM_LOCAL_STATUS_FAILED) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_OK);
        }
    }
}

/**
 * @brief Check deadline supervision for an SE
 */
static void WdgM_CheckDeadlineSupervision(WdgM_SupervisedEntityIdType seId)
{
    const WdgM_SupervisedEntityConfigType *seConfig;
    const WdgM_DeadlineSupervisionConfigType *dsConfig;
    WdgM_DeadlineSupervisionStatusType *deadlineStatus;
    uint32 currentTime;
    uint32 elapsedTime;
    boolean supervisionFailed = FALSE;
    uint8 i;

    seConfig = WdgM_GetSEConfig(seId);
    if ((seConfig == NULL) || (!seConfig->isActive)) {
        return;
    }

    /* Get current time (in ms) */
    currentTime = (uint32)WdgM_ModuleStatus.cycleCounter * WDGM_MAINFUNCTION_PERIOD_MS;

    /* Check each deadline supervision */
    for (i = 0U; i < seConfig->numDeadlineSupervisions; i++) {
        dsConfig = &seConfig->deadlineSupervisions[i];
        deadlineStatus = &WdgM_DeadlineStatus[seId][i];
        
        if (deadlineStatus->startReached && !deadlineStatus->endReached) {
            /* Calculate elapsed time */
            elapsedTime = currentTime - deadlineStatus->startTime;
            
            /* Check deadline constraints */
            if ((elapsedTime < dsConfig->minDeadline) ||
                (elapsedTime > dsConfig->maxDeadline)) {
                supervisionFailed = TRUE;
            }
        }
    }

    /* Handle supervision failure */
    if (supervisionFailed) {
        WdgM_SEStatuses[seId].failureCounter++;
        
        if (WdgM_SEStatuses[seId].failureCounter > seConfig->failureTolerance) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_EXPIRED);
            WdgM_ReportProductionError(WDGM_E_DEADLINE_SUPERVISION, seId);
        } else if (WdgM_SEStatuses[seId].localStatus == WDGM_LOCAL_STATUS_OK) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_FAILED);
        }
    }
}

/**
 * @brief Check logical supervision for an SE
 */
static void WdgM_CheckLogicalSupervision(WdgM_SupervisedEntityIdType seId)
{
    const WdgM_SupervisedEntityConfigType *seConfig;
    const WdgM_LogicalSupervisionConfigType *lsConfig;

    seConfig = WdgM_GetSEConfig(seId);
    if ((seConfig == NULL) || (!seConfig->isActive)) {
        return;
    }

    lsConfig = seConfig->logicalSupervision;
    if ((lsConfig == NULL) || (!lsConfig->isGraphActive)) {
        return;
    }

    /* Logical supervision is checked during checkpoint transitions */
    /* See WdgM_ValidateCheckpointTransition */
}

/**
 * @brief Validate checkpoint transition for logical supervision
 */
static Std_ReturnType WdgM_ValidateCheckpointTransition(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType fromCp,
    WdgM_CheckpointIdType toCp)
{
    const WdgM_SupervisedEntityConfigType *seConfig;
    const WdgM_LogicalSupervisionConfigType *lsConfig;
    const WdgM_TransitionConfigType *transition;
    boolean transitionValid = FALSE;
    uint8 i;
    Std_ReturnType result = E_NOT_OK;

    seConfig = WdgM_GetSEConfig(seId);
    if (seConfig == NULL) {
        return E_NOT_OK;
    }

    lsConfig = seConfig->logicalSupervision;
    if ((lsConfig == NULL) || (!lsConfig->isGraphActive)) {
        /* No logical supervision, transition is valid */
        return E_OK;
    }

    /* Check if transition is valid */
    for (i = 0U; i < lsConfig->numTransitions; i++) {
        transition = &lsConfig->transitions[i];
        if ((transition->fromCheckpoint == fromCp) &&
            (transition->toCheckpoint == toCp)) {
            transitionValid = TRUE;
            break;
        }
    }

    if (transitionValid) {
        result = E_OK;
    } else {
        /* Invalid transition - report logical supervision failure */
        WdgM_SEStatuses[seId].failureCounter++;
        
        if (WdgM_SEStatuses[seId].failureCounter > seConfig->failureTolerance) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_EXPIRED);
            WdgM_ReportProductionError(WDGM_E_LOGICAL_SUPERVISION, seId);
        } else if (WdgM_SEStatuses[seId].localStatus == WDGM_LOCAL_STATUS_OK) {
            WdgM_TransitionSEStatus(seId, WDGM_LOCAL_STATUS_FAILED);
        }
        result = E_NOT_OK;
    }

    return result;
}

/**
 * @brief Trigger the hardware watchdog
 */
static void WdgM_TriggerWatchdog(void)
{
    if ((WdgM_ConfigPtr != NULL) && (WDGM_TRIGGER_WATCHDOG_ENABLED == STD_ON)) {
        WdgIf_Trigger(WDGIF_DEFAULT_DEVICE);
    }
}

/**
 * @brief Report production error to DEM
 */
static void WdgM_ReportProductionError(uint8 errorCode, WdgM_SupervisedEntityIdType seId)
{
    (void)seId;  /* Parameter may be used for extended error info */
    
    if (WDGM_DEM_REPORTING_ENABLED == STD_ON) {
        switch (errorCode) {
            case WDGM_E_ALIVE_SUPERVISION:
                /* Report to DEM: WDGM_E_ALIVE_SUPERVISION_DEM_ID */
                break;
            case WDGM_E_DEADLINE_SUPERVISION:
                /* Report to DEM: WDGM_E_DEADLINE_SUPERVISION_DEM_ID */
                break;
            case WDGM_E_LOGICAL_SUPERVISION:
                /* Report to DEM: WDGM_E_LOGICAL_SUPERVISION_DEM_ID */
                break;
            default:
                /* Unknown error code */
                break;
        }
    }

    /* Call error callback if registered */
    if ((WdgM_ConfigPtr != NULL) && (WdgM_ConfigPtr->errorCallback != NULL)) {
        WdgM_ConfigPtr->errorCallback(errorCode, seId);
    }
}

/**
 * @brief Process all supervision checks for an SE
 */
static void WdgM_ProcessSESupervision(WdgM_SupervisedEntityIdType seId)
{
    const WdgM_SupervisedEntityConfigType *seConfig;

    seConfig = WdgM_GetSEConfig(seId);
    if ((seConfig == NULL) || (!seConfig->isActive)) {
        return;
    }

    /* Skip if SE is already expired */
    if (WdgM_SEStatuses[seId].localStatus == WDGM_LOCAL_STATUS_EXPIRED) {
        return;
    }

    /* Perform supervision checks */
    if (seConfig->numAliveSupervisions > 0U) {
        WdgM_CheckAliveSupervision(seId);
    }
    
    if (seConfig->numDeadlineSupervisions > 0U) {
        WdgM_CheckDeadlineSupervision(seId);
    }
}

/******************************************************************************
 * Public API Functions
 ******************************************************************************/

/**
 * @brief Initialize WdgM module
 */
Std_ReturnType WdgM_Init(const WdgM_ConfigType *configPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;

    /* Check if already initialized */
    if (WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    /* Check parameter */
    if (configPtr == NULL) {
        return E_NOT_OK;
    }

    /* Store configuration pointer */
    WdgM_ConfigPtr = configPtr;

    /* Initialize module status */
    WdgM_ModuleStatus.initialized = TRUE;
    WdgM_ModuleStatus.currentMode = (WdgM_ModeType)configPtr->initialMode;
    WdgM_ModuleStatus.globalStatus = WDGM_GLOBAL_STATUS_OK;
    WdgM_ModuleStatus.failedCyclesCounter = 0U;
    WdgM_ModuleStatus.firstExpiredSEId = WDGM_INVALID_SE_ID;
    WdgM_ModuleStatus.cycleCounter = 0U;

    /* Find initial mode configuration */
    for (i = 0U; i < configPtr->numModes; i++) {
        if (configPtr->modeConfigs[i].modeId == WdgM_ModuleStatus.currentMode) {
            WdgM_CurrentModeConfig = &configPtr->modeConfigs[i];
            break;
        }
    }

    /* Initialize all SE statuses */
    for (i = 0U; i < WDGM_MAX_SUPERVISED_ENTITIES; i++) {
        WdgM_ClearSEStatus(i);
        
        /* Check if SE is active in current mode */
        if (WdgM_CurrentModeConfig != NULL) {
            const WdgM_SupervisedEntityConfigType *seConfig = WdgM_GetSEConfig(i);
            WdgM_SEStatuses[i].isActive = (seConfig != NULL) ? seConfig->isActive : FALSE;
        } else {
            WdgM_SEStatuses[i].isActive = FALSE;
        }
        
        /* Link runtime status arrays */
        WdgM_SEStatuses[i].aliveStatus = WdgM_AliveStatus[i];
        WdgM_SEStatuses[i].deadlineStatus = WdgM_DeadlineStatus[i];
    }

    /* Set initial watchdog mode */
    if (WdgM_CurrentModeConfig != NULL) {
        (void)WdgIf_SetMode(WDGIF_DEFAULT_DEVICE, 
            (WdgIf_ModeType)WdgM_CurrentModeConfig->triggerMode);
    }

    result = E_OK;
    return result;
}

/**
 * @brief Deinitialize WdgM module
 */
Std_ReturnType WdgM_DeInit(void)
{
    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    /* Set watchdog to OFF mode */
    (void)WdgIf_SetMode(WDGIF_DEFAULT_DEVICE, WDGIF_OFF_MODE);

    /* Clear module status */
    WdgM_ModuleStatus.initialized = FALSE;
    WdgM_ModuleStatus.currentMode = WDGM_MODE_OFF;
    WdgM_ModuleStatus.globalStatus = WDGM_GLOBAL_STATUS_STOPPED;
    WdgM_ConfigPtr = NULL;
    WdgM_CurrentModeConfig = NULL;

    return E_OK;
}

/**
 * @brief Set WdgM mode
 */
Std_ReturnType WdgM_SetMode(WdgM_ModeType mode)
{
    Std_ReturnType result = E_NOT_OK;
    const WdgM_ModeConfigType *newModeConfig = NULL;
    uint8 i;

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    /* Find mode configuration */
    if (WdgM_ConfigPtr != NULL) {
        for (i = 0U; i < WdgM_ConfigPtr->numModes; i++) {
            if (WdgM_ConfigPtr->modeConfigs[i].modeId == mode) {
                newModeConfig = &WdgM_ConfigPtr->modeConfigs[i];
                break;
            }
        }
    }

    if (newModeConfig == NULL) {
        WdgM_ReportProductionError(WDGM_E_SET_MODE, WDGM_INVALID_SE_ID);
        return E_NOT_OK;
    }

    /* Set new watchdog mode */
    result = WdgIf_SetMode(WDGIF_DEFAULT_DEVICE, (WdgIf_ModeType)newModeConfig->triggerMode);
    
    if (result == E_OK) {
        /* Update current mode */
        WdgM_CurrentModeConfig = newModeConfig;
        WdgM_ModuleStatus.currentMode = mode;

        /* Update SE activation status */
        for (i = 0U; i < WDGM_MAX_SUPERVISED_ENTITIES; i++) {
            const WdgM_SupervisedEntityConfigType *seConfig = WdgM_GetSEConfig(i);
            
            if (seConfig != NULL) {
                /* Mode change - clear SE status */
                WdgM_ClearSEStatus(i);
                WdgM_SEStatuses[i].isActive = seConfig->isActive;
            } else {
                WdgM_SEStatuses[i].isActive = FALSE;
            }
        }

        /* Update global status */
        WdgM_UpdateGlobalStatus();
    } else {
        WdgM_ReportProductionError(WDGM_E_SET_MODE, WDGM_INVALID_SE_ID);
    }

    return result;
}

/**
 * @brief Get current WdgM mode
 */
Std_ReturnType WdgM_GetMode(WdgM_ModeType *mode)
{
    /* Check parameters */
    if (mode == NULL) {
        return E_NOT_OK;
    }

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    *mode = WdgM_ModuleStatus.currentMode;
    return E_OK;
}

/**
 * @brief Get global supervision status
 */
WdgM_GlobalStatusType WdgM_GetGlobalStatus(void)
{
    if (!WdgM_ModuleStatus.initialized) {
        return WDGM_GLOBAL_STATUS_STOPPED;
    }

    return WdgM_ModuleStatus.globalStatus;
}

/**
 * @brief Get local supervision status
 */
Std_ReturnType WdgM_GetLocalStatus(
    WdgM_SupervisedEntityIdType seId,
    WdgM_LocalStatusType *status)
{
    /* Check parameters */
    if ((status == NULL) || (seId >= WDGM_MAX_SUPERVISED_ENTITIES)) {
        return E_NOT_OK;
    }

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    *status = WdgM_SEStatuses[seId].localStatus;
    return E_OK;
}

/**
 * @brief Report checkpoint reached (AUTOSAR internal)
 */
Std_ReturnType WdgM_CheckpointReached(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType checkpointId)
{
    return WdgM_CheckpointReachedExtended(seId, checkpointId, WDGM_AUTOSAR_INTERNAL);
}

/**
 * @brief Report checkpoint reached (extended)
 */
Std_ReturnType WdgM_CheckpointReachedExtended(
    WdgM_SupervisedEntityIdType seId,
    WdgM_CheckpointIdType checkpointId,
    WdgM_CheckpointSourceType source)
{
    const WdgM_SupervisedEntityConfigType *seConfig;
    const WdgM_DeadlineSupervisionConfigType *dsConfig;
    const WdgM_AliveSupervisionConfigType *asConfig;
    WdgM_CheckpointIdType lastCp;
    uint32 currentTime;
    uint8 i;
    (void)source;  /* May be used for future extensions */

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    /* Check parameters */
    if ((seId >= WDGM_MAX_SUPERVISED_ENTITIES) || 
        (checkpointId >= WDGM_MAX_CHECKPOINTS)) {
        return E_NOT_OK;
    }

    /* Check if SE is active */
    if (!WdgM_SEStatuses[seId].isActive) {
        return E_NOT_OK;
    }

    seConfig = WdgM_GetSEConfig(seId);
    if (seConfig == NULL) {
        return E_NOT_OK;
    }

    /* Get current time */
    currentTime = (uint32)WdgM_ModuleStatus.cycleCounter * WDGM_MAINFUNCTION_PERIOD_MS;

    /* Process alive supervision */
    for (i = 0U; i < seConfig->numAliveSupervisions; i++) {
        asConfig = &seConfig->aliveSupervisions[i];
        if (asConfig->checkpointId == checkpointId) {
            WdgM_AliveStatus[seId][i].aliveIndications++;
            WdgM_AliveStatus[seId][i].checkpointReachedThisCycle = TRUE;
        }
    }

    /* Process deadline supervision */
    for (i = 0U; i < seConfig->numDeadlineSupervisions; i++) {
        dsConfig = &seConfig->deadlineSupervisions[i];
        
        if (checkpointId == dsConfig->startCheckpoint) {
            /* Start checkpoint reached */
            WdgM_DeadlineStatus[seId][i].startTime = currentTime;
            WdgM_DeadlineStatus[seId][i].startReached = TRUE;
            WdgM_DeadlineStatus[seId][i].endReached = FALSE;
        } else if (checkpointId == dsConfig->endCheckpoint) {
            /* End checkpoint reached */
            if (WdgM_DeadlineStatus[seId][i].startReached) {
                WdgM_DeadlineStatus[seId][i].endReached = TRUE;
            }
        }
    }

    /* Process logical supervision */
    lastCp = WdgM_SEStatuses[seId].logicalStatus.lastCheckpoint;
    if (WdgM_SEStatuses[seId].logicalStatus.lastCheckpointValid) {
        (void)WdgM_ValidateCheckpointTransition(seId, lastCp, checkpointId);
    }
    
    /* Update last checkpoint */
    WdgM_SEStatuses[seId].logicalStatus.lastCheckpoint = checkpointId;
    WdgM_SEStatuses[seId].logicalStatus.lastCheckpointValid = TRUE;

    return E_OK;
}

/**
 * @brief Main function - called cyclically
 */
void WdgM_MainFunction(void)
{
    uint8 i;

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return;
    }

    /* Skip if supervision is disabled */
    if ((WdgM_ConfigPtr != NULL) && (!WdgM_ConfigPtr->supervisionEnabled)) {
        WdgM_TriggerWatchdog();
        return;
    }

    /* Increment cycle counter */
    WdgM_ModuleStatus.cycleCounter++;

    /* Process supervision for all SEs */
    for (i = 0U; i < WDGM_MAX_SUPERVISED_ENTITIES; i++) {
        if (WdgM_SEStatuses[i].isActive) {
            WdgM_ProcessSESupervision(i);
        }
    }

    /* Update global status */
    WdgM_UpdateGlobalStatus();

    /* Trigger watchdog if global status is OK */
    if (WdgM_ModuleStatus.globalStatus == WDGM_GLOBAL_STATUS_OK) {
        WdgM_TriggerWatchdog();
    }
}

/**
 * @brief Update alive counter for an SE
 */
Std_ReturnType WdgM_UpdateAliveCounter(WdgM_SupervisedEntityIdType seId)
{
    uint8 i;

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    /* Check parameter */
    if (seId >= WDGM_MAX_SUPERVISED_ENTITIES) {
        return E_NOT_OK;
    }

    /* Increment alive counters for this SE */
    for (i = 0U; i < 4U; i++) {
        WdgM_AliveStatus[seId][i].aliveIndications++;
        WdgM_AliveStatus[seId][i].checkpointReachedThisCycle = TRUE;
    }

    return E_OK;
}

/**
 * @brief Get first expired SE ID
 */
Std_ReturnType WdgM_GetFirstExpiredSEID(WdgM_SupervisedEntityIdType *seId)
{
    /* Check parameters */
    if (seId == NULL) {
        return E_NOT_OK;
    }

    /* Check if initialized */
    if (!WdgM_ModuleStatus.initialized) {
        return E_NOT_OK;
    }

    *seId = WdgM_ModuleStatus.firstExpiredSEId;
    return E_OK;
}

/**
 * @brief Perform reset on supervision failure
 */
void WdgM_PerformReset(void)
{
    /* Disable interrupts */
    /* This is hardware-specific, typically done via OS or MCU driver */
    
    /* Trigger watchdog expiration to cause reset */
    /* Stop triggering the watchdog */
    WdgM_ModuleStatus.initialized = FALSE;
    
    /* Infinite loop to force watchdog timeout */
    while (1) {
        /* Wait for watchdog to expire */
    }
}

/******************************************************************************
 * BswM Integration Functions
 ******************************************************************************/

/**
 * @brief Request mode change from BswM
 */
void WdgM_RequestMode(WdgM_ModeType mode)
{
    (void)WdgM_SetMode(mode);
}

/**
 * @brief Get current mode for BswM
 */
WdgM_ModeType WdgM_GetCurrentMode(void)
{
    return WdgM_ModuleStatus.currentMode;
}

/******************************************************************************
 * EcuM Integration Functions
 ******************************************************************************/

/**
 * @brief Set mode during startup/shutdown
 */
void WdgM_SetModeForEcuM(WdgM_ModeType mode)
{
    /* EcuM can directly set mode without some checks */
    const WdgM_ModeConfigType *newModeConfig = NULL;
    uint8 i;

    if (WdgM_ConfigPtr == NULL) {
        return;
    }

    /* Find mode configuration */
    for (i = 0U; i < WdgM_ConfigPtr->numModes; i++) {
        if (WdgM_ConfigPtr->modeConfigs[i].modeId == mode) {
            newModeConfig = &WdgM_ConfigPtr->modeConfigs[i];
            break;
        }
    }

    if (newModeConfig != NULL) {
        (void)WdgIf_SetMode(WDGIF_DEFAULT_DEVICE, 
            (WdgIf_ModeType)newModeConfig->triggerMode);
        WdgM_CurrentModeConfig = newModeConfig;
        WdgM_ModuleStatus.currentMode = mode;
    }
}

/******************************************************************************
 * Weak Application Callbacks (to be overridden by application)
 ******************************************************************************/

/**
 * @brief Global status change callback - weak default implementation
 */
__attribute__((weak)) void WdgM_GlobalStatusCallback(
    WdgM_GlobalStatusType newStatus, 
    WdgM_GlobalStatusType oldStatus)
{
    (void)newStatus;
    (void)oldStatus;
    /* Default: no action */
}

/**
 * @brief Local status change callback - weak default implementation
 */
__attribute__((weak)) void WdgM_LocalStatusCallback(
    WdgM_SupervisedEntityIdType seId,
    WdgM_LocalStatusType newStatus, 
    WdgM_LocalStatusType oldStatus)
{
    (void)seId;
    (void)newStatus;
    (void)oldStatus;
    /* Default: no action */
}
