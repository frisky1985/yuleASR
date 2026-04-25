/******************************************************************************
 * @file    dcm_ecu_reset.c
 * @brief   DCM ECU Reset Service (0x11) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_ecu_reset.h"
#include "dcm_session.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_ECU_RESET_MAGIC_INIT        0x44434552U  /* "DCER" */

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;                         /* Initialization marker */
    const Dcm_EcuResetConfigType *config;   /* Configuration pointer */
    Dcm_EcuResetStatusType status;          /* Reset status */
} Dcm_EcuResetInternalStateType;

/******************************************************************************
 * Static Data
 ******************************************************************************/
static Dcm_EcuResetInternalStateType s_resetState;
static bool s_initialized = false;

/******************************************************************************
 * Default Configuration
 ******************************************************************************/
static const Dcm_EcuResetConfigType s_defaultResetConfig = {
    .hardResetSupported = true,
    .keyOffOnResetSupported = true,
    .softResetSupported = true,
    .rapidShutdownSupported = false,
    .powerDownTimeSeconds = DCM_ECU_RESET_POWER_DOWN_TIME_NOT_AVAILABLE,
    .resetDelayMs = 10,                     /* 10ms delay for response transmission */
    .requireProgrammingSession = false,
    .requireExtendedSession = false,
    .resetCallback = NULL
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Validate reset type
 */
static bool isResetTypeValid(Dcm_ResetType resetType)
{
    switch (resetType) {
        case DCM_RESET_HARD:
            return s_resetState.config->hardResetSupported;
        case DCM_RESET_KEY_OFF_ON:
            return s_resetState.config->keyOffOnResetSupported;
        case DCM_RESET_SOFT:
            return s_resetState.config->softResetSupported;
        case DCM_RESET_ENABLE_RAPID_SHUTDOWN:
        case DCM_RESET_DISABLE_RAPID_SHUTDOWN:
            return s_resetState.config->rapidShutdownSupported;
        default:
            return false;
    }
}

/**
 * @brief Get subfunction from reset type
 */
static uint8_t getSubfunctionFromResetType(Dcm_ResetType resetType)
{
    switch (resetType) {
        case DCM_RESET_HARD:
            return DCM_RESET_SUBFUNC_HARD_RESET;
        case DCM_RESET_KEY_OFF_ON:
            return DCM_RESET_SUBFUNC_KEY_OFF_ON_RESET;
        case DCM_RESET_SOFT:
            return DCM_RESET_SUBFUNC_SOFT_RESET;
        case DCM_RESET_ENABLE_RAPID_SHUTDOWN:
            return DCM_RESET_SUBFUNC_ENABLE_RAPID_SHUTDOWN;
        case DCM_RESET_DISABLE_RAPID_SHUTDOWN:
            return DCM_RESET_SUBFUNC_DISABLE_RAPID_SHUTDOWN;
        default:
            return 0;
    }
}

/**
 * @brief Record reset request in statistics
 */
static void recordResetRequest(Dcm_ResetType resetType)
{
    s_resetState.status.resetCount++;
    
    /* Update per-type counter */
    switch (resetType) {
        case DCM_RESET_HARD:
            s_resetState.status.resetByType[0]++;
            break;
        case DCM_RESET_KEY_OFF_ON:
            s_resetState.status.resetByType[1]++;
            break;
        case DCM_RESET_SOFT:
            s_resetState.status.resetByType[2]++;
            break;
        case DCM_RESET_ENABLE_RAPID_SHUTDOWN:
            s_resetState.status.resetByType[3]++;
            break;
        case DCM_RESET_DISABLE_RAPID_SHUTDOWN:
            s_resetState.status.resetByType[4]++;
            break;
        default:
            break;
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_EcuResetInit(const Dcm_EcuResetConfigType *config)
{
    /* Use default config if NULL */
    if (config == NULL) {
        config = &s_defaultResetConfig;
    }
    
    /* Initialize state */
    memset(&s_resetState, 0, sizeof(s_resetState));
    
    s_resetState.magic = DCM_ECU_RESET_MAGIC_INIT;
    s_resetState.config = config;
    s_resetState.status.state = DCM_ECU_RESET_STATE_IDLE;
    s_resetState.status.pendingResetType = 0;
    s_resetState.status.resetTimer = 0;
    s_resetState.status.powerDownTime = config->powerDownTimeSeconds;
    s_resetState.status.requestingTesterAddress = 0;
    s_resetState.status.resetRequestTime = 0;
    s_resetState.status.resetCount = 0;
    memset(s_resetState.status.resetByType, 0, sizeof(s_resetState.status.resetByType));
    
    s_initialized = true;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_EcuReset(
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
    
    /* Check if reset already in progress */
    if (s_resetState.status.state != DCM_ECU_RESET_STATE_IDLE) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_BUSY_REPEAT_REQUEST;
        return DCM_E_OK;
    }
    
    /* Extract subfunction */
    uint8_t subfunction = request->data[1] & DCM_SUBFUNCTION_MASK;
    bool suppressResponse = (request->data[1] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0;
    
    /* Map subfunction to reset type */
    Dcm_ResetType resetType;
    switch (subfunction) {
        case DCM_RESET_SUBFUNC_HARD_RESET:
            resetType = DCM_RESET_HARD;
            break;
        case DCM_RESET_SUBFUNC_KEY_OFF_ON_RESET:
            resetType = DCM_RESET_KEY_OFF_ON;
            break;
        case DCM_RESET_SUBFUNC_SOFT_RESET:
            resetType = DCM_RESET_SOFT;
            break;
        case DCM_RESET_SUBFUNC_ENABLE_RAPID_SHUTDOWN:
            resetType = DCM_RESET_ENABLE_RAPID_SHUTDOWN;
            break;
        case DCM_RESET_SUBFUNC_DISABLE_RAPID_SHUTDOWN:
            resetType = DCM_RESET_DISABLE_RAPID_SHUTDOWN;
            break;
        default:
            /* Subfunction not supported */
            response->isNegativeResponse = true;
            response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            return DCM_E_OK;
    }
    
    /* Check if reset type is supported */
    if (!Dcm_IsEcuResetSupported(resetType)) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
        return DCM_E_OK;
    }
    
    /* Check reset conditions */
    Dcm_ReturnType conditionResult = Dcm_CheckResetConditions(resetType);
    if (conditionResult != DCM_E_OK) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = UDS_NRC_CONDITIONS_NOT_CORRECT;
        return DCM_E_OK;
    }
    
    /* Handle rapid shutdown enable/disable (immediate response, no reset) */
    if (resetType == DCM_RESET_ENABLE_RAPID_SHUTDOWN ||
        resetType == DCM_RESET_DISABLE_RAPID_SHUTDOWN) {
        
        bool enable = (resetType == DCM_RESET_ENABLE_RAPID_SHUTDOWN);
        Dcm_Platform_SetRapidShutdown(enable);
        
        if (!suppressResponse) {
            response->isNegativeResponse = false;
            response->data[0] = UDS_SVC_ECU_RESET + DCM_SID_POSITIVE_RESPONSE_OFFSET;
            response->data[1] = subfunction;
            response->length = DCM_ECU_RESET_RESPONSE_MIN_SIZE;
        } else {
            response->suppressPositiveResponse = true;
            response->length = 0;
        }
        
        return DCM_E_OK;
    }
    
    /* For actual reset types, prepare for reset */
    recordResetRequest(resetType);
    
    /* Set reset state to pending */
    s_resetState.status.state = DCM_ECU_RESET_STATE_PENDING;
    s_resetState.status.pendingResetType = resetType;
    s_resetState.status.resetTimer = s_resetState.config->resetDelayMs;
    s_resetState.status.requestingTesterAddress = request->sourceAddress;
    s_resetState.status.resetRequestTime = 0;  /* TODO: Get timestamp */
    
    /* Notify callback if registered */
    if (s_resetState.config->resetCallback != NULL) {
        s_resetState.config->resetCallback(resetType);
    }
    
    /* Build positive response if not suppressed */
    if (!suppressResponse) {
        response->isNegativeResponse = false;
        response->data[0] = UDS_SVC_ECU_RESET + DCM_SID_POSITIVE_RESPONSE_OFFSET;
        response->data[1] = subfunction;
        
        /* Add power down time if available */
        if (s_resetState.status.powerDownTime != DCM_ECU_RESET_POWER_DOWN_TIME_NOT_AVAILABLE) {
            response->data[2] = s_resetState.status.powerDownTime;
            response->length = DCM_ECU_RESET_RESPONSE_FULL_SIZE;
        } else {
            response->length = DCM_ECU_RESET_RESPONSE_MIN_SIZE;
        }
        
        response->suppressPositiveResponse = false;
    } else {
        /* Positive response suppressed */
        response->suppressPositiveResponse = true;
        response->length = 0;
    }
    
    return DCM_E_OK;
}

bool Dcm_IsEcuResetSupported(Dcm_ResetType resetType)
{
    if (!s_initialized) {
        return false;
    }
    
    return isResetTypeValid(resetType);
}

Dcm_ReturnType Dcm_CheckResetConditions(Dcm_ResetType resetType)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Get current session */
    Dcm_SessionType currentSession = Dcm_GetCurrentSession();
    
    /* Hard reset may require programming session */
    if (resetType == DCM_RESET_HARD && 
        s_resetState.config->requireProgrammingSession) {
        if (currentSession != DCM_SESSION_PROGRAMMING) {
            return DCM_E_CONDITIONS_NOT_CORRECT;
        }
    }
    
    /* Soft reset may require extended or programming session */
    if (resetType == DCM_RESET_SOFT && 
        s_resetState.config->requireExtendedSession) {
        if (currentSession != DCM_SESSION_EXTENDED && 
            currentSession != DCM_SESSION_PROGRAMMING) {
            return DCM_E_CONDITIONS_NOT_CORRECT;
        }
    }
    
    /* Key Off/On reset requires extended or programming session per ISO 14229-1 */
    if (resetType == DCM_RESET_KEY_OFF_ON) {
        if (currentSession != DCM_SESSION_EXTENDED && 
            currentSession != DCM_SESSION_PROGRAMMING) {
            return DCM_E_CONDITIONS_NOT_CORRECT;
        }
    }
    
    /* Check if reset already pending */
    if (s_resetState.status.state != DCM_ECU_RESET_STATE_IDLE) {
        return DCM_E_BUSY_REPEAT_REQUEST;
    }
    
    (void)resetType;  /* Unused parameter if conditions above are compiled out */
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_ExecuteEcuReset(void)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Check if reset is pending */
    if (s_resetState.status.state != DCM_ECU_RESET_STATE_PENDING) {
        return DCM_E_NOT_OK;
    }
    
    /* Set state to executing */
    s_resetState.status.state = DCM_ECU_RESET_STATE_EXECUTING;
    
    /* Execute the appropriate reset */
    switch (s_resetState.status.pendingResetType) {
        case DCM_RESET_HARD:
            Dcm_Platform_HardReset();
            break;
            
        case DCM_RESET_KEY_OFF_ON:
            Dcm_Platform_KeyOffOnReset();
            break;
            
        case DCM_RESET_SOFT:
            Dcm_Platform_SoftReset();
            break;
            
        default:
            /* Should not reach here */
            s_resetState.status.state = DCM_ECU_RESET_STATE_IDLE;
            return DCM_E_NOT_OK;
    }
    
    /* Reset functions should not return */
    return DCM_E_OK;
}

Dcm_EcuResetStateType Dcm_GetEcuResetState(void)
{
    if (!s_initialized) {
        return DCM_ECU_RESET_STATE_IDLE;
    }
    return (Dcm_EcuResetStateType)s_resetState.status.state;
}

Dcm_ResetType Dcm_GetPendingResetType(void)
{
    if (!s_initialized) {
        return 0;
    }
    
    if (s_resetState.status.state == DCM_ECU_RESET_STATE_IDLE) {
        return 0;
    }
    
    return s_resetState.status.pendingResetType;
}

Dcm_ReturnType Dcm_CancelEcuReset(void)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    if (s_resetState.status.state == DCM_ECU_RESET_STATE_IDLE) {
        return DCM_E_OK;  /* No reset to cancel */
    }
    
    /* Reset state machine */
    s_resetState.status.state = DCM_ECU_RESET_STATE_IDLE;
    s_resetState.status.pendingResetType = 0;
    s_resetState.status.resetTimer = 0;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_GetEcuResetStatus(Dcm_EcuResetStatusType *status)
{
    if (!s_initialized || status == NULL) {
        return DCM_E_NOT_OK;
    }
    
    memcpy(status, &s_resetState.status, sizeof(Dcm_EcuResetStatusType));
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_EcuResetTimerUpdate(uint32_t elapsedTimeMs)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Only process if reset is pending */
    if (s_resetState.status.state != DCM_ECU_RESET_STATE_PENDING) {
        return DCM_E_OK;
    }
    
    /* Update reset timer */
    if (s_resetState.status.resetTimer > elapsedTimeMs) {
        s_resetState.status.resetTimer -= elapsedTimeMs;
    } else {
        /* Timer expired, execute reset */
        s_resetState.status.resetTimer = 0;
        return Dcm_ExecuteEcuReset();
    }
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_SetPowerDownTime(uint8_t seconds)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    s_resetState.status.powerDownTime = seconds;
    return DCM_E_OK;
}

uint8_t Dcm_GetPowerDownTime(void)
{
    if (!s_initialized) {
        return DCM_ECU_RESET_POWER_DOWN_TIME_NOT_AVAILABLE;
    }
    return s_resetState.status.powerDownTime;
}

Dcm_ReturnType Dcm_ResetEcuResetStatistics(void)
{
    if (!s_initialized) {
        return DCM_E_NOT_OK;
    }
    
    s_resetState.status.resetCount = 0;
    memset(s_resetState.status.resetByType, 0, sizeof(s_resetState.status.resetByType));
    
    return DCM_E_OK;
}

/******************************************************************************
 * Platform-Specific Weak Implementations
 ******************************************************************************/

/**
 * @brief Weak implementation of hard reset - to be overridden by platform
 */
__attribute__((weak)) void Dcm_Platform_HardReset(void)
{
    /* Default implementation - spin forever */
    while (1) {
        /* Should trigger watchdog or hardware reset */
    }
}

/**
 * @brief Weak implementation of soft reset - to be overridden by platform
 */
__attribute__((weak)) void Dcm_Platform_SoftReset(void)
{
    /* Default implementation - spin forever */
    while (1) {
        /* Should trigger soft reset */
    }
}

/**
 * @brief Weak implementation of key off/on reset - to be overridden by platform
 */
__attribute__((weak)) void Dcm_Platform_KeyOffOnReset(void)
{
    /* Default implementation - use hard reset as fallback */
    Dcm_Platform_HardReset();
}

/**
 * @brief Weak implementation of rapid shutdown - to be overridden by platform
 */
__attribute__((weak)) void Dcm_Platform_SetRapidShutdown(bool enable)
{
    /* Default implementation - do nothing */
    (void)enable;
}
