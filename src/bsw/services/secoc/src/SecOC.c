/**
 * @file SecOC.c
 * @brief Secure Onboard Communication
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/*==================================================================================================
 *                              SECURE ONBOARD COMMUNICATION (SecOC)
 *==================================================================================================
 * FILENAME: SecOC.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_SecureOnboardCommunication.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Secure Onboard Communication module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "SecOC.h"
#include "PduR.h"
#include "Csm.h"
#include "Det.h"
#include "SchM_SecOC.h"
#include <string.h>

/*==================================================================================================
 *                                    VERSION CHECK
 *==================================================================================================*/
#if defined(SECOC_AR_RELEASE_MAJOR_VERSION) && (SECOC_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "SecOC.c: AR major version mismatch"
#endif

#if defined(SECOC_AR_RELEASE_MINOR_VERSION) && (SECOC_AR_RELEASE_MINOR_VERSION != 7u)
    #error "SecOC.c: AR minor version mismatch"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define SECOC_FRESHNESS_BYTE_LEN            (SECOC_FRESHNESS_VALUE_LENGTH / 8u)
#define SECOC_FRESHNESS_TX_BYTE_LEN         (SECOC_FRESHNESS_VALUE_TX_LENGTH / 8u)

/*==================================================================================================
 *                                    LOCAL TYPES
 *==================================================================================================*/
typedef struct {
    uint8 data[SECOC_MAX_PDU_LENGTH];
    PduLengthType length;
    boolean inUse;
    PduIdType pduId;
} SecOC_BufferType;

typedef struct {
    uint32 freshnessValue;
    uint32 lastVerifiedFreshness;
    SecOC_VerificationStatusType status;
    SecOC_VerificationResultType lastResult;
    uint8 retryCount;
    boolean authInProgress;
    uint16 timeoutCounter;
} SecOC_RxPduStateType;

typedef struct {
    uint32 freshnessValue;
    boolean txInProgress;
} SecOC_TxPduStateType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "SecOC_MemMap.h"

static SecOC_BufferType SecOC_TxBuffers[SECOC_NUM_TX_PDUS];
static SecOC_BufferType SecOC_RxBuffers[SECOC_NUM_RX_PDUS];
static SecOC_TxPduStateType SecOC_TxPduState[SECOC_NUM_TX_PDUS];
static SecOC_RxPduStateType SecOC_RxPduState[SECOC_NUM_RX_PDUS];
static boolean SecOC_Initialized_Local = FALSE;

/* Freshness value sync master counter */
static uint32 SecOC_SyncMasterFreshness = 0u;

#define SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "SecOC_MemMap.h"

/*==================================================================================================
 *                                    GLOBAL VARIABLES
 *==================================================================================================*/
#define SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "SecOC_MemMap.h"

boolean SecOC_Initialized = FALSE;
const SecOC_ConfigType* SecOC_ConfigPtr = NULL_PTR;

#define SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "SecOC_MemMap.h"

/*==================================================================================================
 *                                    LOCAL FUNCTIONS
 *==================================================================================================*/
#define SECOC_START_SEC_CODE
#include "SecOC_MemMap.h"

/**
 * @brief Get TX PDU index from PDU ID
 * @req SWS_SecOC_00100
 */
static sint16 SecOC_GetTxPduIndex(PduIdType pduId)
{
    if (pduId < SECOC_NUM_TX_PDUS) {
        return (sint16)pduId;
    }
    return -1;
}

/**
 * @brief Get RX PDU index from PDU ID
 * @req SWS_SecOC_00101
 */
static sint16 SecOC_GetRxPduIndex(PduIdType pduId)
{
    if (pduId < SECOC_NUM_RX_PDUS) {
        return (sint16)pduId;
    }
    return -1;
}

/**
 * @brief Increment freshness value
 * @req SWS_SecOC_00102
 */
static uint32 SecOC_IncrementFreshness(uint32 freshness)
{
    freshness++;
    /* Check for overflow */
    if (freshness >= SECOC_FRESHNESS_RESET_THRESHOLD) {
        freshness = 0u;
    }
    return freshness;
}

/**
 * @brief Build authentication data
 * @req SWS_SecOC_00103
 */
static void SecOC_BuildAuthData(const uint8* pduData, PduLengthType pduLength,
                                 uint32 freshnessValue, uint8 dataId,
                                 uint8* authData, uint16* authDataLen)
{
    uint16 idx = 0u;
    uint8 i;
    
    /* Add Data ID */
    authData[idx++] = dataId;
    
    /* Add freshness value (full) */
    for (i = 0u; i < SECOC_FRESHNESS_BYTE_LEN; i++) {
        authData[idx++] = (uint8)(freshnessValue >> ((SECOC_FRESHNESS_BYTE_LEN - 1u - i) * 8u));
    }
    
    /* Add PDU data */
    for (i = 0u; i < (uint16)pduLength; i++) {
        authData[idx++] = pduData[i];
    }
    
    *authDataLen = idx;
}

/**
 * @brief Generate authentication code
 * @req SWS_SecOC_00104
 */
static Std_ReturnType SecOC_GenerateAuthCode(const uint8* authData, uint16 authDataLen,
                                              uint8* authCode, uint32* authCodeLen)
{
    return Csm_MacGenerate(SECOC_CSM_JOB_ID_AUTH, CSM_OPERATIONMODE_STREAMSTART,
                           authData, authDataLen, authCode, authCodeLen);
}

/**
 * @brief Verify authentication code
 * @req SWS_SecOC_00105
 */
static Std_ReturnType SecOC_VerifyAuthCode(const uint8* authData, uint16 authDataLen,
                                            const uint8* authCode, uint32 authCodeLen,
                                            Csm_VerifyResultType* verifyResult)
{
    return Csm_MacVerify(SECOC_CSM_JOB_ID_VERIFY, CSM_OPERATIONMODE_STREAMSTART,
                         authData, authDataLen, authCode, authCodeLen, verifyResult);
}

/**
 * @brief Process TX PDU - add security
 * @req SWS_SecOC_00106
 */
static Std_ReturnType SecOC_ProcessTxPdu(PduIdType pduId)
{
    sint16 idx;
    uint8 authData[SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_BYTE_LEN + 1u];
    uint16 authDataLen;
    uint8 authCode[SECOC_MAX_AUTH_INFO_LEN];
    uint32 authCodeLen = SECOC_AUTH_INFO_LENGTH;
    uint8 securedPdu[SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_TX_BYTE_LEN + SECOC_AUTH_INFO_LENGTH];
    PduLengthType securedPduLen;
    uint8 i;
    Std_ReturnType result;
    
    idx = SecOC_GetTxPduIndex(pduId);
    if (idx < 0) {
        return E_NOT_OK;
    }
    
    if (!SecOC_TxBuffers[idx].inUse) {
        return E_NOT_OK;
    }
    
    /* Increment freshness value */
    SecOC_TxPduState[idx].freshnessValue = 
        SecOC_IncrementFreshness(SecOC_TxPduState[idx].freshnessValue);
    
    /* Build authentication data */
    SecOC_BuildAuthData(SecOC_TxBuffers[idx].data, SecOC_TxBuffers[idx].length,
                        SecOC_TxPduState[idx].freshnessValue, (uint8)pduId,
                        authData, &authDataLen);
    
    /* Generate authentication code */
    result = SecOC_GenerateAuthCode(authData, authDataLen, authCode, &authCodeLen);
    
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    /* Build secured PDU: [Data][Freshness (truncated)][Auth Code] */
    securedPduLen = 0u;
    
    /* Add original data */
    for (i = 0u; i < (uint8)SecOC_TxBuffers[idx].length; i++) {
        securedPdu[securedPduLen++] = SecOC_TxBuffers[idx].data[i];
    }
    
    /* Add truncated freshness value */
    for (i = 0u; i < SECOC_FRESHNESS_TX_BYTE_LEN; i++) {
        securedPdu[securedPduLen++] = 
            (uint8)(SecOC_TxPduState[idx].freshnessValue >> 
                   ((SECOC_FRESHNESS_TX_BYTE_LEN - 1u - i) * 8u));
    }
    
    /* Add authentication code */
    for (i = 0u; i < SECOC_AUTH_INFO_LENGTH; i++) {
        securedPdu[securedPduLen++] = authCode[i];
    }
    
    /* Transmit via PduR */
    {
        PduInfoType pduInfo;
        pduInfo.SduDataPtr = securedPdu;
        pduInfo.SduLength = securedPduLen;
        pduInfo.MetaDataPtr = NULL_PTR;
        
        result = PduR_SecOCTransmit(pduId, &pduInfo);
    }
    
    SecOC_TxBuffers[idx].inUse = FALSE;
    SecOC_TxPduState[idx].txInProgress = FALSE;
    
    return result;
}

/**
 * @brief Process RX PDU - verify security
 * @req SWS_SecOC_00107
 */
static void SecOC_ProcessRxPdu(PduIdType pduId)
{
    sint16 idx;
    uint8 authData[SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_BYTE_LEN + 1u];
    uint16 authDataLen;
    uint8 receivedAuthCode[SECOC_MAX_AUTH_INFO_LEN];
    Csm_VerifyResultType verifyResult;
    uint8* securedPdu;
    PduLengthType securedPduLen;
    PduLengthType originalDataLen;
    uint8 i;
    uint32 receivedFreshness;
    Std_ReturnType result;
    
    idx = SecOC_GetRxPduIndex(pduId);
    if (idx < 0) {
        return;
    }
    
    if (!SecOC_RxBuffers[idx].inUse) {
        return;
    }
    
    SecOC_RxPduState[idx].authInProgress = TRUE;
    
    securedPdu = SecOC_RxBuffers[idx].data;
    securedPduLen = SecOC_RxBuffers[idx].length;
    
    /* Calculate original data length */
    if (securedPduLen < (SECOC_FRESHNESS_TX_BYTE_LEN + SECOC_AUTH_INFO_LENGTH)) {
        SecOC_RxPduState[idx].status = SECOC_VERIFICATIONFAILURE_STATUS;
        SecOC_RxPduState[idx].lastResult = SECOC_AUTHENTICATIONBUILDFAILURE;
        SecOC_RxPduState[idx].authInProgress = FALSE;
        SecOC_RxBuffers[idx].inUse = FALSE;
        return;
    }
    
    originalDataLen = securedPduLen - SECOC_FRESHNESS_TX_BYTE_LEN - SECOC_AUTH_INFO_LENGTH;
    
    /* Extract truncated freshness value */
    receivedFreshness = 0u;
    for (i = 0u; i < SECOC_FRESHNESS_TX_BYTE_LEN; i++) {
        receivedFreshness = (receivedFreshness << 8u) | 
                           securedPdu[originalDataLen + i];
    }
    
    /* Reconstruct full freshness value (simplified - should use freshness manager) */
    receivedFreshness |= (SecOC_RxPduState[idx].lastVerifiedFreshness & 
                         ~(0xFFFFFFFFu >> SECOC_FRESHNESS_VALUE_TX_LENGTH));
    
    /* Extract authentication code */
    for (i = 0u; i < SECOC_AUTH_INFO_LENGTH; i++) {
        receivedAuthCode[i] = securedPdu[originalDataLen + SECOC_FRESHNESS_TX_BYTE_LEN + i];
    }
    
    /* Build authentication data for verification */
    SecOC_BuildAuthData(securedPdu, originalDataLen, receivedFreshness, (uint8)pduId,
                        authData, &authDataLen);
    
    /* Verify authentication code */
    result = SecOC_VerifyAuthCode(authData, authDataLen, receivedAuthCode,
                                   SECOC_AUTH_INFO_LENGTH, &verifyResult);
    
    if ((result == E_OK) && (verifyResult == CSM_E_VER_OK)) {
        SecOC_RxPduState[idx].status = SECOC_VERIFICATIONSUCCESS_STATUS;
        SecOC_RxPduState[idx].lastResult = SECOC_VERIFICATIONSUCCESS;
        SecOC_RxPduState[idx].lastVerifiedFreshness = receivedFreshness;
        
        /* Forward verified data to upper layer */
        {
            PduInfoType pduInfo;
            pduInfo.SduDataPtr = securedPdu;
            pduInfo.SduLength = originalDataLen;
            pduInfo.MetaDataPtr = NULL_PTR;
            
            PduR_SecOCRxIndication(pduId, &pduInfo);
        }
    } else {
        SecOC_RxPduState[idx].status = SECOC_VERIFICATIONFAILURE_STATUS;
        SecOC_RxPduState[idx].lastResult = SECOC_VERIFICATIONFAILURE;
        SecOC_RxPduState[idx].retryCount++;
        
        if (SecOC_RxPduState[idx].retryCount >= SECOC_VERIFICATION_RETRY_COUNT) {
            /* Report failure to DEM */
            /* Dem_ReportErrorStatus(SECOC_E_CRYPTO_AUTH_FAILED, DEM_EVENT_STATUS_FAILED); */
        }
    }
    
    SecOC_RxPduState[idx].authInProgress = FALSE;
    SecOC_RxBuffers[idx].inUse = FALSE;
}

/*==================================================================================================
 *                                    GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief Initializes the SecOC module
 * @req SWS_SecOC_00001
 */
void SecOC_Init(const SecOC_ConfigType* configPtr)
{
    uint16 i;
    
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (SecOC_Initialized_Local == TRUE) {
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_INIT, 
                               SECOC_E_ALREADY_INITIALIZED);
        return;
    }
    
    if (configPtr == NULL_PTR) {
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_INIT, 
                               SECOC_E_PARAM_POINTER);
        return;
    }
#endif

    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0();
    
    /* Initialize TX state */
    for (i = 0u; i < SECOC_NUM_TX_PDUS; i++) {
        SecOC_TxPduState[i].freshnessValue = 0u;
        SecOC_TxPduState[i].txInProgress = FALSE;
        SecOC_TxBuffers[i].inUse = FALSE;
        SecOC_TxBuffers[i].pduId = i;
    }
    
    /* Initialize RX state */
    for (i = 0u; i < SECOC_NUM_RX_PDUS; i++) {
        SecOC_RxPduState[i].freshnessValue = 0u;
        SecOC_RxPduState[i].lastVerifiedFreshness = 0u;
        SecOC_RxPduState[i].status = SECOC_UNVERIFIED;
        SecOC_RxPduState[i].lastResult = SECOC_NO_VERIFICATION;
        SecOC_RxPduState[i].retryCount = 0u;
        SecOC_RxPduState[i].authInProgress = FALSE;
        SecOC_RxPduState[i].timeoutCounter = 0u;
        SecOC_RxBuffers[i].inUse = FALSE;
        SecOC_RxBuffers[i].pduId = i;
    }
    
    SecOC_SyncMasterFreshness = 0u;
    SecOC_ConfigPtr = configPtr;
    SecOC_Initialized_Local = TRUE;
    SecOC_Initialized = TRUE;
    
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0();
}

/**
 * @brief Deinitializes the SecOC module
 * @req SWS_SecOC_00002
 */
void SecOC_DeInit(void)
{
    uint16 i;
    
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (SecOC_Initialized_Local == FALSE) {
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_DEINIT, 
                               SECOC_E_UNINIT);
        return;
    }
#endif

    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0();
    
    /* Reset TX state */
    for (i = 0u; i < SECOC_NUM_TX_PDUS; i++) {
        SecOC_TxPduState[i].txInProgress = FALSE;
        SecOC_TxBuffers[i].inUse = FALSE;
    }
    
    /* Reset RX state */
    for (i = 0u; i < SECOC_NUM_RX_PDUS; i++) {
        SecOC_RxPduState[i].status = SECOC_UNVERIFIED;
        SecOC_RxPduState[i].authInProgress = FALSE;
        SecOC_RxBuffers[i].inUse = FALSE;
    }
    
    SecOC_ConfigPtr = NULL_PTR;
    SecOC_Initialized_Local = FALSE;
    SecOC_Initialized = FALSE;
    
    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0();
}

/**
 * @brief Gets version information
 * @req SWS_SecOC_00040
 */
#if (SECOC_VERSION_INFO_API == STD_ON)
void SecOC_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_GETVERSIONINFO, 
                               SECOC_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = SECOC_VENDOR_ID;
    versioninfo->moduleID = SECOC_MODULE_ID;
    versioninfo->sw_major_version = SECOC_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SECOC_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SECOC_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Transmits a secured PDU
 * @req SWS_SecOC_00010
 */
Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    sint16 idx;
    uint16 i;
    
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (SecOC_Initialized_Local == FALSE) {
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
                               SECOC_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (PduInfoPtr == NULL_PTR) {
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
                               SECOC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    idx = SecOC_GetTxPduIndex(TxPduId);
    if (idx < 0) {
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
                               SECOC_E_INVALID_PDU_SDU_ID);
#endif
        return E_NOT_OK;
    }
    
    if (SecOC_TxBuffers[idx].inUse) {
        return E_NOT_OK;
    }

    /* Copy PDU data into the TX buffer */
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR))
    {
        SecOC_TxBuffers[idx].length = PduInfoPtr->SduLength;
        (void)memcpy(SecOC_TxBuffers[idx].data, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
        SecOC_TxBuffers[idx].inUse = TRUE;
        SecOC_TxBuffers[idx].pduId = TxPduId;
    }

    return E_OK;
}

#define SECOC_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
