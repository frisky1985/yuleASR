/******************************************************************************
 * @file    state_machine.c
 * @brief   State Machine Manager Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/sm/sm_internal.h"
#include <string.h>

/******************************************************************************
 * Private Data
 ******************************************************************************/

static StateMachineContextType g_context;

/* State transition rules table */
static const StateTransitionRuleType g_transitionRules[] = {
    /* From OFF */
    { MACHINE_STATE_OFF, MACHINE_STATE_STARTUP, true, FUNCTION_GROUP_STATE_ON },
    
    /* From STARTUP */
    { MACHINE_STATE_STARTUP, MACHINE_STATE_DRIVING, true, FUNCTION_GROUP_STATE_ON },
    { MACHINE_STATE_STARTUP, MACHINE_STATE_SHUTDOWN, true, FUNCTION_GROUP_STATE_OFF },
    { MACHINE_STATE_STARTUP, MACHINE_STATE_ERROR, true, FUNCTION_GROUP_STATE_OFF },
    
    /* From DRIVING */
    { MACHINE_STATE_DRIVING, MACHINE_STATE_PARKING, true, FUNCTION_GROUP_STATE_ON },
    { MACHINE_STATE_DRIVING, MACHINE_STATE_SHUTDOWN, true, FUNCTION_GROUP_STATE_OFF },
    { MACHINE_STATE_DRIVING, MACHINE_STATE_ERROR, true, FUNCTION_GROUP_STATE_OFF },
    
    /* From PARKING */
    { MACHINE_STATE_PARKING, MACHINE_STATE_DRIVING, true, FUNCTION_GROUP_STATE_ON },
    { MACHINE_STATE_PARKING, MACHINE_STATE_SHUTDOWN, true, FUNCTION_GROUP_STATE_OFF },
    { MACHINE_STATE_PARKING, MACHINE_STATE_ERROR, true, FUNCTION_GROUP_STATE_OFF },
    
    /* From ERROR */
    { MACHINE_STATE_ERROR, MACHINE_STATE_SHUTDOWN, true, FUNCTION_GROUP_STATE_OFF },
    { MACHINE_STATE_ERROR, MACHINE_STATE_STARTUP, true, FUNCTION_GROUP_STATE_ON },
    
    /* From SHUTDOWN */
    { MACHINE_STATE_SHUTDOWN, MACHINE_STATE_OFF, true, FUNCTION_GROUP_STATE_OFF }
};

static const uint32_t g_numTransitionRules = sizeof(g_transitionRules) / sizeof(g_transitionRules[0]);

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static void InitializeFunctionGroups(void) {
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        g_context.functionGroups[i].name = (FunctionGroupNameType)i;
        g_context.functionGroups[i].state = FUNCTION_GROUP_STATE_OFF;
        g_context.functionGroups[i].isActive = false;
        g_context.functionGroups[i].memberCount = 0U;
        g_context.functionGroups[i].stateChangeCallback = NULL;
    }
}

static void NotifyStateChange(MachineStateType oldState, MachineStateType newState) {
    if (g_context.stateChangeCallback != NULL) {
        g_context.stateChangeCallback(oldState, newState);
    }
}

static void NotifyFGStateChange(FunctionGroupNameType fg, FunctionGroupStateType state) {
    FunctionGroupEntryType* entry = &g_context.functionGroups[fg];
    if (entry->stateChangeCallback != NULL) {
        entry->stateChangeCallback(fg, state);
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Std_ReturnType StateMachine_Init(void) {
    if (g_context.isInitialized) {
        return E_OK;
    }
    
    memset(&g_context, 0, sizeof(StateMachineContextType));
    
    g_context.currentState = MACHINE_STATE_OFF;
    g_context.previousState = MACHINE_STATE_OFF;
    g_context.targetState = MACHINE_STATE_OFF;
    g_context.transitionInProgress = false;
    g_context.stateChangeCallback = NULL;
    
    InitializeFunctionGroups();
    
    /* Activate all function groups */
    for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
        g_context.functionGroups[i].isActive = true;
    }
    
    g_context.isInitialized = true;
    return E_OK;
}

Std_ReturnType StateMachine_Deinit(void) {
    if (!g_context.isInitialized) {
        return E_OK;
    }
    
    /* Transition to OFF state if not already */
    if (g_context.currentState != MACHINE_STATE_OFF) {
        StateMachine_RequestTransition(MACHINE_STATE_SHUTDOWN);
        /* In real implementation, should wait for completion */
    }
    
    memset(&g_context, 0, sizeof(StateMachineContextType));
    return E_OK;
}

bool StateMachine_IsInitialized(void) {
    return g_context.isInitialized;
}

StateRequestResultType StateMachine_RequestTransition(MachineStateType targetState) {
    if (!g_context.isInitialized) {
        return STATE_REQUEST_REJECTED;
    }
    
    /* Check if transition is already in progress */
    if (g_context.transitionInProgress) {
        return STATE_REQUEST_REJECTED;
    }
    
    /* Check if already in target state */
    if (g_context.currentState == targetState) {
        return STATE_REQUEST_ACCEPTED;
    }
    
    /* Check if transition is allowed */
    if (!StateMachine_IsTransitionAllowed(g_context.currentState, targetState)) {
        return STATE_REQUEST_REJECTED;
    }
    
    /* Start the transition */
    g_context.transitionInProgress = true;
    g_context.targetState = targetState;
    g_context.previousState = g_context.currentState;
    g_context.transitionStartTime = 0U;  /* Would use actual timestamp */
    
    /* Store the request */
    g_context.pendingRequest.isActive = true;
    g_context.pendingRequest.targetState = targetState;
    g_context.pendingRequest.requestTime = 0U;
    g_context.pendingRequest.timeoutMs = SM_STATE_TRANSITION_TIMEOUT_MS;
    g_context.pendingRequest.confirmationCallback = NULL;
    
    return STATE_REQUEST_ACCEPTED;
}

MachineStateType StateMachine_GetCurrentState(void) {
    return g_context.isInitialized ? g_context.currentState : MACHINE_STATE_OFF;
}

MachineStateType StateMachine_GetPreviousState(void) {
    return g_context.isInitialized ? g_context.previousState : MACHINE_STATE_OFF;
}

bool StateMachine_IsTransitionAllowed(MachineStateType fromState, MachineStateType toState) {
    for (uint32_t i = 0U; i < g_numTransitionRules; i++) {
        if (g_transitionRules[i].fromState == fromState &&
            g_transitionRules[i].toState == toState) {
            return g_transitionRules[i].allowed;
        }
    }
    return false;
}

Std_ReturnType StateMachine_ConfirmTransition(StateRequestResultType result) {
    if (!g_context.isInitialized || !g_context.transitionInProgress) {
        return E_NOT_OK;
    }
    
    if (result == STATE_REQUEST_ACCEPTED) {
        MachineStateType oldState = g_context.currentState;
        g_context.currentState = g_context.targetState;
        
        NotifyStateChange(oldState, g_context.currentState);
        
        /* Reset function groups based on new state */
        if (g_context.currentState == MACHINE_STATE_OFF ||
            g_context.currentState == MACHINE_STATE_SHUTDOWN) {
            for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
                StateMachine_SetFGState((FunctionGroupNameType)i, FUNCTION_GROUP_STATE_OFF);
            }
        }
    }
    
    g_context.transitionInProgress = false;
    g_context.pendingRequest.isActive = false;
    
    return E_OK;
}

Std_ReturnType StateMachine_SetFGState(FunctionGroupNameType fg, FunctionGroupStateType state) {
    if (!g_context.isInitialized) {
        return E_NOT_OK;
    }
    
    if (fg >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    FunctionGroupEntryType* entry = &g_context.functionGroups[fg];
    
    if (entry->state == state) {
        return E_OK;
    }
    
    FunctionGroupStateType oldState = entry->state;
    entry->state = state;
    
    /* Notify callback */
    NotifyFGStateChange(fg, state);
    
    /* Update active member count based on state */
    if (state == FUNCTION_GROUP_STATE_ON) {
        entry->isActive = true;
    } else if (state == FUNCTION_GROUP_STATE_OFF) {
        entry->isActive = false;
    }
    
    (void)oldState;
    return E_OK;
}

FunctionGroupStateType StateMachine_GetFGState(FunctionGroupNameType fg) {
    if (!g_context.isInitialized || fg >= FUNCTION_GROUP_COUNT) {
        return FUNCTION_GROUP_STATE_OFF;
    }
    
    return g_context.functionGroups[fg].state;
}

Std_ReturnType StateMachine_RegisterCallback(void (*callback)(MachineStateType, MachineStateType)) {
    if (!g_context.isInitialized) {
        return E_NOT_OK;
    }
    
    g_context.stateChangeCallback = callback;
    return E_OK;
}

Std_ReturnType StateMachine_UnregisterCallback(void) {
    if (!g_context.isInitialized) {
        return E_NOT_OK;
    }
    
    g_context.stateChangeCallback = NULL;
    return E_OK;
}

Std_ReturnType StateMachine_RegisterFGCallback(FunctionGroupNameType fg,
                                                void (*callback)(FunctionGroupNameType,
                                                                FunctionGroupStateType)) {
    if (!g_context.isInitialized || fg >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    g_context.functionGroups[fg].stateChangeCallback = callback;
    return E_OK;
}

Std_ReturnType StateMachine_UnregisterFGCallback(FunctionGroupNameType fg) {
    if (!g_context.isInitialized || fg >= FUNCTION_GROUP_COUNT) {
        return E_NOT_OK;
    }
    
    g_context.functionGroups[fg].stateChangeCallback = NULL;
    return E_OK;
}

Std_ReturnType StateMachine_CancelRequest(void) {
    if (!g_context.isInitialized || !g_context.transitionInProgress) {
        return E_NOT_OK;
    }
    
    g_context.transitionInProgress = false;
    g_context.pendingRequest.isActive = false;
    g_context.targetState = g_context.currentState;
    
    return E_OK;
}

bool StateMachine_IsRequestPending(void) {
    return g_context.isInitialized && g_context.transitionInProgress;
}

void StateMachine_MainFunction(void) {
    if (!g_context.isInitialized) {
        return;
    }
    
    /* Check for pending requests */
    if (g_context.pendingRequest.isActive) {
        /* Check for timeout */
        uint32_t currentTime = 0U;  /* Would use actual timestamp */
        
        if (currentTime - g_context.pendingRequest.requestTime > g_context.pendingRequest.timeoutMs) {
            /* Request timed out */
            if (g_context.pendingRequest.confirmationCallback != NULL) {
                g_context.pendingRequest.confirmationCallback(STATE_REQUEST_TIMEOUT);
            }
            g_context.pendingRequest.isActive = false;
            g_context.transitionInProgress = false;
        }
    }
    
    /* Check for completed transitions */
    if (g_context.transitionInProgress) {
        bool allFGStable = true;
        
        for (uint32_t i = 0U; i < FUNCTION_GROUP_COUNT; i++) {
            FunctionGroupEntryType* fg = &g_context.functionGroups[i];
            
            if (!fg->isActive) {
                continue;
            }
            
            /* Check if function group has reached stable state */
            if (fg->state == FUNCTION_GROUP_STATE_STARTING ||
                fg->state == FUNCTION_GROUP_STATE_STOPPING) {
                allFGStable = false;
                break;
            }
        }
        
        if (allFGStable) {
            /* Complete the transition */
            StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED);
        }
    }
}

const char* StateMachine_GetFGNameString(FunctionGroupNameType fg) {
    static const char* names[] = {
        "PowerMode",
        "Communication",
        "Diagnostics",
        "Logging",
        "Safety",
        "Update"
    };
    
    if (fg < FUNCTION_GROUP_COUNT) {
        return names[fg];
    }
    return "Unknown";
}

const char* StateMachine_GetStateNameString(MachineStateType state) {
    switch (state) {
        case MACHINE_STATE_OFF:
            return "Off";
        case MACHINE_STATE_STARTUP:
            return "Startup";
        case MACHINE_STATE_DRIVING:
            return "Driving";
        case MACHINE_STATE_PARKING:
            return "Parking";
        case MACHINE_STATE_SHUTDOWN:
            return "Shutdown";
        case MACHINE_STATE_ERROR:
            return "Error";
        default:
            return "Unknown";
    }
}

const char* StateMachine_GetFGStateNameString(FunctionGroupStateType state) {
    switch (state) {
        case FUNCTION_GROUP_STATE_OFF:
            return "Off";
        case FUNCTION_GROUP_STATE_STARTING:
            return "Starting";
        case FUNCTION_GROUP_STATE_ON:
            return "On";
        case FUNCTION_GROUP_STATE_STOPPING:
            return "Stopping";
        default:
            return "Unknown";
    }
}
