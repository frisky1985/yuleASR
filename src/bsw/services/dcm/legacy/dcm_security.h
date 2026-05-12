/******************************************************************************
 * @file    dcm_security.h
 * @brief   DCM Security Access Service (0x27) Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_SECURITY_H
#define DCM_SECURITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Security Access Constants (ISO 14229-1:2020 Table 77)
 ******************************************************************************/
#define DCM_SEC_REQ_SEED_MIN_LENGTH             0x01U
#define DCM_SEC_SEND_KEY_MIN_LENGTH             0x02U
#define DCM_SEC_SEED_LENGTH                     0x04U  /* 4 bytes seed */
#define DCM_SEC_KEY_LENGTH                      0x04U  /* 4 bytes key */
#define DCM_SEC_MAX_ATTEMPTS                    0x03U  /* Max failed attempts */
#define DCM_SEC_LOCKOUT_TIME_MS                 10000U /* 10 seconds lockout */
#define DCM_SEC_REQUIRED_TIME_DELAY_MS          5000U  /* 5 seconds delay */

/******************************************************************************
 * Security Access Subfunctions (ISO 14229-1:2020 Table 76)
 ******************************************************************************/
/* Request Seed subfunctions (odd values) */
#define DCM_SEC_REQ_SEED_LEVEL_1                0x01U
#define DCM_SEC_REQ_SEED_LEVEL_2                0x03U
#define DCM_SEC_REQ_SEED_LEVEL_3                0x05U
#define DCM_SEC_REQ_SEED_LEVEL_4                0x07U
#define DCM_SEC_REQ_SEED_LEVEL_5                0x09U
#define DCM_SEC_REQ_SEED_LEVEL_6                0x0BU
#define DCM_SEC_REQ_SEED_LEVEL_7                0x0DU
#define DCM_SEC_REQ_SEED_LEVEL_8                0x0FU

/* Send Key subfunctions (even values) */
#define DCM_SEC_SEND_KEY_LEVEL_1                0x02U
#define DCM_SEC_SEND_KEY_LEVEL_2                0x04U
#define DCM_SEC_SEND_KEY_LEVEL_3                0x06U
#define DCM_SEC_SEND_KEY_LEVEL_4                0x08U
#define DCM_SEC_SEND_KEY_LEVEL_5                0x0AU
#define DCM_SEC_SEND_KEY_LEVEL_6                0x0CU
#define DCM_SEC_SEND_KEY_LEVEL_7                0x0EU
#define DCM_SEC_SEND_KEY_LEVEL_8                0x10U

/******************************************************************************
 * Security Level Definitions
 ******************************************************************************/
#define DCM_SECURITY_LEVEL_LOCKED               0x00U
#define DCM_SECURITY_LEVEL_1                    0x01U
#define DCM_SECURITY_LEVEL_2                    0x02U
#define DCM_SECURITY_LEVEL_3                    0x03U
#define DCM_SECURITY_LEVEL_4                    0x04U
#define DCM_SECURITY_LEVEL_5                    0x05U
#define DCM_SECURITY_LEVEL_6                    0x06U
#define DCM_SECURITY_LEVEL_7                    0x07U
#define DCM_SECURITY_LEVEL_8                    0x08U

/******************************************************************************
 * Security Access States
 ******************************************************************************/
typedef enum {
    DCM_SEC_STATE_LOCKED = 0,               /* Security locked */
    DCM_SEC_STATE_SEED_REQUESTED,           /* Seed requested, waiting for key */
    DCM_SEC_STATE_UNLOCKED,                 /* Security unlocked */
    DCM_SEC_STATE_LOCKED_OUT                /* Temporarily locked out */
} Dcm_SecurityStateType;

/******************************************************************************
 * Security Access Result Types
 ******************************************************************************/
typedef enum {
    DCM_SEC_OK = 0,                         /* Operation successful */
    DCM_SEC_ERR_NOT_SUPPORTED,              /* Security level not supported */
    DCM_SEC_ERR_INVALID_KEY,                /* Invalid key */
    DCM_SEC_ERR_EXCEED_ATTEMPTS,            /* Exceeded number of attempts */
    DCM_SEC_ERR_TIME_DELAY_NOT_EXPIRED,     /* Required time delay not expired */
    DCM_SEC_ERR_SEQUENCE_ERROR,             /* Request sequence error */
    DCM_SEC_ERR_KEY_NOT_REQUESTED,          /* Key sent without requesting seed */
    DCM_SEC_ERR_LOCKED_OUT                  /* Security access locked out */
} Dcm_SecurityResultType;

/******************************************************************************
 * Seed Generation Callback Type
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_SeedGenerateCallback)(
    uint8_t securityLevel,
    uint8_t *seedBuffer,
    uint8_t seedLength
);

/******************************************************************************
 * Key Validation Callback Type
 ******************************************************************************/
typedef Dcm_ReturnType (*Dcm_KeyValidateCallback)(
    uint8_t securityLevel,
    const uint8_t *seed,
    const uint8_t *key,
    uint8_t length,
    bool *isValid
);

/******************************************************************************
 * Security Level Configuration
 ******************************************************************************/
typedef struct {
    uint8_t securityLevel;                  /* Security level (1-8) */
    uint8_t maxFailedAttempts;              /* Max failed attempts before lockout */
    uint32_t lockoutTimeMs;                 /* Lockout duration in ms */
    uint32_t delayTimeMs;                   /* Delay between attempts */
    uint8_t seedLength;                     /* Seed length in bytes */
    uint8_t keyLength;                      /* Key length in bytes */
    Dcm_SeedGenerateCallback seedCallback;  /* Seed generation callback */
    Dcm_KeyValidateCallback keyCallback;    /* Key validation callback */
    bool csmKeyDeriveEnabled;               /* Use CSM for key derivation */
    uint8_t csmKeyId;                       /* CSM key reference ID */
} Dcm_SecurityLevelConfigType;

/******************************************************************************
 * Security Access Configuration
 ******************************************************************************/
typedef struct {
    const Dcm_SecurityLevelConfigType *levelConfigs; /* Security level configs */
    uint8_t numSecurityLevels;              /* Number of configured levels */
    uint8_t defaultSecurityLevel;           /* Default (locked) level */
    Dcm_SecurityChangeCallback changeCallback; /* Security change callback */
} Dcm_SecurityAccessConfigType;

/******************************************************************************
 * Security Access Status
 ******************************************************************************/
typedef struct {
    uint8_t currentSecurityLevel;           /* Current unlocked level */
    Dcm_SecurityStateType state;            /* Current security state */
    uint8_t pendingSecurityLevel;           /* Level waiting for key */
    uint8_t failedAttempts;                 /* Consecutive failed attempts */
    uint32_t lockoutTimer;                  /* Lockout timer */
    uint32_t delayTimer;                    /* Delay timer */
    uint8_t lastSeed[DCM_SEC_SEED_LENGTH];  /* Last generated seed */
    bool seedValid;                         /* Seed validity flag */
    uint64_t lastAttemptTime;               /* Timestamp of last attempt */
} Dcm_SecurityStatusType;

/******************************************************************************
 * Security Access Functions
 ******************************************************************************/

/**
 * @brief Initialize security access service
 *
 * @param config Pointer to security access configuration
 * @return Dcm_ReturnType Initialization result
 *
 * @note Must be called before using security access service
 * @requirement ISO 14229-1:2020 10.6
 */
Dcm_ReturnType Dcm_SecurityAccessInit(const Dcm_SecurityAccessConfigType *config);

/**
 * @brief Process SecurityAccess (0x27) service request
 *
 * @param request Pointer to request message structure
 * @param response Pointer to response message structure
 * @return Dcm_ReturnType Service processing result
 *
 * @details Implements UDS service 0x27 for security access control
 * @requirement ISO 14229-1:2020 10.6
 */
Dcm_ReturnType Dcm_SecurityAccess(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief Get current security level
 *
 * @return uint8_t Current security level (0 = locked)
 */
uint8_t Dcm_GetSecurityLevel(void);

/**
 * @brief Set security level (for internal use or forced unlock)
 *
 * @param securityLevel Target security level
 * @return Dcm_ReturnType Result of operation
 *
 * @note This bypasses normal seed-key authentication
 */
Dcm_ReturnType Dcm_SetSecurityLevel(uint8_t securityLevel);

/**
 * @brief Check if security level is supported
 *
 * @param securityLevel Security level to check
 * @return bool True if level is supported
 */
bool Dcm_IsSecurityLevelSupported(uint8_t securityLevel);

/**
 * @brief Check if security level is unlocked
 *
 * @param securityLevel Security level to check
 * @return bool True if level is unlocked
 */
bool Dcm_IsSecurityLevelUnlocked(uint8_t securityLevel);

/**
 * @brief Get security level from subfunction
 *
 * @param subfunction Subfunction value (request seed or send key)
 * @return uint8_t Security level (0 if invalid)
 */
uint8_t Dcm_GetSecurityLevelFromSubfunction(uint8_t subfunction);

/**
 * @brief Check if subfunction is request seed
 *
 * @param subfunction Subfunction value
 * @return bool True if request seed subfunction
 */
bool Dcm_IsRequestSeedSubfunction(uint8_t subfunction);

/**
 * @brief Check if subfunction is send key
 *
 * @param subfunction Subfunction value
 * @return bool True if send key subfunction
 */
bool Dcm_IsSendKeySubfunction(uint8_t subfunction);

/**
 * @brief Generate seed for security level
 *
 * @param securityLevel Target security level
 * @param seedBuffer Buffer to store generated seed
 * @param seedLength Length of seed buffer
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GenerateSeed(
    uint8_t securityLevel,
    uint8_t *seedBuffer,
    uint8_t seedLength
);

/**
 * @brief Validate key for security level
 *
 * @param securityLevel Target security level
 * @param seed Original seed
 * @param key Key to validate
 * @param length Length of seed and key
 * @param isValid Output: true if key is valid
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ValidateKey(
    uint8_t securityLevel,
    const uint8_t *seed,
    const uint8_t *key,
    uint8_t length,
    bool *isValid
);

/**
 * @brief Lock security access (return to locked state)
 *
 * @return Dcm_ReturnType Result of operation
 *
 * @note Typically called on session transition to default
 */
Dcm_ReturnType Dcm_LockSecurityAccess(void);

/**
 * @brief Get security access status
 *
 * @param status Pointer to status structure
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_GetSecurityStatus(Dcm_SecurityStatusType *status);

/**
 * @brief Update security timers (call periodically)
 *
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 * @return Dcm_ReturnType Result of operation
 *
 * @note Should be called from Dcm_MainFunction or equivalent
 */
Dcm_ReturnType Dcm_SecurityTimerUpdate(uint32_t elapsedTimeMs);

/**
 * @brief Reset failed attempts counter
 *
 * @return Dcm_ReturnType Result of operation
 */
Dcm_ReturnType Dcm_ResetSecurityAttempts(void);

/**
 * @brief Check if security access is locked out
 *
 * @return bool True if currently locked out
 */
bool Dcm_IsSecurityLockedOut(void);

/**
 * @brief Get remaining lockout time
 *
 * @return uint32_t Remaining lockout time in milliseconds
 */
uint32_t Dcm_GetRemainingLockoutTime(void);

/******************************************************************************
 * Default Security Callbacks (Weak implementations)
 ******************************************************************************/

/**
 * @brief Default seed generation using CSM random
 *
 * @param securityLevel Security level
 * @param seedBuffer Output buffer for seed
 * @param seedLength Seed length
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_DefaultSeedGenerate(
    uint8_t securityLevel,
    uint8_t *seedBuffer,
    uint8_t seedLength
);

/**
 * @brief Default key validation using CSM key derive
 *
 * @param securityLevel Security level
 * @param seed Seed value
 * @param key Key to validate
 * @param length Data length
 * @param isValid Output validation result
 * @return Dcm_ReturnType Result
 */
Dcm_ReturnType Dcm_DefaultKeyValidate(
    uint8_t securityLevel,
    const uint8_t *seed,
    const uint8_t *key,
    uint8_t length,
    bool *isValid
);

#ifdef __cplusplus
}
#endif

#endif /* DCM_SECURITY_H */
