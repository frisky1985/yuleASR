/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file FrTp_Rx.c
 * @brief FlexRay Transport Protocol module - Receive logic
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"
#include "FrTp_Private.h"
#include <string.h>
#include "Det.h"
#include "PduR.h"

/*==================================================================================================
*                                    LOCAL DEFINES
==================================================================================================*/

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void FrTp_ProcessSingleFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);
static void FrTp_ProcessFirstFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);
static void FrTp_ProcessConsecutiveFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);
static void FrTp_ProcessFlowControlRx(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define FRTP_START_SEC_CODE
#include "MemMap.h"
#include <string.h>

/** @req SWS_FrTp_00009 */
/**
 * @brief Handles reception indication from FrIf
 * @param RxPduId Received PDU ID
 * @param PduInfoPtr Pointer to PDU info structure
 */
void FrTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    FrTp_ConnectionIdxType connIdx;
    FrTp_PduType pduType;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_Runtime.initialized != TRUE)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_RXINDICATION, FRTP_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_RXINDICATION, FRTP_E_PARAM_POINTER);
        return;
    }

    if (PduInfoPtr->SduDataPtr == NULL_PTR)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_RXINDICATION, FRTP_E_PARAM_POINTER);
        return;
    }
#endif

    /* Find connection by Rx PDU ID */
    connIdx = FrTp_FindConnectionByRxPdu(RxPduId);
    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
        FRTP_DET_REPORT_ERROR(FRTP_SID_RXINDICATION, FRTP_E_INVALID_PDU_SDU_ID);
#endif
        return;
    }

    /* Decode PDU type from first byte */
    pduType = FrTp_DecodePduType(PduInfoPtr->SduDataPtr);

    /* Process based on PDU type */
    switch (pduType)
    {
        case FRTP_PDU_SF:
            FrTp_ProcessSingleFrame(connIdx, PduInfoPtr);
            break;

        case FRTP_PDU_FF:
            FrTp_ProcessFirstFrame(connIdx, PduInfoPtr);
            break;

        case FRTP_PDU_CF:
            FrTp_ProcessConsecutiveFrame(connIdx, PduInfoPtr);
            break;

        case FRTP_PDU_FC:
            FrTp_ProcessFlowControlRx(connIdx, PduInfoPtr);
            break;

        default:
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
            FRTP_DET_REPORT_ERROR(FRTP_SID_RXINDICATION, FRTP_E_INVALID_FRAME_TYPE);
#endif
            break;
    }
}

/** @req SWS_FrTp_00007 */
/**
 * @brief Cancels an ongoing reception
 * @param RxPduId PDU to cancel
 * @return E_OK if cancelled, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_CancelReceive(PduIdType RxPduId)
{
    FrTp_ConnectionIdxType connIdx;
    const FrTp_ConnectionRuntimeType* runtime;
    Std_ReturnType result = E_NOT_OK;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_Runtime.initialized != TRUE)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_CANCELRECEIVE, FRTP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Find connection by Rx PDU ID */
    connIdx = FrTp_FindConnectionByRxPdu(RxPduId);
    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        runtime = FrTp_GetConnectionRuntime(connIdx);

        if ((runtime != NULL_PTR) && (runtime->state != FRTP_STATE_IDLE))
        {
            /* Check if this is an RX connection */
            if ((runtime->flags & FRTP_FLAG_RX_ACTIVE) != 0U)
            {
                /* Release buffer and reset connection */
                FrTp_ReleaseRxBuffer(connIdx);
                FrTp_ResetConnection(connIdx);
                result = E_OK;
            }
        }
    }

    return result;
}

/**
 * @brief Receive state machine main function
 * @param connIdx Connection index
 */
void FrTp_RxStateMachine(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
        return;
    }

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return;
    }

    /* Process based on current state */
    switch (runtime->state)
    {
        case FRTP_STATE_RX_WAIT_FF:
            /* Waiting for FF - nothing to do here, processing is in RxIndication */
            break;

        case FRTP_STATE_RX_WAIT_CF:
            /* Waiting for CF - check for timeout */
            if ((FrTp_IsTimerExpired(runtime)) != 0U)
            {
                /* N_Cr timeout - release buffer and reset */
                FrTp_ReleaseRxBuffer(connIdx);
                FrTp_ResetConnection(connIdx);
            }
            break;

        case FRTP_STATE_RX_SEND_FC:
            /* Send FC if needed */
            {
                const FrTp_ConnectionConfigType* config;
                
                config = FrTp_GetConnectionConfig(connIdx);
                if (config != NULL_PTR)
                {
                    /* Send CTS FC */
                    FrTp_SendFlowControl(connIdx, FRTP_FC_STATUS_CTS);
                    
                    /* Transition to wait CF state */
                    runtime->state = FRTP_STATE_RX_WAIT_CF;
                    FrTp_StartTimer(runtime, config->timeoutCr);
                }
            }
            break;

        default:
            /* Invalid state for RX */
            break;
    }
}

/**
 * @brief Sends a flow control frame
 * @param connIdx Connection index
 * @param flowStatus Flow status (CTS, WAIT, OVFLW)
 */
void FrTp_SendFlowControl(FrTp_ConnectionIdxType connIdx, uint8 flowStatus)
{
    uint8 txBuffer[FRTP_MAX_PAYLOAD_PER_FRAME];
    PduInfoType txPduInfo;
    uint8 pciLength;
    const FrTp_ConnectionConfigType* config;
    FrTp_ConnectionRuntimeType* runtime;
    uint8 blockSize;
    uint8 stMin;

    config = FrTp_GetConnectionConfig(connIdx);
    runtime = FrTp_GetConnectionRuntime(connIdx);

    if ((config == NULL_PTR) || (runtime == NULL_PTR))
    {
        return;
    }

    /* Determine BS and STmin based on flow status */
    if (flowStatus == FRTP_FC_STATUS_CTS)
    {
        blockSize = runtime->blockSize;
        stMin = runtime->stMin;
    }
    else
    {
        blockSize = 0U;
        stMin = 0U;
    }

    /* Encode FC PCI */
    (void)FrTp_EncodeFcPdu(txBuffer, flowStatus, blockSize, stMin, &pciLength);

    /* Setup TX PDU info */
/*     txPduInfo.SduDataPtr = txBuffer; */
/*     txPduInfo.SduLength = pciLength; */
/*     txPduInfo.MetaDataPtr = NULL_PTR; */

    /* Send via FrIf (placeholder - actual call depends on FrIf API) */
    /* FrIf_Transmit(config->txPduId, &txPduInfo); */
    (void)config;
    (void)runtime;
}

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Processes a single frame
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 */
static void FrTp_ProcessSingleFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    FrTp_ConnectionRuntimeType* runtime;
    uint8 sfLength;
    BufReq_ReturnType bufStatus;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return;
    }

    /* Decode SF PCI */
    if (FrTp_DecodeSfPdu(pduInfo->SduDataPtr, &sfLength) != E_OK)
    {
        return;
    }

    /* Check if connection is busy with another reception */
    if ((runtime->state != FRTP_STATE_IDLE) && ((runtime->flags & FRTP_FLAG_RX_ACTIVE) != 0U))
    {
        /* Connection busy - could send WAIT FC or ignore */
        return;
    }

    /* Request RX buffer from PduR */
    bufStatus = FrTp_RequestRxBuffer(connIdx, sfLength);
    if (bufStatus != BUFREQ_E_OK)
    {
        /* Buffer not available - SF cannot be received */
        return;
    }

    /* Copy SF data to buffer */
    if ((runtime->rxBuffer != NULL_PTR) && (pduInfo->SduDataPtr != NULL_PTR))
    {
        PduLengthType dataLength = sfLength;
        if (dataLength > (pduInfo->SduLength - 1U))
        {
            dataLength = pduInfo->SduLength - 1U;
        }

        (void)memcpy(runtime->rxBuffer, &pduInfo->SduDataPtr[1], dataLength);
        runtime->bytesTransferred = dataLength;
    }

    /* Notify PduR of complete reception */
    PduR_FrTpRxIndication(connIdx, E_OK);

    /* Release buffer and reset connection */
    FrTp_ReleaseRxBuffer(connIdx);
    FrTp_ResetConnection(connIdx);
}

/**
 * @brief Processes a first frame
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 */
static void FrTp_ProcessFirstFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    FrTp_ConnectionRuntimeType* runtime;
    const FrTp_ConnectionConfigType* config;
    uint16 ffLength;
    BufReq_ReturnType bufStatus;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    config = FrTp_GetConnectionConfig(connIdx);

    if ((runtime == NULL_PTR) || (config == NULL_PTR))
    {
        return;
    }

    /* Decode FF PCI */
    if (FrTp_DecodeFfPdu(pduInfo->SduDataPtr, &ffLength) != E_OK)
    {
        return;
    }

    /* Check if connection is idle */
    if (runtime->state != FRTP_STATE_IDLE)
    {
        /* Connection busy - send OVFLW or ignore */
        FrTp_SendFlowControl(connIdx, FRTP_FC_STATUS_OVFLW);
        return;
    }

    /* Request RX buffer from PduR */
    bufStatus = FrTp_RequestRxBuffer(connIdx, ffLength);
    if (bufStatus == BUFREQ_E_OVFL)
    {
        /* Buffer overflow - send OVFLW FC */
        FrTp_SendFlowControl(connIdx, FRTP_FC_STATUS_OVFLW);
        return;
    }
    else if (bufStatus != BUFREQ_E_OK)
    {
        /* Buffer not available - send WAIT FC */
        FrTp_SendFlowControl(connIdx, FRTP_FC_STATUS_WAIT);
        
        /* Setup to wait for buffer */
        runtime->state = FRTP_STATE_RX_SEND_FC;
        runtime->dataLength = ffLength;
        runtime->flags = FRTP_FLAG_RX_ACTIVE;
        FrTp_StartTimer(runtime, config->timeoutBr);
        return;
    }

    /* Buffer available - setup reception */
    runtime->state = FRTP_STATE_RX_SEND_FC;
    runtime->dataLength = ffLength;
    runtime->bytesTransferred = 0U;
    runtime->bytesRemaining = ffLength;
    runtime->sequenceNumber = 0U;
    runtime->blockSize = config->defaultBlockSize;
    runtime->stMin = config->defaultSTmin;
    runtime->flags = FRTP_FLAG_RX_ACTIVE;
    runtime->retryCount = 0U;

    /* Copy FF data to buffer */
    if ((runtime->rxBuffer != NULL_PTR) && (pduInfo->SduDataPtr != NULL_PTR))
    {
        PduLengthType ffDataLength = pduInfo->SduLength - 2U;  /* 2 bytes PCI */
        PduLengthType bufferAvailable = runtime->rxBufferSize;

        if (ffDataLength > bufferAvailable)
        {
            ffDataLength = bufferAvailable;
        }

        (void)memcpy(runtime->rxBuffer, &pduInfo->SduDataPtr[2], ffDataLength);
        runtime->bytesTransferred = ffDataLength;
        runtime->bytesRemaining -= ffDataLength;
    }

    /* Send CTS FC to start CF reception */
    FrTp_SendFlowControl(connIdx, FRTP_FC_STATUS_CTS);

    /* Transition to wait CF state */
    runtime->state = FRTP_STATE_RX_WAIT_CF;
    FrTp_StartTimer(runtime, config->timeoutCr);
}

/**
 * @brief Processes a consecutive frame
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 */
static void FrTp_ProcessConsecutiveFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    FrTp_ConnectionRuntimeType* runtime;
    const FrTp_ConnectionConfigType* config;
    uint8 seqNum;
    PduLengthType cfDataLength;
    PduLengthType dataOffset;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    config = FrTp_GetConnectionConfig(connIdx);

    if ((runtime == NULL_PTR) || (config == NULL_PTR))
    {
        return;
    }

    /* Check if we are waiting for CF */
    if ((runtime->state != FRTP_STATE_RX_WAIT_CF) || ((runtime->flags & FRTP_FLAG_RX_ACTIVE) == 0U))
    {
        /* Unexpected CF - ignore */
        return;
    }

    /* Decode CF PCI */
    if (FrTp_DecodeCfPdu(pduInfo->SduDataPtr, &seqNum) != E_OK)
    {
        return;
    }

    /* Check sequence number */
    if (!FrTp_IsValidSeq(seqNum, runtime->sequenceNumber))
    {
        /* Sequence error - abort reception */
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
        FRTP_DET_REPORT_ERROR(FRTP_SID_RXINDICATION, FRTP_E_INVALID_SEQUENCE_NUMBER);
#endif
        PduR_FrTpRxIndication(connIdx, E_NOT_OK);
        FrTp_ReleaseRxBuffer(connIdx);
        FrTp_ResetConnection(connIdx);
        return;
    }

    /* Reset N_Cr timer */
    FrTp_StartTimer(runtime, config->timeoutCr);

    /* Copy CF data to buffer */
    if ((runtime->rxBuffer != NULL_PTR) && (pduInfo->SduDataPtr != NULL_PTR))
    {
        cfDataLength = pduInfo->SduLength - 1U;  /* 1 byte PCI */
        dataOffset = runtime->bytesTransferred;

        /* Check remaining buffer space */
        if (cfDataLength > runtime->bytesRemaining)
        {
            cfDataLength = runtime->bytesRemaining;
        }

        if ((dataOffset + cfDataLength) <= runtime->rxBufferSize)
        {
            (void)memcpy(&runtime->rxBuffer[dataOffset], &pduInfo->SduDataPtr[1], cfDataLength);
            runtime->bytesTransferred += cfDataLength;
            runtime->bytesRemaining -= cfDataLength;
        }
    }

    /* Increment sequence number */
    runtime->sequenceNumber = FrTp_IncSeq(runtime->sequenceNumber);

    /* Check if reception complete */
    if (runtime->bytesRemaining == 0U)
    {
        /* All data received - notify PduR */
        PduR_FrTpRxIndication(connIdx, E_OK);
        
        /* Release buffer and reset connection */
        FrTp_ReleaseRxBuffer(connIdx);
        FrTp_ResetConnection(connIdx);
    }
    else
    {
        /* Check if we need to send another FC (block size) */
        uint8 cfsInBlock = (uint8)(runtime->bytesTransferred / (config->maxPayload - 1U));
        
        if ((runtime->blockSize > 0U) && (cfsInBlock >= runtime->blockSize))
        {
            /* Send next FC */
            FrTp_SendFlowControl(connIdx, FRTP_FC_STATUS_CTS);
        }
    }
}

/**
 * @brief Processes a flow control frame (for RX side during segmented TX)
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 */
static void FrTp_ProcessFlowControlRx(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    uint8 flowStatus;
    uint8 blockSize;
    uint8 stMin;

    /* Decode FC PDU */
    if (FrTp_DecodeFcPdu(pduInfo->SduDataPtr, &flowStatus, &blockSize, &stMin) != E_OK)
    {
        return;
    }

    /* Process FC - this is used by TX state machine to control CF sending */
    FrTp_ProcessFlowControl(connIdx, flowStatus, blockSize, stMin);
}

/*==================================================================================================
*                                    BUFFER MANAGEMENT FUNCTIONS
==================================================================================================*/

/**
 * @brief Requests TX buffer from PduR
 * @param connIdx Connection index
 * @param len Requested length
 * @return Buffer request status
 */
BufReq_ReturnType FrTp_RequestTxBuffer(FrTp_ConnectionIdxType connIdx, PduLengthType len)
{
    /* Placeholder - actual implementation depends on PduR API */
    (void)connIdx;
    (void)len;
    return BUFREQ_E_OK;
}

/**
 * @brief Requests RX buffer from PduR
 * @param connIdx Connection index
 * @param len Requested length
 * @return Buffer request status
 */
BufReq_ReturnType FrTp_RequestRxBuffer(FrTp_ConnectionIdxType connIdx, PduLengthType len)
{
    FrTp_ConnectionRuntimeType* runtime;
    PduInfoType pduInfo;
    PduLengthType bufferSize;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return BUFREQ_E_NOT_OK;
    }

    /* Request buffer from PduR */
    pduInfo.SduLength = len;
    pduInfo.SduDataPtr = NULL_PTR;

    /* Call PduR_StartOfReception */
    /* bufStatus = PduR_FrTpStartOfReception(connIdx, &pduInfo, len, &bufferSize); */
    
    /* Placeholder - simulate buffer allocation */
    bufferSize = len;
    runtime->rxBufferSize = bufferSize;
    runtime->rxBufferLocked = TRUE;

    /* Allocate temporary buffer (in real implementation, this comes from PduR) */
    /* runtime->rxBuffer = PduR_AllocateBuffer(...); */
    
    (void)pduInfo;
    return BUFREQ_E_OK;
}

/**
 * @brief Releases TX buffer
 * @param connIdx Connection index
 */
void FrTp_ReleaseTxBuffer(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime != NULL_PTR)
    {
        runtime->txPduInfo = NULL_PTR;
    }
}

/**
 * @brief Releases RX buffer
 * @param connIdx Connection index
 */
void FrTp_ReleaseRxBuffer(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime != NULL_PTR)
    {
        /* Call PduR_FrTpRxIndication if reception complete or error */
        /* PduR_FrTpRxIndication(connIdx, status); */
        
        runtime->rxBuffer = NULL_PTR;
        runtime->rxBufferSize = 0U;
        runtime->rxBufferLocked = FALSE;
    }
}

#define FRTP_STOP_SEC_CODE
#include "MemMap.h"
