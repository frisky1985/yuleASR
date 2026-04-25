/******************************************************************************
 * @file    dcm_session.h
 * @brief   DCM Session Control Service (0x10) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_SESSION_H
#define DCM_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Session Control Subfunctions (ISO 14229-1:2020 Table 32)
 ******************************************************************************/
#define DCM_SUBFUNC_DEFAULT_SESSION             0x01U
#define DCM_SUBFUNC_PROGRAMMING_SESSION         0x02U
#define DCM_SUBFUNC_EXTENDED_SESSION            0x03U
#define DCM_SUBFUNC_SAFETY_SYSTEM_SESSION       0x04U

/******************************************************************************
 * Session Timing Parameter Response Sizes
 ******************************************************************************/
#define DCM_SESSION_RESPONSE_MIN_SIZE           0x06U
#define DCM_SESSION_RESPONSE_FULL_SIZE          0x06U

/******************************************************************************
 * Session Control Service Result Types
 ******************************************************************************/
typedef enum {
    DCM_SESSION_OK = 0,                     /* Session change successful */
    DCM_SESSION_ERR_NOT_SUPPORTED,          /* Session not supported */
    DCM_SESSION_ERR_CONDITIONS_NOT_CORRECT, /* Conditions not correct */
    DCM_SESSION_ERR_SECURITY_REQUIRED,      /* Security access required */
    DCM_SESSION_ERR_TRANSITION_FAILED,      /* Session transition failed */
    DCM_SESSION_ERR_INTERNAL                /* Internal error */
} Dcm_SessionResultType;

/******************************************************************************
 * Session State Tracking
 ******************************************************************************/

/* Session transition record for audit trail */
typedef struct {
    Dcm_SessionType fromSession;
    Dcm_SessionType toSession;
    uint16_t testerAddress;
    uint64_t timestamp;
    uint8_t securityLevel;
    bool transitionSuccessful;
} Dcm_SessionTransitionRecordType;

/* Session statistics for monitoring */
typedef struct {
    uint32_t sessionEntryCount[DCM_MAX_SESSIONS];   /* Entry count per session */
    uint32_t sessionTimeoutCount;                    /* Timeout count */
    uint64_t totalTimeInSession[DCM_MAX_SESSIONS];   /* Total time per session */
    uint64_t lastEntryTime;                          /* Last entry timestamp */
} Dcm_SessionStatisticsType;

/******************************************************************************
 * Session Control Configuration
 ******************************************************************************/
typedef struct {
    const Dcm_SessionConfigType *sessionConfigs;    /* Array of session configs */
    uint8_t numSessions;                            /* Number of sessions */
    uint16_t defaultTesterAddress;                  /* Default tester address */
    bool enableAuditTrail;                          /* Enable transition logging */
    uint8_t maxAuditRecords;                        /* Max audit records */
    Dcm_SessionChangeCallback sessionChangeCallback; /* Session change callback */
} Dcm_SessionControlConfigType;

/******************************************************************************
 * Session Control Functions
 ******************************************************************************/

/**
 * @brief Initialize session control module
 * 
 * @param config Pointer to session control configuration
 * @return Dcm_ReturnType Initialization result
 * 
 * @note Must be called before using any session control functions
 * @requirement ISO 14229-1:2020 10.2
 */
Dcm_ReturnType Dcm_SessionInit(const Dcm_SessionControlConfigType *config);

/**
 * @brief Process DiagnosticSessionControl (0x10) service request
 * 
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 * 
 * @details Implements UDS service 0x10 for session management
 * @requirement ISO 14229-1:2020 10.2
 */
Dcm_ReturnType Dcm_DiagnosticSessionControl(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Get current diagnostic session
 * 
 * @return Dcm_SessionType Current session type
 */
Dcm_SessionType Dcm_GetCurrentSession(void);

/**
 * @brief Get previous diagnostic session
 * 
 * @return Dcm_SessionType Previous session type
 */
Dcm_SessionType Dcm_GetPreviousSession(void);

/**
 * @brief Set diagnostic session (internal use or direct transition)
 * 
 * @param session Target session type
 * @param testerAddress Address of tester requesting the change
 * @return Dcm_ReturnType Result of session change
 * 
 * @note This function bypasses normal UDS request processing
 */
Dcm_ReturnType Dcm_SetSession(
    Dcm_SessionType session,
    uint16_t testerAddress
);

/**
 * @brief Check if requested session is supported
 * 
 * @param session Session type to check
 * @return bool True if session is supported
 */
bool Dcm_IsSessionSupported(Dcm_SessionType session);

/**
 * @brief Check if session transition is valid
 * 
 * @param fromSession Current session
 * @param toSession Target session
 * @return bool True if transition is valid
 * 
 * @details Validates session transition according to ISO 14229-1
 */
bool Dcm_IsSessionTransitionValid(
    Dcm_SessionType fromSession,
    Dcm_SessionType toSession
);

/**
 * @brief Get session configuration for current session
 * 
 * @param config Pointer to store configuration
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetCurrentSessionConfig(
    const Dcm_SessionConfigType **config
);

/**
 * @brief Get session configuration by session type
 * 
 * @param session Session type
 * @param config Pointer to store configuration
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetSessionConfig(
    Dcm_SessionType session,
    const Dcm_SessionConfigType **config
);

/**
 * @brief Update session timers (call periodically)
 * 
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 * @return Dcm_ReturnType Result of operation
 * 
 * @note Should be called from Dcm_MainFunction or equivalent
 */
Dcm_ReturnType Dcm_SessionTimerUpdate(uint32_t elapsedTimeMs);

/**
 * @brief Get remaining session timeout
 * 
 * @return uint32_t Remaining timeout in milliseconds
 */
uint32_t Dcm_GetSessionTimeoutRemaining(void);

/**
 * @brief Reset session timer (e.g., on TesterPresent reception)
 * 
 * @return Dcm_ReturnType Result of operation
 * 
 * @requirement ISO 14229-1:2020 10.4 (TesterPresent)
 */
Dcm_ReturnType Dcm_ResetSessionTimer(void);

/**
 * @brief Get timing parameters for current session
 * 
 * @param p2ServerMax Pointer to store P2Server_max (ms)
 * @param p2StarServerMax Pointer to store P2*Server_max (ms)
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetSessionTimingParameters(
    uint16_t *p2ServerMax,
    uint16_t *p2StarServerMax
);

/**
 * @brief Set timing parameters for current session
 * 
 * @param p2ServerMax P2Server_max value in ms
 * @param p2StarServerMax P2*Server_max value in ms
 * @return Dcm_ReturnType Result of operation
 * 
 * @requirement ISO 14229-1:2020 10.5 (AccessTimingParameter)
 */
Dcm_ReturnType Dcm_SetSessionTimingParameters(
    uint16_t p2ServerMax,
    uint16_t p2StarServerMax
);

/**
 * @brief Get session statistics
 * 
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetSessionStatistics(
    Dcm_SessionStatisticsType *stats
);

/**
 * @brief Get session transition audit record
 * 
 * @param index Record index (0 = most recent)
 * @param record Pointer to store record
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetSessionTransitionRecord(
    uint8_t index,
    Dcm_SessionTransitionRecordType *record
);

/**
 * @brief Stop current session (return to default)
 * 
 * @return Dcm_ReturnType Result of operation
 * 
 * @note Typically called on communication timeout or error
 */
Dcm_ReturnType Dcm_StopSession(void);

/**
 * @brief Default session timer handler
 * 
 * @details Called when session timer expires, transitions to default session
 */
void Dcm_SessionTimeoutHandler(void);

/**
 * @brief Get S3 timer value
 * 
 * @return uint32_t S3Server timeout in milliseconds
 */
uint32_t Dcm_GetS3ServerTime(void);

/**
 * @brief Reset S3 timer
 * 
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ResetS3Timer(void);

#ifdef __cplusplus
}
#endif

#endif /* DCM_SESSION_H */
