/**
 * @file mbedtls_wrapper.h
 * @brief Mbed TLS Wrapper Interface for YuleTech AutoSAR
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @author YuleTech AutoSAR Team
 * @version 1.0.0
 *
 * This wrapper provides a simplified API layer over Mbed TLS for:
 * - CCC (Car Connectivity Consortium) Digital Key operations
 * - UWB/BLE secure ranging and authentication
 * - AutoSAR Crypto Services integration
 */

#ifndef MBEDTLS_WRAPPER_H
#define MBEDTLS_WRAPPER_H

/* ============================================================================
 * Includes
 * ============================================================================ */

#include "mbedtls_autosar.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Version Information
 * ============================================================================ */

#define MBEDTLS_WRAPPER_VERSION_MAJOR   1
#define MBEDTLS_WRAPPER_VERSION_MINOR   0
#define MBEDTLS_WRAPPER_VERSION_PATCH   0

/* ============================================================================
 * Configuration Constants
 * ============================================================================ */

#define MBEDTLS_WRAPPER_OK              0
#define MBEDTLS_WRAPPER_ERROR           (-1)
#define MBEDTLS_WRAPPER_INVALID_PARAM   (-2)
#define MBEDTLS_WRAPPER_NO_MEMORY       (-3)
#define MBEDTLS_WRAPPER_CRYPTO_ERROR    (-4)

#define MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE   32
#define MBEDTLS_WRAPPER_ECC_P256_SIG_SIZE   64
#define MBEDTLS_WRAPPER_AES_128_KEY_SIZE    16
#define MBEDTLS_WRAPPER_AES_GCM_IV_SIZE     12
#define MBEDTLS_WRAPPER_AES_GCM_TAG_SIZE    16
#define MBEDTLS_WRAPPER_SHA256_SIZE         32
#define MBEDTLS_WRAPPER_HKDF_MAX_SIZE       64

#define MBEDTLS_WRAPPER_MAX_KEY_MATERIAL_SIZE   128
#define MBEDTLS_WRAPPER_MAX_AAD_SIZE            256
#define MBEDTLS_WRAPPER_MAX_PLAINTEXT_SIZE      1024

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

typedef int32_t mbedtls_wrapper_status_t;

/**
 * @brief Key types supported by the wrapper
 */
typedef enum {
    MBEDTLS_WRAPPER_KEY_TYPE_ECDH_P256 = 0,
    MBEDTLS_WRAPPER_KEY_TYPE_ECDSA_P256,
    MBEDTLS_WRAPPER_KEY_TYPE_AES_128_GCM,
    MBEDTLS_WRAPPER_KEY_TYPE_HKDF,
    MBEDTLS_WRAPPER_KEY_TYPE_UNKNOWN
} mbedtls_wrapper_key_type_t;

/**
 * @brief Operation modes for AES-GCM
 */
typedef enum {
    MBEDTLS_WRAPPER_AES_GCM_ENCRYPT = 0,
    MBEDTLS_WRAPPER_AES_GCM_DECRYPT
} mbedtls_wrapper_aes_gcm_mode_t;

/**
 * @brief ECC key structure
 */
typedef struct {
    uint8_t private_key[MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE];
    uint8_t public_key[MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE * 2 + 1];  /* Uncompressed format */
    bool has_private;
    bool has_public;
} mbedtls_wrapper_ecc_key_t;

/**
 * @brief AES-GCM context
 */
typedef struct {
    uint8_t key[MBEDTLS_WRAPPER_AES_128_KEY_SIZE];
    uint8_t iv[MBEDTLS_WRAPPER_AES_GCM_IV_SIZE];
    uint32_t key_len;
    uint32_t iv_len;
    mbedtls_wrapper_aes_gcm_mode_t mode;
} mbedtls_wrapper_aes_gcm_ctx_t;

/**
 * @brief ECDSA signature structure
 */
typedef struct {
    uint8_t r[MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE];
    uint8_t s[MBEDTLS_WRAPPER_ECC_P256_KEY_SIZE];
} mbedtls_wrapper_ecdsa_sig_t;

/**
 * @brief HKDF context
 */
typedef struct {
    uint8_t ikm[MBEDTLS_WRAPPER_MAX_KEY_MATERIAL_SIZE];
    uint32_t ikm_len;
    uint8_t salt[MBEDTLS_WRAPPER_MAX_KEY_MATERIAL_SIZE];
    uint32_t salt_len;
    uint8_t info[MBEDTLS_WRAPPER_MAX_KEY_MATERIAL_SIZE];
    uint32_t info_len;
} mbedtls_wrapper_hkdf_ctx_t;

/* ============================================================================
 * Function Prototypes - Initialization
 * ============================================================================ */

/**
 * @brief Initialize the Mbed TLS wrapper
 *
 * This function must be called before any other wrapper functions.
 * It initializes the Mbed TLS library, entropy pool, and DRBG.
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_init(void);

/**
 * @brief Deinitialize the Mbed TLS wrapper
 *
 * Clears all sensitive data and releases resources.
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_deinit(void);

/**
 * @brief Get wrapper version information
 *
 * @param[out] major Major version number
 * @param[out] minor Minor version number
 * @param[out] patch Patch version number
 */
void mbedtls_wrapper_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

/* ============================================================================
 * Function Prototypes - Random Number Generation
 * ============================================================================ */

/**
 * @brief Generate random bytes using hardware TRNG
 *
 * @param[out] output Buffer to store random bytes
 * @param[in] len Number of bytes to generate
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_random(uint8_t *output, uint32_t len);

/* ============================================================================
 * Function Prototypes - ECC P-256 Operations
 * ============================================================================ */

/**
 * @brief Generate an ECC P-256 key pair
 *
 * @param[out] key Structure to store the generated key pair
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecc_p256_gen_keypair(mbedtls_wrapper_ecc_key_t *key);

/**
 * @brief Load an ECC P-256 private key
 *
 * @param[out] key Structure to store the key
 * @param[in] private_key 32-byte private key
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecc_p256_load_private_key(
    mbedtls_wrapper_ecc_key_t *key,
    const uint8_t *private_key);

/**
 * @brief Load an ECC P-256 public key
 *
 * @param[out] key Structure to store the key
 * @param[in] public_key 65-byte uncompressed public key (0x04 || X || Y)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecc_p256_load_public_key(
    mbedtls_wrapper_ecc_key_t *key,
    const uint8_t *public_key);

/**
 * @brief Clear an ECC key structure (secure erase)
 *
 * @param[in,out] key Key structure to clear
 */
void mbedtls_wrapper_ecc_p256_clear_key(mbedtls_wrapper_ecc_key_t *key);

/* ============================================================================
 * Function Prototypes - ECDH P-256 Key Agreement
 * ============================================================================ */

/**
 * @brief Perform ECDH key agreement (P-256)
 *
 * Computes shared_secret = private_key * peer_public_key
 *
 * @param[in] private_key Local private key
 * @param[in] peer_public_key Peer public key (65 bytes uncompressed)
 * @param[out] shared_secret Output buffer for shared secret (32 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecdh_p256_compute_shared_secret(
    const mbedtls_wrapper_ecc_key_t *private_key,
    const uint8_t *peer_public_key,
    uint8_t *shared_secret);

/**
 * @brief Perform ECDH key agreement with raw keys
 *
 * @param[in] private_key 32-byte private key
 * @param[in] peer_public_key 65-byte peer public key
 * @param[out] shared_secret 32-byte shared secret output
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecdh_p256_compute_shared_secret_raw(
    const uint8_t *private_key,
    const uint8_t *peer_public_key,
    uint8_t *shared_secret);

/* ============================================================================
 * Function Prototypes - ECDSA P-256 Digital Signature
 * ============================================================================ */

/**
 * @brief Sign a message using ECDSA with P-256
 *
 * The message is hashed internally using SHA-256.
 *
 * @param[in] private_key Signer's private key
 * @param[in] message Message to sign
 * @param[in] message_len Message length in bytes
 * @param[out] signature Output signature (R || S, 64 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_sign(
    const mbedtls_wrapper_ecc_key_t *private_key,
    const uint8_t *message,
    uint32_t message_len,
    uint8_t *signature);

/**
 * @brief Verify an ECDSA P-256 signature
 *
 * @param[in] public_key Signer's public key (65 bytes uncompressed)
 * @param[in] message Message that was signed
 * @param[in] message_len Message length
 * @param[in] signature Signature to verify (R || S, 64 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK if signature is valid, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_verify(
    const uint8_t *public_key,
    const uint8_t *message,
    uint32_t message_len,
    const uint8_t *signature);

/**
 * @brief Sign a pre-hashed message using ECDSA with P-256
 *
 * @param[in] private_key Signer's private key
 * @param[in] hash SHA-256 hash of the message
 * @param[out] signature Output signature (R || S, 64 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_sign_hash(
    const mbedtls_wrapper_ecc_key_t *private_key,
    const uint8_t *hash,
    uint8_t *signature);

/**
 * @brief Verify an ECDSA P-256 signature on a pre-hashed message
 *
 * @param[in] public_key Signer's public key
 * @param[in] hash SHA-256 hash of the message
 * @param[in] signature Signature to verify
 *
 * @return MBEDTLS_WRAPPER_OK if signature is valid, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_ecdsa_p256_verify_hash(
    const uint8_t *public_key,
    const uint8_t *hash,
    const uint8_t *signature);

/* ============================================================================
 * Function Prototypes - AES-128-GCM Encryption/Decryption
 * ============================================================================ */

/**
 * @brief Initialize AES-GCM context
 *
 * @param[out] ctx Context to initialize
 * @param[in] key 128-bit AES key
 * @param[in] iv Initialization vector (typically 12 bytes)
 * @param[in] iv_len IV length in bytes
 * @param[in] mode Operation mode (encrypt/decrypt)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_aes_gcm_init(
    mbedtls_wrapper_aes_gcm_ctx_t *ctx,
    const uint8_t *key,
    const uint8_t *iv,
    uint32_t iv_len,
    mbedtls_wrapper_aes_gcm_mode_t mode);

/**
 * @brief Encrypt data using AES-128-GCM
 *
 * @param[in] ctx Initialized AES-GCM context
 * @param[in] aad Additional authenticated data (can be NULL)
 * @param[in] aad_len AAD length
 * @param[in] plaintext Data to encrypt
 * @param[in] plaintext_len Plaintext length
 * @param[out] ciphertext Encrypted output
 * @param[out] tag Authentication tag (16 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_aes_gcm_encrypt(
    const mbedtls_wrapper_aes_gcm_ctx_t *ctx,
    const uint8_t *aad,
    uint32_t aad_len,
    const uint8_t *plaintext,
    uint32_t plaintext_len,
    uint8_t *ciphertext,
    uint8_t *tag);

/**
 * @brief Decrypt data using AES-128-GCM
 *
 * @param[in] ctx Initialized AES-GCM context
 * @param[in] aad Additional authenticated data (can be NULL)
 * @param[in] aad_len AAD length
 * @param[in] ciphertext Data to decrypt
 * @param[in] ciphertext_len Ciphertext length
 * @param[in] tag Authentication tag (16 bytes)
 * @param[out] plaintext Decrypted output
 *
 * @return MBEDTLS_WRAPPER_OK on success, MBEDTLS_WRAPPER_ERROR if tag verification fails
 */
mbedtls_wrapper_status_t mbedtls_wrapper_aes_gcm_decrypt(
    const mbedtls_wrapper_aes_gcm_ctx_t *ctx,
    const uint8_t *aad,
    uint32_t aad_len,
    const uint8_t *ciphertext,
    uint32_t ciphertext_len,
    const uint8_t *tag,
    uint8_t *plaintext);

/**
 * @brief Clear AES-GCM context (secure erase)
 *
 * @param[in,out] ctx Context to clear
 */
void mbedtls_wrapper_aes_gcm_clear(mbedtls_wrapper_aes_gcm_ctx_t *ctx);

/* ============================================================================
 * Function Prototypes - HKDF Key Derivation
 * ============================================================================ */

/**
 * @brief Derive keys using HKDF-SHA256
 *
 * @param[in] ikm Input keying material
 * @param[in] ikm_len IKM length
 * @param[in] salt Salt (optional, can be NULL)
 * @param[in] salt_len Salt length
 * @param[in] info Context and application specific information
 * @param[in] info_len Info length
 * @param[out] okm Output keying material
 * @param[in] okm_len Desired OKM length
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_hkdf_derive(
    const uint8_t *ikm,
    uint32_t ikm_len,
    const uint8_t *salt,
    uint32_t salt_len,
    const uint8_t *info,
    uint32_t info_len,
    uint8_t *okm,
    uint32_t okm_len);

/**
 * @brief HKDF Extract step
 *
 * @param[in] salt Salt value
 * @param[in] salt_len Salt length
 * @param[in] ikm Input keying material
 * @param[in] ikm_len IKM length
 * @param[out] prk Pseudorandom key output (32 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_hkdf_extract(
    const uint8_t *salt,
    uint32_t salt_len,
    const uint8_t *ikm,
    uint32_t ikm_len,
    uint8_t *prk);

/**
 * @brief HKDF Expand step
 *
 * @param[in] prk Pseudorandom key from extract step
 * @param[in] prk_len PRK length
 * @param[in] info Context and application specific information
 * @param[in] info_len Info length
 * @param[out] okm Output keying material
 * @param[in] okm_len Desired OKM length
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_hkdf_expand(
    const uint8_t *prk,
    uint32_t prk_len,
    const uint8_t *info,
    uint32_t info_len,
    uint8_t *okm,
    uint32_t okm_len);

/* ============================================================================
 * Function Prototypes - SHA-256 Hashing
 * ============================================================================ */

/**
 * @brief Compute SHA-256 hash
 *
 * @param[in] input Input data
 * @param[in] input_len Input length
 * @param[out] hash Output hash (32 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_sha256(
    const uint8_t *input,
    uint32_t input_len,
    uint8_t *hash);

/**
 * @brief Initialize SHA-256 context for incremental hashing
 *
 * @param[out] ctx Context pointer (implementation specific)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_sha256_init(void **ctx);

/**
 * @brief Update SHA-256 hash with more data
 *
 * @param[in] ctx Context from init
 * @param[in] input Input data
 * @param[in] input_len Input length
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_sha256_update(
    void *ctx,
    const uint8_t *input,
    uint32_t input_len);

/**
 * @brief Finalize SHA-256 hash computation
 *
 * @param[in] ctx Context from init
 * @param[out] hash Output hash (32 bytes)
 *
 * @return MBEDTLS_WRAPPER_OK on success, error code otherwise
 */
mbedtls_wrapper_status_t mbedtls_wrapper_sha256_final(void *ctx, uint8_t *hash);

/* ============================================================================
 * End of Header
 * ============================================================================ */

#endif /* MBEDTLS_WRAPPER_H */
