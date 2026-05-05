/**********************************************************************************************************************
 * @file       Crypto_S32K312_Hsm.h
 * @brief      S32K312 HSM Hardware Abstraction Layer Header
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      S32K312 HSM (Hardware Security Module) Hardware Abstraction Layer.
 *      Provides low-level interface to S32K312 hardware cryptographic accelerators:
 *      - ARM Cortex-M7 based HSM core
 *      - Hardware AES-128/256 accelerator (ECB/CBC/GCM modes)
 *      - Hardware ECC accelerator (P-256, P-384 curves)
 *      - Hardware SHA-256 accelerator
 *      - True Random Number Generator (TRNG)
 *      - Secure key storage
 *
 * @hardware_reference
 *      NXP S32K3xx Reference Manual - Security Module (HSM)
 *********************************************************************************************************************/

#ifndef CRYPTO_S32K312_HSM_H
#define CRYPTO_S32K312_HSM_H

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto_Types.h"
#include "Std_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS
 *********************************************************************************************************************/
/* S32K312 HSM Version */
#define S32K312_HSM_SW_MAJOR_VERSION        (1U)
#define S32K312_HSM_SW_MINOR_VERSION        (0U)
#define S32K312_HSM_SW_PATCH_VERSION        (0U)

/* HSM Module IDs */
#define S32K312_HSM_MODULE_ID               (0x81U)
#define S32K312_HSM_SID_INIT                (0x00U)
#define S32K312_HSM_SID_DEINIT              (0x01U)
#define S32K312_HSM_SID_SELFTEST            (0x02U)
#define S32K312_HSM_SID_AES_ENCRYPT         (0x10U)
#define S32K312_HSM_SID_AES_DECRYPT         (0x11U)
#define S32K312_HSM_SID_ECC_SIGN            (0x20U)
#define S32K312_HSM_SID_ECC_VERIFY          (0x21U)
#define S32K312_HSM_SID_ECC_PMULT           (0x22U)
#define S32K312_HSM_SID_SHA256              (0x30U)
#define S32K312_HSM_SID_RNG                 (0x40U)

/* HSM Hardware Register Base Addresses */
#define S32K312_HSM_BASE_ADDR               (0x40460000UL)
#define S32K312_HSM_AES_BASE                (0x40461000UL)
#define S32K312_HSM_ECC_BASE                (0x40462000UL)
#define S32K312_HSM_SHA_BASE                (0x40463000UL)
#define S32K312_HSM_TRNG_BASE               (0x40464000UL)
#define S32K312_HSM_KEYSTORE_BASE           (0x40465000UL)

/* HSM Status Codes */
#define S32K312_HSM_SUCCESS                 (0x00000000UL)
#define S32K312_HSM_ERROR_BUSY              (0x00000001UL)
#define S32K312_HSM_ERROR_TIMEOUT           (0x00000002UL)
#define S32K312_HSM_ERROR_INVALID_PARAM     (0x00000003UL)
#define S32K312_HSM_ERROR_HARDWARE          (0x00000004UL)
#define S32K312_HSM_ERROR_KEY_NOT_FOUND     (0x00000005UL)
#define S32K312_HSM_ERROR_VERIFICATION      (0x00000006UL)
#define S32K312_HSM_ERROR_BUFFER_TOO_SMALL  (0x00000007UL)
#define S32K312_HSM_ERROR_NOT_SUPPORTED     (0x00000008UL)

/* AES Configuration */
#define S32K312_HSM_AES_KEY_SIZE_128        (16U)
#define S32K312_HSM_AES_KEY_SIZE_256        (32U)
#define S32K312_HSM_AES_BLOCK_SIZE          (16U)
#define S32K312_HSM_AES_IV_SIZE             (16U)
#define S32K312_HSM_AES_GCM_TAG_SIZE        (16U)
#define S32K312_HSM_AES_GCM_IV_SIZE         (12U)

/* ECC Configuration */
#define S32K312_HSM_ECC_CURVE_P256          (0x00U)
#define S32K312_HSM_ECC_CURVE_P384          (0x01U)
#define S32K312_HSM_ECC_P256_KEY_SIZE       (32U)
#define S32K312_HSM_ECC_P256_POINT_SIZE     (64U)
#define S32K312_HSM_ECC_P384_KEY_SIZE       (48U)
#define S32K312_HSM_ECC_P384_POINT_SIZE     (96U)
#define S32K312_HSM_ECC_MAX_POINT_SIZE      (96U)
#define S32K312_HSM_ECC_MAX_SIG_SIZE        (96U)

/* SHA-256 Configuration */
#define S32K312_HSM_SHA256_BLOCK_SIZE       (64U)
#define S32K312_HSM_SHA256_DIGEST_SIZE      (32U)

/* TRNG Configuration */
#define S32K312_HSM_TRNG_MIN_ENTROPY        (128U)
#define S32K312_HSM_TRNG_MAX_REQUEST        (256U)

/* Timeout Configuration */
#define S32K312_HSM_TIMEOUT_DEFAULT         (10000U)    /* 10ms default timeout */
#define S32K312_HSM_TIMEOUT_AES             (5000U)     /* 5ms for AES ops */
#define S32K312_HSM_TIMEOUT_ECC             (50000U)    /* 50ms for ECC ops */
#define S32K312_HSM_TIMEOUT_SHA             (5000U)     /* 5ms for SHA ops */
#define S32K312_HSM_TIMEOUT_RNG             (1000U)     /* 1ms for RNG */

/* Key Slot Configuration */
#define S32K312_HSM_MAX_KEY_SLOTS           (32U)
#define S32K312_HSM_KEY_SLOT_INVALID        (0xFFU)

/**********************************************************************************************************************
 * GLOBAL DATA TYPES
 *********************************************************************************************************************/

/* HSM Hardware State */
typedef enum {
    S32K312_HSM_STATE_UNINIT = 0,
    S32K312_HSM_STATE_INIT,
    S32K312_HSM_STATE_READY,
    S32K312_HSM_STATE_BUSY,
    S32K312_HSM_STATE_ERROR,
    S32K312_HSM_STATE_LOCKED
} S32K312_HsmStateType;

/* AES Operation Mode */
typedef enum {
    S32K312_HSM_AES_MODE_ECB = 0,
    S32K312_HSM_AES_MODE_CBC,
    S32K312_HSM_AES_MODE_GCM
} S32K312_HsmAesModeType;

/* ECC Curve Type */
typedef enum {
    S32K312_HSM_ECC_CURVE_SECP256R1 = 0,
    S32K312_HSM_ECC_CURVE_SECP384R1
} S32K312_HsmEccCurveType;

/* HSM Configuration Structure */
typedef struct {
    boolean enableAes;
    boolean enableEcc;
    boolean enableSha;
    boolean enableTrng;
    boolean enableKeyStore;
    uint32  timeoutUs;
    void*   callback;
} S32K312_HsmConfigType;

/* AES Context Structure */
typedef struct {
    uint8   key[S32K312_HSM_AES_KEY_SIZE_256];
    uint8   iv[S32K312_HSM_AES_IV_SIZE];
    uint32  keyLength;
    S32K312_HsmAesModeType mode;
    boolean keyLoaded;
} S32K312_HsmAesContextType;

/* ECC Context Structure */
typedef struct {
    S32K312_HsmEccCurveType curve;
    uint8   privateKey[S32K312_HSM_ECC_P384_KEY_SIZE];
    uint8   publicKey[S32K312_HSM_ECC_P384_POINT_SIZE];
    uint32  keyLength;
    boolean keyLoaded;
} S32K312_HsmEccContextType;

/* SHA-256 Context Structure */
typedef struct {
    uint32  totalLength;
    uint8   buffer[S32K312_HSM_SHA256_BLOCK_SIZE];
    uint32  bufferLength;
    boolean initialized;
} S32K312_HsmSha256ContextType;

/* HSM Key Slot Information */
typedef struct {
    uint8   slotId;
    uint8   keyType;
    uint16  keyLength;
    boolean occupied;
    boolean locked;
} S32K312_HsmKeySlotType;

/* HSM Status Information */
typedef struct {
    S32K312_HsmStateType state;
    uint32  errorCode;
    uint32  operationCount;
    boolean aesAvailable;
    boolean eccAvailable;
    boolean shaAvailable;
    boolean trngAvailable;
    boolean keyStoreAvailable;
    uint8   firmwareVersion[4];
} S32K312_HsmStatusType;

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - INITIALIZATION
 *********************************************************************************************************************/

/**
 * @brief Initializes the S32K312 HSM hardware module
 * @param config Pointer to HSM configuration structure
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_Init(const S32K312_HsmConfigType* config);

/**
 * @brief Deinitializes the S32K312 HSM hardware module
 */
void S32K312_Hsm_DeInit(void);

/**
 * @brief Performs HSM self-test
 * @return E_OK if all tests passed, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_SelfTest(void);

/**
 * @brief Gets current HSM status
 * @param status Pointer to status structure to fill
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_GetStatus(S32K312_HsmStatusType* status);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - AES OPERATIONS
 *********************************************************************************************************************/

/**
 * @brief Initializes AES context for HSM operations
 * @param context Pointer to AES context
 * @param key Pointer to key data
 * @param keyLength Key length (16 or 32 bytes)
 * @param mode AES operation mode (ECB/CBC/GCM)
 * @param iv Pointer to IV (NULL for ECB)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesInit(S32K312_HsmAesContextType* context,
                                    const uint8* key,
                                    uint32 keyLength,
                                    S32K312_HsmAesModeType mode,
                                    const uint8* iv);

/**
 * @brief Performs AES-ECB encryption using HSM
 * @param context Pointer to initialized AES context
 * @param plaintext Pointer to plaintext data
 * @param ciphertext Pointer to ciphertext output buffer
 * @param length Data length (must be multiple of 16)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesEcbEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint8* ciphertext,
                                          uint32 length);

/**
 * @brief Performs AES-ECB decryption using HSM
 * @param context Pointer to initialized AES context
 * @param ciphertext Pointer to ciphertext data
 * @param plaintext Pointer to plaintext output buffer
 * @param length Data length (must be multiple of 16)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesEcbDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint8* plaintext,
                                          uint32 length);

/**
 * @brief Performs AES-CBC encryption using HSM
 * @param context Pointer to initialized AES context
 * @param plaintext Pointer to plaintext data
 * @param ciphertext Pointer to ciphertext output buffer
 * @param length Data length (must be multiple of 16)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesCbcEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint8* ciphertext,
                                          uint32 length);

/**
 * @brief Performs AES-CBC decryption using HSM
 * @param context Pointer to initialized AES context
 * @param ciphertext Pointer to ciphertext data
 * @param plaintext Pointer to plaintext output buffer
 * @param length Data length (must be multiple of 16)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesCbcDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint8* plaintext,
                                          uint32 length);

/**
 * @brief Performs AES-GCM encryption using HSM
 * @param context Pointer to initialized AES context
 * @param plaintext Pointer to plaintext data
 * @param plaintextLength Plaintext length
 * @param aad Pointer to additional authenticated data
 * @param aadLength AAD length
 * @param iv Pointer to IV (12 bytes)
 * @param ciphertext Pointer to ciphertext output buffer
 * @param tag Pointer to authentication tag output buffer (16 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesGcmEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint32 plaintextLength,
                                          const uint8* aad,
                                          uint32 aadLength,
                                          const uint8* iv,
                                          uint8* ciphertext,
                                          uint8* tag);

/**
 * @brief Performs AES-GCM decryption using HSM
 * @param context Pointer to initialized AES context
 * @param ciphertext Pointer to ciphertext data
 * @param ciphertextLength Ciphertext length
 * @param aad Pointer to additional authenticated data
 * @param aadLength AAD length
 * @param iv Pointer to IV (12 bytes)
 * @param tag Pointer to authentication tag (16 bytes)
 * @param plaintext Pointer to plaintext output buffer
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_AesGcmDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint32 ciphertextLength,
                                          const uint8* aad,
                                          uint32 aadLength,
                                          const uint8* iv,
                                          const uint8* tag,
                                          uint8* plaintext);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - ECC OPERATIONS
 *********************************************************************************************************************/

/**
 * @brief Initializes ECC context for HSM operations
 * @param context Pointer to ECC context
 * @param curve ECC curve type (P-256 or P-384)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_EccInit(S32K312_HsmEccContextType* context,
                                    S32K312_HsmEccCurveType curve);

/**
 * @brief Loads private key into ECC context
 * @param context Pointer to ECC context
 * @param privateKey Pointer to private key
 * @param keyLength Key length (32 for P-256, 48 for P-384)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_EccLoadPrivateKey(S32K312_HsmEccContextType* context,
                                              const uint8* privateKey,
                                              uint32 keyLength);

/**
 * @brief Loads public key into ECC context
 * @param context Pointer to ECC context
 * @param publicKey Pointer to public key (uncompressed format: 0x04 || X || Y)
 * @param keyLength Key length (64 for P-256, 96 for P-384)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_EccLoadPublicKey(S32K312_HsmEccContextType* context,
                                             const uint8* publicKey,
                                             uint32 keyLength);

/**
 * @brief Performs ECC point multiplication using HSM
 * @param context Pointer to ECC context with loaded private key
 * @param resultPoint Pointer to output buffer for result point
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_EccPointMultiply(const S32K312_HsmEccContextType* context,
                                             uint8* resultPoint);

/**
 * @brief Performs ECDSA sign operation using HSM
 * @param context Pointer to ECC context with loaded private key
 * @param digest Pointer to message digest (SHA-256 output)
 * @param digestLength Digest length (32 bytes)
 * @param signature Pointer to output buffer for signature (R || S)
 * @param signatureLength Pointer to signature length variable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_EccSign(const S32K312_HsmEccContextType* context,
                                    const uint8* digest,
                                    uint32 digestLength,
                                    uint8* signature,
                                    uint32* signatureLength);

/**
 * @brief Performs ECDSA verify operation using HSM
 * @param context Pointer to ECC context with loaded public key
 * @param digest Pointer to message digest (SHA-256 output)
 * @param digestLength Digest length (32 bytes)
 * @param signature Pointer to signature (R || S)
 * @param signatureLength Signature length
 * @param verifyResult Pointer to verification result
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_EccVerify(const S32K312_HsmEccContextType* context,
                                      const uint8* digest,
                                      uint32 digestLength,
                                      const uint8* signature,
                                      uint32 signatureLength,
                                      Crypto_VerifyResultType* verifyResult);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - SHA-256 OPERATIONS
 *********************************************************************************************************************/

/**
 * @brief Initializes SHA-256 context
 * @param context Pointer to SHA-256 context
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_Sha256Init(S32K312_HsmSha256ContextType* context);

/**
 * @brief Updates SHA-256 hash with data
 * @param context Pointer to SHA-256 context
 * @param data Pointer to input data
 * @param length Data length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_Sha256Update(S32K312_HsmSha256ContextType* context,
                                         const uint8* data,
                                         uint32 length);

/**
 * @brief Finalizes SHA-256 hash computation
 * @param context Pointer to SHA-256 context
 * @param digest Pointer to output buffer for digest (32 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_Sha256Finish(S32K312_HsmSha256ContextType* context,
                                         uint8* digest);

/**
 * @brief Performs single-call SHA-256 hash using HSM
 * @param data Pointer to input data
 * @param length Data length
 * @param digest Pointer to output buffer for digest (32 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_Sha256(const uint8* data,
                                   uint32 length,
                                   uint8* digest);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - KEY STORAGE
 *********************************************************************************************************************/

/**
 * @brief Imports a key into HSM secure key storage
 * @param slotId Key slot ID (0-31)
 * @param keyType Key type identifier
 * @param keyData Pointer to key data
 * @param keyLength Key length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_KeyImport(uint8 slotId,
                                      uint8 keyType,
                                      const uint8* keyData,
                                      uint16 keyLength);

/**
 * @brief Exports a key from HSM secure key storage (if allowed)
 * @param slotId Key slot ID
 * @param keyData Pointer to output buffer
 * @param keyLength Pointer to key length variable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_KeyExport(uint8 slotId,
                                      uint8* keyData,
                                      uint16* keyLength);

/**
 * @brief Erases a key from HSM secure key storage
 * @param slotId Key slot ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_KeyErase(uint8 slotId);

/**
 * @brief Gets information about a key slot
 * @param slotId Key slot ID
 * @param slotInfo Pointer to slot information structure
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_KeyGetSlotInfo(uint8 slotId,
                                           S32K312_HsmKeySlotType* slotInfo);

/**********************************************************************************************************************
 * GLOBAL FUNCTION PROTOTYPES - UTILITY
 *********************************************************************************************************************/

/**
 * @brief Waits for HSM operation completion with timeout
 * @param timeoutUs Timeout in microseconds
 * @return E_OK if operation completed, E_NOT_OK on timeout
 */
Std_ReturnType S32K312_Hsm_WaitReady(uint32 timeoutUs);

/**
 * @brief Clears HSM error status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_ClearError(void);

/**
 * @brief Gets HSM firmware version
 * @param version Pointer to version buffer (4 bytes)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType S32K312_Hsm_GetFirmwareVersion(uint8* version);

#endif /* CRYPTO_S32K312_HSM_H */
