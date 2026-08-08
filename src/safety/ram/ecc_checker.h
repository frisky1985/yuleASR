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

/* ECC check status */
typedef enum {
    ECC_CHECK_OK = 0,           /* No error */
    ECC_CHECK_SINGLE_ERROR,     /* Single bit error corrected */
    ECC_CHECK_DOUBLE_ERROR,     /* Double bit error detected */
    ECC_CHECK_ERROR             /* General error */
} EccCheckStatusType;

/* ECC error classification (design review CR-1) */
typedef enum {
    ECC_ERROR_NONE = 0,         /* No error */
    ECC_ERROR_SINGLE_BIT,       /* Single bit error (correctable) */
    ECC_ERROR_DOUBLE_BIT,       /* Double bit error (uncorrectable) */
    ECC_ERROR_MULTI_BIT,        /* Multi bit error (>2 bits, uncorrectable) */
    ECC_ERROR_ECC_CORRUPT       /* ECC code itself corrupted */
} EccErrorType;

/* Error position sentinels */
#define ECC_ERROR_POS_NONE      0xFFU    /* No error position */
#define ECC_ERROR_POS_ECC       0xFEU    /* Error in ECC bits only */

/* ECC checker result structure */
typedef struct {
    EccErrorType error_type;    /* Error classification */
    uint8_t error_position;     /* Error bit position (if single error) */
    uint32_t corrected_data;    /* Corrected data (32-bit) */
    uint64_t corrected_data64;  /* Corrected data (64-bit) */
    boolean was_corrected;      /* TRUE if single-bit error was corrected */
    uint8_t original_ecc;       /* Original ECC code */
    uint8_t computed_ecc;       /* Computed ECC code */
    uint8_t syndrome;           /* Error syndrome */
} EccCheckResultType;

/* ECC checker result structure (status-oriented API) */
typedef struct {
    uint32_t data;              /* Corrected data (32-bit) */
    uint64_t data64;            /* Corrected data (64-bit) */
    uint8_t ecc_code;           /* Original ECC code */
    uint8_t syndrome;           /* Error syndrome */
    EccCheckStatusType result;  /* Check status */
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

/* ECC error statistics - snake_case fields matching implementation */
typedef struct {
    uint32_t total_checks;              /* Total ECC checks performed */
    uint32_t single_bit_errors;         /* Single-bit errors detected and corrected */
    uint32_t double_bit_errors;         /* Double-bit errors detected (uncorrectable) */
    uint32_t multi_bit_errors;          /* Multi-bit errors (>2 bits) */
    uint32_t corrections_made;          /* Successfully corrected errors */
    uint32_t error_rate;                /* Error rate (per 1000 checks) */
    uint32_t last_error_position;       /* Position of last error */
    uint32_t alerts_triggered;          /* Alert counter */
} EccErrorStatsType;

/* ECC checker configuration - snake_case fields matching implementation */
typedef struct {
    boolean auto_correct;               /* Automatically correct single-bit errors */
    boolean log_errors;                 /* Log all errors */
    uint32_t max_errors_before_alert;   /* Max errors before alert */
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
 * @brief Check and correct 32-bit data with ECC7 (detailed result)
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store detailed result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_CheckAndCorrect32(uint32_t data, uint8_t ecc_code, EccCheckResultType *result);

/**
 * @brief Check and correct 64-bit data with ECC8 (detailed result)
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store detailed result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_CheckAndCorrect64(uint64_t data, uint8_t ecc_code, EccCheckResultType *result);

/**
 * @brief Check 32-bit data without correction (check-only mode)
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store detailed result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_CheckOnly32(uint32_t data, uint8_t ecc_code, EccCheckResultType *result);

/**
 * @brief Check and correct 32-bit data with ECC7 (status-oriented API)
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store status result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Check32(uint32_t data, uint8_t ecc_code, EccCheckedType* result);

/**
 * @brief Check and correct 64-bit data with ECC8 (status-oriented API)
 * @param data Data to check
 * @param ecc_code ECC code
 * @param result Pointer to store status result
 * @return E_OK on success
 */
Std_ReturnType EccChecker_Check64(uint64_t data, uint8_t ecc_code, EccCheckedType* result);

/**
 * @brief Check if error type is uncorrectable
 * @param error_type Error classification
 * @return TRUE if uncorrectable (double/multi bit)
 */
boolean EccChecker_IsUncorrectable(EccErrorType error_type);

/**
 * @brief Get error string for error type
 * @param error_type Error classification
 * @return Static string description
 */
const char* EccChecker_GetErrorString(EccErrorType error_type);

/**
 * @brief Get detailed error position description
 * @param position Error bit position
 * @param mode ECC mode (32/64 bit)
 * @return Static string description
 */
const char* EccChecker_GetPositionString(uint8_t position, EccEncodeModeType mode);

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
 * @brief Get error statistics
 * @return Pointer to error statistics
 */
const EccErrorStatsType* EccChecker_GetStats(void);

/**
 * @brief Reset error statistics
 * @return E_OK on success
 */
Std_ReturnType EccChecker_ResetStats(void);

/**
 * @brief Get version information - CRITICAL FIX: Added missing API
 * @param version Pointer to version info structure
 */
void EccChecker_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* ECC_CHECKER_H */
