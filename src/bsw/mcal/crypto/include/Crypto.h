/**********************************************************************************************************************
 * @file       Crypto.h
 * @brief      Crypto Driver Main Header
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      This file contains the public API for the Crypto Driver.
 *      Supports both software (Mbed TLS) and hardware (HSM) implementations.
 *      Designed for CCC Digital Key compliance.
 *********************************************************************************************************************/

#ifndef CRYPTO_H
#define CRYPTO_H

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_Types.h"
#include "Crypto_Cfg.h"
#include "blake2.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS
 *********************************************************************************************************************/
#define CRYPTO_VENDOR_ID                    (0x2025U)
#define CRYPTO_MODULE_ID                    (110U)

#define CRYPTO_SW_MAJOR_VERSION             (1U)
#define CRYPTO_SW_MINOR_VERSION             (0U)
#define CRYPTO_SW_PATCH_VERSION             (0U)

/* Service IDs for Error Reporting */
#define CRYPTO_SID_INIT                     (0x00U)
#define CRYPTO_SID_DEINIT                   (0x01U)
#define CRYPTO_SID_PROCESSJOB               (0x03U)
#define CRYPTO_SID_CANCELJOB                (0x0EU)
#define CRYPTO_SID_KEYELEMENTSET            (0x11U)
#define CRYPTO_SID_KEYVALIDSET              (0x13U)
#define CRYPTO_SID_KEYELEMENTGET            (0x14U)
#define CRYPTO_SID_KEYEXCHSYNCCALCSECRET    (0x15U)
#define CRYPTO_SID_KEYDERIVE                (0x16U)
#define CRYPTO_SID_RANDOMGENERATE           (0x18U)
#define CRYPTO_SID_KEYGENERATE              (0x19U)
#define CRYPTO_SID_KEYELEMENTSCOPY          (0x1BU)
#define CRYPTO_SID_KEYCOPY                  (0x1CU)
#define CRYPTO_SID_KEYELEMENTIDSGET         (0x21U)
#define CRYPTO_SID_KEYELEMENTMOVES          (0x22U)
#define CRYPTO_SID_KEYELEMENTCLEAR          (0x23U)

#define CRYPTO_SID_MBEDTLS_INIT             (0x80U)
#define CRYPTO_SID_HSM_INIT                 (0x81U)
#define CRYPTO_SID_HSM_PROCESS              (0x82U)

/**********************************************************************************************************************
 * GLOBAL FUNCTION MACROS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * GLOBAL DATA TYPES AND STRUCTURES
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * STANDARD AUTOSAR API
 *********************************************************************************************************************/

/**
 * @brief Initializes the Crypto Driver
 * @param configPtr Pointer to configuration structure
 */
void Crypto_Init(const Crypto_ConfigType* configPtr);

/**
 * @brief Deinitializes the Crypto Driver
 */
void Crypto_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
#if (CRYPTO_CFG_VERSION_INFO_API == STD_ON)
void Crypto_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Processes a crypto job
 * @param objectId The driver object ID
 * @param job Pointer to the job structure
 * @return Result of the operation
 */
Std_ReturnType Crypto_ProcessJob(uint32 objectId, Crypto_JobType* job);

/**
 * @brief Cancels a pending crypto job
 * @param objectId The driver object ID
 * @param job Pointer to the job structure
 * @return Result of the operation
 */
Std_ReturnType Crypto_CancelJob(uint32 objectId, Crypto_JobType* job);

/**********************************************************************************************************************
 * KEY MANAGEMENT API
 *********************************************************************************************************************/

/**
 * @brief Sets a key element
 * @param cryptoKeyId The key ID
 * @param keyElementId The key element ID
 * @param keyPtr Pointer to key data
 * @param keyLength Length of key data
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyElementSet(Crypto_KeyIdType cryptoKeyId,
                                     Crypto_KeyElementIdType keyElementId,
                                     const uint8* keyPtr,
                                     uint32 keyLength);

/**
 * @brief Gets a key element
 * @param cryptoKeyId The key ID
 * @param keyElementId The key element ID
 * @param keyPtr Pointer to buffer for key data
 * @param keyLengthPtr Pointer to length variable
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyElementGet(Crypto_KeyIdType cryptoKeyId,
                                     Crypto_KeyElementIdType keyElementId,
                                     uint8* keyPtr,
                                     uint32* keyLengthPtr);

/**
 * @brief Sets key validity state
 * @param cryptoKeyId The key ID
 * @param valid Validity state
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyValidSet(Crypto_KeyIdType cryptoKeyId,
                                   boolean valid);

/**
 * @brief Gets key element IDs
 * @param cryptoKeyId The key ID
 * @param keyElementIdsPtr Pointer to array for element IDs
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyElementIdsGet(Crypto_KeyIdType cryptoKeyId,
                                        uint32* keyElementIdsPtr);

/**
 * @brief Copies a key element
 * @param cryptoKeyId Source key ID
 * @param keyElementId Source element ID
 * @param targetCryptoKeyId Target key ID
 * @param targetKeyElementId Target element ID
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyElementCopy(Crypto_KeyIdType cryptoKeyId,
                                      Crypto_KeyElementIdType keyElementId,
                                      Crypto_KeyIdType targetCryptoKeyId,
                                      Crypto_KeyElementIdType targetKeyElementId);

/**
 * @brief Moves a key element
 * @param cryptoKeyId Source key ID
 * @param keyElementId Source element ID
 * @param targetCryptoKeyId Target key ID
 * @param targetKeyElementId Target element ID
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyElementMove(Crypto_KeyIdType cryptoKeyId,
                                      Crypto_KeyElementIdType keyElementId,
                                      Crypto_KeyIdType targetCryptoKeyId,
                                      Crypto_KeyElementIdType targetKeyElementId);

/**
 * @brief Clears a key element
 * @param cryptoKeyId The key ID
 * @param keyElementId The key element ID
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyElementClear(Crypto_KeyIdType cryptoKeyId,
                                       Crypto_KeyElementIdType keyElementId);

/**
 * @brief Copies an entire key
 * @param cryptoKeyId Source key ID
 * @param targetCryptoKeyId Target key ID
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyCopy(Crypto_KeyIdType cryptoKeyId,
                               Crypto_KeyIdType targetCryptoKeyId);

/**
 * @brief Generates a new key
 * @param cryptoKeyId The key ID to generate
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyGenerate(Crypto_KeyIdType cryptoKeyId);

/**********************************************************************************************************************
 * CRYPTOGRAPHIC OPERATIONS API
 *********************************************************************************************************************/

/**
 * @brief Derives a key using KDF
 * @param cryptoKeyId Source key ID
 * @param targetCryptoKeyId Target key ID
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyDerive(Crypto_KeyIdType cryptoKeyId,
                                 Crypto_KeyIdType targetCryptoKeyId);

/**
 * @brief Calculates shared secret for key exchange
 * @param cryptoKeyId Private key ID
 * @param partnerPublicKeyPtr Partner's public key
 * @param partnerPublicKeyLength Length of public key
 * @return Result of the operation
 */
Std_ReturnType Crypto_KeyExchangeCalcSecret(Crypto_KeyIdType cryptoKeyId,
                                             const uint8* partnerPublicKeyPtr,
                                             uint32 partnerPublicKeyLength);

/**
 * @brief Generates random data
 * @param cryptoKeyId The key ID for RNG
 * @param resultPtr Pointer to buffer for random data
 * @param resultLength Length of random data to generate
 * @return Result of the operation
 */
Std_ReturnType Crypto_RandomGenerate(Crypto_KeyIdType cryptoKeyId,
                                      uint8* resultPtr,
                                      uint32 resultLength);

/**
 * @brief Seeds the random number generator
 * @param cryptoKeyId The key ID for RNG
 * @param entropyPtr Pointer to entropy data
 * @param entropyLength Length of entropy data
 * @return Result of the operation
 */
Std_ReturnType Crypto_RandomSeed(Crypto_KeyIdType cryptoKeyId,
                                  const uint8* entropyPtr,
                                  uint32 entropyLength);

/**********************************************************************************************************************
 * HSM SPECIFIC API
 *********************************************************************************************************************/

#if (CRYPTO_CFG_HSM_ENABLED == STD_ON)

/**
 * @brief Checks if HSM is available
 * @return TRUE if HSM is ready, FALSE otherwise
 */
boolean Crypto_HsmIsAvailable(void);

/**
 * @brief Gets HSM status
 * @return HSM status
 */
Crypto_HsmStateType Crypto_HsmGetStatus(void);

/**
 * @brief Loads a key into HSM secure storage
 * @param cryptoKeyId The key ID to load
 * @return Result of the operation
 */
Std_ReturnType Crypto_HsmLoadKey(Crypto_KeyIdType cryptoKeyId);

/**
 * @brief Unloads a key from HSM secure storage
 * @param cryptoKeyId The key ID to unload
 * @return Result of the operation
 */
Std_ReturnType Crypto_HsmUnloadKey(Crypto_KeyIdType cryptoKeyId);

/**
 * @brief Performs HSM self-test
 * @return Result of the operation
 */
Std_ReturnType Crypto_HsmSelfTest(void);

/**
 * @brief Gets HSM unique ID
 * @param idPtr Pointer to buffer for HSM ID
 * @param idLengthPtr Pointer to length variable
 * @return Result of the operation
 */
Std_ReturnType Crypto_HsmGetId(uint8* idPtr, uint32* idLengthPtr);

#endif /* CRYPTO_CFG_HSM_ENABLED */

/**********************************************************************************************************************
 * CCC DIGITAL KEY SPECIFIC API
 *********************************************************************************************************************/

/**
 * @brief Generates a CCC device attestation signature
 * @param challengePtr Challenge data
 * @param challengeLength Challenge length
 * @param signaturePtr Output buffer for signature
 * @param signatureLengthPtr Pointer to signature length
 * @return Result of the operation
 */
Std_ReturnType Crypto_CccGenerateAttestation(const uint8* challengePtr,
                                              uint32 challengeLength,
                                              uint8* signaturePtr,
                                              uint32* signatureLengthPtr);

/**
 * @brief Verifies a CCC owner certificate
 * @param certificatePtr Certificate data
 * @param certificateLength Certificate length
 * @param verifyResultPtr Pointer to verification result
 * @return Result of the operation
 */
Std_ReturnType Crypto_CccVerifyOwnerCertificate(const uint8* certificatePtr,
                                                 uint32 certificateLength,
                                                 Crypto_VerifyResultType* verifyResultPtr);

/**
 * @brief Derives CCC session key using ECDH
 * @param ephemeralPublicKeyPtr Ephemeral public key
 * @param ephemeralPublicKeyLength Length of public key
 * @param sessionKeyId Output session key ID
 * @return Result of the operation
 */
Std_ReturnType Crypto_CccDeriveSessionKey(const uint8* ephemeralPublicKeyPtr,
                                           uint32 ephemeralPublicKeyLength,
                                           Crypto_KeyIdType sessionKeyId);

/**
 * @brief Encrypts data using CCC AES-GCM
 * @param keyId Key ID for encryption
 * @param plaintextPtr Plaintext data
 * @param plaintextLength Plaintext length
 * @param aadPtr Additional authenticated data
 * @param aadLength AAD length
 * @param ivPtr Initialization vector
 * @param ivLength IV length
 * @param ciphertextPtr Output buffer for ciphertext
 * @param tagPtr Output buffer for authentication tag
 * @param tagLengthPtr Pointer to tag length
 * @return Result of the operation
 */
Std_ReturnType Crypto_CccEncrypt(Crypto_KeyIdType keyId,
                                  const uint8* plaintextPtr,
                                  uint32 plaintextLength,
                                  const uint8* aadPtr,
                                  uint32 aadLength,
                                  const uint8* ivPtr,
                                  uint32 ivLength,
                                  uint8* ciphertextPtr,
                                  uint8* tagPtr,
                                  uint32* tagLengthPtr);

/**
 * @brief Decrypts data using CCC AES-GCM
 * @param keyId Key ID for decryption
 * @param ciphertextPtr Ciphertext data
 * @param ciphertextLength Ciphertext length
 * @param aadPtr Additional authenticated data
 * @param aadLength AAD length
 * @param ivPtr Initialization vector
 * @param ivLength IV length
 * @param tagPtr Authentication tag
 * @param tagLength Tag length
 * @param plaintextPtr Output buffer for plaintext
 * @param plaintextLengthPtr Pointer to plaintext length
 * @return Result of the operation
 */
Std_ReturnType Crypto_CccDecrypt(Crypto_KeyIdType keyId,
                                  const uint8* ciphertextPtr,
                                  uint32 ciphertextLength,
                                  const uint8* aadPtr,
                                  uint32 aadLength,
                                  const uint8* ivPtr,
                                  uint32 ivLength,
                                  const uint8* tagPtr,
                                  uint32 tagLength,
                                  uint8* plaintextPtr,
                                  uint32* plaintextLengthPtr);

/**********************************************************************************************************************
 * CALLBACK FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * BLAKE2 HASH API
 *********************************************************************************************************************/

/**
 * @brief Computes BLAKE2b hash
 * @param dataPtr Input data
 * @param dataLength Input data length
 * @param keyPtr Optional key for keyed hashing (can be NULL)
 * @param keyLength Key length (0 if no key)
 * @param digestLength Output digest length (1-64 bytes)
 * @param digestPtr Output buffer for hash
 * @return Result of the operation
 */
Std_ReturnType Crypto_Blake2b(const uint8* dataPtr,
                               uint32 dataLength,
                               const uint8* keyPtr,
                               uint32 keyLength,
                               uint32 digestLength,
                               uint8* digestPtr);

/**
 * @brief Computes BLAKE2s hash
 * @param dataPtr Input data
 * @param dataLength Input data length
 * @param keyPtr Optional key for keyed hashing (can be NULL)
 * @param keyLength Key length (0 if no key)
 * @param digestLength Output digest length (1-32 bytes)
 * @param digestPtr Output buffer for hash
 * @return Result of the operation
 */
Std_ReturnType Crypto_Blake2s(const uint8* dataPtr,
                               uint32 dataLength,
                               const uint8* keyPtr,
                               uint32 keyLength,
                               uint32 digestLength,
                               uint8* digestPtr);

/**
 * @brief Starts incremental BLAKE2b hash computation
 * @param jobId Job identifier
 * @param keyPtr Optional key for keyed hashing
 * @param keyLength Key length
 * @param digestLength Output digest length
 * @return Result of the operation
 */
Std_ReturnType Crypto_Blake2b_Start(uint32 jobId,
                                     const uint8* keyPtr,
                                     uint32 keyLength,
                                     uint32 digestLength);

/**
 * @brief Updates BLAKE2b hash with data
 * @param jobId Job identifier
 * @param dataPtr Input data
 * @param dataLength Input data length
 * @return Result of the operation
 */
Std_ReturnType Crypto_Blake2b_Update(uint32 jobId,
                                      const uint8* dataPtr,
                                      uint32 dataLength);

/**
 * @brief Finalizes BLAKE2b hash computation
 * @param jobId Job identifier
 * @param digestPtr Output buffer for hash
 * @param digestLengthPtr Pointer to digest length
 * @return Result of the operation
 */
Std_ReturnType Crypto_Blake2b_Finish(uint32 jobId,
                                      uint8* digestPtr,
                                      uint32* digestLengthPtr);

/**********************************************************************************************************************
 * CALLBACK FUNCTIONS
 *********************************************************************************************************************/

/**
 * @brief Job completion notification callback
 * @param job The completed job
 * @param result The result of the job
 */
void Crypto_JobNotification(Crypto_JobType* job, Crypto_ResultType result);

/**
 * @brief Error notification callback
 * @param errorCode The error code
 * @param moduleId The module ID
 * @param instanceId The instance ID
 * @param apiId The API ID
 * @param errorId The error ID
 */
void Crypto_ErrorNotification(uint16 moduleId, uint8 instanceId, uint8 apiId, uint8 errorId);

#endif /* CRYPTO_H */
