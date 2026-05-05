/*==================================================================================================
 *                                KEY MANAGER (KeyM)
 *==================================================================================================
 * FILENAME: KeyM.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_KeyManager.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Key Manager module
 *==================================================================================================
 */

#ifndef KEYM_H
#define KEYM_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"
#include "KeyM_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define KEYM_VENDOR_ID                   (100u)
#define KEYM_MODULE_ID                   (120u)
#define KEYM_INSTANCE_ID                 (0u)

#define KEYM_AR_RELEASE_MAJOR_VERSION    (4u)
#define KEYM_AR_RELEASE_MINOR_VERSION    (7u)
#define KEYM_AR_RELEASE_REVISION_VERSION (0u)

#define KEYM_SW_MAJOR_VERSION            (1u)
#define KEYM_SW_MINOR_VERSION            (0u)
#define KEYM_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((KEYM_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (KEYM_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of KeyM.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define KEYM_SID_INIT                           (0x00u)
#define KEYM_SID_DEINIT                         (0x01u)
#define KEYM_SID_GETVERSIONINFO                 (0x02u)
#define KEYM_SID_SETKEY                         (0x10u)
#define KEYM_SID_GETKEY                         (0x11u)
#define KEYM_SID_UPDATEKEY                      (0x12u)
#define KEYM_SID_FINALIZEKEY                    (0x13u)
#define KEYM_SID_PARSEKEY                       (0x20u)
#define KEYM_SID_CONVERTKEY                     (0x21u)
#define KEYM_SID_COPYKEY                        (0x22u)
#define KEYM_SID_KEYELEMENTSET                  (0x30u)
#define KEYM_SID_KEYELEMENTGET                  (0x31u)
#define KEYM_SID_KEYSTATUSGET                   (0x40u)
#define KEYM_SID_KEYVERSIONGET                  (0x41u)
#define KEYM_SID_KEYVALIDITYGET                 (0x42u)
#define KEYM_SID_KEYINFOGET                     (0x43u)
#define KEYM_SID_MAINFUNCTION                   (0x50u)
#define KEYM_SID_CALLBACKNOTIFICATION           (0x60u)

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes */
#define KEYM_E_PARAM_POINTER                    (0x01u)  /* API called with NULL pointer */
#define KEYM_E_PARAM_HANDLE                     (0x02u)  /* Invalid key handle */
#define KEYM_E_PARAM_LENGTH                     (0x03u)  /* Invalid length parameter */
#define KEYM_E_UNINIT                           (0x04u)  /* API called before initialization */
#define KEYM_E_ALREADY_INITIALIZED              (0x05u)  /* Multiple initialization call */
#define KEYM_E_INVALID_KEY                      (0x06u)  /* Invalid key ID */
#define KEYM_E_INVALID_KEY_FORMAT               (0x07u)  /* Invalid key format */
#define KEYM_E_INVALID_KEY_STATUS               (0x08u)  /* Invalid key status for operation */
#define KEYM_E_KEY_NOT_AVAILABLE                (0x09u)  /* Key not available */

/* Runtime error codes */
#define KEYM_E_BUSY                             (0x01u)  /* Key operation busy */
#define KEYM_E_QUEUE_FULL                       (0x02u)  /* Operation queue full */
#define KEYM_E_KEY_NOT_VALID                    (0x03u)  /* Key not valid */
#define KEYM_E_KEY_EXPIRED                      (0x04u)  /* Key has expired */
#define KEYM_E_KEY_REVOKED                      (0x05u)  /* Key has been revoked */
#define KEYM_E_CRYPTO_ERROR                     (0x06u)  /* Crypto operation failed */

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/**
 * @brief Key identifier type
 */
typedef uint32 KeyM_KeyIdType;

/**
 * @brief Key element identifier type
 */
typedef uint32 KeyM_KeyElementIdType;

/**
 * @brief Key status type - Represents the lifecycle state of a key
 */
typedef enum {
    KEYM_KEY_STATUS_NEW = 0,            /* Key is newly created, not yet valid */
    KEYM_KEY_STATUS_UPDATE,             /* Key is being updated */
    KEYM_KEY_STATUS_VALID,              /* Key is valid and can be used */
    KEYM_KEY_STATUS_INVALID             /* Key is invalid or revoked */
} KeyM_KeyStatusType;

/**
 * @brief Key format type - Represents the format of key data
 */
typedef enum {
    KEYM_KEY_FORMAT_RAW = 0,            /* Raw binary format */
    KEYM_KEY_FORMAT_DER,                /* DER encoded format */
    KEYM_KEY_FORMAT_PEM,                /* PEM encoded format */
    KEYM_KEY_FORMAT_COSE,               /* CBOR Object Signing and Encryption */
    KEYM_KEY_FORMAT_JWK                 /* JSON Web Key format */
} KeyM_KeyFormatType;

/**
 * @brief Key type - Represents the cryptographic algorithm type
 */
typedef enum {
    KEYM_KEY_TYPE_AES = 0,              /* AES symmetric key */
    KEYM_KEY_TYPE_DES,                  /* DES symmetric key */
    KEYM_KEY_TYPE_3DES,                 /* 3DES symmetric key */
    KEYM_KEY_TYPE_RSA,                  /* RSA asymmetric key */
    KEYM_KEY_TYPE_ECC,                  /* ECC asymmetric key */
    KEYM_KEY_TYPE_HMAC,                 /* HMAC key */
    KEYM_KEY_TYPE_GENERIC               /* Generic key type */
} KeyM_KeyType;

/**
 * @brief Key usage type - Represents allowed key operations
 */
typedef enum {
    KEYM_KEY_USAGE_ENCRYPT = 0x01,      /* Key can be used for encryption */
    KEYM_KEY_USAGE_DECRYPT = 0x02,      /* Key can be used for decryption */
    KEYM_KEY_USAGE_SIGN = 0x04,         /* Key can be used for signing */
    KEYM_KEY_USAGE_VERIFY = 0x08,       /* Key can be used for verification */
    KEYM_KEY_USAGE_MAC_GENERATE = 0x10, /* Key can be used for MAC generation */
    KEYM_KEY_USAGE_MAC_VERIFY = 0x20    /* Key can be used for MAC verification */
} KeyM_KeyUsageType;

/**
 * @brief Key information structure
 */
typedef struct {
    KeyM_KeyIdType keyId;               /* Key identifier */
    KeyM_KeyType keyType;               /* Key type */
    uint32 keyLength;                   /* Key length in bits */
    KeyM_KeyStatusType keyStatus;       /* Current key status */
    uint32 keyVersion;                  /* Key version number */
    uint32 validFrom;                   /* Valid from timestamp */
    uint32 validTo;                     /* Valid to timestamp (0 = no expiry) */
    uint8 keyUsage;                     /* Key usage flags (bitmap of KeyM_KeyUsageType) */
} KeyM_KeyInfoType;

/**
 * @brief Key element structure - Represents a component of a key
 */
typedef struct {
    KeyM_KeyElementIdType elementId;    /* Element identifier */
    uint8* dataPtr;                     /* Element data pointer */
    uint32 dataLength;                  /* Element data length */
    boolean isEncrypted;                /* Element is encrypted */
} KeyM_KeyElementType;

/**
 * @brief Key certificate structure
 */
typedef struct {
    uint32 certId;                      /* Certificate identifier */
    uint8* certDataPtr;                 /* Certificate data pointer */
    uint32 certDataLength;              /* Certificate data length */
    boolean isValid;                    /* Certificate validity */
} KeyM_CertificateType;

/**
 * @brief Key configuration type
 */
typedef struct {
    KeyM_KeyIdType keyId;               /* Key identifier */
    KeyM_KeyType keyType;               /* Key type */
    uint32 maxKeyLength;                /* Maximum key length in bytes */
    uint32 numKeyElements;              /* Number of key elements */
    uint32 initialVersion;              /* Initial key version */
    uint32 validityPeriod;              /* Validity period in seconds (0 = no expiry) */
    uint8 allowedUsage;                 /* Allowed key usage flags */
} KeyM_KeyConfigType;

/**
 * @brief KeyM configuration type
 */
typedef struct {
    const KeyM_KeyConfigType* keyConfigs;   /* Array of key configurations */
    uint16 numKeys;                         /* Number of keys */
    uint16 maxKeyElements;                  /* Maximum key elements per key */
    boolean asyncOperations;                /* Asynchronous operations enabled */
    boolean keyStorageNvM;                  /* Store keys in NvM */
    uint32 maxKeyLifetime;                  /* Maximum key lifetime in seconds */
} KeyM_ConfigType;

/**
 * @brief Key operation result type
 */
typedef enum {
    KEYM_OPRESULT_PENDING = 0,          /* Operation pending */
    KEYM_OPRESULT_SUCCESS,              /* Operation successful */
    KEYM_OPRESULT_FAILED,               /* Operation failed */
    KEYM_OPRESULT_CANCELLED             /* Operation cancelled */
} KeyM_OperationResultType;

/**
 * @brief Key operation notification callback type
 */
typedef void (*KeyM_NotificationCallbackType)(KeyM_KeyIdType keyId,
                                               KeyM_OperationResultType result,
                                               const uint8* dataPtr,
                                               uint32 dataLength);

/*==================================================================================================
 *                                    GLOBAL CONSTANTS
 *==================================================================================================*/
#define KEYM_INVALID_KEY_ID             (0xFFFFFFFFu)
#define KEYM_INVALID_KEY_ELEMENT_ID     (0xFFFFFFFFu)
#define KEYM_INVALID_CERTIFICATE_ID     (0xFFFFFFFFu)
#define KEYM_KEY_VERSION_MASK           (0x00FFFFFFu)

/* Key element IDs (standard) */
#define KEYM_KEY_ELEMENT_ID_KEY         (0x01u)  /* Key material */
#define KEYM_KEY_ELEMENT_ID_IV          (0x02u)  /* Initialization vector */
#define KEYM_KEY_ELEMENT_ID_SALT        (0x03u)  /* Salt value */
#define KEYM_KEY_ELEMENT_ID_TAG         (0x04u)  /* Authentication tag */

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define KEYM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "KeyM_MemMap.h"

extern boolean KeyM_Initialized;
extern const KeyM_ConfigType* KeyM_ConfigPtr;

#define KEYM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "KeyM_MemMap.h"

/*==================================================================================================
 *                                    API DECLARATIONS
 *==================================================================================================*/
#define KEYM_START_SEC_CODE
#include "KeyM_MemMap.h"

/**
 * @brief Initializes the Key Manager module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_KeyM_00001
 */
extern void KeyM_Init(const KeyM_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the Key Manager module
 * @return None
 * @req SWS_KeyM_00002
 */
extern void KeyM_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_KeyM_00003
 */
#if (KEYM_VERSION_INFO_API == STD_ON)
extern void KeyM_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Sets a key with the given data
 * @param keyId Key identifier
 * @param keyPtr Key data pointer
 * @param keyLength Key data length
 * @param keyFormat Format of the key data
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00100
 */
extern Std_ReturnType KeyM_SetKey(KeyM_KeyIdType keyId,
                                   const uint8* keyPtr,
                                   uint32 keyLength,
                                   KeyM_KeyFormatType keyFormat);

/**
 * @brief Gets a key
 * @param keyId Key identifier
 * @param keyPtr Output buffer for key data
 * @param keyLengthPtr Pointer to key length (input: buffer size, output: actual length)
 * @param keyFormatPtr Pointer to store key format
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00110
 */
extern Std_ReturnType KeyM_GetKey(KeyM_KeyIdType keyId,
                                   uint8* keyPtr,
                                   uint32* keyLengthPtr,
                                   KeyM_KeyFormatType* keyFormatPtr);

/**
 * @brief Updates an existing key
 * @param keyId Key identifier
 * @param keyPtr New key data pointer
 * @param keyLength New key data length
 * @param keyFormat Format of the key data
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00120
 */
extern Std_ReturnType KeyM_UpdateKey(KeyM_KeyIdType keyId,
                                      const uint8* keyPtr,
                                      uint32 keyLength,
                                      KeyM_KeyFormatType keyFormat);

/**
 * @brief Finalizes a key, making it valid for use
 * @param keyId Key identifier
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00130
 */
extern Std_ReturnType KeyM_FinalizeKey(KeyM_KeyIdType keyId);

/**
 * @brief Parses key data from a specific format
 * @param keyId Key identifier
 * @param inputPtr Input data pointer
 * @param inputLength Input data length
 * @param inputFormat Input data format
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00200
 */
extern Std_ReturnType KeyM_ParseKey(KeyM_KeyIdType keyId,
                                     const uint8* inputPtr,
                                     uint32 inputLength,
                                     KeyM_KeyFormatType inputFormat);

/**
 * @brief Converts key data to a specific format
 * @param keyId Key identifier
 * @param outputPtr Output buffer pointer
 * @param outputLengthPtr Pointer to output length
 * @param outputFormat Desired output format
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00210
 */
extern Std_ReturnType KeyM_ConvertKey(KeyM_KeyIdType keyId,
                                       uint8* outputPtr,
                                       uint32* outputLengthPtr,
                                       KeyM_KeyFormatType outputFormat);

/**
 * @brief Copies a key from source to destination
 * @param srcKeyId Source key identifier
 * @param destKeyId Destination key identifier
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00220
 */
extern Std_ReturnType KeyM_CopyKey(KeyM_KeyIdType srcKeyId,
                                    KeyM_KeyIdType destKeyId);

/**
 * @brief Sets a key element
 * @param keyId Key identifier
 * @param keyElementId Key element identifier
 * @param keyElementPtr Key element data pointer
 * @param keyElementLength Key element data length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00300
 */
extern Std_ReturnType KeyM_KeyElementSet(KeyM_KeyIdType keyId,
                                          KeyM_KeyElementIdType keyElementId,
                                          const uint8* keyElementPtr,
                                          uint32 keyElementLength);

/**
 * @brief Gets a key element
 * @param keyId Key identifier
 * @param keyElementId Key element identifier
 * @param keyElementPtr Output buffer pointer
 * @param keyElementLengthPtr Pointer to element length
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00310
 */
extern Std_ReturnType KeyM_KeyElementGet(KeyM_KeyIdType keyId,
                                          KeyM_KeyElementIdType keyElementId,
                                          uint8* keyElementPtr,
                                          uint32* keyElementLengthPtr);

/**
 * @brief Gets the status of a key
 * @param keyId Key identifier
 * @param keyStatusPtr Pointer to store key status
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00400
 */
extern Std_ReturnType KeyM_KeyStatusGet(KeyM_KeyIdType keyId,
                                         KeyM_KeyStatusType* keyStatusPtr);

/**
 * @brief Gets the version of a key
 * @param keyId Key identifier
 * @param keyVersionPtr Pointer to store key version
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00410
 */
extern Std_ReturnType KeyM_KeyVersionGet(KeyM_KeyIdType keyId,
                                          uint32* keyVersionPtr);

/**
 * @brief Gets the validity information of a key
 * @param keyId Key identifier
 * @param validFromPtr Pointer to store valid from timestamp
 * @param validToPtr Pointer to store valid to timestamp
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00420
 */
extern Std_ReturnType KeyM_KeyValidityGet(KeyM_KeyIdType keyId,
                                           uint32* validFromPtr,
                                           uint32* validToPtr);

/**
 * @brief Gets key information
 * @param keyId Key identifier
 * @param keyInfoPtr Pointer to key info structure
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00430
 */
extern Std_ReturnType KeyM_KeyInfoGet(KeyM_KeyIdType keyId,
                                       KeyM_KeyInfoType* keyInfoPtr);

/**
 * @brief Sets the notification callback
 * @param callback Notification callback function
 * @return E_OK if successful, E_NOT_OK otherwise
 * @req SWS_KeyM_00500
 */
extern Std_ReturnType KeyM_SetNotificationCallback(KeyM_NotificationCallbackType callback);

/**
 * @brief Main function for background key operations
 * @return None
 * @req SWS_KeyM_00510
 */
extern void KeyM_MainFunction(void);

#define KEYM_STOP_SEC_CODE
#include "KeyM_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* KEYM_H */
