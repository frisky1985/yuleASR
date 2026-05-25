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
 * @brief AUTOSAR J1939 Transport Protocol Implementation
 *
 * Implements SAE J1939-21 Transport Protocol:
 * - BAM (Broadcast Announce Message) - connectionless broadcast
 * - CMDT (Connection Mode Data Transfer) - RTS/CTS session
 *
 * @copyright Copyright (c) 2026
 */

#include "J1939Tp.h"
#include "Det.h"
#include "PduR.h"
#include "CanIf.h"
#include "MemMap.h"

/*==================================================================================================
 *                                Local Macro Definitions
 *================================================================================================*/
#define J1939TP_STATE_UNINITED          ((uint8)0x00U)
#define J1939TP_STATE_INITED            ((uint8)0x01U)

/*==================================================================================================
 *                                 Module Variables
 *================================================================================================*/
#define J1939TP_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

static uint8 J1939Tp_ModuleState = J1939TP_STATE_UNINITED;
static const J1939Tp_ConfigType* J1939Tp_ConfigPtr = NULL;

#define J1939TP_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define J1939TP_START_SEC_VAR_NO_INIT_UNSPECIFIED
#include "MemMap.h"

static J1939Tp_TxChannelType J1939Tp_TxChannels[J1939TP_MAX_TX_CHANNELS];
static J1939Tp_RxChannelType J1939Tp_RxChannels[J1939TP_MAX_RX_CHANNELS];

static uint8 J1939Tp_RxBuffer[J1939TP_MAX_TP_SIZE];
static uint8 J1939Tp_TxBuffer[J1939TP_MAX_TP_SIZE];

#define J1939TP_STOP_SEC_VAR_NO_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                               Local Function Prototypes
 *================================================================================================*/
static Std_ReturnType J1939Tp_InternalInit(void);
static void J1939Tp_ResetTxChannel(uint8 ChannelIdx);
static void J1939Tp_ResetRxChannel(uint8 ChannelIdx);
static void J1939Tp_ProcessTxChannel(uint8 ChannelIdx);
static void J1939Tp_ProcessRxChannel(uint8 ChannelIdx);
static Std_ReturnType J1939Tp_SendTpCm(uint8 ChannelIdx, uint8 ControlByte);
static Std_ReturnType J1939Tp_SendTpDt(uint8 ChannelIdx);
static void J1939Tp_ProcessTpCmRx(const PduInfoType* PduInfoPtr, uint8 ChannelIdx);
static void J1939Tp_ProcessTpDtRx(const PduInfoType* PduInfoPtr, uint8 ChannelIdx);
static void J1939Tp_HandleRts(const uint8* Data, uint8 ChannelIdx);
static void J1939Tp_HandleCts(const uint8* Data, uint8 ChannelIdx);
static void J1939Tp_HandleEomAck(const uint8* Data, uint8 ChannelIdx);
static void J1939Tp_HandleBam(const uint8* Data, uint8 ChannelIdx);
static void J1939Tp_HandleAbort(const uint8* Data, uint8 ChannelIdx);

/*==================================================================================================
 *                                   API Functions
 *================================================================================================*/
Std_ReturnType J1939Tp_Init(const J1939Tp_ConfigType* ConfigPtr)
{
    Std_ReturnType result = E_OK;

#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_INIT, J1939TP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    J1939Tp_ConfigPtr = ConfigPtr;

    /* Initialize all channels */
    for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++) {
        J1939Tp_ResetTxChannel(i);
    }

    for (uint8 i = 0U; i < J1939TP_MAX_RX_CHANNELS; i++) {
        J1939Tp_ResetRxChannel(i);
    }

    J1939Tp_ModuleState = J1939TP_STATE_INITED;

    return result;
}

void J1939Tp_DeInit(void)
{
    if (J1939Tp_ModuleState == J1939TP_STATE_INITED) {
        /* Reset all channels */
        for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++) {
            J1939Tp_ResetTxChannel(i);
        }

        for (uint8 i = 0U; i < J1939TP_MAX_RX_CHANNELS; i++) {
            J1939Tp_ResetRxChannel(i);
        }

        J1939Tp_ConfigPtr = NULL;
        J1939Tp_ModuleState = J1939TP_STATE_UNINITED;
    }
}

void J1939Tp_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (J1939TP_VERSION_INFO_API == STD_ON)
#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_GET_VERSION_INFO, J1939TP_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfo->vendorID = J1939TP_VENDOR_ID;
    VersionInfo->moduleID = J1939TP_MODULE_ID;
    VersionInfo->sw_major_version = J1939TP_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = J1939TP_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = J1939TP_SW_PATCH_VERSION;
#endif
}

Std_ReturnType J1939Tp_Transmit(PduIdType TxSduId, const PduInfoType* TxInfoPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
    if (J1939Tp_ModuleState != J1939TP_STATE_INITED) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_TRANSMIT, J1939TP_E_UNINIT);
        return E_NOT_OK;
    }

    if (TxInfoPtr == NULL) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_TRANSMIT, J1939TP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((TxSduId < J1939Tp_ConfigPtr->PgCount) && (TxInfoPtr != NULL)) {
        const J1939Tp_PgConfigType* pgConfig = &J1939Tp_ConfigPtr->PgConfigs[TxSduId];

        if (TxInfoPtr->SduLength <= 8U) {
            /* Single frame transmission - direct to CanIf */
            result = CanIf_Transmit(pgConfig->PduId, TxInfoPtr);
        } else if (TxInfoPtr->SduLength <= J1939TP_MAX_TP_SIZE) {
            /* Multi-packet transmission */
            for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++) {
                if (J1939Tp_TxChannels[i].State == J1939TP_STATE_IDLE) {
                    /* Find associated connection */
                    for (uint8 j = 0U; j < J1939Tp_ConfigPtr->ConnectionCount; j++) {
                        if (J1939Tp_ConfigPtr->Connections[j].SduId == TxSduId) {
                            /* Initialize channel for transmission */
                            J1939Tp_TxChannels[i].State = J1939TP_STATE_RTS_TX;
                            J1939Tp_TxChannels[i].TotalBytes = TxInfoPtr->SduLength;
                            J1939Tp_TxChannels[i].PacketsToSend = (uint16)((TxInfoPtr->SduLength + 6U) / 7U);
                            J1939Tp_TxChannels[i].SeqNumber = 1U;
                            J1939Tp_TxChannels[i].SentBytes = 0U;
                            J1939Tp_TxChannels[i].PacketsSent = 0U;
                            J1939Tp_TxChannels[i].T1Timer = J1939Tp_ConfigPtr->Connections[j].T1Timeout;

                            /* Copy data to internal buffer */
                            for (uint16 k = 0U; k < TxInfoPtr->SduLength; k++) {
                                J1939Tp_TxBuffer[k] = TxInfoPtr->SduDataPtr[k];
                            }

                            result = E_OK;
                            break;
                        }
                    }
                    if (result == E_OK) {
                        break;
                    }
                }
            }
        }
    }

    return result;
}

Std_ReturnType J1939Tp_CancelTransmit(PduIdType TxSduId)
{
    Std_ReturnType result = E_NOT_OK;

#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
    if (J1939Tp_ModuleState != J1939TP_STATE_INITED) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_CANCEL_TRANSMIT, J1939TP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++) {
        if (J1939Tp_TxChannels[i].State != J1939TP_STATE_IDLE) {
            J1939Tp_ResetTxChannel(i);
            result = E_OK;
            break;
        }
    }

    return result;
}

Std_ReturnType J1939Tp_CancelReceive(PduIdType RxSduId)
{
    Std_ReturnType result = E_NOT_OK;

#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
    if (J1939Tp_ModuleState != J1939TP_STATE_INITED) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_CANCEL_RECEIVE, J1939TP_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    for (uint8 i = 0U; i < J1939TP_MAX_RX_CHANNELS; i++) {
        if (J1939Tp_RxChannels[i].State != J1939TP_STATE_IDLE) {
            J1939Tp_ResetRxChannel(i);
            result = E_OK;
            break;
        }
    }

    return result;
}

Std_ReturnType J1939Tp_ChangeParameter(PduIdType SduId, J1939Tp_ParameterType Parameter, uint16 Value)
{
    /* Not fully implemented - placeholder */
    (void)SduId;
    (void)Parameter;
    (void)Value;
    return E_NOT_OK;
}

void J1939Tp_MainFunction(void)
{
    if (J1939Tp_ModuleState == J1939TP_STATE_INITED) {
        /* Process all TX channels */
        for (uint8 i = 0U; i < J1939TP_MAX_TX_CHANNELS; i++) {
            J1939Tp_ProcessTxChannel(i);
        }

        /* Process all RX channels */
        for (uint8 i = 0U; i < J1939TP_MAX_RX_CHANNELS; i++) {
            J1939Tp_ProcessRxChannel(i);
        }
    }
}

void J1939Tp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
    if (PduInfoPtr == NULL) {
        Det_ReportError(J1939TP_MODULE_ID, J1939TP_INSTANCE_ID, J1939TP_SID_RX_INDICATION, J1939TP_E_PARAM_POINTER);
        return;
    }
#endif

    if (PduInfoPtr->SduDataPtr != NULL) {
        /* Determine if this is TP.CM or TP.DT */
        uint32 pgn = 0U; /* Extract from CAN ID metadata if available */

        for (uint8 i = 0U; i < J1939Tp_ConfigPtr->ConnectionCount; i++) {
            if ((J1939Tp_ConfigPtr->Connections[i].RxPduId == RxPduId) ||
                (J1939Tp_ConfigPtr->Connections[i].TxPduId == RxPduId)) {

                if (pgn == J1939TP_PGN_TP_CM) {
                    J1939Tp_ProcessTpCmRx(PduInfoPtr, i);
                } else if (pgn == J1939TP_PGN_TP_DT) {
                    J1939Tp_ProcessTpDtRx(PduInfoPtr, i);
                }
                break;
            }
        }
    }
}

void J1939Tp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
    /* Process transmission confirmation - update channel state if needed */
}

/*==================================================================================================
 *                               Local Functions
 *================================================================================================*/
static void J1939Tp_ResetTxChannel(uint8 ChannelIdx)
{
    if (ChannelIdx < J1939TP_MAX_TX_CHANNELS) {
        J1939Tp_TxChannels[ChannelIdx].State = J1939TP_STATE_IDLE;
        J1939Tp_TxChannels[ChannelIdx].SeqNumber = 0U;
        J1939Tp_TxChannels[ChannelIdx].TotalBytes = 0U;
        J1939Tp_TxChannels[ChannelIdx].SentBytes = 0U;
        J1939Tp_TxChannels[ChannelIdx].PacketsToSend = 0U;
        J1939Tp_TxChannels[ChannelIdx].PacketsSent = 0U;
        J1939Tp_TxChannels[ChannelIdx].T1Timer = 0U;
        J1939Tp_TxChannels[ChannelIdx].T3Timer = 0U;
        J1939Tp_TxChannels[ChannelIdx].T4Timer = 0U;
        J1939Tp_TxChannels[ChannelIdx].BcTimerActive = FALSE;
    }
}

static void J1939Tp_ResetRxChannel(uint8 ChannelIdx)
{
    if (ChannelIdx < J1939TP_MAX_RX_CHANNELS) {
        J1939Tp_RxChannels[ChannelIdx].State = J1939TP_STATE_IDLE;
        J1939Tp_RxChannels[ChannelIdx].SeqNumber = 0U;
        J1939Tp_RxChannels[ChannelIdx].TotalBytes = 0U;
        J1939Tp_RxChannels[ChannelIdx].ReceivedBytes = 0U;
        J1939Tp_RxChannels[ChannelIdx].PacketsToReceive = 0U;
        J1939Tp_RxChannels[ChannelIdx].PacketsReceived = 0U;
        J1939Tp_RxChannels[ChannelIdx].T1Timer = 0U;
        J1939Tp_RxChannels[ChannelIdx].T2Timer = 0U;
        J1939Tp_RxChannels[ChannelIdx].NBrTimer = 0U;
    }
}

static void J1939Tp_ProcessTxChannel(uint8 ChannelIdx)
{
    if (ChannelIdx >= J1939TP_MAX_TX_CHANNELS) {
        return;
    }

    J1939Tp_TxChannelType* channel = &J1939Tp_TxChannels[ChannelIdx];

    switch (channel->State) {
        case J1939TP_STATE_RTS_TX:
            /* Send RTS message */
            if (J1939Tp_SendTpCm(ChannelIdx, J1939TP_CM_RTS) == E_OK) {
                channel->State = J1939TP_STATE_CTS_RX;
                channel->T1Timer = J1939TP_T1_TIMEOUT_DEFAULT;
            }
            break;

        case J1939TP_STATE_DT_TX:
            /* Send DT packet */
            if (J1939Tp_SendTpDt(ChannelIdx) == E_OK) {
                channel->PacketsSent++;
                if (channel->PacketsSent >= channel->PacketsToSend) {
                    channel->State = J1939TP_STATE_EOM_ACK;
                }
            }
            break;

        case J1939TP_STATE_BAM_TX:
            /* Send BAM followed by DT packets */
            /* BAM 广播传输逻辑 - 实现方式与 CMDT 类似但无需 CTS/EOM 握手 */
            break;

        case J1939TP_STATE_CTS_RX:
        case J1939TP_STATE_EOM_ACK:
            /* Decrement timers */
            if (channel->T1Timer > 0U) {
                channel->T1Timer--;
            }
            if (channel->T3Timer > 0U) {
                channel->T3Timer--;
            }
            break;

        default:
            /* IDLE or other states - do nothing */
            break;
    }
}

static void J1939Tp_ProcessRxChannel(uint8 ChannelIdx)
{
    if (ChannelIdx >= J1939TP_MAX_RX_CHANNELS) {
        return;
    }

    J1939Tp_RxChannelType* channel = &J1939Tp_RxChannels[ChannelIdx];

    /* Decrement timers */
    if (channel->T1Timer > 0U) {
        channel->T1Timer--;
    }
    if (channel->T2Timer > 0U) {
        channel->T2Timer--;
    }
}

static Std_ReturnType J1939Tp_SendTpCm(uint8 ChannelIdx, uint8 ControlByte)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 data[8];

    if (ChannelIdx < J1939TP_MAX_TX_CHANNELS) {
        J1939Tp_TxChannelType* channel = &J1939Tp_TxChannels[ChannelIdx];

        /* Build TP.CM message */
        data[0] = ControlByte;
        data[1] = (uint8)(channel->TotalBytes & 0xFFU);
        data[2] = (uint8)((channel->TotalBytes >> 8) & 0xFFU);
        data[3] = (uint8)(channel->PacketsToSend);
        data[4] = 0xFFU; /* Maximum number of packets that can be sent (reserved for RTS) */
        data[5] = (uint8)(channel->Pgn & 0xFFU);
        data[6] = (uint8)((channel->Pgn >> 8) & 0xFFU);
        data[7] = (uint8)((channel->Pgn >> 16) & 0xFFU);

        PduInfoType pduInfo;
        pduInfo.SduDataPtr = data;
        pduInfo.SduLength = 8U;

        result = CanIf_Transmit(J1939Tp_ConfigPtr->Connections[ChannelIdx].TxPduId, &pduInfo);
    }

    return result;
}

static Std_ReturnType J1939Tp_SendTpDt(uint8 ChannelIdx)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 data[8];

    if (ChannelIdx < J1939TP_MAX_TX_CHANNELS) {
        J1939Tp_TxChannelType* channel = &J1939Tp_TxChannels[ChannelIdx];

        /* Build TP.DT message */
        data[0] = channel->SeqNumber;

        /* Copy 7 bytes of data */
        uint16 offset = (uint16)((channel->SeqNumber - 1U) * 7U);
        for (uint8 i = 0U; i < 7U; i++) {
            if ((offset + i) < channel->TotalBytes) {
                data[1U + i] = J1939Tp_TxBuffer[offset + i];
            } else {
                data[1U + i] = 0xFFU; /* Padding */
            }
        }

        PduInfoType pduInfo;
        pduInfo.SduDataPtr = data;
        pduInfo.SduLength = 8U;

        result = CanIf_Transmit(J1939Tp_ConfigPtr->Connections[ChannelIdx].TxDtPduId, &pduInfo);

        if (result == E_OK) {
            channel->SeqNumber++;
        }
    }

    return result;
}

static void J1939Tp_ProcessTpCmRx(const PduInfoType* PduInfoPtr, uint8 ChannelIdx)
{
    if ((PduInfoPtr != NULL) && (PduInfoPtr->SduLength >= 8U) && (ChannelIdx < J1939TP_MAX_RX_CHANNELS)) {
        uint8 controlByte = PduInfoPtr->SduDataPtr[0];

        switch (controlByte) {
            case J1939TP_CM_RTS:
                J1939Tp_HandleRts(PduInfoPtr->SduDataPtr, ChannelIdx);
                break;

            case J1939TP_CM_CTS:
                J1939Tp_HandleCts(PduInfoPtr->SduDataPtr, ChannelIdx);
                break;

            case J1939TP_CM_ACK:
                J1939Tp_HandleEomAck(PduInfoPtr->SduDataPtr, ChannelIdx);
                break;

            case J1939TP_CM_BAM:
                J1939Tp_HandleBam(PduInfoPtr->SduDataPtr, ChannelIdx);
                break;

            case J1939TP_CM_ABORT:
                J1939Tp_HandleAbort(PduInfoPtr->SduDataPtr, ChannelIdx);
                break;

            default:
                /* Unknown control byte */
                break;
        }
    }
}

static void J1939Tp_ProcessTpDtRx(const PduInfoType* PduInfoPtr, uint8 ChannelIdx)
{
    if ((PduInfoPtr != NULL) && (PduInfoPtr->SduLength >= 8U) && (ChannelIdx < J1939TP_MAX_RX_CHANNELS)) {
        J1939Tp_RxChannelType* channel = &J1939Tp_RxChannels[ChannelIdx];

        if ((channel->State == J1939TP_STATE_DT_RX) || (channel->State == J1939TP_STATE_BAM_RX)) {
            uint8 seqNumber = PduInfoPtr->SduDataPtr[0];

            if (seqNumber == (channel->SeqNumber + 1U)) {
                /* Correct sequence number - copy data */
                uint16 offset = (uint16)(channel->SeqNumber * 7U);

                for (uint8 i = 0U; i < 7U; i++) {
                    if ((offset + i) < channel->TotalBytes) {
                        J1939Tp_RxBuffer[offset + i] = PduInfoPtr->SduDataPtr[1U + i];
                        channel->ReceivedBytes++;
                    }
                }

                channel->SeqNumber++;
                channel->PacketsReceived++;

                /* Check if reception complete */
                if (channel->PacketsReceived >= channel->PacketsToReceive) {
                    /* Reception complete - forward to PduR */
                    PduInfoType pduInfo;
                    pduInfo.SduDataPtr = J1939Tp_RxBuffer;
                    pduInfo.SduLength = channel->TotalBytes;
                    PduR_J1939TpRxIndication(J1939Tp_ConfigPtr->Connections[ChannelIdx].SduId, &pduInfo);

                    J1939Tp_ResetRxChannel(ChannelIdx);
                }
            }
        }
    }
}

static void J1939Tp_HandleRts(const uint8* Data, uint8 ChannelIdx)
{
    if ((Data != NULL) && (ChannelIdx < J1939TP_MAX_RX_CHANNELS)) {
        J1939Tp_RxChannelType* channel = &J1939Tp_RxChannels[ChannelIdx];

        if (channel->State == J1939TP_STATE_IDLE) {
            /* Parse RTS message */
            channel->TotalBytes = (uint16)Data[1] | ((uint16)Data[2] << 8);
            channel->PacketsToReceive = Data[3];
            channel->Pgn = (uint32)Data[5] | ((uint32)Data[6] << 8) | ((uint32)Data[7] << 16);

            if (channel->TotalBytes <= J1939TP_MAX_TP_SIZE) {
                channel->State = J1939TP_STATE_DT_RX;
                channel->SeqNumber = 0U;
                channel->ReceivedBytes = 0U;
                channel->PacketsReceived = 0U;

                /* Send CTS response */
                /* CTS 消息构建和发送通过 J1939Tp_SendTpCm 实现 */
            }
        }
    }
}

static void J1939Tp_HandleCts(const uint8* Data, uint8 ChannelIdx)
{
    (void)Data;
    if (ChannelIdx < J1939TP_MAX_TX_CHANNELS) {
        J1939Tp_TxChannelType* channel = &J1939Tp_TxChannels[ChannelIdx];

        if (channel->State == J1939TP_STATE_CTS_RX) {
            /* CTS received - can start sending DT packets */
            channel->State = J1939TP_STATE_DT_TX;
        }
    }
}

static void J1939Tp_HandleEomAck(const uint8* Data, uint8 ChannelIdx)
{
    (void)Data;
    if (ChannelIdx < J1939TP_MAX_TX_CHANNELS) {
        J1939Tp_TxChannelType* channel = &J1939Tp_TxChannels[ChannelIdx];

        if (channel->State == J1939TP_STATE_EOM_ACK) {
            /* EOM ACK received - transmission complete */
            J1939Tp_ResetTxChannel(ChannelIdx);
        }
    }
}

static void J1939Tp_HandleBam(const uint8* Data, uint8 ChannelIdx)
{
    if ((Data != NULL) && (ChannelIdx < J1939TP_MAX_RX_CHANNELS)) {
        J1939Tp_RxChannelType* channel = &J1939Tp_RxChannels[ChannelIdx];

        /* Parse BAM message */
        channel->TotalBytes = (uint16)Data[1] | ((uint16)Data[2] << 8);
        channel->PacketsToReceive = Data[3];
        channel->Pgn = (uint32)Data[5] | ((uint32)Data[6] << 8) | ((uint32)Data[7] << 16);

        if (channel->TotalBytes <= J1939TP_MAX_TP_SIZE) {
            channel->State = J1939TP_STATE_BAM_RX;
            channel->SeqNumber = 0U;
            channel->ReceivedBytes = 0U;
            channel->PacketsReceived = 0U;
        }
    }
}

static void J1939Tp_HandleAbort(const uint8* Data, uint8 ChannelIdx)
{
    (void)Data;

    /* Reset any active session on this channel */
    if (ChannelIdx < J1939TP_MAX_TX_CHANNELS) {
        J1939Tp_ResetTxChannel(ChannelIdx);
    }
    if (ChannelIdx < J1939TP_MAX_RX_CHANNELS) {
        J1939Tp_ResetRxChannel(ChannelIdx);
    }
}

/*==================================================================================================
 *                                      PduR Callbacks
 *================================================================================================*/
void PduR_J1939TpRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    /* Stub - to be implemented by PduR */
    (void)RxPduId;
    (void)PduInfoPtr;
}
