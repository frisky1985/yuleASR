/******************************************************************************
 * @file    ecc_checker.c
 * @brief   ECC Checker - SECDED Error Detection and Correction Implementation
 *
 * Error detection using syndrome calculation:
 * - Syndrome = computed_ecc XOR stored_ecc
 * - Syndrome = 0: No error
 * - Syndrome matches Hamming matrix column: Single-bit error at that position
 * - Syndrome != 0 but doesn't match: Double-bit or multi-bit error
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ecc_checker.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Local Constants
 ******************************************************************************/

/* Hamming matrix column positions for 32-bit SECDED
 * Maps syndrome values to error positions
 * Position 0-31: data bits, Position 32-38: ECC bits */
static const uint8_t ecc7_syndrome_map[128] = {
    /* Syndrome 0-127 mapping to bit positions */
    0xFF, 0x00, 0x01, 0xFF, 0x02, 0xFF, 0xFF, 0x20,  /* 0-7 */
    0x03, 0xFF, 0xFF, 0x21, 0xFF, 0x22, 0x23, 0xFF,  /* 8-15 */
    0x04, 0xFF, 0xFF, 0x24, 0xFF, 0x25, 0x26, 0xFF,  /* 16-23 */
    0xFF, 0x27, 0x28, 0xFF, 0x29, 0xFF, 0xFF, 0xFF,  /* 24-31 */
    0x05, 0xFF, 0xFF, 0x2A, 0xFF, 0x2B, 0x2C, 0xFF,  /* 32-39 */
    0xFF, 0x2D, 0x2E, 0xFF, 0x2F, 0xFF, 0xFF, 0xFF,  /* 40-47 */
    0xFF, 0x30, 0x31, 0xFF, 0x32, 0xFF, 0xFF, 0xFF,  /* 48-55 */
    0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 56-63 */
    0x06, 0xFF, 0xFF, 0x34, 0xFF, 0x35, 0x36, 0xFF,  /* 64-71 */
    0xFF, 0x37, 0x38, 0xFF, 0x39, 0xFF, 0xFF, 0xFF,  /* 72-79 */
    0xFF, 0x3A, 0x3B, 0xFF, 0x3C, 0xFF, 0xFF, 0xFF,  /* 80-87 */
    0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 88-95 */
    0xFF, 0x3E, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 96-103 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 104-111 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 112-119 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF   /* 120-127 */
};

/* Module version */
#define ECC_CHECKER_VENDOR_ID   0x0001U
#define ECC_CHECKER_MODULE_ID   0x0101U
#define ECC_CHECKER_SW_MAJOR    1U
#define ECC_CHECKER_SW_MINOR    0U
#define ECC_CHECKER_SW_PATCH    0U

/******************************************************************************
 * Local Variables
 ******************************************************************************/

static EccCheckerStateType g_checker_state;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

static void EccChecker_UpdateStats(EccErrorType error_type, uint8_t position);
static uint8_t EccChecker_ComputeSyndrome32(uint32_t data, uint8_t ecc_code);
static uint8_t EccChecker_ComputeSyndrome64(uint64_t data, uint8_t ecc_code);
static void EccChecker_ClearResult(EccCheckResultType *result);

/******************************************************************************
 * Local Function Implementations
 ******************************************************************************/

/**
 * @brief Clear result structure
 */
static void EccChecker_ClearResult(EccCheckResultType *result)
{
    if (result != NULL) {
        result->error_type = ECC_ERROR_NONE;
        result->error_position = ECC_ERROR_POS_NONE;
        result->corrected_data = 0U;
        result->corrected_data64 = 0ULL;
        result->was_corrected = FALSE;
        result->original_ecc = 0U;
        result->computed_ecc = 0U;
        result->syndrome = 0U;
    }
}

/**
 * @brief Compute syndrome for 32-bit data
 * Syndrome indicates which bits are in error
 */
static uint8_t EccChecker_ComputeSyndrome32(uint32_t data, uint8_t ecc_code)
{
    uint8_t computed_ecc;
    uint8_t syndrome;

    /* Get computed ECC (re-encode data) */
    EccEncoder_Encode32(data, &computed_ecc);

    /* Syndrome is XOR of stored and computed ECC */
    syndrome = ecc_code ^ computed_ecc;

    return syndrome;
}

/**
 * @brief Compute syndrome for 64-bit data
 */
static uint8_t EccChecker_ComputeSyndrome64(uint64_t data, uint8_t ecc_code)
{
    uint8_t computed_ecc;
    uint8_t syndrome;

    /* Get computed ECC (re-encode data) */
    EccEncoder_Encode64(data, &computed_ecc);

    /* Syndrome is XOR of stored and computed ECC */
    syndrome = ecc_code ^ computed_ecc;

    return syndrome;
}

/**
 * @brief Update error statistics
 */
static void EccChecker_UpdateStats(EccErrorType error_type, uint8_t position)
{
    g_checker_state.stats.total_checks++;
    g_checker_state.stats.last_error_position = position;

    switch (error_type) {
        case ECC_ERROR_SINGLE_BIT:
            g_checker_state.stats.single_bit_errors++;
            if (g_checker_state.config.auto_correct) {
                g_checker_state.stats.corrections_made++;
            }
            break;

        case ECC_ERROR_DOUBLE_BIT:
            g_checker_state.stats.double_bit_errors++;
            break;

        case ECC_ERROR_MULTI_BIT:
            g_checker_state.stats.multi_bit_errors++;
            break;

        default:
            break;
    }

    /* Calculate error rate (errors per 1000 checks * 1000) */
    if (g_checker_state.stats.total_checks > 0U) {
        uint32_t total_errors = g_checker_state.stats.single_bit_errors +
                               g_checker_state.stats.double_bit_errors +
                               g_checker_state.stats.multi_bit_errors;
        g_checker_state.stats.error_rate = 
            (total_errors * 1000U) / g_checker_state.stats.total_checks;
    }

    /* Check alert threshold */
    if (g_checker_state.config.max_errors_before_alert > 0U) {
        uint32_t total_errors = g_checker_state.stats.single_bit_errors +
                               g_checker_state.stats.double_bit_errors +
                               g_checker_state.stats.multi_bit_errors;
        if (total_errors >= g_checker_state.config.max_errors_before_alert) {
            g_checker_state.stats.alerts_triggered++;
        }
    }
}

/******************************************************************************
 * Public API Implementation
 ******************************************************************************/

/**
 * @brief Initialize ECC checker
 */
Std_ReturnType EccChecker_Init(const EccCheckerConfigType *config)
{
    if (config == NULL) {
        return E_NOT_OK;
    }

    /* Clear state */
    memset(&g_checker_state, 0, sizeof(g_checker_state));

    /* Copy configuration */
    g_checker_state.config = *config;
    g_checker_state.initialized = TRUE;

    g_initialized = TRUE;

    return E_OK;
}

/**
 * @brief Deinitialize ECC checker
 */
Std_ReturnType EccChecker_DeInit(void)
{
    memset(&g_checker_state, 0, sizeof(g_checker_state));
    g_initialized = FALSE;
    return E_OK;
}

/**
 * @brief Check and correct 32-bit data with ECC7
 */
Std_ReturnType EccChecker_CheckAndCorrect32(uint32_t data,
                                             uint8_t ecc_code,
                                             EccCheckResultType *result)
{
    Std_ReturnType status = E_OK;
    uint8_t syndrome;
    uint8_t error_pos;

    /* Safety checks */
    if (g_initialized == FALSE) {
        return E_NOT_OK;
    }

    if (result == NULL) {
        return E_NOT_OK;
    }

    /* Clear result structure */
    EccChecker_ClearResult(result);
    result->original_ecc = ecc_code;

    /* Compute syndrome */
    syndrome = EccChecker_ComputeSyndrome32(data, ecc_code);
    result->syndrome = syndrome;
    result->computed_ecc = ecc_code ^ syndrome;

    /* Syndrome = 0: No error */
    if (syndrome == 0U) {
        result->error_type = ECC_ERROR_NONE;
        result->corrected_data = data;
        g_checker_state.stats.total_checks++;
        return E_OK;
    }

    /* Look up error position from syndrome */
    error_pos = ecc7_syndrome_map[syndrome & 0x7FU];

    if (error_pos == 0xFFU) {
        /* Syndrome doesn't match any valid position - multi-bit error */
        result->error_type = ECC_ERROR_MULTI_BIT;
        result->error_position = ECC_ERROR_POS_NONE;
        EccChecker_UpdateStats(ECC_ERROR_MULTI_BIT, ECC_ERROR_POS_NONE);
        return E_NOT_OK;
    }

    /* Check if this is a double-bit error using overall parity bit */
    if ((syndrome & 0x40U) != 0U) {
        /* Parity bit set - odd number of errors */
        if (error_pos < 32U) {
            /* Single-bit error in data */
            result->error_type = ECC_ERROR_SINGLE_BIT;
            result->error_position = error_pos;
            result->corrected_data = data ^ (1U << error_pos);
            result->was_corrected = g_checker_state.config.auto_correct;
            EccChecker_UpdateStats(ECC_ERROR_SINGLE_BIT, error_pos);
            
            if (!g_checker_state.config.auto_correct) {
                status = E_NOT_OK;  /* Report error if not auto-correcting */
            }
        } else {
            /* Single-bit error in ECC code only - data is correct */
            result->error_type = ECC_ERROR_SINGLE_BIT;
            result->error_position = error_pos;
            result->corrected_data = data;
            result->was_corrected = FALSE;
            EccChecker_UpdateStats(ECC_ERROR_SINGLE_BIT, error_pos);
        }
    } else {
        /* Parity bit not set - even number of errors (double-bit) */
        result->error_type = ECC_ERROR_DOUBLE_BIT;
        result->error_position = ECC_ERROR_POS_NONE;
        EccChecker_UpdateStats(ECC_ERROR_DOUBLE_BIT, ECC_ERROR_POS_NONE);
        status = E_NOT_OK;
    }

    return status;
}

/**
 * @brief Check and correct 64-bit data with ECC8
 */
Std_ReturnType EccChecker_CheckAndCorrect64(uint64_t data,
                                             uint8_t ecc_code,
                                             EccCheckResultType *result)
{
    Std_ReturnType status = E_OK;
    uint8_t syndrome;

    /* Safety checks */
    if (g_initialized == FALSE) {
        return E_NOT_OK;
    }

    if (result == NULL) {
        return E_NOT_OK;
    }

    /* Clear result structure */
    EccChecker_ClearResult(result);
    result->original_ecc = ecc_code;

    /* Compute syndrome */
    syndrome = EccChecker_ComputeSyndrome64(data, ecc_code);
    result->syndrome = syndrome;
    result->computed_ecc = ecc_code ^ syndrome;

    /* Syndrome = 0: No error */
    if (syndrome == 0U) {
        result->error_type = ECC_ERROR_NONE;
        result->corrected_data64 = data;
        g_checker_state.stats.total_checks++;
        return E_OK;
    }

    /* For 64-bit, check overall parity (bit 7) */
    if ((syndrome & 0x80U) != 0U) {
        /* Odd number of errors - single-bit error */
        uint8_t hamming_syndrome = syndrome & 0x7FU;
        
        /* Calculate error position from Hamming syndrome */
        /* Position is derived from syndrome bits (simplified for this implementation) */
        uint8_t error_pos = 0xFFU;
        
        /* Try to identify the error position */
        if (hamming_syndrome > 0U && hamming_syndrome <= 64U) {
            /* Syndrome matches a data bit position (simplified mapping) */
            error_pos = hamming_syndrome - 1U;
        }

        if (error_pos != 0xFFU && error_pos < 64U) {
            /* Correctable single-bit error in data */
            result->error_type = ECC_ERROR_SINGLE_BIT;
            result->error_position = error_pos;
            result->corrected_data64 = data ^ (1ULL << error_pos);
            result->was_corrected = g_checker_state.config.auto_correct;
            EccChecker_UpdateStats(ECC_ERROR_SINGLE_BIT, error_pos);
            
            if (!g_checker_state.config.auto_correct) {
                status = E_NOT_OK;
            }
        } else {
            /* Error in ECC bits or unknown position */
            result->error_type = ECC_ERROR_SINGLE_BIT;
            result->error_position = ECC_ERROR_POS_ECC;
            result->corrected_data64 = data;
            result->was_corrected = FALSE;
            EccChecker_UpdateStats(ECC_ERROR_SINGLE_BIT, ECC_ERROR_POS_ECC);
        }
    } else {
        /* Even number of errors - double-bit error (uncorrectable) */
        result->error_type = ECC_ERROR_DOUBLE_BIT;
        result->error_position = ECC_ERROR_POS_NONE;
        EccChecker_UpdateStats(ECC_ERROR_DOUBLE_BIT, ECC_ERROR_POS_NONE);
        status = E_NOT_OK;
    }

    return status;
}

/**
 * @brief Check data without correction
 */
Std_ReturnType EccChecker_CheckOnly32(uint32_t data,
                                       uint8_t ecc_code,
                                       EccCheckResultType *result)
{
    Std_ReturnType status;
    boolean saved_auto_correct;

    /* Temporarily disable auto-correction */
    saved_auto_correct = g_checker_state.config.auto_correct;
    g_checker_state.config.auto_correct = FALSE;

    /* Perform check */
    status = EccChecker_CheckAndCorrect32(data, ecc_code, result);

    /* Restore auto-correction setting */
    g_checker_state.config.auto_correct = saved_auto_correct;

    return status;
}

/**
 * @brief Get error statistics
 */
const EccErrorStatsType* EccChecker_GetStats(void)
{
    return &g_checker_state.stats;
}

/**
 * @brief Reset error statistics
 */
Std_ReturnType EccChecker_ResetStats(void)
{
    memset(&g_checker_state.stats, 0, sizeof(g_checker_state.stats));
    return E_OK;
}

/**
 * @brief Get string description of error type
 */
const char* EccChecker_GetErrorString(EccErrorType error_type)
{
    switch (error_type) {
        case ECC_ERROR_NONE:
            return "No Error";
        case ECC_ERROR_SINGLE_BIT:
            return "Single-Bit Error (Correctable)";
        case ECC_ERROR_DOUBLE_BIT:
            return "Double-Bit Error (Uncorrectable)";
        case ECC_ERROR_MULTI_BIT:
            return "Multi-Bit Error (Uncorrectable)";
        case ECC_ERROR_ECC_CORRUPT:
            return "ECC Code Corrupted";
        default:
            return "Unknown Error";
    }
}

/**
 * @brief Get detailed error position description
 */
const char* EccChecker_GetPositionString(uint8_t position, EccEncodeModeType mode)
{
    static char buffer[32];

    if (position == ECC_ERROR_POS_NONE) {
        return "No error / Unknown";
    } else if (position == ECC_ERROR_POS_ECC) {
        return "ECC bits only";
    }

    if (mode == ECC_MODE_32BIT) {
        if (position < 32U) {
            snprintf(buffer, sizeof(buffer), "Data bit %u", (unsigned)position);
        } else if (position <= 38U) {
            snprintf(buffer, sizeof(buffer), "ECC bit %u", (unsigned)(position - 32U));
        } else {
            return "Invalid position";
        }
    } else {
        if (position < 64U) {
            snprintf(buffer, sizeof(buffer), "Data bit %u", (unsigned)position);
        } else if (position <= 71U) {
            snprintf(buffer, sizeof(buffer), "ECC bit %u", (unsigned)(position - 64U));
        } else {
            return "Invalid position";
        }
    }

    return buffer;
}

/**
 * @brief Get version information
 */
void EccChecker_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = ECC_CHECKER_VENDOR_ID;
        version->moduleID = ECC_CHECKER_MODULE_ID;
        version->sw_major_version = ECC_CHECKER_SW_MAJOR;
        version->sw_minor_version = ECC_CHECKER_SW_MINOR;
        version->sw_patch_version = ECC_CHECKER_SW_PATCH;
    }
}
