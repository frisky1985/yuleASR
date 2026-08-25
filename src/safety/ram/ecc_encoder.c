/******************************************************************************
 * @file    ecc_encoder.c
 * @brief   ECC Encoder - SECDED Algorithm Implementation
 *
 * SECDED (Single Error Correction, Double Error Detection) encoding:
 * - Uses Hamming code with additional overall parity bit
 * - 32-bit data: 7 ECC bits (6 Hamming + 1 parity)
 * - 64-bit data: 8 ECC bits (7 Hamming + 1 parity)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
/* @req SHALL_RAM */


#include "ecc_encoder.h"
#include <string.h>

/******************************************************************************
 * Local Constants
 ******************************************************************************/

/* SECDED Hamming matrix for 32-bit data (ECC7)
 * Each row represents the positions of data bits that affect that ECC bit
 * Bit positions are 0-indexed: bit 0 = LSB
 * Matrix regenerated (2026-08) to guarantee SECDED properties:
 * - all columns unique (single-bit syndromes unambiguous)
 * - odd column weights (single vs double error discrimination)
 * - no XOR of two columns collides with a third (double errors not
 *   misread as single errors)
 */
static const uint32_t ecc7_matrix[7] = {
    /* ECC bit 0 */
    0x929E55BBU,
    /* ECC bit 1 */
    0xB4ACA69FU,
    /* ECC bit 2 */
    0xC6F534E9U,
    /* ECC bit 3 */
    0xE739D8CEU,
    /* ECC bit 4 */
    0xF83E1F0CU,
    /* ECC bit 5 */
    0xFFC01FF0U,
    /* ECC bit 6 (overall parity) */
    0xFFFFE000U
};

/* SECDED Hamming matrix for 64-bit data (ECC8)
 * Uses 8-bit ECC code */
static const uint64_t ecc8_matrix[8] = {
    0xAAAAAAAAAAAAAAA5ULL,  /* P1 */
    0xCCCCCCCCCCCCCC93ULL,  /* P2 */
    0xF0F0F0F0F0F0F0E8ULL,  /* P4 */
    0xFF00FF00FF00FF00ULL,  /* P8 */
    0xFFFF0000FFFF0000ULL,  /* P16 */
    0xFFFFFFFF00000000ULL,  /* P32 */
    0xFFFFFFFFFFFFFFFFULL,  /* P64 */
    0xFFFFFFFFFFFFFFFFULL   /* Overall parity */
};

/* Module version information */
#define ECC_ENCODER_VENDOR_ID       0x0001U
#define ECC_ENCODER_MODULE_ID       0x0100U
#define ECC_ENCODER_SW_MAJOR        1U
#define ECC_ENCODER_SW_MINOR        0U
#define ECC_ENCODER_SW_PATCH        0U

/******************************************************************************
 * Local Variables
 ******************************************************************************/

static EccEncoderStateType g_encoder_state;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

static uint8_t EccEncoder_ComputeEcc7(uint32_t data);
static uint8_t EccEncoder_ComputeEcc8(uint64_t data);
static uint8_t EccEncoder_PopCount32(uint32_t value);
static uint8_t EccEncoder_PopCount64(uint64_t value);

/******************************************************************************
 * Local Function Implementations
 ******************************************************************************/

/**
 * @brief Count number of set bits in 32-bit value (population count)
 */
static uint8_t EccEncoder_PopCount32(uint32_t value)
{
    /* Hacker's Delight popcount algorithm */
    value = value - ((value >> 1) & 0x55555555U);
    value = (value & 0x33333333U) + ((value >> 2) & 0x33333333U);
    value = (value + (value >> 4)) & 0x0F0F0F0FU;
    value = value + (value >> 8);
    value = value + (value >> 16);
    return (uint8_t)(value & 0x3FU);
}

/**
 * @brief Count number of set bits in 64-bit value
 */
static uint8_t EccEncoder_PopCount64(uint64_t value)
{
    value = value - ((value >> 1) & 0x5555555555555555ULL);
    value = (value & 0x3333333333333333ULL) + ((value >> 2) & 0x3333333333333333ULL);
    value = (value + (value >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    value = value + (value >> 8);
    value = value + (value >> 16);
    value = value + (value >> 32);
    return (uint8_t)(value & 0x7FU);
}

/**
 * @brief Compute 7-bit ECC code for 32-bit data using SECDED
 *
 * ECC bit calculation:
 * - P1  = D0  ^ D1  ^ D3  ^ D4  ^ D6  ^ D8  ^ D9  ^ D11 ^ D13 ^ D15 ^ D17 ^ D19 ^ D21 ^ D23 ^ D25 ^ D27 ^ D29 ^ D31
 * - P2  = D0  ^ D2  ^ D3  ^ D5  ^ D6  ^ D10 ^ D11 ^ D14 ^ D15 ^ D18 ^ D19 ^ D22 ^ D23 ^ D26 ^ D27 ^ D30 ^ D31
 * - P4  = D1  ^ D2  ^ D3  ^ D7  ^ D8  ^ D9  ^ D10 ^ D11 ^ D20 ^ D21 ^ D22 ^ D23 ^ D28 ^ D29 ^ D30 ^ D31
 * - P8  = D4  ^ D5  ^ D6  ^ D7  ^ D8  ^ D9  ^ D10 ^ D11
 * - P16 = D12 ^ D13 ^ D14 ^ D15 ^ D16 ^ D17 ^ D18 ^ D19 ^ D20 ^ D21 ^ D22 ^ D23
 * - P32 = D24 ^ D25 ^ D26 ^ D27 ^ D28 ^ D29 ^ D30 ^ D31
 * - P64 = All data bits XOR (overall parity)
 */
static uint8_t EccEncoder_ComputeEcc7(uint32_t data)
{
    uint8_t ecc = 0U;
    uint8_t i;
    uint8_t parity;

    /* Calculate each ECC bit using the Hamming matrix (all 7 rows,
     * including the overall-parity row from the regenerated matrix) */
    for (i = 0U; i < 7U; i++) {
        parity = (EccEncoder_PopCount32(data & ecc7_matrix[i]) & 1U);
        ecc |= (parity << i);
    }

    return ecc;
}

/**
 * @brief Compute 8-bit ECC code for 64-bit data using SECDED
 */
static uint8_t EccEncoder_ComputeEcc8(uint64_t data)
{
    uint8_t ecc = 0U;
    uint8_t i;
    uint8_t parity;

    /* Calculate Hamming bits (P1-P64) */
    for (i = 0U; i < 7U; i++) {
        parity = (EccEncoder_PopCount64(data & ecc8_matrix[i]) & 1U);
        ecc |= (parity << i);
    }

    /* Calculate overall parity bit for SECDED */
    parity = (EccEncoder_PopCount64(data) & 1U);
    ecc |= (parity << 7);

    return ecc;
}

/******************************************************************************
 * Public API Implementation
 ******************************************************************************/

/**
 * @brief Initialize ECC encoder
 */
Std_ReturnType EccEncoder_Init(const EccEncoderConfigType *config)
{
    EccEncoderConfigType default_config;

    if (config == NULL) {
        /* Use defaults: 32-bit ECC mode, ASIL-D */
        memset(&default_config, 0, sizeof(default_config));
        default_config.mode = ECC_MODE_32BIT;
        default_config.asil_level = ASIL_D;
        config = &default_config;
    }

    /* Clear state structure */
    memset(&g_encoder_state, 0, sizeof(g_encoder_state));

    /* Copy configuration */
    g_encoder_state.config = *config;
    g_encoder_state.initialized = TRUE;

    g_initialized = TRUE;

    return E_OK;
}

/**
 * @brief Deinitialize ECC encoder
 */
Std_ReturnType EccEncoder_DeInit(void)
{
    memset(&g_encoder_state, 0, sizeof(g_encoder_state));
    g_initialized = FALSE;
    return E_OK;
}

/**
 * @brief Encode 32-bit data with SECDED ECC7
 */
Std_ReturnType EccEncoder_Encode32(uint32_t data, uint8_t *ecc_code)
{
    Std_ReturnType result = E_OK;

    /* Safety checks for ASIL-D */
    if (g_initialized == FALSE) {
        return E_NOT_OK;
    }

    if (ecc_code == NULL) {
        g_encoder_state.error_count++;
        return E_NOT_OK;
    }

    /* Compute ECC code */
    *ecc_code = EccEncoder_ComputeEcc7(data);

    /* Update statistics */
    g_encoder_state.encode_count++;

    return result;
}

/**
 * @brief Encode 64-bit data with SECDED ECC8
 */
Std_ReturnType EccEncoder_Encode64(uint64_t data, uint8_t *ecc_code)
{
    Std_ReturnType result = E_OK;

    /* Safety checks for ASIL-D */
    if (g_initialized == FALSE) {
        return E_NOT_OK;
    }

    if (ecc_code == NULL) {
        g_encoder_state.error_count++;
        return E_NOT_OK;
    }

    /* Compute ECC code */
    *ecc_code = EccEncoder_ComputeEcc8(data);

    /* Update statistics */
    g_encoder_state.encode_count++;

    return result;
}

/**
 * @brief Encode data with full result (32-bit)
 */
Std_ReturnType EccEncoder_EncodeFull32(uint32_t data, EccEncodedType *result)
{
    Std_ReturnType status;

    if (result == NULL) {
        return E_NOT_OK;
    }

    status = EccEncoder_Encode32(data, &result->ecc_code);
    if (status == E_OK) {
        result->data = data;
        result->data64 = 0ULL;
        result->mode = ECC_MODE_32BIT;
    }

    return status;
}

/**
 * @brief Encode data with full result (64-bit)
 */
Std_ReturnType EccEncoder_EncodeFull64(uint64_t data, EccEncodedType *result)
{
    Std_ReturnType status;

    if (result == NULL) {
        return E_NOT_OK;
    }

    status = EccEncoder_Encode64(data, &result->ecc_code);
    if (status == E_OK) {
        result->data64 = data;
        result->data = 0U;
        result->mode = ECC_MODE_64BIT;
    }

    return status;
}

/**
 * @brief Batch encode multiple 32-bit words
 */
Std_ReturnType EccEncoder_BatchEncode32(const uint32_t *data, 
                                        uint8_t *ecc_codes, 
                                        uint32_t count)
{
    uint32_t i;
    Std_ReturnType status = E_OK;

    if ((data == NULL) || (ecc_codes == NULL) || (count == 0U)) {
        return E_NOT_OK;
    }

    if (g_initialized == FALSE) {
        return E_NOT_OK;
    }

    /* Process all words */
    for (i = 0U; i < count; i++) {
        ecc_codes[i] = EccEncoder_ComputeEcc7(data[i]);
    }

    g_encoder_state.encode_count += count;

    return status;
}

/**
 * @brief Get encoder state
 */
const EccEncoderStateType* EccEncoder_GetState(void)
{
    return &g_encoder_state;
}

/**
 * @brief Get version information
 */
void EccEncoder_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = ECC_ENCODER_VENDOR_ID;
        version->moduleID = ECC_ENCODER_MODULE_ID;
        version->sw_major_version = ECC_ENCODER_SW_MAJOR;
        version->sw_minor_version = ECC_ENCODER_SW_MINOR;
        version->sw_patch_version = ECC_ENCODER_SW_PATCH;
    }
}
