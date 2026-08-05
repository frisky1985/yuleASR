/******************************************************************************
 * @file    sm_internal.h
 * @brief   State Manager Internal Header
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef SM_INTERNAL_H
#define SM_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar_types.h"
#include "autosar_errors.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define SM_MAX_FUNCTION_GROUPS          8U
#define SM_MAX_STATE_CALLBACKS          8U
#define SM_MAX_FG_CALLBACKS             4U
#define SM_MAX_PENDING_REQUESTS         4U
#define SM_DEFAULT_REQUEST_TIMEOUT_MS   5000U
#define SM_STATE_TRANSITION_TIMEOUT_MS  10000U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Machine states */
typedef enum {
    MACHINE_STATE_OFF = 0,
    MACHINE_STATE_STARTUP,
    MACHINE_STATE_DRIVING,
    MACHINE_STATE_PARKING,
    MACHINE_STATE_SHUTDOWN,
    MACHINE_STATE_ERROR
} MachineStateType;

/* Function group names */
typedef enum {
    FUNCTION_GROUP_POWER_MODE = 0,
    FUNCTION_GROUP_COMMUNICATION,
    FUNCTION_GROUP_DIAGNOSTICS,
    FUNCTION_GROUP_LOGGING,
    FUNCTION_GROUP_SAFETY,
    FUNCTION_GROUP_UPDATE,
    FUNCTION_GROUP_COUNT
} FunctionGroupNameType;

/* Function group states */
typedef enum {
    FUNCTION_GROUP_STATE_OFF = 0,
    FUNCTION_GROUP_STATE_STARTING,
    FUNCTION_GROUP_STATE_ON,
    FUNCTION_GROUP_STATE_STOPPING
} FunctionGroupStateType;

/* State request result */
typedef enum {
    STATE_REQUEST_ACCEPTED = 0,
    STATE_REQUEST_REJECTED,
    STATE_REQUEST_TIMEOUT,
    STATE_REQUEST_IN_PROGRESS
} StateRequestResultType;

/* State transition rule */
typedef struct {
    MachineStateType fromState;
    MachineStateType toState;
    bool allowed;
    FunctionGroupStateType requiredFGState;
} StateTransitionRuleType;

/* Function group entry */
typedef struct {
    FunctionGroupNameType name;
    FunctionGroupStateType state;
    bool isActive;
    uint32_t memberCount;
    void (*stateChangeCallback)(FunctionGroupNameType, FunctionGroupStateType);
} FunctionGroupEntryType;

/* Pending state request */
typedef struct {
    bool isActive;
    MachineStateType targetState;
    uint32_t requestTime;
    uint32_t timeoutMs;
    void (*confirmationCallback)(StateRequestResultType);
} PendingRequestType;

/* State machine context */
typedef struct {
    bool isInitialized;
    MachineStateType currentState;
    MachineStateType previousState;
    MachineStateType targetState;
    FunctionGroupEntryType functionGroups[SM_MAX_FUNCTION_GROUPS];
    PendingRequestType pendingRequest;
    void (*stateChangeCallback)(MachineStateType, MachineStateType);
    bool transitionInProgress;
    uint32_t transitionStartTime;
} StateMachineContextType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize state machine */
Std_ReturnType StateMachine_Init(void);

/* Deinitialize state machine */
Std_ReturnType StateMachine_Deinit(void);

/* Check if initialized */
bool StateMachine_IsInitialized(void);

/* Request state transition */
StateRequestResultType StateMachine_RequestTransition(MachineStateType targetState);

/* Get current state */
MachineStateType StateMachine_GetCurrentState(void);

/* Get previous state */
MachineStateType StateMachine_GetPreviousState(void);

/* Check if transition is allowed */
bool StateMachine_IsTransitionAllowed(MachineStateType fromState, MachineStateType toState);

/* Confirm state transition */
Std_ReturnType StateMachine_ConfirmTransition(StateRequestResultType result);

/* Set function group state */
Std_ReturnType StateMachine_SetFGState(FunctionGroupNameType fg, FunctionGroupStateType state);

/* Get function group state */
FunctionGroupStateType StateMachine_GetFGState(FunctionGroupNameType fg);

/* Register state change callback */
Std_ReturnType StateMachine_RegisterCallback(void (*callback)(MachineStateType, MachineStateType));

/* Unregister state change callback */
Std_ReturnType StateMachine_UnregisterCallback(void);

/* Register function group state callback */
Std_ReturnType StateMachine_RegisterFGCallback(FunctionGroupNameType fg,
                                                void (*callback)(FunctionGroupNameType, 
                                                                FunctionGroupStateType));

/* Unregister function group state callback */
Std_ReturnType StateMachine_UnregisterFGCallback(FunctionGroupNameType fg);

/* Cancel pending request */
Std_ReturnType StateMachine_CancelRequest(void);

/* Check if request is pending */
bool StateMachine_IsRequestPending(void);

/* State machine main function (call periodically) */
void StateMachine_MainFunction(void);

/* Get function group name as string */
const char* StateMachine_GetFGNameString(FunctionGroupNameType fg);

/* Get state name as string */
const char* StateMachine_GetStateNameString(MachineStateType state);

/* Get function group state name as string */
const char* StateMachine_GetFGStateNameString(FunctionGroupStateType state);

#ifdef __cplusplus
}
#endif

#endif /* SM_INTERNAL_H */
