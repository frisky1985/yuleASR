/**
 * @file SomeIpTp.c
 * @brief SOME/IP Transport Protocol
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

     1|/*==================================================================================================
     2|* Project              : YuleTech AutoSAR BSW
     3|* Platform             : NXP i.MX8M Mini
     4|* Peripheral           : Ethernet
     5|* Dependencies         : SoAd, Det
     6|*
     7|* SW Version           : 4.7.0
     8|* Build Version        : YULETECH_AUTOSAR_4.7.0
     9|* Build Date           : 2026-04-29
    10|* Author               : AI Agent (SomeIpTp Development)
    11|*
    12|* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
    13|* All Rights Reserved.
    14|==================================================================================================*/
    15|
    16|/*==================================================================================================
    17|*                                             INCLUDES
    18|==================================================================================================*/
    19|#include "SomeIpTp.h"
    20|#include "SomeIpTp_Cfg.h"
    21|#include "Det.h"
    22|#include "MemMap.h"
    23|#include <string.h>
    24|
    25|/*==================================================================================================
    26|*                                  LOCAL CONSTANT DEFINITIONS
    27|==================================================================================================*/
    28|#define SOMEIPTP_STATE_UNINIT                   (0x00U)
    29|#define SOMEIPTP_STATE_INIT                     (0x01U)
    30|
    31|/* TP Header field offsets */
    32|#define SOMEIPTP_HDR_OFFSET_FLAGS               (0U)
    33|#define SOMEIPTP_HDR_SIZE                       (4U)
    34|
    35|/*==================================================================================================
    36|*                                  LOCAL MACRO DEFINITIONS
    37|==================================================================================================*/
    38|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
    39|    #define SOMEIPTP_DET_REPORT_ERROR(ApiId, ErrorId) \
    40|        Det_ReportError(SOMEIPTP_MODULE_ID, SOMEIPTP_INSTANCE_ID, (ApiId), (ErrorId))
    41|#else
    42|    #define SOMEIPTP_DET_REPORT_ERROR(ApiId, ErrorId)
    43|#endif
    44|
    45|#define SOMEIPTP_IS_VALID_CHANNEL_ID(Id) \
    46|    (((Id) < SOMEIPTP_NUMBER_OF_CHANNELS) ? TRUE : FALSE)
    47|
    48|#define SOMEIPTP_SET_TP_OFFSET(buffer, offset, moreSeg) \
    49|    do { \
    50|        uint32 value = ((moreSeg) ? 0x40000000UL : 0x00UL) | ((offset) & 0x3FFFFFFFUL); \
    51|        (buffer)[0] = (uint8)((value) >> 24); \
    52|        (buffer)[1] = (uint8)((value) >> 16); \
    53|        (buffer)[2] = (uint8)((value) >> 8); \
    54|        (buffer)[3] = (uint8)(value); \
    55|    } while(0)
    56|
    57|#define SOMEIPTP_GET_TP_OFFSET(buffer, offset, moreSeg) \
    58|    do { \
    59|        uint32 value = (((uint32)(buffer)[0]) << 24) | \
    60|                       (((uint32)(buffer)[1]) << 16) | \
    61|                       (((uint32)(buffer)[2]) << 8) | \
    62|                       ((uint32)(buffer)[3]); \
    63|        (offset) = (value) & 0x3FFFFFFFUL; \
    64|        (moreSeg) = ((value) & 0x40000000UL) ? TRUE : FALSE; \
    65|    } while(0)
    66|
    67|/*==================================================================================================
    68|*                                  LOCAL TYPE DEFINITIONS
    69|==================================================================================================*/
    70|typedef struct {
    71|    uint8 State;
    72|    const SomeIpTp_ConfigType* ConfigPtr;
    73|    SomeIpTp_ChannelType Channels[SOMEIPTP_NUMBER_OF_CHANNELS];
    74|    uint8 RxBufferPool[SOMEIPTP_NUMBER_OF_CHANNELS][SOMEIPTP_RX_BUFFER_SIZE];
    75|} SomeIpTp_InternalStateType;
    76|
    77|/*==================================================================================================
    78|*                                  LOCAL VARIABLE DECLARATIONS
    79|==================================================================================================*/
    80|#define SOMEIPTP_START_SEC_VAR_CLEARED_UNSPECIFIED
    81|#include "MemMap.h"
    82|
    83|STATIC SomeIpTp_InternalStateType SomeIpTp_InternalState;
    84|
    85|#define SOMEIPTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    86|#include "MemMap.h"
    87|
    88|/*==================================================================================================
    89|*                                  LOCAL FUNCTION PROTOTYPES
    90|==================================================================================================*/
    91|STATIC Std_ReturnType SomeIpTp_FindChannelByTxPduId(PduIdType TxPduId, uint16* ChannelIdPtr);
    92|STATIC Std_ReturnType SomeIpTp_FindChannelByRxPduId(PduIdType RxPduId, uint16* ChannelIdPtr);
    93|STATIC Std_ReturnType SomeIpTp_SendNextSegment(uint16 ChannelId);
    94|STATIC void SomeIpTp_ProcessReassembly(uint16 ChannelId, const PduInfoType* PduInfoPtr);
    95|STATIC void SomeIpTp_UpdateTimeouts(void);
    96|STATIC void SomeIpTp_ResetChannel(uint16 ChannelId);
    97|
    98|/*==================================================================================================
    99|*                                      LOCAL FUNCTIONS
   100|==================================================================================================*/
   101|#define SOMEIPTP_START_SEC_CODE
   102|#include "MemMap.h"
   103|
   104|/**
   105| * @brief   Find channel ID by TX PDU ID
   106| */
   107|STATIC Std_ReturnType SomeIpTp_FindChannelByTxPduId(PduIdType TxPduId, uint16* ChannelIdPtr)
   108|{
   109|    Std_ReturnType result = E_NOT_OK;
   110|    uint16 i;
   111|    const SomeIpTp_ConfigType* configPtr = SomeIpTp_InternalState.ConfigPtr;
   112|
   113|    if (configPtr != NULL_PTR)
   114|    {
   115|        for (i = 0U; i < configPtr->NumChannels; i++)
   116|        {
   117|            if (configPtr->ChannelConfigs[i].TxPduId == TxPduId)
   118|            {
   119|                *ChannelIdPtr = i;
   120|                result = E_OK;
   121|                break;
   122|            }
   123|        }
   124|    }
   125|
   126|    return result;
   127|}
   128|
   129|/**
   130| * @brief   Find channel ID by RX PDU ID
   131| */
   132|STATIC Std_ReturnType SomeIpTp_FindChannelByRxPduId(PduIdType RxPduId, uint16* ChannelIdPtr)
   133|{
   134|    Std_ReturnType result = E_NOT_OK;
   135|    uint16 i;
   136|    const SomeIpTp_ConfigType* configPtr = SomeIpTp_InternalState.ConfigPtr;
   137|
   138|    if (configPtr != NULL_PTR)
   139|    {
   140|        for (i = 0U; i < configPtr->NumChannels; i++)
   141|        {
   142|            if (configPtr->ChannelConfigs[i].RxPduId == RxPduId)
   143|            {
   144|                *ChannelIdPtr = i;
   145|                result = E_OK;
   146|                break;
   147|            }
   148|        }
   149|    }
   150|
   151|    return result;
   152|}
   153|
   154|/**
   155| * @brief   Send next segment
   156| */
   157|STATIC Std_ReturnType SomeIpTp_SendNextSegment(uint16 ChannelId)
   158|{
   159|    Std_ReturnType result = E_NOT_OK;
   160|    SomeIpTp_ChannelType* channelPtr;
   161|    SomeIpTp_TxBufferType* txBufPtr;
   162|    const SomeIpTp_ChannelConfigType* configPtr;
   163|    uint8 segmentData[SOMEIPTP_MAX_SEGMENT_SIZE + SOMEIPTP_HDR_SIZE];
   164|    uint32 remainingLen;
   165|    uint16 segLen;
   166|    boolean moreSeg;
   167|    PduInfoType pduInfo;
   168|
   169|    if (ChannelId < SOMEIPTP_NUMBER_OF_CHANNELS)
   170|    {
   171|        channelPtr = &SomeIpTp_InternalState.Channels[ChannelId];
   172|        txBufPtr = &channelPtr->TxBuffer;
   173|        configPtr = &SomeIpTp_InternalState.ConfigPtr->ChannelConfigs[ChannelId];
   174|
   175|        if (channelPtr->State == SOMEIPTP_CHANNEL_TX_ACTIVE)
   176|        {
   177|            remainingLen = txBufPtr->RemainingLength;
   178|
   179|            if (remainingLen > 0U)
   180|            {
   181|                /* Calculate segment size */
   182|                segLen = (remainingLen > configPtr->MaxSegmentSize) ? 
   183|                         configPtr->MaxSegmentSize : (uint16)remainingLen;
   184|                moreSeg = (remainingLen > configPtr->MaxSegmentSize);
   185|
   186|                /* Build TP header */
   187|                SOMEIPTP_SET_TP_OFFSET(segmentData, txBufPtr->CurrentOffset, moreSeg);
   188|
   189|                /* Copy segment data */
   190|                (void)memcpy(&segmentData[SOMEIPTP_HDR_SIZE], 
   191|                            &txBufPtr->Data[txBufPtr->CurrentOffset], segLen);
   192|
   193|                /* Send segment via SoAd */
   194|                pduInfo.SduDataPtr = segmentData;
   195|                pduInfo.SduLength = segLen + SOMEIPTP_HDR_SIZE;
   196|                pduInfo.MetaDataPtr = NULL_PTR;
   197|
   198|                /* Call SoAd_Transmit */
   199|                extern Std_ReturnType SoAd_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
   200|                result = SoAd_Transmit(configPtr->TxPduId, &pduInfo);
   201|
   202|                if (result == E_OK)
   203|                {
   204|                    /* Update transmission state */
   205|                    txBufPtr->CurrentOffset += segLen;
   206|                    txBufPtr->RemainingLength -= segLen;
   207|                    channelPtr->State = SOMEIPTP_CHANNEL_TX_WAIT_CONFIRM;
   208|                    channelPtr->TimeoutCounter = configPtr->TxTimeout / 
   209|                                                 SOMEIPTP_MAIN_FUNCTION_PERIOD_MS;
   210|                    channelPtr->RetryCount = 0U;
   211|                }
   212|            }
   213|            else
   214|            {
   215|                /* Transmission complete */
   216|                channelPtr->State = SOMEIPTP_CHANNEL_IDLE;
   217|                result = E_OK;
   218|            }
   219|        }
   220|    }
   221|
   222|    return result;
   223|}
   224|
   225|/**
   226| * @brief   Process received segment for reassembly
   227| */
   228|STATIC void SomeIpTp_ProcessReassembly(uint16 ChannelId, const PduInfoType* PduInfoPtr)
   229|{
   230|    SomeIpTp_ChannelType* channelPtr;
   231|    SomeIpTp_RxBufferType* rxBufPtr;
   232|    const SomeIpTp_ChannelConfigType* configPtr;
   233|    uint32 offset;
   234|    boolean moreSeg;
   235|    uint16 payloadLen;
   236|
   237|    if ((ChannelId < SOMEIPTP_NUMBER_OF_CHANNELS) && (PduInfoPtr != NULL_PTR))
   238|    {
   239|        channelPtr = &SomeIpTp_InternalState.Channels[ChannelId];
   240|        rxBufPtr = &channelPtr->RxBuffer;
   241|        configPtr = &SomeIpTp_InternalState.ConfigPtr->ChannelConfigs[ChannelId];
   242|
   243|        if (PduInfoPtr->SduLength >= SOMEIPTP_HDR_SIZE)
   244|        {
   245|            /* Parse TP header */
   246|            SOMEIPTP_GET_TP_OFFSET(PduInfoPtr->SduDataPtr, offset, moreSeg);
   247|            payloadLen = PduInfoPtr->SduLength - SOMEIPTP_HDR_SIZE;
   248|
   249|            /* Check if this is a new session or continuation */
   250|            if (offset == 0U)
   251|            {
   252|                /* New session - reset buffer */
   253|                rxBufPtr->Length = 0U;
   254|                rxBufPtr->NextOffset = 0U;
   255|                rxBufPtr->IsComplete = FALSE;
   256|                channelPtr->State = SOMEIPTP_CHANNEL_RX_ACTIVE;
   257|            }
   258|
   259|            /* Validate offset */
   260|            if (offset == rxBufPtr->NextOffset)
   261|            {
   262|                /* Copy segment data to reassembly buffer */
   263|                if ((rxBufPtr->Length + payloadLen) <= rxBufPtr->MaxLength)
   264|                {
   265|                    (void)memcpy(&rxBufPtr->Data[rxBufPtr->Length], 
   266|                                &PduInfoPtr->SduDataPtr[SOMEIPTP_HDR_SIZE], payloadLen);
   267|                    rxBufPtr->Length += payloadLen;
   268|                    rxBufPtr->NextOffset += payloadLen;
   269|                    rxBufPtr->MoreSegmentsExpected = moreSeg;
   270|
   271|                    /* Update timeout */
   272|                    channelPtr->TimeoutCounter = configPtr->RxTimeout / 
   273|                                                 SOMEIPTP_MAIN_FUNCTION_PERIOD_MS;
   274|
   275|                    /* Check if complete */
   276|                    if (!moreSeg)
   277|                    {
   278|                        rxBufPtr->IsComplete = TRUE;
   279|                        channelPtr->State = SOMEIPTP_CHANNEL_RX_COMPLETED;
   280|                        
   281|                        /* Notify upper layer (SomeIpXf) via callback */
   282|                        extern void SomeIpXf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
   283|                        PduInfoType completePdu;
   284|                        completePdu.SduDataPtr = rxBufPtr->Data;
   285|                        completePdu.SduLength = (PduLengthType)rxBufPtr->Length;
   286|                        completePdu.MetaDataPtr = NULL_PTR;
   287|                        SomeIpXf_RxIndication(configPtr->RxPduId, &completePdu);
   288|                    }
   289|                }
   290|            }
   291|            else
   292|            {
   293|                /* Out of sequence segment - reset */
   294|                SomeIpTp_ResetChannel(ChannelId);
   295|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
   296|                SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_RXINDICATION, SOMEIPTP_E_REASSEMBLY_ERROR);
   297|#endif
   298|            }
   299|        }
   300|    }
   301|}
   302|
   303|/**
   304| * @brief   Update channel timeouts
   305| */
   306|STATIC void SomeIpTp_UpdateTimeouts(void)
   307|{
   308|    uint16 i;
   309|    SomeIpTp_ChannelType* channelPtr;
   310|
   311|    for (i = 0U; i < SOMEIPTP_NUMBER_OF_CHANNELS; i++)
   312|    {
   313|        channelPtr = &SomeIpTp_InternalState.Channels[i];
   314|
   315|        if ((channelPtr->State == SOMEIPTP_CHANNEL_TX_WAIT_CONFIRM) ||
   316|            (channelPtr->State == SOMEIPTP_CHANNEL_RX_ACTIVE))
   317|        {
   318|            if (channelPtr->TimeoutCounter > 0U)
   319|            {
   320|                channelPtr->TimeoutCounter--;
   321|                
   322|                if (channelPtr->TimeoutCounter == 0U)
   323|                {
   324|                    /* Timeout occurred */
   325|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
   326|                    SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_MAINFUNCTION, SOMEIPTP_E_TIMEOUT);
   327|#endif
   328|                    SomeIpTp_ResetChannel(i);
   329|                }
   330|            }
   331|        }
   332|    }
   333|}
   334|
   335|/**
   336| * @brief   Reset channel to idle state
   337| */
   338|STATIC void SomeIpTp_ResetChannel(uint16 ChannelId)
   339|{
   340|    if (ChannelId < SOMEIPTP_NUMBER_OF_CHANNELS)
   341|    {
   342|        SomeIpTp_ChannelType* channelPtr = &SomeIpTp_InternalState.Channels[ChannelId];
   343|        
   344|        channelPtr->State = SOMEIPTP_CHANNEL_IDLE;
   345|        channelPtr->TimeoutCounter = 0U;
   346|        channelPtr->RetryCount = 0U;
   347|        channelPtr->RxBuffer.Length = 0U;
   348|        channelPtr->RxBuffer.NextOffset = 0U;
   349|        channelPtr->RxBuffer.IsComplete = FALSE;
   350|        channelPtr->RxBuffer.MoreSegmentsExpected = FALSE;
   351|        channelPtr->TxBuffer.CurrentOffset = 0U;
   352|        channelPtr->TxBuffer.RemainingLength = 0U;
   353|    }
   354|}
   355|
   356|/*==================================================================================================
   357|*                                      GLOBAL FUNCTIONS
   358|==================================================================================================*/
   359|
   360|/**
   361| * @brief   Initializes the SOME/IP TP module
   362| */
   363|void SomeIpTp_Init(const SomeIpTp_ConfigType* ConfigPtr)
   364|{
   365|    uint16 i;
   366|
   367|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
   368|    if (SomeIpTp_InternalState.State == SOMEIPTP_STATE_INIT)
   369|    {
   370|        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_INIT, SOMEIPTP_E_ALREADY_INITIALIZED);
   371|        return;
   372|    }
   373|
   374|    if (ConfigPtr == NULL_PTR)
   375|    {
   376|        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_INIT, SOMEIPTP_E_PARAM_POINTER);
   377|        return;
   378|    }
   379|#endif
   380|
   381|    /* Store configuration pointer */
   382|    SomeIpTp_InternalState.ConfigPtr = ConfigPtr;
   383|
   384|    /* Initialize channels */
   385|    for (i = 0U; i < SOMEIPTP_NUMBER_OF_CHANNELS; i++)
   386|    {
   387|        SomeIpTp_ResetChannel(i);
   388|        
   389|        /* Setup RX buffer */
   390|        SomeIpTp_InternalState.Channels[i].RxBuffer.Data = 
   391|            SomeIpTp_InternalState.RxBufferPool[i];
   392|        SomeIpTp_InternalState.Channels[i].RxBuffer.MaxLength = SOMEIPTP_RX_BUFFER_SIZE;
   393|    }
   394|
   395|    /* Set module state to initialized */
   396|    SomeIpTp_InternalState.State = SOMEIPTP_STATE_INIT;
   397|}
   398|
   399|/**
   400| * @brief   Deinitializes the SOME/IP TP module
   401| */
   402|void SomeIpTp_DeInit(void)
   403|{
   404|    uint16 i;
   405|
   406|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
   407|    if (SomeIpTp_InternalState.State != SOMEIPTP_STATE_INIT)
   408|    {
   409|        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_DEINIT, SOMEIPTP_E_UNINIT);
   410|        return;
   411|    }
   412|#endif
   413|
   414|    /* Reset all channels */
   415|    for (i = 0U; i < SOMEIPTP_NUMBER_OF_CHANNELS; i++)
   416|    {
   417|        SomeIpTp_ResetChannel(i);
   418|    }
   419|
   420|    /* Clear configuration pointer */
   421|    SomeIpTp_InternalState.ConfigPtr = NULL_PTR;
   422|
   423|    /* Set module state to uninitialized */
   424|    SomeIpTp_InternalState.State = SOMEIPTP_STATE_UNINIT;
   425|}
   426|
   427|/**
   428| * @brief   Gets version information
   429| */
   430|#if (SOMEIPTP_VERSION_INFO_API == STD_ON)
   431|void SomeIpTp_GetVersionInfo(Std_VersionInfoType* versioninfo)
   432|{
   433|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
   434|    if (versioninfo == NULL_PTR)
   435|    {
   436|        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_GETVERSIONINFO, SOMEIPTP_E_PARAM_POINTER);
   437|        return;
   438|    }
   439|#endif
   440|
   441|    versioninfo->vendorID = SOMEIPTP_VENDOR_ID;
   442|    versioninfo->moduleID = SOMEIPTP_MODULE_ID;
   443|    versioninfo->sw_major_version = SOMEIPTP_SW_MAJOR_VERSION;
   444|    versioninfo->sw_minor_version = SOMEIPTP_SW_MINOR_VERSION;
   445|    versioninfo->sw_patch_version = SOMEIPTP_SW_PATCH_VERSION;
   446|}
   447|#endif
   448|
   449|/**
   450| * @brief   Transmits a large PDU using fragmentation
   451| */
   452|Std_ReturnType SomeIpTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr,
   453|                                  const RetryInfoType* RetryInfoPtr,
   454|                                  PduLengthType* TxDataCntPtr)
   455|{
   456|    Std_ReturnType result = E_NOT_OK;
   457|    uint16 channelId;
   458|    SomeIpTp_ChannelType* channelPtr;
   459|    SomeIpTp_TxBufferType* txBufPtr;
   460|
   461|    (void)RetryInfoPtr; /* Not used in this implementation */
   462|    (void)TxDataCntPtr;
   463|
   464|#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
   465|    if (SomeIpTp_InternalState.State != SOMEIPTP_STATE_INIT)
   466|    {
   467|        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_TRANSMIT, SOMEIPTP_E_UNINIT);
   468|        return E_NOT_OK;
   469|    }
   470|
   471|    if (PduInfoPtr == NULL_PTR)
   472|    {
   473|        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_TRANSMIT, SOMEIPTP_E_PARAM_POINTER);
   474|        return E_NOT_OK;
   475|    }
   476|#endif
   477|
   478|    if (SomeIpTp_FindChannelByTxPduId(TxPduId, &channelId) == E_OK)
   479|    {
   480|        channelPtr = &SomeIpTp_InternalState.Channels[channelId];
   481|        txBufPtr = &channelPtr->TxBuffer;
   482|
   483|        if (channelPtr->State == SOMEIPTP_CHANNEL_IDLE)
   484|        {
   485|            /* Setup transmission buffer */
   486|            txBufPtr->Data = PduInfoPtr->SduDataPtr;
   487|            txBufPtr->Length = PduInfoPtr->SduLength;
   488|            txBufPtr->CurrentOffset = 0U;
   489|            txBufPtr->RemainingLength = PduInfoPtr->SduLength;
   490|
   491|            channelPtr->State = SOMEIPTP_CHANNEL_TX_ACTIVE;
   492|
   493|            /* Start transmission */
   494|            result = SomeIpTp_SendNextSegment(channelId);
   495|        }
   496|        else
   497|        {
   498|            result = E_NOT_OK; /* Channel busy */
   499|        }
   500|    }
   501|