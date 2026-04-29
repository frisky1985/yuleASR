/*==================================================================================================
 *                                CRYPTO SERVICES MANAGER (Csm)
 *==================================================================================================
 * FILENAME: Csm.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_CryptoServicesManager.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Crypto Services Manager module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Csm.h"
#include "Det.h"
#include "SchM_Csm.h"

/*==================================================================================================
 *                                    VERSION CHECK
 *==================================================================================================*/
#if (CSM_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Csm.c: AR major version mismatch"
#endif

#if (CSM_AR_RELEASE_MINOR_VERSION != 7u)
    #error "Csm.c: AR minor version mismatch"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define CSM_AES_BLOCK_SIZE              (16u)
#define CSM_SHA256_SIZE                 (32u)
#define CSM_SHA512_SIZE                 (64u)
#define CSM_SHA1_SIZE                   (20u)
#define CSM_HMAC_SIZE                   (32u)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/
typedef struct {
    Csm_JobIdType jobId;
    Csm_JobStateType state;
    Csm_OperationModeType mode;
    uint8 retryCount;
    boolean active;
} Csm_JobRuntimeType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

static Csm_JobRuntimeType Csm_Jobs[CSM_NUM_JOBS];
static Csm_QueueElementType Csm_Queue[CSM_JOB_QUEUE_SIZE];
static Csm_CallbackType Csm_Callback = NULL_PTR;
static boolean Csm_Initialized = FALSE;

/* Simulated key storage */
static uint8 Csm_KeyStorage[CSM_NUM_KEYS][64u];  /* Max 512 bits */
static boolean Csm_KeyValid[CSM_NUM_KEYS];

#define CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

/*==================================================================================================
 *                                    GLOBAL VARIABLES
 *==================================================================================================*/
#define CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

boolean Csm_Initialized_Global = FALSE;
const Csm_ConfigType* Csm_ConfigPtr = NULL_PTR;

#define CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Csm_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/
#define CSM_START_SEC_CODE
#include "Csm_MemMap.h"

/**
 * @brief Find job index by job ID
 */
static sint16 Csm_FindJobIndex(Csm_JobIdType jobId)
{
    uint16 i;
    
    for (i = 0u; i < CSM_NUM_JOBS; i++) {
        if (Csm_Jobs[i].jobId == jobId) {
            return (sint16)i;
        }
    }
    return -1;
}

/**
 * @brief Find key index by key ID
 */
static sint16 Csm_FindKeyIndex(Csm_KeyIdType keyId)
{
    if (keyId < CSM_NUM_KEYS) {
        return (sint16)keyId;
    }
    return -1;
}

/**
 * @brief Queue a job
 */
static Std_ReturnType Csm_QueueJob(Csm_JobIdType jobId, Csm_OperationModeType mode, 
                                    Csm_JobPrimitiveInputOutputType* inputOutput)
{
    uint16 i;
    
    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
        if (!Csm_Queue[i].inUse) {
            Csm_Queue[i].jobId = jobId;
            Csm_Queue[i].mode = mode;
            Csm_Queue[i].inputOutput = inputOutput;
            Csm_Queue[i].inUse = TRUE;
            return E_OK;
        }
    }
    return E_NOT_OK;  /* Queue full */
}

/**
 * @brief Simple XOR encryption (placeholder for real crypto)
 */
static void Csm_XorEncrypt(const uint8* input, uint8* output, uint32 length, const uint8* key)
{
    uint32 i;
    for (i = 0u; i < length; i++) {
        output[i] = input[i] ^ key[i % CSM_AES_BLOCK_SIZE];
    }
}

/**
 * @brief Simple XOR decryption (placeholder for real crypto)
 */
static void Csm_XorDecrypt(const uint8* input, uint8* output, uint32 length, const uint8* key)
{
    uint32 i;
    for (i = 0u; i < length; i++) {
        output[i] = input[i] ^ key[i % CSM_AES_BLOCK_SIZE];
    }
}

/**
 * @brief Simple hash calculation (placeholder for SHA-256)
 */
static void Csm_CalculateHash(const uint8* input, uint32 length, uint8* output, uint32 outputLength)
{
    uint32 i;
    uint32 hash = 0x12345678u;
    
    /* Simple hash algorithm (placeholder) */
    for (i = 0u; i < length; i++) {
        hash = ((hash << 5) + hash) + input[i];
    }
    
    /* Fill output */
    for (i = 0u; i < outputLength; i++) {
        output[i] = (uint8)(hash >> ((i % 4) * 8));
    }
}

/**
 * @brief Generate random bytes
 */
static void Csm_GenerateRandom(uint8* output, uint32 length)
{
    uint32 i;
    static uint32 seed = 0x12345678u;
    
    for (i = 0u; i < length; i++) {
        seed = (seed * 1103515245u + 12345u) & 0x7FFFFFFFu;
        output[i] = (uint8)(seed ^ (seed >> 8) ^ (seed >> 16) ^ (seed >> 24));
    }
}

/**
 * @brief Process a crypto job
 */
static Std_ReturnType Csm_ProcessJob(Csm_JobIdType jobId, Csm_OperationModeType mode,
                                      Csm_JobPrimitiveInputOutputType* inputOutput)
{
    sint16 jobIndex;
    Std_ReturnType result = E_NOT_OK;
    
    jobIndex = Csm_FindJobIndex(jobId);
    if (jobIndex < 0) {
        return E_NOT_OK;
    }
    
    /* Update job state */
    Csm_Jobs[jobIndex].state = CSM_JOB_STATE_PROGRESSING;
    Csm_Jobs[jobIndex].mode = mode;
    
    /* Process based on job type */
    switch (jobId) {
        case CSM_JOB_ID_ENCRYPT_1:
            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
                (inputOutput->outputPtr != NULL_PTR)) {
                Csm_XorEncrypt(inputOutput->inputPtr, inputOutput->outputPtr,
                               inputOutput->inputLength, Csm_KeyStorage[CSM_KEY_ID_AES_128]);
                if (inputOutput->outputLengthPtr != NULL_PTR) {
                    *inputOutput->outputLengthPtr = inputOutput->inputLength;
                }
                result = E_OK;
            }
            break;
            
        case CSM_JOB_ID_DECRYPT_1:
            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
                (inputOutput->outputPtr != NULL_PTR)) {
                Csm_XorDecrypt(inputOutput->inputPtr, inputOutput->outputPtr,
                               inputOutput->inputLength, Csm_KeyStorage[CSM_KEY_ID_AES_128]);
                if (inputOutput->outputLengthPtr != NULL_PTR) {
                    *inputOutput->outputLengthPtr = inputOutput->inputLength;
                }
                result = E_OK;
            }
            break;
            
        case CSM_JOB_ID_MAC_GENERATE_1:
            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
                (inputOutput->outputPtr != NULL_PTR)) {
                Csm_CalculateHash(inputOutput->inputPtr, inputOutput->inputLength,
                                  inputOutput->outputPtr, CSM_HMAC_SIZE);
                if (inputOutput->outputLengthPtr != NULL_PTR) {
                    *inputOutput->outputLengthPtr = CSM_HMAC_SIZE;
                }
                result = E_OK;
            }
            break;
            
        case CSM_JOB_ID_HASH_SHA256:
            if ((inputOutput != NULL_PTR) && (inputOutput->inputPtr != NULL_PTR) && 
                (inputOutput->outputPtr != NULL_PTR)) {
                Csm_CalculateHash(inputOutput->inputPtr, inputOutput->inputLength,
                                  inputOutput->outputPtr, CSM_SHA256_SIZE);
                if (inputOutput->outputLengthPtr != NULL_PTR) {
                    *inputOutput->outputLengthPtr = CSM_SHA256_SIZE;
                }
                result = E_OK;
            }
            break;
            
        case CSM_JOB_ID_RANDOM_GENERATE:
            if ((inputOutput != NULL_PTR) && (inputOutput->outputPtr != NULL_PTR)) {
                Csm_GenerateRandom(inputOutput->outputPtr, inputOutput->outputLength);
                result = E_OK;
            }
            break;
            
        default:
            result = E_NOT_OK;
            break;
    }
    
    /* Update job state */
    Csm_Jobs[jobIndex].state = (result == E_OK) ? CSM_JOB_STATE_COMPLETED : CSM_JOB_STATE_FAILED;
    
    /* Trigger callback if configured */
#if (CSM_CALLBACK_SUPPORTED == STD_ON)
    if (Csm_Callback != NULL_PTR) {
        Csm_Callback(jobId, Csm_Jobs[jobIndex].state, 
                      inputOutput != NULL_PTR ? inputOutput->outputPtr : NULL_PTR,
                      inputOutput != NULL_PTR && inputOutput->outputLengthPtr != NULL_PTR ? 
                      *inputOutput->outputLengthPtr : 0u);
    }
#endif
    
    return result;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Crypto Services Manager module
 */
void Csm_Init(const Csm_ConfigType* ConfigPtr)
{
    uint16 i;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == TRUE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_INIT, CSM_E_ALREADY_INITIALIZED);
        return;
    }
    
    if (ConfigPtr == NULL_PTR) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_INIT, CSM_E_PARAM_POINTER);
        return;
    }
#endif

    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    
    /* Initialize jobs */
    for (i = 0u; i < CSM_NUM_JOBS; i++) {
        Csm_Jobs[i].jobId = (Csm_JobIdType)i;
        Csm_Jobs[i].state = CSM_JOB_STATE_IDLE;
        Csm_Jobs[i].mode = CSM_OPERATIONMODE_START;
        Csm_Jobs[i].retryCount = 0u;
        Csm_Jobs[i].active = FALSE;
    }
    
    /* Initialize queue */
    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
        Csm_Queue[i].inUse = FALSE;
    }
    
    /* Initialize key storage */
    for (i = 0u; i < CSM_NUM_KEYS; i++) {
        Csm_KeyValid[i] = FALSE;
    }
    
    Csm_ConfigPtr = ConfigPtr;
    Csm_Initialized = TRUE;
    Csm_Initialized_Global = TRUE;
    
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
}

/**
 * @brief Deinitializes the Crypto Services Manager module
 */
void Csm_DeInit(void)
{
    uint16 i;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DEINIT, CSM_E_UNINIT);
        return;
    }
#endif

    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    
    /* Reset all jobs */
    for (i = 0u; i < CSM_NUM_JOBS; i++) {
        Csm_Jobs[i].state = CSM_JOB_STATE_IDLE;
        Csm_Jobs[i].active = FALSE;
    }
    
    /* Clear queue */
    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
        Csm_Queue[i].inUse = FALSE;
    }
    
    /* Clear key validity */
    for (i = 0u; i < CSM_NUM_KEYS; i++) {
        Csm_KeyValid[i] = FALSE;
    }
    
    Csm_ConfigPtr = NULL_PTR;
    Csm_Initialized = FALSE;
    Csm_Initialized_Global = FALSE;
    Csm_Callback = NULL_PTR;
    
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
}

/**
 * @brief Gets version information
 */
#if (CSM_VERSION_INFO_API == STD_ON)
void Csm_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_GETVERSIONINFO, CSM_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = CSM_VENDOR_ID;
    versioninfo->moduleID = CSM_MODULE_ID;
    versioninfo->sw_major_version = CSM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CSM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CSM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Encrypts data
 */
Std_ReturnType Csm_Encrypt(Csm_JobIdType jobId,
                            Csm_OperationModeType mode,
                            const uint8* dataPtr,
                            uint32 dataLength,
                            uint8* resultPtr,
                            uint32* resultLengthPtr)
{
    Csm_JobPrimitiveInputOutputType inputOutput;
    Std_ReturnType result;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_ENCRYPT, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((dataPtr == NULL_PTR) || (resultPtr == NULL_PTR) || (resultLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_ENCRYPT, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    inputOutput.inputPtr = (uint8*)dataPtr;
    inputOutput.inputLength = dataLength;
    inputOutput.outputPtr = resultPtr;
    inputOutput.outputLengthPtr = resultLengthPtr;
    inputOutput.secondaryInputPtr = NULL_PTR;
    inputOutput.secondaryInputLength = 0u;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    result = Csm_ProcessJob(jobId, mode, &inputOutput);
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Decrypts data
 */
Std_ReturnType Csm_Decrypt(Csm_JobIdType jobId,
                            Csm_OperationModeType mode,
                            const uint8* dataPtr,
                            uint32 dataLength,
                            uint8* resultPtr,
                            uint32* resultLengthPtr)
{
    Csm_JobPrimitiveInputOutputType inputOutput;
    Std_ReturnType result;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DECRYPT, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((dataPtr == NULL_PTR) || (resultPtr == NULL_PTR) || (resultLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_DECRYPT, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    inputOutput.inputPtr = (uint8*)dataPtr;
    inputOutput.inputLength = dataLength;
    inputOutput.outputPtr = resultPtr;
    inputOutput.outputLengthPtr = resultLengthPtr;
    inputOutput.secondaryInputPtr = NULL_PTR;
    inputOutput.secondaryInputLength = 0u;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    result = Csm_ProcessJob(jobId, mode, &inputOutput);
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Generates MAC
 */
Std_ReturnType Csm_MacGenerate(Csm_JobIdType jobId,
                                Csm_OperationModeType mode,
                                const uint8* dataPtr,
                                uint32 dataLength,
                                uint8* macPtr,
                                uint32* macLengthPtr)
{
    Csm_JobPrimitiveInputOutputType inputOutput;
    Std_ReturnType result;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MACGENERATE, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((dataPtr == NULL_PTR) || (macPtr == NULL_PTR) || (macLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MACGENERATE, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    inputOutput.inputPtr = (uint8*)dataPtr;
    inputOutput.inputLength = dataLength;
    inputOutput.outputPtr = macPtr;
    inputOutput.outputLengthPtr = macLengthPtr;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    result = Csm_ProcessJob(jobId, mode, &inputOutput);
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Verifies MAC
 */
Std_ReturnType Csm_MacVerify(Csm_JobIdType jobId,
                              Csm_OperationModeType mode,
                              const uint8* dataPtr,
                              uint32 dataLength,
                              const uint8* macPtr,
                              uint32 macLength,
                              Csm_VerifyResultType* verifyPtr)
{
    uint8 calculatedMac[CSM_HMAC_SIZE];
    uint32 calculatedMacLength = CSM_HMAC_SIZE;
    Std_ReturnType result;
    uint32 i;
    boolean match = TRUE;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MACVERIFY, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((dataPtr == NULL_PTR) || (macPtr == NULL_PTR) || (verifyPtr == NULL_PTR)) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MACVERIFY, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Generate MAC */
    result = Csm_MacGenerate(CSM_JOB_ID_MAC_GENERATE_1, mode, dataPtr, dataLength,
                              calculatedMac, &calculatedMacLength);
    
    if (result == E_OK) {
        /* Compare MACs */
        if (macLength != calculatedMacLength) {
            match = FALSE;
        } else {
            for (i = 0u; i < macLength; i++) {
                if (macPtr[i] != calculatedMac[i]) {
                    match = FALSE;
                    break;
                }
            }
        }
        
        *verifyPtr = match ? CSM_E_VER_OK : CSM_E_VER_NOT_OK;
    }
    
    return result;
}

/**
 * @brief Calculates hash
 */
Std_ReturnType Csm_Hash(Csm_JobIdType jobId,
                         Csm_OperationModeType mode,
                         const uint8* dataPtr,
                         uint32 dataLength,
                         uint8* resultPtr,
                         uint32* resultLengthPtr)
{
    Csm_JobPrimitiveInputOutputType inputOutput;
    Std_ReturnType result;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_HASH, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((dataPtr == NULL_PTR) || (resultPtr == NULL_PTR) || (resultLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_HASH, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    inputOutput.inputPtr = (uint8*)dataPtr;
    inputOutput.inputLength = dataLength;
    inputOutput.outputPtr = resultPtr;
    inputOutput.outputLengthPtr = resultLengthPtr;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    result = Csm_ProcessJob(jobId, mode, &inputOutput);
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Generates random number
 */
Std_ReturnType Csm_RandomGenerate(Csm_JobIdType jobId,
                                   uint8* resultPtr,
                                   uint32 resultLength)
{
    Csm_JobPrimitiveInputOutputType inputOutput;
    Std_ReturnType result;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_RANDOMGENERATE, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (resultPtr == NULL_PTR) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_RANDOMGENERATE, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (resultLength > CSM_RANDOM_MAX_SIZE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_RANDOMGENERATE, CSM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    inputOutput.outputPtr = resultPtr;
    inputOutput.outputLength = resultLength;
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    result = Csm_ProcessJob(jobId, CSM_OPERATIONMODE_STREAMSTART, &inputOutput);
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return result;
}

/**
 * @brief Sets key element
 */
Std_ReturnType Csm_KeyElementSet(Csm_KeyIdType keyId,
                                  uint32 keyElementId,
                                  const uint8* keyPtr,
                                  uint32 keyLength)
{
    sint16 keyIndex;
    uint32 i;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_KEYELEMENTSET, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyPtr == NULL_PTR) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_KEYELEMENTSET, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (keyLength > 64u) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_KEYELEMENTSET, CSM_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    keyIndex = Csm_FindKeyIndex(keyId);
    if (keyIndex < 0) {
        return E_NOT_OK;
    }
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    
    /* Copy key data */
    for (i = 0u; i < keyLength; i++) {
        Csm_KeyStorage[keyIndex][i] = keyPtr[i];
    }
    
    Csm_KeyValid[keyIndex] = FALSE;  /* Key not valid until Csm_KeySetValid */
    
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return E_OK;
}

/**
 * @brief Sets key as valid
 */
Std_ReturnType Csm_KeySetValid(Csm_KeyIdType keyId)
{
    sint16 keyIndex;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_KEYSETVALID, CSM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    keyIndex = Csm_FindKeyIndex(keyId);
    if (keyIndex < 0) {
        return E_NOT_OK;
    }
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    Csm_KeyValid[keyIndex] = TRUE;
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return E_OK;
}

/**
 * @brief Gets key element
 */
Std_ReturnType Csm_KeyElementGet(Csm_KeyIdType keyId,
                                  uint32 keyElementId,
                                  uint8* keyPtr,
                                  uint32* keyLengthPtr)
{
    sint16 keyIndex;
    uint32 i;
    uint32 copyLength;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_KEYELEMENTGET, CSM_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((keyPtr == NULL_PTR) || (keyLengthPtr == NULL_PTR)) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_KEYELEMENTGET, CSM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    keyIndex = Csm_FindKeyIndex(keyId);
    if (keyIndex < 0) {
        return E_NOT_OK;
    }
    
    if (!Csm_KeyValid[keyIndex]) {
        return E_NOT_OK;
    }
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    
    /* Copy key data */
    copyLength = (*keyLengthPtr < 64u) ? *keyLengthPtr : 64u;
    for (i = 0u; i < copyLength; i++) {
        keyPtr[i] = Csm_KeyStorage[keyIndex][i];
    }
    *keyLengthPtr = copyLength;
    
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return E_OK;
}

/**
 * @brief Cancels a job
 */
Std_ReturnType Csm_CancelJob(Csm_JobIdType jobId)
{
    sint16 jobIndex;
    uint16 i;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_CANCELJOB, CSM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    jobIndex = Csm_FindJobIndex(jobId);
    if (jobIndex < 0) {
        return E_NOT_OK;
    }
    
    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    
    /* Reset job state */
    Csm_Jobs[jobIndex].state = CSM_JOB_STATE_IDLE;
    Csm_Jobs[jobIndex].active = FALSE;
    
    /* Remove from queue if present */
    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
        if ((Csm_Queue[i].inUse) && (Csm_Queue[i].jobId == jobId)) {
            Csm_Queue[i].inUse = FALSE;
            break;
        }
    }
    
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
    
    return E_OK;
}

/**
 * @brief Main function for job processing
 */
void Csm_MainFunction(void)
{
    uint16 i;
    
#if (CSM_DEV_ERROR_DETECT == STD_ON)
    if (Csm_Initialized == FALSE) {
        (void)Det_ReportError(CSM_MODULE_ID, CSM_INSTANCE_ID, CSM_SID_MAINFUNCTION, CSM_E_UNINIT);
        return;
    }
#endif

    SchM_Enter_Csm_CSM_EXCLUSIVE_AREA_0();
    
    /* Process queued jobs */
    for (i = 0u; i < CSM_JOB_QUEUE_SIZE; i++) {
        if (Csm_Queue[i].inUse) {
            (void)Csm_ProcessJob(Csm_Queue[i].jobId, Csm_Queue[i].mode, Csm_Queue[i].inputOutput);
            Csm_Queue[i].inUse = FALSE;
        }
    }
    
    SchM_Exit_Csm_CSM_EXCLUSIVE_AREA_0();
}

#define CSM_STOP_SEC_CODE
#include "Csm_MemMap.h"
