/**
 * @file SecOC.c
 * @brief Secure Onboard Communication
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2| *                              SECURE ONBOARD COMMUNICATION (SecOC)
     3| *==================================================================================================
     4| * FILENAME: SecOC.c
     5| * AUTOSAR VERSION: R22-11
     6| * DOCUMENT: AUTOSAR_SWS_SecureOnboardCommunication.pdf
     7| *==================================================================================================
     8| * PROJECT: yuleASR Classic AUTOSAR BSW
     9| * DESCRIPTION: Implementation of Secure Onboard Communication module
    10| *==================================================================================================
    11| */
    12|
    13|/*==================================================================================================
    14| *                                         INCLUDES
    15| *==================================================================================================*/
    16|#include "SecOC.h"
    17|#include "PduR.h"
    18|#include "Csm.h"
    19|#include "Det.h"
    20|#include "SchM_SecOC.h"
    21|
    22|/*==================================================================================================
    23| *                                    VERSION CHECK
    24| *==================================================================================================*/
    25|#if defined(SECOC_AR_RELEASE_MAJOR_VERSION) && (SECOC_AR_RELEASE_MAJOR_VERSION != 4u)
    26|    #error "SecOC.c: AR major version mismatch"
    27|#endif
    28|
    29|#if defined(SECOC_AR_RELEASE_MINOR_VERSION) && (SECOC_AR_RELEASE_MINOR_VERSION != 4u)
    30|    #error "SecOC.c: AR minor version mismatch"
    31|#endif
    32|
    33|/*==================================================================================================
    34| *                                    LOCAL DEFINES
    35| *==================================================================================================*/
    36|#define SECOC_FRESHNESS_BYTE_LEN            (SECOC_FRESHNESS_VALUE_LENGTH / 8u)
    37|#define SECOC_FRESHNESS_TX_BYTE_LEN         (SECOC_FRESHNESS_VALUE_TX_LENGTH / 8u)
    38|
    39|/*==================================================================================================
    40| *                                    LOCAL TYPES
    41| *==================================================================================================*/
    42|typedef struct {
    43|    uint8 data[SECOC_MAX_PDU_LENGTH];
    44|    PduLengthType length;
    45|    boolean inUse;
    46|    PduIdType pduId;
    47|} SecOC_BufferType;
    48|
    49|typedef struct {
    50|    uint32 freshnessValue;
    51|    uint32 lastVerifiedFreshness;
    52|    SecOC_VerificationStatusType status;
    53|    SecOC_VerificationResultType lastResult;
    54|    uint8 retryCount;
    55|    boolean authInProgress;
    56|    uint16 timeoutCounter;
    57|} SecOC_RxPduStateType;
    58|
    59|typedef struct {
    60|    uint32 freshnessValue;
    61|    boolean txInProgress;
    62|} SecOC_TxPduStateType;
    63|
    64|/*==================================================================================================
    65| *                                    LOCAL VARIABLES
    66| *==================================================================================================*/
    67|#define SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
    68|#include "SecOC_MemMap.h"
    69|
    70|static SecOC_BufferType SecOC_TxBuffers[SECOC_NUM_TX_PDUS];
    71|static SecOC_BufferType SecOC_RxBuffers[SECOC_NUM_RX_PDUS];
    72|static SecOC_TxPduStateType SecOC_TxPduState[SECOC_NUM_TX_PDUS];
    73|static SecOC_RxPduStateType SecOC_RxPduState[SECOC_NUM_RX_PDUS];
    74|static boolean SecOC_Initialized_Local = FALSE;
    75|
    76|/* Freshness value sync master counter */
    77|static uint32 SecOC_SyncMasterFreshness = 0u;
    78|
    79|#define SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    80|#include "SecOC_MemMap.h"
    81|
    82|/*==================================================================================================
    83| *                                    GLOBAL VARIABLES
    84| *==================================================================================================*/
    85|#define SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
    86|#include "SecOC_MemMap.h"
    87|
    88|boolean SecOC_Initialized = FALSE;
    89|const SecOC_ConfigType* SecOC_ConfigPtr = NULL_PTR;
    90|
    91|#define SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    92|#include "SecOC_MemMap.h"
    93|
    94|/*==================================================================================================
    95| *                                    LOCAL FUNCTIONS
    96| *==================================================================================================*/
    97|#define SECOC_START_SEC_CODE
    98|#include "SecOC_MemMap.h"
    99|
   100|/**
   101| * @brief Get TX PDU index from PDU ID
   102| */
   103|static sint16 SecOC_GetTxPduIndex(PduIdType pduId)
   104|{
   105|    if (pduId < SECOC_NUM_TX_PDUS) {
   106|        return (sint16)pduId;
   107|    }
   108|    return -1;
   109|}
   110|
   111|/**
   112| * @brief Get RX PDU index from PDU ID
   113| */
   114|static sint16 SecOC_GetRxPduIndex(PduIdType pduId)
   115|{
   116|    if (pduId < SECOC_NUM_RX_PDUS) {
   117|        return (sint16)pduId;
   118|    }
   119|    return -1;
   120|}
   121|
   122|/**
   123| * @brief Increment freshness value
   124| */
   125|static uint32 SecOC_IncrementFreshness(uint32 freshness)
   126|{
   127|    freshness++;
   128|    /* Check for overflow */
   129|    if (freshness >= SECOC_FRESHNESS_RESET_THRESHOLD) {
   130|        freshness = 0u;
   131|    }
   132|    return freshness;
   133|}
   134|
   135|/**
   136| * @brief Build authentication data
   137| */
   138|static void SecOC_BuildAuthData(const uint8* pduData, PduLengthType pduLength,
   139|                                 uint32 freshnessValue, uint8 dataId,
   140|                                 uint8* authData, uint16* authDataLen)
   141|{
   142|    uint16 idx = 0u;
   143|    uint8 i;
   144|    
   145|    /* Add Data ID */
   146|    authData[idx++] = dataId;
   147|    
   148|    /* Add freshness value (full) */
   149|    for (i = 0u; i < SECOC_FRESHNESS_BYTE_LEN; i++) {
   150|        authData[idx++] = (uint8)(freshnessValue >> ((SECOC_FRESHNESS_BYTE_LEN - 1u - i) * 8u));
   151|    }
   152|    
   153|    /* Add PDU data */
   154|    for (i = 0u; i < (uint16)pduLength; i++) {
   155|        authData[idx++] = pduData[i];
   156|    }
   157|    
   158|    *authDataLen = idx;
   159|}
   160|
   161|/**
   162| * @brief Generate authentication code
   163| */
   164|static Std_ReturnType SecOC_GenerateAuthCode(const uint8* authData, uint16 authDataLen,
   165|                                              uint8* authCode, uint32* authCodeLen)
   166|{
   167|    return Csm_MacGenerate(SECOC_CSM_JOB_ID_AUTH, CSM_OPERATIONMODE_STREAMSTART,
   168|                           authData, authDataLen, authCode, authCodeLen);
   169|}
   170|
   171|/**
   172| * @brief Verify authentication code
   173| */
   174|static Std_ReturnType SecOC_VerifyAuthCode(const uint8* authData, uint16 authDataLen,
   175|                                            const uint8* authCode, uint32 authCodeLen,
   176|                                            Csm_VerifyResultType* verifyResult)
   177|{
   178|    return Csm_MacVerify(SECOC_CSM_JOB_ID_VERIFY, CSM_OPERATIONMODE_STREAMSTART,
   179|                         authData, authDataLen, authCode, authCodeLen, verifyResult);
   180|}
   181|
   182|/**
   183| * @brief Process TX PDU - add security
   184| */
   185|static Std_ReturnType SecOC_ProcessTxPdu(PduIdType pduId)
   186|{
   187|    sint16 idx;
   188|    uint8 authData[SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_BYTE_LEN + 1u];
   189|    uint16 authDataLen;
   190|    uint8 authCode[SECOC_MAX_AUTH_INFO_LEN];
   191|    uint32 authCodeLen = SECOC_AUTH_INFO_LENGTH;
   192|    uint8 securedPdu[SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_TX_BYTE_LEN + SECOC_AUTH_INFO_LENGTH];
   193|    PduLengthType securedPduLen;
   194|    uint8 i;
   195|    Std_ReturnType result;
   196|    
   197|    idx = SecOC_GetTxPduIndex(pduId);
   198|    if (idx < 0) {
   199|        return E_NOT_OK;
   200|    }
   201|    
   202|    if (!SecOC_TxBuffers[idx].inUse) {
   203|        return E_NOT_OK;
   204|    }
   205|    
   206|    /* Increment freshness value */
   207|    SecOC_TxPduState[idx].freshnessValue = 
   208|        SecOC_IncrementFreshness(SecOC_TxPduState[idx].freshnessValue);
   209|    
   210|    /* Build authentication data */
   211|    SecOC_BuildAuthData(SecOC_TxBuffers[idx].data, SecOC_TxBuffers[idx].length,
   212|                        SecOC_TxPduState[idx].freshnessValue, (uint8)pduId,
   213|                        authData, &authDataLen);
   214|    
   215|    /* Generate authentication code */
   216|    result = SecOC_GenerateAuthCode(authData, authDataLen, authCode, &authCodeLen);
   217|    
   218|    if (result != E_OK) {
   219|        return E_NOT_OK;
   220|    }
   221|    
   222|    /* Build secured PDU: [Data][Freshness (truncated)][Auth Code] */
   223|    securedPduLen = 0u;
   224|    
   225|    /* Add original data */
   226|    for (i = 0u; i < (uint8)SecOC_TxBuffers[idx].length; i++) {
   227|        securedPdu[securedPduLen++] = SecOC_TxBuffers[idx].data[i];
   228|    }
   229|    
   230|    /* Add truncated freshness value */
   231|    for (i = 0u; i < SECOC_FRESHNESS_TX_BYTE_LEN; i++) {
   232|        securedPdu[securedPduLen++] = 
   233|            (uint8)(SecOC_TxPduState[idx].freshnessValue >> 
   234|                   ((SECOC_FRESHNESS_TX_BYTE_LEN - 1u - i) * 8u));
   235|    }
   236|    
   237|    /* Add authentication code */
   238|    for (i = 0u; i < SECOC_AUTH_INFO_LENGTH; i++) {
   239|        securedPdu[securedPduLen++] = authCode[i];
   240|    }
   241|    
   242|    /* Transmit via PduR */
   243|    {
   244|        PduInfoType pduInfo;
   245|        pduInfo.SduDataPtr = securedPdu;
   246|        pduInfo.SduLength = securedPduLen;
   247|        pduInfo.MetaDataPtr = NULL_PTR;
   248|        
   249|        result = PduR_SecOCTransmit(pduId, &pduInfo);
   250|    }
   251|    
   252|    SecOC_TxBuffers[idx].inUse = FALSE;
   253|    SecOC_TxPduState[idx].txInProgress = FALSE;
   254|    
   255|    return result;
   256|}
   257|
   258|/**
   259| * @brief Process RX PDU - verify security
   260| */
   261|static void SecOC_ProcessRxPdu(PduIdType pduId)
   262|{
   263|    sint16 idx;
   264|    uint8 authData[SECOC_MAX_PDU_LENGTH + SECOC_FRESHNESS_BYTE_LEN + 1u];
   265|    uint16 authDataLen;
   266|    uint8 receivedAuthCode[SECOC_MAX_AUTH_INFO_LEN];
   267|    Csm_VerifyResultType verifyResult;
   268|    uint8* securedPdu;
   269|    PduLengthType securedPduLen;
   270|    PduLengthType originalDataLen;
   271|    uint8 i;
   272|    uint32 receivedFreshness;
   273|    Std_ReturnType result;
   274|    
   275|    idx = SecOC_GetRxPduIndex(pduId);
   276|    if (idx < 0) {
   277|        return;
   278|    }
   279|    
   280|    if (!SecOC_RxBuffers[idx].inUse) {
   281|        return;
   282|    }
   283|    
   284|    SecOC_RxPduState[idx].authInProgress = TRUE;
   285|    
   286|    securedPdu = SecOC_RxBuffers[idx].data;
   287|    securedPduLen = SecOC_RxBuffers[idx].length;
   288|    
   289|    /* Calculate original data length */
   290|    if (securedPduLen < (SECOC_FRESHNESS_TX_BYTE_LEN + SECOC_AUTH_INFO_LENGTH)) {
   291|        SecOC_RxPduState[idx].status = SECOC_VERIFICATIONFAILURE_STATUS;
   292|        SecOC_RxPduState[idx].lastResult = SECOC_AUTHENTICATIONBUILDFAILURE;
   293|        SecOC_RxPduState[idx].authInProgress = FALSE;
   294|        SecOC_RxBuffers[idx].inUse = FALSE;
   295|        return;
   296|    }
   297|    
   298|    originalDataLen = securedPduLen - SECOC_FRESHNESS_TX_BYTE_LEN - SECOC_AUTH_INFO_LENGTH;
   299|    
   300|    /* Extract truncated freshness value */
   301|    receivedFreshness = 0u;
   302|    for (i = 0u; i < SECOC_FRESHNESS_TX_BYTE_LEN; i++) {
   303|        receivedFreshness = (receivedFreshness << 8u) | 
   304|                           securedPdu[originalDataLen + i];
   305|    }
   306|    
   307|    /* Reconstruct full freshness value (simplified - should use freshness manager) */
   308|    receivedFreshness |= (SecOC_RxPduState[idx].lastVerifiedFreshness & 
   309|                         ~(0xFFFFFFFFu >> SECOC_FRESHNESS_VALUE_TX_LENGTH));
   310|    
   311|    /* Extract authentication code */
   312|    for (i = 0u; i < SECOC_AUTH_INFO_LENGTH; i++) {
   313|        receivedAuthCode[i] = securedPdu[originalDataLen + SECOC_FRESHNESS_TX_BYTE_LEN + i];
   314|    }
   315|    
   316|    /* Build authentication data for verification */
   317|    SecOC_BuildAuthData(securedPdu, originalDataLen, receivedFreshness, (uint8)pduId,
   318|                        authData, &authDataLen);
   319|    
   320|    /* Verify authentication code */
   321|    result = SecOC_VerifyAuthCode(authData, authDataLen, receivedAuthCode,
   322|                                   SECOC_AUTH_INFO_LENGTH, &verifyResult);
   323|    
   324|    if ((result == E_OK) && (verifyResult == CSM_E_VER_OK)) {
   325|        SecOC_RxPduState[idx].status = SECOC_VERIFICATIONSUCCESS_STATUS;
   326|        SecOC_RxPduState[idx].lastResult = SECOC_VERIFICATIONSUCCESS;
   327|        SecOC_RxPduState[idx].lastVerifiedFreshness = receivedFreshness;
   328|        
   329|        /* Forward verified data to upper layer */
   330|        {
   331|            PduInfoType pduInfo;
   332|            pduInfo.SduDataPtr = securedPdu;
   333|            pduInfo.SduLength = originalDataLen;
   334|            pduInfo.MetaDataPtr = NULL_PTR;
   335|            
   336|            PduR_SecOCRxIndication(pduId, &pduInfo);
   337|        }
   338|    } else {
   339|        SecOC_RxPduState[idx].status = SECOC_VERIFICATIONFAILURE_STATUS;
   340|        SecOC_RxPduState[idx].lastResult = SECOC_VERIFICATIONFAILURE;
   341|        SecOC_RxPduState[idx].retryCount++;
   342|        
   343|        if (SecOC_RxPduState[idx].retryCount >= SECOC_VERIFICATION_RETRY_COUNT) {
   344|            /* Report failure to DEM */
   345|            /* Dem_ReportErrorStatus(SECOC_E_CRYPTO_AUTH_FAILED, DEM_EVENT_STATUS_FAILED); */
   346|        }
   347|    }
   348|    
   349|    SecOC_RxPduState[idx].authInProgress = FALSE;
   350|    SecOC_RxBuffers[idx].inUse = FALSE;
   351|}
   352|
   353|/*==================================================================================================
   354| *                                    GLOBAL FUNCTIONS
   355| *==================================================================================================*/
   356|
   357|/**
   358| * @brief Initializes the SecOC module
   359| */
   360|void SecOC_Init(const SecOC_ConfigType* configPtr)
   361|{
   362|    uint16 i;
   363|    
   364|#if (SECOC_DEV_ERROR_DETECT == STD_ON)
   365|    if (SecOC_Initialized_Local == TRUE) {
   366|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_INIT, 
   367|                               SECOC_E_ALREADY_INITIALIZED);
   368|        return;
   369|    }
   370|    
   371|    if (configPtr == NULL_PTR) {
   372|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_INIT, 
   373|                               SECOC_E_PARAM_POINTER);
   374|        return;
   375|    }
   376|#endif
   377|
   378|    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0();
   379|    
   380|    /* Initialize TX state */
   381|    for (i = 0u; i < SECOC_NUM_TX_PDUS; i++) {
   382|        SecOC_TxPduState[i].freshnessValue = 0u;
   383|        SecOC_TxPduState[i].txInProgress = FALSE;
   384|        SecOC_TxBuffers[i].inUse = FALSE;
   385|        SecOC_TxBuffers[i].pduId = i;
   386|    }
   387|    
   388|    /* Initialize RX state */
   389|    for (i = 0u; i < SECOC_NUM_RX_PDUS; i++) {
   390|        SecOC_RxPduState[i].freshnessValue = 0u;
   391|        SecOC_RxPduState[i].lastVerifiedFreshness = 0u;
   392|        SecOC_RxPduState[i].status = SECOC_UNVERIFIED;
   393|        SecOC_RxPduState[i].lastResult = SECOC_NO_VERIFICATION;
   394|        SecOC_RxPduState[i].retryCount = 0u;
   395|        SecOC_RxPduState[i].authInProgress = FALSE;
   396|        SecOC_RxPduState[i].timeoutCounter = 0u;
   397|        SecOC_RxBuffers[i].inUse = FALSE;
   398|        SecOC_RxBuffers[i].pduId = i;
   399|    }
   400|    
   401|    SecOC_SyncMasterFreshness = 0u;
   402|    SecOC_ConfigPtr = configPtr;
   403|    SecOC_Initialized_Local = TRUE;
   404|    SecOC_Initialized = TRUE;
   405|    
   406|    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0();
   407|}
   408|
   409|/**
   410| * @brief Deinitializes the SecOC module
   411| */
   412|void SecOC_DeInit(void)
   413|{
   414|    uint16 i;
   415|    
   416|#if (SECOC_DEV_ERROR_DETECT == STD_ON)
   417|    if (SecOC_Initialized_Local == FALSE) {
   418|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_DEINIT, 
   419|                               SECOC_E_UNINIT);
   420|        return;
   421|    }
   422|#endif
   423|
   424|    SchM_Enter_SecOC_SECOC_EXCLUSIVE_AREA_0();
   425|    
   426|    /* Reset TX state */
   427|    for (i = 0u; i < SECOC_NUM_TX_PDUS; i++) {
   428|        SecOC_TxPduState[i].txInProgress = FALSE;
   429|        SecOC_TxBuffers[i].inUse = FALSE;
   430|    }
   431|    
   432|    /* Reset RX state */
   433|    for (i = 0u; i < SECOC_NUM_RX_PDUS; i++) {
   434|        SecOC_RxPduState[i].status = SECOC_UNVERIFIED;
   435|        SecOC_RxPduState[i].authInProgress = FALSE;
   436|        SecOC_RxBuffers[i].inUse = FALSE;
   437|    }
   438|    
   439|    SecOC_ConfigPtr = NULL_PTR;
   440|    SecOC_Initialized_Local = FALSE;
   441|    SecOC_Initialized = FALSE;
   442|    
   443|    SchM_Exit_SecOC_SECOC_EXCLUSIVE_AREA_0();
   444|}
   445|
   446|/**
   447| * @brief Gets version information
   448| */
   449|#if (SECOC_VERSION_INFO_API == STD_ON)
   450|void SecOC_GetVersionInfo(Std_VersionInfoType* versioninfo)
   451|{
   452|#if (SECOC_DEV_ERROR_DETECT == STD_ON)
   453|    if (versioninfo == NULL_PTR) {
   454|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_GETVERSIONINFO, 
   455|                               SECOC_E_PARAM_POINTER);
   456|        return;
   457|    }
   458|#endif
   459|
   460|    versioninfo->vendorID = SECOC_VENDOR_ID;
   461|    versioninfo->moduleID = SECOC_MODULE_ID;
   462|    versioninfo->sw_major_version = SECOC_SW_MAJOR_VERSION;
   463|    versioninfo->sw_minor_version = SECOC_SW_MINOR_VERSION;
   464|    versioninfo->sw_patch_version = SECOC_SW_PATCH_VERSION;
   465|}
   466|#endif
   467|
   468|/**
   469| * @brief Transmits a secured PDU
   470| */
   471|Std_ReturnType SecOC_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
   472|{
   473|    sint16 idx;
   474|    uint16 i;
   475|    
   476|#if (SECOC_DEV_ERROR_DETECT == STD_ON)
   477|    if (SecOC_Initialized_Local == FALSE) {
   478|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
   479|                               SECOC_E_UNINIT);
   480|        return E_NOT_OK;
   481|    }
   482|    
   483|    if (PduInfoPtr == NULL_PTR) {
   484|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
   485|                               SECOC_E_PARAM_POINTER);
   486|        return E_NOT_OK;
   487|    }
   488|#endif
   489|
   490|    idx = SecOC_GetTxPduIndex(TxPduId);
   491|    if (idx < 0) {
   492|#if (SECOC_DEV_ERROR_DETECT == STD_ON)
   493|        (void)Det_ReportError(SECOC_MODULE_ID, SECOC_INSTANCE_ID, SECOC_SID_IFTRANSMIT, 
   494|                               SECOC_E_INVALID_PDU_SDU_ID);
   495|#endif
   496|        return E_NOT_OK;
   497|    }
   498|    
   499|    if (SecOC_TxBuffers[idx].inUse) {
   500|        return E_NOT_OK;
   501|