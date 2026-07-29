/**
 * @file mbedtls_autosar.h
 * @brief AUTOSAR Cryptographic Services Adapter for Mbed TLS
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @author YuleTech AutoSAR Team
 * @version 1.0.0
 *
 * This header provides the interface between Mbed TLS and AutoSAR Crypto Stack.
 * It maps AutoSAR Crypto Services API calls to Mbed TLS implementations.
 *
 * @note Conforms to AutoSAR Classic Platform R20-11 Crypto Services specification
 */

#ifndef MBEDTLS_AUTOSAR_H
#define MBEDTLS_AUTOSAR_H

/* ============================================================================
 * Includes
 * ============================================================================ */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * AutoSAR Standard Includes (when available)
 * ============================================================================ */

/* When building with full AutoSAR stack, include actual headers */
#ifdef USE_FULL_AUTOSAR_STACK
#include "Csm.h"
#include "CryIf.h"
#include "Crypto.h"
#else
/* Minimal type definitions for standalone builds */

/* ============================================================================
 * AutoSAR Type Definitions (Minimal)
 * ============================================================================ */

/**
 * @brief Standard AutoSAR return types
 */
typedef uint8_t Std_ReturnType;
#define E_OK            ((Std_ReturnType)0)
#define E_NOT_OK        ((Std_ReturnType)1)

/**
 * @brief Crypto operation modes
 */
typedef uint8_t Crypto_OperationModeType;
#define CRYPTO_OPERATIONMODE_START      0x01U
#define CRYPTO_OPERATIONMODE_UPDATE     0x02U
#define CRYPTO_OPERATIONMODE_FINISH     0x04U
#define CRYPTO_OPERATIONMODE_STREAMSTART 0x08U

/**
 * @brief Crypto algorithm families
 */
typedef uint8_t Crypto_AlgorithmFamilyType;
#define CRYPTO_ALGOFAM_AES              0x00U
#define CRYPTO_ALGOFAM_SHA2_256         0x01U
#define CRYPTO_ALGOFAM_ECCSEC_P256      0x02U
#define CRYPTO_ALGOFAM_ECDSA            0x03U
#define CRYPTO_ALGOFAM_ECDH             0x04U
#define CRYPTO_ALGOFAM_HKDF             0x05U
#define CRYPTO_ALGOFAM_SHAKE128         0x06U
#define CRYPTO_ALGOFAM_RNG              0x07U

/**
 * @brief Crypto algorithm modes
 */
typedef uint8_t Crypto_AlgorithmModeType;
#define CRYPTO_ALGOMODE_GCM             0x00U
#define CRYPTO_ALGOMODE_CTR             0x01U
#define CRYPTO_ALGOMODE_ECB             0x02U
#define CRYPTO_ALGOMODE_CBC             0x03U
#define CRYPTO_ALGOMODE_NOT_SET         0xFFU

/**
 * @brief Key element indices for CCC operations
 */
typedef uint32_t Crypto_KeyElementIdType;
#define CRYPTO_KEY_MATERIAL             1U
#define CRYPTO_IV                       2U
#define CRYPTO_PUBLIC_KEY               3U
#define CRYPTO_PRIVATE_KEY              4U
#define CRYPTO_KEY_HASH                 5U
#define CRYPTO_KEY_SHARED_SECRET        6U
#define CRYPTO_KEY_SIGNATURE            7U

/**
 * @brief Key types
 */
typedef uint32_t Crypto_KeyIdType;

/**
 * @brief Job ID types
 */
typedef uint32_t Crypto_JobIdType;

/**
 * @brief Result types
 */
typedef uint8_t Crypto_ResultType;
#define CRYPTO_E_BUSY                   0x02U
#define CRYPTO_E_SMALL_BUFFER           0x03U
#define CRYPTO_E_ENTROPY_EXHAUSTED      0x04U
#define CRYPTO_E_KEY_READ_FAIL          0x06U
#define CRYPTO_E_KEY_WRITE_FAIL         0x07U
#define CRYPTO_E_KEY_NOT_VALID          0x08U
#define CRYPTO_E_KEY_SIZE_MISMATCH      0x0AU
#define CRYPTO_E_JOB_CANCELED           0x0CU
#define CRYPTO_E_KEY_EMPTY              0x0DU

/**
 * @brief Service information types
 */
typedef uint8_t Crypto_ServiceInfoType;
#define CRYPTO_HASH                     0x00U
#define CRYPTO_MACGENERATE              0x01U
#define CRYPTO_MACVERIFY                0x02U
#define CRYPTO_ENCRYPT                  0x03U
#define CRYPTO_DECRYPT                  0x04U
#define CRYPTO_AEADENCRYPT              0x05U
#define CRYPTO_AEADDECRYPT              0x06U
#define CRYPTO_SIGNATUREGENERATE        0x07U
#define CRYPTO_SIGNATUREVERIFY          0x08U
#define CRYPTO_RANDOMGENERATE           0x0BU
#define CRYPTO_KEYDERIVE                0x0DU
#define CRYPTO_KEYEXCHANGECALCSECRET    0x0EU

#endif /* USE_FULL_AUTOSAR_STACK */

/* ============================================================================
 * AutoSAR Configuration
 * ============================================================================ */

/**
 * @brief Maximum number of concurrent crypto jobs
 */
#define MBEDTLS_AUTOSAR_MAX_JOBS        8U

/**
 * @brief Maximum key length supported
 */
#define MBEDTLS_AUTOSAR_MAX_KEY_SIZE    128U

/**
 * @brief Maximum number of key elements per key
 */
#define MBEDTLS_AUTOSAR_MAX_KEY_ELEMENTS 8U

/**
 * @brief Priority levels for crypto jobs
 */
typedef uint8_t Crypto_ProcessingType;
#define CRYPTO_PROCESSING_ASYNC         0x00U
#define CRYPTO_PROCESSING_SYNC          0x01U

/* ============================================================================
 * Mbed TLS AutoSAR Adapter Types
 * ============================================================================ */

/**
 * @brief Extended key structure for AutoSAR integration
 */
typedef struct {
    Crypto_KeyIdType keyId;
    Crypto_KeyElementIdType elementId;
    uint8_t data[MBEDTLS_AUTOSAR_MAX_KEY_SIZE];
    uint32_t length;
    bool isValid;
    bool isPersistant;
} mbedtls_autosar_key_element_t;

/**
 * @brief AutoSAR crypto job structure (simplified)
 */
typedef struct {
    Crypto_JobIdType jobId;
    Crypto_ServiceInfoType service;
    Crypto_AlgorithmFamilyType algorithmFamily;
    Crypto_AlgorithmModeType algorithmMode;
    uint8_t *inputPtr;
    uint32_t inputLength;
    uint8_t *outputPtr;
    uint32_t *outputLengthPtr;
    uint8_t *secondaryInputPtr;
    uint32_t secondaryInputLength;
    uint8_t *secondaryOutputPtr;
    uint32_t *secondaryOutputLengthPtr;
    uint8_t *tertiaryInputPtr;
    uint32_t tertiaryInputLength;
    Crypto_OperationModeType mode;
    Crypto_ProcessingType processingType;
    bool isBusy;
} mbedtls_autosar_job_t;

/**
 * @brief Job state tracking
 */
typedef struct {
    mbedtls_autosar_job_t jobs[MBEDTLS_AUTOSAR_MAX_JOBS];
    uint8_t jobCount;
    bool initialized;
} mbedtls_autosar_job_pool_t;

/* ============================================================================
 * Function Prototypes - AutoSAR Crypto Interface
 * ============================================================================ */

/**
 * @brief Initialize AutoSAR crypto adapter
 *
 * This function initializes the AutoSAR-MbedTLS bridge layer.
 * Must be called before any other AutoSAR crypto operations.
 *
 * @return E_OK if initialization successful, E_NOT_OK otherwise
 */
Std_ReturnType Mbedtls_Autosar_Init(void);

/**
 * @brief Deinitialize AutoSAR crypto adapter
 *
 * @return E_OK if deinitialization successful, E_NOT_OK otherwise
 */
Std_ReturnType Mbedtls_Autosar_Deinit(void);

/**
 * @brief Process a crypto job according to AutoSAR specification
 *
 * @param[in] job Pointer to the job structure
 * @return E_OK if job processed successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_ProcessJob(mbedtls_autosar_job_t *job);

/**
 * @brief Cancel a pending crypto job
 *
 * @param[in] jobId ID of the job to cancel
 * @return E_OK if job cancelled, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_CancelJob(Crypto_JobIdType jobId);

/**
 * @brief Set a key element value
 *
 * @param[in] keyId Key identifier
 * @param[in] elementId Element identifier within the key
 * @param[in] data Pointer to the data
 * @param[in] length Data length
 * @return E_OK if key element set successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_KeyElementSet(
    Crypto_KeyIdType keyId,
    Crypto_KeyElementIdType elementId,
    const uint8_t *data,
    uint32_t length);

/**
 * @brief Get a key element value
 *
 * @param[in] keyId Key identifier
 * @param[in] elementId Element identifier within the key
 * @param[out] data Pointer to store the data
 * @param[in,out] length Pointer to data length (in/out)
 * @return E_OK if key element retrieved successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_KeyElementGet(
    Crypto_KeyIdType keyId,
    Crypto_KeyElementIdType elementId,
    uint8_t *data,
    uint32_t *length);

/**
 * @brief Copy key material from one key to another
 *
 * @param[in] keyId Source key identifier
 * @param[in] targetKeyId Target key identifier
 * @return E_OK if key copied successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_KeyCopy(
    Crypto_KeyIdType keyId,
    Crypto_KeyIdType targetKeyId);

/**
 * @brief Derive a key using HKDF
 *
 * This implements the AutoSAR KeyDerive service using Mbed TLS HKDF.
 *
 * @param[in] keyId Source key identifier (contains IKM)
 * @param[in] targetKeyId Target key identifier for derived key
 * @param[in] saltPtr Pointer to salt (optional)
 * @param[in] saltLength Salt length
 * @param[in] infoPtr Pointer to info/context
 * @param[in] infoLength Info length
 * @return E_OK if key derived successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_KeyDerive(
    Crypto_KeyIdType keyId,
    Crypto_KeyIdType targetKeyId,
    const uint8_t *saltPtr,
    uint32_t saltLength,
    const uint8_t *infoPtr,
    uint32_t infoLength);

/**
 * @brief Exchange keys using ECDH
 *
 * This implements the AutoSAR KeyExchangeCalcSecret service using Mbed TLS ECDH.
 *
 * @param[in] keyId Local private key identifier
 * @param[in] peerPublicKeyPtr Pointer to peer's public key
 * @param[in] peerPublicKeyLength Peer public key length
 * @param[out] sharedSecretPtr Pointer to store shared secret
 * @param[in,out] sharedSecretLengthPtr Pointer to shared secret length (in/out)
 * @return E_OK if shared secret computed successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_KeyExchangeCalcSecret(
    Crypto_KeyIdType keyId,
    const uint8_t *peerPublicKeyPtr,
    uint32_t peerPublicKeyLength,
    uint8_t *sharedSecretPtr,
    uint32_t *sharedSecretLengthPtr);

/**
 * @brief Generate random bytes
 *
 * This implements the AutoSAR RandomGenerate service using hardware TRNG.
 *
 * @param[out] resultPtr Pointer to store random bytes
 * @param[in] resultLength Number of random bytes to generate
 * @return E_OK if random bytes generated successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_RandomGenerate(
    uint8_t *resultPtr,
    uint32_t resultLength);

/**
 * @brief Encrypt data using AEAD (AES-GCM)
 *
 * This implements the AutoSAR AEADEncrypt service.
 *
 * @param[in] keyId Key identifier (contains AES key)
 * @param[in] plaintextPtr Pointer to plaintext
 * @param[in] plaintextLength Plaintext length
 * @param[in] associatedDataPtr Pointer to associated data (AAD)
 * @param[in] associatedDataLength AAD length
 * @param[out] ciphertextPtr Pointer to store ciphertext
 * @param[in,out] ciphertextLengthPtr Pointer to ciphertext length (in/out)
 * @param[out] tagPtr Pointer to store authentication tag
 * @param[in,out] tagLengthPtr Pointer to tag length (in/out)
 * @return E_OK if encryption successful, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_AEADEncrypt(
    Crypto_KeyIdType keyId,
    const uint8_t *plaintextPtr,
    uint32_t plaintextLength,
    const uint8_t *associatedDataPtr,
    uint32_t associatedDataLength,
    uint8_t *ciphertextPtr,
    uint32_t *ciphertextLengthPtr,
    uint8_t *tagPtr,
    uint32_t *tagLengthPtr);

/**
 * @brief Decrypt data using AEAD (AES-GCM)
 *
 * This implements the AutoSAR AEADDecrypt service.
 *
 * @param[in] keyId Key identifier (contains AES key)
 * @param[in] ciphertextPtr Pointer to ciphertext
 * @param[in] ciphertextLength Ciphertext length
 * @param[in] associatedDataPtr Pointer to associated data (AAD)
 * @param[in] associatedDataLength AAD length
 * @param[in] tagPtr Pointer to authentication tag
 * @param[in] tagLength Tag length
 * @param[out] plaintextPtr Pointer to store plaintext
 * @param[in,out] plaintextLengthPtr Pointer to plaintext length (in/out)
 * @param[out] verifyPtr Pointer to verification result
 * @return E_OK if decryption successful, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_AEADDecrypt(
    Crypto_KeyIdType keyId,
    const uint8_t *ciphertextPtr,
    uint32_t ciphertextLength,
    const uint8_t *associatedDataPtr,
    uint32_t associatedDataLength,
    const uint8_t *tagPtr,
    uint32_t tagLength,
    uint8_t *plaintextPtr,
    uint32_t *plaintextLengthPtr,
    Crypto_VerifyResultType *verifyPtr);

/**
 * @brief Generate ECDSA signature
 *
 * This implements the AutoSAR SignatureGenerate service.
 *
 * @param[in] keyId Private key identifier
 * @param[in] mode Operation mode (START/UPDATE/FINISH)
 * @param[in] dataPtr Pointer to data to sign
 * @param[in] dataLength Data length
 * @param[out] signaturePtr Pointer to store signature
 * @param[in,out] signatureLengthPtr Pointer to signature length (in/out)
 * @return E_OK if signature generated successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_SignatureGenerate(
    Crypto_KeyIdType keyId,
    Crypto_OperationModeType mode,
    const uint8_t *dataPtr,
    uint32_t dataLength,
    uint8_t *signaturePtr,
    uint32_t *signatureLengthPtr);

/**
 * @brief Verify ECDSA signature
 *
 * This implements the AutoSAR SignatureVerify service.
 *
 * @param[in] keyId Public key identifier
 * @param[in] mode Operation mode (START/UPDATE/FINISH)
 * @param[in] dataPtr Pointer to signed data
 * @param[in] dataLength Data length
 * @param[in] signaturePtr Pointer to signature
 * @param[in] signatureLength Signature length
 * @param[out] verifyPtr Pointer to verification result
 * @return E_OK if signature verified successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_SignatureVerify(
    Crypto_KeyIdType keyId,
    Crypto_OperationModeType mode,
    const uint8_t *dataPtr,
    uint32_t dataLength,
    const uint8_t *signaturePtr,
    uint32_t signatureLength,
    Crypto_VerifyResultType *verifyPtr);

/**
 * @brief Compute hash (SHA-256)
 *
 * This implements the AutoSAR Hash service.
 *
 * @param[in] mode Operation mode (START/UPDATE/FINISH)
 * @param[in] dataPtr Pointer to data to hash
 * @param[in] dataLength Data length
 * @param[out] resultPtr Pointer to store hash result
 * @param[in,out] resultLengthPtr Pointer to result length (in/out)
 * @return E_OK if hash computed successfully, error code otherwise
 */
Std_ReturnType Mbedtls_Autosar_Hash(
    Crypto_OperationModeType mode,
    const uint8_t *dataPtr,
    uint32_t dataLength,
    uint8_t *resultPtr,
    uint32_t *resultLengthPtr);

/* ============================================================================
 * Verification Result Type (when not using full stack)
 * ============================================================================ */

#ifndef USE_FULL_AUTOSAR_STACK
typedef uint8_t Crypto_VerifyResultType;
#define CRYPTO_E_VER_OK                 0x00U
#define CRYPTO_E_VER_NOT_OK             0x01U
#endif

/* ============================================================================
 * End of Header
 * ============================================================================ */

#endif /* MBEDTLS_AUTOSAR_H */
