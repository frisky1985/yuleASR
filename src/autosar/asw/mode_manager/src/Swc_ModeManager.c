/**
 * @file Swc_ModeManager.c
 * @brief Mode Manager Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: System mode management and state machine coordination
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_ModeManager.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_MODEMANAGER_MODULE_ID           0x86
#define SWC_MODEMANAGER_INSTANCE_ID         0x00

/* Maximum components to manage */
#define MODE_MAX_COMPONENTS                 16

/* Mode transition timeout */
#define MODE_TRANSITION_TIMEOUT_MS          5000

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    Swc_ModeManagerStatusType status;
    Swc_ComponentModeType components[MODE_MAX_COMPONENTS];
    Swc_ModeTransitionRequestType pendingRequest;
    boolean hasPendingRequest;
    uint8 numComponents;
    boolean isInitialized;
} Swc_ModeManagerInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_ModeManagerInternalType swcModeManager = {
    .status = {
        .currentMode = SYSTEM_MODE_OFF,
        .previousMode = SYSTEM_MODE_OFF,
        .requestedMode = SYSTEM_MODE_OFF,
        .systemState = SYSTEM_STATE_OFF,
        .transitionInProgress = FALSE,
        .modeEntryTime = 0,
        .modeDuration = 0
    },
    .components = {{0}},
    .pendingRequest = {0},
    .hasPendingRequest = FALSE,
    .numComponents = 0,
    .isInitialized = FALSE
};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC void Swc_ModeManager_ProcessModeRequest(void);
STATIC void Swc_ModeManager_ExecuteModeTransition(void);
STATIC void Swc_ModeManager_UpdateModeDuration(void);
STATIC sint16 Swc_ModeManager_FindComponent(uint8 componentId);
STATIC boolean Swc_ModeManager_ValidateModeTransition(Swc_SystemModeType currentMode,
                                                       Swc_SystemModeType targetMode);
STATIC void Swc_ModeManager_NotifyAllComponents(Swc_SystemModeType newMode);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Processes pending mode request
 */
STATIC void Swc_ModeManager_ProcessModeRequest(void)
{
    Swc_ModeTransitionRequestType request;

    /* Check for new request via RTE */
    if (Rte_Read_ModeRequest(&request) == RTE_E_OK) {
        /* Store as pending request */
        swcModeManager.pendingRequest = request;
        swcModeManager.hasPendingRequest = TRUE;
    }

    /* Process pending request */
    if (swcModeManager.hasPendingRequest &&
        !swcModeManager.status.transitionInProgress) {

        /* Validate transition */
        if (Swc_ModeManager_ValidateModeTransition(
                swcModeManager.status.currentMode,
                swcModeManager.pendingRequest.targetMode)) {

            /* Start transition */
            swcModeManager.status.requestedMode = swcModeManager.pendingRequest.targetMode;
            swcModeManager.status.transitionInProgress = TRUE;
            swcModeManager.status.systemState = SYSTEM_STATE_INITIALIZING;

            /* Notify all components */
            Swc_ModeManager_NotifyAllComponents(swcModeManager.pendingRequest.targetMode);
        }

        swcModeManager.hasPendingRequest = FALSE;
    }
}

/**
 * @brief Executes mode transition
 */
STATIC void Swc_ModeManager_ExecuteModeTransition(void)
{
    uint32 currentTime;

    if (!swcModeManager.status.transitionInProgress) {
        return;
    }

    currentTime = Rte_GetTime();

    /* Check for timeout */
    if ((currentTime - swcModeManager.status.modeEntryTime) > MODE_TRANSITION_TIMEOUT_MS) {
        /* Transition timeout - force completion */
        swcModeManager.status.transitionInProgress = FALSE;
        swcModeManager.status.systemState = SYSTEM_STATE_ERROR;
        Det_ReportError(SWC_MODEMANAGER_MODULE_ID, SWC_MODEMANAGER_INSTANCE_ID,
                        0x50, RTE_E_NOT_OK);
        return;
    }

    /* Check if all components are ready */
    if (Swc_ModeManager_AreAllComponentsReady()) {
        /* Complete transition */
        swcModeManager.status.previousMode = swcModeManager.status.currentMode;
        swcModeManager.status.currentMode = swcModeManager.status.requestedMode;
        swcModeManager.status.transitionInProgress = FALSE;
        swcModeManager.status.modeEntryTime = currentTime;

        /* Update system state based on mode */
        switch (swcModeManager.status.currentMode) {
            case SYSTEM_MODE_OFF:
                swcModeManager.status.systemState = SYSTEM_STATE_OFF;
                break;
            case SYSTEM_MODE_INIT:
                swcModeManager.status.systemState = SYSTEM_STATE_INITIALIZING;
                break;
            case SYSTEM_MODE_STANDBY:
            case SYSTEM_MODE_NORMAL:
                swcModeManager.status.systemState = SYSTEM_STATE_RUNNING;
                break;
            case SYSTEM_MODE_DIAGNOSTIC:
                swcModeManager.status.systemState = SYSTEM_STATE_READY;
                break;
            case SYSTEM_MODE_SLEEP:
                swcModeManager.status.systemState = SYSTEM_STATE_SHUTDOWN;
                break;
            case SYSTEM_MODE_EMERGENCY:
                swcModeManager.status.systemState = SYSTEM_STATE_DEGRADED;
                break;
            default:
                swcModeManager.status.systemState = SYSTEM_STATE_ERROR;
                break;
        }
    }
}

/**
 * @brief Updates mode duration
 */
STATIC void Swc_ModeManager_UpdateModeDuration(void)
{
    swcModeManager.status.modeDuration = Rte_GetTime() - swcModeManager.status.modeEntryTime;
}

/**
 * @brief Finds component by ID
 */
STATIC sint16 Swc_ModeManager_FindComponent(uint8 componentId)
{
    uint8 i;

    for (i = 0; i < swcModeManager.numComponents; i++) {
        if (swcModeManager.components[i].componentId == componentId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Validates mode transition
 */
STATIC boolean Swc_ModeManager_ValidateModeTransition(Swc_SystemModeType currentMode,
                                                       Swc_SystemModeType targetMode)
{
    /* Define valid transitions */
    switch (currentMode) {
        case SYSTEM_MODE_OFF:
            return (targetMode == SYSTEM_MODE_INIT);

        case SYSTEM_MODE_INIT:
            return (targetMode == SYSTEM_MODE_STANDBY ||
                    targetMode == SYSTEM_MODE_OFF);

        case SYSTEM_MODE_STANDBY:
            return (targetMode == SYSTEM_MODE_NORMAL ||
                    targetMode == SYSTEM_MODE_DIAGNOSTIC ||
                    targetMode == SYSTEM_MODE_OFF);

        case SYSTEM_MODE_NORMAL:
            return (targetMode == SYSTEM_MODE_STANDBY ||
                    targetMode == SYSTEM_MODE_DIAGNOSTIC ||
                    targetMode == SYSTEM_MODE_SLEEP ||
                    targetMode == SYSTEM_MODE_EMERGENCY);

        case SYSTEM_MODE_DIAGNOSTIC:
            return (targetMode == SYSTEM_MODE_NORMAL ||
                    targetMode == SYSTEM_MODE_STANDBY);

        case SYSTEM_MODE_SLEEP:
            return (targetMode == SYSTEM_MODE_OFF ||
                    targetMode == SYSTEM_MODE_INIT);

        case SYSTEM_MODE_EMERGENCY:
            return (targetMode == SYSTEM_MODE_NORMAL ||
                    targetMode == SYSTEM_MODE_OFF);

        default:
            return FALSE;
    }
}

/**
 * @brief Notifies all components of mode change
 */
STATIC void Swc_ModeManager_NotifyAllComponents(Swc_SystemModeType newMode)
{
    uint8 i;
    Swc_ComponentModeType notification;

    for (i = 0; i < swcModeManager.numComponents; i++) {
        swcModeManager.components[i].currentMode = newMode;
        swcModeManager.components[i].modeAcknowledged = FALSE;
        swcModeManager.components[i].modeReady = FALSE;

        /* Send notification via RTE */
        notification = swcModeManager.components[i];
        (void)Rte_Write_ModeNotification(&notification);
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Mode Manager component
 */
void Swc_ModeManager_Init(void)
{
    uint8 i;

    if (swcModeManager.isInitialized) {
        return;
    }

    /* Initialize status */
    swcModeManager.status.currentMode = SYSTEM_MODE_OFF;
    swcModeManager.status.previousMode = SYSTEM_MODE_OFF;
    swcModeManager.status.requestedMode = SYSTEM_MODE_OFF;
    swcModeManager.status.systemState = SYSTEM_STATE_OFF;
    swcModeManager.status.transitionInProgress = FALSE;
    swcModeManager.status.modeEntryTime = Rte_GetTime();
    swcModeManager.status.modeDuration = 0;

    /* Initialize components */
    for (i = 0; i < MODE_MAX_COMPONENTS; i++) {
        swcModeManager.components[i].componentId = i;
        swcModeManager.components[i].currentMode = SYSTEM_MODE_OFF;
        swcModeManager.components[i].modeAcknowledged = FALSE;
        swcModeManager.components[i].modeReady = FALSE;
    }
    swcModeManager.numComponents = MODE_MAX_COMPONENTS;

    /* Initialize request handling */
    swcModeManager.pendingRequest.targetMode = SYSTEM_MODE_OFF;
    swcModeManager.pendingRequest.requestSource = 0;
    swcModeManager.pendingRequest.requestTime = 0;
    swcModeManager.pendingRequest.priority = 0;
    swcModeManager.pendingRequest.isForced = FALSE;
    swcModeManager.hasPendingRequest = FALSE;

    swcModeManager.isInitialized = TRUE;

    Det_ReportError(SWC_MODEMANAGER_MODULE_ID, SWC_MODEMANAGER_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 50ms cyclic runnable
 */
void Swc_ModeManager_50ms(void)
{
    if (!swcModeManager.isInitialized) {
        return;
    }

    /* Process mode requests */
    Swc_ModeManager_ProcessModeRequest();

    /* Execute mode transition */
    Swc_ModeManager_ExecuteModeTransition();

    /* Update mode duration */
    Swc_ModeManager_UpdateModeDuration();

    /* Write status via RTE */
    (void)Rte_Write_SystemMode(&swcModeManager.status.currentMode);
    (void)Rte_Write_SystemState(&swcModeManager.status.systemState);
}

/**
 * @brief Mode switch runnable
 */
void Swc_ModeManager_ModeSwitch(void)
{
    if (!swcModeManager.isInitialized) {
        return;
    }

    /* This runnable is triggered by mode switch events */
    /* Process any pending mode switches */
    Swc_ModeManager_ProcessModeRequest();
}

/**
 * @brief Requests mode transition
 */
Swc_ModeTransitionResultType Swc_ModeManager_RequestModeTransition(
    const Swc_ModeTransitionRequestType* request)
{
    if (request == NULL) {
        return MODE_TRANSITION_REJECTED;
    }

    if (!swcModeManager.isInitialized) {
        return MODE_TRANSITION_REJECTED;
    }

    if (request->targetMode > SYSTEM_MODE_EMERGENCY) {
        return MODE_TRANSITION_REJECTED;
    }

    /* Check if transition is valid */
    if (!Swc_ModeManager_ValidateModeTransition(
            swcModeManager.status.currentMode, request->targetMode)) {
        return MODE_TRANSITION_REJECTED;
    }

    /* Check if forced request or higher priority */
    if (swcModeManager.status.transitionInProgress &&
        !request->isForced &&
        request->priority <= swcModeManager.pendingRequest.priority) {
        return MODE_TRANSITION_REJECTED;
    }

    /* Store request */
    swcModeManager.pendingRequest = *request;
    swcModeManager.hasPendingRequest = TRUE;

    return MODE_TRANSITION_OK;
}

/**
 * @brief Gets current system mode
 */
Rte_StatusType Swc_ModeManager_GetCurrentMode(Swc_SystemModeType* mode)
{
    if (mode == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *mode = swcModeManager.status.currentMode;
    return RTE_E_OK;
}

/**
 * @brief Gets previous system mode
 */
Rte_StatusType Swc_ModeManager_GetPreviousMode(Swc_SystemModeType* mode)
{
    if (mode == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *mode = swcModeManager.status.previousMode;
    return RTE_E_OK;
}

/**
 * @brief Gets system state
 */
Rte_StatusType Swc_ModeManager_GetSystemState(Swc_SystemStateType* state)
{
    if (state == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *state = swcModeManager.status.systemState;
    return RTE_E_OK;
}

/**
 * @brief Gets mode manager status
 */
Rte_StatusType Swc_ModeManager_GetStatus(Swc_ModeManagerStatusType* status)
{
    if (status == NULL) {
        return RTE_E_INVALID;
    }

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    *status = swcModeManager.status;
    return RTE_E_OK;
}

/**
 * @brief Notifies component mode change
 */
Rte_StatusType Swc_ModeManager_NotifyComponentMode(uint8 componentId,
                                                    Swc_SystemModeType mode)
{
    sint16 componentIndex;
    Swc_ComponentModeType notification;

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    componentIndex = Swc_ModeManager_FindComponent(componentId);

    if (componentIndex < 0) {
        return RTE_E_INVALID;
    }

    swcModeManager.components[componentIndex].currentMode = mode;
    swcModeManager.components[componentIndex].modeAcknowledged = FALSE;
    swcModeManager.components[componentIndex].modeReady = FALSE;

    /* Send notification via RTE */
    notification = swcModeManager.components[componentIndex];
    return Rte_Write_ModeNotification(&notification);
}

/**
 * @brief Acknowledges mode change from component
 */
Rte_StatusType Swc_ModeManager_AcknowledgeModeChange(uint8 componentId,
                                                      boolean acknowledged)
{
    sint16 componentIndex;

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    componentIndex = Swc_ModeManager_FindComponent(componentId);

    if (componentIndex < 0) {
        return RTE_E_INVALID;
    }

    swcModeManager.components[componentIndex].modeAcknowledged = acknowledged;
    swcModeManager.components[componentIndex].modeReady = acknowledged;

    return RTE_E_OK;
}

/**
 * @brief Checks if all components are ready
 */
boolean Swc_ModeManager_AreAllComponentsReady(void)
{
    uint8 i;

    for (i = 0; i < swcModeManager.numComponents; i++) {
        if (!swcModeManager.components[i].modeReady) {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Forces mode transition
 */
Rte_StatusType Swc_ModeManager_ForceModeTransition(Swc_SystemModeType targetMode)
{
    Swc_ModeTransitionRequestType request;

    if (!swcModeManager.isInitialized) {
        return RTE_E_UNINIT;
    }

    if (targetMode > SYSTEM_MODE_EMERGENCY) {
        return RTE_E_INVALID;
    }

    /* Create forced request */
    request.targetMode = targetMode;
    request.requestSource = 0xFF;  /* System/Internal */
    request.requestTime = Rte_GetTime();
    request.priority = 0xFF;  /* Highest priority */
    request.isForced = TRUE;

    /* Execute request */
    if (Swc_ModeManager_RequestModeTransition(&request) == MODE_TRANSITION_OK) {
        /* Immediately complete transition */
        swcModeManager.status.previousMode = swcModeManager.status.currentMode;
        swcModeManager.status.currentMode = targetMode;
        swcModeManager.status.transitionInProgress = FALSE;
        swcModeManager.status.modeEntryTime = Rte_GetTime();

        return RTE_E_OK;
    }

    return RTE_E_NOT_OK;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
