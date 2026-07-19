/**
 * @file KeyM.c
 * @brief Key Manager Implementation
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/*==================================================================================================
 *                                KEY MANAGER (KeyM)
 *==================================================================================================
 * FILENAME: KeyM.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_KeyManager.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Key Manager module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "KeyM.h"
#include "KeyM_MemMap.h"
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
#include "SchM_KeyM.h"
#endif

/*==================================================================================================
 *                                    VERSION CHECK
 *==================================================================================================*/
#if defined(KEYM_AR_RELEASE_MAJOR_VERSION) && (KEYM_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "KeyM.c: AR major version mismatch"
#endif

#if defined(KEYM_AR_RELEASE_MINOR_VERSION) && (KEYM_AR_RELEASE_MINOR_VERSION != 4u)
    #error "KeyM.c: AR minor version mismatch"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define KEYM_KEY_VERSION_INCREMENT      (1u)
#define KEYM_KEY_STATUS_MASK            (0x03u)
#define KEYM_MAX_KEY_STORAGE_SIZE       (KEYM_NUM_KEYS * KEYM_MAX_KEY_LENGTH)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/

/**
 * @brief Key runtime information structure
 */
typedef struct {
    uint8 keyData[KEYM_MAX_KEY_LENGTH];     /* Key material */
    uint32 keyLength;                        /* Actual key length */
    KeyM_KeyStatusType keyStatus;            /* Current key status */
    uint32 keyVersion;                       /* Key version */
    uint32 validFrom;                        /* Valid from timestamp */
    uint32 validTo;                          /* Valid to timestamp */
    boolean isValid;                         /* Key validity flag */
    boolean isLocked;                        /* Key locked for operation */
    uint32 operationCounter;                 /* Operation counter for rate limiting */
} KeyM_KeyRuntimeType;

/**
 * @brief Key element runtime structure
 */
typedef struct {
    uint8 elementData[KEYM_MAX_KEY_LENGTH]; /* Element data */
    uint32 elementLength;                    /* Element length */
    boolean inUse;                           /* Element in use flag */
} KeyM_KeyElementRuntimeType;

/**
 * @brief Key operation queue element
 */
typedef struct {
    KeyM_KeyIdType keyId;                    /* Key ID for operation */
    uint8 operationType;                     /* Operation type */
    boolean inUse;                           /* Queue slot in use */
    KeyM_OperationResultType result;         /* Operation result */
} KeyM_OperationQueueType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define KEYM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "KeyM_MemMap.h"

/* Key runtime data */
static KeyM_KeyRuntimeType KeyM_Keys[KEYM_NUM_KEYS];

/* Key element storage */
static KeyM_KeyElementRuntimeType KeyM_KeyElements[KEYM_NUM_KEYS][KEYM_MAX_KEY_ELEMENTS];

/* Operation queue for async operations */
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
static KeyM_OperationQueueType KeyM_OpQueue[KEYM_OPERATION_QUEUE_SIZE];
#endif

/* Module state */
boolean KeyM_Initialized = FALSE;
static KeyM_NotificationCallbackType KeyM_NotificationCallback = NULL_PTR;
static uint32 KeyM_SystemTime = 0u;

#define KEYM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "KeyM_MemMap.h"

/*==================================================================================================
 *                                    GLOBAL VARIABLES
 *==================================================================================================*/
#define KEYM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "KeyM_MemMap.h"

const KeyM_ConfigType* KeyM_ConfigPtr = NULL_PTR;

#define KEYM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "KeyM_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/
#define KEYM_START_SEC_CODE
#include "KeyM_MemMap.h"

/**
 * @brief Check if key ID is valid
 */
static boolean KeyM_IsKeyIdValid(KeyM_KeyIdType keyId)
{
    return (keyId < KEYM_NUM_KEYS) ? TRUE : FALSE;
}

/**
 * @brief Check if key element ID is valid
 */
static boolean KeyM_IsKeyElementIdValid(KeyM_KeyElementIdType elementId)
{
    return (elementId < KEYM_MAX_KEY_ELEMENTS) ? TRUE : FALSE;
}

/**
 * @brief Check if key is currently valid (not expired)
 */
static boolean KeyM_IsKeyCurrentlyValid(KeyM_KeyIdType keyId)
{
    if (!KeyM_Keys[keyId].isValid) {
        return FALSE;
    }
    
    /* Check expiry */
    if ((KeyM_Keys[keyId].validTo != 0u) && 
        (KeyM_SystemTime > KeyM_Keys[keyId].validTo)) {
        KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_INVALID;
        return FALSE;
    }
    
    return TRUE;
}

/**
 * @brief Get next available key version
 */
static uint32 KeyM_GetNextKeyVersion(KeyM_KeyIdType keyId)
{
    uint32 currentVersion = KeyM_Keys[keyId].keyVersion;
    
    /* Increment version with wraparound protection */
    if (currentVersion >= KEYM_KEY_VERSION_MASK) {
        return 1u;  /* Wrap to 1 (0 reserved for invalid) */
    }
    return currentVersion + KEYM_KEY_VERSION_INCREMENT;
}

/**
 * @brief Initialize key runtime data
 */
static void KeyM_InitKeyData(KeyM_KeyIdType keyId)
{
    uint32 i;
    uint32 j;
    
    /* Clear key data */
    for (i = 0u; i < KEYM_MAX_KEY_LENGTH; i++) {
        KeyM_Keys[keyId].keyData[i] = 0u;
    }
    
    KeyM_Keys[keyId].keyLength = 0u;
    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_NEW;
    KeyM_Keys[keyId].keyVersion = 0u;
    KeyM_Keys[keyId].validFrom = 0u;
    KeyM_Keys[keyId].validTo = 0u;
    KeyM_Keys[keyId].isValid = FALSE;
    KeyM_Keys[keyId].isLocked = FALSE;
    KeyM_Keys[keyId].operationCounter = 0u;
    
    /* Clear key elements */
    for (j = 0u; j < KEYM_MAX_KEY_ELEMENTS; j++) {
        KeyM_KeyElements[keyId][j].elementLength = 0u;
        KeyM_KeyElements[keyId][j].inUse = FALSE;
        for (i = 0u; i < KEYM_MAX_KEY_LENGTH; i++) {
            KeyM_KeyElements[keyId][j].elementData[i] = 0u;
        }
    }
}

/**
 * @brief Initialize operation queue
 */
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
static void KeyM_InitOperationQueue(void)
{
    uint16 i;
    
    for (i = 0u; i < KEYM_OPERATION_QUEUE_SIZE; i++) {
        KeyM_OpQueue[i].keyId = KEYM_INVALID_KEY_ID;
        KeyM_OpQueue[i].operationType = 0u;
        KeyM_OpQueue[i].inUse = FALSE;
        KeyM_OpQueue[i].result = KEYM_OPRESULT_SUCCESS;
    }
}

/**
 * @brief Queue an operation for async processing
 */
static Std_ReturnType KeyM_QueueOperation(KeyM_KeyIdType keyId, uint8 operationType)
{
    uint16 i;
    
    for (i = 0u; i < KEYM_OPERATION_QUEUE_SIZE; i++) {
        if (!KeyM_OpQueue[i].inUse) {
            KeyM_OpQueue[i].keyId = keyId;
            KeyM_OpQueue[i].operationType = operationType;
            KeyM_OpQueue[i].inUse = TRUE;
            KeyM_OpQueue[i].result = KEYM_OPRESULT_PENDING;
            return E_OK;
        }
    }
    return E_NOT_OK;  /* Queue full */
}
#endif

/**
 * @brief Simple XOR encryption/decryption for key storage protection
 */
static void KeyM_ProtectKeyData(uint8* dataPtr, uint32 length, const uint8* keyPtr)
{
    uint32 i;
    static const uint8 protectionKey[16] = {
        0xA5, 0x5A, 0x3C, 0xC3, 0x69, 0x96, 0xF0, 0x0F,
        0x1E, 0xE1, 0x2D, 0xD2, 0x4B, 0xB4, 0x87, 0x78
    };
    
    for (i = 0u; i < length; i++) {
        dataPtr[i] ^= protectionKey[i % 16u];
        if (keyPtr != NULL_PTR) {
            dataPtr[i] ^= keyPtr[i % 16u];
        }
    }
}

/**
 * @brief Copy key data with protection
 */
static void KeyM_CopyKeyData(uint8* destPtr, const uint8* srcPtr, uint32 length)
{
    uint32 i;
    
    for (i = 0u; i < length; i++) {
        destPtr[i] = srcPtr[i];
    }
}

/**
 * @brief Trigger notification callback
 */
static void KeyM_TriggerNotification(KeyM_KeyIdType keyId, 
                                      KeyM_OperationResultType result,
                                      const uint8* dataPtr,
                                      uint32 dataLength)
{
#if (KEYM_NOTIFICATION_CALLBACK == STD_ON)
    if (KeyM_NotificationCallback != NULL_PTR) {
        KeyM_NotificationCallback(keyId, result, dataPtr, dataLength);
    }
#else
    (void)keyId;
    (void)result;
    (void)dataPtr;
    (void)dataLength;
#endif
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Key Manager module
 */
void KeyM_Init(const KeyM_ConfigType* ConfigPtr)
{
    uint16 i;
    
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == TRUE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_INIT, 
                               KEYM_E_ALREADY_INITIALIZED);
        return;
    }
    
    if (ConfigPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_INIT, 
                               KEYM_E_PARAM_POINTER);
        return;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Initialize all keys */
    for (i = 0u; i < KEYM_NUM_KEYS; i++) {
        KeyM_InitKeyData((KeyM_KeyIdType)i);
    }
    
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    /* Initialize operation queue */
    KeyM_InitOperationQueue();
#endif

    KeyM_ConfigPtr = ConfigPtr;
    KeyM_SystemTime = 0u;
    KeyM_Initialized = TRUE;
    
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
}

/**
 * @brief Deinitializes the Key Manager module
 */
void KeyM_DeInit(void)
{
    uint16 i;
    
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_DEINIT, 
                               KEYM_E_UNINIT);
        return;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Clear all key data */
    for (i = 0u; i < KEYM_NUM_KEYS; i++) {
        KeyM_InitKeyData((KeyM_KeyIdType)i);
    }
    
    KeyM_ConfigPtr = NULL_PTR;
    KeyM_NotificationCallback = NULL_PTR;
    KeyM_Initialized = FALSE;
    
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
}

/**
 * @brief Gets version information
 */
#if (KEYM_VERSION_INFO_API == STD_ON)
void KeyM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETVERSIONINFO, 
                               KEYM_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = KEYM_VENDOR_ID;
    versioninfo->moduleID = KEYM_MODULE_ID;
    versioninfo->sw_major_version = KEYM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = KEYM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = KEYM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Sets a key with the given data
 */
Std_ReturnType KeyM_SetKey(KeyM_KeyIdType keyId,
                            const uint8* keyPtr,
                            uint32 keyLength,
                            KeyM_KeyFormatType keyFormat)
{
    uint32 i;
    
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
    
    if (keyLength > KEYM_MAX_KEY_LENGTH) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
                               KEYM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    (void)keyFormat;  /* Key format handling would be implemented for production */

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check if key is locked */
    if (KeyM_Keys[keyId].isLocked) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Copy key data */
    KeyM_CopyKeyData(KeyM_Keys[keyId].keyData, keyPtr, keyLength);
    KeyM_Keys[keyId].keyLength = keyLength;
    
    /* Set key status to NEW until finalized */
    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_NEW;
    KeyM_Keys[keyId].isValid = FALSE;
    KeyM_Keys[keyId].keyVersion = KeyM_GetNextKeyVersion(keyId);
    
    /* Set validity period */
    KeyM_Keys[keyId].validFrom = KeyM_SystemTime;
    if (KEYM_DEFAULT_KEY_VALIDITY > 0u) {
        KeyM_Keys[keyId].validTo = KeyM_SystemTime + KEYM_DEFAULT_KEY_VALIDITY;
    } else {
        KeyM_Keys[keyId].validTo = 0u;  /* No expiry */
    }

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Gets a key
 */
Std_ReturnType KeyM_GetKey(KeyM_KeyIdType keyId,
                            uint8* keyPtr,
                            uint32* keyLengthPtr,
                            KeyM_KeyFormatType* keyFormatPtr)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((keyPtr == NULL_PTR) || (keyLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETKEY, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check key validity */
    if (!KeyM_IsKeyCurrentlyValid(keyId)) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Check buffer size */
    if (*keyLengthPtr < KeyM_Keys[keyId].keyLength) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Copy key data */
    KeyM_CopyKeyData(keyPtr, KeyM_Keys[keyId].keyData, KeyM_Keys[keyId].keyLength);
    *keyLengthPtr = KeyM_Keys[keyId].keyLength;
    
    if (keyFormatPtr != NULL_PTR) {
        *keyFormatPtr = KEYM_KEY_FORMAT_RAW;
    }

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Updates an existing key
 */
Std_ReturnType KeyM_UpdateKey(KeyM_KeyIdType keyId,
                               const uint8* keyPtr,
                               uint32 keyLength,
                               KeyM_KeyFormatType keyFormat)
{
    Std_ReturnType result;
    
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_UPDATEKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_UPDATEKEY, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_UPDATEKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
    
    if (keyLength > KEYM_MAX_KEY_LENGTH) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_UPDATEKEY, 
                               KEYM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    (void)keyFormat;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check if key is locked */
    if (KeyM_Keys[keyId].isLocked) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Set status to UPDATE */
    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_UPDATE;
    KeyM_Keys[keyId].isValid = FALSE;

    /* Copy new key data */
    KeyM_CopyKeyData(KeyM_Keys[keyId].keyData, keyPtr, keyLength);
    KeyM_Keys[keyId].keyLength = keyLength;
    KeyM_Keys[keyId].keyVersion = KeyM_GetNextKeyVersion(keyId);
    
    /* Update validity period */
    KeyM_Keys[keyId].validFrom = KeyM_SystemTime;
    if (KEYM_DEFAULT_KEY_VALIDITY > 0u) {
        KeyM_Keys[keyId].validTo = KeyM_SystemTime + KEYM_DEFAULT_KEY_VALIDITY;
    } else {
        KeyM_Keys[keyId].validTo = 0u;
    }

    /* Key remains in UPDATE status until finalized */

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Finalizes a key, making it valid for use
 */
Std_ReturnType KeyM_FinalizeKey(KeyM_KeyIdType keyId)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_FINALIZEKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_FINALIZEKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check if key has data */
    if (KeyM_Keys[keyId].keyLength == 0u) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Set key to VALID status */
    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_VALID;
    KeyM_Keys[keyId].isValid = TRUE;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    KeyM_TriggerNotification(keyId, KEYM_OPRESULT_SUCCESS, NULL_PTR, 0u);

    return E_OK;
}

/**
 * @brief Parses key data from a specific format
 */
Std_ReturnType KeyM_ParseKey(KeyM_KeyIdType keyId,
                              const uint8* inputPtr,
                              uint32 inputLength,
                              KeyM_KeyFormatType inputFormat)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_PARSEKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (inputPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_PARSEKEY, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_PARSEKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
    
    if (inputLength > KEYM_MAX_KEY_LENGTH) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_PARSEKEY, 
                               KEYM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check if key is locked */
    if (KeyM_Keys[keyId].isLocked) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* For now, just copy the data (format parsing would be implemented for production) */
    KeyM_CopyKeyData(KeyM_Keys[keyId].keyData, inputPtr, inputLength);
    KeyM_Keys[keyId].keyLength = inputLength;
    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_NEW;
    KeyM_Keys[keyId].isValid = FALSE;
    KeyM_Keys[keyId].keyVersion = KeyM_GetNextKeyVersion(keyId);
    
    /* Set validity period */
    KeyM_Keys[keyId].validFrom = KeyM_SystemTime;
    if (KEYM_DEFAULT_KEY_VALIDITY > 0u) {
        KeyM_Keys[keyId].validTo = KeyM_SystemTime + KEYM_DEFAULT_KEY_VALIDITY;
    } else {
        KeyM_Keys[keyId].validTo = 0u;
    }

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    (void)inputFormat;

    return E_OK;
}

/**
 * @brief Converts key data to a specific format
 */
Std_ReturnType KeyM_ConvertKey(KeyM_KeyIdType keyId,
                                uint8* outputPtr,
                                uint32* outputLengthPtr,
                                KeyM_KeyFormatType outputFormat)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_CONVERTKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((outputPtr == NULL_PTR) || (outputLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_CONVERTKEY, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_CONVERTKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check key validity */
    if (!KeyM_IsKeyCurrentlyValid(keyId)) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Check buffer size */
    if (*outputLengthPtr < KeyM_Keys[keyId].keyLength) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Copy key data (format conversion would be implemented for production) */
    KeyM_CopyKeyData(outputPtr, KeyM_Keys[keyId].keyData, KeyM_Keys[keyId].keyLength);
    *outputLengthPtr = KeyM_Keys[keyId].keyLength;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    (void)outputFormat;

    return E_OK;
}

/**
 * @brief Copies a key from source to destination
 */
Std_ReturnType KeyM_CopyKey(KeyM_KeyIdType srcKeyId,
                             KeyM_KeyIdType destKeyId)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_COPYKEY, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((!KeyM_IsKeyIdValid(srcKeyId)) || (!KeyM_IsKeyIdValid(destKeyId))) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_COPYKEY, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check source key validity */
    if (!KeyM_IsKeyCurrentlyValid(srcKeyId)) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Check if destination key is locked */
    if (KeyM_Keys[destKeyId].isLocked) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Copy key data */
    KeyM_CopyKeyData(KeyM_Keys[destKeyId].keyData, KeyM_Keys[srcKeyId].keyData, 
                     KeyM_Keys[srcKeyId].keyLength);
    KeyM_Keys[destKeyId].keyLength = KeyM_Keys[srcKeyId].keyLength;
    KeyM_Keys[destKeyId].keyStatus = KEYM_KEY_STATUS_NEW;
    KeyM_Keys[destKeyId].isValid = FALSE;
    KeyM_Keys[destKeyId].keyVersion = KeyM_GetNextKeyVersion(destKeyId);
    KeyM_Keys[destKeyId].validFrom = KeyM_Keys[srcKeyId].validFrom;
    KeyM_Keys[destKeyId].validTo = KeyM_Keys[srcKeyId].validTo;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Sets a key element
 */
Std_ReturnType KeyM_KeyElementSet(KeyM_KeyIdType keyId,
                                   KeyM_KeyElementIdType keyElementId,
                                   const uint8* keyElementPtr,
                                   uint32 keyElementLength)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTSET, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyElementPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTSET, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTSET, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyElementIdValid(keyElementId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTSET, 
                               KEYM_E_PARAM_HANDLE);
        return E_NOT_OK;
    }
    
    if (keyElementLength > KEYM_MAX_KEY_LENGTH) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTSET, 
                               KEYM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Copy element data */
    KeyM_CopyKeyData(KeyM_KeyElements[keyId][keyElementId].elementData, 
                     keyElementPtr, keyElementLength);
    KeyM_KeyElements[keyId][keyElementId].elementLength = keyElementLength;
    KeyM_KeyElements[keyId][keyElementId].inUse = TRUE;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Gets a key element
 */
Std_ReturnType KeyM_KeyElementGet(KeyM_KeyIdType keyId,
                                   KeyM_KeyElementIdType keyElementId,
                                   uint8* keyElementPtr,
                                   uint32* keyElementLengthPtr)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTGET, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((keyElementPtr == NULL_PTR) || (keyElementLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTGET, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTGET, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyElementIdValid(keyElementId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYELEMENTGET, 
                               KEYM_E_PARAM_HANDLE);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check if element is in use */
    if (!KeyM_KeyElements[keyId][keyElementId].inUse) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Check buffer size */
    if (*keyElementLengthPtr < KeyM_KeyElements[keyId][keyElementId].elementLength) {
#if (KEYM_ASYNC_OPERATIONS == STD_ON)
        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
        return E_NOT_OK;
    }

    /* Copy element data */
    KeyM_CopyKeyData(keyElementPtr, KeyM_KeyElements[keyId][keyElementId].elementData,
                     KeyM_KeyElements[keyId][keyElementId].elementLength);
    *keyElementLengthPtr = KeyM_KeyElements[keyId][keyElementId].elementLength;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Gets the status of a key
 */
Std_ReturnType KeyM_KeyStatusGet(KeyM_KeyIdType keyId,
                                  KeyM_KeyStatusType* keyStatusPtr)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYSTATUSGET, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyStatusPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYSTATUSGET, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYSTATUSGET, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check and update key validity based on expiry */
    (void)KeyM_IsKeyCurrentlyValid(keyId);
    
    *keyStatusPtr = KeyM_Keys[keyId].keyStatus;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Gets the version of a key
 */
Std_ReturnType KeyM_KeyVersionGet(KeyM_KeyIdType keyId,
                                   uint32* keyVersionPtr)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYVERSIONGET, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyVersionPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYVERSIONGET, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYVERSIONGET, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    *keyVersionPtr = KeyM_Keys[keyId].keyVersion;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Gets the validity information of a key
 */
Std_ReturnType KeyM_KeyValidityGet(KeyM_KeyIdType keyId,
                                    uint32* validFromPtr,
                                    uint32* validToPtr)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYVALIDITYGET, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((validFromPtr == NULL_PTR) || (validToPtr == NULL_PTR)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYVALIDITYGET, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYVALIDITYGET, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    *validFromPtr = KeyM_Keys[keyId].validFrom;
    *validToPtr = KeyM_Keys[keyId].validTo;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Gets key information
 */
Std_ReturnType KeyM_KeyInfoGet(KeyM_KeyIdType keyId,
                                KeyM_KeyInfoType* keyInfoPtr)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYINFOGET, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyInfoPtr == NULL_PTR) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYINFOGET, 
                               KEYM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (!KeyM_IsKeyIdValid(keyId)) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_KEYINFOGET, 
                               KEYM_E_INVALID_KEY);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Check and update key validity */
    (void)KeyM_IsKeyCurrentlyValid(keyId);

    keyInfoPtr->keyId = keyId;
    keyInfoPtr->keyLength = KeyM_Keys[keyId].keyLength * 8u; /* Convert to bits */
    keyInfoPtr->keyStatus = KeyM_Keys[keyId].keyStatus;
    keyInfoPtr->keyVersion = KeyM_Keys[keyId].keyVersion;
    keyInfoPtr->validFrom = KeyM_Keys[keyId].validFrom;
    keyInfoPtr->validTo = KeyM_Keys[keyId].validTo;
    
    /* Get key type from config if available */
    if ((KeyM_ConfigPtr != NULL_PTR) && (KeyM_ConfigPtr->keyConfigs != NULL_PTR) && (keyId < KeyM_ConfigPtr->numKeys)) {
        keyInfoPtr->keyType = KeyM_ConfigPtr->keyConfigs[keyId].keyType;
        keyInfoPtr->keyUsage = KeyM_ConfigPtr->keyConfigs[keyId].allowedUsage;
    } else {
        keyInfoPtr->keyType = KEYM_KEY_TYPE_GENERIC;
        keyInfoPtr->keyUsage = 0xFFu;
    }

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Sets the notification callback
 */
Std_ReturnType KeyM_SetNotificationCallback(KeyM_NotificationCallbackType callback)
{
#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    if (KeyM_Initialized == FALSE) {
        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_CALLBACKNOTIFICATION, 
                               KEYM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    KeyM_NotificationCallback = callback;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    return E_OK;
}

/**
 * @brief Main function for background key operations
 */
void KeyM_MainFunction(void)
{
    uint16 i;
    
    if (KeyM_Initialized == FALSE) {
        return;
    }

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif

    /* Update system time */
    KeyM_SystemTime++;

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    /* Process operation queue */
    for (i = 0u; i < KEYM_OPERATION_QUEUE_SIZE; i++) {
        if (KeyM_OpQueue[i].inUse) {
            /* Process pending operation */
            if (KeyM_OpQueue[i].result == KEYM_OPRESULT_PENDING) {
                /* Simulate operation completion */
                KeyM_OpQueue[i].result = KEYM_OPRESULT_SUCCESS;
                KeyM_TriggerNotification(KeyM_OpQueue[i].keyId, 
                                         KEYM_OPRESULT_SUCCESS, NULL_PTR, 0u);
            }
            /* Clear completed operations */
            if (KeyM_OpQueue[i].result != KEYM_OPRESULT_PENDING) {
                KeyM_OpQueue[i].inUse = FALSE;
            }
        }
    }
#endif

    /* Check for key expiry */
    for (i = 0u; i < KEYM_NUM_KEYS; i++) {
        if (KeyM_Keys[i].isValid) {
            (void)KeyM_IsKeyCurrentlyValid((KeyM_KeyIdType)i);
        }
    }

#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
#endif
}

#define KEYM_STOP_SEC_CODE
#include "KeyM_MemMap.h"
