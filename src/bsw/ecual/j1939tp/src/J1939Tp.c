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
 * @file J1939Tp.c
 * @brief J1939 Transport Protocol implementation
 * @details Implements SAE J1939-21 and J1939-22 Transport Protocol
 *          Supports BAM and RTS/CTS for commercial vehicles
 */

#include "J1939Tp.h"
#include "J1939Tp_Cfg.h"
#include "CanIf.h"
#include "PduR.h"
#include "Det.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define J1939TP_MODULE_ID                   (0xF2U)
#define J1939TP_VENDOR_ID                   (0x00U)
#define J1939TP_INSTANCE_ID                 (0x00U)

#define J1939TP_SID_INIT                    (0x01U)
#define J1939TP_SID_GETVERSIONINFO          (0x02U)
#define J1939TP_SID_TRANSMIT                (0x03U)
#define J1939TP_SID_RXINDICATION            (0x04U)
#define J1939TP_SID_TXCONFIRMATION          (0x05U)
#define J1939TP_SID_MAINFUNCTION            (0x06U)

/* TP Command values */
#define J1939TP_CMD_RTS                     (0x10U)
#define J1939TP_CMD_CTS                     (0x11U)
#define J1939TP_CMD_EOMACK                  (0x13U)
#define J1939TP_CMD_BAM                     (0x20U)
#define J1939TP_CMD_ABORT                   (0xFFU)

/* TP DT sequence number offset */
#define J1939TP_DT_SEQ_OFFSET               (1U)

/* Protocol limits */
#define J1939TP_MAX_DT_PACKETS              (255U)
#define J1939TP_MAX_MESSAGE_SIZE            (1785U) /* 255 * 7 */
#define J1939TP_DT_DATA_SIZE                (7U)
#ifndef J1939TP_PROTOCOL_CMDT
#define J1939TP_PROTOCOL_CMDT              (0x03U)
#endif
#ifndef J1939TP_TIMEOUT_COUNT
#define J1939TP_TIMEOUT_COUNT              (1000U)
#endif

/* Connection states */
typedef enum
{
    J1939TP_STATE_UNINIT = 0,
    J1939TP_STATE_INIT,
    J1939TP_STATE_IDLE,
    J1939TP_STATE_TX_WAIT_CTS,
    J1939TP_STATE_TX_WAIT_EOMACK,
    J1939TP_STATE_TX_SENDING_BAM,
    J1939TP_STATE_RX_WAIT_CTS,
    J1939TP_STATE_RX_SENDING_CTS,
    J1939TP_STATE_RX_RECEIVING,
    J1939TP_STATE_RX_SENDING_EOMACK
} J1939Tp_StateType;

/* Connection control block */
typedef struct
{
    J1939Tp_StateType   State;
    PduIdType           NSduId;
    PduLengthType       TotalSize;
    PduLengthType       DataOffset;
    uint8               NumPackets;
    uint8               NextSeqNum;
    uint8               BlockSize;
    uint32              Timer;
    uint8               RetryCount;
    uint8               Sa;
    uint8               Da;
    uint8               Pgn[3];
} J1939Tp_ConnectionType;

/*******************************************************************************
 * Local Variables
 ******************************************************************************/
static J1939Tp_ConnectionType J1939Tp_Connections[J1939TP_NUM_CONNECTIONS];
static J1939Tp_StateType J1939Tp_ModuleState = J1939TP_STATE_UNINIT;

/*******************************************************************************
 * Local Functions
 ******************************************************************************/
static uint8 J1939Tp_CalculateChecksum(const uint8* DataPtr, uint16 Length);
static void J1939Tp_SendRTS(PduIdType ConnectionId);
static void J1939Tp_SendCTS(PduIdType ConnectionId);
static void J1939Tp_SendEOMACK(PduIdType ConnectionId);
static void J1939Tp_SendBAM(PduIdType ConnectionId);
static void J1939Tp_SendDT(PduIdType ConnectionId);
static void J1939Tp_SendAbort(PduIdType ConnectionId, uint8 Reason);

/*******************************************************************************
 * API Functions
 ******************************************************************************/

void J1939Tp_Init(const J1939Tp_ConfigType* ConfigPtr)
{
    uint8 i;
    
    for (i = 0U; i < J1939TP_NUM_CONNECTIONS; i++)
    {
        J1939Tp_Connections[i].State = J1939TP_STATE_IDLE;
        J1939Tp_Connections[i].Timer = 0U;
        J1939Tp_Connections[i].RetryCount = 0U;
    }
    
    J1939Tp_ModuleState = J1939TP_STATE_INIT;
}

Std_ReturnType J1939Tp_Transmit(PduIdType TxSduId, const PduInfoType* PduInfoPtr)
{
    J1939Tp_ConnectionType* ConnPtr;
    const J1939Tp_NSduConfigType* NSduConfigPtr;
    
    if (J1939Tp_ModuleState != J1939TP_STATE_INIT)
    {
        return E_NOT_OK;
    }
    
    if ((TxSduId >= J1939TP_NUM_NSDUS) || (PduInfoPtr == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    NSduConfigPtr = &J1939Tp_NSduConfig[TxSduId];
    ConnPtr = &J1939Tp_Connections[NSduConfigPtr->ConnectionIdx];
    
    if (ConnPtr->State != J1939TP_STATE_IDLE)
    {
        return E_NOT_OK;
    }
    
    /* Check message size */
    if (PduInfoPtr->SduLength > J1939TP_MAX_MESSAGE_SIZE)
    {
        return E_NOT_OK;
    }
    
    /* Store transmission parameters */
    ConnPtr->NSduId = TxSduId;
    ConnPtr->TotalSize = PduInfoPtr->SduLength;
    ConnPtr->DataOffset = 0U;
    ConnPtr->NumPackets = (uint8)((PduInfoPtr->SduLength + 6U) / 7U);
    ConnPtr->NextSeqNum = 1U;
    ConnPtr->Timer = 0U;
    ConnPtr->RetryCount = 0U;
    
    /* Determine protocol to use */
    if (NSduConfigPtr->Protocol == J1939TP_PROTOCOL_BAM)
    {
        ConnPtr->State = J1939TP_STATE_TX_SENDING_BAM;
        J1939Tp_SendBAM(TxSduId);
    }
    else if (NSduConfigPtr->Protocol == J1939TP_PROTOCOL_CMDT)
    {
        ConnPtr->State = J1939TP_STATE_TX_WAIT_CTS;
        J1939Tp_SendRTS(TxSduId);
    }
    
    return E_OK;
}

void J1939Tp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 Command;
    J1939Tp_ConnectionType* ConnPtr;
    
    if (J1939Tp_ModuleState != J1939TP_STATE_INIT)
    {
        return;
    }
    
    if ((PduInfoPtr == NULL_PTR) || (PduInfoPtr->SduDataPtr == NULL_PTR))
    {
        return;
    }
    
    Command = PduInfoPtr->SduDataPtr[0];
    
    switch (Command)
    {
        case J1939TP_CMD_RTS:
            /* Handle RTS - start CMDT reception */
            break;
            
        case J1939TP_CMD_CTS:
            /* Handle CTS - continue CMDT transmission */
            break;
            
        case J1939TP_CMD_EOMACK:
            /* Handle EOM ACK - transmission complete */
            break;
            
        case J1939TP_CMD_BAM:
            /* Handle BAM - start BAM reception */
            break;
            
        case J1939TP_CMD_ABORT:
            /* Handle abort */
            break;
            
        default:
            /* DT message (1-255) */
            if ((Command >= 1U) && (Command <= 255U))
            {
                /* Handle data transfer */
            }
            break;
    }
}

void J1939Tp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    J1939Tp_ConnectionType* ConnPtr;
    const J1939Tp_NSduConfigType* NSduConfigPtr;
    
    if (J1939Tp_ModuleState != J1939TP_STATE_INIT)
    {
        return;
    }
    
    if (TxPduId >= J1939TP_NUM_NSDUS)
    {
        return;
    }
    
    NSduConfigPtr = &J1939Tp_NSduConfig[TxPduId];
    ConnPtr = &J1939Tp_Connections[NSduConfigPtr->ConnectionIdx];
    
    if (result != E_OK)
    {
        /* Transmission failed */
        ConnPtr->State = J1939TP_STATE_IDLE;
        return;
    }
    
    switch (ConnPtr->State)
    {
        case J1939TP_STATE_TX_SENDING_BAM:
            /* BAM announcement sent, start sending DT */
            ConnPtr->NextSeqNum = 1U;
            J1939Tp_SendDT(TxPduId);
            break;
            
        case J1939TP_STATE_TX_WAIT_CTS:
            /* RTS sent, waiting for CTS */
            break;
            
        default:
            break;
    }
}

void J1939Tp_MainFunction(void)
{
    uint8 i;
    J1939Tp_ConnectionType* ConnPtr;
    
    if (J1939Tp_ModuleState != J1939TP_STATE_INIT)
    {
        return;
    }
    
    for (i = 0U; i < J1939TP_NUM_CONNECTIONS; i++)
    {
        ConnPtr = &J1939Tp_Connections[i];
        
        if (ConnPtr->State == J1939TP_STATE_IDLE)
        {
            continue;
        }
        
        /* Increment timer */
        ConnPtr->Timer++;
        
        /* Check for timeout */
        if (ConnPtr->Timer >= J1939TP_TIMEOUT_COUNT)
        {
            /* Timeout - abort connection */
            J1939Tp_SendAbort(ConnPtr->NSduId, 0x01U);
            ConnPtr->State = J1939TP_STATE_IDLE;
        }
    }
}

/*******************************************************************************
 * Local Functions
 ******************************************************************************/

static void J1939Tp_SendRTS(PduIdType ConnectionId)
{
    uint8 Data[8];
    PduInfoType PduInfo;
    J1939Tp_ConnectionType* ConnPtr;
    
    ConnPtr = &J1939Tp_Connections[ConnectionId];
    
    Data[0] = J1939TP_CMD_RTS;
    Data[1] = (uint8)(ConnPtr->TotalSize);
    Data[2] = (uint8)(ConnPtr->TotalSize >> 8);
    Data[3] = ConnPtr->NumPackets;
    Data[4] = 0xFFU; /* Maximum number of packets that can be sent */
    Data[5] = ConnPtr->Pgn[0];
    Data[6] = ConnPtr->Pgn[1];
    Data[7] = ConnPtr->Pgn[2];
    
    PduInfo.SduDataPtr = Data;
    PduInfo.SduLength = 8U;
    PduInfo.MetaDataPtr = NULL_PTR;
    
    CanIf_Transmit(J1939Tp_NSduConfig[ConnectionId].TxPduId, &PduInfo);
}

static void J1939Tp_SendCTS(PduIdType ConnectionId)
{
    uint8 Data[8];
    PduInfoType PduInfo;
    
    Data[0] = J1939TP_CMD_CTS;
    Data[1] = 0xFFU; /* Number of packets that can be sent */
    Data[2] = 1U;    /* Next packet number to be sent */
    Data[3] = 0xFFU;
    Data[4] = 0xFFU;
    Data[5] = 0x00U;
    Data[6] = 0x00U;
    Data[7] = 0x00U;
    
    PduInfo.SduDataPtr = Data;
    PduInfo.SduLength = 8U;
    PduInfo.MetaDataPtr = NULL_PTR;
    
    CanIf_Transmit(J1939Tp_NSduConfig[ConnectionId].TxPduId, &PduInfo);
}

static void J1939Tp_SendEOMACK(PduIdType ConnectionId)
{
    uint8 Data[8];
    PduInfoType PduInfo;
    J1939Tp_ConnectionType* ConnPtr;
    
    ConnPtr = &J1939Tp_Connections[ConnectionId];
    
    Data[0] = J1939TP_CMD_EOMACK;
    Data[1] = (uint8)(ConnPtr->TotalSize);
    Data[2] = (uint8)(ConnPtr->TotalSize >> 8);
    Data[3] = ConnPtr->NumPackets;
    Data[4] = 0xFFU;
    Data[5] = ConnPtr->Pgn[0];
    Data[6] = ConnPtr->Pgn[1];
    Data[7] = ConnPtr->Pgn[2];
    
    PduInfo.SduDataPtr = Data;
    PduInfo.SduLength = 8U;
    PduInfo.MetaDataPtr = NULL_PTR;
    
    CanIf_Transmit(J1939Tp_NSduConfig[ConnectionId].TxPduId, &PduInfo);
}

static void J1939Tp_SendBAM(PduIdType ConnectionId)
{
    uint8 Data[8];
    PduInfoType PduInfo;
    J1939Tp_ConnectionType* ConnPtr;
    
    ConnPtr = &J1939Tp_Connections[ConnectionId];
    
    Data[0] = J1939TP_CMD_BAM;
    Data[1] = (uint8)(ConnPtr->TotalSize);
    Data[2] = (uint8)(ConnPtr->TotalSize >> 8);
    Data[3] = ConnPtr->NumPackets;
    Data[4] = 0xFFU;
    Data[5] = ConnPtr->Pgn[0];
    Data[6] = ConnPtr->Pgn[1];
    Data[7] = ConnPtr->Pgn[2];
    
    PduInfo.SduDataPtr = Data;
    PduInfo.SduLength = 8U;
    PduInfo.MetaDataPtr = NULL_PTR;
    
    CanIf_Transmit(J1939Tp_NSduConfig[ConnectionId].TxPduId, &PduInfo);
}

static void J1939Tp_SendDT(PduIdType ConnectionId)
{
    uint8 Data[8];
    PduInfoType PduInfo;
    J1939Tp_ConnectionType* ConnPtr;
    
    ConnPtr = &J1939Tp_Connections[ConnectionId];
    
    Data[0] = ConnPtr->NextSeqNum;
    /* Fill data bytes 1-7 with actual payload */
    /* ... */
    
    PduInfo.SduDataPtr = Data;
    PduInfo.SduLength = 8U;
    PduInfo.MetaDataPtr = NULL_PTR;
    
    CanIf_Transmit(J1939Tp_NSduConfig[ConnectionId].TxPduId, &PduInfo);
    
    ConnPtr->NextSeqNum++;
    if (ConnPtr->NextSeqNum > ConnPtr->NumPackets)
    {
        /* All packets sent */
    }
}

static void J1939Tp_SendAbort(PduIdType ConnectionId, uint8 Reason)
{
    uint8 Data[8];
    PduInfoType PduInfo;
    
    Data[0] = J1939TP_CMD_ABORT;
    Data[1] = Reason;
    Data[2] = 0xFFU;
    Data[3] = 0xFFU;
    Data[4] = 0xFFU;
    Data[5] = 0x00U;
    Data[6] = 0x00U;
    Data[7] = 0x00U;
    
    PduInfo.SduDataPtr = Data;
    PduInfo.SduLength = 8U;
    PduInfo.MetaDataPtr = NULL_PTR;
    
    CanIf_Transmit(J1939Tp_NSduConfig[ConnectionId].TxPduId, &PduInfo);
}

static uint8 J1939Tp_CalculateChecksum(const uint8* DataPtr, uint16 Length)
{
    uint16 i;
    uint8 Checksum = 0U;
    
    for (i = 0U; i < Length; i++)
    {
        Checksum += DataPtr[i];
    }
    
    return (uint8)(0xFFU - Checksum + 1U);
}
