/******************************************************************************
 * @file    ecc_checker.h
 * @brief   ECC Checker - SECDED Error Detection and Correction
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#ifndef ECC_CHECKER_H
#define ECC_CHECKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "ecc_encoder.h"

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* ECC check result */
typedef enum {
    ECC_CHECK_OK = 0,           /* No error */
    ECC_CHECK_SINGLE_ERROR,     /* Single bit error corrected */
    ECC_CHECK_DOUBLE_ERROR,     /* Double bit error detected */
    ECC_CHECK_ERROR             /* General error */
} EccCheckResultType;

/* ECC checker result structure */
typedef struct {
    uint32_t data;              /* Corrected data (32-bit) */
    uint64_t data64;            /* Corrected data (64-bit) */
    uint8_t ecc_code;           /* Original ECC code */
    uint8_t syndrome;           /* Error syndrome */
    EccCheckResultType result;  /* Check result */
    uint8_t error_position;     /* Error bit position (if single error) */
} EccCheckedType;

/* ECC checker state - CRITICAL FIX: Added missing type */
typedef enum {
    ECC_CHECKER_STATE_UNINIT = 0,       /* Uninitialized */
    ECC_CHECKER_STATE_INIT,             /* Initialized */
    ECC_CHECKER_STATE_ACTIVE,           /* Active and operational */
    ECC_CHECKER_STATE_ERROR,            /* Error state */
    ECC_CHECKER_STATE_RECOVERY          /* Recovery in progress */
} EccCheckerStateType;

/* ECC error statistics - CRITICAL FIX: Added missing type */
typedef struct {
    uint32_t totalChecks;               /* Total ECC checks performed */
    uint32_t singleBitErrors;           /* Single-bit errors detected and corrected */
    uint32_t doubleBitErrors;           /* Double-bit errors detected (uncorrectable) */
    uint32_t multiBitErrors;            /* Multi-bit errors (>2 bits) */
    uint32_t correctedErrors;           /* Successfully corrected errors */
    uint32_t uncorrectableErrors;       /* Errors that could not be corrected */
    uint32_t lastErrorAddress;          /* Address of last error (if available) */
    uint32_t lastErrorTime;             /* Timestamp of last error */
    uint8_t consecutiveErrors;          /* Consecutive error count */
    boolean errorRateHigh;              /* Error rate above threshold */
} EccErrorStatsType;

/* ECC checker configuration - CRITICAL FIX: Added missing type */
typedef struct {
    boolean autoCorrect;                /* Automatically correct single-bit errors */
    boolean logErrors;                  /* Log all errors */
    boolean notifyDoubleBit;            /* Notify on double-bit errors */
    uint8_t maxConsecutiveErrors;       /* Max consecutive errors before alert */
    uint32_t errorRateThreshold;        /* Error rate threshold (errors per hour) */
} EccCheckerConfigType;

/* ECC checker instance structure - CRITICAL FIX: Added missing type */
typedef struct {
    EccCheckerStateType state;          /* Current checker state */
    EccCheckerConfigType config;        /* Checker configuration */
    EccErrorStatsType stats;            /* Error statistics */
    boolean initialized;                /* Initialization flag */
    uint32_t initTime;                  /* Initialization timestamp */
} EccCheckerType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Check and correct 32-bit data with ECC7
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Check32(uint32_t data, uint8_t ecc_code, EccCheckedType* result);

/**
 * @brief Check and correct 64-bit data with ECC8
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Check64(uint64_t data, uint8_t ecc_code, EccCheckedType* result);

/**
 * @brief Calculate syndrome for 32-bit data
 * @param data Data
 * @param ecc_code ECC code
 * @return Syndrome value
 */
uint8_t EccChecker_CalculateSyndrome32(uint32_t data, uint8_t ecc_code);

/**
 * @brief Calculate syndrome for 64-bit data
 * @param data Data
 * @param ecc_code ECC code
 * @return Syndrome value
 */
uint8_t EccChecker_CalculateSyndrome64(uint64_t data, uint8_t ecc_code);

/**
 * @brief Initialize ECC checker - CRITICAL FIX: Added missing API
 * @param config Pointer to checker configuration (NULL for defaults)
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Init(const EccCheckerConfigType *config);

/**
 * @brief Deinitialize ECC checker - CRITICAL FIX: Added missing API
 * @return E_OK on success
 */
Std_ReturnType EccChecker_DeInit(void);

/**
 * @brief Get checker state - CRITICAL FIX: Added missing API
 * @return Pointer to checker state
 */
const EccCheckerType* EccChecker_GetState(void);

/**
 * @brief Get error statistics - CRITICAL FIX: Added missing API
 * @return Pointer to error statistics
 */
const EccErrorStatsType* EccChecker_GetStats(void);

/**
 * @brief Reset error statistics - CRITICAL FIX: Added missing API
 * @return E_OK on success
 */
Std_ReturnType EccChecker_ResetStats(void);

/**
 * @brief Check if checker is healthy - CRITICAL FIX: Added missing API
 * @return TRUE if checker is operating normally
 */
boolean EccChecker_IsHealthy(void);

/**
 * @brief Get version information - CRITICAL FIX: Added missing API
 * @param version Pointer to version info structure
 */
void EccChecker_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* ECC_CHECKER_H */
