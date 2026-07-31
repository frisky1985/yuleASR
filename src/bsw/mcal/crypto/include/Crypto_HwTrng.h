/**********************************************************************************************************************
 * @file       Crypto_HwTrng.h
 * @brief      Hardware True Random Number Generator (TRNG) Interface
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Hardware True Random Number Generator (TRNG) interface for S32K312.
 *      Provides cryptographically secure random number generation using
 *      hardware entropy source.
 *
 * @hardware_reference
 *      NXP S32K3xx Reference Manual - TRNG Module
 *********************************************************************************************************************/

#ifndef CRYPTO_HWTRNG_H
#define CRYPTO_HWTRNG_H

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Std_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS
 *********************************************************************************************************************/
/* TRNG Module Version */
#define CRYPTO_HWTRNG_SW_MAJOR_VERSION      (1U)
#define CRYPTO_HWTRNG_SW_MINOR_VERSION      (0U)
#define CRYPTO_HWTRNG_SW_PATCH_VERSION      (0U)

/* TRNG Service IDs */
#define CRYPTO_HWTRNG_SID_INIT              (0x00U)
#define CRYPTO_HWTRNG_SID_DEINIT            (0x01U)
#define CRYPTO_HWTRNG_SID_SELFTEST          (0x02U)
#define CRYPTO_HWTRNG_SID_GENERATE          (0x10U)
#define CRYPTO_HWTRNG_SID_GETENTROPY        (0x11U)
#define CRYPTO_HWTRNG_SID_RESEED            (0x12U)

/* TRNG Error Codes */
#define CRYPTO_HWTRNG_E_NO_ERROR            (0x00U)
#define CRYPTO_HWTRNG_E_NOT_INITIALIZED     (0x01U)
#define CRYPTO_HWTRNG_E_BUSY                (0x02U)
#define CRYPTO_HWTRNG_E_TIMEOUT             (0x03U)
#define CRYPTO_HWTRNG_E_ENTROPY_EXHAUSTED   (0x04U)
#define CRYPTO_HWTRNG_E_INVALID_PARAM       (0x05U)
#define CRYPTO_HWTRNG_E_SELFTEST_FAILED     (0x06U)
#define CRYPTO_HWTRNG_E_HARDWARE_ERROR      (0x07U)

/* TRNG Configuration */
#define CRYPTO_HWTRNG_MAX_REQUEST_SIZE      (256U)
#define CRYPTO_HWTRNG_MIN_REQUEST_SIZE      (1U)
#define CRYPTO_HWTRNG_ENTROPY_BITS          (128U)
#define CRYPTO_HWTRNG_SEED_SIZE             (32U)

/* TRNG Timeout Configuration */
#define CRYPTO_HWTRNG_TIMEOUT_DEFAULT       (10000U)    /* 10ms */
#define CRYPTO_HWTRNG_TIMEOUT_SELFTEST      (50000U)    /* 50ms */

/**********************************************************************************************************************
 * GLOBAL DATA TYPES
 *********************************************************************************************************************/

/* TRNG State */
typedef enum {
    CRYPTO_HWTRNG_STATE_UNINIT = 0,
    CRYPTO_HWTRNG_STATE_INIT,
    CRYPTO_HWTRNG_STATE_READY,
    CRYPTO_HWTRNG_STATE_BUSY,
    CRYPTO_HWTRNG_STATE_ERROR
} Crypto_HwTrngStateType;

/* TRNG Entropy Quality */
typedef enum {
    CRYPTO_HWTRNG_ENTROPY_LOW = 0,
    CRYPTO_HWTRNG_ENTROPY_MEDIUM,
    CRYPTO_HWTRNG_ENTROPY_HIGH
} Crypto_HwTrngEntropyType;

/* TRNG Configuration Structure */
typedef struct {
    boolean enableHealthTests;
    boolean enableConditioning;
    uint32  sampleCount;
    uint32  timeoutUs;
} Crypto_HwTrngConfigType;

/* TRNG Status Structure */
typedef struct {
    Crypto_HwTrngStateType  state;
    Crypto_HwTrngEntropyType entropyLevel;
    uint32                  generatedBytes;
    uint32                  errorCount;
    boolean                 healthTestPassed;
    boolean                 entropyValid;
} Crypto_HwTrngStatusType;

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/

/**
 * @brief Initializes the hardware TRNG module
 * @param config Pointer to TRNG configuration structure
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_Init(const Crypto_HwTrngConfigType* config);

/**
 * @brief Deinitializes the hardware TRNG module
 */
void Crypto_HwTrng_DeInit(void);

/**
 * @brief Performs TRNG self-test
 * @return E_OK if all tests passed, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_SelfTest(void);

/**
 * @brief Gets current TRNG status
 * @param status Pointer to status structure to fill
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_GetStatus(Crypto_HwTrngStatusType* status);

/**
 * @brief Generates random data using hardware TRNG
 * @param output Pointer to output buffer
 * @param length Number of random bytes to generate (1-256)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_Generate(uint8* output, uint32 length);

/**
 * @brief Generates random data with blocking wait
 * @param output Pointer to output buffer
 * @param length Number of random bytes to generate
 * @param timeoutUs Timeout in microseconds
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_GenerateBlocking(uint8* output, 
                                               uint32 length, 
                                               uint32 timeoutUs);

/**
 * @brief Gets current entropy estimate from TRNG
 * @param entropyBits Pointer to store entropy estimate in bits
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_GetEntropyEstimate(uint32* entropyBits);

/**
 * @brief Performs manual reseed of TRNG entropy pool
 * @param seedData Pointer to additional seed data (optional)
 * @param seedLength Seed data length (0 or 32 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Crypto_HwTrng_Reseed(const uint8* seedData, uint32 seedLength);

/**
 * @brief Checks if TRNG entropy is available
 * @return TRUE if entropy is available, FALSE otherwise
 */
boolean Crypto_HwTrng_IsEntropyAvailable(void);

/**
 * @brief Waits for TRNG to be ready
 * @param timeoutUs Timeout in microseconds
 * @return E_OK if ready, E_NOT_OK on timeout
 */
Std_ReturnType Crypto_HwTrng_WaitReady(uint32 timeoutUs);

#endif /* CRYPTO_HWTRNG_H */
