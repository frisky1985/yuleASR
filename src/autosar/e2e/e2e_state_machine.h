/******************************************************************************
 * @file    e2e_state_machine.h
 * @brief   AUTOSAR E2E State Machine (E2E_SM) Header
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * E2E State Machine manages the communication state based on received
 * message verification results. Implements window checking for robust
 * error detection.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef E2E_STATE_MACHINE_H
#define E2E_STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "autosar_types.h"
#include "e2e_protection.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* E2E State Machine Version */
#define E2E_SM_MAJOR_VERSION            22
#define E2E_SM_MINOR_VERSION            11
#define E2E_SM_PATCH_VERSION            0

/* Window size definitions */
#define E2E_SM_WINDOW_SIZE_MIN          1U
#define E2E_SM_WINDOW_SIZE_DEFAULT      3U
#define E2E_SM_WINDOW_SIZE_MAX          15U

/* Error thresholds */
#define E2E_SM_MAX_ERROR_THRESHOLD      3U
#define E2E_SM_OK_THRESHOLD             1U
#define E2E_SM_SYNC_THRESHOLD           2U

/******************************************************************************
 * Data Types
 ******************************************************************************/

/* E2E State Machine states */
typedef enum {
    E2E_SM_DEINIT = 0,          /* Not initialized */
    E2E_SM_NODATA,              /* No data received yet */
    E2E_SM_INIT,                /* Initial state, waiting for sync */
    E2E_SM_INVALID,             /* Data invalid, errors detected */
    E2E_SM_VALID,               /* Data valid, communication OK */
    E2E_SM_SYNC                 /* Synchronizing after error */
} E2E_SM_StateType;

/* Window check configuration */
typedef struct {
    uint8_t windowSize;             /* Window size for error detection */
    uint8_t maxErrorThreshold;      /* Max errors before state change */
    uint8_t minOkThreshold;         /* Min OK messages to go to VALID */
    uint8_t syncCounterThreshold;   /* Max sync attempts */
} E2E_SM_WindowConfigType;

/* State machine configuration */
typedef struct {
    uint8_t profile;                /* E2E Profile ID */
    E2E_SM_WindowConfigType window; /* Window configuration */
    uint32_t timeoutMs;             /* Timeout for freshness */
    bool enableWindowCheck;         /* Enable window checking */
} E2E_SM_ConfigType;

/* State machine internal state */
typedef struct {
    E2E_SM_StateType state;         /* Current state */
    uint8_t errorCounter;           /* Consecutive errors in window */
    uint8_t okCounter;              /* Consecutive OK messages */
    uint8_t syncCounter;            /* Sync attempts counter */
    uint32_t lastValidTimestamp;    /* Last valid message timestamp */
    bool isAvailable;               /* Data availability flag */
    uint16_t lastCheckStatus;       /* Last E2E check status */
} E2E_SM_InternalStateType;

/* State machine check results */
typedef struct {
    E2E_SM_StateType state;         /* Current state */
    bool isValid;                   /* Data is valid */
    bool isAvailable;               /* New data available */
    uint8_t errorCounter;           /* Current error count */
    uint8_t windowFillLevel;        /* Current window fill level */
} E2E_SM_CheckResultType;

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize the E2E State Machine module
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_SM_Init(void);

/**
 * @brief Deinitialize the E2E State Machine module
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_SM_Deinit(void);

/**
 * @brief Check if E2E State Machine is initialized
 * @return TRUE if initialized, FALSE otherwise
 */
bool E2E_SM_IsInitialized(void);

/******************************************************************************
 * Function Prototypes - State Machine Management
 ******************************************************************************/

/**
 * @brief Initialize a state machine instance
 * @param config State machine configuration
 * @param state Internal state storage
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_SM_InitStateMachine(
    const E2E_SM_ConfigType* config,
    E2E_SM_InternalStateType* state);

/**
 * @brief Reset state machine to initial state
 * @param state Internal state storage
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_SM_Reset(E2E_SM_InternalStateType* state);

/**
 * @brief Process E2E check result and update state machine
 * @param state State machine internal state
 * @param checkResult Result from E2E_Protect/E2E_Check
 * @param timestamp Current timestamp (ms)
 * @return Updated state
 */
E2E_SM_StateType E2E_SM_ProcessCheckResult(
    E2E_SM_InternalStateType* state,
    const E2E_SM_ConfigType* config,
    E2E_PCheckStatusType checkResult,
    uint32_t timestamp);

/**
 * @brief Check message with window verification
 * @param state State machine internal state
 * @param config State machine configuration
 * @param profileResult E2E profile check result
 * @param timestamp Current timestamp (ms)
 * @param result Output check result structure
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType E2E_SM_Check(
    E2E_SM_InternalStateType* state,
    const E2E_SM_ConfigType* config,
    E2E_PCheckStatusType profileResult,
    uint32_t timestamp,
    E2E_SM_CheckResultType* result);

/******************************************************************************
 * Function Prototypes - Window Checking
 ******************************************************************************/

/**
 * @brief Update window with new message status
 * @param state State machine internal state
 * @param config Window configuration
 * @param isOk TRUE if message OK, FALSE if error
 * @return Current error counter value
 */
uint8_t E2E_SM_UpdateWindow(
    E2E_SM_InternalStateType* state,
    const E2E_SM_WindowConfigType* config,
    bool isOk);

/**
 * @brief Check if window threshold exceeded
 * @param state State machine internal state
 * @param config Window configuration
 * @return TRUE if too many errors in window
 */
bool E2E_SM_IsWindowErrorThresholdExceeded(
    const E2E_SM_InternalStateType* state,
    const E2E_SM_WindowConfigType* config);

/**
 * @brief Check if enough OK messages received
 * @param state State machine internal state
 * @param config Window configuration
 * @return TRUE if enough OK messages
 */
bool E2E_SM_IsWindowOkThresholdMet(
    const E2E_SM_InternalStateType* state,
    const E2E_SM_WindowConfigType* config);

/******************************************************************************
 * Function Prototypes - State Queries
 ******************************************************************************/

/**
 * @brief Get state machine state as string
 * @param state State value
 * @return State name string
 */
const char* E2E_SM_StateToString(E2E_SM_StateType state);

/**
 * @brief Check if data is valid in current state
 * @param state Current state
 * @return TRUE if data is valid
 */
bool E2E_SM_IsDataValid(E2E_SM_StateType state);

/**
 * @brief Check if data is available in current state
 * @param state Current state
 * @return TRUE if data is available
 */
bool E2E_SM_IsDataAvailable(E2E_SM_StateType state);

/**
 * @brief Get state machine version
 * @return Version string
 */
const char* E2E_SM_GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* E2E_STATE_MACHINE_H */
