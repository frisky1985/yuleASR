/**
 * @file Csm.c
 * @brief Crypto Services Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2| *                                CRYPTO SERVICES MANAGER (Csm)
     3| *==================================================================================================
     4| * FILENAME: Csm.c
     5| * AUTOSAR VERSION: R22-11
     6| * DOCUMENT: AUTOSAR_SWS_CryptoServicesManager.pdf
     7| *==================================================================================================
     8| * PROJECT: yuleASR Classic AUTOSAR BSW
     9| * DESCRIPTION: Implementation of Crypto Services Manager module
    10| *==================================================================================================
    11| */
    12|
    13|/*==================================================================================================
    14| *                                         INCLUDES
    15| *==================================================================================================*/
    16|#include "Csm.h"
    17|#include "Det.h"
    18|#include "SchM_Csm.h"
    19|
    20|/*==================================================================================================
    21| *                                    VERSION CHECK
    22| *==================================================================================================*/
    23|#if (CSM_AR_RELEASE_MAJOR_VERSION != 4u)
    24|    #error "Csm.c: AR major version mismatch"
    25|#endif
    26|
    27|#if (CSM_AR_RELEASE_MINOR_VERSION != 7u)
    28|    #error "Csm.c: AR minor version mismatch"
    29|#endif
    30|
    31|/*==================================================================================================
    32| *                                    LOCAL DEFINES
    33| *==================================================================================================*/
    34|#define CSM_AES_BLOCK_SIZE              (16u)
    35|#define CSM_SHA256_SIZE                 (32u)
    36|#define CSM_SHA512_SIZE                 (64u)
    37|#define CSM_SHA1_SIZE                   (20u)
    38|#define CSM_HMAC_SIZE                   (32u)
    39|
    40|/*==================================================================================================
    41| *                                    LOCAL TYPES
    42| *==================================================================================================*/
    43|typedef struct {
    44|    Csm_JobIdType jobId;
    45|    Csm_JobStateType state;
    46|    Csm_OperationModeType mode;
    47|    uint8 retryCount;
    48|    boolean active;
    49|} Csm_JobRuntimeType;
    50|
    51|/*==================================================================================================
    52| *                                    LOCAL VARIABLES
    53| *==================================================================================================*/
    54|#define CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    55|#include "Csm_MemMap.h"
    56|
    57|static Csm_JobRuntimeType Csm_Jobs[CSM_NUM_JOBS];
    58|static Csm_QueueElementType Csm_Queue[CSM_JOB_QUEUE_SIZE];
    59|static Csm_CallbackType Csm_Callback = NULL_PTR;
    60|static boolean Csm_Initialized = FALSE;
    61|
    62|/* Simulated key storage */
    63|static uint8 Csm_KeyStorage[CSM_NUM_KEYS][64u];  /* Max 512 bits */
    64|static boolean Csm_KeyValid[CSM_NUM_KEYS];
    65|
    66|#define CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    67|#include "Csm_MemMap.h"
    68|
    69|/*==================================================================================================
    70| *                                    GLOBAL VARIABLES
    71| *==================================================================================================*/
    72|#define CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    73|#include "Csm_MemMap.h"
    74|
    75|boolean Csm_Initialized_Global = FALSE;
    76|const Csm_ConfigType* Csm_ConfigPtr = NULL_PTR;
    77|
    78|#define CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    79|#include "Csm_MemMap.h"
    80|
    81|/*==================================================================================================
    82| *                                    LOCAL FUNCTIONS
    83| *==================================================================================================*/
    84|#define CSM_START_SEC_CODE
    85|#include "Csm_MemMap.h"
    86|
    87|/**
    88| * @brief Find job index by job ID
    89| */
    90|static sint16 Csm_FindJobIndex(Csm_JobIdType jobId)
    91|{
    92|    uint16 i;
    93|    
    94|    for (i = 0u; i < CSM_NUM_JOBS; i++) {
    95|        if (Csm_Jobs[i].jobId == jobId) {
    96|            return (sint16)i;
    97|        }
    98|    }
    99|    return -1;
   100|}
   101|
   102|/**
   103| * @brief Find key index by key ID
   104| */
   105|static sint16 Csm_FindKeyIndex(Csm_KeyIdType keyId)
   106|{
   107|    if (keyId < CSM_NUM_KEYS) {
   108|        return (sint16)keyId;
   109|    }
   110|    return -1;
   111|}
   112|
   113|/**
   114| * @brief Queue a job
   115| */
   116|static Std_ReturnType Csm_QueueJob(Csm_JobIdType jobId, Csm_OperationModeType mode, 
   117|                                    Csm_JobPrimitiveInputOutputType* inputOutput)
   118|{
   119|    uint16 i;
   120|    
   121|    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
   122|        if (!Csm_Queue[i].inUse) {
   123|            Csm_Queue[i].jobId = jobId;
   124|            Csm_Queue[i].mode = mode;
   125|            Csm_Queue[i].inputOutput = inputOutput;
   126|            Csm_Queue[i].inUse = TRUE;
   127|            return E_OK;
   128|        }
   129|    }
   130|    return E_NOT_OK;  /* Queue full */
   131|}
   132|
   133|/**
   134| * @brief Simple XOR encryption (placeholder for real crypto)
   135| */
   136|static void Csm_XorEncrypt(const uint8* input, uint8* output, uint32 length, const uint8* key)
   137|{
   138|    uint32 i;
   139|    for (i = 0u; i < length; i++) {
   140|        output[i] = input[i] ^ key[i % CSM_AES_BLOCK_SIZE];
   141|    }
   142|}
   143|
   144|/**
   145| * @brief Simple XOR decryption (placeholder for real crypto)
   146| */
   147|static void Csm_XorDecrypt(const uint8* input, uint8* output, uint32 length, const uint8* key)
   148|{
   149|    uint32 i;
   150|    for (i = 0u; i < length; i++) {
   151|        output[i] = input[i] ^ key[i % CSM_AES_BLOCK_SIZE];
   152|    }
   153|}
   154|
   155|/**
   156| * @brief Simple hash calculation (placeholder for SHA-256)
   157| */
   158|static void Csm_CalculateHash(const uint8* input, uint32 length, uint8* output, uint32 outputLength)
   159|{
   160|    uint32 i;
   161|    uint32 hash = 0x12345678u;
   162|    
   163|    /* Simple hash algorithm (placeholder) */
   164|    for (i = 0u; i < length; i++) {
   165|        hash = ((hash << 5) + hash) + input[i];
   166|    }
   167|    
   168|    /* Fill output */
   169|    for (i = 0u; i < outputLength; i++) {
   170|        output[i] = (uint8)(hash >> ((i % 4) * 8));
   171|    }
   172|}
   173|
   174|/**
   175| * @brief Generate random bytes
   176| */
   177|static void Csm_GenerateRandom(uint8* output, uint32 length)
   178|{
   179|    uint32 i;
   180|    static uint32 seed = 0x12345678u;
   181|    
   182|    for (i = 0u; i < length; i++) {
   183|        seed = (seed * 1103515245u + 12345u) & 0x7FFFFFFFu;
   184|        output[i] = (uint8)(seed ^ (seed >> 8) ^ (seed >> 16) ^ (seed >> 24));
   185|    }
   186|}
   187|
   188|/**
   189| * @brief Process a crypto job
   190| */
   191|static Std_ReturnType Csm_ProcessJob(Csm_JobIdType jobId, Csm_OperationModeType mode,
   192|                                      Csm_JobPrimitiveInputOutputType* inputOutput)
   193|{
   194|    sint16 jobIndex;
   195|    Std_ReturnType result = E_NOT_OK;
   196|    
   197|    jobIndex = Csm_FindJobIndex(jobId);
   198|    if (jobIndex < 0) {
   199|        return E_NOT_OK;
   200|    }
   201|    
   202|    /* Update job state */
   203|    Csm_Jobs[jobIndex].state = CSM_JOB_STATE_PROGRESSING;
   204|    Csm_Jobs[jobIndex].mode = mode;
   205|    
   206|    /* Process based on job type */
   207|    switch (jobId) {
   208|        case CSM_JOB_ID_ENCRYPT_1:
   209|            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
   210|                (inputOutput->outputPtr != NULL_PTR)) {
   211|                Csm_XorEncrypt(inputOutput->inputPtr, inputOutput->outputPtr,
   212|                               inputOutput->inputLength, Csm_KeyStorage[CSM_KEY_ID_AES_128]);
   213|                if (inputOutput->outputLengthPtr != NULL_PTR) {
   214|                    *inputOutput->outputLengthPtr = inputOutput->inputLength;
   215|                }
   216|                result = E_OK;
   217|            }
   218|            break;
   219|            
   220|        case CSM_JOB_ID_DECRYPT_1:
   221|            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
   222|                (inputOutput->outputPtr != NULL_PTR)) {
   223|                Csm_XorDecrypt(inputOutput->inputPtr, inputOutput->outputPtr,
   224|                               inputOutput->inputLength, Csm_KeyStorage[CSM_KEY_ID_AES_128]);
   225|                if (inputOutput->outputLengthPtr != NULL_PTR) {
   226|                    *inputOutput->outputLengthPtr = inputOutput->inputLength;
   227|                }
   228|                result = E_OK;
   229|            }
   230|            break;
   231|            
   232|        case CSM_JOB_ID_MAC_GENERATE_1:
   233|            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
   234|                (inputOutput->outputPtr != NULL_PTR)) {
   235|                Csm_CalculateHash(inputOutput->inputPtr, inputOutput->inputLength,
   236|                                  inputOutput->outputPtr, CSM_HMAC_SIZE);
   237|                if (inputOutput->outputLengthPtr != NULL_PTR) {
   238|                    *inputOutput->outputLengthPtr = CSM_HMAC_SIZE;
   239|                }
   240|                result = E_OK;
   241|            }
   242|            break;
   243|            
   244|        case CSM_JOB_ID_HASH_SHA256:
   245|            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
   246|                (inputOutput->outputPtr != NULL_PTR)) {
   247|                Csm_CalculateHash(inputOutput->inputPtr, inputOutput->inputLength,
   248|                                  inputOutput->outputPtr, CSM_SHA256_SIZE);
   249|                if (inputOutput->outputLengthPtr != NULL_PTR) {
   250|                    *inputOutput->outputLengthPtr = CSM_SHA256_SIZE;
   251|                }
   252|                result = E_OK;
   253|            }
   254|            break;
   255|            
   256|        case CSM_JOB_ID_RANDOM_GENERATE:
   257|            if ((inputOutput != NULL_PTR) && (inputOutput->outputPtr != NULL_PTR)) {
   258|                Csm_GenerateRandom(inputOutput->outputPtr, inputOutput->outputLength);
   259|                result = E_OK;
   260|            }
   261|            break;
   262|            
   263|        default:
   264|            result = E_NOT_OK;
   265|            break;
   266|    }
   267|    
   268|    /* Update job state */
   269|    Csm_Jobs[jobIndex].state = (result == E_OK) ? CSM_JOB_STATE_COMPLETED : CSM_JOB_STATE_FAILED;
   270|    
   271|    /* Trigger callback if configured */
   272|#if (CSM_CALLBACK_SUPPORTED == STD_ON)
   273|    if (Csm_Callback != NULL_PTR) {
   274|        Csm_Callback(jobId, Csm_Jobs[jobIndex].state, 
   275|                      inputOutput != NULL_PTR ? inputOutput->outputPtr : NULL_PTR,
   276|                      inputOutput != NULL_PTR && inputOutput->outputLengthPtr != NULL_PTR ? 
   277|                      *inputOutput->outputLengthPtr : 0u);
   278|    }
   279|#endif
   280|    
   281|    return result;
   282|}
   283|
   284|/*==================================================================================================
   285| *                                    GLOBAL FUNCTIONS
   286| *==================================================================================================*/
   287|
   288|/**
   289| * @brief Initializes the Crypto Services Manager module
   290| */
   291|void Csm_Init(const Csm_ConfigType* ConfigPtr)
   292|{
   293|    uint16 i;
   294|    
   295|#if (CSM_DEV_ERROR_DETECT == STD_ON)
   296|    if (Csm_Initialized == TRUE) {
   297|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_INIT, CSM_E_ALREADY_INITIALIZED);
   298|        return;
   299|    }
   300|    
   301|    if (ConfigPtr == NULL_PTR) {
   302|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_INIT, CSM_E_PARAM_POINTER);
   303|        return;
   304|    }
   305|#endif
   306|
   307|    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
   308|    
   309|    /* Initialize jobs */
   310|    for (i = 0u; i < CSM_NUM_JOBS; i++) {
   311|        Csm_Jobs[i].jobId = (Csm_JobIdType)i;
   312|        Csm_Jobs[i].state = CSM_JOB_STATE_IDLE;
   313|        Csm_Jobs[i].mode = CSM_OPERATIONMODE_START;
   314|        Csm_Jobs[i].retryCount = 0u;
   315|        Csm_Jobs[i].active = FALSE;
   316|    }
   317|    
   318|    /* Initialize queue */
   319|    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
   320|        Csm_Queue[i].inUse = FALSE;
   321|    }
   322|    
   323|    /* Initialize key storage */
   324|    for (i = 0u; i < CSM_NUM_KEYS; i++) {
   325|        Csm_KeyValid[i] = FALSE;
   326|    }
   327|    
   328|    Csm_ConfigPtr = ConfigPtr;
   329|    Csm_Initialized = TRUE;
   330|    Csm_Initialized_Global = TRUE;
   331|    
   332|    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
   333|}
   334|
   335|/**
   336| * @brief Deinitializes the Crypto Services Manager module
   337| */
   338|void Csm_DeInit(void)
   339|{
   340|    uint16 i;
   341|    
   342|#if (CSM_DEV_ERROR_DETECT == STD_ON)
   343|    if (Csm_Initialized == FALSE) {
   344|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DEINIT, CSM_E_UNINIT);
   345|        return;
   346|    }
   347|#endif
   348|
   349|    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
   350|    
   351|    /* Reset all jobs */
   352|    for (i = 0u; i < CSM_NUM_JOBS; i++) {
   353|        Csm_Jobs[i].state = CSM_JOB_STATE_IDLE;
   354|        Csm_Jobs[i].active = FALSE;
   355|    }
   356|    
   357|    /* Clear queue */
   358|    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
   359|        Csm_Queue[i].inUse = FALSE;
   360|    }
   361|    
   362|    /* Clear key validity */
   363|    for (i = 0u; i < CSM_NUM_KEYS; i++) {
   364|        Csm_KeyValid[i] = FALSE;
   365|    }
   366|    
   367|    Csm_ConfigPtr = NULL_PTR;
   368|    Csm_Initialized = FALSE;
   369|    Csm_Initialized_Global = FALSE;
   370|    Csm_Callback = NULL_PTR;
   371|    
   372|    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
   373|}
   374|
   375|/**
   376| * @brief Gets version information
   377| */
   378|#if (CSM_VERSION_INFO_API == STD_ON)
   379|void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo)
   380|{
   381|#if (CSM_DEV_ERROR_DETECT == STD_ON)
   382|    if (versioninfo == NULL_PTR) {
   383|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_GETVERSIONINFO, CSM_E_PARAM_POINTER);
   384|        return;
   385|    }
   386|#endif
   387|
   388|    versioninfo->vendorID = CSM_VENDOR_ID;
   389|    versioninfo->moduleID = CSM_MODULE_ID;
   390|    versioninfo->sw_major_version = CSM_SW_MAJOR_VERSION;
   391|    versioninfo->sw_minor_version = CSM_SW_MINOR_VERSION;
   392|    versioninfo->sw_patch_version = CSM_SW_PATCH_VERSION;
   393|}
   394|#endif
   395|
   396|/**
   397| * @brief Encrypts data
   398| */
   399|Std_ReturnType Csm_Encrypt(Csm_JobIdType jobId,
   400|                            Csm_OperationModeType mode,
   401|                            const uint8* dataPtr,
   402|                            uint32 dataLength,
   403|                            uint8* resultPtr,
   404|                            uint32* resultLengthPtr)
   405|{
   406|    Csm_JobPrimitiveInputOutputType inputOutput;
   407|    Std_ReturnType result;
   408|    
   409|#if (CSM_DEV_ERROR_DETECT == STD_ON)
   410|    if (Csm_Initialized == FALSE) {
   411|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_ENCRYPT, CSM_E_UNINIT);
   412|        return E_NOT_OK;
   413|    }
   414|    
   415|    if ((dataPtr == NULL_PTR) || (resultPtr == NULL_PTR) || (resultLengthPtr == NULL_PTR)) {
   416|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_ENCRYPT, CSM_E_PARAM_POINTER);
   417|        return E_NOT_OK;
   418|    }
   419|#endif
   420|
   421|    inputOutput.inputPtr = (uint8*)dataPtr;
   422|    inputOutput.inputLength = dataLength;
   423|    inputOutput.outputPtr = resultPtr;
   424|    inputOutput.outputLengthPtr = resultLengthPtr;
   425|    inputOutput.secondaryInputPtr = NULL_PTR;
   426|    inputOutput.secondaryInputLength = 0u;
   427|    
   428|    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
   429|    result = Csm_ProcessJob(jobId, mode, &inputOutput);
   430|    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
   431|    
   432|    return result;
   433|}
   434|
   435|/**
   436| * @brief Decrypts data
   437| */
   438|Std_ReturnType Csm_Decrypt(Csm_JobIdType jobId,
   439|                            Csm_OperationModeType mode,
   440|                            const uint8* dataPtr,
   441|                            uint32 dataLength,
   442|                            uint8* resultPtr,
   443|                            uint32* resultLengthPtr)
   444|{
   445|    Csm_JobPrimitiveInputOutputType inputOutput;
   446|    Std_ReturnType result;
   447|    
   448|#if (CSM_DEV_ERROR_DETECT == STD_ON)
   449|    if (Csm_Initialized == FALSE) {
   450|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DECRYPT, CSM_E_UNINIT);
   451|        return E_NOT_OK;
   452|    }
   453|    
   454|    if ((dataPtr == NULL_PTR) || (resultPtr == NULL_PTR) || (resultLengthPtr == NULL_PTR)) {
   455|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DECRYPT, CSM_E_PARAM_POINTER);
   456|        return E_NOT_OK;
   457|    }
   458|#endif
   459|
   460|    inputOutput.inputPtr = (uint8*)dataPtr;
   461|    inputOutput.inputLength = dataLength;
   462|    inputOutput.outputPtr = resultPtr;
   463|    inputOutput.outputLengthPtr = resultLengthPtr;
   464|    inputOutput.secondaryInputPtr = NULL_PTR;
   465|    inputOutput.secondaryInputLength = 0u;
   466|    
   467|    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
   468|    result = Csm_ProcessJob(jobId, mode, &inputOutput);
   469|    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
   470|    
   471|    return result;
   472|}
   473|
   474|/**
   475| * @brief Generates MAC
   476| */
   477|Std_ReturnType Csm_MacGenerate(Csm_JobIdType jobId,
   478|                                Csm_OperationModeType mode,
   479|                                const uint8* dataPtr,
   480|                                uint32 dataLength,
   481|                                uint8* macPtr,
   482|                                uint32* macLengthPtr)
   483|{
   484|    Csm_JobPrimitiveInputOutputType inputOutput;
   485|    Std_ReturnType result;
   486|    
   487|#if (CSM_DEV_ERROR_DETECT == STD_ON)
   488|    if (Csm_Initialized == FALSE) {
   489|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MACGENERATE, CSM_E_UNINIT);
   490|        return E_NOT_OK;
   491|    }
   492|    
   493|    if ((dataPtr == NULL_PTR) || (macPtr == NULL_PTR) || (macLengthPtr == NULL_PTR)) {
   494|        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MACGENERATE, CSM_E_PARAM_POINTER);
   495|        return E_NOT_OK;
   496|    }
   497|#endif
   498|
   499|    inputOutput.inputPtr = (uint8*)dataPtr;
   500|    inputOutput.inputLength = dataLength;
   501|