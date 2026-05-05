/*==================================================================================================
 *                              CRYPTO INTERFACE (CryIf)
 *==================================================================================================
 * FILENAME: CryIf.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_CryptoInterface.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Crypto Interface module
 *==================================================================================================
 */

#ifndef CRYIF_H
#define CRYIF_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "CryIf_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define CRYIF_VENDOR_ID                   (100u)
#define CRYIF_MODULE_ID                   (112u)
#define CRYIF_INSTANCE_ID                 (0u)

#define CRYIF_AR_RELEASE_MAJOR_VERSION    (4u)
#define CRYIF_AR_RELEASE_MINOR_VERSION    (7u)
#define CRYIF_AR_RELEASE_REVISION_VERSION (0u)

#define CRYIF_SW_MAJOR_VERSION            (1u)
#define CRYIF_SW_MINOR_VERSION            (0u)
#define CRYIF_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((CRYIF_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (CRYIF_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of CryIf.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define CRYIF_SID_INIT                        (0x00u)
#define CRYIF_SID_DEINIT                      (0x01u)
#define CRYIF_SID_GETVERSIONINFO              (0x02u)
#define CRYIF_SID_CRYPTOCALCULATION           (0x03u)
#define CRYIF_SID_CIPHERINIT                  (0x10u)
#define CRYIF_SID_CIPHERUPDATE                (0x11u)
#define CRYIF_SID_CIPHERFINISH                (0x12u)
#define CRYIF_SID_ENCRYPT                     (0x20u)
#define CRYIF_SID_DECRYPT                     (0x21u)
#define CRYIF_SID_MACGENERATE                 (0x30u)
#define CRYIF_SID_MACVERIFY                   (0x31u)
#define CRYIF_SID_HASH                        (0x40u)
#define CRYIF_SID_HASHSTART                   (0x41u)
#define CRYIF_SID_HASHUPDATE                  (0x42u)
#define CRYIF_SID_HASHFINISH                  (0x43u)
#define CRYIF_SID_RANDOMSEED                  (0x50u)
#define CRYIF_SID_RANDOMGENERATE              (0x51u)
#define CRYIF_SID_SIGNATUREGENERATE           (0x60u)
#define CRYIF_SID_SIGNATUREVERIFY             (0x61u)
#define CRYIF_SID_KEYSETVALID                 (0x70u)
#define CRYIF_SID_KEYELEMENTSET               (0x71u)
#define CRYIF_SID_KEYELEMENTGET               (0x72u)
#define CRYIF_SID_KEYELEMENTCOPY              (0x73u)
#define CRYIF_SID_KEYEXCHANGECALCPUBVAL       (0x80u)
#define CRYIF_SID_KEYEXCHANGECALCSECRET       (0x81u)
#define CRYIF_SID_KEYDERIVE                   (0x90u)
#define CRYIF_SID_KEYGENERATE                 (0x91u)
#define CRYIF_SID_CERTIFICATEPARSE            (0xA0u)
#define CRYIF_SID_CERTIFICATEVERIFY           (0xA1u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define CRYIF_E_PARAM_POINTER                 (0x01u)  /* API called with NULL pointer */
#define CRYIF_E_PARAM_HANDLE                  (0x02u)  /* Invalid handle parameter */
#define CRYIF_E_PARAM_VALUE                   (0x03u)  /* Invalid value parameter */
#define CRYIF_E_UNINIT                        (0x04u)  /* API called before initialization */
#define CRYIF_E_ALREADY_INITIALIZED           (0x05u)  /* Multiple initialization call */
#define CRYIF_E_INVALID_CHANNEL               (0x06u)  /* Invalid channel ID */
#define CRYIF_E_INVALID_KEY_ID                (0x07u)  /* Invalid key ID */
#define CRYIF_E_INVALID_ALGORITHM             (0x08u)  /* Invalid algorithm */
#define CRYIF_E_INVALID_BUFFER_SIZE           (0x09u)  /* Invalid buffer size */

/* Runtime error codes */
#define CRYIF_E_BUSY                          (0x01u)  /* Channel busy */
#define CRYIF_E_QUEUE_FULL                    (0x02u)  /* Queue full */
#define CRYIF_E_KEY_NOT_AVAILABLE             (0x03u)  /* Key not available */
#define CRYIF_E_KEY_NOT_VALID                 (0x04u)  /* Key not valid */
#define CRYIF_E_ENTROPY_EXHAUSTION            (0x05u)  /* Entropy exhaustion */

/*==================================================================================================
 *                                    ALGORITHM FAMILY DEFINITIONS
 *==================================================================================================*/
/**
 * @brief Algorithm family definitions for CryIf
 */
typedef enum {
    CRYIF_ALGOFAM_NOT_SET = 0,          /* Algorithm not set */
    CRYIF_ALGOFAM_AES,                  /* AES algorithm */
    CRYIF_ALGOFAM_DES,                  /* DES algorithm */
    CRYIF_ALGOFAM_3DES,                 /* Triple DES algorithm */
    CRYIF_ALGOFAM_RSA,                  /* RSA algorithm */
    CRYIF_ALGOFAM_ECC,                  /* Elliptic Curve Cryptography */
    CRYIF_ALGOFAM_SHA1,                 /* SHA-1 hash */
    CRYIF_ALGOFAM_SHA2_224,             /* SHA-224 hash */
    CRYIF_ALGOFAM_SHA2_256,             /* SHA-256 hash */
    CRYIF_ALGOFAM_SHA2_384,             /* SHA-384 hash */
    CRYIF_ALGOFAM_SHA2_512,             /* SHA-512 hash */
    CRYIF_ALGOFAM_MD5,                  /* MD5 hash */
    CRYIF_ALGOFAM_HMAC,                 /* HMAC */
    CRYIF_ALGOFAM_CMAC,                 /* CMAC */
    CRYIF_ALGOFAM_GCM,                  /* Galois Counter Mode */
    CRYIF_ALGOFAM_CCM                   /* Counter with CBC-MAC */
} CryIf_AlgorithmFamilyType;

/**
 * @brief Algorithm mode definitions
 */
typedef enum {
    CRYIF_ALGOMODE_NOT_SET = 0,         /* Mode not set */
    CRYIF_ALGOMODE_ECB,                 /* Electronic Codebook */
    CRYIF_ALGOMODE_CBC,                 /* Cipher Block Chaining */
    CRYIF_ALGOMODE_CFB,                 /* Cipher Feedback */
    CRYIF_ALGOMODE_OFB,                 /* Output Feedback */
    CRYIF_ALGOMODE_CTR,                 /* Counter Mode */
    CRYIF_ALGOMODE_GCM,                 /* Galois/Counter Mode */
    CRYIF_ALGOMODE_CCM                  /* Counter with CBC-MAC */
} CryIf_AlgorithmModeType;

/**
 * @brief Crypto operation type
 */
typedef enum {
    CRYIF_CRYPTOPRIMITIVE_NOT_SET = 0,  /* Not set */
    CRYIF_CRYPTOPRIMITIVE_ENCRYPT,      /* Encryption */
    CRYIF_CRYPTOPRIMITIVE_DECRYPT,      /* Decryption */
    CRYIF_CRYPTOPRIMITIVE_MAC_GENERATE, /* MAC generation */
    CRYIF_CRYPTOPRIMITIVE_MAC_VERIFY,   /* MAC verification */
    CRYIF_CRYPTOPRIMITIVE_HASH,         /* Hash calculation */
    CRYIF_CRYPTOPRIMITIVE_SIGNATURE_GENERATE, /* Signature generation */
    CRYIF_CRYPTOPRIMITIVE_SIGNATURE_VERIFY,   /* Signature verification */
    CRYIF_CRYPTOPRIMITIVE_RANDOM_GENERATE     /* Random number generation */
} CryIf_CryptoPrimitiveType;

/**
 * @brief Operation mode type
 */
typedef enum {
    CRYIF_OPERATIONMODE_START = 0x01,       /* Start operation */
    CRYIF_OPERATIONMODE_UPDATE = 0x02,      /* Update operation */
    CRYIF_OPERATIONMODE_FINISH = 0x04,      /* Finish operation */
    CRYIF_OPERATIONMODE_STREAMSTART = 0x07, /* Single call (start+update+finish) */
    CRYIF_OPERATIONMODE_SINGLECALL = 0x08   /* Single call operation */
} CryIf_OperationModeType;

/**
 * @brief Verification result type
 */
typedef enum {
    CRYIF_E_VER_OK = 0,                 /* Verification successful */
    CRYIF_E_VER_NOT_OK                  /* Verification failed */
} CryIf_VerifyResultType;

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief CryIf channel ID type
 */
typedef uint32 CryIf_ChannelIdType;

/**
 * @brief CryIf key ID type
 */
typedef uint32 CryIf_KeyIdType;

/**
 * @brief CryIf key element ID type
 */
typedef uint32 CryIf_KeyElementIdType;

/**
 * @brief CryIf result length type
 */
typedef uint32 CryIf_ResultLengthType;

/**
 * @brief CryIf crypto operation handle type
 */
typedef uint32 CryIf_CryptoHandleType;

/**
 * @brief Algorithm info type
 */
typedef struct {
    CryIf_AlgorithmFamilyType family;
    CryIf_AlgorithmModeType mode;
    uint32 keyLength;
    uint32 ivLength;
    uint32 authTagLength;
} CryIf_AlgorithmInfoType;

/**
 * @brief Job primitive info type
 */
typedef struct {
    CryIf_CryptoPrimitiveType primitive;
    CryIf_AlgorithmInfoType algorithm;
    uint32 resultLength;
} CryIf_JobPrimitiveInfoType;

/**
 * @brief Job info type
 */
typedef struct {
    CryIf_ChannelIdType channelId;
    CryIf_JobPrimitiveInfoType* primitiveInfo;
    CryIf_KeyIdType keyId;
    boolean callbackActive;
} CryIf_JobInfoType;

/**
 * @brief Key info type
 */
typedef struct {
    CryIf_KeyIdType keyId;
    uint32 keyLength;
    boolean keyValid;
} CryIf_KeyInfoType;

/**
 * @brief Channel configuration type
 */
typedef struct {
    CryIf_ChannelIdType channelId;
    uint32 priority;
    CryIf_CryptoPrimitiveType primitive;
    CryIf_AlgorithmFamilyType algorithmFamily;
    CryIf_AlgorithmModeType algorithmMode;
    boolean callbackActive;
} CryIf_ChannelConfigType;

/**
 * @brief Key configuration type
 */
typedef struct {
    CryIf_KeyIdType cryIfKeyId;
    uint32 keyLength;
    boolean keyValid;
} CryIf_KeyConfigType;

/**
 * @brief CryIf configuration type
 */
typedef struct {
    const CryIf_ChannelConfigType* channelConfigs;
    uint16 numChannels;
    const CryIf_KeyConfigType* keyConfigs;
    uint16 numKeys;
} CryIf_ConfigType;

/*==================================================================================================
 *                                    GLOBAL CONSTANTS
 *==================================================================================================*/
#define CRYIF_INVALID_CHANNEL_ID          (0xFFFFFFFFu)
#define CRYIF_INVALID_KEY_ID              (0xFFFFFFFFu)
#define CRYIF_INVALID_KEY_ELEMENT         (0xFFFFFFFFu)

/* AES Constants */
#define CRYIF_AES_BLOCK_SIZE              (16u)       /* 128 bits */
#define CRYIF_AES_IV_SIZE                 (16u)       /* 128 bits */
#define CRYIF_AES_KEY_SIZE_128            (16u)       /* 128 bits */
#define CRYIF_AES_KEY_SIZE_192            (24u)       /* 192 bits */
#define CRYIF_AES_KEY_SIZE_256            (32u)       /* 256 bits */

/* SHA Constants */
#define CRYIF_SHA1_SIZE                   (20u)       /* 160 bits */
#define CRYIF_SHA224_SIZE                 (28u)       /* 224 bits */
#define CRYIF_SHA256_SIZE                 (32u)       /* 256 bits */
#define CRYIF_SHA384_SIZE                 (48u)       /* 384 bits */
#define CRYIF_SHA512_SIZE                 (64u)       /* 512 bits */

/* HMAC Constants */
#define CRYIF_HMAC_SHA256_SIZE            (32u)       /* 256 bits */

/* RSA Constants */
#define CRYIF_RSA_KEY_SIZE_1024           (128u)      /* 1024 bits */
#define CRYIF_RSA_KEY_SIZE_2048           (256u)      /* 2048 bits */
#define CRYIF_RSA_KEY_SIZE_4096           (512u)      /* 4096 bits */

/*==================================================================================================
 *                                    CALLBACK TYPE
 *==================================================================================================*/
/**
 * @brief Channel notification callback type
 */
typedef void (*CryIf_CallbackType)(const CryIf_ChannelIdType channelId,
                                   Std_ReturnType result);

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define CRYIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "CryIf_MemMap.h"

extern boolean CryIf_Initialized;
extern const CryIf_ConfigType* CryIf_ConfigPtr;

#define CRYIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "CryIf_MemMap.h"

/*==================================================================================================
 *                                     API DECLARATIONS
 *==================================================================================================*/
#define CRYIF_START_SEC_CODE
#include "CryIf_MemMap.h"

/**
 * @brief Initializes the Crypto Interface module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_CryIf_00001
 */
extern void CryIf_Init(const CryIf_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Crypto Interface module
 * @return None
 * @req SWS_CryIf_00002
 */
extern void CryIf_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_CryIf_00003
 */
#if (CRYIF_VERSION_INFO_API == STD_ON)
extern void CryIf_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Initializes a cipher session
 * @param channelId Channel ID for the cipher operation
 * @param algorithm Algorithm to use
 * @param keyId Key ID for the operation
 * @param ivPtr Initialization vector pointer
 * @param ivLength IV length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00100
 */
extern Std_ReturnType CryIf_CipherInit(CryIf_ChannelIdType channelId,
                                        const CryIf_AlgorithmInfoType* algorithm,
                                        CryIf_KeyIdType keyId,
                                        const uint8* ivPtr,
                                        uint32 ivLength);

/**
 * @brief Encrypts data
 * @param channelId Channel ID for encryption
 * @param mode Operation mode
 * @param plaintextPtr Input plaintext pointer
 * @param plaintextLength Input length
 * @param ciphertextPtr Output ciphertext buffer
 * @param ciphertextLengthPtr Output length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00200
 */
extern Std_ReturnType CryIf_Encrypt(CryIf_ChannelIdType channelId,
                                     CryIf_OperationModeType mode,
                                     const uint8* plaintextPtr,
                                     uint32 plaintextLength,
                                     uint8* ciphertextPtr,
                                     uint32* ciphertextLengthPtr);

/**
 * @brief Decrypts data
 * @param channelId Channel ID for decryption
 * @param mode Operation mode
 * @param ciphertextPtr Input ciphertext pointer
 * @param ciphertextLength Input length
 * @param plaintextPtr Output plaintext buffer
 * @param plaintextLengthPtr Output length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00210
 */
extern Std_ReturnType CryIf_Decrypt(CryIf_ChannelIdType channelId,
                                     CryIf_OperationModeType mode,
                                     const uint8* ciphertextPtr,
                                     uint32 ciphertextLength,
                                     uint8* plaintextPtr,
                                     uint32* plaintextLengthPtr);

/**
 * @brief Generates MAC (Message Authentication Code)
 * @param channelId Channel ID for MAC generation
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input length
 * @param macPtr MAC output buffer
 * @param macLengthPtr MAC length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00300
 */
extern Std_ReturnType CryIf_MacGenerate(CryIf_ChannelIdType channelId,
                                         CryIf_OperationModeType mode,
                                         const uint8* dataPtr,
                                         uint32 dataLength,
                                         uint8* macPtr,
                                         uint32* macLengthPtr);

/**
 * @brief Verifies MAC
 * @param channelId Channel ID for MAC verification
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input length
 * @param macPtr MAC to verify
 * @param macLength MAC length
 * @param verifyPtr Verification result pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00310
 */
extern Std_ReturnType CryIf_MacVerify(CryIf_ChannelIdType channelId,
                                       CryIf_OperationModeType mode,
                                       const uint8* dataPtr,
                                       uint32 dataLength,
                                       const uint8* macPtr,
                                       uint32 macLength,
                                       CryIf_VerifyResultType* verifyPtr);

/**
 * @brief Calculates hash
 * @param channelId Channel ID for hash calculation
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input length
 * @param resultPtr Hash output buffer
 * @param resultLengthPtr Hash length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00400
 */
extern Std_ReturnType CryIf_Hash(CryIf_ChannelIdType channelId,
                                  CryIf_OperationModeType mode,
                                  const uint8* dataPtr,
                                  uint32 dataLength,
                                  uint8* resultPtr,
                                  uint32* resultLengthPtr);

/**
 * @brief Starts hash calculation
 * @param channelId Channel ID
 * @param algorithm Algorithm to use
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType CryIf_HashStart(CryIf_ChannelIdType channelId,
                                       const CryIf_AlgorithmInfoType* algorithm);

/**
 * @brief Updates hash calculation
 * @param channelId Channel ID
 * @param dataPtr Input data pointer
 * @param dataLength Input length
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType CryIf_HashUpdate(CryIf_ChannelIdType channelId,
                                        const uint8* dataPtr,
                                        uint32 dataLength);

/**
 * @brief Finishes hash calculation
 * @param channelId Channel ID
 * @param resultPtr Hash output buffer
 * @param resultLengthPtr Hash length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType CryIf_HashFinish(CryIf_ChannelIdType channelId,
                                        uint8* resultPtr,
                                        uint32* resultLengthPtr);

/**
 * @brief Generates signature
 * @param channelId Channel ID for signature generation
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input length
 * @param signaturePtr Signature output buffer
 * @param signatureLengthPtr Signature length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00500
 */
extern Std_ReturnType CryIf_SignatureGenerate(CryIf_ChannelIdType channelId,
                                               CryIf_OperationModeType mode,
                                               const uint8* dataPtr,
                                               uint32 dataLength,
                                               uint8* signaturePtr,
                                               uint32* signatureLengthPtr);

/**
 * @brief Verifies signature
 * @param channelId Channel ID for signature verification
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input length
 * @param signaturePtr Signature to verify
 * @param signatureLength Signature length
 * @param verifyPtr Verification result pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00510
 */
extern Std_ReturnType CryIf_SignatureVerify(CryIf_ChannelIdType channelId,
                                             CryIf_OperationModeType mode,
                                             const uint8* dataPtr,
                                             uint32 dataLength,
                                             const uint8* signaturePtr,
                                             uint32 signatureLength,
                                             CryIf_VerifyResultType* verifyPtr);

/**
 * @brief Generates random number
 * @param channelId Channel ID for random generation
 * @param resultPtr Output buffer
 * @param resultLength Requested random length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00600
 */
extern Std_ReturnType CryIf_RandomGenerate(CryIf_ChannelIdType channelId,
                                            uint8* resultPtr,
                                            uint32 resultLength);

/**
 * @brief Seeds the random number generator
 * @param channelId Channel ID
 * @param seedPtr Seed data pointer
 * @param seedLength Seed length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00610
 */
extern Std_ReturnType CryIf_RandomSeed(CryIf_ChannelIdType channelId,
                                        const uint8* seedPtr,
                                        uint32 seedLength);

/**
 * @brief Sets key element
 * @param cryIfKeyId Key ID
 * @param keyElementId Key element ID
 * @param keyPtr Key data pointer
 * @param keyLength Key length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00700
 */
extern Std_ReturnType CryIf_KeyElementSet(CryIf_KeyIdType cryIfKeyId,
                                           CryIf_KeyElementIdType keyElementId,
                                           const uint8* keyPtr,
                                           uint32 keyLength);

/**
 * @brief Gets key element
 * @param cryIfKeyId Key ID
 * @param keyElementId Key element ID
 * @param keyPtr Output buffer
 * @param keyLengthPtr Length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00710
 */
extern Std_ReturnType CryIf_KeyElementGet(CryIf_KeyIdType cryIfKeyId,
                                           CryIf_KeyElementIdType keyElementId,
                                           uint8* keyPtr,
                                           uint32* keyLengthPtr);

/**
 * @brief Copies key element
 * @param cryIfKeyId Source key ID
 * @param keyElementId Source key element ID
 * @param targetCryIfKeyId Target key ID
 * @param targetKeyElementId Target key element ID
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00720
 */
extern Std_ReturnType CryIf_KeyElementCopy(CryIf_KeyIdType cryIfKeyId,
                                            CryIf_KeyElementIdType keyElementId,
                                            CryIf_KeyIdType targetCryIfKeyId,
                                            CryIf_KeyElementIdType targetKeyElementId);

/**
 * @brief Sets key as valid
 * @param cryIfKeyId Key ID
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00730
 */
extern Std_ReturnType CryIf_KeySetValid(CryIf_KeyIdType cryIfKeyId);

/**
 * @brief Generates a key
 * @param cryIfKeyId Key ID
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00800
 */
extern Std_ReturnType CryIf_KeyGenerate(CryIf_KeyIdType cryIfKeyId);

/**
 * @brief Derives a key
 * @param cryIfKeyId Source key ID
 * @param targetCryIfKeyId Target key ID
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00810
 */
extern Std_ReturnType CryIf_KeyDerive(CryIf_KeyIdType cryIfKeyId,
                                       CryIf_KeyIdType targetCryIfKeyId);

/**
 * @brief Calculates public value for key exchange
 * @param cryIfKeyId Key ID
 * @param publicValuePtr Public value output buffer
 * @param publicValueLengthPtr Public value length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00900
 */
extern Std_ReturnType CryIf_KeyExchangeCalcPubVal(CryIf_KeyIdType cryIfKeyId,
                                                   uint8* publicValuePtr,
                                                   uint32* publicValueLengthPtr);

/**
 * @brief Calculates secret for key exchange
 * @param cryIfKeyId Key ID
 * @param partnerPublicValuePtr Partner's public value
 * @param partnerPublicValueLength Partner's public value length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00910
 */
extern Std_ReturnType CryIf_KeyExchangeCalcSecret(CryIf_KeyIdType cryIfKeyId,
                                                   const uint8* partnerPublicValuePtr,
                                                   uint32 partnerPublicValueLength);

/**
 * @brief Parses a certificate
 * @param cryIfKeyId Certificate key ID
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00A00
 */
extern Std_ReturnType CryIf_CertificateParse(CryIf_KeyIdType cryIfKeyId);

/**
 * @brief Verifies a certificate
 * @param cryIfKeyId Certificate key ID
 * @param verifyCryIfKeyId Verification key ID
 * @param verifyPtr Verification result pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_CryIf_00A10
 */
extern Std_ReturnType CryIf_CertificateVerify(CryIf_KeyIdType cryIfKeyId,
                                               CryIf_KeyIdType verifyCryIfKeyId,
                                               CryIf_VerifyResultType* verifyPtr);

/**
 * @brief Main function for processing
 * @return None
 * @req SWS_CryIf_01000
 */
extern void CryIf_MainFunction(void);

/**
 * @brief Processes a crypto job (for Csm integration)
 * @param channelId Channel ID
 * @param mode Operation mode
 * @param jobPrimitiveInputOutput Job input/output data
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType CryIf_ProcessJob(CryIf_ChannelIdType channelId,
                                        CryIf_OperationModeType mode,
                                        void* jobPrimitiveInputOutput);

#define CRYIF_STOP_SEC_CODE
#include "CryIf_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* CRYIF_H */
