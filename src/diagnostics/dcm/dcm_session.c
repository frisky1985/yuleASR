/******************************************************************************
 * @file    dcm_session.c
 * @brief   DCM Session Control Service (0x10) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_session.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_SESSION_MAGIC_INIT          0x44434D53U  /* "DCMS" */

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;                                 /* Initialization marker */
    const Dcm_SessionControlConfigType *config;     /* Configuration pointer */
    Dcm_SessionType currentSession;                 /* Current session */
    Dcm_SessionType previousSession;                /* Previous session */
    uint32_t sessionTimer;                          /* Session timeout timer */
    uint32_t s3Timer;                               /* S3Server timer */
    uint16_t testerSourceAddress;                   /* Current tester address */
    uint16_t currentP2ServerMax;                    /* Current P2Server_max */
    uint16_t currentP2StarServerMax;                /* Current P2*Server_max */
    bool sessionTimerActive;                        /* Session timer state */
    bool s3TimerActive;                             /* S3 timer state */
    Dcm_SessionStatisticsType statistics;           /* Session statistics */
    Dcm_SessionTransitionRecordType auditRecords[8]; /* Audit trail (circular buffer) */
    uint8_t auditIndex;                             /* Current audit index */
    uint64_t sessionEntryTime;                      /* Time when entered current session */
} Dcm_SessionStateType;

/******************************************************************************
 * Static Data
 ******************************************************************************/
static Dcm_SessionStateType s_sessionState;
static bool s_initialized = false;

/******************************************************************************
 * Default Session Configurations
 ******************************************************************************/
static const Dcm_SessionConfigType s_defaultSessionConfigs[] = {
    {
        .sessionType = DCM_SESSION_DEFAULT,
        .timing = {
            .p2ServerMax = DCM_DEFAULT_P2_SERVER_MAX,
            .p2StarServerMax = DCM_DEFAULT_P2STAR_SERVER_MAX,
            .s3Server = DCM_DEFAULT_S3_SERVER
        },
        .isDefaultSession = true,
        .sessionTimeoutMs = 0,  /* No timeout in default session */
        .supportedSecurityLevels = 0x01,  /* Level 0 (unlocked) only */
        .suppressPosResponseAllowed = true
    },
    {
        .sessionType = DCM_SESSION_PROGRAMMING,
        .timing = {
            .p2ServerMax = 50,   /* 50ms */
            .p2StarServerMax = 5000,  /* 5000ms */
            .s3Server = 5000
        },
        .isDefaultSession = false,
        .sessionTimeoutMs = 5000,
        .supportedSecurityLevels = 0x03,  /* Level 0 and 1 */
        .suppressPosResponseAllowed = true
    },
    {
        .sessionType = DCM_SESSION_EXTENDED,
        .timing = {
            .p2ServerMax = 50,
            .p2StarServerMax = 5000,
            .s3Server = 5000
        },
        .isDefaultSession = false,
        .sessionTimeoutMs = 5000,
        .supportedSecurityLevels = 0x07,  /* Level 0, 1, 2 */
        .suppressPosResponseAllowed = true
    },
    {
        .sessionType = DCM_SESSION_SAFETY_SYSTEM,
        .timing = {
            .p2ServerMax = 25,   /* Faster response required */
            .p2StarServerMax = 5000,
            .s3Server = 5000
        },
        .isDefaultSession = false,
        .sessionTimeoutMs = 5000,
        .supportedSecurityLevels = 0x0F,  /* Level 0-3 */
        .suppressPosResponseAllowed = false  /* Always respond in safety session */
    }
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get session configuration index
 */
static int32_t getSessionConfigIndex(Dcm_SessionType session)
{
    if (s_sessionState.config == NULL) {
        return -1;
    }
    
    for (uint8_t i = 0; i < s_sessionState.config->numSessions; i++) {
        if (s_sessionState.config->sessionConfigs[i].sessionType == session) {
            return (int32_t)i;
        }
    }
    return -1;
}

/**
 * @brief Record session transition in audit log
 */
static void recordSessionTransition(
    Dcm_SessionType fromSession,
    Dcm_SessionType toSession,
    bool successful
)
{
    if (!s_sessionState.config->enableAuditTrail) {
        return;
    }
    
    Dcm_SessionTransitionRecordType *record = 
        &s_sessionState.auditRecords[s_sessionState.auditIndex];
    
    record->fromSession = fromSession;
    record->toSession = toSession;
    record->testerAddress = s_sessionState.testerSourceAddress;
    record->timestamp = 0;  /* TODO: Get current timestamp */
    record->securityLevel = 0;  /* TODO: Get current security level */
    record->transitionSuccessful = successful;
    
    s_sessionState.auditIndex = 
        (s_sessionState.auditIndex + 1) % s_sessionState.config->maxAuditRecords;
}

/**
 * @brief Update statistics when entering new session
 */
static void updateStatisticsOnEntry(Dcm_SessionType session)
{
    int32_t idx = getSessionConfigIndex(session);
    if (idx >= 0) {
        s_sessionState.statistics.sessionEntryCount[idx]++;
    }
    
    s_sessionState.sessionEntryTime = 0;  /* TODO: Get current timestamp */
    s_sessionState.statistics.lastEntryTime = s_sessionState.sessionEntryTime;
}

/**
 * @brief Update statistics when leaving session
 */
static void updateStatisticsOnExit(Dcm_SessionType session)
{
    int32_t idx = getSessionConfigIndex(session);
    if (idx >= 0) {
        uint64_t duration = 0;  /* TODO: Calculate duration */
        s_sessionState.statistics.totalTimeInSession[idx] += duration;
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_SessionInit(const Dcm_SessionControlConfigType *config)
{
    if (config == NULL) {
        return DCM_E_NOT_OK;
    }
    
    /* Initialize state */
    memset(&s_sessionState, 0, sizeof(s_sessionState));
    
    s_sessionState.magic = DCM_SESSION_MAGIC_INIT;
    s_sessionState.config = config;
    s_sessionState.currentSession = DCM_SESSION_DEFAULT;
    s_sessionState.previousSession = DCM_SESSION_DEFAULT;
    s_sessionState.sessionTimer = 0;
    s_sessionState.s3Timer = 0;
    s_sessionState.testerSourceAddress = config->defaultTesterAddress;
    s_sessionState.sessionTimerActive = false;
    s_sessionState.s3TimerActive = true;  /* S3 timer active in non-default session */
    
    /* Set default timing parameters */
    s_sessionState.currentP2ServerMax = DCM_DEFAULT_P2_SERVER_MAX;
    s_sessionState.currentP2StarServerMax = DCM_DEFAULT_P2STAR_SERVER_MAX;
    
    /* Initialize statistics */
    memset(&s_sessionState.statistics, 0, sizeof(s_sessionState.statistics));
    
    /* Initialize audit trail */
    if (config->enableAuditTrail) {
        memset(s_sessionState.auditRecords, 0, sizeof(s_sessionState.auditRecords));
        s_sessionState.auditIndex = 0;
    }
    
    s_initialized = true;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_DiagnosticSessionControl(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    if (request == NULL || response == NULL) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate request length */
    if (request->length < 2) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        return DCM_E_OK;
    }
    
    /* Extract subfunction */
    uint8_t subfunction = request->data[1] & DCM_SUBFUNCTION_MASK;
    bool suppressResponse = (request->data[1] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0;
    
    /* Validate session type */
    Dcm_SessionType targetSession;
    switch (subfunction) {
        case DCM_SUBFUNC_DEFAULT_SESSION:
            targetSession = DCM_SESSION_DEFAULT;
            break;
        case DCM_SUBFUNC_PROGRAMMING_SESSION:
            targetSession = DCM_SESSION_PROGRAMMING;
            break;
        case DCM_SUBFUNC_EXTENDED_SESSION:
            targetSession = DCM_SESSION_EXTENDED;
            break;
        case DCM_SUBFUNC_SAFETY_SYSTEM_SESSION:
            targetSession = DCM_SESSION_SAFETY_SYSTEM;
            break;
        default:
            /* Subfunction not supported */
            response->isNegativeResponse = true;
            response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            return DCM_E_OK;
    }
    
    /* Check if session is supported */
    if (!Dcm_IsSessionSupported(targetSession)) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
        return DCM_E_OK;
    }
    
    /* Validate session transition */
    if (!Dcm_IsSessionTransitionValid(s_sessionState.currentSession, targetSession)) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_CONDITIONS_NOT_CORRECT;
        return DCM_E_OK;
    }
    
    /* Perform session change */
    Dcm_ReturnType result = Dcm_SetSession(targetSession, request->sourceAddress);
    
    if (result != DCM_E_OK) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_CONDITIONS_NOT_CORRECT;
        return DCM_E_OK;
    }
    
    /* Build positive response if not suppressed */
    if (!suppressResponse) {
        response->isNegativeResponse = false;
        response->data[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL + DCM_SID_POSITIVE_RESPONSE_OFFSET;
        response->data[1] = subfunction;
        
        /* Add timing parameters (P2Server_max and P2*Server_max) */
        response->data[2] = (uint8_t)(s_sessionState.currentP2ServerMax >> 8);
        response->data[3] = (uint8_t)(s_sessionState.currentP2ServerMax);
        response->data[4] = (uint8_t)(s_sessionState.currentP2StarServerMax >> 8);
        response->data[5] = (uint8_t)(s_sessionState.currentP2StarServerMax);
        
        response->length = DCM_SESSION_RESPONSE_FULL_SIZE;
        response->suppressPositiveResponse = false;
    } else {
        /* Positive response suppressed */
        response->suppressPositiveResponse = true;
        response->length = 0;
    }
    
    return DCM_E_OK;
}

Dcm_SessionType Dcm_GetCurrentSession(void)
{
    if (!s_initialized) {
        return DCM_SESSION_DEFAULT;
    }
    return s_sessionState.currentSession;
}

Dcm_SessionType Dcm_GetPreviousSession(void)
{
    if (!s_initialized) {
        return DCM_SESSION_DEFAULT;
    }
    return s_sessionState.previousSession;
}

Dcm_ReturnType Dcm_SetSession(Dcm_SessionType session, uint16_t testerAddress)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    if (!Dcm_IsSessionSupported(session)) {
        return DCM_E_SESSION_NOT_SUPPORTED;
    }
    
    /* Validate transition */
    if (!Dcm_IsSessionTransitionValid(s_sessionState.currentSession, session)) {
        return DCM_E_CONDITIONS_NOT_CORRECT;
    }
    
    /* Update statistics for old session */
    updateStatisticsOnExit(s_sessionState.currentSession);
    
    /* Record previous session */
    Dcm_SessionType oldSession = s_sessionState.currentSession;
    s_sessionState.previousSession = oldSession;
    s_sessionState.currentSession = session;
    s_sessionState.testerSourceAddress = testerAddress;
    
    /* Update statistics for new session */
    updateStatisticsOnEntry(session);
    
    /* Get configuration for new session */
    const Dcm_SessionConfigType *config = NULL;
    if (Dcm_GetSessionConfig(session, &config) == DCM_E_OK && config != NULL) {
        /* Update timing parameters */
        s_sessionState.currentP2ServerMax = config->timing.p2ServerMax;
        s_sessionState.currentP2StarServerMax = config->timing.p2StarServerMax;
        
        /* Configure session timer */
        if (config->sessionTimeoutMs > 0) {
            s_sessionState.sessionTimer = config->sessionTimeoutMs;
            s_sessionState.sessionTimerActive = true;
        } else {
            s_sessionState.sessionTimerActive = false;
            s_sessionState.sessionTimer = 0;
        }
    }
    
    /* Reset S3 timer */
    s_sessionState.s3Timer = DCM_DEFAULT_S3_SERVER;
    s_sessionState.s3TimerActive = (session != DCM_SESSION_DEFAULT);
    
    /* Record audit trail */
    recordSessionTransition(oldSession, session, true);
    
    /* Notify callback if registered */
    if (s_sessionState.config->sessionChangeCallback != NULL) {
        s_sessionState.config->sessionChangeCallback(oldSession, session);
    }
    
    return DCM_E_OK;
}

bool Dcm_IsSessionSupported(Dcm_SessionType session)
{
    return (getSessionConfigIndex(session) >= 0);
}

bool Dcm_IsSessionTransitionValid(Dcm_SessionType fromSession, Dcm_SessionType toSession)
{
    /* All sessions can transition to default session */
    if (toSession == DCM_SESSION_DEFAULT) {
        return true;
    }
    
    /* Default session can transition to any non-default session */
    if (fromSession == DCM_SESSION_DEFAULT) {
        return true;
    }
    
    /* Non-default sessions can transition between each other */
    /* Specific restrictions can be added here per OEM requirements */
    
    return true;
}

Dcm_ReturnType Dcm_GetCurrentSessionConfig(const Dcm_SessionConfigType **config)
{
    if (!s_initialized || config == NULL) {
        return DCM_E_NOT_OK;
    }
    
    return Dcm_GetSessionConfig(s_sessionState.currentSession, config);
}

Dcm_ReturnType Dcm_GetSessionConfig(
    Dcm_SessionType session,
    const Dcm_SessionConfigType **config
)
{
    if (config == NULL) {
        return DCM_E_NOT_OK;
    }
    
    int32_t idx = getSessionConfigIndex(session);
    if (idx < 0) {
        *config = NULL;
        return DCM_E_NOT_OK;
    }
    
    *config = &s_sessionState.config->sessionConfigs[idx];
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_SessionTimerUpdate(uint32_t elapsedTimeMs)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Update session timer */
    if (s_sessionState.sessionTimerActive && s_sessionState.sessionTimer > 0) {
        if (s_sessionState.sessionTimer > elapsedTimeMs) {
            s_sessionState.sessionTimer -= elapsedTimeMs;
        } else {
            /* Session timeout */
            s_sessionState.sessionTimer = 0;
            s_sessionState.statistics.sessionTimeoutCount++;
            Dcm_SessionTimeoutHandler();
        }
    }
    
    /* Update S3 timer */
    if (s_sessionState.s3TimerActive && s_sessionState.s3Timer > 0) {
        if (s_sessionState.s3Timer > elapsedTimeMs) {
            s_sessionState.s3Timer -= elapsedTimeMs;
        } else {
            /* S3 timeout - return to default session */
            s_sessionState.s3Timer = 0;
            Dcm_StopSession();
        }
    }
    
    return DCM_E_OK;
}

uint32_t Dcm_GetSessionTimeoutRemaining(void)
{
    if (!s_initialized) {
        return 0;
    }
    return s_sessionState.sessionTimer;
}

Dcm_ReturnType Dcm_ResetSessionTimer(void)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    const Dcm_SessionConfigType *config = NULL;
    if (Dcm_GetCurrentSessionConfig(&config) == DCM_E_OK && config != NULL) {
        s_sessionState.sessionTimer = config->sessionTimeoutMs;
    }
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_GetSessionTimingParameters(
    uint16_t *p2ServerMax,
    uint16_t *p2StarServerMax
)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    if (p2ServerMax != NULL) {
        *p2ServerMax = s_sessionState.currentP2ServerMax;
    }
    
    if (p2StarServerMax != NULL) {
        *p2StarServerMax = s_sessionState.currentP2StarServerMax;
    }
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_SetSessionTimingParameters(
    uint16_t p2ServerMax,
    uint16_t p2StarServerMax
)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate parameters */
    if (p2ServerMax == 0 || p2StarServerMax == 0) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    s_sessionState.currentP2ServerMax = p2ServerMax;
    s_sessionState.currentP2StarServerMax = p2StarServerMax;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_GetSessionStatistics(Dcm_SessionStatisticsType *stats)
{
    if (!s_initialized || stats == NULL) {
        return DCM_E_NOT_OK;
    }
    
    memcpy(stats, &s_sessionState.statistics, sizeof(Dcm_SessionStatisticsType));
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_GetSessionTransitionRecord(
    uint8_t index,
    Dcm_SessionTransitionRecordType *record
)
{
    if (!s_initialized || record == NULL) {
        return DCM_E_NOT_OK;
    }
    
    if (!s_sessionState.config->enableAuditTrail) {
        return DCM_E_NOT_OK;
    }
    
    if (index >= s_sessionState.config->maxAuditRecords) {
        return DCM_E_REQUEST_OUT_OF_RANGE;
    }
    
    /* Calculate actual index (newest first) */
    uint8_t actualIdx = (s_sessionState.auditIndex + 
        s_sessionState.config->maxAuditRecords - 1 - index) % 
        s_sessionState.config->maxAuditRecords;
    
    memcpy(record, &s_sessionState.auditRecords[actualIdx], 
           sizeof(Dcm_SessionTransitionRecordType));
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_StopSession(void)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Transition back to default session */
    return Dcm_SetSession(DCM_SESSION_DEFAULT, 0x0000);
}

void Dcm_SessionTimeoutHandler(void)
{
    if (!s_initialized) {
        return;
    }
    
    /* Return to default session on timeout */
    Dcm_StopSession();
}

uint32_t Dcm_GetS3ServerTime(void)
{
    if (!s_initialized) {
        return DCM_DEFAULT_S3_SERVER;
    }
    return s_sessionState.s3Timer;
}

Dcm_ReturnType Dcm_ResetS3Timer(void)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    s_sessionState.s3Timer = DCM_DEFAULT_S3_SERVER;
    return DCM_E_OK;
}

/******************************************************************************
 * Default Configuration Getter
 ******************************************************************************/
const Dcm_SessionConfigType* Dcm_GetDefaultSessionConfigs(uint8_t *numSessions)
{
    if (numSessions != NULL) {
        *numSessions = sizeof(s_defaultSessionConfigs) / sizeof(s_defaultSessionConfigs[0]);
    }
    return s_defaultSessionConfigs;
}
