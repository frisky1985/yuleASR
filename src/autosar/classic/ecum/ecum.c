/******************************************************************************
 * @file    ecum.c
 * @brief   ECU State Manager (EcuM) Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/classic/ecum/ecum.h"
#include <string.h>

/******************************************************************************
 * Module Internal Constants
 ******************************************************************************/
#define ECUM_MAX_CALLBACKS              8U
#define ECUM_STATE_REQUEST_MASK_ALL     0xFFFFFFFFU
#define ECUM_RUN_COUNTER_THRESHOLD      0U
#define ECUM_POSTRUN_COUNTER_THRESHOLD  0U
#define ECUM_MAINFUNCTION_PERIOD_MS     10U
#define ECUM_VALIDATION_TIMEOUT_DEFAULT 100U

/******************************************************************************
 * Module Internal Types
 ******************************************************************************/
typedef struct {
    EcuM_StateChangeCallback callbacks[ECUM_MAX_CALLBACKS];
    uint8 count;
} EcuM_StateCallbackListType;

typedef struct {
    EcuM_WakeupCallback callbacks[ECUM_MAX_CALLBACKS];
    uint8 count;
} EcuM_WakeupCallbackListType;

/******************************************************************************
 * Module Variables
 ******************************************************************************/
static const EcuM_ConfigType *EcuM_ConfigPtr = NULL;
static EcuM_StatusType EcuM_Status;
static EcuM_StateCallbackListType EcuM_StateCallbacks;
static EcuM_WakeupCallbackListType EcuM_WakeupCallbacks;
static EcuM_ShutdownTargetType EcuM_LastShutdownTarget = ECUM_TARGET_OFF;
static uint8 EcuM_LastShutdownMode = 0U;

/* State transition table - defines valid transitions
 * Using function-based check instead of array due to large state values */
static boolean EcuM_IsStateTransitionValid(EcuM_StateType currentState, EcuM_StateType newState)
{
    boolean valid = FALSE;
    
    switch (currentState) {
        case ECUM_STATE_STARTUP:
            valid = (newState == ECUM_STATE_RUN);
            break;
            
        case ECUM_STATE_RUN:
            valid = ((newState == ECUM_STATE_SLEEP) ||
                     (newState == ECUM_STATE_SHUTDOWN) ||
                     (newState == ECUM_STATE_PREP_SHUTDOWN));
            break;
            
        case ECUM_STATE_SLEEP:
            valid = ((newState == ECUM_STATE_WAKE_SLEEP) ||
                     (newState == ECUM_STATE_RESET));
            break;
            
        case ECUM_STATE_WAKE_SLEEP:
            valid = ((newState == ECUM_STATE_STARTUP) ||
                     (newState == ECUM_STATE_RUN));
            break;
            
        case ECUM_STATE_SHUTDOWN:
            valid = ((newState == ECUM_STATE_OFF) ||
                     (newState == ECUM_STATE_RESET));
            break;
            
        case ECUM_STATE_PREP_SHUTDOWN:
            valid = ((newState == ECUM_STATE_GO_SLEEP) ||
                     (newState == ECUM_STATE_GO_OFF_ONE) ||
                     (newState == ECUM_STATE_GO_OFF_TWO));
            break;
            
        case ECUM_STATE_GO_SLEEP:
            valid = (newState == ECUM_STATE_SLEEP);
            break;
            
        case ECUM_STATE_GO_OFF_ONE:
            valid = (newState == ECUM_STATE_GO_OFF_TWO);
            break;
            
        case ECUM_STATE_GO_OFF_TWO:
            valid = ((newState == ECUM_STATE_OFF) ||
                     (newState == ECUM_STATE_RESET));
            break;
            
        case ECUM_STATE_WAKEUP_ONE:
        case ECUM_STATE_WAKEUP_TWO:
            valid = ((newState == ECUM_STATE_RUN) ||
                     (newState == ECUM_STATE_STARTUP));
            break;
            
        default:
            valid = FALSE;
            break;
    }
    
    return valid;
}

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static void EcuM_StateTransition(EcuM_StateType newState);
static void EcuM_NotifyStateChange(EcuM_StateType oldState, EcuM_StateType newState);
static void EcuM_NotifyWakeup(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status);
static void EcuM_ProcessStateMachine(void);
static void EcuM_ProcessWakeupEvents(void);
static void EcuM_HandleRunState(void);
static void EcuM_HandlePrepShutdown(void);
static void EcuM_HandleGoSleep(void);
static void EcuM_HandleGoOff(void);
static void EcuM_HandleWakeup(void);
static Std_ReturnType EcuM_ValidateSleepConditions(void);

/******************************************************************************
 * Internal Function Implementations
 ******************************************************************************/

/**
 * @brief Perform state transition with callbacks
 */
static void EcuM_StateTransition(EcuM_StateType newState)
{
    EcuM_StateType oldState = EcuM_Status.currentState;
    
    /* Validate transition */
    if (!EcuM_IsStateTransitionValid(oldState, newState)) {
        /* Invalid transition - log error (would use DET in production) */
        return;
    }
    
    /* Call exit callback for old state */
    switch (oldState) {
        case ECUM_STATE_RUN:
            EcuM_OnExitRun();
            break;
        default:
            /* No exit callback for other states */
            break;
    }
    
    /* Perform transition */
    EcuM_Status.currentState = newState;
    
    /* Notify registered callbacks */
    EcuM_NotifyStateChange(oldState, newState);
    
    /* Call entry callback for new state */
    switch (newState) {
        case ECUM_STATE_RUN:
            EcuM_OnEnterRun();
            break;
        default:
            /* No entry callback for other states */
            break;
    }
}

/**
 * @brief Notify all registered state change callbacks
 */
static void EcuM_NotifyStateChange(EcuM_StateType oldState, EcuM_StateType newState)
{
    uint8 i;
    for (i = 0U; i < EcuM_StateCallbacks.count; i++) {
        if (EcuM_StateCallbacks.callbacks[i] != NULL) {
            EcuM_StateCallbacks.callbacks[i](oldState, newState);
        }
    }
}

/**
 * @brief Notify all registered wakeup callbacks
 */
static void EcuM_NotifyWakeup(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status)
{
    uint8 i;
    for (i = 0U; i < EcuM_WakeupCallbacks.count; i++) {
        if (EcuM_WakeupCallbacks.callbacks[i] != NULL) {
            EcuM_WakeupCallbacks.callbacks[i](source, status);
        }
    }
}

/**
 * @brief Process state machine based on current state
 */
static void EcuM_ProcessStateMachine(void)
{
    switch (EcuM_Status.currentState) {
        case ECUM_STATE_RUN:
            EcuM_HandleRunState();
            break;
            
        case ECUM_STATE_PREP_SHUTDOWN:
            EcuM_HandlePrepShutdown();
            break;
            
        case ECUM_STATE_GO_SLEEP:
            EcuM_HandleGoSleep();
            break;
            
        case ECUM_STATE_GO_OFF_ONE:
        case ECUM_STATE_GO_OFF_TWO:
            EcuM_HandleGoOff();
            break;
            
        case ECUM_STATE_WAKE_SLEEP:
        case ECUM_STATE_WAKEUP_ONE:
        case ECUM_STATE_WAKEUP_TWO:
            EcuM_HandleWakeup();
            break;
            
        default:
            /* No action needed for other states */
            break;
    }
}

/**
 * @brief Handle RUN state - check for RUN requests
 */
static void EcuM_HandleRunState(void)
{
    /* Check if all RUN requests are released */
    if ((EcuM_Status.runCounter == ECUM_RUN_COUNTER_THRESHOLD) &&
        (EcuM_Status.stateRequestMask == 0U)) {
        /* Check if POST RUN is requested */
        if (EcuM_Status.postRunCounter > ECUM_POSTRUN_COUNTER_THRESHOLD) {
            EcuM_OnExitRun();
            EcuM_OnEnterPostRun();
            EcuM_Status.subState = ECUM_SUBSTATE_RUN_APP;
        } else {
            /* Transition to PREP_SHUTDOWN */
            EcuM_StateTransition(ECUM_STATE_PREP_SHUTDOWN);
        }
    }
}

/**
 * @brief Handle PREP_SHUTDOWN state
 */
static void EcuM_HandlePrepShutdown(void)
{
    EcuM_OnPrepShutdown();
    
    /* Determine target based on selection */
    switch (EcuM_Status.shutdownTarget) {
        case ECUM_TARGET_SLEEP:
            EcuM_StateTransition(ECUM_STATE_GO_SLEEP);
            break;
            
        case ECUM_TARGET_OFF:
            EcuM_StateTransition(ECUM_STATE_GO_OFF_ONE);
            break;
            
        case ECUM_TARGET_RESET:
            EcuM_GoToReset(ECUM_WKSOURCE_INTERNAL_RESET);
            break;
            
        default:
            /* Default to OFF */
            EcuM_StateTransition(ECUM_STATE_GO_OFF_ONE);
            break;
    }
}

/**
 * @brief Handle GO_SLEEP state
 */
static void EcuM_HandleGoSleep(void)
{
    /* Validate sleep conditions */
    if (EcuM_ValidateSleepConditions() == E_OK) {
        /* Go to actual sleep */
        EcuM_StateTransition(ECUM_STATE_SLEEP);
        (void)EcuM_GoToSleep();
    } else {
        /* Conditions not met, return to RUN */
        EcuM_StateTransition(ECUM_STATE_RUN);
    }
}

/**
 * @brief Handle GO_OFF states
 */
static void EcuM_HandleGoOff(void)
{
    if (EcuM_Status.currentState == ECUM_STATE_GO_OFF_ONE) {
        /* Shutdown phase one - notify modules */
        EcuM_StateTransition(ECUM_STATE_GO_OFF_TWO);
    } else if (EcuM_Status.currentState == ECUM_STATE_GO_OFF_TWO) {
        /* Final shutdown phase */
        EcuM_AL_SwitchOff();
        EcuM_StateTransition(ECUM_STATE_OFF);
    }
}

/**
 * @brief Handle WAKEUP states
 */
static void EcuM_HandleWakeup(void)
{
    EcuM_AL_Wakeup();
    EcuM_StateTransition(ECUM_STATE_RUN);
}

/**
 * @brief Process pending wakeup events
 */
static void EcuM_ProcessWakeupEvents(void)
{
    EcuM_WakeupSourceType source;
    EcuM_WakeupSourceType pending = EcuM_Status.pendingWakeupEvents;
    
    /* Process each pending wakeup event */
    while (pending != 0U) {
        /* Get lowest bit set */
        source = pending & (~pending + 1U);
        pending &= ~source;
        
        /* Validate the wakeup source */
        if (EcuM_CheckWakeup(source)) {
            EcuM_Status.validatedWakeupEvents |= source;
            EcuM_Status.pendingWakeupEvents &= ~source;
            EcuM_NotifyWakeup(source, ECUM_WKSTATUS_VALIDATED);
        } else {
            /* Check for timeout - if expired, mark as expired */
            /* In real implementation, would check timestamp */
            EcuM_Status.expiredWakeupEvents |= source;
            EcuM_Status.pendingWakeupEvents &= ~source;
            EcuM_NotifyWakeup(source, ECUM_WKSTATUS_EXPIRED);
        }
    }
}

/**
 * @brief Validate sleep conditions
 */
static Std_ReturnType EcuM_ValidateSleepConditions(void)
{
    Std_ReturnType result = E_OK;
    
    /* Check if sleep is allowed */
    if (!EcuM_CheckSleep()) {
        result = E_NOT_OK;
    }
    
    /* Check if there are any pending RUN requests */
    if ((EcuM_Status.runCounter > ECUM_RUN_COUNTER_THRESHOLD) ||
        (EcuM_Status.stateRequestMask != 0U)) {
        result = E_NOT_OK;
    }
    
    return result;
}

/******************************************************************************
 * External API Implementations
 ******************************************************************************/

Std_ReturnType EcuM_Init(const EcuM_ConfigType *config)
{
    Std_ReturnType result = E_OK;
    
    if (config == NULL) {
        result = E_NOT_OK;
    } else if (EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else {
        /* Store configuration */
        EcuM_ConfigPtr = config;
        
        /* Initialize status */
        (void)memset(&EcuM_Status, 0, sizeof(EcuM_StatusType));
        EcuM_Status.currentState = ECUM_STATE_STARTUP;
        EcuM_Status.subState = ECUM_SUBSTATE_STARTUP_FIRST;
        EcuM_Status.shutdownTarget = config->defaultTarget;
        EcuM_Status.sleepMode = config->defaultSleepMode;
        
        /* Initialize callback lists */
        (void)memset(&EcuM_StateCallbacks, 0, sizeof(EcuM_StateCallbackListType));
        (void)memset(&EcuM_WakeupCallbacks, 0, sizeof(EcuM_WakeupCallbackListType));
        
        EcuM_Status.initialized = TRUE;
    }
    
    return result;
}

Std_ReturnType EcuM_DeInit(void)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else {
        EcuM_ConfigPtr = NULL;
        (void)memset(&EcuM_Status, 0, sizeof(EcuM_StatusType));
        (void)memset(&EcuM_StateCallbacks, 0, sizeof(EcuM_StateCallbackListType));
        (void)memset(&EcuM_WakeupCallbacks, 0, sizeof(EcuM_WakeupCallbackListType));
    }
    
    return result;
}

void EcuM_StartupTwo(void)
{
    if (EcuM_Status.initialized) {
        /* Transition from STARTUP to RUN */
        if (EcuM_Status.currentState == ECUM_STATE_STARTUP) {
            EcuM_StateTransition(ECUM_STATE_RUN);
            EcuM_Status.subState = ECUM_SUBSTATE_RUN_APP;
        }
    }
}

void EcuM_EnterRunMode(void)
{
    if (EcuM_Status.initialized) {
        if (EcuM_Status.currentState == ECUM_STATE_STARTUP) {
            EcuM_StartupTwo();
        } else if (EcuM_Status.currentState == ECUM_STATE_WAKE_SLEEP) {
            EcuM_StateTransition(ECUM_STATE_RUN);
        }
    }
}

void EcuM_MainFunction(void)
{
    if (EcuM_Status.initialized) {
        /* Process state machine */
        EcuM_ProcessStateMachine();
        
        /* Process wakeup events */
        EcuM_ProcessWakeupEvents();
    }
}

EcuM_StateType EcuM_GetState(void)
{
    return EcuM_Status.currentState;
}

EcuM_SubStateType EcuM_GetSubState(void)
{
    return EcuM_Status.subState;
}

void EcuM_SetState(EcuM_StateType state)
{
    if (EcuM_Status.initialized) {
        EcuM_StateTransition(state);
    }
}

Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else if (user >= ECUM_MAX_USERS) {
        result = E_NOT_OK;
    } else {
        /* Set bit in request mask */
        EcuM_Status.stateRequestMask |= (1UL << (uint32)user);
        EcuM_Status.runCounter++;
    }
    
    return result;
}

Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else if (user >= ECUM_MAX_USERS) {
        result = E_NOT_OK;
    } else {
        /* Clear bit in request mask */
        EcuM_Status.stateRequestMask &= ~(1UL << (uint32)user);
        if (EcuM_Status.runCounter > 0U) {
            EcuM_Status.runCounter--;
        }
    }
    
    return result;
}

Std_ReturnType EcuM_RequestPOST_RUN(EcuM_UserType user)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else if (user >= ECUM_MAX_USERS) {
        result = E_NOT_OK;
    } else {
        EcuM_Status.postRunCounter++;
    }
    
    return result;
}

Std_ReturnType EcuM_ReleasePOST_RUN(EcuM_UserType user)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else if (user >= ECUM_MAX_USERS) {
        result = E_NOT_OK;
    } else {
        if (EcuM_Status.postRunCounter > 0U) {
            EcuM_Status.postRunCounter--;
        }
    }
    
    return result;
}

Std_ReturnType EcuM_KillAllRUNRequests(void)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else {
        EcuM_Status.stateRequestMask = 0U;
        EcuM_Status.runCounter = 0U;
        EcuM_Status.postRunCounter = 0U;
    }
    
    return result;
}

Std_ReturnType EcuM_SelectShutdownTarget(
    EcuM_ShutdownTargetType target,
    uint8 mode)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else {
        /* Store current target as last target */
        EcuM_LastShutdownTarget = EcuM_Status.shutdownTarget;
        EcuM_LastShutdownMode = (uint8)EcuM_Status.sleepMode;
        
        /* Set new target */
        EcuM_Status.shutdownTarget = target;
        EcuM_Status.sleepMode = mode;
    }
    
    return result;
}

Std_ReturnType EcuM_GetShutdownTarget(
    EcuM_ShutdownTargetType *target,
    uint8 *mode)
{
    Std_ReturnType result = E_OK;
    
    if ((target == NULL) || (mode == NULL)) {
        result = E_NOT_OK;
    } else if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else {
        *target = EcuM_Status.shutdownTarget;
        *mode = (uint8)EcuM_Status.sleepMode;
    }
    
    return result;
}

Std_ReturnType EcuM_GetLastShutdownTarget(
    EcuM_ShutdownTargetType *target,
    uint8 *mode)
{
    Std_ReturnType result = E_OK;
    
    if ((target == NULL) || (mode == NULL)) {
        result = E_NOT_OK;
    } else {
        *target = EcuM_LastShutdownTarget;
        *mode = EcuM_LastShutdownMode;
    }
    
    return result;
}

Std_ReturnType EcuM_RegisterStateChangeCallback(EcuM_StateChangeCallback callback)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    if (callback != NULL) {
        for (i = 0U; i < ECUM_MAX_CALLBACKS; i++) {
            if (EcuM_StateCallbacks.callbacks[i] == NULL) {
                EcuM_StateCallbacks.callbacks[i] = callback;
                EcuM_StateCallbacks.count++;
                result = E_OK;
                break;
            }
        }
    }
    
    return result;
}

Std_ReturnType EcuM_UnregisterStateChangeCallback(EcuM_StateChangeCallback callback)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    if (callback != NULL) {
        for (i = 0U; i < ECUM_MAX_CALLBACKS; i++) {
            if (EcuM_StateCallbacks.callbacks[i] == callback) {
                EcuM_StateCallbacks.callbacks[i] = NULL;
                if (EcuM_StateCallbacks.count > 0U) {
                    EcuM_StateCallbacks.count--;
                }
                result = E_OK;
                break;
            }
        }
    }
    
    return result;
}

Std_ReturnType EcuM_RegisterWakeupCallback(EcuM_WakeupCallback callback)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    if (callback != NULL) {
        for (i = 0U; i < ECUM_MAX_CALLBACKS; i++) {
            if (EcuM_WakeupCallbacks.callbacks[i] == NULL) {
                EcuM_WakeupCallbacks.callbacks[i] = callback;
                EcuM_WakeupCallbacks.count++;
                result = E_OK;
                break;
            }
        }
    }
    
    return result;
}

Std_ReturnType EcuM_UnregisterWakeupCallback(EcuM_WakeupCallback callback)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    if (callback != NULL) {
        for (i = 0U; i < ECUM_MAX_CALLBACKS; i++) {
            if (EcuM_WakeupCallbacks.callbacks[i] == callback) {
                EcuM_WakeupCallbacks.callbacks[i] = NULL;
                if (EcuM_WakeupCallbacks.count > 0U) {
                    EcuM_WakeupCallbacks.count--;
                }
                result = E_OK;
                break;
            }
        }
    }
    
    return result;
}

const EcuM_StatusType* EcuM_GetStatus(void)
{
    return &EcuM_Status;
}

boolean EcuM_IsInitialized(void)
{
    return EcuM_Status.initialized;
}

void EcuM_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = ECUM_VENDOR_ID;
        version->moduleID = ECUM_MODULE_ID;
        version->sw_major_version = ECUM_SW_MAJOR_VERSION;
        version->sw_minor_version = ECUM_SW_MINOR_VERSION;
        version->sw_patch_version = ECUM_SW_PATCH_VERSION;
    }
}

/******************************************************************************
 * Default Weak Application Callbacks
 ******************************************************************************/
__attribute__((weak)) void EcuM_AL_DriverInitZero(void) {}
__attribute__((weak)) const EcuM_ConfigType* EcuM_DeterminePbConfiguration(void) { return NULL; }
__attribute__((weak)) void EcuM_AL_DriverInitOne(const EcuM_ConfigType *config) { (void)config; }
__attribute__((weak)) void EcuM_OnEnterRun(void) {}
__attribute__((weak)) void EcuM_OnExitRun(void) {}
__attribute__((weak)) void EcuM_OnEnterPostRun(void) {}
__attribute__((weak)) void EcuM_OnExitPostRun(void) {}
__attribute__((weak)) void EcuM_OnPrepShutdown(void) {}
__attribute__((weak)) void EcuM_AL_SwitchOff(void) {}
__attribute__((weak)) void EcuM_AL_Wakeup(void) {}
__attribute__((weak)) void EcuM_SleepActivity(EcuM_SleepModeType sleepMode) { (void)sleepMode; }
__attribute__((weak)) boolean EcuM_CheckSleep(void) { return TRUE; }
__attribute__((weak)) boolean EcuM_CheckWakeup(EcuM_WakeupSourceType wakeupSource) { (void)wakeupSource; return TRUE; }
__attribute__((weak)) uint8 EcuM_GetBootTarget(void) { return 0U; }
__attribute__((weak)) void EcuM_SetBootTarget(uint8 target) { (void)target; }
__attribute__((weak)) void EcuM_GoToShutdown(void) {}
__attribute__((weak)) void EcuM_GoToReset(EcuM_WakeupSourceType resetMode) { (void)resetMode; }
__attribute__((weak)) Std_ReturnType EcuM_GoToSleep(void) { return E_OK; }
__attribute__((weak)) Std_ReturnType EcuM_GoToHalt(void) { return E_OK; }
__attribute__((weak)) Std_ReturnType EcuM_GoToPoll(void) { return E_OK; }
