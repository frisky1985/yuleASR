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
 * @file FrTp_TxSm.c
 * @brief FlexRay Transport Protocol module - Transmit State Machine
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"
#include "FrTp_Private.h"
#include "Det.h"
#include "PduR.h"

/*==================================================================================================
*                                    LOCAL DEFINES
==================================================================================================*/

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static void FrTp_TxSm_StateStarting(FrTp_ConnectionIdxType connIdx);
static void FrTp_TxSm_StateWaitFc(FrTp_ConnectionIdxType connIdx);
static void FrTp_TxSm_StateSendingCf(FrTp_ConnectionIdxType connIdx);
static void FrTp_TxSm_StateWaitConfirm(FrTp_ConnectionIdxType connIdx);

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
#define FRTP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Transmit state machine main function
 * @param connIdx Connection index
 */
void FrTp_TxStateMachine(FrTp_ConnectionIdxType connIdx)
{
    const FrTp_ConnectionRuntimeType* runtime;

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
        case FRTP_STATE_TX_STARTING:
            FrTp_TxSm_StateStarting(connIdx);
            break;

        case FRTP_STATE_TX_WAIT_FC:
            FrTp_TxSm_StateWaitFc(connIdx);
            break;

        case FRTP_STATE_TX_SENDING_CF:
            FrTp_TxSm_StateSendingCf(connIdx);
            break;

        case FRTP_STATE_TX_WAIT_CONFIRM:
            FrTp_TxSm_StateWaitConfirm(connIdx);
            break;

        case FRTP_STATE_IDLE:
            /* Nothing to do in idle state */
            break;

        default:
            /* Invalid state for TX - reset connection */
#if (FRTP_DEV_ERROR_DETECT == STD_ON)
            FRTP_DET_REPORT_ERROR(FRTP_SID_MAINFUNCTION, FRTP_E_INVALID_TX_STATE);
#endif
            FrTp_ResetConnection(connIdx);
            break;
    }
}

/**
 * @brief Processes flow control reception for TX side
 * @param connIdx Connection index
 * @param flowStatus Flow status (CTS, WAIT, OVFLW)
 * @param blockSize Block size for CF transmission
 * @param stMin Minimum separation time
 */
void FrTp_ProcessFlowControl(FrTp_ConnectionIdxType connIdx, uint8 flowStatus, uint8 blockSize, uint8 stMin)
{
    FrTp_ConnectionRuntimeType* runtime;
    const FrTp_ConnectionConfigType* config;

    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
        return;
    }

    runtime = FrTp_GetConnectionRuntime(connIdx);
    config = FrTp_GetConnectionConfig(connIdx);

    if ((runtime == NULL_PTR) || (config == NULL_PTR))
    {
        return;
    }

    /* Check if we are waiting for FC */
    if (runtime->state != FRTP_STATE_TX_WAIT_FC)
    {
        /* Unexpected FC - ignore or reset */
        return;
    }

    /* Stop N_Bs timer */
    FrTp_StopTimer(runtime);

    /* Process flow status */
    switch (flowStatus)
    {
        case FRTP_FC_STATUS_CTS:
            /* Clear To Send - update parameters and start sending CFs */
            runtime->blockSize = blockSize;
            runtime->stMin = stMin;
            runtime->sequenceNumber = 0U;
            runtime->retryCount = 0U;
            
            /* Transition to CF sending state */
            runtime->state = FRTP_STATE_TX_SENDING_CF;
            break;

        case FRTP_FC_STATUS_WAIT:
            /* Receiver needs more time - restart N_Bs timer and continue waiting */
            FrTp_StartTimer(runtime, config->timeoutBs);
            break;

        case FRTP_FC_STATUS_OVFLW:
            /* Receiver buffer overflow - abort transmission */
            PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
            FrTp_ResetConnection(connIdx);
            break;

        default:
            /* Invalid flow status - abort transmission */
            PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
            FrTp_ResetConnection(connIdx);
            break;
    }
}

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief TX State: TX_STARTING
 * @param connIdx Connection index
 */
static void FrTp_TxSm_StateStarting(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return;
    }

    /* In this state, we wait for the FF to be sent and confirmed */
    /* The actual sending is done in FrTp_Transmit() */
    /* State transition happens in FrTp_TxConfirmation() */

    /* If we're here and still in STARTING state, check for timeout */
    if (FrTp_IsTimerExpired(runtime))
    {
        runtime->retryCount++;
        if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
        {
            /* Max retries - abort */
            PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
            FrTp_ResetConnection(connIdx);
        }
        /* Otherwise, retry is handled by FrTp_TxConfirmation() failure path */
    }
}

/**
 * @brief TX State: TX_WAIT_FC
 * @param connIdx Connection index
 */
static void FrTp_TxSm_StateWaitFc(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return;
    }

    /* In this state, we wait for FC from receiver */
    /* The FC processing is done in FrTp_ProcessFlowControl() */
    /* This function only handles timeout checking */

    if (FrTp_IsTimerExpired(runtime))
    {
        runtime->retryCount++;
        if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
        {
            /* Max retries - abort */
            PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
            FrTp_ResetConnection(connIdx);
        }
        /* Otherwise, resend FF and restart timer */
        /* This is handled by FrTp_MainFunction() timeout handling */
    }
}

/**
 * @brief TX State: TX_SENDING_CF
 * @param connIdx Connection index
 */
static void FrTp_TxSm_StateSendingCf(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;
    const FrTp_ConnectionConfigType* config;
    static uint8 cfCounter = 0U;  /* Counter for block size tracking */

    runtime = FrTp_GetConnectionRuntime(connIdx);
    config = FrTp_GetConnectionConfig(connIdx);

    if ((runtime == NULL_PTR) || (config == NULL_PTR))
    {
        return;
    }

    /* Check if all data has been sent */
    if (runtime->bytesRemaining == 0U)
    {
        /* All data sent - wait for final confirmation */
        runtime->state = FRTP_STATE_TX_WAIT_CONFIRM;
        FrTp_StartTimer(runtime, config->timeoutAs);
        cfCounter = 0U;
        return;
    }

    /* Check if we need to wait for next FC (block size reached) */
    if ((runtime->blockSize > 0U) && (cfCounter >= runtime->blockSize))
    {
        /* Block size reached - wait for next FC */
        runtime->state = FRTP_STATE_TX_WAIT_FC;
        FrTp_StartTimer(runtime, config->timeoutBs);
        cfCounter = 0U;
        return;
    }

    /* Check STmin before sending next CF */
    /* For simplicity, we assume FrTp_MainFunction() period is sufficient */
    /* A more detailed implementation would track STmin timing */

    /* Send next CF */
    {
        uint8 txBuffer[FRTP_MAX_PAYLOAD_PER_FRAME];
        PduInfoType txPduInfo;
        uint8 pciLength;
        PduLengthType cfDataLength;
        PduLengthType dataOffset;
        Std_ReturnType sendResult;

        /* Encode CF PCI */
        (void)FrTp_EncodeCfPdu(txBuffer, runtime->txPduInfo, runtime->sequenceNumber, &pciLength);

        /* Calculate CF data length */
        cfDataLength = config->maxPayload - pciLength;
        if (cfDataLength > runtime->bytesRemaining)
        {
            cfDataLength = runtime->bytesRemaining;
        }

        /* Copy data */
        dataOffset = runtime->bytesTransferred;
        if (runtime->txPduInfo->SduDataPtr != NULL_PTR)
        {
            (void)memcpy(&txBuffer[pciLength],
                        &runtime->txPduInfo->SduDataPtr[dataOffset],
                        cfDataLength);
        }

        /* Setup TX PDU info */
/*         txPduInfo.SduDataPtr = txBuffer; */
/*         txPduInfo.SduLength = pciLength + (uint16)cfDataLength; */
/*         txPduInfo.MetaDataPtr = NULL_PTR; */

        /* Update counters */
        runtime->bytesTransferred += cfDataLength;
        runtime->bytesRemaining -= cfDataLength;
        runtime->sequenceNumber = FrTp_IncSeq(runtime->sequenceNumber);
        cfCounter++;

        /* Send via FrIf (placeholder) */
        sendResult = E_OK;  /* FrIf_Transmit(config->txPduId, &txPduInfo); */
        
        if (sendResult != E_OK)
        {
            /* Send failed - will be handled by retry mechanism */
            runtime->retryCount++;
            if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
            {
                PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
                FrTp_ResetConnection(connIdx);
            }
        }
    }
}

/**
 * @brief TX State: TX_WAIT_CONFIRM
 * @param connIdx Connection index
 */
static void FrTp_TxSm_StateWaitConfirm(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    runtime = FrTp_GetConnectionRuntime(connIdx);
    if (runtime == NULL_PTR)
    {
        return;
    }

    /* In this state, we wait for transmission confirmation from FrIf */
    /* The confirmation handling is done in FrTp_TxConfirmation() */
    /* This function only handles timeout checking */

    if (FrTp_IsTimerExpired(runtime))
    {
        runtime->retryCount++;
        if (runtime->retryCount >= FRTP_MAX_RETRY_COUNT)
        {
            /* Max retries - abort */
            PduR_FrTpTxConfirmation(connIdx, E_NOT_OK);
            FrTp_ResetConnection(connIdx);
        }
        /* Otherwise, retry will be handled by FrTp_TxConfirmation() failure path */
    }
}

#define FRTP_STOP_SEC_CODE
#include "MemMap.h"
