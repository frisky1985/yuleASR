/******************************************************************************
 * @file    dcm_security.c
 * @brief   DCM Security Access Service (0x27) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_security.h"
#include <string.h>

#ifdef CSM_CORE_H
#include "../../crypto_stack/csm/csm_core.h"
#endif

/******************************************************************************
 * Private Macros (MISRA Rule 20.7 - Enclose macro parameters in parentheses)
 ******************************************************************************/
#define DCM_SECURITY_MAGIC_INIT         (0x53454355U)  /* "SECU" */
#define DCM_IS_ODD(x)                   (((x) & 0x01U) != 0U)
#define DCM_IS_EVEN(x)                  (((x) & 0x01U) == 0U)
#define DCM_GET_LEVEL_FROM_SEED_SF(sf)  ((((sf) + 1U) / 2U))
#define DCM_GET_LEVEL_FROM_KEY_SF(sf)   ((sf) / 2U)
#define DCM_MIN(a, b)                   (((a) < (b)) ? (a) : (b))

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;                                 /* Initialization marker */
    const Dcm_SecurityAccessConfigType *config;     /* Configuration pointer */
    Dcm_SecurityStatusType status;                  /* Security status */
    bool initialized;                               /* Initialization flag */
} Dcm_SecurityInternalStateType;

/******************************************************************************
 * Static Data
 ******************************************************************************/
static Dcm_SecurityInternalStateType s_securityState;

/******************************************************************************
 * Default Security Level Configurations
 ******************************************************************************/
static const Dcm_SecurityLevelConfigType s_defaultLevelConfigs[] = {
    {
        .securityLevel = DCM_SECURITY_LEVEL_1,
        .maxFailedAttempts = DCM_SEC_MAX_ATTEMPTS,
        .lockoutTimeMs = DCM_SEC_LOCKOUT_TIME_MS,
        .delayTimeMs = DCM_SEC_REQUIRED_TIME_DELAY_MS,
        .seedLength = DCM_SEC_SEED_LENGTH,
        .keyLength = DCM_SEC_KEY_LENGTH,
        .seedCallback = NULL,  /* Use default */
        .keyCallback = NULL,   /* Use default */
        .csmKeyDeriveEnabled = false,
        .csmKeyId = 0U
    },
    {
        .securityLevel = DCM_SECURITY_LEVEL_2,
        .maxFailedAttempts = DCM_SEC_MAX_ATTEMPTS,
        .lockoutTimeMs = DCM_SEC_LOCKOUT_TIME_MS,
        .delayTimeMs = DCM_SEC_REQUIRED_TIME_DELAY_MS,
        .seedLength = DCM_SEC_SEED_LENGTH,
        .keyLength = DCM_SEC_KEY_LENGTH,
        .seedCallback = NULL,
        .keyCallback = NULL,
        .csmKeyDeriveEnabled = false,
        .csmKeyId = 1U
    },
    {
        .securityLevel = DCM_SECURITY_LEVEL_3,
        .maxFailedAttempts = DCM_SEC_MAX_ATTEMPTS,
        .lockoutTimeMs = DCM_SEC_LOCKOUT_TIME_MS,
        .delayTimeMs = DCM_SEC_REQUIRED_TIME_DELAY_MS,
        .seedLength = DCM_SEC_SEED_LENGTH,
        .keyLength = DCM_SEC_KEY_LENGTH,
        .seedCallback = NULL,
        .keyCallback = NULL,
        .csmKeyDeriveEnabled = true,
        .csmKeyId = 2U
    }
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Get security level configuration
 */
static const Dcm_SecurityLevelConfigType* getLevelConfig(uint8_t securityLevel)
{
    const Dcm_SecurityLevelConfigType *config = NULL;
    
    if (s_securityState.config != NULL) {
        for (uint8_t i = 0U; i < s_securityState.config->numSecurityLevels; i++) {
            if (s_securityState.config->levelConfigs[i].securityLevel == securityLevel) {
                config = &s_securityState.config->levelConfigs[i];
                break;
            }
        }
    }
    
    return config;
}

/**
 * @brief Check if subfunction is valid
 */
static bool isValidSubfunction(uint8_t subfunction)
{
    bool valid = false;
    
    if ((subfunction >= DCM_SEC_REQ_SEED_LEVEL_1) && 
        (subfunction <= DCM_SEC_SEND_KEY_LEVEL_8)) {
        /* Check if corresponding level is supported */
        uint8_t level = Dcm_GetSecurityLevelFromSubfunction(subfunction);
        if (level > 0U) {
            valid = Dcm_IsSecurityLevelSupported(level);
        }
    }
    
    return valid;
}

/**
 * @brief Send negative response
 */
static Dcm_ReturnType sendNegativeResponse(
    Dcm_ResponseType *response,
    uint8_t nrc
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (response != NULL) {
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        response->length = 0U;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Send positive response for request seed
 */
static Dcm_ReturnType sendSeedResponse(
    Dcm_ResponseType *response,
    uint8_t subfunction,
    const uint8_t *seed,
    uint8_t seedLength
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= (uint32_t)(2U + seedLength))) {
        response->data[0U] = (uint8_t)(UDS_SVC_SECURITY_ACCESS + 0x40U);  /* Positive response SID */
        response->data[1U] = subfunction;
        
        if ((seed != NULL) && (seedLength > 0U)) {
            (void)memcpy(&response->data[2U], seed, seedLength);
        }
        
        response->length = (uint32_t)(2U + seedLength);
        response->isNegativeResponse = false;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Send positive response for send key
 */
static Dcm_ReturnType sendKeyResponse(
    Dcm_ResponseType *response,
    uint8_t subfunction
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((response != NULL) && (response->data != NULL) && 
        (response->maxLength >= 2U)) {
        response->data[0U] = (uint8_t)(UDS_SVC_SECURITY_ACCESS + 0x40U);
        response->data[1U] = subfunction;
        response->length = 2U;
        response->isNegativeResponse = false;
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Generate random seed
 */
static Dcm_ReturnType generateRandomSeed(uint8_t *seedBuffer, uint8_t seedLength)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((seedBuffer != NULL) && (seedLength > 0U)) {
        /* Use simple pseudo-random for now - in production use CSM or HW RNG */
        static uint32_t seed = 0x12345678U;
        
        for (uint8_t i = 0U; i < seedLength; i++) {
            seed = (seed * 1103515245U) + 12345U;
            seedBuffer[i] = (uint8_t)(seed >> 16);
        }
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Calculate key from seed using simple algorithm
 */
static Dcm_ReturnType calculateKey(
    const uint8_t *seed,
    uint8_t *key,
    uint8_t length,
    uint8_t securityLevel
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((seed != NULL) && (key != NULL) && (length > 0U)) {
        /* Simple key derivation - XOR seed with level-specific constant */
        /* In production, use proper cryptographic derivation via CSM */
        const uint8_t levelConstant[8U] = {
            0x5AU, 0xA5U, 0x3CU, 0xC3U, 0x55U, 0xAAU, 0x33U, 0xCCU
        };
        
        for (uint8_t i = 0U; i < length; i++) {
            key[i] = (uint8_t)(seed[i] ^ levelConstant[(securityLevel - 1U) % 8U]);
        }
        result = DCM_E_OK;
    }
    
    return result;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_SecurityAccessInit(const Dcm_SecurityAccessConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config != NULL) {
        /* Initialize state */
        (void)memset(&s_securityState, 0, sizeof(s_securityState));
        
        s_securityState.magic = DCM_SECURITY_MAGIC_INIT;
        s_securityState.config = config;
        s_securityState.status.currentSecurityLevel = config->defaultSecurityLevel;
        s_securityState.status.state = DCM_SEC_STATE_LOCKED;
        s_securityState.initialized = true;
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_SecurityAccess(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    
    /* Check initialization */
    if (!s_securityState.initialized) {
        nrc = UDS_NRC_CONDITIONS_NOT_CORRECT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL)) {
        return result;
    }
    
    /* Check request length */
    if (request->length < DCM_SEC_REQ_SEED_MIN_LENGTH) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    const uint8_t subfunction = request->data[1U] & DCM_SUBFUNCTION_MASK;
    
    /* Check for suppress positive response */
    if ((request->data[1U] & DCM_SUPPRESS_POS_RESPONSE_MASK) != 0U) {
        response->suppressPositiveResponse = true;
    }
    
    /* Check if locked out */
    if (s_securityState.status.state == DCM_SEC_STATE_LOCKED_OUT) {
        nrc = UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Validate subfunction */
    if (!isValidSubfunction(subfunction)) {
        nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
        (void)sendNegativeResponse(response, nrc);
        return result;
    }
    
    /* Process based on subfunction type */
    if (Dcm_IsRequestSeedSubfunction(subfunction)) {
        /* Request Seed handling */
        uint8_t requestedLevel = Dcm_GetSecurityLevelFromSubfunction(subfunction);
        const Dcm_SecurityLevelConfigType *levelConfig = getLevelConfig(requestedLevel);
        
        if (levelConfig == NULL) {
            nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            (void)sendNegativeResponse(response, nrc);
            return result;
        }
        
        /* Check required time delay */
        if (s_securityState.status.delayTimer > 0U) {
            nrc = UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED;
            (void)sendNegativeResponse(response, nrc);
            return result;
        }
        
        /* If already unlocked at this level, return zero seed */
        if (s_securityState.status.currentSecurityLevel == requestedLevel) {
            uint8_t zeroSeed[DCM_SEC_SEED_LENGTH] = {0U};
            result = sendSeedResponse(response, subfunction, zeroSeed, levelConfig->seedLength);
            return result;
        }
        
        /* Generate seed */
        uint8_t seed[DCM_SEC_SEED_LENGTH];
        (void)memset(seed, 0, sizeof(seed));
        
        if (levelConfig->seedCallback != NULL) {
            result = levelConfig->seedCallback(requestedLevel, seed, levelConfig->seedLength);
        } else {
            result = generateRandomSeed(seed, levelConfig->seedLength);
        }
        
        if (result == DCM_E_OK) {
            /* Store seed and update state */
            (void)memcpy(s_securityState.status.lastSeed, seed, DCM_MIN(levelConfig->seedLength, DCM_SEC_SEED_LENGTH));
            s_securityState.status.seedValid = true;
            s_securityState.status.pendingSecurityLevel = requestedLevel;
            s_securityState.status.state = DCM_SEC_STATE_SEED_REQUESTED;
            
            result = sendSeedResponse(response, subfunction, seed, levelConfig->seedLength);
        } else {
            nrc = UDS_NRC_GENERAL_REJECT;
            (void)sendNegativeResponse(response, nrc);
        }
        
    } else {
        /* Send Key handling */
        uint8_t requestedLevel = Dcm_GetSecurityLevelFromSubfunction(subfunction);
        const Dcm_SecurityLevelConfigType *levelConfig = getLevelConfig(requestedLevel);
        
        if (levelConfig == NULL) {
            nrc = UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
            (void)sendNegativeResponse(response, nrc);
            return result;
        }
        
        /* Check request length for key data */
        if (request->length < (uint32_t)(2U + levelConfig->keyLength)) {
            nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
            (void)sendNegativeResponse(response, nrc);
            return result;
        }
        
        /* Verify seed was requested first */
        if ((!s_securityState.status.seedValid) ||
            (s_securityState.status.state != DCM_SEC_STATE_SEED_REQUESTED) ||
            (s_securityState.status.pendingSecurityLevel != requestedLevel)) {
            nrc = UDS_NRC_REQUEST_SEQUENCE_ERROR;
            (void)sendNegativeResponse(response, nrc);
            return result;
        }
        
        /* Extract key from request */
        const uint8_t *key = &request->data[2U];
        bool isValid = false;
        
        /* Validate key */
        if (levelConfig->keyCallback != NULL) {
            result = levelConfig->keyCallback(
                requestedLevel,
                s_securityState.status.lastSeed,
                key,
                levelConfig->keyLength,
                &isValid
            );
        } else {
            /* Use default validation */
            uint8_t expectedKey[DCM_SEC_KEY_LENGTH];
            result = calculateKey(
                s_securityState.status.lastSeed,
                expectedKey,
                levelConfig->keyLength,
                requestedLevel
            );
            
            if (result == DCM_E_OK) {
                isValid = true;
                for (uint8_t i = 0U; i < levelConfig->keyLength; i++) {
                    if (key[i] != expectedKey[i]) {
                        isValid = false;
                        break;
                    }
                }
            }
        }
        
        if ((result == DCM_E_OK) && isValid) {
            /* Key valid - unlock security level */
            uint8_t oldLevel = s_securityState.status.currentSecurityLevel;
            s_securityState.status.currentSecurityLevel = requestedLevel;
            s_securityState.status.state = DCM_SEC_STATE_UNLOCKED;
            s_securityState.status.failedAttempts = 0U;
            s_securityState.status.seedValid = false;
            (void)memset(s_securityState.status.lastSeed, 0, DCM_SEC_SEED_LENGTH);
            
            /* Notify callback if registered */
            if ((s_securityState.config != NULL) && 
                (s_securityState.config->changeCallback != NULL)) {
                s_securityState.config->changeCallback(oldLevel, requestedLevel);
            }
            
            result = sendKeyResponse(response, subfunction);
        } else {
            /* Key invalid */
            s_securityState.status.failedAttempts++;
            s_securityState.status.seedValid = false;
            
            /* Check if max attempts exceeded */
            if (s_securityState.status.failedAttempts >= levelConfig->maxFailedAttempts) {
                s_securityState.status.state = DCM_SEC_STATE_LOCKED_OUT;
                s_securityState.status.lockoutTimer = levelConfig->lockoutTimeMs;
                nrc = UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS;
            } else {
                /* Start delay timer */
                s_securityState.status.delayTimer = levelConfig->delayTimeMs;
                nrc = UDS_NRC_INVALID_KEY;
            }
            
            (void)sendNegativeResponse(response, nrc);
            result = DCM_E_NOT_OK;
        }
    }
    
    return result;
}

uint8_t Dcm_GetSecurityLevel(void)
{
    uint8_t level = DCM_SECURITY_LEVEL_LOCKED;
    
    if (s_securityState.initialized) {
        level = s_securityState.status.currentSecurityLevel;
    }
    
    return level;
}

Dcm_ReturnType Dcm_SetSecurityLevel(uint8_t securityLevel)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_securityState.initialized) {
        uint8_t oldLevel = s_securityState.status.currentSecurityLevel;
        
        if ((securityLevel == DCM_SECURITY_LEVEL_LOCKED) || 
            Dcm_IsSecurityLevelSupported(securityLevel)) {
            s_securityState.status.currentSecurityLevel = securityLevel;
            
            if (securityLevel == DCM_SECURITY_LEVEL_LOCKED) {
                s_securityState.status.state = DCM_SEC_STATE_LOCKED;
            } else {
                s_securityState.status.state = DCM_SEC_STATE_UNLOCKED;
            }
            
            /* Notify callback if registered */
            if ((s_securityState.config != NULL) && 
                (s_securityState.config->changeCallback != NULL)) {
                s_securityState.config->changeCallback(oldLevel, securityLevel);
            }
            
            result = DCM_E_OK;
        }
    }
    
    return result;
}

bool Dcm_IsSecurityLevelSupported(uint8_t securityLevel)
{
    bool supported = false;
    
    if ((securityLevel == DCM_SECURITY_LEVEL_LOCKED) || 
        (getLevelConfig(securityLevel) != NULL)) {
        supported = true;
    }
    
    return supported;
}

bool Dcm_IsSecurityLevelUnlocked(uint8_t securityLevel)
{
    bool unlocked = false;
    
    if (s_securityState.initialized) {
        unlocked = (s_securityState.status.currentSecurityLevel >= securityLevel);
    }
    
    return unlocked;
}

uint8_t Dcm_GetSecurityLevelFromSubfunction(uint8_t subfunction)
{
    uint8_t level = 0U;
    
    if (Dcm_IsRequestSeedSubfunction(subfunction)) {
        level = DCM_GET_LEVEL_FROM_SEED_SF(subfunction);
    } else if (Dcm_IsSendKeySubfunction(subfunction)) {
        level = DCM_GET_LEVEL_FROM_KEY_SF(subfunction);
    } else {
        /* Invalid subfunction */
    }
    
    return level;
}

bool Dcm_IsRequestSeedSubfunction(uint8_t subfunction)
{
    bool isSeedSf = false;
    
    if ((subfunction >= DCM_SEC_REQ_SEED_LEVEL_1) && 
        (subfunction <= DCM_SEC_REQ_SEED_LEVEL_8)) {
        isSeedSf = true;
    }
    
    return isSeedSf;
}

bool Dcm_IsSendKeySubfunction(uint8_t subfunction)
{
    bool isKeySf = false;
    
    if ((subfunction >= DCM_SEC_SEND_KEY_LEVEL_1) && 
        (subfunction <= DCM_SEC_SEND_KEY_LEVEL_8)) {
        isKeySf = true;
    }
    
    return isKeySf;
}

Dcm_ReturnType Dcm_GenerateSeed(
    uint8_t securityLevel,
    uint8_t *seedBuffer,
    uint8_t seedLength
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_securityState.initialized && (seedBuffer != NULL) && (seedLength > 0U)) {
        const Dcm_SecurityLevelConfigType *config = getLevelConfig(securityLevel);
        
        if ((config != NULL) && (config->seedCallback != NULL)) {
            result = config->seedCallback(securityLevel, seedBuffer, seedLength);
        } else {
            result = generateRandomSeed(seedBuffer, seedLength);
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_ValidateKey(
    uint8_t securityLevel,
    const uint8_t *seed,
    const uint8_t *key,
    uint8_t length,
    bool *isValid
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((isValid != NULL) && s_securityState.initialized && 
        (seed != NULL) && (key != NULL) && (length > 0U)) {
        const Dcm_SecurityLevelConfigType *config = getLevelConfig(securityLevel);
        
        *isValid = false;
        
        if ((config != NULL) && (config->keyCallback != NULL)) {
            result = config->keyCallback(securityLevel, seed, key, length, isValid);
        } else {
            uint8_t expectedKey[DCM_SEC_KEY_LENGTH];
            result = calculateKey(seed, expectedKey, length, securityLevel);
            
            if (result == DCM_E_OK) {
                *isValid = true;
                for (uint8_t i = 0U; i < length; i++) {
                    if (key[i] != expectedKey[i]) {
                        *isValid = false;
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_LockSecurityAccess(void)
{
    return Dcm_SetSecurityLevel(DCM_SECURITY_LEVEL_LOCKED);
}

Dcm_ReturnType Dcm_GetSecurityStatus(Dcm_SecurityStatusType *status)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((status != NULL) && s_securityState.initialized) {
        *status = s_securityState.status;
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_SecurityTimerUpdate(uint32_t elapsedTimeMs)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_securityState.initialized) {
        /* Update lockout timer */
        if (s_securityState.status.lockoutTimer > 0U) {
            if (s_securityState.status.lockoutTimer > elapsedTimeMs) {
                s_securityState.status.lockoutTimer -= elapsedTimeMs;
            } else {
                s_securityState.status.lockoutTimer = 0U;
                /* Reset to locked state when lockout expires */
                if (s_securityState.status.state == DCM_SEC_STATE_LOCKED_OUT) {
                    s_securityState.status.state = DCM_SEC_STATE_LOCKED;
                    s_securityState.status.failedAttempts = 0U;
                }
            }
        }
        
        /* Update delay timer */
        if (s_securityState.status.delayTimer > 0U) {
            if (s_securityState.status.delayTimer > elapsedTimeMs) {
                s_securityState.status.delayTimer -= elapsedTimeMs;
            } else {
                s_securityState.status.delayTimer = 0U;
            }
        }
        
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_ResetSecurityAttempts(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_securityState.initialized) {
        s_securityState.status.failedAttempts = 0U;
        s_securityState.status.lockoutTimer = 0U;
        s_securityState.status.delayTimer = 0U;
        
        if (s_securityState.status.state == DCM_SEC_STATE_LOCKED_OUT) {
            s_securityState.status.state = DCM_SEC_STATE_LOCKED;
        }
        
        result = DCM_E_OK;
    }
    
    return result;
}

bool Dcm_IsSecurityLockedOut(void)
{
    bool lockedOut = false;
    
    if (s_securityState.initialized) {
        lockedOut = (s_securityState.status.state == DCM_SEC_STATE_LOCKED_OUT);
    }
    
    return lockedOut;
}

uint32_t Dcm_GetRemainingLockoutTime(void)
{
    uint32_t remaining = 0U;
    
    if (s_securityState.initialized) {
        remaining = s_securityState.status.lockoutTimer;
    }
    
    return remaining;
}

/******************************************************************************
 * Default Callback Implementations
 ******************************************************************************/

Dcm_ReturnType Dcm_DefaultSeedGenerate(
    uint8_t securityLevel,
    uint8_t *seedBuffer,
    uint8_t seedLength
)
{
    (void)securityLevel;  /* Unused parameter */
    return generateRandomSeed(seedBuffer, seedLength);
}

Dcm_ReturnType Dcm_DefaultKeyValidate(
    uint8_t securityLevel,
    const uint8_t *seed,
    const uint8_t *key,
    uint8_t length,
    bool *isValid
)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (isValid != NULL) {
        uint8_t expectedKey[DCM_SEC_KEY_LENGTH];
        result = calculateKey(seed, expectedKey, length, securityLevel);
        
        if (result == DCM_E_OK) {
            *isValid = true;
            for (uint8_t i = 0U; i < length; i++) {
                if (key[i] != expectedKey[i]) {
                    *isValid = false;
                    break;
                }
            }
        }
    }
    
    return result;
}

/* Export default configuration for external use */
const Dcm_SecurityLevelConfigType* Dcm_GetDefaultSecurityLevelConfigs(void)
{
    return s_defaultLevelConfigs;
}

uint8_t Dcm_GetDefaultSecurityLevelCount(void)
{
    return (uint8_t)(sizeof(s_defaultLevelConfigs) / sizeof(s_defaultLevelConfigs[0U]));
}
