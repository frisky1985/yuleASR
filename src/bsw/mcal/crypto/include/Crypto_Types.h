/**********************************************************************************************************************
 * @file       Crypto_Types.h
 * @brief      Crypto Driver Type Definitions
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#ifndef CRYPTO_TYPES_H
#define CRYPTO_TYPES_H

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Std_Types.h"

/**********************************************************************************************************************
 * GLOBAL CONSTANT MACROS
 *********************************************************************************************************************/
#define CRYPTO_VENDOR_ID                    (0x2025U)
#define CRYPTO_MODULE_ID                    (110U)
#define CRYPTO_SW_MAJOR_VERSION             (1U)
#define CRYPTO_SW_MINOR_VERSION             (0U)
#define CRYPTO_SW_PATCH_VERSION             (0U)

/**********************************************************************************************************************
 * ERROR CODES
 *********************************************************************************************************************/
#define CRYPTO_E_NO_ERROR                   (0x00U)
#define CRYPTO_E_BUSY                       (0x01U)
#define CRYPTO_E_SMALL_BUFFER               (0x02U)
#define CRYPTO_E_ENTROPY_EXHAUSTION         (0x03U)
#define CRYPTO_E_QUEUE_FULL                 (0x04U)
#define CRYPTO_E_JOB_CANCELED               (0x05U)
#define CRYPTO_E_KEY_NOT_VALID              (0x06U)
#define CRYPTO_E_KEY_SIZE_MISMATCH          (0x07U)
#define CRYPTO_E_COUNTER_OVERFLOW           (0x08U)
#define CRYPTO_E_NOT_SUPPORTED              (0x09U)
#define CRYPTO_E_KEY_READ_FAIL              (0x0AU)
#define CRYPTO_E_KEY_WRITE_FAIL             (0x0BU)
#define CRYPTO_E_KEY_NOT_AVAILABLE          (0x0CU)
#define CRYPTO_E_KEY_NOT_VALID_STATE        (0x0DU)
#define CRYPTO_E_KEY_INVALID                (0x0EU)
#define CRYPTO_E_HSM_GENERAL_ERROR          (0x10U)
#define CRYPTO_E_HSM_NOT_RESPONDING         (0x11U)
#define CRYPTO_E_HSM_BUFFER_TOO_SMALL       (0x12U)
#define CRYPTO_E_HSM_FATAL_ERROR            (0x13U)

/**********************************************************************************************************************
 * CRYPTO ALGORITHM MACROS
 *********************************************************************************************************************/
/* Key Exchange Algorithms */
#define CRYPTO_ALGOFAM_ECIES                (0x01U)
#define CRYPTO_ALGOFAM_ECDH                 (0x02U)
#define CRYPTO_ALGOFAM_DH                   (0x03U)

/* Signature Algorithms */
#define CRYPTO_ALGOFAM_ECC                  (0x0FU)
#define CRYPTO_ALGOFAM_ECDSA                (0x10U)
#define CRYPTO_ALGOFAM_RSA                  (0x11U)
#define CRYPTO_ALGOFAM_EDDSA                (0x12U)

/* Symmetric Encryption */
#define CRYPTO_ALGOFAM_AES                  (0x20U)
#define CRYPTO_ALGOFAM_CHACHA               (0x21U)

/* Block Cipher Modes */
#define CRYPTO_ALGOMODE_NOT_SET             (0xFFU)
#define CRYPTO_ALGOMODE_ECB                 (0x00U)
#define CRYPTO_ALGOMODE_CBC                 (0x01U)
#define CRYPTO_ALGOMODE_CFB                 (0x02U)
#define CRYPTO_ALGOMODE_OFB                 (0x03U)
#define CRYPTO_ALGOMODE_CTR                 (0x04U)
#define CRYPTO_ALGOMODE_GCM                 (0x05U)
#define CRYPTO_ALGOMODE_CCM                 (0x06U)

/* Hash Algorithms */
#define CRYPTO_ALGOFAM_SHA2_256             (0x30U)
#define CRYPTO_ALGOFAM_SHA2_384             (0x31U)
#define CRYPTO_ALGOFAM_SHA2_512             (0x32U)
#define CRYPTO_ALGOFAM_SHA3_256             (0x33U)
#define CRYPTO_ALGOFAM_SHA3_384             (0x34U)
#define CRYPTO_ALGOFAM_SHA3_512             (0x35U)

/* MAC Algorithms */
#define CRYPTO_ALGOFAM_HMAC                 (0x40U)
#define CRYPTO_ALGOFAM_HMAC_SHA256          (0x41U)
#define CRYPTO_ALGOFAM_NOT_SET              (0x00U)
#define CRYPTO_ALGOFAM_CMAC                 (0x41U)
#define CRYPTO_ALGOFAM_GMAC                 (0x42U)

/* Key Derivation */
#define CRYPTO_ALGOFAM_HKDF                 (0x50U)
#define CRYPTO_ALGOFAM_PBKDF2               (0x51U)

/* ECC Curves */
#define CRYPTO_EC_CURVE_SECP256R1           (0x00U)
#define CRYPTO_EC_CURVE_SECP384R1           (0x01U)
#define CRYPTO_EC_CURVE_SECP521R1           (0x02U)
#define CRYPTO_EC_CURVE_SECP256K1           (0x03U)
#define CRYPTO_EC_CURVE_BRAINPOOLP256R1     (0x10U)
#define CRYPTO_EC_CURVE_BRAINPOOLP384R1     (0x11U)
#define CRYPTO_EC_CURVE_BRAINPOOLP512R1     (0x12U)

/**********************************************************************************************************************
 * CRYPTO OPERATION MACROS
 *********************************************************************************************************************/
#define CRYPTO_OPERATIONMODE_START          (0x01U)
#define CRYPTO_OPERATIONMODE_UPDATE         (0x02U)
#define CRYPTO_OPERATIONMODE_STREAMSTART    (0x03U)
#define CRYPTO_OPERATIONMODE_FINISH         (0x04U)
#define CRYPTO_OPERATIONMODE_SINGLECALL     (0x07U)

#define CRYPTO_KEYSETELEMENT_KDF_SALT       (0x01U)
#define CRYPTO_KEYSETELEMENT_KDF_INFO       (0x02U)
#define CRYPTO_KEYSETELEMENT_IV             (0x03U)
#define CRYPTO_KEYSETELEMENT_AUTH_DATA      (0x04U)
#define CRYPTO_KEYSETELEMENT_TAG            (0x05U)

/**********************************************************************************************************************
 * KEY MANAGEMENT MACROS
 *********************************************************************************************************************/
#define CRYPTO_KEY_ELEMENT_IV               (1U)
#define CRYPTO_KEY_ELEMENT_SALT             (2U)
#define CRYPTO_KEY_ELEMENT_SEED             (3U)
#define CRYPTO_KEY_ELEMENT_DIGEST           (4U)
#define CRYPTO_KEY_ELEMENT_TAG              (5U)
#define CRYPTO_KEY_ELEMENT_KEY              (10U)
#define CRYPTO_KEY_ELEMENT_PRIVATE_KEY      (11U)
#define CRYPTO_KEY_ELEMENT_PUBLIC_KEY       (12U)
#define CRYPTO_KEY_ELEMENT_SIGNATURE        (20U)

#define CRYPTO_KEY_WRITE_ACCESS_ALLOWED     (0x00U)
#define CRYPTO_KEY_WRITE_ACCESS_DENIED      (0x01U)

#define CRYPTO_KEY_VALID                    (0x01U)
#define CRYPTO_KEY_INVALID                  (0x00U)

#define CRYPTO_KEYSTATE_VALID               (0x00U)
#define CRYPTO_KEYSTATE_INVALID             (0x01U)
#define CRYPTO_KEYSTATE_EMPTY               (0x02U)

/**********************************************************************************************************************
 * SERVICE INFORMATION MACROS
 *********************************************************************************************************************/
#define CRYPTO_SERVICE_ENCRYPT              (0x00U)
#define CRYPTO_SERVICE_DECRYPT              (0x01U)
#define CRYPTO_SERVICE_HASH                 (0x02U)
#define CRYPTO_SERVICE_MACGENERATE          (0x03U)
#define CRYPTO_SERVICE_MACVERIFY            (0x04U)
#define CRYPTO_SERVICE_SIGN                 (0x05U)
#define CRYPTO_SERVICE_VERIFY               (0x06U)
#define CRYPTO_SERVICE_RANDOMGENERATE       (0x07U)
#define CRYPTO_SERVICE_KEYEXCHANGECALCSECRET (0x08U)
#define CRYPTO_SERVICE_KEYDERIVE            (0x09U)

/**********************************************************************************************************************
 * CRYPTO OBJECT TYPES
 *********************************************************************************************************************/

/* Verification results */
#ifndef CRYPTO_VERIFY_PASSED
#define CRYPTO_VERIFY_PASSED                (0x00000001U)
#define CRYPTO_VERIFY_FAILED                (0x00000002U)
#endif
typedef uint32 Crypto_AlgorithmFamilyType;
typedef uint32 Crypto_AlgorithmModeType;
typedef uint32 Crypto_OperationModeType;
typedef uint32 Crypto_ServiceInfoType;
typedef uint32 Crypto_VerifyResultType;

typedef uint32 Crypto_JobIdType;
typedef uint32 Crypto_JobStateType;

/* Job state constants (T3 fix, 2026-08-08): Crypto.c referenced these but
 * they were only defined in the classic CryptoStack_Types.h enum, which
 * this header's CRYPTO_ALGOFAM_* macros clash with. Values mirror the
 * classic enum. */
#define CRYPTO_JOBSTATE_IDLE            (0x00U)
#define CRYPTO_JOBSTATE_QUEUED          (0x01U)
#define CRYPTO_JOBSTATE_PROCESSING      (0x02U)
#define CRYPTO_JOBSTATE_WAITING         (0x03U)
#define CRYPTO_JOBSTATE_RESULT_READY    (0x04U)
#define CRYPTO_JOBSTATE_CANCELED        (0x05U)

typedef uint32 Crypto_KeyIdType;
typedef uint32 Crypto_KeyElementIdType;
typedef uint32 Crypto_HsmStatusType;

/**********************************************************************************************************************
 * ENUMERATION TYPES
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_RESULT_OK = 0,
    CRYPTO_RESULT_NOT_OK,
    CRYPTO_RESULT_BUSY,
    CRYPTO_RESULT_BUSY_RETRY_LATER,
    CRYPTO_RESULT_ENTROPY_EXHAUSTED
} Crypto_ResultType;

typedef enum {
    CRYPTO_PROCESSING_ASYNC = 0,
    CRYPTO_PROCESSING_SYNC
} Crypto_ProcessingType;

typedef enum {
    CRYPTO_SIGNATURE_GENERATE = 0,
    CRYPTO_SIGNATURE_VERIFY
} Crypto_SignatureType;

typedef enum {
    CRYPTO_VERIFICATION_FAILED = 0,
    CRYPTO_VERIFICATION_PASSED,
    CRYPTO_VERIFICATION_IN_PROGRESS,
    CRYPTO_VERIFICATION_NOT_OK
} Crypto_VerifyResult;

typedef enum {
    CRYPTO_KEY_TYPE_SEED = 0,
    CRYPTO_KEY_TYPE_SHE,
    CRYPTO_KEY_TYPE_HSM,
    CRYPTO_KEY_TYPE_CUSTOM
} Crypto_KeyTypeEnum;

typedef enum {
    CRYPTO_HSM_IDLE = 0,
    CRYPTO_HSM_BUSY,
    CRYPTO_HSM_ERROR,
    CRYPTO_HSM_UNINIT
} Crypto_HsmStateType;

typedef enum {
    CRYPTO_ECC_CURVE_SECP256R1 = 0,
    CRYPTO_ECC_CURVE_SECP384R1,
    CRYPTO_ECC_CURVE_SECP521R1,
    CRYPTO_ECC_CURVE_SECP256K1,
    CRYPTO_ECC_CURVE_BRAINPOOLP256R1,
    CRYPTO_ECC_CURVE_BRAINPOOLP384R1,
    CRYPTO_ECC_CURVE_BRAINPOOLP512R1
} Crypto_EccCurveType;

typedef enum {
    CRYPTO_KEY_FORMAT_BIN_RAW = 0,
    CRYPTO_KEY_FORMAT_BIN_CPK,
    CRYPTO_KEY_FORMAT_BIN_HSM,
    CRYPTO_KEY_FORMAT_BIN_SHE
} Crypto_KeyFormatType;

typedef enum {
    CRYPTO_KEY_EXCHANGE_PARTY_LOCAL = 0,
    CRYPTO_KEY_EXCHANGE_PARTY_REMOTE
} Crypto_KeyExchangePartyType;

/**********************************************************************************************************************
 * STRUCTURE TYPES
 *********************************************************************************************************************/

/* Algorithm Configuration */
typedef struct {
    Crypto_AlgorithmFamilyType family;
    Crypto_AlgorithmModeType   mode;
    uint32                     keyLength;
    Crypto_EccCurveType        curve;
} Crypto_AlgorithmInfoType;

/* Key Element Configuration */
typedef struct {
    Crypto_KeyElementIdType id;
    uint32                  size;
    boolean                 allowPartialAccess;
    boolean                 writeAccess;
    uint8*                  data;
} Crypto_KeyElementType;

/* Key Configuration */
typedef struct {
    Crypto_KeyIdType         keyId;
    uint32                   numElements;
    Crypto_KeyElementType*   keyElements;
    Crypto_KeyTypeEnum       keyType;
    uint8                    keyState;
} Crypto_KeyType;

/* Job Primitive Info */
typedef struct {
    uint32                      callbackId;
    Crypto_AlgorithmInfoType*   algorithm;
    Crypto_ServiceInfoType      service;
    Crypto_ProcessingType       processingType;
    boolean                     primitiveCallbackUpdateNotification;
} Crypto_JobPrimitiveInfoType;

/* Job Info */
typedef struct {
    uint32    jobId;
    uint32    jobPriority;
} Crypto_JobInfoType;

/* Job Primitive Input Output */
typedef struct {
    uint32*   inputPtr;
    uint32    inputLength;
    Crypto_OperationModeType mode;
    uint32*   secondaryInputPtr;
    uint32    secondaryInputLength;
    uint32*   tertiaryInputPtr;
    uint32    tertiaryInputLength;
    uint32*   outputPtr;
    uint32*   outputLengthPtr;
    uint32*   secondaryOutputPtr;
    uint32*   secondaryOutputLengthPtr;
    uint64    input64;
    const uint8*  input8Ptr;
    uint8*        output8Ptr;
    uint32*       outputLength8Ptr;
    Crypto_VerifyResultType* verifyPtr;
} Crypto_JobPrimitiveInputOutputType;

/* Crypto Job Type */
typedef struct {
    uint32                              jobId;
    Crypto_JobStateType                 jobState;
    Crypto_JobPrimitiveInputOutputType* jobPrimitiveInputOutput;
    Crypto_JobPrimitiveInfoType*        jobPrimitiveInfo;
    Crypto_JobInfoType*                 jobInfo;
    uint32                              cryptoKeyId;
    uint32                              targetCryptoKeyId;
    uint32                              jobRedirectionInfoRef;
    uint32                              targetKeyId;
} Crypto_JobType;

/* Config-time key element definition (read-only, from generated config) */
typedef struct {
    Crypto_KeyElementIdType keyElementId;
    uint32                  keyElementSize;
    boolean                 allowPartialAccess;
    boolean                 readAccess;
    boolean                 writeAccess;
} Crypto_KeyElementConfigType;

/* Config-time key definition (read-only, from generated config) */
typedef struct {
    Crypto_KeyIdType                   keyId;
    const Crypto_KeyElementConfigType* keyElements;
    uint32                             numKeyElements;
    boolean                            keyValid;
} Crypto_KeyConfigType;

/* Job completion callback (config-time) */
typedef void (*Crypto_JobCallbackType)(Crypto_JobType* job, Crypto_JobStateType result);

/* Driver object configuration (config-time) */
typedef struct {
    uint32                   driverObjectId;
    uint32                   priority;
    uint32                   maxJobs;
    boolean                  asyncMode;
    Crypto_JobCallbackType   callback;
} Crypto_DriverObjectConfigType;

/* Channel configuration (config-time) */
typedef struct {
    uint32                        channelId;
    uint32                        driverObjectId;
    Crypto_AlgorithmFamilyType    algorithmFamily;
    Crypto_AlgorithmModeType      algorithmMode;
    boolean                       hwAcceleration;
    uint32                        maxKeySize;
} Crypto_ChannelConfigType;


/* Queue Element */
typedef struct Crypto_QueueElementStruct {
    Crypto_JobType*                 job;
    struct Crypto_QueueElementStruct* next;
} Crypto_QueueElementType;

/* HSM Configuration */
typedef struct {
    boolean hsmEnabled;
    uint32  hsmInstanceId;
    uint32  hsmChannelId;
    uint32  hsmCommandTimeout;
    uint32  hsmResponseTimeout;
} Crypto_HsmConfigType;

/* Crypto Driver Configuration */
typedef struct {
    uint32                  numKeys;
    Crypto_KeyType*         keys;
    uint32                  numChannels;
    uint32                  queueSize;
    Crypto_HsmConfigType    hsmConfig;
    boolean                 cryptoDevErrorDetect;
    boolean                 cryptoVersionInfoApi;
    /* Config-time structures (from generated config) */
    const Crypto_DriverObjectConfigType* driverObjects;
    uint32                              numDriverObjects;
    const Crypto_ChannelConfigType*     channels;
    boolean                            hwAccelerationEnabled;
    uint32                             clockFrequency;
} Crypto_ConfigType;

/**********************************************************************************************************************
 * HSM SPECIFIC TYPES
 *********************************************************************************************************************/

/* HSM Command Types */
typedef enum {
    CRYPTO_HSM_CMD_NONE = 0,
    CRYPTO_HSM_CMD_GENERATE_RANDOM,
    CRYPTO_HSM_CMD_GENERATE_KEY_PAIR,
    CRYPTO_HSM_CMD_SIGN,
    CRYPTO_HSM_CMD_VERIFY,
    CRYPTO_HSM_CMD_ENCRYPT,
    CRYPTO_HSM_CMD_DECRYPT,
    CRYPTO_HSM_CMD_HASH,
    CRYPTO_HSM_CMD_HMAC,
    CRYPTO_HSM_CMD_KDF,
    CRYPTO_HSM_CMD_LOAD_KEY,
    CRYPTO_HSM_CMD_UNLOAD_KEY
} Crypto_HsmCommandType;

/* HSM Response Structure */
typedef struct {
    Crypto_HsmCommandType   command;
    Crypto_ResultType       result;
    uint8*                  data;
    uint32                  dataLength;
    uint32                  errorCode;
} Crypto_HsmResponseType;

/* HSM Job Context */
typedef struct {
    Crypto_HsmCommandType   command;
    uint32                  keyId;
    uint8*                  inputData;
    uint32                  inputLength;
    uint8*                  outputData;
    uint32*                 outputLengthPtr;
    Crypto_VerifyResultType* verifyResult;
    void (*callback)(void);
} Crypto_HsmJobContextType;

#endif /* CRYPTO_TYPES_H */
