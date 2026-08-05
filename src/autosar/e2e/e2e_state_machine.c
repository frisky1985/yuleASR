/******************************************************************************
 * @file    e2e_state_machine.c
 * @brief   AUTOSAR E2E State Machine (E2E_SM) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#include "e2e_state_machine.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Private Definitions
 ******************************************************************************/
#define E2E_SM_INITIALIZED_MAGIC        0x4532534DU  /* "E2SM" */

/******************************************************************************
 * Private Data
 ******************************************************************************/
static uint32_t g_initMagic = 0;
static bool g_initialized = FALSE;

/******************************************************************************
 * Private Functions - State Transitions
 ******************************************************************************/

/**
 * @brief Get next state based on current state and check result
 */
static E2E_SM_StateType E2E_SM_GetNextState(
    E2E_SM_StateType currentState,
    E2E_PCheckStatusType checkResult,
    const E2E_SM_InternalStateType* state,
    const E2E_SM_ConfigType* config)
{
    switch (currentState) {
        case E2E_SM_DEINIT:
            /* Cannot transition from DEINIT without init */
            return E2E_SM_DEINIT;

        case E2E_SM_NODATA:
            /* First data received */
            if (checkResult == E2E_P_OK) {
                return E2E_SM_INIT;
            } else if (checkResult == E2E_P_ERROR) {
                return E2E_SM_INVALID;
            }
            return E2E_SM_NODATA;

        case E2E_SM_INIT:
            /* Waiting for sync/validation */
            if (checkResult == E2E_P_OK) {
                if (E2E_SM_IsWindowOkThresholdMet(state, &config->window)) {
                    return E2E_SM_VALID;
                }
                return E2E_SM_INIT;
            } else if (checkResult == E2E_P_ERROR) {
                if (E2E_SM_IsWindowErrorThresholdExceeded(state, &config->window)) {
                    return E2E_SM_INVALID;
                }
                return E2E_SM_INIT;
            }
            return E2E_SM_INIT;

        case E2E_SM_VALID:
            /* Normal operation */
            if (checkResult == E2E_P_OK) {
                return E2E_SM_VALID;
            } else if (checkResult == E2E_P_ERROR) {
                if (E2E_SM_IsWindowErrorThresholdExceeded(state, &config->window)) {
                    return E2E_SM_INVALID;
                }
                return E2E_SM_SYNC;
            } else if (checkResult == E2E_P_REPEATED || checkResult == E2E_P_WRONGSEQUENCE) {
                return E2E_SM_SYNC;
            }
            return E2E_SM_VALID;

        case E2E_SM_INVALID:
            /* Error state, trying to recover */
            if (checkResult == E2E_P_OK) {
                if (E2E_SM_IsWindowOkThresholdMet(state, &config->window)) {
                    return E2E_SM_VALID;
                }
                return E2E_SM_SYNC;
            }
            return E2E_SM_INVALID;

        case E2E_SM_SYNC:
            /* Synchronization in progress */
            if (checkResult == E2E_P_OK) {
                if (state->syncCounter >= config->window.syncCounterThreshold) {
                    return E2E_SM_VALID;
                }
                return E2E_SM_SYNC;
            } else if (checkResult == E2E_P_ERROR) {
                if (E2E_SM_IsWindowErrorThresholdExceeded(state, &config->window)) {
                    return E2E_SM_INVALID;
                }
                return E2E_SM_SYNC;
            }
            return E2E_SM_SYNC;

        default:
            return E2E_SM_INVALID;
    }
}

/******************************************************************************
 * Public Functions - Initialization
 ******************************************************************************/

/**
 * @brief Initialize the E2E State Machine module
 */
Std_ReturnType E2E_SM_Init(void)
{
    if (g_initialized) {
        return E_OK;
    }

    g_initMagic = E2E_SM_INITIALIZED_MAGIC;
    g_initialized = TRUE;

    return E_OK;
}

/**
 * @brief Deinitialize the E2E State Machine module
 */
Std_ReturnType E2E_SM_Deinit(void)
{
    g_initialized = FALSE;
    g_initMagic = 0;
    return E_OK;
}

/**
 * @brief Check if E2E State Machine is initialized
 */
bool E2E_SM_IsInitialized(void)
{
    return g_initialized && (g_initMagic == E2E_SM_INITIALIZED_MAGIC);
}

/******************************************************************************
 * Public Functions - State Machine Management
 ******************************************************************************/

/**
 * @brief Initialize a state machine instance
 */
Std_ReturnType E2E_SM_InitStateMachine(
    const E2E_SM_ConfigType* config,
    E2E_SM_InternalStateType* state)
{
    if (!E2E_SM_IsInitialized() || config == NULL || state == NULL) {
        return E_NOT_OK;
    }

    /* Validate configuration */
    if (config->window.windowSize < E2E_SM_WINDOW_SIZE_MIN ||
        config->window.windowSize > E2E_SM_WINDOW_SIZE_MAX) {
        return E_NOT_OK;
    }

    memset(state, 0, sizeof(E2E_SM_InternalStateType));
    state->state = E2E_SM_NODATA;
    state->errorCounter = 0;
    state->okCounter = 0;
    state->syncCounter = 0;
    state->isAvailable = FALSE;
    state->lastCheckStatus = E2E_ERROR_NONE;

    return E_OK;
}

/**
 * @brief Reset state machine to initial state
 */
Std_ReturnType E2E_SM_Reset(E2E_SM_InternalStateType* state)
{
    if (!E2E_SM_IsInitialized() || state == NULL) {
        return E_NOT_OK;
    }

    state->state = E2E_SM_NODATA;
    state->errorCounter = 0;
    state->okCounter = 0;
    state->syncCounter = 0;
    state->isAvailable = FALSE;
    state->lastCheckStatus = E2E_ERROR_NONE;

    return E_OK;
}

/**
 * @brief Process E2E check result and update state machine
 */
E2E_SM_StateType E2E_SM_ProcessCheckResult(
    E2E_SM_InternalStateType* state,
    const E2E_SM_ConfigType* config,
    E2E_PCheckStatusType checkResult,
    uint32_t timestamp)
{
    if (!E2E_SM_IsInitialized() || state == NULL || config == NULL) {
        return E2E_SM_DEINIT;
    }

    bool isOk = (checkResult == E2E_P_OK);

    /* Update window counters */
    (void)E2E_SM_UpdateWindow(state, &config->window, isOk);

    /* Update sync counter */
    if (isOk) {
        state->syncCounter++;
    }

    /* Store last check status */
    state->lastCheckStatus = (uint16_t)checkResult;

    /* Check for timeout */
    if (config->timeoutMs > 0 && state->lastValidTimestamp > 0) {
        uint32_t elapsed = timestamp - state->lastValidTimestamp;
        if (elapsed > config->timeoutMs) {
            state->state = E2E_SM_INVALID;
            state->isAvailable = FALSE;
            return state->state;
        }
    }

    /* Determine next state */
    E2E_SM_StateType nextState = E2E_SM_GetNextState(
        state->state, checkResult, state, config);

    /* Update timestamp on valid messages */
    if (isOk) {
        state->lastValidTimestamp = timestamp;
    }

    /* Update availability flag */
    state->isAvailable = (nextState == E2E_SM_VALID ||
                          nextState == E2E_SM_SYNC ||
                          nextState == E2E_SM_INIT);

    state->state = nextState;
    return nextState;
}

/**
 * @brief Check message with window verification
 */
Std_ReturnType E2E_SM_Check(
    E2E_SM_InternalStateType* state,
    const E2E_SM_ConfigType* config,
    E2E_PCheckStatusType profileResult,
    uint32_t timestamp,
    E2E_SM_CheckResultType* result)
{
    if (!E2E_SM_IsInitialized() || state == NULL || config == NULL || result == NULL) {
        return E_NOT_OK;
    }

    /* Process the check result */
    E2E_SM_StateType newState = E2E_SM_ProcessCheckResult(
        state, config, profileResult, timestamp);

    /* Fill result structure */
    result->state = newState;
    result->isValid = E2E_SM_IsDataValid(newState);
    result->isAvailable = state->isAvailable;
    result->errorCounter = state->errorCounter;
    result->windowFillLevel = (state->okCounter + state->errorCounter > config->window.windowSize) ?
                              config->window.windowSize :
                              (state->okCounter + state->errorCounter);

    return E_OK;
}

/******************************************************************************
 * Public Functions - Window Checking
 ******************************************************************************/

/**
 * @brief Update window with new message status
 */
uint8_t E2E_SM_UpdateWindow(
    E2E_SM_InternalStateType* state,
    const E2E_SM_WindowConfigType* config,
    bool isOk)
{
    if (state == NULL || config == NULL) {
        return 0;
    }

    /* Update counters */
    if (isOk) {
        state->okCounter++;
        /* Decrement error counter on successful message (sliding window) */
        if (state->errorCounter > 0) {
            state->errorCounter--;
        }
    } else {
        state->errorCounter++;
        /* Decrement OK counter on error (sliding window) */
        if (state->okCounter > 0) {
            state->okCounter--;
        }
    }

    /* Cap counters at window size */
    if (state->errorCounter > config->windowSize) {
        state->errorCounter = config->windowSize;
    }
    if (state->okCounter > config->windowSize) {
        state->okCounter = config->windowSize;
    }

    return state->errorCounter;
}

/**
 * @brief Check if window threshold exceeded
 */
bool E2E_SM_IsWindowErrorThresholdExceeded(
    const E2E_SM_InternalStateType* state,
    const E2E_SM_WindowConfigType* config)
{
    if (state == NULL || config == NULL) {
        return TRUE;
    }

    return (state->errorCounter >= config->maxErrorThreshold);
}

/**
 * @brief Check if enough OK messages received
 */
bool E2E_SM_IsWindowOkThresholdMet(
    const E2E_SM_InternalStateType* state,
    const E2E_SM_WindowConfigType* config)
{
    if (state == NULL || config == NULL) {
        return FALSE;
    }

    return (state->okCounter >= config->minOkThreshold);
}

/******************************************************************************
 * Public Functions - State Queries
 ******************************************************************************/

/**
 * @brief Get state machine state as string
 */
const char* E2E_SM_StateToString(E2E_SM_StateType state)
{
    switch (state) {
        case E2E_SM_DEINIT:
            return "DEINIT";
        case E2E_SM_NODATA:
            return "NODATA";
        case E2E_SM_INIT:
            return "INIT";
        case E2E_SM_INVALID:
            return "INVALID";
        case E2E_SM_VALID:
            return "VALID";
        case E2E_SM_SYNC:
            return "SYNC";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Check if data is valid in current state
 */
bool E2E_SM_IsDataValid(E2E_SM_StateType state)
{
    return (state == E2E_SM_VALID);
}

/**
 * @brief Check if data is available in current state
 */
bool E2E_SM_IsDataAvailable(E2E_SM_StateType state)
{
    return (state == E2E_SM_VALID ||
            state == E2E_SM_SYNC ||
            state == E2E_SM_INIT);
}

/**
 * @brief Get state machine version
 */
const char* E2E_SM_GetVersion(void)
{
    static char version[32];
    (void)snprintf(version, sizeof(version), "%d.%d.%d",
                     E2E_SM_MAJOR_VERSION, E2E_SM_MINOR_VERSION, E2E_SM_PATCH_VERSION);
    return version;
}
