/******************************************************************************
 * @file    ecc_checker.h
 * @brief   ECC Checker - SECDED Error Detection and Correction
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * Features:
 * - Single-bit error detection and correction
 * - Double-bit error detection (uncorrectable)
 * - Error position identification
 * - Error statistics tracking
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef ECC_CHECKER_H
#define ECC_CHECKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"
#include "ecc_encoder.h"
#include <stddef.h>

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Error detection results */
#define ECC_ERROR_NONE          0x00U   /* No error */
#define ECC_ERROR_SINGLE_BIT    0x01U   /* Single-bit error (correctable) */
#define ECC_ERROR_DOUBLE_BIT    0x02U   /* Double-bit error (uncorrectable) */
#define ECC_ERROR_MULTI_BIT     0x04U   /* Multi-bit error (uncorrectable) */
#define ECC_ERROR_ECC_CORRUPT   0x08U   /* ECC code corrupted */

/* Error position special values */
#define ECC_ERROR_POS_NONE      0xFFU   /* No error or unknown position */
#define ECC_ERROR_POS_ECC       0xFEU   /* Error in ECC bits only */

/* Maximum error position for 32-bit data (0-31 data bits, 32-38 ECC bits) */
#define ECC_MAX_POS_32BIT       38U
#define ECC_MAX_POS_64BIT       71U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Error detection result type */
typedef uint8_t EccErrorType;

/* ECC check result structure */
typedef struct {
    EccErrorType error_type;        /* Type of error detected */
    uint8_t error_position;         /* Bit position of error (0-31/63 for data, 32+ for ECC) */
    uint32_t corrected_data;        /* Corrected 32-bit data (if correctable) */
    uint64_t corrected_data64;      /* Corrected 64-bit data (if correctable) */
    boolean was_corrected;          /* TRUE if error was corrected */
    uint8_t original_ecc;           /* Original ECC code read from memory */
    uint8_t computed_ecc;           /* Recomputed ECC code */
    uint8_t syndrome;               /* Error syndrome (XOR of ECC codes) */
} EccCheckResultType;

/* ECC checker configuration */
typedef struct {
    boolean auto_correct;           /* TRUE: automatically correct single-bit errors */
    boolean log_errors;             /* TRUE: log all errors detected */
    uint8_t max_errors_before_alert; /* Threshold for error alert */
    boolean use_hardware_checker;   /* TRUE: use hardware ECC checker if available */
} EccCheckerConfigType;

/* ECC error statistics */
typedef struct {
    uint32_t total_checks;          /* Total number of checks performed */
    uint32_t single_bit_errors;     /* Number of single-bit errors detected */
    uint32_t double_bit_errors;     /* Number of double-bit errors detected */
    uint32_t multi_bit_errors;      /* Number of multi-bit errors detected */
    uint32_t corrections_made;      /* Number of corrections performed */
    uint32_t alerts_triggered;      /* Number of error alerts triggered */
    uint8_t last_error_position;    /* Position of last error */
    uint32_t error_rate;            /* Errors per 1000 checks (x1000) */
} EccErrorStatsType;

/* ECC checker state */
typedef struct {
    boolean initialized;
    EccCheckerConfigType config;
    EccErrorStatsType stats;
} EccCheckerStateType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize ECC checker
 * @param config Pointer to checker configuration
 * @return E_OK on success, E_NOT_OK on error
 */
Std_ReturnType EccChecker_Init(const EccCheckerConfigType *config);

/**
 * @brief Deinitialize ECC checker
 * @return E_OK on success
 */
Std_ReturnType EccChecker_DeInit(void);

/**
 * @brief Check and correct 32-bit data with ECC7
 * @param data 32-bit data to check
 * @param ecc_code ECC7 code associated with data
 * @param result Pointer to store check result
 * @return E_OK if no error or corrected, E_NOT_OK if uncorrectable error
 */
Std_ReturnType EccChecker_CheckAndCorrect32(uint32_t data, 
                                             uint8_t ecc_code,
                                             EccCheckResultType *result);

/**
 * @brief Check and correct 64-bit data with ECC8
 * @param data 64-bit data to check
 * @param ecc_code ECC8 code associated with data
 * @param result Pointer to store check result
 * @return E_OK if no error or corrected, E_NOT_OK if uncorrectable error
 */
Std_ReturnType EccChecker_CheckAndCorrect64(uint64_t data,
                                             uint8_t ecc_code,
                                             EccCheckResultType *result);

/**
 * @brief Check data without correction
 * @param data 32-bit data to check
 * @param ecc_code ECC7 code
 * @param result Pointer to store check result
 * @return E_OK on success, E_NOT_OK if error detected
 */
Std_ReturnType EccChecker_CheckOnly32(uint32_t data,
                                       uint8_t ecc_code,
                                       EccCheckResultType *result);

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
 * @brief Get string description of error type
 * @param error_type Error type code
 * @return Human-readable error description
 */
const char* EccChecker_GetErrorString(EccErrorType error_type);

/**
 * @brief Get detailed error position description
 * @param position Error position code
 * @param mode ECC mode (32/64-bit)
 * @return Description string
 */
const char* EccChecker_GetPositionString(uint8_t position, EccEncodeModeType mode);

/**
 * @brief Get version information
 * @param version Pointer to version info structure
 */
void EccChecker_GetVersionInfo(Std_VersionInfoType *version);

/******************************************************************************
 * Inline Helper Functions
 ******************************************************************************/

/**
 * @brief Check if error is correctable
 * @param error_type Error type code
 * @return TRUE if error can be corrected
 */
static inline boolean EccChecker_IsCorrectable(EccErrorType error_type)
{
    return (error_type == ECC_ERROR_SINGLE_BIT) ? TRUE : FALSE;
}

/**
 * @brief Check if error is uncorrectable (fatal)
 * @param error_type Error type code
 * @return TRUE if error is uncorrectable
 */
static inline boolean EccChecker_IsUncorrectable(EccErrorType error_type)
{
    return ((error_type & (ECC_ERROR_DOUBLE_BIT | 
                           ECC_ERROR_MULTI_BIT | 
                           ECC_ERROR_ECC_CORRUPT)) != 0U) ? TRUE : FALSE;
}

#ifdef __cplusplus
}
#endif

#endif /* ECC_CHECKER_H */
