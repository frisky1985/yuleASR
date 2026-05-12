/**
 * @file CryIf_Types.h
 * @brief Crypto Interface Types Definition
 * @version 1.0.0
 * @date 2026-05-01
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details Type definitions for CRYIF module following AutoSAR Classic Platform 4.x standard
 */

#ifndef CRYIF_TYPES_H
#define CRYIF_TYPES_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define CRYIF_TYPES_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define CRYIF_TYPES_AR_RELEASE_MINOR_VERSION    (0x04U)
#define CRYIF_TYPES_AR_RELEASE_REVISION_VERSION (0x00U)
#define CRYIF_TYPES_SW_MAJOR_VERSION            (0x01U)
#define CRYIF_TYPES_SW_MINOR_VERSION            (0x00U)
#define CRYIF_TYPES_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/

/** @brief Maximum number of channels */
#ifndef CRYIF_MAX_CHANNEL_COUNT
#define CRYIF_MAX_CHANNEL_COUNT                 (0x08U)
#endif

/** @brief Maximum number of keys */
#ifndef CRYIF_MAX_KEY_COUNT
#define CRYIF_MAX_KEY_COUNT                     (0x10U)
#endif

/** @brief Maximum number of jobs */
#ifndef CRYIF_MAX_JOB_COUNT
#define CRYIF_MAX_JOB_COUNT                     (0x20U)
#endif

/** @brief Maximum key element size */
#ifndef CRYIF_MAX_KEY_ELEMENT_SIZE
#define CRYIF_MAX_KEY_ELEMENT_SIZE              (0x100U)
#endif

/** @brief Invalid channel ID */
#define CRYIF_INVALID_CHANNEL_ID                (0xFFU)

/** @brief Invalid key ID */
#define CRYIF_INVALID_KEY_ID                    (0xFFFFU)

/** @brief Invalid job ID */
#define CRYIF_INVALID_JOB_ID                    (0xFFFFU)

/*==================================================================================================
*                                      TYPE DEFINITIONS
==================================================================================================*/

/** @brief Return type for CryIf APIs */
typedef uint8 CryIf_ReturnType;

/** @brief Channel identifier type */
typedef uint8 CryIf_ChannelIdType;

/** @brief Key identifier type */
typedef uint16 CryIf_KeyIdType;

/** @brief Job identifier type */
typedef uint16 CryIf_JobIdType;

/** @brief Key element identifier type */
typedef uint32 CryIf_KeyElementIdType;

/** @brief Crypto operation mode type */
typedef uint8 CryIf_OperationModeType;

/** @brief Crypto service information type */
typedef uint8 CryIf_ServiceInfoType;

/** @brief Algorithm family type */
typedef uint8 CryIf_AlgorithmFamilyType;

/** @brief Algorithm mode type */
typedef uint8 CryIf_AlgorithmModeType;

/** @brief Crypto operation type */
typedef uint8 CryIf_CryptoOperationType;

/** @brief Result type for crypto operations */
typedef uint8 CryIf_ResultType;

/** @brief Security level type */
typedef uint8 CryIf_SecurityLevelType;

/** @brief Processing type */
typedef uint8 CryIf_ProcessingType;

/** @brief Callback notification type */
typedef void (*CryIf_NotificationCallbackType)(CryIf_JobIdType jobId, CryIf_ResultType result);

/** @brief CryIf module states */
typedef enum {
    CRYIF_STATE_UNINIT = 0x00U,
    CRYIF_STATE_INIT   = 0x01U
} CryIf_StateType;

/** @brief CryIf operation modes */
enum {
    CRYIF_OP_MODE_SINGLE    = 0x01U,
    CRYIF_OP_MODE_START     = 0x02U,
    CRYIF_OP_MODE_UPDATE    = 0x04U,
    CRYIF_OP_MODE_STREAM    = 0x08U,
    CRYIF_OP_MODE_FINISH    = 0x10U
};

/** @brief CryIf algorithm families */
typedef enum {
    CRYIF_ALGOFAM_NOT_SET          = 0x00U,
    CRYIF_ALGOFAM_AES              = 0x01U,
    CRYIF_ALGOFAM_DES              = 0x02U,
    CRYIF_ALGOFAM_RSA              = 0x03U,
    CRYIF_ALGOFAM_ECC              = 0x04U,
    CRYIF_ALGOFAM_SHA1             = 0x05U,
    CRYIF_ALGOFAM_SHA2_224         = 0x06U,
    CRYIF_ALGOFAM_SHA2_256         = 0x07U,
    CRYIF_ALGOFAM_SHA2_384         = 0x08U,
    CRYIF_ALGOFAM_SHA2_512         = 0x09U,
    CRYIF_ALGOFAM_SHA3_224         = 0x0AU,
    CRYIF_ALGOFAM_SHA3_256         = 0x0BU,
    CRYIF_ALGOFAM_SHA3_384         = 0x0CU,
    CRYIF_ALGOFAM_SHA3_512         = 0x0DU,
    CRYIF_ALGOFAM_HMAC             = 0x0EU,
    CRYIF_ALGOFAM_CMAC             = 0x0FU,
    CRYIF_ALGOFAM_HMAC_SHA256      = 0x10U,
    CRYIF_ALGOFAM_DRBG             = 0x11U,
    CRYIF_ALGOFAM_PBKF2HMAC        = 0x12U,
    CRYIF_ALGOFAM_KDFX963          = 0x13U,
    CRYIF_ALGOFAM_RIPEMD160        = 0x14U,
    CRYIF_ALGOFAM_ECDSA            = 0x15U,
    CRYIF_ALGOFAM_ED25519          = 0x16U,
    CRYIF_ALGOFAM_CUSTOM           = 0xFFU
} CryIf_AlgorithmFamilyEnumType;

/** @brief CryIf algorithm modes */
typedef enum {
    CRYIF_ALGOMODE_NOT_SET         = 0x00U,
    CRYIF_ALGOMODE_ECB             = 0x01U,
    CRYIF_ALGOMODE_CBC             = 0x02U,
    CRYIF_ALGOMODE_CFB             = 0x03U,
    CRYIF_ALGOMODE_OFB             = 0x04U,
    CRYIF_ALGOMODE_CTR             = 0x05U,
    CRYIF_ALGOMODE_GCM             = 0x06U,
    CRYIF_ALGOMODE_CCM             = 0x07U,
    CRYIF_ALGOMODE_PKCS7           = 0x08U,
    CRYIF_ALGOMODE_XTS             = 0x09U,
    CRYIF_ALGOMODE_RSA_PKCS1       = 0x0AU,
    CRYIF_ALGOMODE_RSA_PSS         = 0x0BU,
    CRYIF_ALGOMODE_RSA_OAEP        = 0x0CU,
    CRYIF_ALGOMODE_ECDSA           = 0x0DU,
    CRYIF_ALGOMODE_CUSTOM          = 0xFFU
} CryIf_AlgorithmModeEnumType;

/** @brief CryIf crypto operations */
typedef enum {
    CRYIF_CRYPTO_OPERATION_ENCRYPT = 0x01U,
    CRYIF_CRYPTO_OPERATION_DECRYPT = 0x02U,
    CRYIF_CRYPTO_OPERATION_SIGN    = 0x03U,
    CRYIF_CRYPTO_OPERATION_VERIFY  = 0x04U,
    CRYIF_CRYPTO_OPERATION_HASH    = 0x05U,
    CRYIF_CRYPTO_OPERATION_MAC     = 0x06U,
    CRYIF_CRYPTO_OPERATION_KEYGEN  = 0x07U,
    CRYIF_CRYPTO_OPERATION_KEYDER  = 0x08U
} CryIf_CryptoOperationEnumType;

/** @brief CryIf processing types */
typedef enum {
    CRYIF_PROCESSING_SYNC          = 0x01U,
    CRYIF_PROCESSING_ASYNC         = 0x02U
} CryIf_ProcessingEnumType;

/** @brief CryIf result types */
typedef enum {
    CRYIF_E_OK                     = 0x00U,
    CRYIF_E_NOT_OK                 = 0x01U,
    CRYIF_E_BUSY                   = 0x02U,
    CRYIF_E_QUEUE_FULL             = 0x03U,
    CRYIF_E_KEY_NOT_AVAILABLE      = 0x04U,
    CRYIF_E_KEY_INVALID            = 0x05U,
    CRYIF_E_KEY_SIZE_MISMATCH      = 0x06U,
    CRYIF_E_JOB_NOT_AVAILABLE      = 0x07U,
    CRYIF_E_PARAM_POINTER          = 0x08U,
    CRYIF_E_PARAM_VALUE            = 0x09U,
    CRYIF_E_PARAM_LENGTH           = 0x0AU,
    CRYIF_E_SMALL_BUFFER           = 0x0BU,
    CRYIF_E_ENTROPY_EXHAUSTED      = 0x0CU,
    CRYIF_E_ENTROPY_EXHAUSTION     = 0x0CU
} CryIf_ResultEnumType;

/** @brief CryIf security levels */
typedef enum {
    CRYIF_SEC_LEVEL_NONE           = 0x00U,
    CRYIF_SEC_LEVEL_1              = 0x01U,
    CRYIF_SEC_LEVEL_2              = 0x02U,
    CRYIF_SEC_LEVEL_3              = 0x03U,
    CRYIF_SEC_LEVEL_4              = 0x04U,
    CRYIF_SEC_LEVEL_5              = 0x05U,
    CRYIF_SEC_LEVEL_6              = 0x06U,
    CRYIF_SEC_LEVEL_7              = 0x07U
} CryIf_SecurityLevelEnumType;

/** @brief Key element information structure */
typedef struct {
    CryIf_KeyElementIdType id;
    uint32 size;
} CryIf_KeyElementInfoType;

/** @brief Key element structure */
typedef struct {
    CryIf_KeyElementIdType id;
    uint8* dataPtr;
    uint32 dataLength;
    uint32 maxDataLength;
} CryIf_KeyElementType;

/** @brief Key structure */
typedef struct {
    CryIf_KeyIdType keyId;
    CryIf_KeyIdType cryptoKeyId;
    uint32 keyElementCount;
    CryIf_SecurityLevelType securityLevel;
    boolean isValid;
} CryIf_KeyType;

/** @brief Job primitive information */
typedef struct {
    CryIf_CryptoOperationType cryptoOperation;
    CryIf_AlgorithmFamilyType algorithmFamily;
    CryIf_AlgorithmModeType algorithmMode;
    CryIf_OperationModeType operationMode;
    uint32 resultLength;
    CryIf_ServiceInfoType service;
} CryIf_JobPrimitiveInfoType;

/** @brief Job primitive input/output structure */
typedef struct {
    const uint8* inputPtr;
    uint32 inputLength;
    uint8* outputPtr;
    uint32* outputLengthPtr;
    const uint8* secondaryInputPtr;
    uint32 secondaryInputLength;
    uint8* secondaryOutputPtr;
    uint32* secondaryOutputLengthPtr;
    uint32* verifyPtr;
    CryIf_OperationModeType mode;
} CryIf_JobPrimitiveInputOutputType;

/** @brief Job information structure */
typedef struct {
    CryIf_JobIdType jobId;
    CryIf_JobPrimitiveInfoType jobPrimitiveInfo;
    CryIf_JobPrimitiveInputOutputType jobPrimitiveInputOutput;
    CryIf_ProcessingType processingType;
    CryIf_NotificationCallbackType callback;
    uint32 priority;
    boolean isBusy;
} CryIf_JobType;

/** @brief Channel structure */
typedef struct {
    CryIf_ChannelIdType channelId;
    uint8 driverIndex;
    uint8 channelIndex;
    uint32 maxKeySize;
    uint32 maxJobSize;
    boolean isActive;
} CryIf_ChannelType;

/** @brief Key configuration structure */
typedef struct {
    CryIf_KeyIdType cryIfKeyId;
    CryIf_KeyIdType cryptoKeyId;
    uint8 driverIndex;
    CryIf_SecurityLevelType securityLevel;
} CryIf_KeyCfgType;

/** @brief Channel configuration structure */
typedef struct {
    CryIf_ChannelIdType cryIfChannelId;
    uint8 driverObjectIndex;
    uint8 driverIndex;
    uint32 maxKeySize;
    uint32 maxJobSize;
} CryIf_ChannelCfgType;

/** @brief General configuration structure */
typedef struct {
    uint32 maxChannels;
    uint32 maxKeys;
    uint32 maxJobs;
    uint32 versionInfoApi;
    uint32 keyElementCopyApi;
    uint32 keyValidCheckApi;
    const CryIf_ChannelCfgType* channelConfig;
    const CryIf_KeyCfgType* keyConfig;
} CryIf_GeneralCfgType;

/** @brief CryIf configuration structure */
typedef struct {
    const CryIf_GeneralCfgType* generalConfig;
    uint32 numChannels;
    uint32 numKeys;
} CryIf_ConfigType;

#endif /* CRYIF_TYPES_H */
