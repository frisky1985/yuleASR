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
 * @file FrTp_Tx.c
 * @brief FlexRay Transport Protocol module - Transmit logic
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
static Std_ReturnType FrTp_SendSingleFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);
static Std_ReturnType FrTp_SendFirstFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);
static Std_ReturnType FrTp_SendConsecutiveFrame(FrTp_ConnectionIdxType connIdx);
static Std_ReturnType FrTp_SendFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo);

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define FRTP_START_SEC_CODE
#include "MemMap.h"
#include <string.h>

/**
 * @brief Requests transmission of data
 * @param TxPduId PDU to transmit
 * @param PduInfoPtr Pointer to PDU info structure
 * @return E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    FrTp_ConnectionIdxType connIdx;
    FrTp_ConnectionRuntimeType* runtime;
    const FrTp_ConnectionConfigType* config;
    Std_ReturnType result = E_NOT_OK;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_Runtime.initialized != TRUE)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_TRANSMIT, FRTP_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_TRANSMIT, FRTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find connection by Tx PDU ID */
    connIdx = FrTp_FindConnectionByTxPdu(TxPduId);
    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
        FRTP_DET_REPORT_ERROR(FRTP_SID_TRANSMIT, FRTP_E_INVALID_PDU_SDU_ID);
#endif
        return E_NOT_OK;
    }

    runtime = FrTp_GetConnectionRuntime(connIdx);
    config = FrTp_GetConnectionConfig(connIdx);

    if ((runtime == NULL_PTR) || (config == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Check if connection is already busy */
    if (runtime->state != FRTP_STATE_IDLE)
    {
        return E_NOT_OK;
    }

    /* Store PDU info and initialize transmission parameters */
    runtime->txPduInfo = (PduInfoType*)PduInfoPtr;
    runtime->dataLength = PduInfoPtr->SduLength;
    runtime->bytesTransferred = 0U;
    runtime->bytesRemaining = PduInfoPtr->SduLength;
    runtime->sequenceNumber = 0U;
    runtime->blockSize = config->defaultBlockSize;
    runtime->stMin = config->defaultSTmin;
    runtime->retryCount = 0U;
    runtime->flags = FRTP_FLAG_TX_ACTIVE;

    /* Determine if single frame or multi-frame transmission */
    if (PduInfoPtr->SduLength <= FRTP_SF_MAX_DATA_LENGTH)
    {
        /* Single frame transmission */
        result = FrTp_SendSingleFrame(connIdx, PduInfoPtr);
        if (result == E_OK)
        {
            runtime->state = FRTP_STATE_TX_WAIT_CONFIRM;
            FrTp_StartTimer(runtime, config->timeoutAs);
        }
    }
    else
    {
        /* Multi-frame transmission */
        result = FrTp_SendFirstFrame(connIdx, PduInfoPtr);
        if (result == E_OK)
        {
            runtime->state = FRTP_STATE_TX_WAIT_FC;
            FrTp_StartTimer(runtime, config->timeoutBs);
        }
    }

    if (result != E_OK)
    {
        /* Transmission failed - reset connection */
        FrTp_ResetConnection(connIdx);
    }

    return result;
}

/**
 * @brief Cancels an ongoing transmission
 * @param TxPduId PDU to cancel
 * @return E_OK if cancelled, E_NOT_OK otherwise
 */
Std_ReturnType FrTp_CancelTransmit(PduIdType TxPduId)
{
    FrTp_ConnectionIdxType connIdx;
    const FrTp_ConnectionRuntimeType* runtime;
    Std_ReturnType result = E_NOT_OK;

#if (FRTP_DEV_ERROR_DETECT == STD_ON)
    if (FrTp_Runtime.initialized != TRUE)
    {
        FRTP_DET_REPORT_ERROR(FRTP_SID_CANCELTRANSMIT, FRTP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Find connection by Tx PDU ID */
    connIdx = FrTp_FindConnectionByTxPdu(TxPduId);
    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        runtime = FrTp_GetConnectionRuntime(connIdx);

        if ((runtime != NULL_PTR) && (runtime->state != FRTP_STATE_IDLE))
        {
            /* Check if this is a TX connection */
            if ((runtime->flags & FRTP_FLAG_TX_ACTIVE) != 0U)
            {
                /* Reset connection and notify PduR */
                PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
                FrTp_ResetConnection(connIdx);
                result = E_OK;
            }
        }
    }

    return result;
}

/**
 * @brief Handles transmission confirmation from FrIf
 * @param TxPduId Transmitted PDU ID
 * @param result Result of transmission
 */
void FrTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    FrTp_ConnectionIdxType connIdx;
    FrTp_ConnectionRuntimeType* runtime;

    /* Find connection by Tx PDU ID */
    connIdx = FrTp_FindConnectionByTxPdu(TxPduId);
    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
        FRTP_DET_REPORT_ERROR(FRTP_SID_TXCONFIRMATION, FRTP_E_INVALID_PDU_SDU_ID);
#endif
        return;
    }

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return;
    }

    /* Stop timer */
    FrTp_StopTimer(runtime);

    if (result != E_OK)
    {
        /* Transmission failed */
        runtime->retryCount++;
        
        if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
        {
            /* Max retries reached - notify PduR and reset */
            PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
            FrTp_ResetConnection(connIdx);
        }
        /* Otherwise, retry will be handled by state machine */
        return;
    }

    /* Transmission successful - process based on state */
    switch (runtime->state)
    {
        case FRTP_STATE_TX_WAIT_CONFIRM:
            /* SF or last CF sent successfully */
            if (runtime->bytesRemaining == 0U)
            {
                /* All data transmitted */
                PduR_FrTpTxConfirmation(connIdx, E_OK);
                FrTp_ResetConnection(connIdx);
            }
            else if (runtime->bytesRemaining > 0U)
            {
                /* More CFs to send - continue in state machine */
                runtime->state = FRTP_STATE_TX_SENDING_CF;
            }
            break;

        case FRTP_STATE_TX_SENDING_CF:
            /* CF sent - continue sending next CF */
            if (runtime->bytesRemaining == 0U)
            {
                /* All data transmitted */
                PduR_FrTpTxConfirmation(connIdx, E_OK);
                FrTp_ResetConnection(connIdx);
            }
            break;

        default:
            /* Unexpected confirmation in other states */
            break;
    }
}

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Sends a single frame
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 * @return E_OK if sent, E_NOT_OK otherwise
 */
static Std_ReturnType FrTp_SendSingleFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    uint8 txBuffer[FRTP_MAX_PAYLOAD_PER_FRAME];
    PduInfoType txPduInfo;
    uint8 pciLength;
    const FrTp_ConnectionConfigType* config;
    Std_ReturnType result;

    config = FrTp_GetConnectionConfig(connIdx);
    if (config == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Encode SF PCI */
    result = FrTp_EncodeSfPdu(txBuffer, pduInfo, &pciLength);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Copy data after PCI */
    if (pduInfo->SduDataPtr != NULL_PTR)
    {
        (void)memcpy(&txBuffer[pciLength], pduInfo->SduDataPtr, pduInfo->SduLength);
    }

    /* Setup TX PDU info */
    txPduInfo.SduDataPtr = txBuffer;
    txPduInfo.SduLength = pciLength + pduInfo->SduLength;
    txPduInfo.MetaDataPtr = NULL_PTR;

    /* Send via FrIf */
    return FrTp_SendFrame(connIdx, &txPduInfo);
}

/**
 * @brief Sends a first frame (FF)
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 * @return E_OK if sent, E_NOT_OK otherwise
 */
static Std_ReturnType FrTp_SendFirstFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    uint8 txBuffer[FRTP_MAX_PAYLOAD_PER_FRAME];
    PduInfoType txPduInfo;
    uint8 pciLength;
    const FrTp_ConnectionConfigType* config;
    FrTp_ConnectionRuntimeType* runtime;
    PduLengthType ffDataLength;
    Std_ReturnType result;

    config = FrTp_GetConnectionConfig(connIdx);
    runtime = FrTp_GetConnectionRuntime(connIdx);
    
    if ((config == NULL_PTR) || (runtime == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Encode FF PCI */
    result = FrTp_EncodeFfPdu(txBuffer, pduInfo, &pciLength);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Calculate FF data length (max payload - PCI length) */
    ffDataLength = config->maxPayload - pciLength;
    if (ffDataLength > pduInfo->SduLength)
    {
        ffDataLength = pduInfo->SduLength;
    }

    /* Copy data after PCI */
    if (pduInfo->SduDataPtr != NULL_PTR)
    {
        (void)memcpy(&txBuffer[pciLength], pduInfo->SduDataPtr, ffDataLength);
    }

    /* Setup TX PDU info */
    txPduInfo.SduDataPtr = txBuffer;
    txPduInfo.SduLength = pciLength + (uint16)ffDataLength;
    txPduInfo.MetaDataPtr = NULL_PTR;

    /* Update transfer counters */
    runtime->bytesTransferred = ffDataLength;
    runtime->bytesRemaining = pduInfo->SduLength - ffDataLength;

    /* Send via FrIf */
    return FrTp_SendFrame(connIdx, &txPduInfo);
}

/**
 * @brief Sends a consecutive frame (CF)
 * @param connIdx Connection index
 * @return E_OK if sent, E_NOT_OK otherwise
 */
static Std_ReturnType FrTp_SendConsecutiveFrame(FrTp_ConnectionIdxType connIdx)
{
    uint8 txBuffer[FRTP_MAX_PAYLOAD_PER_FRAME];
    PduInfoType txPduInfo;
    uint8 pciLength;
    const FrTp_ConnectionConfigType* config;
    FrTp_ConnectionRuntimeType* runtime;
    PduLengthType cfDataLength;
    PduLengthType dataOffset;
    Std_ReturnType result;

    config = FrTp_GetConnectionConfig(connIdx);
    runtime = FrTp_GetConnectionRuntime(connIdx);
    
    if ((config == NULL_PTR) || (runtime == NULL_PTR) || (runtime->txPduInfo == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Check if all data sent */
    if (runtime->bytesRemaining == 0U)
    {
        return E_OK;
    }

    /* Encode CF PCI */
    result = FrTp_EncodeCfPdu(txBuffer, runtime->txPduInfo, runtime->sequenceNumber, &pciLength);
    if (result != E_OK)
    {
        return E_NOT_OK;
    }

    /* Calculate CF data length */
    cfDataLength = config->maxPayload - pciLength;
    if (cfDataLength > runtime->bytesRemaining)
    {
        cfDataLength = runtime->bytesRemaining;
    }

    /* Copy data after PCI */
    dataOffset = runtime->bytesTransferred;
    if (runtime->txPduInfo->SduDataPtr != NULL_PTR)
    {
        (void)memcpy(&txBuffer[pciLength], 
                     &runtime->txPduInfo->SduDataPtr[dataOffset], 
                     cfDataLength);
    }

    /* Setup TX PDU info */
    txPduInfo.SduDataPtr = txBuffer;
    txPduInfo.SduLength = pciLength + (uint16)cfDataLength;
    txPduInfo.MetaDataPtr = NULL_PTR;

    /* Update transfer counters */
    runtime->bytesTransferred += cfDataLength;
    runtime->bytesRemaining -= cfDataLength;

    /* Increment sequence number */
    runtime->sequenceNumber = FrTp_IncSeq(runtime->sequenceNumber);

    /* Send via FrIf */
    return FrTp_SendFrame(connIdx, &txPduInfo);
}

/**
 * @brief Sends a frame via FrIf
 * @param connIdx Connection index
 * @param pduInfo Pointer to PDU info
 * @return E_OK if sent, E_NOT_OK otherwise
 */
static Std_ReturnType FrTp_SendFrame(FrTp_ConnectionIdxType connIdx, const PduInfoType* pduInfo)
{
    const FrTp_ConnectionConfigType* config;

    config = FrTp_GetConnectionConfig(connIdx);
    if (config == NULL_PTR)
    {
        return E_NOT_OK;
    }

    /* Call FrIf_Transmit (to be implemented based on FrIf API) */
    /* For now, we assume the FrIf API is compatible */
    
    /* Placeholder for FrIf call */
    /* return FrIf_Transmit(config->txPduId, pduInfo); */
    
    /* Return OK for now - actual implementation depends on FrIf */
    (void)config;
    (void)pduInfo;
    return E_OK;
}

#define FRTP_STOP_SEC_CODE
#include "MemMap.h"
