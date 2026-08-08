/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : portable (any C99)
* Dependencies         : none (stdint.h only)
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Lib_Aes.h
 * @brief   Independent AES block cipher library (FIPS-197)
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Decoupled, reusable AES implementation (XMEN Libraries/ alignment).
 *   Pure C99 + stdint.h — no AUTOSAR / mbedTLS dependencies.
 *
 *   Features:
 *     - AES-128 / AES-192 / AES-256 (FIPS-197)
 *     - single-block Encrypt/Decrypt
 *     - ECB mode (len must be a multiple of 16 bytes)
 *     - CBC mode (encrypt + decrypt, in-place safe)
 *
 *   S-box / inverse S-box are generated at first Lib_AesInit (GF(2^8)
 *   multiplicative inverse + affine transform), verified by the unit
 *   tests against the NIST FIPS-197 / SP 800-38A vectors.
 *
 *   Note: this library is a software reference implementation for
 *   bootloader / secure-boot / non-Crypto-stack use cases.  The Crypto
 *   driver keeps its mbedTLS/HSM-backed implementation (GCM/CCM/CTR +
 *   hardware acceleration); see src/libs/README.md for the boundary.
 */

#ifndef LIB_AES_H
#define LIB_AES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         CONSTANTS
 *==================================================================================================*/

/** @brief AES block size in bytes */
#define LIB_AES_BLOCK_SIZE              (16u)

/** @brief Return codes */
#define LIB_AES_OK                      (0)
#define LIB_AES_ERR_PARAM               (-1)    /**< NULL pointer argument */
#define LIB_AES_ERR_KEY_LEN             (-2)    /**< Unsupported key length */
#define LIB_AES_ERR_LENGTH              (-3)    /**< Data length not a multiple of 16 */
#define LIB_AES_ERR_STATE               (-4)    /**< Context not initialized */

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief AES key length selector */
typedef enum {
    LIB_AES_KEY_128 = 0,    /**< AES-128 (10 rounds) */
    LIB_AES_KEY_192 = 1,    /**< AES-192 (12 rounds) */
    LIB_AES_KEY_256 = 2     /**< AES-256 (14 rounds) */
} Lib_AesKeyLenType;

/** @brief AES context (expanded round keys) */
typedef struct {
    uint32_t          roundKeys[60];  /**< Expanded key schedule (15 rounds max) */
    uint8_t           rounds;         /**< Number of rounds (10/12/14), 0 = uninitialized */
    Lib_AesKeyLenType keyLen;         /**< Key length selector */
} Lib_AesContextType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/

/**
 * @brief   Initialize an AES context and expand the key schedule
 * @param   ctx    [out] Context to initialize
 * @param   key    [in]  Raw key bytes (16 / 24 / 32 bytes)
 * @param   keyLen [in]  Key length selector
 * @return  LIB_AES_OK on success, LIB_AES_ERR_PARAM / LIB_AES_ERR_KEY_LEN
 */
int Lib_AesInit(Lib_AesContextType* ctx, const uint8_t* key, Lib_AesKeyLenType keyLen);

/**
 * @brief   Encrypt a single 16-byte block
 * @param   ctx [in] Initialized context
 * @param   in  [in]  16-byte plaintext block
 * @param   out [out] 16-byte ciphertext block
 * @return  LIB_AES_OK / LIB_AES_ERR_PARAM / LIB_AES_ERR_STATE
 */
int Lib_AesEncryptBlock(const Lib_AesContextType* ctx, const uint8_t in[LIB_AES_BLOCK_SIZE],
                        uint8_t out[LIB_AES_BLOCK_SIZE]);

/**
 * @brief   Decrypt a single 16-byte block
 * @param   ctx [in] Initialized context
 * @param   in  [in]  16-byte ciphertext block
 * @param   out [out] 16-byte plaintext block
 * @return  LIB_AES_OK / LIB_AES_ERR_PARAM / LIB_AES_ERR_STATE
 */
int Lib_AesDecryptBlock(const Lib_AesContextType* ctx, const uint8_t in[LIB_AES_BLOCK_SIZE],
                        uint8_t out[LIB_AES_BLOCK_SIZE]);

/**
 * @brief   Encrypt a buffer in ECB mode (len must be a multiple of 16)
 * @param   ctx [in]  Initialized context
 * @param   in  [in]  Plaintext buffer (may equal out for in-place)
 * @param   out [out] Ciphertext buffer
 * @param   len [in]  Total length in bytes (multiple of 16)
 * @return  LIB_AES_OK / LIB_AES_ERR_PARAM / LIB_AES_ERR_STATE / LIB_AES_ERR_LENGTH
 */
int Lib_AesEncryptEcb(const Lib_AesContextType* ctx, const uint8_t* in,
                      uint8_t* out, size_t len);

/**
 * @brief   Decrypt a buffer in ECB mode (len must be a multiple of 16)
 * @param   ctx [in]  Initialized context
 * @param   in  [in]  Ciphertext buffer (may equal out for in-place)
 * @param   out [out] Plaintext buffer
 * @param   len [in]  Total length in bytes (multiple of 16)
 * @return  LIB_AES_OK / LIB_AES_ERR_PARAM / LIB_AES_ERR_STATE / LIB_AES_ERR_LENGTH
 */
int Lib_AesDecryptEcb(const Lib_AesContextType* ctx, const uint8_t* in,
                      uint8_t* out, size_t len);

/**
 * @brief   Encrypt a buffer in CBC mode (len must be a multiple of 16)
 * @param   ctx [in]  Initialized context
 * @param   iv  [in]  16-byte initialization vector (may equal out region? no: read-only)
 * @param   in  [in]  Plaintext buffer (may equal out for in-place)
 * @param   out [out] Ciphertext buffer
 * @param   len [in]  Total length in bytes (multiple of 16)
 * @return  LIB_AES_OK / LIB_AES_ERR_PARAM / LIB_AES_ERR_STATE / LIB_AES_ERR_LENGTH
 */
int Lib_AesEncryptCbc(const Lib_AesContextType* ctx, const uint8_t iv[LIB_AES_BLOCK_SIZE],
                      const uint8_t* in, uint8_t* out, size_t len);

/**
 * @brief   Decrypt a buffer in CBC mode (len must be a multiple of 16)
 * @param   ctx [in]  Initialized context
 * @param   iv  [in]  16-byte initialization vector (read-only)
 * @param   in  [in]  Ciphertext buffer (may equal out for in-place)
 * @param   out [out] Plaintext buffer
 * @param   len [in]  Total length in bytes (multiple of 16)
 * @return  LIB_AES_OK / LIB_AES_ERR_PARAM / LIB_AES_ERR_STATE / LIB_AES_ERR_LENGTH
 */
int Lib_AesDecryptCbc(const Lib_AesContextType* ctx, const uint8_t iv[LIB_AES_BLOCK_SIZE],
                      const uint8_t* in, uint8_t* out, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* LIB_AES_H */
