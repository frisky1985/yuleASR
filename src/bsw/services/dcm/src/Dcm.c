/**
 * @file Dcm.c
 * @brief Diagnostic Communication Manager
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2|* Project              : YuleTech AutoSAR BSW
     3|* Platform             : NXP i.MX8M Mini
     4|* Peripheral           : N/A (Service Layer)
     5|* Dependencies         : PduR, Dem
     6|*
     7|* SW Version           : 1.0.0
     8|* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
     9|* Build Date           : 2026-04-15
    10|* Author               : AI Agent (Dcm Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "Dcm.h"
    20|#include "Dcm_Cfg.h"
    21|#include "dcm_transfer.h"
    22|#include "PduR.h"
    23|#include "Det.h"
    24|#include "MemMap.h"
    25|#include "string.h"
    26|
    27|/*==================================================================================================
    28|*                                  LOCAL CONSTANT DEFINITIONS
    29|==================================================================================================*/
    30|#define DCM_INSTANCE_ID                 (0x00U)
    31|
    32|/* Module state */
    33|#define DCM_STATE_UNINIT                (0x00U)
    34|#define DCM_STATE_INIT                  (0x01U)
    35|#define DCM_STATE_BUSY                  (0x02U)
    36|
    37|/* Protocol state */
    38|#define DCM_PROTOCOL_IDLE               (0x00U)
    39|#define DCM_PROTOCOL_RX_IN_PROGRESS     (0x01U)
    40|#define DCM_PROTOCOL_PROCESSING         (0x02U)
    41|#define DCM_PROTOCOL_TX_IN_PROGRESS     (0x03U)
    42|
    43|/* Response type */
    44|#define DCM_RESPONSE_POSITIVE           (0x00U)
    45|#define DCM_RESPONSE_NEGATIVE           (0x01U)
    46|
    47|/*==================================================================================================
    48|*                                  LOCAL MACRO DEFINITIONS
    49|==================================================================================================*/
    50|#if (DCM_DEV_ERROR_DETECT == STD_ON)
    51|    #define DCM_DET_REPORT_ERROR(ApiId, ErrorId) \
    52|        Det_ReportError(DCM_MODULE_ID, DCM_INSTANCE_ID, (ApiId), (ErrorId))
    53|#else
    54|    #define DCM_DET_REPORT_ERROR(ApiId, ErrorId)
    55|#endif
    56|
    57|/*==================================================================================================
    58|*                                  LOCAL TYPE DEFINITIONS
    59|==================================================================================================*/
    60|/* Protocol runtime state */
    61|typedef struct
    62|{
    63|    uint8 State;
    64|    uint8 CurrentSID;
    65|    uint8 CurrentSubFunction;
    66|    uint16 RxDataLength;
    67|    uint16 TxDataLength;
    68|    uint8 RxBuffer[DCM_RX_BUFFER_SIZE];
    69|    uint8 TxBuffer[DCM_TX_BUFFER_SIZE];
    70|    uint32 P2Timer;
    71|    uint32 S3Timer;
    72|    boolean ResponsePending;
    73|} Dcm_ProtocolStateType;
    74|
    75|/* Module internal state */
    76|typedef struct
    77|{
    78|    uint8 State;
    79|    const Dcm_ConfigType* ConfigPtr;
    80|    uint8 CurrentSession;
    81|    uint8 CurrentSecurityLevel;
    82|    uint8 SecurityAttempts;
    83|    uint32 SecurityDelayTimer;
    84|    boolean SecurityDelayActive;
    85|    Dcm_ProtocolStateType ProtocolStates[DCM_NUM_PROTOCOLS];
    86|} Dcm_InternalStateType;
    87|
    88|/*==================================================================================================
    89|*                                  LOCAL VARIABLE DECLARATIONS
    90|==================================================================================================*/
    91|#define DCM_START_SEC_VAR_CLEARED_UNSPECIFIED
    92|#include "MemMap.h"
    93|
    94|STATIC Dcm_InternalStateType Dcm_InternalState;
    95|
    96|#define DCM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    97|#include "MemMap.h"
    98|
    99|/*==================================================================================================
   100|*                                  LOCAL FUNCTION PROTOTYPES
   101|==================================================================================================*/
   102|STATIC void Dcm_ProcessRequest(uint8 ProtocolId);
   103|STATIC void Dcm_SendPositiveResponse(uint8 ProtocolId, uint8 SID, const uint8* Data, uint16 Length);
   104|STATIC void Dcm_SendNegativeResponse(uint8 ProtocolId, uint8 SID, uint8 NRC);
   105|STATIC Std_ReturnType Dcm_ProcessDiagnosticSessionControl(uint8 ProtocolId, const uint8* Data, uint16 Length);
   106|STATIC Std_ReturnType Dcm_ProcessEcuReset(uint8 ProtocolId, const uint8* Data, uint16 Length);
   107|STATIC Std_ReturnType Dcm_ProcessSecurityAccess(uint8 ProtocolId, const uint8* Data, uint16 Length);
   108|STATIC Std_ReturnType Dcm_ProcessTesterPresent(uint8 ProtocolId, const uint8* Data, uint16 Length);
   109|STATIC Std_ReturnType Dcm_ProcessReadDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length);
   110|STATIC Std_ReturnType Dcm_ProcessWriteDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length);
   111|STATIC Std_ReturnType Dcm_ProcessReadDTCInformation(uint8 ProtocolId, const uint8* Data, uint16 Length);
   112|STATIC Std_ReturnType Dcm_ProcessClearDiagnosticInformation(uint8 ProtocolId, const uint8* Data, uint16 Length);
   113|STATIC Std_ReturnType Dcm_ProcessRoutineControl(uint8 ProtocolId, const uint8* Data, uint16 Length);
   114|STATIC Std_ReturnType Dcm_ProcessRequestDownload(uint8 ProtocolId, const uint8* Data, uint16 Length);
   115|STATIC Std_ReturnType Dcm_ProcessRequestUpload(uint8 ProtocolId, const uint8* Data, uint16 Length);
   116|STATIC Std_ReturnType Dcm_ProcessTransferData(uint8 ProtocolId, const uint8* Data, uint16 Length);
   117|STATIC Std_ReturnType Dcm_ProcessRequestTransferExit(uint8 ProtocolId, const uint8* Data, uint16 Length);
   118|STATIC const Dcm_DIDConfigType* Dcm_FindDID(uint16 DID);
   119|STATIC const Dcm_RIDConfigType* Dcm_FindRID(uint16 RID);
   120|
   121|/*==================================================================================================
   122|*                                      LOCAL FUNCTIONS
   123|==================================================================================================*/
   124|#define DCM_START_SEC_CODE
   125|#include "MemMap.h"
   126|
   127|/**
   128| * @brief   Find DID configuration by DID value
   129| */
   130|STATIC const Dcm_DIDConfigType* Dcm_FindDID(uint16 DID)
   131|{
   132|    const Dcm_DIDConfigType* result = NULL_PTR;
   133|    uint8 i;
   134|
   135|    if (Dcm_InternalState.ConfigPtr != NULL_PTR)
   136|    {
   137|        for (i = 0U; i < Dcm_InternalState.ConfigPtr->NumDIDs; i++)
   138|        {
   139|            if (Dcm_InternalState.ConfigPtr->DIDs[i].DID == DID)
   140|            {
   141|                result = &Dcm_InternalState.ConfigPtr->DIDs[i];
   142|                break;
   143|            }
   144|        }
   145|    }
   146|
   147|    return result;
   148|}
   149|
   150|/**
   151| * @brief   Find RID configuration by RID value
   152| */
   153|STATIC const Dcm_RIDConfigType* Dcm_FindRID(uint16 RID)
   154|{
   155|    const Dcm_RIDConfigType* result = NULL_PTR;
   156|    uint8 i;
   157|
   158|    if (Dcm_InternalState.ConfigPtr != NULL_PTR)
   159|    {
   160|        for (i = 0U; i < Dcm_InternalState.ConfigPtr->NumRIDs; i++)
   161|        {
   162|            if (Dcm_InternalState.ConfigPtr->RIDs[i].RID == RID)
   163|            {
   164|                result = &Dcm_InternalState.ConfigPtr->RIDs[i];
   165|                break;
   166|            }
   167|        }
   168|    }
   169|
   170|    return result;
   171|}
   172|
   173|/**
   174| * @brief   Send positive response
   175| */
   176|STATIC void Dcm_SendPositiveResponse(uint8 ProtocolId, uint8 SID, const uint8* Data, uint16 Length)
   177|{
   178|    Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[ProtocolId];
   179|    uint8 i;
   180|
   181|    /* Build positive response: SID + 0x40 */
   182|    protocolState->TxBuffer[0] = SID + 0x40U;
   183|
   184|    /* Copy response data */
   185|    for (i = 0U; i < Length; i++)
   186|    {
   187|        protocolState->TxBuffer[i + 1U] = Data[i];
   188|    }
   189|
   190|    protocolState->TxDataLength = Length + 1U;
   191|    protocolState->State = DCM_PROTOCOL_TX_IN_PROGRESS;
   192|
   193|    /* Trigger transmission via PduR */
   194|    {
   195|        PduInfoType pduInfo;
   196|        pduInfo.SduDataPtr = protocolState->TxBuffer;
   197|        pduInfo.SduLength = protocolState->TxDataLength;
   198|        pduInfo.MetaDataPtr = NULL_PTR;
   199|
   200|        (void)PduR_Transmit(ProtocolId, &pduInfo);
   201|    }
   202|}
   203|
   204|/**
   205| * @brief   Send negative response
   206| */
   207|STATIC void Dcm_SendNegativeResponse(uint8 ProtocolId, uint8 SID, uint8 NRC)
   208|{
   209|    Dcm_ProtocolStateType* protocolState = &Dcm_InternalState.ProtocolStates[ProtocolId];
   210|
   211|    /* Build negative response: 0x7F + SID + NRC */
   212|    protocolState->TxBuffer[0] = 0x7FU;
   213|    protocolState->TxBuffer[1] = SID;
   214|    protocolState->TxBuffer[2] = NRC;
   215|
   216|    protocolState->TxDataLength = 3U;
   217|    protocolState->State = DCM_PROTOCOL_TX_IN_PROGRESS;
   218|
   219|    /* Trigger transmission via PduR */
   220|    {
   221|        PduInfoType pduInfo;
   222|        pduInfo.SduDataPtr = protocolState->TxBuffer;
   223|        pduInfo.SduLength = protocolState->TxDataLength;
   224|        pduInfo.MetaDataPtr = NULL_PTR;
   225|
   226|        (void)PduR_Transmit(ProtocolId, &pduInfo);
   227|    }
   228|}
   229|
   230|/**
   231| * @brief   Process Diagnostic Session Control service
   232| */
   233|STATIC Std_ReturnType Dcm_ProcessDiagnosticSessionControl(uint8 ProtocolId, const uint8* Data, uint16 Length)
   234|{
   235|    Std_ReturnType result = E_NOT_OK;
   236|    uint8 sessionType;
   237|    uint8 responseData[4];
   238|
   239|    if (Length >= 1U)
   240|    {
   241|        sessionType = Data[0];
   242|
   243|        /* Validate session type */
   244|        switch (sessionType)
   245|        {
   246|            case DCM_DEFAULT_SESSION:
   247|            case DCM_PROGRAMMING_SESSION:
   248|            case DCM_EXTENDED_DIAGNOSTIC_SESSION:
   249|            case DCM_SAFETY_SYSTEM_DIAGNOSTIC_SESSION:
   250|                /* Update current session */
   251|                Dcm_InternalState.CurrentSession = sessionType;
   252|
   253|                /* Build response: session type + P2 + P2* */
   254|                responseData[0] = sessionType;
   255|                responseData[1] = (uint8)(DCM_P2SERVER_MAX >> 8);
   256|                responseData[2] = (uint8)(DCM_P2SERVER_MAX);
   257|                responseData[3] = (uint8)(DCM_P2STAR_SERVER_MAX >> 8);
   258|                responseData[4] = (uint8)(DCM_P2STAR_SERVER_MAX);
   259|
   260|                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL, responseData, 5U);
   261|                result = E_OK;
   262|                break;
   263|
   264|            default:
   265|                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
   266|                break;
   267|        }
   268|    }
   269|    else
   270|    {
   271|        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_DIAGNOSTIC_SESSION_CONTROL, DCM_E_INCORRECT_MESSAGE_LENGTH);
   272|    }
   273|
   274|    return result;
   275|}
   276|
   277|/**
   278| * @brief   Process ECU Reset service
   279| */
   280|STATIC Std_ReturnType Dcm_ProcessEcuReset(uint8 ProtocolId, const uint8* Data, uint16 Length)
   281|{
   282|    Std_ReturnType result = E_NOT_OK;
   283|    uint8 resetType;
   284|    uint8 responseData[1];
   285|
   286|    if (Length >= 1U)
   287|    {
   288|        resetType = Data[0];
   289|
   290|        /* Validate reset type */
   291|        switch (resetType)
   292|        {
   293|            case 0x01U: /* Hard Reset */
   294|            case 0x02U: /* Key Off On Reset */
   295|            case 0x03U: /* Soft Reset */
   296|            case 0x04U: /* Enable Rapid Power Shutdown */
   297|            case 0x05U: /* Disable Rapid Power Shutdown */
   298|                /* Build response: reset type */
   299|                responseData[0] = resetType;
   300|                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_ECU_RESET, responseData, 1U);
   301|
   302|                /* Perform reset (implementation specific) */
   303|                /* Mcu_PerformReset(); */
   304|
   305|                result = E_OK;
   306|                break;
   307|
   308|            default:
   309|                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ECU_RESET, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
   310|                break;
   311|        }
   312|    }
   313|    else
   314|    {
   315|        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_ECU_RESET, DCM_E_INCORRECT_MESSAGE_LENGTH);
   316|    }
   317|
   318|    return result;
   319|}
   320|
   321|/**
   322| * @brief   Process Security Access service
   323| */
   324|STATIC Std_ReturnType Dcm_ProcessSecurityAccess(uint8 ProtocolId, const uint8* Data, uint16 Length)
   325|{
   326|    Std_ReturnType result = E_NOT_OK;
   327|    uint8 subFunction;
   328|    uint8 securityLevel;
   329|    uint8 responseData[DCM_SEED_SIZE + 1];
   330|    uint8 i;
   331|
   332|    if (Length >= 1U)
   333|    {
   334|        subFunction = Data[0];
   335|        securityLevel = subFunction & 0x3FU;
   336|
   337|        if ((subFunction & 0x40U) == 0U)
   338|        {
   339|            /* Request Seed */
   340|            if (Dcm_InternalState.SecurityDelayActive)
   341|            {
   342|                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_REQUIRED_TIME_DELAY_NOT_EXPIRED);
   343|            }
   344|            else if (Dcm_InternalState.SecurityAttempts >= DCM_MAX_SECURITY_ATTEMPTS)
   345|            {
   346|                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_EXCEED_NUMBER_OF_ATTEMPTS);
   347|                Dcm_InternalState.SecurityDelayActive = TRUE;
   348|                Dcm_InternalState.SecurityDelayTimer = DCM_SECURITY_DELAY_TIME;
   349|            }
   350|            else
   351|            {
   352|                /* Generate seed (simplified - should be random) */
   353|                responseData[0] = subFunction;
   354|                for (i = 0U; i < DCM_SEED_SIZE; i++)
   355|                {
   356|                    responseData[i + 1U] = (uint8)(0xA5U + i);
   357|                }
   358|
   359|                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, responseData, DCM_SEED_SIZE + 1U);
   360|                result = E_OK;
   361|            }
   362|        }
   363|        else
   364|        {
   365|            /* Send Key */
   366|            if (Length >= (DCM_KEY_SIZE + 1U))
   367|            {
   368|                /* Validate key (simplified - should compare calculated key) */
   369|                boolean keyValid = TRUE;
   370|
   371|                /* Check if key matches expected value */
   372|                for (i = 0U; i < DCM_KEY_SIZE; i++)
   373|                {
   374|                    if (Data[i + 1U] != (uint8)(0xA5U + i))
   375|                    {
   376|                        keyValid = FALSE;
   377|                        break;
   378|                    }
   379|                }
   380|
   381|                if (keyValid)
   382|                {
   383|                    Dcm_InternalState.CurrentSecurityLevel = securityLevel;
   384|                    Dcm_InternalState.SecurityAttempts = 0U;
   385|
   386|                    responseData[0] = subFunction;
   387|                    Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, responseData, 1U);
   388|                    result = E_OK;
   389|                }
   390|                else
   391|                {
   392|                    Dcm_InternalState.SecurityAttempts++;
   393|                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_INVALID_KEY);
   394|                }
   395|            }
   396|            else
   397|            {
   398|                Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_INCORRECT_MESSAGE_LENGTH);
   399|            }
   400|        }
   401|    }
   402|    else
   403|    {
   404|        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_SECURITY_ACCESS, DCM_E_INCORRECT_MESSAGE_LENGTH);
   405|    }
   406|
   407|    return result;
   408|}
   409|
   410|/**
   411| * @brief   Process Tester Present service
   412| */
   413|STATIC Std_ReturnType Dcm_ProcessTesterPresent(uint8 ProtocolId, const uint8* Data, uint16 Length)
   414|{
   415|    Std_ReturnType result = E_NOT_OK;
   416|    uint8 subFunction;
   417|    uint8 responseData[1];
   418|
   419|    if (Length >= 1U)
   420|    {
   421|        subFunction = Data[0];
   422|
   423|        if ((subFunction == 0x00U) || (subFunction == 0x80U))
   424|        {
   425|            /* Reset S3 timer */
   426|            Dcm_InternalState.ProtocolStates[ProtocolId].S3Timer = DCM_S3SERVER;
   427|
   428|            /* Send response (suppress for subFunction 0x80 if required) */
   429|            if (subFunction == 0x00U)
   430|            {
   431|                responseData[0] = subFunction;
   432|                Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_TESTER_PRESENT, responseData, 1U);
   433|            }
   434|
   435|            result = E_OK;
   436|        }
   437|        else
   438|        {
   439|            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_TESTER_PRESENT, DCM_E_SUBFUNCTION_NOT_SUPPORTED);
   440|        }
   441|    }
   442|    else
   443|    {
   444|        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_TESTER_PRESENT, DCM_E_INCORRECT_MESSAGE_LENGTH);
   445|    }
   446|
   447|    return result;
   448|}
   449|
   450|/**
   451| * @brief   Process Read Data By Identifier service
   452| */
   453|STATIC Std_ReturnType Dcm_ProcessReadDataByIdentifier(uint8 ProtocolId, const uint8* Data, uint16 Length)
   454|{
   455|    Std_ReturnType result = E_NOT_OK;
   456|    uint16 did;
   457|    const Dcm_DIDConfigType* didConfig;
   458|    uint8 responseData[DCM_TX_BUFFER_SIZE];
   459|
   460|    if (Length >= 2U)
   461|    {
   462|        /* Extract DID (big endian) */
   463|        did = ((uint16)Data[0] << 8U) | Data[1];
   464|
   465|        /* Find DID configuration */
   466|        didConfig = Dcm_FindDID(did);
   467|
   468|        if (didConfig != NULL_PTR)
   469|        {
   470|            /* Check security level */
   471|            if (Dcm_InternalState.CurrentSecurityLevel >= didConfig->SecurityLevel)
   472|            {
   473|                /* Check session type */
   474|                if ((didConfig->SessionType == DCM_DEFAULT_SESSION) ||
   475|                    (Dcm_InternalState.CurrentSession == didConfig->SessionType))
   476|                {
   477|                    /* Read data */
   478|                    if (didConfig->ReadDataFnc != NULL_PTR)
   479|                    {
   480|                        if (didConfig->ReadDataFnc(&responseData[2]) == E_OK)
   481|                        {
   482|                            /* Build response: DID + data */
   483|                            responseData[0] = (uint8)(did >> 8U);
   484|                            responseData[1] = (uint8)(did);
   485|                            Dcm_SendPositiveResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, responseData, didConfig->DataLength + 2U);
   486|                            result = E_OK;
   487|                        }
   488|                        else
   489|                        {
   490|                            Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
   491|                        }
   492|                    }
   493|                    else
   494|                    {
   495|                        Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_CONDITIONS_NOT_CORRECT);
   496|                    }
   497|                }
   498|                else
   499|                {
   500|                    Dcm_SendNegativeResponse(ProtocolId, DCM_SERVICE_READ_DATA_BY_IDENTIFIER, DCM_E_SERVICE_NOT_SUPPORTED);
   501|