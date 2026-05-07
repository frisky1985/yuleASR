/**=================================================================================================
 * @file Crypto.h
 * @brief Hardware Crypto Driver interface following AUTOSAR Classic Platform 4.4/R22-11 standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AUTOSAR Standard: Crypto Driver (Crypto)
 * Layer: MCAL (Microcontroller Driver Layer)
 *==================================================================================================*/

#ifndef CRYPTO_H
#define CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Crypto_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define CRYPTO_VENDOR_ID                   (0x64U) /* YuleTech Vendor ID */
#define CRYPTO_MODULE_ID                   (0x78U) /* Crypto Driver Module ID */
#define CRYPTO_INSTANCE_ID                 (0x00U)

#define CRYPTO_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define CRYPTO_AR_RELEASE_MINOR_VERSION    (0x07U)
#define CRYPTO_AR_RELEASE_REVISION_VERSION (0x00U)

#define CRYPTO_SW_MAJOR_VERSION            (0x01U)
#define CRYPTO_SW_MINOR_VERSION            (0x00U)
#define CRYPTO_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define CRYPTO_SID_INIT                         (0x00U)
#define CRYPTO_SID_DEINIT                       (0x01U)
#define CRYPTO_SID_GETVERSIONINFO               (0x02U)
#define CRYPTO_SID_PROCESSJOB                   (0x03U)
#define CRYPTO_SID_CANCELJOB                    (0x04U)
#define CRYPTO_SID_KEYELEMENTSET                (0x05U)
#define CRYPTO_SID_KEYVALIDSET                  (0x06U)
#define CRYPTO_SID_KEYELEMENTGET                (0x07U)
#define CRYPTO_SID_KEYELEMENTCOPY               (0x08U)
#define CRYPTO_SID_KEYCOPY                      (0x09U)
#define CRYPTO_SID_KEYGENERATE                  (0x0AU)
#define CRYPTO_SID_KEYDERIVE                    (0x0BU)
#define CRYPTO_SID_KEYEXCHANGECALCPUBVAL        (0x0CU)
#define CRYPTO_SID_KEYEXCHANGECALCSECRET        (0x0DU)
#define CRYPTO_SID_CERTIFICATEPARSE             (0x0EU)
#define CRYPTO_SID_CERTIFICATEVERIFY            (0x0FU)
#define CRYPTO_SID_RANDOMSEED                   (0x10U)
#define CRYPTO_SID_MACGENERATE                  (0x11U)
#define CRYPTO_SID_MACVERIFY                    (0x12U)
#define CRYPTO_SID_ENCRYPT                      (0x13U)
#define CRYPTO_SID_DECRYPT                      (0x14U)
#define CRYPTO_SID_SIGNATUREGENERATE            (0x15U)
#define CRYPTO_SID_SIGNATUREVERIFY              (0x16U)
#define CRYPTO_SID_HASH                         (0x17U)
#define CRYPTO_SID_RANDOMGENERATE               (0x18U)
#define CRYPTO_SID_KEYELEMENTIDSGET             (0x19U)
#define CRYPTO_SID_KEYEXCHANGESETPUBVAL         (0x1AU)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define CRYPTO_E_PARAM_POINTER                  (0x01U)  /* API called with NULL pointer */
#define CRYPTO_E_PARAM_HANDLE                   (0x02U)  /* Invalid handle parameter */
#define CRYPTO_E_PARAM_VALUE                    (0x03U)  /* Invalid value parameter */
#define CRYPTO_E_UNINIT                         (0x04U)  /* API called before initialization */
#define CRYPTO_E_ALREADY_INITIALIZED            (0x05U)  /* Multiple initialization call */
#define CRYPTO_E_BUSY                           (0x06U)  /* Crypto hardware busy */
#define CRYPTO_E_QUEUE_FULL                     (0x07U)  /* Job queue full */
#define CRYPTO_E_KEY_NOT_AVAILABLE              (0x08U)  /* Key not available */
#define CRYPTO_E_KEY_NOT_VALID                  (0x09U)  /* Key not valid */
#define CRYPTO_E_KEY_SIZE_MISMATCH              (0x0AU)  /* Key size mismatch */
#define CRYPTO_E_SMALL_BUFFER                   (0x0BU)  /* Buffer too small */
#define CRYPTO_E_ENTROPY_EXHAUSTION             (0x0CU)  /* Entropy exhaustion */
#define CRYPTO_E_KEY_READ_FAIL                  (0x0DU)  /* Key read failure */
#define CRYPTO_E_KEY_WRITE_FAIL                 (0x0EU)  /* Key write failure */
#define CRYPTO_E_KEY_NOT_EMPTY                  (0x0FU)  /* Key element not empty */
#define CRYPTO_E_KEY_INVALID                    (0x10U)  /* Invalid key */
#define CRYPTO_E_NO_ENTROPY                     (0x11U)  /* No entropy available */

/* Runtime error codes */
#define CRYPTO_E_JOB_NOT_CANCELED               (0x20U)  /* Job cancel failed */

/*==================================================================================================
 *                                    ALGORITHM FAMILY DEFINITIONS
 *==================================================================================================*/
/**
 * @brief Algorithm family definitions for Crypto driver
 */
typedef enum {
    CRYPTO_ALGOFAM_NOT_SET = 0,          /* Algorithm not set */
    CRYPTO_ALGOFAM_AES,                  /* AES algorithm */
    CRYPTO_ALGOFAM_DES,                  /* DES algorithm */
    CRYPTO_ALGOFAM_3DES,                 /* Triple DES algorithm */
    CRYPTO_ALGOFAM_RSA,                  /* RSA algorithm */
    CRYPTO_ALGOFAM_ECC,                  /* Elliptic Curve Cryptography */
    CRYPTO_ALGOFAM_SHA1,                 /* SHA-1 hash */
    CRYPTO_ALGOFAM_SHA2_224,             /* SHA-224 hash */
    CRYPTO_ALGOFAM_SHA2_256,             /* SHA-256 hash */
    CRYPTO_ALGOFAM_SHA2_384,             /* SHA-384 hash */
    CRYPTO_ALGOFAM_SHA2_512,             /* SHA-512 hash */
    CRYPTO_ALGOFAM_MD5,                  /* MD5 hash */
    CRYPTO_ALGOFAM_HMAC,                 /* HMAC */
    CRYPTO_ALGOFAM_CMAC,                 /* CMAC */
    CRYPTO_ALGOFAM_GCM,                  /* Galois Counter Mode */
    CRYPTO_ALGOFAM_CCM,                  /* Counter with CBC-MAC */
    CRYPTO_ALGOFAM_HMAC_SHA256,          /* HMAC-SHA256 */
    CRYPTO_ALGOFAM_POLY1305              /* Poly1305 MAC */
} Crypto_AlgorithmFamilyType;

/**
 * @brief Algorithm mode definitions
 */
typedef enum {
    CRYPTO_ALGOMODE_NOT_SET = 0,         /* Mode not set */
    CRYPTO_ALGOMODE_ECB,                 /* Electronic Codebook */
    CRYPTO_ALGOMODE_CBC,                 /* Cipher Block Chaining */
    CRYPTO_ALGOMODE_CFB,                 /* Cipher Feedback */
    CRYPTO_ALGOMODE_OFB,                 /* Output Feedback */
    CRYPTO_ALGOMODE_CTR,                 /* Counter Mode */
    CRYPTO_ALGOMODE_GCM,                 /* Galois/Counter Mode */
    CRYPTO_ALGOMODE_CCM,                 /* Counter with CBC-MAC */
    CRYPTO_ALGOMODE_XTS                  /* XEX-based Tweaked CodeBook */
} Crypto_AlgorithmModeType;

/**
 * @brief Crypto operation type
 */
typedef enum {
    CRYPTO_OPERATION_NOT_SET = 0,            /* Not set */
    CRYPTO_OPERATION_ENCRYPT,                /* Encryption */
    CRYPTO_OPERATION_DECRYPT,                /* Decryption */
    CRYPTO_OPERATION_MAC_GENERATE,           /* MAC generation */
    CRYPTO_OPERATION_MAC_VERIFY,             /* MAC verification */
    CRYPTO_OPERATION_HASH,                   /* Hash calculation */
    CRYPTO_OPERATION_SIGNATURE_GENERATE,     /* Signature generation */
    CRYPTO_OPERATION_SIGNATURE_VERIFY,       /* Signature verification */
    CRYPTO_OPERATION_RANDOM_GENERATE,        /* Random number generation */
    CRYPTO_OPERATION_AEAD_ENCRYPT,           /* AEAD encryption */
    CRYPTO_OPERATION_AEAD_DECRYPT            /* AEAD decryption */
} Crypto_OperationType;

/**
 * @brief Key element types
 */
typedef enum {
    CRYPTO_KEYELEMENT_NOT_SET = 0,       /* Not set */
    CRYPTO_KEYELEMENT_AES_KEY,           /* AES key */
    CRYPTO_KEYELEMENT_IV,                /* Initialization vector */
    CRYPTO_KEYELEMENT_NONCE,             /* Nonce */
    CRYPTO_KEYELEMENT_RSA_MODULUS,       /* RSA modulus */
    CRYPTO_KEYELEMENT_RSA_PUBEXP,        /* RSA public exponent */
    CRYPTO_KEYELEMENT_RSA_PRIVTEXP,      /* RSA private exponent */
    CRYPTO_KEYELEMENT_ECC_PUBKEY_X,      /* ECC public key X */
    CRYPTO_KEYELEMENT_ECC_PUBKEY_Y,      /* ECC public key Y */
    CRYPTO_KEYELEMENT_ECC_PRIVKEY,       /* ECC private key */
    CRYPTO_KEYELEMENT_HMAC_KEY,          /* HMAC key */
    CRYPTO_KEYELEMENT_GCM_TAG,           /* GCM authentication tag */
    CRYPTO_KEYELEMENT_SALT,              /* Salt for key derivation */
    CRYPTO_KEYELEMENT_SEED               /* Seed for RNG */
} Crypto_KeyElementType;

/**
 * @brief Job state type
 */
typedef enum {
    CRYPTO_JOBSTATE_IDLE = 0,            /* Job idle */
    CRYPTO_JOBSTATE_ACTIVE,              /* Job active */
    CRYPTO_JOBSTATE_COMPLETED,           /* Job completed */
    CRYPTO_JOBSTATE_CANCELED             /* Job canceled */
} Crypto_JobStateType;

/**
 * @brief Processing mode type
 */
typedef enum {
    CRYPTO_PROCESSING_SYNC = 0,          /* Synchronous processing */
    CRYPTO_PROCESSING_ASYNC              /* Asynchronous processing */
} Crypto_ProcessingModeType;

/**
 * @brief Operation mode type (start/update/stream/finish)
 */
typedef enum {
    CRYPTO_OPERATIONMODE_START = 0x01,       /* Start operation */
    CRYPTO_OPERATIONMODE_UPDATE = 0x02,      /* Update operation */
    CRYPTO_OPERATIONMODE_STREAMSTART = 0x03, /* Stream start */
    CRYPTO_OPERATIONMODE_FINISH = 0x04,      /* Finish operation */
    CRYPTO_OPERATIONMODE_SINGLECALL = 0x07   /* Single call (start+update+finish) */
} Crypto_OperationModeType;

/**
 * @brief Verification result type
 */
typedef enum {
    CRYPTO_E_VER_OK = 0,                 /* Verification successful */
    CRYPTO_E_VER_NOT_OK,                 /* Verification failed */
    CRYPTO_E_VER_PENDING                 /* Verification pending */
} Crypto_VerifyResultType;

/**
 * @brief Driver object state type
 */
typedef enum {
    CRYPTO_DRIVER_OBJECT_STATE_IDLE = 0,     /* Driver object idle */
    CRYPTO_DRIVER_OBJECT_STATE_ACTIVE,       /* Driver object active */
    CRYPTO_DRIVER_OBJECT_STATE_LOCKED        /* Driver object locked */
} Crypto_DriverObjectStateType;

/**
 * @brief Key format type
 */
typedef enum {
    CRYPTO_KEYFORMAT_BIN = 0,            /* Binary format */
    CRYPTO_KEYFORMAT_DER,                /* DER format */
    CRYPTO_KEYFORMAT_PEM                 /* PEM format */
} Crypto_KeyFormatType;

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief Crypto key ID type
 */
typedef uint32 Crypto_KeyIdType;

/**
 * @brief Crypto key element ID type
 */
typedef uint32 Crypto_KeyElementIdType;

/**
 * @brief Crypto job ID type
 */
typedef uint32 Crypto_JobIdType;

/**
 * @brief Crypto channel ID type
 */
typedef uint32 Crypto_ChannelIdType;

/**
 * @brief Crypto driver object ID type
 */
typedef uint32 Crypto_DriverObjectIdType;

/**
 * @brief Result length type
 */
typedef uint32 Crypto_ResultLengthType;

/**
 * @brief Algorithm info structure
 */
typedef struct {
    Crypto_AlgorithmFamilyType family;       /* Algorithm family */
    Crypto_AlgorithmModeType mode;           /* Algorithm mode */
    uint32 keyLength;                        /* Key length in bits */
    uint32 ivLength;                         /* IV length in bytes */
    uint32 authTagLength;                    /* Authentication tag length in bytes */
} Crypto_AlgorithmInfoType;

/**
 * @brief Key element configuration type
 */
typedef struct {
    Crypto_KeyElementIdType keyElementId;    /* Key element ID */
    uint32 keyElementSize;                   /* Key element size in bytes */
    boolean allowPartialAccess;              /* Allow partial access */
    boolean readAccess;                      /* Read access allowed */
    boolean writeAccess;                     /* Write access allowed */
} Crypto_KeyElementConfigType;

/**
 * @brief Key configuration type
 */
typedef struct {
    Crypto_KeyIdType keyId;                              /* Key ID */
    const Crypto_KeyElementConfigType* keyElements;      /* Key elements array */
    uint16 numKeyElements;                               /* Number of key elements */
    boolean keyValid;                                    /* Key is valid */
} Crypto_KeyConfigType;

/**
 * @brief Job primitive input-output structure
 */
typedef struct {
    const uint8* inputPtr;                   /* Input data pointer */
    uint32 inputLength;                      /* Input data length */
    const uint8* secondaryInputPtr;          /* Secondary input (e.g., AAD for AEAD) */
    uint32 secondaryInputLength;             /* Secondary input length */
    uint8* outputPtr;                        /* Output data pointer */
    uint32* outputLengthPtr;                 /* Output data length pointer */
    uint8* secondaryOutputPtr;               /* Secondary output (e.g., auth tag) */
    uint32* secondaryOutputLengthPtr;        /* Secondary output length pointer */
    uint32 bufferSize;                       /* Buffer size */
    Crypto_VerifyResultType* verifyPtr;      /* Verification result pointer */
} Crypto_JobPrimitiveInputOutputType;

/**
 * @brief Job primitive info structure
 */
typedef struct {
    Crypto_OperationType primitive;          /* Primitive type */
    Crypto_AlgorithmInfoType algorithm;      /* Algorithm info */
    uint32 resultLength;                     /* Expected result length */
} Crypto_JobPrimitiveInfoType;

/**
 * @brief Job info structure
 */
typedef struct {
    Crypto_JobIdType jobId;                  /* Job ID */
    Crypto_DriverObjectIdType driverObjectId; /* Driver object ID */
    Crypto_JobPrimitiveInfoType* jobPrimitiveInfo; /* Job primitive info */
    uint32 priority;                         /* Job priority */
} Crypto_JobInfoType;

/**
 * @brief Crypto job structure
 */
typedef struct {
    Crypto_JobIdType jobId;                                  /* Job ID */
    Crypto_ChannelIdType channelId;                          /* Channel ID */
    Crypto_OperationModeType jobPrimitiveInputOutput;        /* Operation mode */
    Crypto_JobPrimitiveInputOutputType* jobPrimitiveInputOutputPtr; /* I/O pointer */
    const Crypto_JobPrimitiveInfoType* jobPrimitiveInfo;     /* Primitive info */
    const Crypto_KeyIdType* jobKeyId;                        /* Key ID pointer */
    uint32 jobState;                                         /* Job state (Crypto_JobStateType) */
    uint32 processingType;                                   /* Processing type (Crypto_ProcessingModeType) */
    void* cryptoKeyElementId;                                /* Key element ID pointer */
    void* cryptoKeyElement;                                  /* Key element pointer */
} Crypto_JobType;

/**
 * @brief Driver object configuration type
 */
typedef struct {
    Crypto_DriverObjectIdType driverObjectId;    /* Driver object ID */
    uint32 priority;                             /* Priority */
    uint32 maxJobs;                              /* Maximum concurrent jobs */
    boolean asyncMode;                           /* Asynchronous mode support */
    void (*callback)(Crypto_JobType* job, Crypto_JobStateType result); /* Callback function */
} Crypto_DriverObjectConfigType;

/**
 * @brief Hardware channel configuration type
 */
typedef struct {
    Crypto_ChannelIdType channelId;              /* Channel ID */
    Crypto_DriverObjectIdType driverObjectId;    /* Associated driver object ID */
    Crypto_AlgorithmFamilyType algorithmFamily;  /* Supported algorithm family */
    Crypto_AlgorithmModeType algorithmMode;      /* Supported algorithm mode */
    boolean hwAcceleration;                      /* Hardware acceleration enabled */
    uint32 maxKeySize;                           /* Maximum key size */
} Crypto_ChannelConfigType;

/**
 * @brief Crypto configuration type
 */
typedef struct {
    const Crypto_DriverObjectConfigType* driverObjects;  /* Driver objects array */
    uint16 numDriverObjects;                             /* Number of driver objects */
    const Crypto_ChannelConfigType* channels;            /* Channels array */
    uint16 numChannels;                                  /* Number of channels */
    const Crypto_KeyConfigType* keys;                    /* Keys array */
    uint16 numKeys;                                      /* Number of keys */
    boolean hwAccelerationEnabled;                       /* Hardware acceleration enabled */
    uint32 clockFrequency;                               /* Crypto clock frequency */
} Crypto_ConfigType;

/*==================================================================================================
 *                                    GLOBAL CONSTANTS
 *==================================================================================================*/
#define CRYPTO_INVALID_KEY_ID                   (0xFFFFFFFFU)
#define CRYPTO_INVALID_CHANNEL_ID               (0xFFFFFFFFU)
#define CRYPTO_INVALID_JOB_ID                   (0xFFFFFFFFU)
#define CRYPTO_INVALID_KEY_ELEMENT_ID           (0xFFFFFFFFU)
#define CRYPTO_INVALID_DRIVER_OBJECT_ID         (0xFFFFFFFFU)

/* AES Constants */
#define CRYPTO_AES_BLOCK_SIZE                   (16U)       /* 128 bits */
#define CRYPTO_AES_IV_SIZE                      (16U)       /* 128 bits */
#define CRYPTO_AES_KEY_SIZE_128                 (16U)       /* 128 bits */
#define CRYPTO_AES_KEY_SIZE_192                 (24U)       /* 192 bits */
#define CRYPTO_AES_KEY_SIZE_256                 (32U)       /* 256 bits */
#define CRYPTO_AES_GCM_TAG_SIZE                 (16U)       /* 128 bits */

/* SHA Constants */
#define CRYPTO_SHA1_SIZE                        (20U)       /* 160 bits */
#define CRYPTO_SHA224_SIZE                      (28U)       /* 224 bits */
#define CRYPTO_SHA256_SIZE                      (32U)       /* 256 bits */
#define CRYPTO_SHA384_SIZE                      (48U)       /* 384 bits */
#define CRYPTO_SHA512_SIZE                      (64U)       /* 512 bits */

/* HMAC Constants */
#define CRYPTO_HMAC_SHA256_SIZE                 (32U)       /* 256 bits */
#define CRYPTO_HMAC_MAX_KEY_SIZE                (64U)       /* 512 bits */

/* RSA Constants */
#define CRYPTO_RSA_KEY_SIZE_1024                (128U)      /* 1024 bits */
#define CRYPTO_RSA_KEY_SIZE_2048                (256U)      /* 2048 bits */
#define CRYPTO_RSA_KEY_SIZE_4096                (512U)      /* 4096 bits */

/* ECC Constants */
#define CRYPTO_ECC_P256_KEY_SIZE                (32U)       /* 256 bits */
#define CRYPTO_ECC_P384_KEY_SIZE                (48U)       /* 384 bits */
#define CRYPTO_ECC_P521_KEY_SIZE                (66U)       /* 521 bits */

/* RNG Constants */
#define CRYPTO_TRNG_MIN_ENTROPY                 (128U)      /* Minimum entropy bits */
#define CRYPTO_TRNG_MAX_REQUEST_SIZE            (256U)      /* Max random bytes per request */

/* Job Queue Constants */
#define CRYPTO_MAX_JOBS                         (16U)       /* Maximum jobs in queue */
#define CRYPTO_MAX_CHANNELS                     (8U)        /* Maximum channels */
#define CRYPTO_MAX_KEYS                         (32U)       /* Maximum keys */
#define CRYPTO_MAX_KEY_ELEMENTS                 (10U)       /* Maximum key elements per key */

/*==================================================================================================
 *                                    CALLBACK TYPE
 *==================================================================================================*/
/**
 * @brief Job notification callback type
 */
typedef void (*Crypto_NotificationCallbackType)(const Crypto_JobType* job,
                                                 Std_ReturnType result,
                                                 Crypto_JobStateType jobState);

/*==================================================================================================
 *                                    API DECLARATIONS
 *==================================================================================================*/
#define CRYPTO_START_SEC_CODE
#include "Crypto_MemMap.h"

/**
 * @brief Initializes the Crypto Driver module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_Crypto_00001
 */
extern void Crypto_Init(const Crypto_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Crypto Driver module
 * @return None
 * @req SWS_Crypto_00002
 */
extern void Crypto_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Crypto_00003
 */
#if (CRYPTO_VERSION_INFO_API == STD_ON)
extern void Crypto_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Processes a crypto job
 * @param objectId Driver object ID
 * @param job Pointer to the job structure
 * @return E_OK: Job started or queued successfully
 *         E_NOT_OK: Job could not be started or queued
 *         CRYPTO_E_BUSY: Driver object busy
 * @req SWS_Crypto_00100
 */
extern Std_ReturnType Crypto_ProcessJob(Crypto_DriverObjectIdType objectId,
                                         Crypto_JobType* job);

/**
 * @brief Cancels a pending crypto job
 * @param objectId Driver object ID
 * @param job Pointer to the job structure
 * @return E_OK: Job canceled successfully
 *         E_NOT_OK: Job could not be canceled
 * @req SWS_Crypto_00101
 */
extern Std_ReturnType Crypto_CancelJob(Crypto_DriverObjectIdType objectId,
                                        const Crypto_JobType* job);

/*==================================================================================================
 *                                    KEY MANAGEMENT APIs
 *==================================================================================================*/

/**
 * @brief Sets key element data
 * @param cryptoKeyId Key ID
 * @param keyElementId Key element ID
 * @param keyPtr Key data pointer
 * @param keyLength Key data length
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00200
 */
extern Std_ReturnType Crypto_KeyElementSet(Crypto_KeyIdType cryptoKeyId,
                                            Crypto_KeyElementIdType keyElementId,
                                            const uint8* keyPtr,
                                            uint32 keyLength);

/**
 * @brief Gets key element data
 * @param cryptoKeyId Key ID
 * @param keyElementId Key element ID
 * @param keyPtr Output buffer pointer
 * @param keyLengthPtr Length pointer
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00210
 */
extern Std_ReturnType Crypto_KeyElementGet(Crypto_KeyIdType cryptoKeyId,
                                            Crypto_KeyElementIdType keyElementId,
                                            uint8* keyPtr,
                                            uint32* keyLengthPtr);

/**
 * @brief Copies key element from source to target
 * @param cryptoKeyId Source key ID
 * @param keyElementId Source key element ID
 * @param targetCryptoKeyId Target key ID
 * @param targetKeyElementId Target key element ID
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00220
 */
extern Std_ReturnType Crypto_KeyElementCopy(Crypto_KeyIdType cryptoKeyId,
                                             Crypto_KeyElementIdType keyElementId,
                                             Crypto_KeyIdType targetCryptoKeyId,
                                             Crypto_KeyElementIdType targetKeyElementId);

/**
 * @brief Gets all key element IDs for a key
 * @param cryptoKeyId Key ID
 * @param keyElementIdsPtr Buffer for key element IDs
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00230
 */
extern Std_ReturnType Crypto_KeyElementIdsGet(Crypto_KeyIdType cryptoKeyId,
                                               Crypto_KeyElementIdType* keyElementIdsPtr);

/**
 * @brief Copies key (all elements) from source to target
 * @param cryptoKeyId Source key ID
 * @param targetCryptoKeyId Target key ID
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00240
 */
extern Std_ReturnType Crypto_KeyCopy(Crypto_KeyIdType cryptoKeyId,
                                      Crypto_KeyIdType targetCryptoKeyId);

/**
 * @brief Sets key as valid
 * @param cryptoKeyId Key ID
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00250
 */
extern Std_ReturnType Crypto_KeyValidSet(Crypto_KeyIdType cryptoKeyId);

/**
 * @brief Generates a key
 * @param cryptoKeyId Key ID to generate
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00300
 */
extern Std_ReturnType Crypto_KeyGenerate(Crypto_KeyIdType cryptoKeyId);

/**
 * @brief Derives a key from another key
 * @param cryptoKeyId Source key ID
 * @param targetCryptoKeyId Target key ID
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00310
 */
extern Std_ReturnType Crypto_KeyDerive(Crypto_KeyIdType cryptoKeyId,
                                        Crypto_KeyIdType targetCryptoKeyId);

/**
 * @brief Calculates public value for key exchange
 * @param cryptoKeyId Key ID
 * @param publicValuePtr Public value output buffer
 * @param publicValueLengthPtr Public value length pointer
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00400
 */
extern Std_ReturnType Crypto_KeyExchangeCalcPubVal(Crypto_KeyIdType cryptoKeyId,
                                                    uint8* publicValuePtr,
                                                    uint32* publicValueLengthPtr);

/**
 * @brief Sets partner public value for key exchange
 * @param cryptoKeyId Key ID
 * @param partnerPublicValuePtr Partner's public value
 * @param partnerPublicValueLength Partner's public value length
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00410
 */
extern Std_ReturnType Crypto_KeyExchangeSetPubVal(Crypto_KeyIdType cryptoKeyId,
                                                   const uint8* partnerPublicValuePtr,
                                                   uint32 partnerPublicValueLength);

/**
 * @brief Calculates shared secret
 * @param cryptoKeyId Key ID
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00420
 */
extern Std_ReturnType Crypto_KeyExchangeCalcSecret(Crypto_KeyIdType cryptoKeyId);

/*==================================================================================================
 *                                    CERTIFICATE APIs
 *==================================================================================================*/

/**
 * @brief Parses a certificate
 * @param cryptoKeyId Certificate key ID
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00500
 */
extern Std_ReturnType Crypto_CertificateParse(Crypto_KeyIdType cryptoKeyId);

/**
 * @brief Verifies a certificate
 * @param cryptoKeyId Certificate key ID
 * @param verifyCryptoKeyId Verification key ID
 * @param verifyPtr Verification result pointer
 * @return E_OK: Success
 *         E_NOT_OK: Operation failed
 * @req SWS_Crypto_00510
 */
extern Std_ReturnType Crypto_CertificateVerify(Crypto_KeyIdType cryptoKeyId,
                                                Crypto_KeyIdType verifyCryptoKeyId,
                                                Crypto_VerifyResultType* verifyPtr);

/*==================================================================================================
 *                                    HARDWARE ABSTRCTION APIs
 *==================================================================================================*/

/**
 * @brief Encrypts data using hardware AES
 * @param channelId Channel ID
 * @param mode Operation mode (start/update/finish/singlecall)
 * @param algorithm Algorithm (AES-CBC/ECB/CTR/GCM)
 * @param keyId Key ID
 * @param ivPtr Initialization vector (for CBC/CTR/GCM)
 * @param plaintextPtr Plaintext input
 * @param plaintextLength Plaintext length
 * @param ciphertextPtr Ciphertext output buffer
 * @param ciphertextLengthPtr Ciphertext length pointer
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwAesEncrypt(Crypto_ChannelIdType channelId,
                                           Crypto_OperationModeType mode,
                                           const Crypto_AlgorithmInfoType* algorithm,
                                           Crypto_KeyIdType keyId,
                                           const uint8* ivPtr,
                                           const uint8* plaintextPtr,
                                           uint32 plaintextLength,
                                           uint8* ciphertextPtr,
                                           uint32* ciphertextLengthPtr);

/**
 * @brief Decrypts data using hardware AES
 * @param channelId Channel ID
 * @param mode Operation mode (start/update/finish/singlecall)
 * @param algorithm Algorithm (AES-CBC/ECB/CTR/GCM)
 * @param keyId Key ID
 * @param ivPtr Initialization vector (for CBC/CTR/GCM)
 * @param ciphertextPtr Ciphertext input
 * @param ciphertextLength Ciphertext length
 * @param plaintextPtr Plaintext output buffer
 * @param plaintextLengthPtr Plaintext length pointer
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwAesDecrypt(Crypto_ChannelIdType channelId,
                                           Crypto_OperationModeType mode,
                                           const Crypto_AlgorithmInfoType* algorithm,
                                           Crypto_KeyIdType keyId,
                                           const uint8* ivPtr,
                                           const uint8* ciphertextPtr,
                                           uint32 ciphertextLength,
                                           uint8* plaintextPtr,
                                           uint32* plaintextLengthPtr);

/**
 * @brief Calculates hash using hardware SHA-256
 * @param channelId Channel ID
 * @param mode Operation mode
 * @param dataPtr Input data
 * @param dataLength Input length
 * @param hashPtr Hash output buffer
 * @param hashLengthPtr Hash length pointer
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwHashSha256(Crypto_ChannelIdType channelId,
                                           Crypto_OperationModeType mode,
                                           const uint8* dataPtr,
                                           uint32 dataLength,
                                           uint8* hashPtr,
                                           uint32* hashLengthPtr);

/**
 * @brief Generates HMAC using hardware
 * @param channelId Channel ID
 * @param mode Operation mode
 * @param keyId Key ID
 * @param dataPtr Input data
 * @param dataLength Input length
 * @param macPtr MAC output buffer
 * @param macLengthPtr MAC length pointer
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwHmacGenerate(Crypto_ChannelIdType channelId,
                                             Crypto_OperationModeType mode,
                                             Crypto_KeyIdType keyId,
                                             const uint8* dataPtr,
                                             uint32 dataLength,
                                             uint8* macPtr,
                                             uint32* macLengthPtr);

/**
 * @brief Verifies HMAC using hardware
 * @param channelId Channel ID
 * @param mode Operation mode
 * @param keyId Key ID
 * @param dataPtr Input data
 * @param dataLength Input length
 * @param macPtr MAC to verify
 * @param macLength MAC length
 * @param verifyPtr Verification result
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwHmacVerify(Crypto_ChannelIdType channelId,
                                           Crypto_OperationModeType mode,
                                           Crypto_KeyIdType keyId,
                                           const uint8* dataPtr,
                                           uint32 dataLength,
                                           const uint8* macPtr,
                                           uint32 macLength,
                                           Crypto_VerifyResultType* verifyPtr);

/**
 * @brief Generates RSA signature using hardware
 * @param channelId Channel ID
 * @param mode Operation mode
 * @param keyId Key ID (private key)
 * @param dataPtr Data to sign
 * @param dataLength Data length
 * @param signaturePtr Signature output buffer
 * @param signatureLengthPtr Signature length pointer
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwRsaSign(Crypto_ChannelIdType channelId,
                                        Crypto_OperationModeType mode,
                                        Crypto_KeyIdType keyId,
                                        const uint8* dataPtr,
                                        uint32 dataLength,
                                        uint8* signaturePtr,
                                        uint32* signatureLengthPtr);

/**
 * @brief Verifies RSA signature using hardware
 * @param channelId Channel ID
 * @param mode Operation mode
 * @param keyId Key ID (public key)
 * @param dataPtr Data that was signed
 * @param dataLength Data length
 * @param signaturePtr Signature to verify
 * @param signatureLength Signature length
 * @param verifyPtr Verification result
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwRsaVerify(Crypto_ChannelIdType channelId,
                                          Crypto_OperationModeType mode,
                                          Crypto_KeyIdType keyId,
                                          const uint8* dataPtr,
                                          uint32 dataLength,
                                          const uint8* signaturePtr,
                                          uint32 signatureLength,
                                          Crypto_VerifyResultType* verifyPtr);

/**
 * @brief Generates random numbers using hardware TRNG
 * @param resultPtr Output buffer
 * @param resultLength Requested random length
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwRandomGenerate(uint8* resultPtr, uint32 resultLength);

/**
 * @brief Seeds the hardware TRNG
 * @param seedPtr Seed data pointer
 * @param seedLength Seed length
 * @return E_OK: Success, E_NOT_OK: Failed
 */
extern Std_ReturnType Crypto_HwRandomSeed(const uint8* seedPtr, uint32 seedLength);

/*==================================================================================================
 *                                    MAIN FUNCTION
 *==================================================================================================*/
/**
 * @brief Main function for processing async jobs
 * @return None
 * @req SWS_Crypto_01000
 */
extern void Crypto_MainFunction(void);

#define CRYPTO_STOP_SEC_CODE
#include "Crypto_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_H */
