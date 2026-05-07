/*==================================================================================================
 *                                CRYPTO SERVICES MANAGER (Csm)
 *==================================================================================================
 * FILENAME: Csm.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_CryptoServicesManager.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Crypto Services Manager module
 *==================================================================================================
 */

#ifndef CSM_H
#define CSM_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "Csm_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define CSM_VENDOR_ID                   (100u)
#define CSM_MODULE_ID                   (110u)
#define CSM_INSTANCE_ID                 (0u)

#define CSM_AR_RELEASE_MAJOR_VERSION    (4u)
#define CSM_AR_RELEASE_MINOR_VERSION    (7u)
#define CSM_AR_RELEASE_REVISION_VERSION (0u)

#define CSM_SW_MAJOR_VERSION            (1u)
#define CSM_SW_MINOR_VERSION            (0u)
#define CSM_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((CSM_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (CSM_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of Csm.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define CSM_SID_INIT                        (0x00u)
#define CSM_SID_DEINIT                      (0x01u)
#define CSM_SID_GETVERSIONINFO              (0x02u)
#define CSM_SID_ENCRYPT                     (0x10u)
#define CSM_SID_DECRYPT                     (0x11u)
#define CSM_SID_MACGENERATE                 (0x20u)
#define CSM_SID_MACVERIFY                   (0x21u)
#define CSM_SID_HASH                        (0x30u)
#define CSM_SID_SIGNATUREGENERATE           (0x40u)
#define CSM_SID_SIGNATUREVERIFY             (0x41u)
#define CSM_SID_RANDOMGENERATE              (0x50u)
#define CSM_SID_KEYSETVALID                 (0x60u)
#define CSM_SID_KEYELEMENTSET               (0x61u)
#define CSM_SID_KEYELEMENTGET               (0x62u)
#define CSM_SID_KEYEXCHANGECALCPUBVAL       (0x70u)
#define CSM_SID_KEYEXCHANGECALCSECRET       (0x71u)
#define CSM_SID_CALLBACKNOTIFICATION        (0x80u)
#define CSM_SID_MAINFUNCTION                (0x90u)
#define CSM_SID_CANCELJOB                   (0xA0u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define CSM_E_PARAM_POINTER                 (0x01u)  /* API called with NULL pointer */
#define CSM_E_PARAM_HANDLE                  (0x02u)  /* Invalid configuration handle */
#define CSM_E_PARAM_LENGTH                  (0x03u)  /* Invalid length parameter */
#define CSM_E_UNINIT                        (0x04u)  /* API called before initialization */
#define CSM_E_ALREADY_INITIALIZED           (0x05u)  /* Multiple initialization call */
#define CSM_E_INVALID_JOB                   (0x06u)  /* Invalid job ID */
#define CSM_E_INVALID_KEY                   (0x07u)  /* Invalid key ID */
#define CSM_E_INVALID_CRYPTO_OPERATION      (0x08u)  /* Invalid crypto operation */

/* Runtime error codes */
#define CSM_E_BUSY                          (0x01u)  /* Crypto operation busy */
#define CSM_E_QUEUE_FULL                    (0x02u)  /* Job queue full */
#define CSM_E_KEY_NOT_AVAILABLE             (0x03u)  /* Key not available */
#define CSM_E_KEY_NOT_VALID                 (0x04u)  /* Key not valid */
#define CSM_E_ENTROPY_EXHAUSTION            (0x05u)  /* Entropy exhaustion */

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief Csm configuration ID type
 */
typedef uint16 Csm_ConfigIdType;

/**
 * @brief Job ID type
 */
typedef uint32 Csm_JobIdType;

/**
 * @brief Key ID type
 */
typedef uint32 Csm_KeyIdType;

/**
 * @brief Crypto operation mode type
 */
typedef enum {
    CSM_OPERATIONMODE_START = 0x01,     /* Start operation */
    CSM_OPERATIONMODE_UPDATE = 0x02,    /* Update operation */
    CSM_OPERATIONMODE_FINISH = 0x04,    /* Finish operation */
    CSM_OPERATIONMODE_STREAMSTART = 0x07 /* Single call (start+update+finish) */
} Csm_OperationModeType;

/**
 * @brief Job state type
 */
typedef enum {
    CSM_JOB_STATE_IDLE = 0,             /* Job idle */
    CSM_JOB_STATE_ACTIVE,               /* Job active */
    CSM_JOB_STATE_PROGRESSING,          /* Job in progress */
    CSM_JOB_STATE_COMPLETED,            /* Job completed */
    CSM_JOB_STATE_FAILED                /* Job failed */
} Csm_JobStateType;

/**
 * @brief Job primitive information type
 */
typedef enum {
    CSM_CRYPTO_PRIMITIVE_ENCRYPT = 0,   /* Encryption */
    CSM_CRYPTO_PRIMITIVE_DECRYPT,       /* Decryption */
    CSM_CRYPTO_PRIMITIVE_MAC_GENERATE,  /* MAC generation */
    CSM_CRYPTO_PRIMITIVE_MAC_VERIFY,    /* MAC verification */
    CSM_CRYPTO_PRIMITIVE_HASH,          /* Hash calculation */
    CSM_CRYPTO_PRIMITIVE_SIGNATURE_GENERATE, /* Signature generation */
    CSM_CRYPTO_PRIMITIVE_SIGNATURE_VERIFY,   /* Signature verification */
    CSM_CRYPTO_PRIMITIVE_RANDOM_GENERATE     /* Random number generation */
} Csm_CryptoPrimitiveType;

/**
 * @brief Algorithm family type
 */
typedef enum {
    CSM_ALGOFAM_AES = 0,                /* AES algorithm */
    CSM_ALGOFAM_DES,                    /* DES algorithm */
    CSM_ALGOFAM_3DES,                   /* 3DES algorithm */
    CSM_ALGOFAM_RSA,                    /* RSA algorithm */
    CSM_ALGOFAM_ECC,                    /* ECC algorithm */
    CSM_ALGOFAM_SHA1,                   /* SHA-1 hash */
    CSM_ALGOFAM_SHA2_224,               /* SHA-224 hash */
    CSM_ALGOFAM_SHA2_256,               /* SHA-256 hash */
    CSM_ALGOFAM_SHA2_384,               /* SHA-384 hash */
    CSM_ALGOFAM_SHA2_512,               /* SHA-512 hash */
    CSM_ALGOFAM_MD5                     /* MD5 hash */
} Csm_AlgorithmFamilyType;

/**
 * @brief Algorithm mode type
 */
typedef enum {
    CSM_ALGOMODE_ECB = 0,               /* Electronic Codebook */
    CSM_ALGOMODE_CBC,                   /* Cipher Block Chaining */
    CSM_ALGOMODE_CFB,                   /* Cipher Feedback */
    CSM_ALGOMODE_OFB,                   /* Output Feedback */
    CSM_ALGOMODE_CTR,                   /* Counter Mode */
    CSM_ALGOMODE_GCM,                   /* Galois/Counter Mode */
    CSM_ALGOMODE_CCM                    /* Counter with CBC-MAC */
} Csm_AlgorithmModeType;

/**
 * @brief Crypto job primitive info type
 */
typedef struct {
    Csm_CryptoPrimitiveType primitiveType;
    Csm_AlgorithmFamilyType algorithmFamily;
    Csm_AlgorithmModeType algorithmMode;
    uint32 keyLength;                   /* Key length in bits */
    uint32 resultLength;                /* Expected result length */
} Csm_CryptoJobPrimitiveInfoType;

/**
 * @brief Crypto job info type
 */
typedef struct {
    Csm_JobIdType jobId;
    Csm_JobStateType jobState;
    Csm_CryptoJobPrimitiveInfoType* primitiveInfo;
    Csm_KeyIdType keyId;
    boolean callbackActive;
} Csm_CryptoJobInfoType;

/**
 * @brief Input/output data structure for crypto operations
 */
typedef struct {
    uint8* inputPtr;                    /* Input data pointer */
    uint32 inputLength;                 /* Input data length */
    uint8* outputPtr;                   /* Output data pointer */
    uint32* outputLengthPtr;            /* Output length pointer */
    uint8* secondaryInputPtr;           /* Secondary input (e.g., IV) */
    uint32 secondaryInputLength;
    uint8* tertiaryInputPtr;            /* Tertiary input (e.g., auth data) */
    uint32 tertiaryInputLength;
    uint8* verifyPtr;                   /* Verification data (for MAC/signature verify) */
    uint32 verifyLength;
} Csm_JobPrimitiveInputOutputType;

/**
 * @brief Job configuration type
 */
typedef struct {
    Csm_JobIdType jobId;
    Csm_CryptoPrimitiveType primitiveType;
    Csm_AlgorithmFamilyType algorithmFamily;
    Csm_AlgorithmModeType algorithmMode;
    Csm_KeyIdType keyId;
    uint32 priority;                    /* Job priority (0 = lowest) */
    boolean callbackActive;             /* Callback notification enabled */
} Csm_JobConfigType;

/**
 * @brief Key configuration type
 */
typedef struct {
    Csm_KeyIdType keyId;
    uint32 keyLength;
    boolean keyValid;
} Csm_KeyConfigType;

/**
 * @brief Queue element type
 */
typedef struct {
    Csm_JobIdType jobId;
    Csm_OperationModeType mode;
    Csm_JobPrimitiveInputOutputType* inputOutput;
    boolean inUse;
} Csm_QueueElementType;

/**
 * @brief Csm configuration type
 */
typedef struct {
    const Csm_JobConfigType* jobConfigs;
    uint16 numJobs;
    const Csm_KeyConfigType* keyConfigs;
    uint16 numKeys;
    uint16 queueSize;
    boolean callbackSupported;
    boolean retryFailedJobs;
} Csm_ConfigType;

/*==================================================================================================
 *                                    GLOBAL CONSTANTS
 *==================================================================================================*/
#define CSM_INVALID_JOB_ID              (0xFFFFFFFFu)
#define CSM_INVALID_KEY_ID              (0xFFFFFFFFu)
#define CSM_MAX_QUEUE_SIZE              (16u)

/*==================================================================================================
 *                                    CALLBACK TYPE
 *==================================================================================================*/
/**
 * @brief Job notification callback type
 */
typedef void (*Csm_CallbackType)(const Csm_JobIdType jobId, 
                                  Csm_JobStateType jobState,
                                  const uint8* resultPtr,
                                  uint32 resultLength);

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

extern boolean Csm_Initialized;
extern const Csm_ConfigType* Csm_ConfigPtr;

#define CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

/**
 * @brief Verification result type
 */
typedef enum {
    CSM_E_VER_OK = 0,
    CSM_E_VER_NOT_OK
} Csm_VerifyResultType;

/*==================================================================================================
 *                                    API DECLARATIONS
 *==================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

/**
 * @brief Initializes the Crypto Services Manager module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_Csm_00001
 */
extern void Csm_Init(const Csm_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Crypto Services Manager module
 * @return None
 * @req SWS_Csm_00002
 */
extern void Csm_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Csm_00003
 */
#if (CSM_VERSION_INFO_API == STD_ON)
extern void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Encrypts data
 * @param jobId Job ID for encryption
 * @param mode Operation mode (start/update/finish)
 * @param dataPtr Input data pointer
 * @param dataLength Input data length
 * @param resultPtr Output buffer pointer
 * @param resultLengthPtr Output length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00100
 */
extern Std_ReturnType Csm_Encrypt(Csm_JobIdType jobId,
                                   Csm_OperationModeType mode,
                                   const uint8* dataPtr,
                                   uint32 dataLength,
                                   uint8* resultPtr,
                                   uint32* resultLengthPtr);

/**
 * @brief Decrypts data
 * @param jobId Job ID for decryption
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input data length
 * @param resultPtr Output buffer pointer
 * @param resultLengthPtr Output length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00110
 */
extern Std_ReturnType Csm_Decrypt(Csm_JobIdType jobId,
                                   Csm_OperationModeType mode,
                                   const uint8* dataPtr,
                                   uint32 dataLength,
                                   uint8* resultPtr,
                                   uint32* resultLengthPtr);

/**
 * @brief Generates MAC (Message Authentication Code)
 * @param jobId Job ID for MAC generation
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input data length
 * @param macPtr MAC output buffer
 * @param macLengthPtr MAC length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00200
 */
extern Std_ReturnType Csm_MacGenerate(Csm_JobIdType jobId,
                                       Csm_OperationModeType mode,
                                       const uint8* dataPtr,
                                       uint32 dataLength,
                                       uint8* macPtr,
                                       uint32* macLengthPtr);

/**
 * @brief Verifies MAC
 * @param jobId Job ID for MAC verification
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input data length
 * @param macPtr MAC to verify
 * @param macLength MAC length
 * @param verifyPtr Verification result pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00210
 */
extern Std_ReturnType Csm_MacVerify(Csm_JobIdType jobId,
                                     Csm_OperationModeType mode,
                                     const uint8* dataPtr,
                                     uint32 dataLength,
                                     const uint8* macPtr,
                                     uint32 macLength,
                                     Csm_VerifyResultType* verifyPtr);

/**
 * @brief Calculates hash
 * @param jobId Job ID for hash calculation
 * @param mode Operation mode
 * @param dataPtr Input data pointer
 * @param dataLength Input data length
 * @param resultPtr Hash output buffer
 * @param resultLengthPtr Hash length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00300
 */
extern Std_ReturnType Csm_Hash(Csm_JobIdType jobId,
                                Csm_OperationModeType mode,
                                const uint8* dataPtr,
                                uint32 dataLength,
                                uint8* resultPtr,
                                uint32* resultLengthPtr);

/**
 * @brief Generates random number
 * @param jobId Job ID for random generation
 * @param resultPtr Output buffer
 * @param resultLength Requested random length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00500
 */
extern Std_ReturnType Csm_RandomGenerate(Csm_JobIdType jobId,
                                          uint8* resultPtr,
                                          uint32 resultLength);

/**
 * @brief Sets key element
 * @param keyId Key ID
 * @param keyElementId Key element ID
 * @param keyPtr Key data pointer
 * @param keyLength Key length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00600
 */
extern Std_ReturnType Csm_KeyElementSet(Csm_KeyIdType keyId,
                                         uint32 keyElementId,
                                         const uint8* keyPtr,
                                         uint32 keyLength);

/**
 * @brief Sets key as valid
 * @param keyId Key ID
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00610
 */
extern Std_ReturnType Csm_KeySetValid(Csm_KeyIdType keyId);

/**
 * @brief Gets key element
 * @param keyId Key ID
 * @param keyElementId Key element ID
 * @param keyPtr Output buffer
 * @param keyLengthPtr Length pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00620
 */
extern Std_ReturnType Csm_KeyElementGet(Csm_KeyIdType keyId,
                                         uint32 keyElementId,
                                         uint8* keyPtr,
                                         uint32* keyLengthPtr);

/**
 * @brief Cancels a job
 * @param jobId Job ID to cancel
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_Csm_00900
 */
extern Std_ReturnType Csm_CancelJob(Csm_JobIdType jobId);

/**
 * @brief Main function for job processing
 * @return None
 * @req SWS_Csm_00910
 */
extern void Csm_MainFunction(void);

#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* CSM_H */
