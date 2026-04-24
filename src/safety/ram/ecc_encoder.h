/******************************************************************************
 * @file    ecc_encoder.h
 * @brief   ECC Encoder - SECDED (Single Error Correction, Double Error Detection)
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * SECDED Algorithm Implementation:
 * - 32-bit data: ECC7 (7-bit ECC code)
 * - 64-bit data: ECC8 (8-bit ECC code)
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef ECC_ENCODER_H
#define ECC_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* ECC code lengths */
#define ECC7_CODE_BITS      7U      /* For 32-bit data */
#define ECC8_CODE_BITS      8U      /* For 64-bit data */

/* Maximum data lengths */
#define ECC_MAX_DATA_BITS_32    32U
#define ECC_MAX_DATA_BITS_64    64U

/* Hamming matrix sizes */
#define ECC_H_MATRIX_ROWS_32    7U
#define ECC_H_MATRIX_COLS_32    39U  /* 32 data + 7 ECC bits */
#define ECC_H_MATRIX_ROWS_64    8U
#define ECC_H_MATRIX_COLS_64    72U  /* 64 data + 8 ECC bits */

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* ECC encoding mode */
typedef enum {
    ECC_MODE_32BIT = 0,     /* 32-bit data with ECC7 */
    ECC_MODE_64BIT,         /* 64-bit data with ECC8 */
    ECC_MODE_HARDWARE       /* Hardware ECC (if available) */
} EccEncodeModeType;

/* ECC encoding result */
typedef struct {
    uint32_t data;          /* Original data (32-bit) */
    uint64_t data64;        /* Original data (64-bit) */
    uint8_t ecc_code;       /* Computed ECC code */
    EccEncodeModeType mode; /* Encoding mode used */
} EccEncodedType;

/* ECC encoder configuration */
typedef struct {
    EccEncodeModeType mode;
    boolean use_hardware;   /* TRUE: use hardware ECC if available */
    uint8_t asil_level;     /* ASIL level for safety checks */
} EccEncoderConfigType;

/* ECC encoder state */
typedef struct {
    boolean initialized;
    uint32_t encode_count;
    uint32_t error_count;
    EccEncoderConfigType config;
} EccEncoderStateType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize ECC encoder
 * @param config Pointer to encoder configuration
 * @return E_OK on success, E_NOT_OK on error
 */
Std_ReturnType EccEncoder_Init(const EccEncoderConfigType *config);

/**
 * @brief Deinitialize ECC encoder
 * @return E_OK on success
 */
Std_ReturnType EccEncoder_DeInit(void);

/**
 * @brief Encode 32-bit data with SECDED ECC7
 * @param data 32-bit input data
 * @param ecc_code Pointer to store computed 7-bit ECC code
 * @return E_OK on success
 */
Std_ReturnType EccEncoder_Encode32(uint32_t data, uint8_t *ecc_code);

/**
 * @brief Encode 64-bit data with SECDED ECC8
 * @param data 64-bit input data
 * @param ecc_code Pointer to store computed 8-bit ECC code
 * @return E_OK on success
 */
Std_ReturnType EccEncoder_Encode64(uint64_t data, uint8_t *ecc_code);

/**
 * @brief Encode data with full result
 * @param data 32-bit input data
 * @param result Pointer to store encoding result
 * @return E_OK on success
 */
Std_ReturnType EccEncoder_EncodeFull32(uint32_t data, EccEncodedType *result);

/**
 * @brief Encode 64-bit data with full result
 * @param data 64-bit input data
 * @param result Pointer to store encoding result
 * @return E_OK on success
 */
Std_ReturnType EccEncoder_EncodeFull64(uint64_t data, EccEncodedType *result);

/**
 * @brief Batch encode multiple 32-bit words
 * @param data Array of input data
 * @param ecc_codes Array to store ECC codes
 * @param count Number of words to encode
 * @return E_OK on success
 */
Std_ReturnType EccEncoder_BatchEncode32(const uint32_t *data, 
                                        uint8_t *ecc_codes, 
                                        uint32_t count);

/**
 * @brief Get encoder state
 * @return Pointer to encoder state
 */
const EccEncoderStateType* EccEncoder_GetState(void);

/**
 * @brief Get version information
 * @param version Pointer to version info structure
 */
void EccEncoder_GetVersionInfo(Std_VersionInfoType *version);

/******************************************************************************
 * Inline Helper Functions
 ******************************************************************************/

/**
 * @brief Calculate parity of a value
 * @param value Input value
 * @return Parity bit (0 or 1)
 */
static inline uint8_t EccEncoder_CalculateParity(uint32_t value)
{
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;
    return (uint8_t)(value & 1U);
}

/**
 * @brief Calculate parity of 64-bit value
 * @param value 64-bit input value
 * @return Parity bit (0 or 1)
 */
static inline uint8_t EccEncoder_CalculateParity64(uint64_t value)
{
    value ^= value >> 32;
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;
    return (uint8_t)(value & 1ULL);
}

#ifdef __cplusplus
}
#endif

#endif /* ECC_ENCODER_H */
