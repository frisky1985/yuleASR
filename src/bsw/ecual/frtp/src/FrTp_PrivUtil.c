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
 * @file FrTp_PrivUtil.c
 * @brief FlexRay Transport Protocol private utilities
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "FrTp.h"
#include "FrTp_Private.h"

/*==================================================================================================
*                                    CONNECTION MANAGEMENT
==================================================================================================*/
#define FRTP_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Finds connection index by Tx PDU ID
 * @param txPduId Tx PDU ID
 * @return Connection index or FRTP_MAX_CONNECTIONS if not found
 */
FrTp_ConnectionIdxType FrTp_FindConnectionByTxPdu(PduIdType txPduId)
{
    uint8 connIdx;
    FrTp_ConnectionIdxType result = FRTP_MAX_CONNECTIONS;

    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        if (FrTp_ConnectionConfigs[connIdx].txPduId == txPduId)
        {
            result = connIdx;
            break;
        }
    }

    return result;
}

/**
 * @brief Finds connection index by Rx PDU ID
 * @param rxPduId Rx PDU ID
 * @return Connection index or FRTP_MAX_CONNECTIONS if not found
 */
FrTp_ConnectionIdxType FrTp_FindConnectionByRxPdu(PduIdType rxPduId)
{
    uint8 connIdx;
    FrTp_ConnectionIdxType result = FRTP_MAX_CONNECTIONS;

    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        if (FrTp_ConnectionConfigs[connIdx].rxPduId == rxPduId)
        {
            result = connIdx;
            break;
        }
    }

    return result;
}

/**
 * @brief Gets connection runtime data
 * @param connIdx Connection index
 * @return Pointer to connection runtime data
 */
FrTp_ConnectionRuntimeType* FrTp_GetConnectionRuntime(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* result = NULL_PTR;

    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        result = &FrTp_Runtime.connections[connIdx];
    }

    return result;
}

/**
 * @brief Gets connection configuration
 * @param connIdx Connection index
 * @return Pointer to connection configuration
 */
const FrTp_ConnectionConfigType* FrTp_GetConnectionConfig(FrTp_ConnectionIdxType connIdx)
{
    const FrTp_ConnectionConfigType* result = NULL_PTR;

    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        result = &FrTp_ConnectionConfigs[connIdx];
    }

    return result;
}

/**
 * @brief Sets connection state
 * @param connIdx Connection index
 * @param newState New state
 */
void FrTp_SetConnectionState(FrTp_ConnectionIdxType connIdx, FrTp_ConnectionStateType newState)
{
    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        FrTp_Runtime.connections[connIdx].state = newState;
    }
}

/**
 * @brief Resets connection to idle state
 * @param connIdx Connection index
 */
void FrTp_ResetConnection(FrTp_ConnectionIdxType connIdx)
{
    FrTp_ConnectionRuntimeType* runtime;

    if (connIdx >= FRTP_MAX_CONNECTIONS)
    {
        return;
    }

    runtime = &FrTp_Runtime.connections[connIdx];

    /* Clear all runtime data */
    runtime->state = FRTP_STATE_IDLE;
    runtime->dataLength = 0U;
    runtime->bytesTransferred = 0U;
    runtime->bytesRemaining = 0U;
    runtime->sequenceNumber = 0U;
    runtime->blockSize = 0U;
    runtime->stMin = 0U;
    runtime->retryCount = 0U;
    runtime->flags = FRTP_FLAG_NONE;
    runtime->timer = FRTP_TIMER_INACTIVE;
    runtime->timeoutValue = 0U;
    runtime->txPduInfo = NULL_PTR;
    runtime->rxPduInfo = NULL_PTR;
    runtime->rxBuffer = NULL_PTR;
    runtime->rxBufferSize = 0U;
    runtime->rxBufferLocked = FALSE;
}

/**
 * @brief Checks if connection is idle
 * @param connIdx Connection index
 * @return TRUE if idle, FALSE otherwise
 */
boolean FrTp_IsConnectionIdle(FrTp_ConnectionIdxType connIdx)
{
    boolean result = FALSE;

    if (connIdx < FRTP_MAX_CONNECTIONS)
    {
        result = (FrTp_Runtime.connections[connIdx].state == FRTP_STATE_IDLE) ? TRUE : FALSE;
    }

    return result;
}

/*==================================================================================================
*                                    PDU ENCODING
==================================================================================================*/

/**
 * @brief Encodes a single frame PDU
 * @param buffer Output buffer
 * @param pduInfo PDU info
 * @param pciLength Output PCI length
 * @return E_OK if successful
 */
Std_ReturnType FrTp_EncodeSfPdu(uint8* buffer, const PduInfoType* pduInfo, uint8* pciLength)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (pduInfo != NULL_PTR) && (pciLength != NULL_PTR))
    {
        if (pduInfo->SduLength <= FRTP_SF_MAX_DATA_LENGTH)
        {
            /* Encode SF PCI: type (00) + length (6 bits) */
            buffer[0] = FrTp_SetSfLength((uint8)pduInfo->SduLength);
            *pciLength = 1U;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief Encodes a first frame PDU
 * @param buffer Output buffer
 * @param pduInfo PDU info
 * @param pciLength Output PCI length
 * @return E_OK if successful
 */
Std_ReturnType FrTp_EncodeFfPdu(uint8* buffer, const PduInfoType* pduInfo, uint8* pciLength)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (pduInfo != NULL_PTR) && (pciLength != NULL_PTR))
    {
        if (pduInfo->SduLength > FRTP_SF_MAX_DATA_LENGTH)
        {
            /* Encode FF PCI: type (01) + length high (4 bits), length low (8 bits) */
            buffer[0] = FrTp_SetFfLengthHigh((uint16)pduInfo->SduLength);
            buffer[1] = FrTp_SetFfLengthLow((uint16)pduInfo->SduLength);
            *pciLength = 2U;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief Encodes a consecutive frame PDU
 * @param buffer Output buffer
 * @param pduInfo PDU info
 * @param seqNum Sequence number
 * @param pciLength Output PCI length
 * @return E_OK if successful
 */
Std_ReturnType FrTp_EncodeCfPdu(uint8* buffer, const PduInfoType* pduInfo, uint8 seqNum, uint8* pciLength)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (pduInfo != NULL_PTR) && (pciLength != NULL_PTR))
    {
        /* Encode CF PCI: type (10) + sequence number (4 bits) */
        buffer[0] = FrTp_SetCfSeq(seqNum);
        *pciLength = 1U;
        result = E_OK;
    }

    return result;
}

/**
 * @brief Encodes a flow control PDU
 * @param buffer Output buffer
 * @param flowStatus Flow status
 * @param blockSize Block size
 * @param stMin STmin value
 * @param pciLength Output PCI length
 * @return E_OK if successful
 */
Std_ReturnType FrTp_EncodeFcPdu(uint8* buffer, uint8 flowStatus, uint8 blockSize, uint8 stMin, uint8* pciLength)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (pciLength != NULL_PTR))
    {
        /* Encode FC PCI: type (11) + flow status (4 bits) */
        buffer[0] = FrTp_SetFcStatus(flowStatus);
        buffer[1] = blockSize;
        buffer[2] = stMin;
        *pciLength = 3U;
        result = E_OK;
    }

    return result;
}

/*==================================================================================================
*                                    PDU DECODING
==================================================================================================*/

/**
 * @brief Decodes PDU type from buffer
 * @param buffer Input buffer
 * @return PDU type
 */
FrTp_PduType FrTp_DecodePduType(const uint8* buffer)
{
    FrTp_PduType result = FRTP_PDU_SF;

    if (buffer != NULL_PTR)
    {
        uint8 pduType = FrTp_GetPduType(buffer[0]);

        switch (pduType)
        {
            case FRTP_PCI_TYPE_SF:
                result = FRTP_PDU_SF;
                break;
            case FRTP_PCI_TYPE_FF:
                result = FRTP_PDU_FF;
                break;
            case FRTP_PCI_TYPE_CF:
                result = FRTP_PDU_CF;
                break;
            case FRTP_PCI_TYPE_FC:
                result = FRTP_PDU_FC;
                break;
            default:
                result = FRTP_PDU_SF;
                break;
        }
    }

    return result;
}

/**
 * @brief Decodes a single frame PDU
 * @param buffer Input buffer
 * @param dataLength Output data length
 * @return E_OK if successful
 */
Std_ReturnType FrTp_DecodeSfPdu(const uint8* buffer, uint8* dataLength)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (dataLength != NULL_PTR))
    {
        *dataLength = FrTp_GetSfLength(buffer[0]);
        result = E_OK;
    }

    return result;
}

/**
 * @brief Decodes a first frame PDU
 * @param buffer Input buffer
 * @param dataLength Output data length
 * @return E_OK if successful
 */
Std_ReturnType FrTp_DecodeFfPdu(const uint8* buffer, uint16* dataLength)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (dataLength != NULL_PTR))
    {
        *dataLength = FrTp_GetFfLength(buffer[0], buffer[1]);
        result = E_OK;
    }

    return result;
}

/**
 * @brief Decodes a consecutive frame PDU
 * @param buffer Input buffer
 * @param seqNum Output sequence number
 * @return E_OK if successful
 */
Std_ReturnType FrTp_DecodeCfPdu(const uint8* buffer, uint8* seqNum)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (seqNum != NULL_PTR))
    {
        *seqNum = FrTp_GetCfSeq(buffer[0]);
        result = E_OK;
    }

    return result;
}

/**
 * @brief Decodes a flow control PDU
 * @param buffer Input buffer
 * @param flowStatus Output flow status
 * @param blockSize Output block size
 * @param stMin Output STmin
 * @return E_OK if successful
 */
Std_ReturnType FrTp_DecodeFcPdu(const uint8* buffer, uint8* flowStatus, uint8* blockSize, uint8* stMin)
{
    Std_ReturnType result = E_NOT_OK;

    if ((buffer != NULL_PTR) && (flowStatus != NULL_PTR) && 
        (blockSize != NULL_PTR) && (stMin != NULL_PTR))
    {
        *flowStatus = FrTp_GetFcStatus(buffer[0]);
        *blockSize = buffer[1];
        *stMin = buffer[2];
        result = E_OK;
    }

    return result;
}

/*==================================================================================================
*                                    TIMER MANAGEMENT
==================================================================================================*/

/**
 * @brief Starts a timer
 * @param conn Connection runtime
 * @param timeoutValue Timeout value
 */
void FrTp_StartTimer(FrTp_ConnectionRuntimeType* conn, uint16 timeoutValue)
{
    if (conn != NULL_PTR)
    {
        conn->timer = timeoutValue;
        conn->timeoutValue = timeoutValue;
    }
}

/**
 * @brief Stops a timer
 * @param conn Connection runtime
 */
void FrTp_StopTimer(FrTp_ConnectionRuntimeType* conn)
{
    if (conn != NULL_PTR)
    {
        conn->timer = FRTP_TIMER_INACTIVE;
        conn->timeoutValue = 0U;
    }
}

/**
 * @brief Checks if timer is running
 * @param conn Connection runtime
 * @return TRUE if running
 */
boolean FrTp_IsTimerRunning(const FrTp_ConnectionRuntimeType* conn)
{
    boolean result = FALSE;

    if (conn != NULL_PTR)
    {
        result = (conn->timer != FRTP_TIMER_INACTIVE) ? TRUE : FALSE;
    }

    return result;
}

/**
 * @brief Checks if timer has expired
 * @param conn Connection runtime
 * @return TRUE if expired
 */
boolean FrTp_IsTimerExpired(const FrTp_ConnectionRuntimeType* conn)
{
    boolean result = FALSE;

    if (conn != NULL_PTR)
    {
        result = (conn->timer == FRTP_TIMER_EXPIRED) ? TRUE : FALSE;
    }

    return result;
}

/**
 * @brief Updates all timers (called from MainFunction)
 */
void FrTp_UpdateTimers(void)
{
    uint8 connIdx;
    FrTp_ConnectionRuntimeType* runtime;

    for (connIdx = 0U; connIdx < FRTP_MAX_CONNECTIONS; connIdx++)
    {
        runtime = &FrTp_Runtime.connections[connIdx];

        if ((runtime->timer != FRTP_TIMER_INACTIVE) && (runtime->timer > 0U))
        {
            /* Decrement timer by main function period */
            if (runtime->timer > FRTP_MAIN_FUNCTION_PERIOD)
            {
                runtime->timer -= FRTP_MAIN_FUNCTION_PERIOD;
            }
            else
            {
                runtime->timer = FRTP_TIMER_EXPIRED;
            }
        }
    }
}

#define FRTP_STOP_SEC_CODE
#include "MemMap.h"
