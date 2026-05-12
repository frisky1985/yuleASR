/**
 * @file KeyM.c
 * @brief Key Manager Implementation
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2| *                                KEY MANAGER (KeyM)
     3| *==================================================================================================
     4| * FILENAME: KeyM.c
     5| * AUTOSAR VERSION: R22-11
     6| * DOCUMENT: AUTOSAR_SWS_KeyManager.pdf
     7| *==================================================================================================
     8| * PROJECT: yuleASR Classic AUTOSAR BSW
     9| * DESCRIPTION: Implementation of Key Manager module
    10| *==================================================================================================
    11| */
    12|
    13|/*==================================================================================================
    14| *                                         INCLUDES
    15| *==================================================================================================*/
    16|#include "KeyM.h"
    17|#include "KeyM_MemMap.h"
    18|#if (KEYM_DEV_ERROR_DETECT == STD_ON)
    19|#include "Det.h"
    20|#endif
    21|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    22|#include "SchM_KeyM.h"
    23|#endif
    24|
    25|/*==================================================================================================
    26| *                                    VERSION CHECK
    27| *==================================================================================================*/
    28|#if (KEYM_AR_RELEASE_MAJOR_VERSION != 4u)
    29|    #error "KeyM.c: AR major version mismatch"
    30|#endif
    31|
    32|#if (KEYM_AR_RELEASE_MINOR_VERSION != 7u)
    33|    #error "KeyM.c: AR minor version mismatch"
    34|#endif
    35|
    36|/*==================================================================================================
    37| *                                    LOCAL DEFINES
    38| *==================================================================================================*/
    39|#define KEYM_KEY_VERSION_INCREMENT      (1u)
    40|#define KEYM_KEY_STATUS_MASK            (0x03u)
    41|#define KEYM_MAX_KEY_STORAGE_SIZE       (KEYM_NUM_KEYS * KEYM_MAX_KEY_LENGTH)
    42|
    43|/*==================================================================================================
    44| *                                    LOCAL TYPES
    45| *==================================================================================================*/
    46|
    47|/**
    48| * @brief Key runtime information structure
    49| */
    50|typedef struct {
    51|    uint8 keyData[KEYM_MAX_KEY_LENGTH];     /* Key material */
    52|    uint32 keyLength;                        /* Actual key length */
    53|    KeyM_KeyStatusType keyStatus;            /* Current key status */
    54|    uint32 keyVersion;                       /* Key version */
    55|    uint32 validFrom;                        /* Valid from timestamp */
    56|    uint32 validTo;                          /* Valid to timestamp */
    57|    boolean isValid;                         /* Key validity flag */
    58|    boolean isLocked;                        /* Key locked for operation */
    59|    uint32 operationCounter;                 /* Operation counter for rate limiting */
    60|} KeyM_KeyRuntimeType;
    61|
    62|/**
    63| * @brief Key element runtime structure
    64| */
    65|typedef struct {
    66|    uint8 elementData[KEYM_MAX_KEY_LENGTH]; /* Element data */
    67|    uint32 elementLength;                    /* Element length */
    68|    boolean inUse;                           /* Element in use flag */
    69|} KeyM_KeyElementRuntimeType;
    70|
    71|/**
    72| * @brief Key operation queue element
    73| */
    74|typedef struct {
    75|    KeyM_KeyIdType keyId;                    /* Key ID for operation */
    76|    uint8 operationType;                     /* Operation type */
    77|    boolean inUse;                           /* Queue slot in use */
    78|    KeyM_OperationResultType result;         /* Operation result */
    79|} KeyM_OperationQueueType;
    80|
    81|/*==================================================================================================
    82| *                                    LOCAL VARIABLES
    83| *==================================================================================================*/
    84|#define KEYM_START_SEC_VAR_CLEARED_UNSPECIFIED
    85|#include "KeyM_MemMap.h"
    86|
    87|/* Key runtime data */
    88|static KeyM_KeyRuntimeType KeyM_Keys[KEYM_NUM_KEYS];
    89|
    90|/* Key element storage */
    91|static KeyM_KeyElementRuntimeType KeyM_KeyElements[KEYM_NUM_KEYS][KEYM_MAX_KEY_ELEMENTS];
    92|
    93|/* Operation queue for async operations */
    94|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
    95|static KeyM_OperationQueueType KeyM_OpQueue[KEYM_OPERATION_QUEUE_SIZE];
    96|#endif
    97|
    98|/* Module state */
    99|boolean KeyM_Initialized = FALSE;
   100|static KeyM_NotificationCallbackType KeyM_NotificationCallback = NULL_PTR;
   101|static uint32 KeyM_SystemTime = 0u;
   102|
   103|#define KEYM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
   104|#include "KeyM_MemMap.h"
   105|
   106|/*==================================================================================================
   107| *                                    GLOBAL VARIABLES
   108| *==================================================================================================*/
   109|#define KEYM_START_SEC_VAR_CLEARED_UNSPECIFIED
   110|#include "KeyM_MemMap.h"
   111|
   112|const KeyM_ConfigType* KeyM_ConfigPtr = NULL_PTR;
   113|
   114|#define KEYM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
   115|#include "KeyM_MemMap.h"
   116|
   117|/*==================================================================================================
   118| *                                    LOCAL FUNCTIONS
   119| *==================================================================================================*/
   120|#define KEYM_START_SEC_CODE
   121|#include "KeyM_MemMap.h"
   122|
   123|/**
   124| * @brief Check if key ID is valid
   125| */
   126|static boolean KeyM_IsKeyIdValid(KeyM_KeyIdType keyId)
   127|{
   128|    return (keyId < KEYM_NUM_KEYS) ? TRUE : FALSE;
   129|}
   130|
   131|/**
   132| * @brief Check if key element ID is valid
   133| */
   134|static boolean KeyM_IsKeyElementIdValid(KeyM_KeyElementIdType elementId)
   135|{
   136|    return (elementId < KEYM_MAX_KEY_ELEMENTS) ? TRUE : FALSE;
   137|}
   138|
   139|/**
   140| * @brief Check if key is currently valid (not expired)
   141| */
   142|static boolean KeyM_IsKeyCurrentlyValid(KeyM_KeyIdType keyId)
   143|{
   144|    if (!KeyM_Keys[keyId].isValid) {
   145|        return FALSE;
   146|    }
   147|    
   148|    /* Check expiry */
   149|    if ((KeyM_Keys[keyId].validTo != 0u) && 
   150|        (KeyM_SystemTime > KeyM_Keys[keyId].validTo)) {
   151|        KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_INVALID;
   152|        return FALSE;
   153|    }
   154|    
   155|    return TRUE;
   156|}
   157|
   158|/**
   159| * @brief Get next available key version
   160| */
   161|static uint32 KeyM_GetNextKeyVersion(KeyM_KeyIdType keyId)
   162|{
   163|    uint32 currentVersion = KeyM_Keys[keyId].keyVersion;
   164|    
   165|    /* Increment version with wraparound protection */
   166|    if (currentVersion >= KEYM_KEY_VERSION_MASK) {
   167|        return 1u;  /* Wrap to 1 (0 reserved for invalid) */
   168|    }
   169|    return currentVersion + KEYM_KEY_VERSION_INCREMENT;
   170|}
   171|
   172|/**
   173| * @brief Initialize key runtime data
   174| */
   175|static void KeyM_InitKeyData(KeyM_KeyIdType keyId)
   176|{
   177|    uint32 i;
   178|    uint32 j;
   179|    
   180|    /* Clear key data */
   181|    for (i = 0u; i < KEYM_MAX_KEY_LENGTH; i++) {
   182|        KeyM_Keys[keyId].keyData[i] = 0u;
   183|    }
   184|    
   185|    KeyM_Keys[keyId].keyLength = 0u;
   186|    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_NEW;
   187|    KeyM_Keys[keyId].keyVersion = 0u;
   188|    KeyM_Keys[keyId].validFrom = 0u;
   189|    KeyM_Keys[keyId].validTo = 0u;
   190|    KeyM_Keys[keyId].isValid = FALSE;
   191|    KeyM_Keys[keyId].isLocked = FALSE;
   192|    KeyM_Keys[keyId].operationCounter = 0u;
   193|    
   194|    /* Clear key elements */
   195|    for (j = 0u; j < KEYM_MAX_KEY_ELEMENTS; j++) {
   196|        KeyM_KeyElements[keyId][j].elementLength = 0u;
   197|        KeyM_KeyElements[keyId][j].inUse = FALSE;
   198|        for (i = 0u; i < KEYM_MAX_KEY_LENGTH; i++) {
   199|            KeyM_KeyElements[keyId][j].elementData[i] = 0u;
   200|        }
   201|    }
   202|}
   203|
   204|/**
   205| * @brief Initialize operation queue
   206| */
   207|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   208|static void KeyM_InitOperationQueue(void)
   209|{
   210|    uint16 i;
   211|    
   212|    for (i = 0u; i < KEYM_OPERATION_QUEUE_SIZE; i++) {
   213|        KeyM_OpQueue[i].keyId = KEYM_INVALID_KEY_ID;
   214|        KeyM_OpQueue[i].operationType = 0u;
   215|        KeyM_OpQueue[i].inUse = FALSE;
   216|        KeyM_OpQueue[i].result = KEYM_OPRESULT_SUCCESS;
   217|    }
   218|}
   219|
   220|/**
   221| * @brief Queue an operation for async processing
   222| */
   223|static Std_ReturnType KeyM_QueueOperation(KeyM_KeyIdType keyId, uint8 operationType)
   224|{
   225|    uint16 i;
   226|    
   227|    for (i = 0u; i < KEYM_OPERATION_QUEUE_SIZE; i++) {
   228|        if (!KeyM_OpQueue[i].inUse) {
   229|            KeyM_OpQueue[i].keyId = keyId;
   230|            KeyM_OpQueue[i].operationType = operationType;
   231|            KeyM_OpQueue[i].inUse = TRUE;
   232|            KeyM_OpQueue[i].result = KEYM_OPRESULT_PENDING;
   233|            return E_OK;
   234|        }
   235|    }
   236|    return E_NOT_OK;  /* Queue full */
   237|}
   238|#endif
   239|
   240|/**
   241| * @brief Simple XOR encryption/decryption for key storage protection
   242| */
   243|static void KeyM_ProtectKeyData(uint8* dataPtr, uint32 length, const uint8* keyPtr)
   244|{
   245|    uint32 i;
   246|    static const uint8 protectionKey[16] = {
   247|        0xA5, 0x5A, 0x3C, 0xC3, 0x69, 0x96, 0xF0, 0x0F,
   248|        0x1E, 0xE1, 0x2D, 0xD2, 0x4B, 0xB4, 0x87, 0x78
   249|    };
   250|    
   251|    for (i = 0u; i < length; i++) {
   252|        dataPtr[i] ^= protectionKey[i % 16u];
   253|        if (keyPtr != NULL_PTR) {
   254|            dataPtr[i] ^= keyPtr[i % 16u];
   255|        }
   256|    }
   257|}
   258|
   259|/**
   260| * @brief Copy key data with protection
   261| */
   262|static void KeyM_CopyKeyData(uint8* destPtr, const uint8* srcPtr, uint32 length)
   263|{
   264|    uint32 i;
   265|    
   266|    for (i = 0u; i < length; i++) {
   267|        destPtr[i] = srcPtr[i];
   268|    }
   269|}
   270|
   271|/**
   272| * @brief Trigger notification callback
   273| */
   274|static void KeyM_TriggerNotification(KeyM_KeyIdType keyId, 
   275|                                      KeyM_OperationResultType result,
   276|                                      const uint8* dataPtr,
   277|                                      uint32 dataLength)
   278|{
   279|#if (KEYM_NOTIFICATION_CALLBACK == STD_ON)
   280|    if (KeyM_NotificationCallback != NULL_PTR) {
   281|        KeyM_NotificationCallback(keyId, result, dataPtr, dataLength);
   282|    }
   283|#else
   284|    (void)keyId;
   285|    (void)result;
   286|    (void)dataPtr;
   287|    (void)dataLength;
   288|#endif
   289|}
   290|
   291|/*==================================================================================================
   292| *                                    GLOBAL FUNCTIONS
   293| *==================================================================================================*/
   294|
   295|/**
   296| * @brief Initializes the Key Manager module
   297| */
   298|void KeyM_Init(const KeyM_ConfigType* ConfigPtr)
   299|{
   300|    uint16 i;
   301|    
   302|#if (KEYM_DEV_ERROR_DETECT == STD_ON)
   303|    if (KeyM_Initialized == TRUE) {
   304|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_INIT, 
   305|                               KEYM_E_ALREADY_INITIALIZED);
   306|        return;
   307|    }
   308|    
   309|    if (ConfigPtr == NULL_PTR) {
   310|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_INIT, 
   311|                               KEYM_E_PARAM_POINTER);
   312|        return;
   313|    }
   314|#endif
   315|
   316|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   317|    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
   318|#endif
   319|
   320|    /* Initialize all keys */
   321|    for (i = 0u; i < KEYM_NUM_KEYS; i++) {
   322|        KeyM_InitKeyData((KeyM_KeyIdType)i);
   323|    }
   324|    
   325|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   326|    /* Initialize operation queue */
   327|    KeyM_InitOperationQueue();
   328|#endif
   329|
   330|    KeyM_ConfigPtr = ConfigPtr;
   331|    KeyM_SystemTime = 0u;
   332|    KeyM_Initialized = TRUE;
   333|    
   334|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   335|    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
   336|#endif
   337|}
   338|
   339|/**
   340| * @brief Deinitializes the Key Manager module
   341| */
   342|void KeyM_DeInit(void)
   343|{
   344|    uint16 i;
   345|    
   346|#if (KEYM_DEV_ERROR_DETECT == STD_ON)
   347|    if (KeyM_Initialized == FALSE) {
   348|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_DEINIT, 
   349|                               KEYM_E_UNINIT);
   350|        return;
   351|    }
   352|#endif
   353|
   354|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   355|    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
   356|#endif
   357|
   358|    /* Clear all key data */
   359|    for (i = 0u; i < KEYM_NUM_KEYS; i++) {
   360|        KeyM_InitKeyData((KeyM_KeyIdType)i);
   361|    }
   362|    
   363|    KeyM_ConfigPtr = NULL_PTR;
   364|    KeyM_NotificationCallback = NULL_PTR;
   365|    KeyM_Initialized = FALSE;
   366|    
   367|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   368|    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
   369|#endif
   370|}
   371|
   372|/**
   373| * @brief Gets version information
   374| */
   375|#if (KEYM_VERSION_INFO_API == STD_ON)
   376|void KeyM_GetVersionInfo(Std_VersionInfoType* versioninfo)
   377|{
   378|#if (KEYM_DEV_ERROR_DETECT == STD_ON)
   379|    if (versioninfo == NULL_PTR) {
   380|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETVERSIONINFO, 
   381|                               KEYM_E_PARAM_POINTER);
   382|        return;
   383|    }
   384|#endif
   385|
   386|    versioninfo->vendorID = KEYM_VENDOR_ID;
   387|    versioninfo->moduleID = KEYM_MODULE_ID;
   388|    versioninfo->sw_major_version = KEYM_SW_MAJOR_VERSION;
   389|    versioninfo->sw_minor_version = KEYM_SW_MINOR_VERSION;
   390|    versioninfo->sw_patch_version = KEYM_SW_PATCH_VERSION;
   391|}
   392|#endif
   393|
   394|/**
   395| * @brief Sets a key with the given data
   396| */
   397|Std_ReturnType KeyM_SetKey(KeyM_KeyIdType keyId,
   398|                            const uint8* keyPtr,
   399|                            uint32 keyLength,
   400|                            KeyM_KeyFormatType keyFormat)
   401|{
   402|    uint32 i;
   403|    
   404|#if (KEYM_DEV_ERROR_DETECT == STD_ON)
   405|    if (KeyM_Initialized == FALSE) {
   406|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
   407|                               KEYM_E_UNINIT);
   408|        return E_NOT_OK;
   409|    }
   410|    
   411|    if (keyPtr == NULL_PTR) {
   412|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
   413|                               KEYM_E_PARAM_POINTER);
   414|        return E_NOT_OK;
   415|    }
   416|    
   417|    if (!KeyM_IsKeyIdValid(keyId)) {
   418|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
   419|                               KEYM_E_INVALID_KEY);
   420|        return E_NOT_OK;
   421|    }
   422|    
   423|    if (keyLength > KEYM_MAX_KEY_LENGTH) {
   424|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_SETKEY, 
   425|                               KEYM_E_PARAM_LENGTH);
   426|        return E_NOT_OK;
   427|    }
   428|#endif
   429|
   430|    (void)keyFormat;  /* Key format handling would be implemented for production */
   431|
   432|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   433|    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
   434|#endif
   435|
   436|    /* Check if key is locked */
   437|    if (KeyM_Keys[keyId].isLocked) {
   438|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   439|        SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
   440|#endif
   441|        return E_NOT_OK;
   442|    }
   443|
   444|    /* Copy key data */
   445|    KeyM_CopyKeyData(KeyM_Keys[keyId].keyData, keyPtr, keyLength);
   446|    KeyM_Keys[keyId].keyLength = keyLength;
   447|    
   448|    /* Set key status to NEW until finalized */
   449|    KeyM_Keys[keyId].keyStatus = KEYM_KEY_STATUS_NEW;
   450|    KeyM_Keys[keyId].isValid = FALSE;
   451|    KeyM_Keys[keyId].keyVersion = KeyM_GetNextKeyVersion(keyId);
   452|    
   453|    /* Set validity period */
   454|    KeyM_Keys[keyId].validFrom = KeyM_SystemTime;
   455|    if (KEYM_DEFAULT_KEY_VALIDITY > 0u) {
   456|        KeyM_Keys[keyId].validTo = KeyM_SystemTime + KEYM_DEFAULT_KEY_VALIDITY;
   457|    } else {
   458|        KeyM_Keys[keyId].validTo = 0u;  /* No expiry */
   459|    }
   460|
   461|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   462|    SchM_Exit_KeyM_KEYM_EXCLUSIVE_AREA_0();
   463|#endif
   464|
   465|    return E_OK;
   466|}
   467|
   468|/**
   469| * @brief Gets a key
   470| */
   471|Std_ReturnType KeyM_GetKey(KeyM_KeyIdType keyId,
   472|                            uint8* keyPtr,
   473|                            uint32* keyLengthPtr,
   474|                            KeyM_KeyFormatType* keyFormatPtr)
   475|{
   476|#if (KEYM_DEV_ERROR_DETECT == STD_ON)
   477|    if (KeyM_Initialized == FALSE) {
   478|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETKEY, 
   479|                               KEYM_E_UNINIT);
   480|        return E_NOT_OK;
   481|    }
   482|    
   483|    if ((keyPtr == NULL_PTR) || (keyLengthPtr == NULL_PTR)) {
   484|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETKEY, 
   485|                               KEYM_E_PARAM_POINTER);
   486|        return E_NOT_OK;
   487|    }
   488|    
   489|    if (!KeyM_IsKeyIdValid(keyId)) {
   490|        (void)Det_ReportError(KEYM_MODULE_ID, KEYM_INSTANCE_ID, KEYM_SID_GETKEY, 
   491|                               KEYM_E_INVALID_KEY);
   492|        return E_NOT_OK;
   493|    }
   494|#endif
   495|
   496|#if (KEYM_ASYNC_OPERATIONS == STD_ON)
   497|    SchM_Enter_KeyM_KEYM_EXCLUSIVE_AREA_0();
   498|#endif
   499|
   500|    /* Check key validity */
   501|