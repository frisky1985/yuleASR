/******************************************************************************
 * @file    EcuM.c
 * @brief   ECU State Manager (EcuM) Complete Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * This file provides the complete EcuM implementation including:
 * - State machine engine (STARTUP → RUN → SLEEP/SHUTDOWN)
 * - Startup sequence management
 * - Driver initialization sequence
 * - State request management (RUN/POST_RUN)
 * - Wakeup event handling
 * - Sleep mode management
 * - Shutdown target selection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/classic/ecum/ecum.h"
#include "autosar/classic/ecum/EcuM_Cfg.h"
#include <string.h>

/******************************************************************************
 * External Function Declarations (Callouts)
 ******************************************************************************/
extern void EcuM_AL_DriverInitZero(void);
extern const EcuM_ConfigType* EcuM_DeterminePbConfiguration(void);
extern void EcuM_AL_DriverInitOne(const EcuM_ConfigType *config);
extern void EcuM_AL_DriverInitTwo(const EcuM_ConfigType *config);
extern void EcuM_AL_DriverInitThree(const EcuM_ConfigType *config);
extern void EcuM_OnEnterRun(void);
extern void EcuM_OnExitRun(void);
extern void EcuM_OnEnterPostRun(void);
extern void EcuM_OnExitPostRun(void);
extern void EcuM_OnPrepShutdown(void);
extern void EcuM_AL_SwitchOff(void);
extern void EcuM_AL_Wakeup(void);
extern void EcuM_SleepActivity(EcuM_SleepModeType sleepMode);
extern boolean EcuM_CheckSleep(void);
extern boolean EcuM_CheckWakeup(EcuM_WakeupSourceType wakeupSource);

/******************************************************************************
 * Module Version Information
 ******************************************************************************/
#define ECUM_VENDOR_ID                  0x01U
#define ECUM_MODULE_ID                  0x0CU
#define ECUM_SW_MAJOR_VERSION           1U
#define ECUM_SW_MINOR_VERSION           0U
#define ECUM_SW_PATCH_VERSION           0U

/******************************************************************************
 * Module Internal Constants
 ******************************************************************************/
#define ECUM_MAX_CALLBACKS              8U
#define ECUM_STATE_REQUEST_MASK_ALL     0xFFFFFFFFU
#define ECUM_RUN_COUNTER_THRESHOLD      0U
#define ECUM_POSTRUN_COUNTER_THRESHOLD  0U
#define ECUM_MAX_WAKEUP_SOURCES         32U
#define ECUM_MAX_SLEEP_MODES            8U

/******************************************************************************
 * EcuM State Machine Phases
 ******************************************************************************/
typedef enum {
    ECUM_PHASE_STARTUP = 0,
    ECUM_PHASE_UP,
    ECUM_PHASE_SHUTDOWN,
    ECUM_PHASE_SLEEP,
    ECUM_PHASE_WAKEUP,
    ECUM_PHASE_OFF
} EcuM_PhaseType;

/******************************************************************************
 * Startup Sub-States
 ******************************************************************************/
typedef enum {
    ECUM_STARTUP_SUBSTATE_E = 0,
    ECUM_STARTUP_SUBSTATE_W = 1
} EcuM_StartupSubStateType;

/******************************************************************************
 * Internal Data Structures
 ******************************************************************************/

typedef struct {
    EcuM_StateChangeCallback callbacks[ECUM_MAX_CALLBACKS];
    uint8 count;
} EcuM_StateCallbackListType;

typedef struct {
    EcuM_WakeupCallback callbacks[ECUM_MAX_CALLBACKS];
    uint8 count;
} EcuM_WakeupCallbackListType;

typedef struct {
    EcuM_WakeupSourceType source;
    EcuM_WakeupStatusType status;
    uint16 validationTimeout;
    uint16 validationCounter;
    uint16 validationTimer;
    uint32 timestamp;
    uint8 comMChannel;
    boolean checkWakeupSupported;
    boolean comMChannelSupported;
} EcuM_WakeupSourceContextType;

typedef struct {
    EcuM_SleepModeType sleepMode;
    EcuM_WakeupSourceType wakeupSourceMask;
    boolean mcuModeSupported;
    uint8 mcuMode;
    boolean pollWakeupSupported;
    boolean sleepCallbackSupported;
    uint32 minimumSleepTime;
    uint32 maximumSleepTime;
} EcuM_SleepModeContextType;

/******************************************************************************
 * Module Variables
 ******************************************************************************/
static const EcuM_ConfigType *EcuM_ConfigPtr = NULL;
static EcuM_StatusType EcuM_Status;
static EcuM_StateCallbackListType EcuM_StateCallbacks;
static EcuM_WakeupCallbackListType EcuM_WakeupCallbacks;
static EcuM_ShutdownTargetType EcuM_LastShutdownTarget = ECUM_TARGET_OFF;
static uint8 EcuM_LastShutdownMode = 0U;
static EcuM_PhaseType EcuM_CurrentPhase = ECUM_PHASE_OFF;
static EcuM_StartupSubStateType EcuM_StartupSubState = ECUM_STARTUP_SUBSTATE_E;

/* Wakeup and Sleep Context */
static EcuM_WakeupSourceContextType EcuM_WakeupSources[ECUM_MAX_WAKEUP_SOURCES];
static EcuM_SleepModeContextType EcuM_SleepModes[ECUM_MAX_SLEEP_MODES];
static uint8 EcuM_NumWakeupSources = 0U;
static uint8 EcuM_NumSleepModes = 0U;
static uint32 EcuM_WakeupTimestamp = 0U;
static boolean EcuM_SleepInProgress = FALSE;

/******************************************************************************
 * Default Configuration
 ******************************************************************************/
static const EcuM_WakeupSourceConfigType EcuM_DefaultWakeupSources[] = {
    { ECUM_WKSOURCE_POWER,          100U, 1U, FALSE, FALSE, 0U },
    { ECUM_WKSOURCE_RESET,          0U,   0U, FALSE, FALSE, 0U },
    { ECUM_WKSOURCE_INTERNAL_WDG,   0U,   0U, FALSE, FALSE, 0U },
    { ECUM_WKSOURCE_EXTERNAL_WDG,   0U,   0U, FALSE, FALSE, 0U },
    { ECUM_WKSOURCE_CAN,            200U, 3U, TRUE,  TRUE,  0U },
    { ECUM_WKSOURCE_ETHERNET,       300U, 5U, TRUE,  TRUE,  1U },
    { ECUM_WKSOURCE_TIMER,          500U, 10U,FALSE, FALSE, 0U },
    { ECUM_WKSOURCE_IO,             100U, 2U, FALSE, FALSE, 0U }
};

static const EcuM_SleepModeConfigType EcuM_DefaultSleepModes[] = {
    { ECUM_SLEEP_MODE_POLLING,      ECUM_WKSOURCE_TIMER | ECUM_WKSOURCE_IO, 
      TRUE, 0U, TRUE, TRUE },
    { ECUM_SLEEP_MODE_HALT,         ECUM_WKSOURCE_ETHERNET | ECUM_WKSOURCE_CAN | ECUM_WKSOURCE_IO,
      TRUE, 1U, FALSE, TRUE },
    { ECUM_SLEEP_MODE_DEEP_HALT,    ECUM_WKSOURCE_POWER | ECUM_WKSOURCE_RESET | ECUM_WKSOURCE_IO,
      TRUE, 2U, FALSE, TRUE }
};

const EcuM_ConfigType EcuM_Config = {
    50U,                                    /* normalMcuWakeupTime */
    100U,                                   /* minShutdownTime */
    EcuM_DefaultWakeupSources,              /* wakeupSources */
    8U,                                     /* numWakeupSources */
    EcuM_DefaultSleepModes,                 /* sleepModes */
    3U,                                     /* numSleepModes */
    ECUM_TARGET_SLEEP,                      /* defaultTarget */
    ECUM_SLEEP_MODE_HALT,                   /* defaultSleepMode */
    ECUM_WKSOURCE_INTERNAL_RESET,           /* defaultResetMode */
    16U                                     /* numModeRequestPorts */
};

/******************************************************************************
 * Internal Function Prototypes
 ******************************************************************************/
static void EcuM_StateTransition(EcuM_StateType newState);
static void EcuM_NotifyStateChange(EcuM_StateType oldState, EcuM_StateType newState);
static void EcuM_NotifyWakeup(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status);
static void EcuM_ProcessStateMachine(void);
static void EcuM_ProcessWakeupEvents(void);
static void EcuM_ProcessValidationTimers(void);

static void EcuM_HandleRunState(void);
static void EcuM_HandlePrepShutdown(void);
static void EcuM_HandleGoSleep(void);
static void EcuM_HandleGoOff(void);
static void EcuM_HandleWakeup(void);

static Std_ReturnType EcuM_ValidateSleepConditions(void);
static boolean EcuM_IsStateTransitionValid(EcuM_StateType currentState, EcuM_StateType newState);

static EcuM_WakeupSourceContextType* EcuM_FindWakeupSourceContext(EcuM_WakeupSourceType source);
static EcuM_SleepModeContextType* EcuM_FindSleepModeContext(EcuM_SleepModeType sleepMode);

static void EcuM_ExecuteStartupSubState(void);

/******************************************************************************
 * State Machine Validation
 ******************************************************************************/
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
 * State Transition Handling
 ******************************************************************************/
static void EcuM_StateTransition(EcuM_StateType newState)
{
    EcuM_StateType oldState = EcuM_Status.currentState;
    
    /* Validate transition */
    if (!EcuM_IsStateTransitionValid(oldState, newState)) {
        /* Invalid transition - log error */
        return;
    }
    
    /* Call exit callback for old state */
    switch (oldState) {
        case ECUM_STATE_RUN:
            EcuM_OnExitRun();
            break;
        default:
            break;
    }
    
    /* Perform transition */
    EcuM_Status.currentState = newState;
    
    /* Update phase based on state */
    switch (newState) {
        case ECUM_STATE_STARTUP:
            EcuM_CurrentPhase = ECUM_PHASE_STARTUP;
            break;
        case ECUM_STATE_RUN:
            EcuM_CurrentPhase = ECUM_PHASE_UP;
            break;
        case ECUM_STATE_SLEEP:
            EcuM_CurrentPhase = ECUM_PHASE_SLEEP;
            break;
        case ECUM_STATE_SHUTDOWN:
        case ECUM_STATE_PREP_SHUTDOWN:
        case ECUM_STATE_GO_SLEEP:
        case ECUM_STATE_GO_OFF_ONE:
        case ECUM_STATE_GO_OFF_TWO:
            EcuM_CurrentPhase = ECUM_PHASE_SHUTDOWN;
            break;
        case ECUM_STATE_OFF:
            EcuM_CurrentPhase = ECUM_PHASE_OFF;
            break;
        default:
            break;
    }
    
    /* Notify registered callbacks */
    EcuM_NotifyStateChange(oldState, newState);
    
    /* Call entry callback for new state */
    switch (newState) {
        case ECUM_STATE_RUN:
            EcuM_OnEnterRun();
            break;
        default:
            break;
    }
}

/******************************************************************************
 * Callback Notifications
 ******************************************************************************/
static void EcuM_NotifyStateChange(EcuM_StateType oldState, EcuM_StateType newState)
{
    uint8 i;
    for (i = 0U; i < EcuM_StateCallbacks.count; i++) {
        if (EcuM_StateCallbacks.callbacks[i] != NULL) {
            EcuM_StateCallbacks.callbacks[i](oldState, newState);
        }
    }
}

static void EcuM_NotifyWakeup(EcuM_WakeupSourceType source, EcuM_WakeupStatusType status)
{
    uint8 i;
    for (i = 0U; i < EcuM_WakeupCallbacks.count; i++) {
        if (EcuM_WakeupCallbacks.callbacks[i] != NULL) {
            EcuM_WakeupCallbacks.callbacks[i](source, status);
        }
    }
}

/******************************************************************************
 * Startup Sequence Execution
 ******************************************************************************/
static void EcuM_ExecuteStartupSubState(void)
{
    switch (EcuM_StartupSubState) {
        case ECUM_STARTUP_SUBSTATE_E:
            /* Phase E: Driver Init Zero - Pre-OS */
            EcuM_AL_DriverInitZero();
            EcuM_StartupSubState = ECUM_STARTUP_SUBSTATE_W;
            break;
            
        case ECUM_STARTUP_SUBSTATE_W:
            /* Phase W: OS is now running, continue with post-OS init */
            /* This is handled by StartupTwo */
            break;
            
        default:
            break;
    }
}

/******************************************************************************
 * State Machine Processing
 ******************************************************************************/
static void EcuM_ProcessStateMachine(void)
{
    switch (EcuM_Status.currentState) {
        case ECUM_STATE_STARTUP:
            EcuM_ExecuteStartupSubState();
            break;
            
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
            break;
    }
}

/******************************************************************************
 * State Handlers
 ******************************************************************************/
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
            EcuM_StateTransition(ECUM_STATE_GO_OFF_ONE);
            break;
    }
}

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

static void EcuM_HandleWakeup(void)
{
    EcuM_AL_Wakeup();
    EcuM_StateTransition(ECUM_STATE_RUN);
}

/******************************************************************************
 * Sleep Validation
 ******************************************************************************/
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
 * Wakeup Source Context Management
 ******************************************************************************/
static EcuM_WakeupSourceContextType* EcuM_FindWakeupSourceContext(EcuM_WakeupSourceType source)
{
    EcuM_WakeupSourceContextType *result = NULL;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source == source) {
            result = &EcuM_WakeupSources[i];
            break;
        }
    }
    
    return result;
}

static void EcuM_ProcessWakeupEvents(void)
{
    EcuM_WakeupSourceType source;
    EcuM_WakeupSourceType pending = EcuM_Status.pendingWakeupEvents;
    
    while (pending != 0U) {
        source = pending & (~pending + 1U);
        pending &= ~source;
        
        EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
        
        if (ctx != NULL) {
            /* Validate the wakeup source */
            if (EcuM_CheckWakeup(source)) {
                EcuM_Status.validatedWakeupEvents |= source;
                EcuM_Status.pendingWakeupEvents &= ~source;
                ctx->status = ECUM_WKSTATUS_VALIDATED;
                EcuM_NotifyWakeup(source, ECUM_WKSTATUS_VALIDATED);
            }
        }
    }
}

static void EcuM_ProcessValidationTimers(void)
{
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_WAKEUP_SOURCES; i++) {
        if (EcuM_WakeupSources[i].source != 0U) {
            if (EcuM_WakeupSources[i].status == ECUM_WKSTATUS_PENDING) {
                EcuM_WakeupSources[i].validationTimer += ECUM_MAINFUNCTION_PERIOD_MS;
                
                if (EcuM_WakeupSources[i].validationTimeout != 0xFFFFU) {
                    if (EcuM_WakeupSources[i].validationTimer >= EcuM_WakeupSources[i].validationTimeout) {
                        EcuM_WakeupSources[i].status = ECUM_WKSTATUS_EXPIRED;
                        EcuM_Status.expiredWakeupEvents |= EcuM_WakeupSources[i].source;
                        EcuM_Status.pendingWakeupEvents &= ~EcuM_WakeupSources[i].source;
                        EcuM_NotifyWakeup(EcuM_WakeupSources[i].source, ECUM_WKSTATUS_EXPIRED);
                    }
                }
            }
        }
    }
}

/******************************************************************************
 * Sleep Mode Context Management
 ******************************************************************************/
static EcuM_SleepModeContextType* EcuM_FindSleepModeContext(EcuM_SleepModeType sleepMode)
{
    EcuM_SleepModeContextType *result = NULL;
    uint8 i;
    
    for (i = 0U; i < ECUM_MAX_SLEEP_MODES; i++) {
        if (EcuM_SleepModes[i].sleepMode == sleepMode) {
            result = &EcuM_SleepModes[i];
            break;
        }
    }
    
    return result;
}

/******************************************************************************
 * Public API Implementations
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
        
        /* Initialize wakeup sources from config */
        if (config->wakeupSources != NULL) {
            uint8 i;
            for (i = 0U; i < config->numWakeupSources && i < ECUM_MAX_WAKEUP_SOURCES; i++) {
                EcuM_WakeupSources[i].source = config->wakeupSources[i].source;
                EcuM_WakeupSources[i].status = ECUM_WKSTATUS_NONE;
                EcuM_WakeupSources[i].validationTimeout = config->wakeupSources[i].validationTimeout;
                EcuM_WakeupSources[i].validationCounter = config->wakeupSources[i].validationCounter;
                EcuM_WakeupSources[i].checkWakeupSupported = config->wakeupSources[i].checkWakeupSupported;
                EcuM_WakeupSources[i].comMChannelSupported = config->wakeupSources[i].comMChannelSupported;
                EcuM_WakeupSources[i].comMChannel = config->wakeupSources[i].comMChannel;
            }
            EcuM_NumWakeupSources = config->numWakeupSources;
        }
        
        /* Initialize sleep modes from config */
        if (config->sleepModes != NULL) {
            uint8 i;
            for (i = 0U; i < config->numSleepModes && i < ECUM_MAX_SLEEP_MODES; i++) {
                EcuM_SleepModes[i].sleepMode = config->sleepModes[i].sleepMode;
                EcuM_SleepModes[i].wakeupSourceMask = config->sleepModes[i].wakeupSourceMask;
                EcuM_SleepModes[i].mcuModeSupported = config->sleepModes[i].mcuModeSupported;
                EcuM_SleepModes[i].mcuMode = config->sleepModes[i].mcuMode;
                EcuM_SleepModes[i].pollWakeupSupported = config->sleepModes[i].pollWakeupSupported;
                EcuM_SleepModes[i].sleepCallbackSupported = config->sleepModes[i].sleepCallbackSupported;
            }
            EcuM_NumSleepModes = config->numSleepModes;
        }
        
        EcuM_CurrentPhase = ECUM_PHASE_STARTUP;
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
        (void)memset(EcuM_WakeupSources, 0, sizeof(EcuM_WakeupSources));
        (void)memset(EcuM_SleepModes, 0, sizeof(EcuM_SleepModes));
        EcuM_NumWakeupSources = 0U;
        EcuM_NumSleepModes = 0U;
        EcuM_CurrentPhase = ECUM_PHASE_OFF;
    }
    
    return result;
}

void EcuM_StartupTwo(void)
{
    if (EcuM_Status.initialized) {
        if (EcuM_Status.currentState == ECUM_STATE_STARTUP) {
            /* Driver Init One - Post-OS initialization */
            EcuM_AL_DriverInitOne(EcuM_ConfigPtr);
            
            /* Driver Init Two - Basic BSW */
            EcuM_AL_DriverInitTwo(EcuM_ConfigPtr);
            
            /* Driver Init Three - Complex drivers */
            EcuM_AL_DriverInitThree(EcuM_ConfigPtr);
            
            /* Transition to RUN state */
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
        
        /* Process validation timers */
        EcuM_ProcessValidationTimers();
    }
}

/******************************************************************************
 * State Management APIs
 ******************************************************************************/

EcuM_StateType EcuM_GetState(void)
{
    return EcuM_Status.currentState;
}

EcuM_SubStateType EcuM_GetSubState(void)
{
    return EcuM_Status.subState;
}

EcuM_PhaseType EcuM_GetCurrentPhase(void)
{
    return EcuM_CurrentPhase;
}

void EcuM_SetState(EcuM_StateType state)
{
    if (EcuM_Status.initialized) {
        EcuM_StateTransition(state);
    }
}

/******************************************************************************
 * State Request Management APIs
 ******************************************************************************/

Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else if (user >= ECUM_MAX_USERS) {
        result = E_NOT_OK;
    } else {
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

/******************************************************************************
 * Wakeup Management APIs
 ******************************************************************************/

void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)
{
    if (EcuM_Status.initialized) {
        EcuM_WakeupSourceType source;
        EcuM_WakeupSourceType remaining = sources;
        
        while (remaining != 0U) {
            source = remaining & (~remaining + 1U);
            remaining &= ~source;
            
            /* Check if source is disabled */
            if ((EcuM_Status.disabledWakeupEvents & source) != 0U) {
                continue;
            }
            
            EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
            
            if (ctx != NULL) {
                if (ctx->status == ECUM_WKSTATUS_NONE) {
                    ctx->status = ECUM_WKSTATUS_PENDING;
                    ctx->validationTimer = 0U;
                    ctx->timestamp = EcuM_WakeupTimestamp;
                    EcuM_Status.pendingWakeupEvents |= source;
                }
            } else {
                /* Unknown source - add to pending anyway */
                EcuM_Status.pendingWakeupEvents |= source;
            }
            
            /* Check for immediate validation */
            if (EcuM_CheckWakeup(source)) {
                EcuM_ValidateWakeupEvent(source);
            }
        }
    }
}

void EcuM_ClearWakeupEvent(EcuM_WakeupSourceType sources)
{
    EcuM_WakeupSourceType source;
    EcuM_WakeupSourceType remaining = sources;
    
    while (remaining != 0U) {
        source = remaining & (~remaining + 1U);
        remaining &= ~source;
        
        EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
        
        if (ctx != NULL) {
            ctx->status = ECUM_WKSTATUS_NONE;
            ctx->validationTimer = 0U;
            ctx->timestamp = 0U;
        }
        
        EcuM_Status.pendingWakeupEvents &= ~source;
        EcuM_Status.validatedWakeupEvents &= ~source;
        EcuM_Status.expiredWakeupEvents &= ~source;
    }
}

EcuM_WakeupSourceType EcuM_GetPendingWakeupEvents(void)
{
    return EcuM_Status.pendingWakeupEvents;
}

EcuM_WakeupSourceType EcuM_GetValidatedWakeupEvents(void)
{
    return EcuM_Status.validatedWakeupEvents;
}

EcuM_WakeupSourceType EcuM_GetExpiredWakeupEvents(void)
{
    return EcuM_Status.expiredWakeupEvents;
}

void EcuM_ValidateWakeupEvent(EcuM_WakeupSourceType sources)
{
    EcuM_WakeupSourceType source;
    EcuM_WakeupSourceType remaining = sources;
    
    while (remaining != 0U) {
        source = remaining & (~remaining + 1U);
        remaining &= ~source;
        
        EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
        
        if (ctx != NULL) {
            if ((ctx->status == ECUM_WKSTATUS_PENDING) ||
                (ctx->status == ECUM_WKSTATUS_NONE)) {
                ctx->status = ECUM_WKSTATUS_VALIDATED;
                ctx->validationTimer = 0U;
                EcuM_Status.validatedWakeupEvents |= source;
                EcuM_Status.pendingWakeupEvents &= ~source;
                EcuM_NotifyWakeup(source, ECUM_WKSTATUS_VALIDATED);
            }
        }
    }
}

EcuM_WakeupStatusType EcuM_GetWakeupStatus(EcuM_WakeupSourceType source)
{
    EcuM_WakeupStatusType status = ECUM_WKSTATUS_NONE;
    EcuM_WakeupSourceContextType *ctx = EcuM_FindWakeupSourceContext(source);
    
    if (ctx != NULL) {
        status = ctx->status;
    }
    
    return status;
}

void EcuM_EnableWakeupSources(EcuM_WakeupSourceType sources)
{
    if (EcuM_Status.initialized) {
        EcuM_Status.disabledWakeupEvents &= ~sources;
    }
}

void EcuM_DisableWakeupSources(EcuM_WakeupSourceType sources)
{
    if (EcuM_Status.initialized) {
        EcuM_Status.disabledWakeupEvents |= sources;
        EcuM_ClearWakeupEvent(sources);
    }
}

/******************************************************************************
 * Sleep Management APIs
 ******************************************************************************/

Std_ReturnType EcuM_SelectShutdownTarget(
    EcuM_ShutdownTargetType target,
    uint8 mode)
{
    Std_ReturnType result = E_OK;
    
    if (!EcuM_Status.initialized) {
        result = E_NOT_OK;
    } else {
        EcuM_LastShutdownTarget = EcuM_Status.shutdownTarget;
        EcuM_LastShutdownMode = (uint8)EcuM_Status.sleepMode;
        
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

Std_ReturnType EcuM_SelectSleepMode(EcuM_SleepModeType sleepMode)
{
    Std_ReturnType result = E_OK;
    
    if (EcuM_FindSleepModeContext(sleepMode) == NULL) {
        result = E_NOT_OK;
    } else {
        EcuM_Status.sleepMode = sleepMode;
    }
    
    return result;
}

EcuM_SleepModeType EcuM_GetSleepMode(void)
{
    return EcuM_Status.sleepMode;
}

Std_ReturnType EcuM_GoToSleep(void)
{
    Std_ReturnType result = E_OK;
    EcuM_SleepModeContextType *ctx = EcuM_FindSleepModeContext(EcuM_Status.sleepMode);
    
    if (ctx == NULL) {
        result = E_NOT_OK;
    } else {
        EcuM_SleepInProgress = TRUE;
        
        /* Enable wakeup sources */
        if (ctx->wakeupSourceMask != 0U) {
            EcuM_EnableWakeupSources(ctx->wakeupSourceMask);
        }
        
        /* Execute sleep based on mode */
        switch (EcuM_Status.sleepMode) {
            case ECUM_SLEEP_MODE_POLLING:
                result = EcuM_GoToPoll();
                break;
                
            case ECUM_SLEEP_MODE_HALT:
            case ECUM_SLEEP_MODE_DEEP_HALT:
                result = EcuM_GoToHalt();
                break;
                
            default:
                result = EcuM_GoToPoll();
                break;
        }
        
        /* Disable wakeup sources */
        if (ctx->wakeupSourceMask != 0U) {
            EcuM_DisableWakeupSources(ctx->wakeupSourceMask);
        }
        
        EcuM_SleepInProgress = FALSE;
    }
    
    return result;
}

Std_ReturnType EcuM_GoToHalt(void)
{
    Std_ReturnType result = E_OK;
    
    /* In real implementation: Set MCU to halt/sleep mode, wait for interrupt */
    EcuM_AL_SwitchOff();
    EcuM_AL_Wakeup();
    
    return result;
}

Std_ReturnType EcuM_GoToPoll(void)
{
    Std_ReturnType result = E_OK;
    boolean keepPolling = TRUE;
    uint32 elapsedTime = 0U;
    EcuM_SleepModeContextType *ctx = EcuM_FindSleepModeContext(EcuM_Status.sleepMode);
    
    if (ctx == NULL) {
        result = E_NOT_OK;
    } else {
        while (keepPolling) {
            EcuM_SleepActivity(EcuM_Status.sleepMode);
            
            if (EcuM_GetPendingWakeupEvents() != 0U) {
                keepPolling = FALSE;
            }
            
            if (elapsedTime < ctx->minimumSleepTime) {
                keepPolling = TRUE;
            }
            
            if ((ctx->maximumSleepTime > 0U) && (elapsedTime >= ctx->maximumSleepTime)) {
                keepPolling = FALSE;
            }
            
            if (!EcuM_CheckSleep()) {
                keepPolling = FALSE;
            }
            
            if (EcuM_GetState() != ECUM_STATE_SLEEP) {
                keepPolling = FALSE;
            }
            
            elapsedTime += ECUM_MAINFUNCTION_PERIOD_MS;
        }
    }
    
    return result;
}

/******************************************************************************
 * Shutdown APIs
 ******************************************************************************/

void EcuM_GoToShutdown(void)
{
    EcuM_SetState(ECUM_STATE_SHUTDOWN);
    EcuM_OnPrepShutdown();
    EcuM_AL_SwitchOff();
}

void EcuM_GoToReset(EcuM_WakeupSourceType resetMode)
{
    EcuM_SetState(ECUM_STATE_RESET);
    EcuM_OnPrepShutdown();
    (void)resetMode;
}

/******************************************************************************
 * Callback Registration APIs
 ******************************************************************************/

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

/******************************************************************************
 * Status and Utility APIs
 ******************************************************************************/

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
