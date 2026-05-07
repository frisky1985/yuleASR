/**********************************************************************************************************************
 * @file       Crypto.c
 * @brief      Crypto Driver Core Implementation
 * @author     YuleTech AutoSAR Team
 * @version    1.0.0
 * @date       2025-05-01
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *
 * @description
 *      Core implementation of the Crypto Driver supporting both software (Mbed TLS)
 *      and hardware (HSM) cryptographic operations. Implements CCC Digital Key
 *      required algorithms: ECDSA, ECDH, AES-128-GCM, SHA-256, HKDF, HMAC.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * INCLUDES
 *********************************************************************************************************************/
#include "Crypto.h"
#include "Det.h"
#include "MemMap.h"

/**********************************************************************************************************************
 * LOCAL MACROS
 *********************************************************************************************************************/
#define CRYPTO_INIT_MAGIC                   (0x43525950U)   /* "CRYP" */
#define CRYPTO_JOB_ID_INVALID               (0xFFFFFFFFU)

/**********************************************************************************************************************
 * LOCAL DATA TYPES
 *********************************************************************************************************************/
typedef enum {
    CRYPTO_DRIVER_UNINIT = 0,
    CRYPTO_DRIVER_INIT,
    CRYPTO_DRIVER_BUSY
} Crypto_DriverStateType;

/**********************************************************************************************************************
 * LOCAL VARIABLES
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Crypto_DriverStateType Crypto_DriverState = CRYPTO_DRIVER_UNINIT;
STATIC const Crypto_ConfigType* Crypto_ConfigPtr = NULL_PTR;
STATIC uint32 Crypto_InitMagic = 0U;

#define CRYPTO_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define CRYPTO_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Crypto_QueueElementType* Crypto_JobQueueHead = NULL_PTR;
STATIC Crypto_QueueElementType* Crypto_JobQueueTail = NULL_PTR;
STATIC uint32 Crypto_QueueCount = 0U;

#define CRYPTO_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define CRYPTO_START_SEC_VAR_CLEARED_BOOLEAN
#include "MemMap.h"

STATIC boolean Crypto_HsmAvailable = FALSE;

#define CRYPTO_STOP_SEC_VAR_CLEARED_BOOLEAN
#include "MemMap.h"

/**********************************************************************************************************************
 * EXTERNAL DECLARATIONS
 *********************************************************************************************************************/
extern Std_ReturnType Crypto_MbedTLS_Init(void);
extern void Crypto_MbedTLS_DeInit(void);
extern Std_ReturnType Crypto_MbedTLS_ProcessJob(Crypto_JobType* job);
extern Std_ReturnType Crypto_MbedTLS_RandomGenerate(uint8* resultPtr, uint32 resultLength);
extern Std_ReturnType Crypto_MbedTLS_KeyGenerate(Crypto_KeyIdType keyId);
extern Std_ReturnType Crypto_MbedTLS_KeyDerive(Crypto_KeyIdType srcKeyId, Crypto_KeyIdType dstKeyId);
extern Std_ReturnType Crypto_MbedTLS_ECDH_CalcSecret(Crypto_KeyIdType privKeyId, const uint8* pubKeyPtr, uint32 pubKeyLen);
extern Std_ReturnType Crypto_MbedTLS_ECDSA_Sign(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen, uint8* sig, uint32* sigLen);
extern Std_ReturnType Crypto_MbedTLS_ECDSA_Verify(Crypto_KeyIdType keyId, const uint8* digest, uint32 digestLen, const uint8* sig, uint32 sigLen);
extern Std_ReturnType Crypto_MbedTLS_AES_GCM_Encrypt(Crypto_KeyIdType keyId, const uint8* pt, uint32 ptLen, const uint8* aad, uint32 aadLen, const uint8* iv, uint8* ct, uint8* tag);
extern Std_ReturnType Crypto_MbedTLS_AES_GCM_Decrypt(Crypto_KeyIdType keyId, const uint8* ct, uint32 ctLen, const uint8* aad, uint32 aadLen, const uint8* iv, const uint8* tag, uint8* pt);
extern Std_ReturnType Crypto_MbedTLS_SHA256(const uint8* data, uint32 dataLen, uint8* digest);
extern Std_ReturnType Crypto_MbedTLS_HMAC(Crypto_KeyIdType keyId, const uint8* data, uint32 dataLen, uint8* mac);
extern Std_ReturnType Crypto_MbedTLS_HKDF(Crypto_KeyIdType ikmKeyId, const uint8* salt, uint32 saltLen, const uint8* info, uint32 infoLen, uint8* okm, uint32 okmLen);

#if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
extern Std_ReturnType Crypto_Hsm_Init(const Crypto_HsmConfigType* config);
extern void Crypto_Hsm_DeInit(void);
extern boolean Crypto_Hsm_IsAvailable(void);
extern Crypto_HsmStateType Crypto_Hsm_GetState(void);
extern Std_ReturnType Crypto_Hsm_ProcessJob(Crypto_JobType* job);
extern Std_ReturnType Crypto_Hsm_LoadKey(Crypto_KeyIdType keyId);
extern Std_ReturnType Crypto_Hsm_SelfTest(void);
#endif

/**********************************************************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************************/
STATIC Std_ReturnType Crypto_ValidateJob(Crypto_JobType* job);
STATIC Std_ReturnType Crypto_QueuePush(Crypto_JobType* job);
STATIC Crypto_JobType* Crypto_QueuePop(void);
STATIC Std_ReturnType Crypto_ProcessJobInternal(Crypto_JobType* job);
STATIC Std_ReturnType Crypto_ProcessService(Crypto_JobType* job);

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - STANDARD AUTOSAR API
 *********************************************************************************************************************/

#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * Crypto_Init
 *********************************************************************************************************************/
void Crypto_Init(const Crypto_ConfigType* configPtr)
{
    uint32 i;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (configPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_INIT, CRYPTO_E_PARAM_POINTER);
        return;
    }
    #endif
    
    if (Crypto_DriverState != CRYPTO_DRIVER_UNINIT) {
        #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_INIT, CRYPTO_E_ALREADY_INITIALIZED);
        #endif
        return;
    }
    
    Crypto_ConfigPtr = configPtr;
    Crypto_JobQueueHead = NULL_PTR;
    Crypto_JobQueueTail = NULL_PTR;
    Crypto_QueueCount = 0U;
    
    /* Initialize Mbed TLS software implementation */
    if (Crypto_MbedTLS_Init() != E_OK) {
        #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_MBEDTLS_INIT, CRYPTO_E_NOT_SUPPORTED);
        #endif
        return;
    }
    
    /* Initialize HSM if enabled */
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    if (Crypto_Hsm_Init(&configPtr->hsmConfig) == E_OK) {
        if (Crypto_Hsm_IsAvailable()) {
            Crypto_HsmAvailable = TRUE;
            (void)Crypto_Hsm_SelfTest();
        }
    }
    #endif
    
    Crypto_InitMagic = CRYPTO_INIT_MAGIC;
    Crypto_DriverState = CRYPTO_DRIVER_INIT;
}

/**********************************************************************************************************************
 * Crypto_DeInit
 *********************************************************************************************************************/
void Crypto_DeInit(void)
{
    if (Crypto_DriverState == CRYPTO_DRIVER_UNINIT) {
        return;
    }
    
    /* Clear job queue */
    while (Crypto_JobQueueHead != NULL_PTR) {
        (void)Crypto_QueuePop();
    }
    
    /* Deinitialize subsystems */
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    Crypto_Hsm_DeInit();
    #endif
    
    Crypto_MbedTLS_DeInit();
    
    Crypto_DriverState = CRYPTO_DRIVER_UNINIT;
    Crypto_InitMagic = 0U;
    Crypto_ConfigPtr = NULL_PTR;
    Crypto_HsmAvailable = FALSE;
}

/**********************************************************************************************************************
 * Crypto_GetVersionInfo
 *********************************************************************************************************************/
#if (CRYPTO_CFG_VERSION_INFO_API == STD_ON)
void Crypto_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_DEINIT, CRYPTO_E_PARAM_POINTER);
        return;
    }
    #endif
    
    versioninfo->vendorID = CRYPTO_VENDOR_ID;
    versioninfo->moduleID = CRYPTO_MODULE_ID;
    versioninfo->sw_major_version = CRYPTO_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CRYPTO_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CRYPTO_SW_PATCH_VERSION;
}
#endif

/**********************************************************************************************************************
 * Crypto_ProcessJob
 *********************************************************************************************************************/
Std_ReturnType Crypto_ProcessJob(uint32 objectId, Crypto_JobType* job)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, (uint8)objectId, CRYPTO_SID_PROCESSJOB, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (job == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, (uint8)objectId, CRYPTO_SID_PROCESSJOB, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (objectId >= Crypto_ConfigPtr->numChannels) {
        Det_ReportError(CRYPTO_MODULE_ID, (uint8)objectId, CRYPTO_SID_PROCESSJOB, CRYPTO_E_PARAM_HANDLE);
        return E_NOT_OK;
    }
    #endif
    
    if (Crypto_ValidateJob(job) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check if synchronous or asynchronous processing */
    if (job->jobPrimitiveInfo->processingType == CRYPTO_PROCESSING_SYNC) {
        /* Process immediately */
        result = Crypto_ProcessJobInternal(job);
    } else {
        /* Queue for async processing */
        if (Crypto_QueueCount >= Crypto_ConfigPtr->queueSize) {
            #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(CRYPTO_MODULE_ID, (uint8)objectId, CRYPTO_SID_PROCESSJOB, CRYPTO_E_QUEUE_FULL);
            #endif
            result = E_NOT_OK;
        } else {
            result = Crypto_QueuePush(job);
        }
    }
    
    return result;
}

/**********************************************************************************************************************
 * Crypto_CancelJob
 *********************************************************************************************************************/
Std_ReturnType Crypto_CancelJob(uint32 objectId, Crypto_JobType* job)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, (uint8)objectId, CRYPTO_SID_CANCELJOB, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (job == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, (uint8)objectId, CRYPTO_SID_CANCELJOB, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    /* Mark job as cancelled - actual removal from queue would require more complex handling */
    job->jobState = CRYPTO_E_JOB_CANCELED;
    
    if (job->jobPrimitiveInfo->callbackId != 0U) {
        Crypto_JobNotification(job, CRYPTO_E_JOB_CANCELED);
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * KEY MANAGEMENT FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_KeyElementSet
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyElementSet(Crypto_KeyIdType cryptoKeyId,
                                     Crypto_KeyElementIdType keyElementId,
                                     const uint8* keyPtr,
                                     uint32 keyLength)
{
    Crypto_KeyType* key;
    Crypto_KeyElementType* element;
    uint32 i;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (keyLength == 0U) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    #endif
    
    if (cryptoKeyId >= Crypto_ConfigPtr->numKeys) {
        return E_NOT_OK;
    }
    
    key = &Crypto_ConfigPtr->keys[cryptoKeyId];
    element = NULL_PTR;
    
    /* Find the key element */
    for (i = 0U; i < key->numElements; i++) {
        if (key->keyElements[i].id == keyElementId) {
            element = &key->keyElements[i];
            break;
        }
    }
    
    if (element == NULL_PTR) {
        return E_NOT_OK;
    }
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if ((!element->allowPartialAccess) && (keyLength > element->size)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTSET, CRYPTO_E_SMALL_BUFFER);
        return E_NOT_OK;
    }
    #endif
    
    /* Copy key data */
    for (i = 0U; i < keyLength && i < element->size; i++) {
        element->data[i] = keyPtr[i];
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_KeyElementGet
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyElementGet(Crypto_KeyIdType cryptoKeyId,
                                     Crypto_KeyElementIdType keyElementId,
                                     uint8* keyPtr,
                                     uint32* keyLengthPtr)
{
    Crypto_KeyType* key;
    Crypto_KeyElementType* element;
    uint32 i;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTGET, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if ((keyPtr == NULL_PTR) || (keyLengthPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTGET, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    if (cryptoKeyId >= Crypto_ConfigPtr->numKeys) {
        return E_NOT_OK;
    }
    
    key = &Crypto_ConfigPtr->keys[cryptoKeyId];
    element = NULL_PTR;
    
    for (i = 0U; i < key->numElements; i++) {
        if (key->keyElements[i].id == keyElementId) {
            element = &key->keyElements[i];
            break;
        }
    }
    
    if (element == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (*keyLengthPtr < element->size) {
        return E_NOT_OK;
    }
    
    /* Copy data */
    for (i = 0U; i < element->size; i++) {
        keyPtr[i] = element->data[i];
    }
    *keyLengthPtr = element->size;
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_KeyValidSet
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyValidSet(Crypto_KeyIdType cryptoKeyId, boolean valid)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYVALIDSET, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    if (cryptoKeyId >= Crypto_ConfigPtr->numKeys) {
        return E_NOT_OK;
    }
    
    Crypto_ConfigPtr->keys[cryptoKeyId].keyState = valid ? CRYPTO_KEY_VALID : CRYPTO_KEY_INVALID;
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_KeyElementIdsGet
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyElementIdsGet(Crypto_KeyIdType cryptoKeyId, uint32* keyElementIdsPtr)
{
    Crypto_KeyType* key;
    uint32 i;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTIDSGET, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (keyElementIdsPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYELEMENTIDSGET, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    if (cryptoKeyId >= Crypto_ConfigPtr->numKeys) {
        return E_NOT_OK;
    }
    
    key = &Crypto_ConfigPtr->keys[cryptoKeyId];
    
    for (i = 0U; i < key->numElements; i++) {
        keyElementIdsPtr[i] = key->keyElements[i].id;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_KeyElementCopy
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyElementCopy(Crypto_KeyIdType cryptoKeyId,
                                      Crypto_KeyElementIdType keyElementId,
                                      Crypto_KeyIdType targetCryptoKeyId,
                                      Crypto_KeyElementIdType targetKeyElementId)
{
    uint8 keyBuffer[CRYPTO_CFG_MAX_KEY_SIZE];
    uint32 keyLength = CRYPTO_CFG_MAX_KEY_SIZE;
    
    if (Crypto_KeyElementGet(cryptoKeyId, keyElementId, keyBuffer, &keyLength) != E_OK) {
        return E_NOT_OK;
    }
    
    return Crypto_KeyElementSet(targetCryptoKeyId, targetKeyElementId, keyBuffer, keyLength);
}

/**********************************************************************************************************************
 * Crypto_KeyGenerate
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyGenerate(Crypto_KeyIdType cryptoKeyId)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYGENERATE, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (cryptoKeyId >= Crypto_ConfigPtr->numKeys) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYGENERATE, CRYPTO_E_PARAM_HANDLE);
        return E_NOT_OK;
    }
    #endif
    
    /* Try HSM first if available */
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    if (Crypto_HsmAvailable) {
        if (Crypto_Hsm_LoadKey(cryptoKeyId) == E_OK) {
            return E_OK;
        }
    }
    #endif
    
    /* Fall back to software implementation */
    return Crypto_MbedTLS_KeyGenerate(cryptoKeyId);
}

/**********************************************************************************************************************
 * Crypto_KeyDerive
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyDerive(Crypto_KeyIdType cryptoKeyId, Crypto_KeyIdType targetCryptoKeyId)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYDERIVE, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    return Crypto_MbedTLS_KeyDerive(cryptoKeyId, targetCryptoKeyId);
}

/**********************************************************************************************************************
 * Crypto_KeyExchangeCalcSecret
 *********************************************************************************************************************/
Std_ReturnType Crypto_KeyExchangeCalcSecret(Crypto_KeyIdType cryptoKeyId,
                                             const uint8* partnerPublicKeyPtr,
                                             uint32 partnerPublicKeyLength)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYEXCHSYNCCALCSECRET, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (partnerPublicKeyPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_KEYEXCHSYNCCALCSECRET, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    return Crypto_MbedTLS_ECDH_CalcSecret(cryptoKeyId, partnerPublicKeyPtr, partnerPublicKeyLength);
}

/**********************************************************************************************************************
 * Crypto_RandomGenerate
 *********************************************************************************************************************/
Std_ReturnType Crypto_RandomGenerate(Crypto_KeyIdType cryptoKeyId,
                                      uint8* resultPtr,
                                      uint32 resultLength)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (resultPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (resultLength == 0U) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    #endif
    
    (void)cryptoKeyId;  /* Key ID used for RNG seeding context */
    
    /* Try HSM first if available for high-quality random */
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    if (Crypto_HsmAvailable) {
        /* HSM random generation would go here */
    }
    #endif
    
    return Crypto_MbedTLS_RandomGenerate(resultPtr, resultLength);
}

/**********************************************************************************************************************
 * Crypto_RandomSeed
 *********************************************************************************************************************/
Std_ReturnType Crypto_RandomSeed(Crypto_KeyIdType cryptoKeyId,
                                  const uint8* entropyPtr,
                                  uint32 entropyLength)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (entropyPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_RANDOMGENERATE, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    (void)cryptoKeyId;
    (void)entropyPtr;
    (void)entropyLength;
    
    /* Seed implementation specific to Mbed TLS */
    return E_OK;
}

/**********************************************************************************************************************
 * HSM SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

#if (CRYPTO_CFG_HSM_ENABLED == STD_ON)

/**********************************************************************************************************************
 * Crypto_HsmIsAvailable
 *********************************************************************************************************************/
boolean Crypto_HsmIsAvailable(void)
{
    return Crypto_HsmAvailable;
}

/**********************************************************************************************************************
 * Crypto_HsmGetStatus
 *********************************************************************************************************************/
Crypto_HsmStateType Crypto_HsmGetStatus(void)
{
    if (!Crypto_HsmAvailable) {
        return CRYPTO_HSM_UNINIT;
    }
    return Crypto_Hsm_GetState();
}

/**********************************************************************************************************************
 * Crypto_HsmLoadKey
 *********************************************************************************************************************/
Std_ReturnType Crypto_HsmLoadKey(Crypto_KeyIdType cryptoKeyId)
{
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, CRYPTO_SID_HSM_INIT, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    if (!Crypto_HsmAvailable) {
        return E_NOT_OK;
    }
    
    return Crypto_Hsm_LoadKey(cryptoKeyId);
}

/**********************************************************************************************************************
 * Crypto_HsmSelfTest
 *********************************************************************************************************************/
Std_ReturnType Crypto_HsmSelfTest(void)
{
    if (!Crypto_HsmAvailable) {
        return E_NOT_OK;
    }
    return Crypto_Hsm_SelfTest();
}

/**********************************************************************************************************************
 * Crypto_HsmGetId
 *********************************************************************************************************************/
Std_ReturnType Crypto_HsmGetId(uint8* idPtr, uint32* idLengthPtr)
{
    if (!Crypto_HsmAvailable) {
        return E_NOT_OK;
    }
    
    (void)idPtr;
    (void)idLengthPtr;
    
    return E_NOT_OK;  /* Not implemented yet */
}

#endif /* CRYPTO_CFG_HSM_ENABLED */

/**********************************************************************************************************************
 * BLAKE2 HASH FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_Blake2b
 *********************************************************************************************************************/
Std_ReturnType Crypto_Blake2b(const uint8* dataPtr,
                               uint32 dataLength,
                               const uint8* keyPtr,
                               uint32 keyLength,
                               uint32 digestLength,
                               uint8* digestPtr)
{
    Blake2_ReturnType ret;

    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA0U, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    if (digestPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA0U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((digestLength == 0U) || (digestLength > BLAKE2B_OUTBYTES)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA0U, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    if ((dataLength > 0U) && (dataPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA0U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((keyLength > 0U) && (keyPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA0U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (keyLength > BLAKE2B_KEYBYTES) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA0U, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    #endif

    ret = blake2b(digestPtr, dataPtr, dataLength, keyPtr, (uint8)keyLength, (uint8)digestLength);

    return (ret == BLAKE2_ERR_NONE) ? E_OK : E_NOT_OK;
}

/**********************************************************************************************************************
 * Crypto_Blake2s
 *********************************************************************************************************************/
Std_ReturnType Crypto_Blake2s(const uint8* dataPtr,
                               uint32 dataLength,
                               const uint8* keyPtr,
                               uint32 keyLength,
                               uint32 digestLength,
                               uint8* digestPtr)
{
    Blake2_ReturnType ret;

    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA1U, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    if (digestPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA1U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((digestLength == 0U) || (digestLength > BLAKE2S_OUTBYTES)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA1U, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    if ((dataLength > 0U) && (dataPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA1U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((keyLength > 0U) && (keyPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA1U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (keyLength > BLAKE2S_KEYBYTES) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA1U, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    #endif

    ret = blake2s(digestPtr, dataPtr, dataLength, keyPtr, (uint8)keyLength, (uint8)digestLength);

    return (ret == BLAKE2_ERR_NONE) ? E_OK : E_NOT_OK;
}

/**********************************************************************************************************************
 * BLAKE2 Context for Incremental Hashing
 *********************************************************************************************************************/
static blake2b_state_t Crypto_Blake2b_Context;
static boolean Crypto_Blake2b_ContextInitialized = FALSE;

/**********************************************************************************************************************
 * Crypto_Blake2b_Start
 *********************************************************************************************************************/
Std_ReturnType Crypto_Blake2b_Start(uint32 jobId,
                                     const uint8* keyPtr,
                                     uint32 keyLength,
                                     uint32 digestLength)
{
    Blake2_ReturnType ret;

    (void)jobId; /* Not used in this implementation */

    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA2U, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    if ((digestLength == 0U) || (digestLength > BLAKE2B_OUTBYTES)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA2U, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    if ((keyLength > 0U) && (keyPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA2U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (keyLength > BLAKE2B_KEYBYTES) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA2U, CRYPTO_E_PARAM_VALUE);
        return E_NOT_OK;
    }
    #endif

    if (keyLength > 0U) {
        ret = blake2b_init_key(&Crypto_Blake2b_Context, (uint8)digestLength, keyPtr, (uint8)keyLength);
    } else {
        ret = blake2b_init(&Crypto_Blake2b_Context, (uint8)digestLength);
    }

    if (ret == BLAKE2_ERR_NONE) {
        Crypto_Blake2b_ContextInitialized = TRUE;
        return E_OK;
    }

    return E_NOT_OK;
}

/**********************************************************************************************************************
 * Crypto_Blake2b_Update
 *********************************************************************************************************************/
Std_ReturnType Crypto_Blake2b_Update(uint32 jobId,
                                      const uint8* dataPtr,
                                      uint32 dataLength)
{
    Blake2_ReturnType ret;

    (void)jobId; /* Not used in this implementation */

    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA3U, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    if (!Crypto_Blake2b_ContextInitialized) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA3U, CRYPTO_E_PARAM_STATE);
        return E_NOT_OK;
    }
    if ((dataLength > 0U) && (dataPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA3U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    ret = blake2b_update(&Crypto_Blake2b_Context, dataPtr, dataLength);

    return (ret == BLAKE2_ERR_NONE) ? E_OK : E_NOT_OK;
}

/**********************************************************************************************************************
 * Crypto_Blake2b_Finish
 *********************************************************************************************************************/
Std_ReturnType Crypto_Blake2b_Finish(uint32 jobId,
                                      uint8* digestPtr,
                                      uint32* digestLengthPtr)
{
    Blake2_ReturnType ret;
    uint8 outlen;

    (void)jobId; /* Not used in this implementation */

    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA4U, CRYPTO_E_UNINIT);
        return E_NOT_OK;
    }
    if (!Crypto_Blake2b_ContextInitialized) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA4U, CRYPTO_E_PARAM_STATE);
        return E_NOT_OK;
    }
    if (digestPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA4U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if (digestLengthPtr == NULL_PTR) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xA4U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    outlen = (uint8)(*digestLengthPtr);
    ret = blake2b_final(&Crypto_Blake2b_Context, digestPtr, outlen);

    if (ret == BLAKE2_ERR_NONE) {
        *digestLengthPtr = outlen;
        Crypto_Blake2b_ContextInitialized = FALSE;
        return E_OK;
    }

    return E_NOT_OK;
}

/**********************************************************************************************************************
 * CCC DIGITAL KEY SPECIFIC FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_CccGenerateAttestation
 *********************************************************************************************************************/
Std_ReturnType Crypto_CccGenerateAttestation(const uint8* challengePtr,
                                              uint32 challengeLength,
                                              uint8* signaturePtr,
                                              uint32* signatureLengthPtr)
{
    uint8 digest[32];
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    if ((challengePtr == NULL_PTR) || (signaturePtr == NULL_PTR) || (signatureLengthPtr == NULL_PTR)) {
        Det_ReportError(CRYPTO_MODULE_ID, 0U, 0xF0U, CRYPTO_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    /* Hash the challenge */
    if (Crypto_MbedTLS_SHA256(challengePtr, challengeLength, digest) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Sign with device attestation key */
    return Crypto_MbedTLS_ECDSA_Sign(CRYPTO_KEY_ID_CCC_DEVICE_KEY, digest, 32, signaturePtr, signatureLengthPtr);
}

/**********************************************************************************************************************
 * Crypto_CccVerifyOwnerCertificate
 *********************************************************************************************************************/
Std_ReturnType Crypto_CccVerifyOwnerCertificate(const uint8* certificatePtr,
                                                 uint32 certificateLength,
                                                 Crypto_VerifyResultType* verifyResultPtr)
{
    (void)certificatePtr;
    (void)certificateLength;
    
    if (verifyResultPtr != NULL_PTR) {
        *verifyResultPtr = CRYPTO_VERIFICATION_FAILED;
    }
    
    /* Certificate chain verification logic would go here */
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * Crypto_CccDeriveSessionKey
 *********************************************************************************************************************/
Std_ReturnType Crypto_CccDeriveSessionKey(const uint8* ephemeralPublicKeyPtr,
                                           uint32 ephemeralPublicKeyLength,
                                           Crypto_KeyIdType sessionKeyId)
{
    /* Perform ECDH to derive shared secret */
    return Crypto_MbedTLS_ECDH_CalcSecret(CRYPTO_KEY_ID_EPHEMERAL, ephemeralPublicKeyPtr, ephemeralPublicKeyLength);
}

/**********************************************************************************************************************
 * Crypto_CccEncrypt
 *********************************************************************************************************************/
Std_ReturnType Crypto_CccEncrypt(Crypto_KeyIdType keyId,
                                  const uint8* plaintextPtr,
                                  uint32 plaintextLength,
                                  const uint8* aadPtr,
                                  uint32 aadLength,
                                  const uint8* ivPtr,
                                  uint32 ivLength,
                                  uint8* ciphertextPtr,
                                  uint8* tagPtr,
                                  uint32* tagLengthPtr)
{
    (void)ivLength;
    
    if (tagLengthPtr != NULL_PTR) {
        *tagLengthPtr = CRYPTO_CFG_CCC_TAG_SIZE;
    }
    
    return Crypto_MbedTLS_AES_GCM_Encrypt(keyId, plaintextPtr, plaintextLength, 
                                           aadPtr, aadLength, ivPtr, 
                                           ciphertextPtr, tagPtr);
}

/**********************************************************************************************************************
 * Crypto_CccDecrypt
 *********************************************************************************************************************/
Std_ReturnType Crypto_CccDecrypt(Crypto_KeyIdType keyId,
                                  const uint8* ciphertextPtr,
                                  uint32 ciphertextLength,
                                  const uint8* aadPtr,
                                  uint32 aadLength,
                                  const uint8* ivPtr,
                                  uint32 ivLength,
                                  const uint8* tagPtr,
                                  uint32 tagLength,
                                  uint8* plaintextPtr,
                                  uint32* plaintextLengthPtr)
{
    (void)ivLength;
    (void)tagLength;
    
    if (plaintextLengthPtr != NULL_PTR) {
        *plaintextLengthPtr = ciphertextLength;
    }
    
    return Crypto_MbedTLS_AES_GCM_Decrypt(keyId, ciphertextPtr, ciphertextLength,
                                           aadPtr, aadLength, ivPtr, tagPtr,
                                           plaintextPtr);
}

/**********************************************************************************************************************
 * CALLBACK FUNCTIONS
 **********************************************************************************************************************/
__attribute__((weak)) void Crypto_JobNotification(Crypto_JobType* job, Crypto_ResultType result)
{
    (void)job;
    (void)result;
    /* Default empty implementation - can be overridden by application */
}

__attribute__((weak)) void Crypto_ErrorNotification(uint16 moduleId, uint8 instanceId, uint8 apiId, uint8 errorId)
{
    (void)moduleId;
    (void)instanceId;
    (void)apiId;
    (void)errorId;
    /* Default empty implementation */
}

/**********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Crypto_ValidateJob
 **********************************************************************************************************************/
STATIC Std_ReturnType Crypto_ValidateJob(Crypto_JobType* job)
{
    if (job->jobPrimitiveInfo == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (job->jobPrimitiveInputOutput == NULL_PTR) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_QueuePush
 **********************************************************************************************************************/
STATIC Std_ReturnType Crypto_QueuePush(Crypto_JobType* job)
{
    /* Simple queue implementation - in production would use pre-allocated pool */
    job->jobState = CRYPTO_JOBSTATE_QUEUED;
    Crypto_QueueCount++;
    
    return E_OK;
}

/**********************************************************************************************************************
 * Crypto_QueuePop
 **********************************************************************************************************************/
STATIC Crypto_JobType* Crypto_QueuePop(void)
{
    if (Crypto_JobQueueHead == NULL_PTR) {
        return NULL_PTR;
    }
    
    /* Simple implementation */
    Crypto_QueueCount--;
    return NULL_PTR;  /* Would return actual job in full implementation */
}

/**********************************************************************************************************************
 * Crypto_ProcessJobInternal
 **********************************************************************************************************************/
STATIC Std_ReturnType Crypto_ProcessJobInternal(Crypto_JobType* job)
{
    Std_ReturnType result;
    
    job->jobState = CRYPTO_JOBSTATE_ACTIVE;
    
    #if (CRYPTO_CFG_HSM_ENABLED == STD_ON)
    /* Try HSM first if available and appropriate */
    if (Crypto_HsmAvailable && (job->jobPrimitiveInfo->processingType == CRYPTO_PROCESSING_SYNC)) {
        result = Crypto_Hsm_ProcessJob(job);
        if (result == E_OK) {
            job->jobState = CRYPTO_JOBSTATE_IDLE;
            if (job->jobPrimitiveInfo->callbackId != 0U) {
                Crypto_JobNotification(job, CRYPTO_RESULT_OK);
            }
            return E_OK;
        }
        /* Fall back to software if HSM fails and fallback is allowed */
        #if (CRYPTO_CFG_HSM_FALLBACK_TO_SW == STD_OFF)
        job->jobState = CRYPTO_JOBSTATE_IDLE;
        return result;
        #endif
    }
    #endif
    
    /* Process via software implementation */
    result = Crypto_MbedTLS_ProcessJob(job);
    
    if (result == E_OK) {
        job->jobState = CRYPTO_JOBSTATE_IDLE;
        if (job->jobPrimitiveInfo->callbackId != 0U) {
            Crypto_JobNotification(job, CRYPTO_RESULT_OK);
        }
    } else {
        job->jobState = CRYPTO_JOBSTATE_IDLE;
    }
    
    return result;
}

/**********************************************************************************************************************
 * Crypto_MainFunction
 **********************************************************************************************************************/
void Crypto_MainFunction(void)
{
    Crypto_JobType* job;
    
    if (Crypto_DriverState != CRYPTO_DRIVER_INIT) {
        return;
    }
    
    /* Process queued jobs */
    while (Crypto_QueueCount > 0U) {
        job = Crypto_QueuePop();
        if (job != NULL_PTR) {
            (void)Crypto_ProcessJobInternal(job);
        } else {
            break;
        }
    }
}

#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"

/**********************************************************************************************************************
 * END OF FILE
 **********************************************************************************************************************/
