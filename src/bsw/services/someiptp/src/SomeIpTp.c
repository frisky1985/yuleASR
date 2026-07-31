/**
 * @file SomeIpTp.c
 * @brief SOME/IP Transport Protocol
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * AUTOSAR Classic Platform - BSW Module
 * This file is part of the yuleASR AUTOSAR implementation.
 */

/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : Ethernet
* Dependencies         : SoAd, Det
*
* SW Version           : 4.7.0
* Build Version        : YULETECH_AUTOSAR_4.7.0
* Build Date           : 2026-04-29
* Author               : AI Agent (SomeIpTp Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "SomeIpTp.h"
#include "SomeIpTp_Cfg.h"
#include "Det.h"
#include "MemMap.h"
#include <string.h>

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define SOMEIPTP_STATE_UNINIT                   (0x00U)
#define SOMEIPTP_STATE_INIT                     (0x01U)

/* TP Header field offsets */
#define SOMEIPTP_HDR_OFFSET_FLAGS               (0U)
#define SOMEIPTP_HDR_SIZE                       (4U)

/*==================================================================================================
*                                  LOCAL MACRO DEFINITIONS
==================================================================================================*/
#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
    #define SOMEIPTP_DET_REPORT_ERROR(ApiId, ErrorId) \
        Det_ReportError(SOMEIPTP_MODULE_ID, SOMEIPTP_INSTANCE_ID, (ApiId), (ErrorId))
#else
    #define SOMEIPTP_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

#define SOMEIPTP_IS_VALID_CHANNEL_ID(Id) \
    (((Id) < SOMEIPTP_NUMBER_OF_CHANNELS) ? TRUE : FALSE)

#define SOMEIPTP_SET_TP_OFFSET(buffer, offset, moreSeg) \
    do { \
        uint32 value = ((moreSeg) ? 0x40000000UL : 0x00UL) | ((offset) & 0x3FFFFFFFUL); \
        (buffer)[0] = (uint8)((value) >> 24); \
        (buffer)[1] = (uint8)((value) >> 16); \
        (buffer)[2] = (uint8)((value) >> 8); \
        (buffer)[3] = (uint8)(value); \
    } while(0)

#define SOMEIPTP_GET_TP_OFFSET(buffer, offset, moreSeg) \
    do { \
        uint32 value = (((uint32)(buffer)[0]) << 24) | \
                       (((uint32)(buffer)[1]) << 16) | \
                       (((uint32)(buffer)[2]) << 8) | \
                       ((uint32)(buffer)[3]); \
        (offset) = (value) & 0x3FFFFFFFUL; \
        (moreSeg) = ((value) & 0x40000000UL) ? TRUE : FALSE; \
    } while(0)

/*==================================================================================================
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
typedef struct {
    uint8 State;
    const SomeIpTp_ConfigType* ConfigPtr;
    SomeIpTp_ChannelType Channels[SOMEIPTP_NUMBER_OF_CHANNELS];
    uint8 RxBufferPool[SOMEIPTP_NUMBER_OF_CHANNELS][SOMEIPTP_RX_BUFFER_SIZE];
} SomeIpTp_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define SOMEIPTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC SomeIpTp_InternalStateType SomeIpTp_InternalState;

#define SOMEIPTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType SomeIpTp_FindChannelByTxPduId(PduIdType TxPduId, uint16* ChannelIdPtr);
STATIC Std_ReturnType SomeIpTp_FindChannelByRxPduId(PduIdType RxPduId, uint16* ChannelIdPtr);
STATIC Std_ReturnType SomeIpTp_SendNextSegment(uint16 ChannelId);
STATIC void SomeIpTp_ProcessReassembly(uint16 ChannelId, const PduInfoType* PduInfoPtr);
STATIC void SomeIpTp_UpdateTimeouts(void);
STATIC void SomeIpTp_ResetChannel(uint16 ChannelId);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define SOMEIPTP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find channel ID by TX PDU ID
 */
STATIC Std_ReturnType SomeIpTp_FindChannelByTxPduId(PduIdType TxPduId, uint16* ChannelIdPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 i;
    const SomeIpTp_ConfigType* configPtr = SomeIpTp_InternalState.ConfigPtr;

    if (configPtr != NULL_PTR)
    {
        for (i = 0U; i < configPtr->NumChannels; i++)
        {
            if (configPtr->ChannelConfigs[i].TxPduId == TxPduId)
            {
                *ChannelIdPtr = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find channel ID by RX PDU ID
 */
STATIC Std_ReturnType SomeIpTp_FindChannelByRxPduId(PduIdType RxPduId, uint16* ChannelIdPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 i;
    const SomeIpTp_ConfigType* configPtr = SomeIpTp_InternalState.ConfigPtr;

    if (configPtr != NULL_PTR)
    {
        for (i = 0U; i < configPtr->NumChannels; i++)
        {
            if (configPtr->ChannelConfigs[i].RxPduId == RxPduId)
            {
                *ChannelIdPtr = i;
                result = E_OK;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Send next segment
 */
STATIC Std_ReturnType SomeIpTp_SendNextSegment(uint16 ChannelId)
{
    Std_ReturnType result = E_NOT_OK;
    SomeIpTp_ChannelType* channelPtr;
    SomeIpTp_TxBufferType* txBufPtr;
    const SomeIpTp_ChannelConfigType* configPtr;
    uint8 segmentData[SOMEIPTP_MAX_SEGMENT_SIZE + SOMEIPTP_HDR_SIZE];
    uint32 remainingLen;
    uint16 segLen;
    boolean moreSeg;
    PduInfoType pduInfo;

    if (ChannelId < SOMEIPTP_NUMBER_OF_CHANNELS)
    {
        channelPtr = &SomeIpTp_InternalState.Channels[ChannelId];
        txBufPtr = &channelPtr->TxBuffer;
        configPtr = &SomeIpTp_InternalState.ConfigPtr->ChannelConfigs[ChannelId];

        if (channelPtr->State == SOMEIPTP_CHANNEL_TX_ACTIVE)
        {
            remainingLen = txBufPtr->RemainingLength;

            if (remainingLen > 0U)
            {
                /* Calculate segment size */
                segLen = (remainingLen > configPtr->MaxSegmentSize) ? 
                         configPtr->MaxSegmentSize : (uint16)remainingLen;
                moreSeg = (remainingLen > configPtr->MaxSegmentSize);

                /* Build TP header */
                SOMEIPTP_SET_TP_OFFSET(segmentData, txBufPtr->CurrentOffset, moreSeg);

                /* Copy segment data */
                (void)memcpy(&segmentData[SOMEIPTP_HDR_SIZE], 
                            &txBufPtr->Data[txBufPtr->CurrentOffset], segLen);

                /* Send segment via SoAd */
                pduInfo.SduDataPtr = segmentData;
                pduInfo.SduLength = segLen + SOMEIPTP_HDR_SIZE;
                pduInfo.MetaDataPtr = NULL_PTR;

                /* Call SoAd_Transmit */
                extern Std_ReturnType SoAd_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
                result = SoAd_Transmit(configPtr->TxPduId, &pduInfo);

                if (result == E_OK)
                {
                    /* Update transmission state */
                    txBufPtr->CurrentOffset += segLen;
                    txBufPtr->RemainingLength -= segLen;
                    channelPtr->State = SOMEIPTP_CHANNEL_TX_WAIT_CONFIRM;
                    channelPtr->TimeoutCounter = configPtr->TxTimeout / 
                                                 SOMEIPTP_MAIN_FUNCTION_PERIOD_MS;
                    channelPtr->RetryCount = 0U;
                }
            }
            else
            {
                /* Transmission complete */
                channelPtr->State = SOMEIPTP_CHANNEL_IDLE;
                result = E_OK;
            }
        }
    }

    return result;
}

/**
 * @brief   Process received segment for reassembly
 */
STATIC void SomeIpTp_ProcessReassembly(uint16 ChannelId, const PduInfoType* PduInfoPtr)
{
    SomeIpTp_ChannelType* channelPtr;
    SomeIpTp_RxBufferType* rxBufPtr;
    const SomeIpTp_ChannelConfigType* configPtr;
    uint32 offset;
    boolean moreSeg;
    uint16 payloadLen;

    if ((ChannelId < SOMEIPTP_NUMBER_OF_CHANNELS) && (PduInfoPtr != NULL_PTR))
    {
        channelPtr = &SomeIpTp_InternalState.Channels[ChannelId];
        rxBufPtr = &channelPtr->RxBuffer;
        configPtr = &SomeIpTp_InternalState.ConfigPtr->ChannelConfigs[ChannelId];

        if (PduInfoPtr->SduLength >= SOMEIPTP_HDR_SIZE)
        {
            /* Parse TP header */
            SOMEIPTP_GET_TP_OFFSET(PduInfoPtr->SduDataPtr, offset, moreSeg);
            payloadLen = PduInfoPtr->SduLength - SOMEIPTP_HDR_SIZE;

            /* Check if this is a new session or continuation */
            if (offset == 0U)
            {
                /* New session - reset buffer */
                rxBufPtr->Length = 0U;
                rxBufPtr->NextOffset = 0U;
                rxBufPtr->IsComplete = FALSE;
                channelPtr->State = SOMEIPTP_CHANNEL_RX_ACTIVE;
            }

            /* Validate offset */
            if (offset == rxBufPtr->NextOffset)
            {
                /* Copy segment data to reassembly buffer */
                if ((rxBufPtr->Length + payloadLen) <= rxBufPtr->MaxLength)
                {
                    (void)memcpy(&rxBufPtr->Data[rxBufPtr->Length], 
                                &PduInfoPtr->SduDataPtr[SOMEIPTP_HDR_SIZE], payloadLen);
                    rxBufPtr->Length += payloadLen;
                    rxBufPtr->NextOffset += payloadLen;
                    rxBufPtr->MoreSegmentsExpected = moreSeg;

                    /* Update timeout */
                    channelPtr->TimeoutCounter = configPtr->RxTimeout / 
                                                 SOMEIPTP_MAIN_FUNCTION_PERIOD_MS;

                    /* Check if complete */
                    if (!moreSeg)
                    {
                        rxBufPtr->IsComplete = TRUE;
                        channelPtr->State = SOMEIPTP_CHANNEL_RX_COMPLETED;
                        
                        /* Notify upper layer (SomeIpXf) via callback */
                        extern void SomeIpXf_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
                        PduInfoType completePdu;
                        completePdu.SduDataPtr = rxBufPtr->Data;
                        completePdu.SduLength = (PduLengthType)rxBufPtr->Length;
                        completePdu.MetaDataPtr = NULL_PTR;
                        SomeIpXf_RxIndication(configPtr->RxPduId, &completePdu);
                    }
                }
            }
            else
            {
                /* Out of sequence segment - reset */
                SomeIpTp_ResetChannel(ChannelId);
#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
                SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_RXINDICATION, SOMEIPTP_E_REASSEMBLY_ERROR);
#endif
            }
        }
    }
}

/**
 * @brief   Update channel timeouts
 */
STATIC void SomeIpTp_UpdateTimeouts(void)
{
    uint16 i;
    SomeIpTp_ChannelType* channelPtr;

    for (i = 0U; i < SOMEIPTP_NUMBER_OF_CHANNELS; i++)
    {
        channelPtr = &SomeIpTp_InternalState.Channels[i];

        if ((channelPtr->State == SOMEIPTP_CHANNEL_TX_WAIT_CONFIRM) ||
            (channelPtr->State == SOMEIPTP_CHANNEL_RX_ACTIVE))
        {
            if (channelPtr->TimeoutCounter > 0U)
            {
                channelPtr->TimeoutCounter--;
                
                if (channelPtr->TimeoutCounter == 0U)
                {
                    /* Timeout occurred */
#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
                    SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_MAINFUNCTION, SOMEIPTP_E_TIMEOUT);
#endif
                    SomeIpTp_ResetChannel(i);
                }
            }
        }
    }
}

/**
 * @brief   Reset channel to idle state
 */
STATIC void SomeIpTp_ResetChannel(uint16 ChannelId)
{
    if (ChannelId < SOMEIPTP_NUMBER_OF_CHANNELS)
    {
        SomeIpTp_ChannelType* channelPtr = &SomeIpTp_InternalState.Channels[ChannelId];
        
        channelPtr->State = SOMEIPTP_CHANNEL_IDLE;
        channelPtr->TimeoutCounter = 0U;
        channelPtr->RetryCount = 0U;
        channelPtr->RxBuffer.Length = 0U;
        channelPtr->RxBuffer.NextOffset = 0U;
        channelPtr->RxBuffer.IsComplete = FALSE;
        channelPtr->RxBuffer.MoreSegmentsExpected = FALSE;
        channelPtr->TxBuffer.CurrentOffset = 0U;
        channelPtr->TxBuffer.RemainingLength = 0U;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the SOME/IP TP module
 */
void SomeIpTp_Init(const SomeIpTp_ConfigType* ConfigPtr)
{
    uint16 i;

#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpTp_InternalState.State == SOMEIPTP_STATE_INIT)
    {
        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_INIT, SOMEIPTP_E_ALREADY_INITIALIZED);
        return;
    }

    if (ConfigPtr == NULL_PTR)
    {
        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_INIT, SOMEIPTP_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    SomeIpTp_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize channels */
    for (i = 0U; i < SOMEIPTP_NUMBER_OF_CHANNELS; i++)
    {
        SomeIpTp_ResetChannel(i);
        
        /* Setup RX buffer */
        SomeIpTp_InternalState.Channels[i].RxBuffer.Data = 
            SomeIpTp_InternalState.RxBufferPool[i];
        SomeIpTp_InternalState.Channels[i].RxBuffer.MaxLength = SOMEIPTP_RX_BUFFER_SIZE;
    }

    /* Set module state to initialized */
    SomeIpTp_InternalState.State = SOMEIPTP_STATE_INIT;
}

/**
 * @brief   Deinitializes the SOME/IP TP module
 */
void SomeIpTp_DeInit(void)
{
    uint16 i;

#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpTp_InternalState.State != SOMEIPTP_STATE_INIT)
    {
        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_DEINIT, SOMEIPTP_E_UNINIT);
        return;
    }
#endif

    /* Reset all channels */
    for (i = 0U; i < SOMEIPTP_NUMBER_OF_CHANNELS; i++)
    {
        SomeIpTp_ResetChannel(i);
    }

    /* Clear configuration pointer */
    SomeIpTp_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    SomeIpTp_InternalState.State = SOMEIPTP_STATE_UNINIT;
}

/**
 * @brief   Gets version information
 */
#if (SOMEIPTP_VERSION_INFO_API == STD_ON)
void SomeIpTp_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_GETVERSIONINFO, SOMEIPTP_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = SOMEIPTP_VENDOR_ID;
    versioninfo->moduleID = SOMEIPTP_MODULE_ID;
    versioninfo->sw_major_version = SOMEIPTP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = SOMEIPTP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = SOMEIPTP_SW_PATCH_VERSION;
}
#endif

/**
 * @brief   Transmits a large PDU using fragmentation
 */
Std_ReturnType SomeIpTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr,
                                  const RetryInfoType* RetryInfoPtr,
                                  PduLengthType* TxDataCntPtr)
{
    Std_ReturnType result = E_NOT_OK;
    uint16 channelId;
    SomeIpTp_ChannelType* channelPtr;
    SomeIpTp_TxBufferType* txBufPtr;

    (void)RetryInfoPtr; /* Not used in this implementation */
    (void)TxDataCntPtr;

#if (SOMEIPTP_DEV_ERROR_DETECT == STD_ON)
    if (SomeIpTp_InternalState.State != SOMEIPTP_STATE_INIT)
    {
        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_TRANSMIT, SOMEIPTP_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        SOMEIPTP_DET_REPORT_ERROR(SOMEIPTP_SID_TRANSMIT, SOMEIPTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (SomeIpTp_FindChannelByTxPduId(TxPduId, &channelId) == E_OK)
    {
        channelPtr = &SomeIpTp_InternalState.Channels[channelId];
        txBufPtr = &channelPtr->TxBuffer;

        if (channelPtr->State == SOMEIPTP_CHANNEL_IDLE)
        {
            /* Setup transmission buffer */
            txBufPtr->Data = PduInfoPtr->SduDataPtr;
            txBufPtr->Length = PduInfoPtr->SduLength;
            txBufPtr->CurrentOffset = 0U;
            txBufPtr->RemainingLength = PduInfoPtr->SduLength;

            channelPtr->State = SOMEIPTP_CHANNEL_TX_ACTIVE;

            /* Start transmission */
            result = SomeIpTp_SendNextSegment(channelId);
        }
        else
        {
            result = E_NOT_OK; /* Channel busy */
        }
    }

    return result;
}

#define SOMEIPTP_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
