/*
 * LinTp.c
 *
 *  Created on: May 5, 2026
 *      Author: YuleTech
 *
 *  AUTOSAR LinTp (LIN Transport Protocol) Implementation
 *  Based on ISO 17987-2 Transport Protocol
 *  Following AUTOSAR_SWS_LINTransportProtocol
 *
 *  Supports:
 *  - Single Frame (SF) transmission/reception
 *  - First Frame (FF) transmission/reception
 *  - Consecutive Frame (CF) transmission/reception
 *  - Flow Control (FC) with BlockSize and STmin
 *  - Segmentation and reassembly
 *  - Multi-channel support
 */

/*==================================================================================================
 *                                         INCLUDES
 *================================================================================================*/
#include "LinTp.h"
#include "PduR_LinTp.h"
#include "Det.h"
#include "LinIf.h"
#include <string.h>

/*==================================================================================================
 *                                         LOCAL MACROS
 *================================================================================================*/
/* PCI (Protocol Control Information) masks */
#define LINTP_PCI_TYPE_MASK                     0xF0U
#define LINTP_PCI_SF_DL_MASK                    0x0FU
#define LINTP_PCI_CF_SN_MASK                    0x0FU
#define LINTP_PCI_FC_FS_MASK                    0x0FU

/* Extract PCI information */
#define LINTP_GET_PCI_TYPE(byte)                ((byte) & LINTP_PCI_TYPE_MASK)
#define LINTP_GET_SF_DL(byte)                   ((byte) & LINTP_PCI_SF_DL_MASK)
#define LINTP_GET_CF_SN(byte)                   ((byte) & LINTP_PCI_CF_SN_MASK)
#define LINTP_GET_FC_FS(byte)                   ((byte) & LINTP_PCI_FC_FS_MASK)

/* Build PCI bytes */
#define LINTP_BUILD_PCI_SF(dl)                  (0x00U | ((dl) & 0x0FU))
#define LINTP_BUILD_PCI_FF(dl_high, dl_low)     (0x10U | ((dl_high) & 0x0FU)), (dl_low)
#define LINTP_BUILD_PCI_CF(sn)                  (0x20U | ((sn) & 0x0FU))
#define LINTP_BUILD_PCI_FC(fs)                  (0x30U | ((fs) & 0x0FU))

/* Timeout decrementation value per MainFunction call (assuming 1ms cycle) */
#define LINTP_TIMEOUT_DECREMENT                 1U

/*==================================================================================================
 *                                         LOCAL TYPES
 *================================================================================================*/
typedef struct
{
    LinTp_StateType             ModuleState;
    LinTp_ChannelRuntimeType    ChannelRuntime[LINTP_MAX_CHANNEL_COUNT];
    const LinTp_ConfigType      *ConfigPtr;
} LinTp_InternalType;

/*==================================================================================================
 *                                         LOCAL VARIABLES
 *================================================================================================*/
#define LINTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static LinTp_InternalType LinTp_Internal;

/* Static buffers for data transfer */
static uint8 LinTp_TxBuffer[LINTP_MAX_CHANNEL_COUNT][LINTP_BUFFER_SIZE];
static uint8 LinTp_RxBuffer[LINTP_MAX_CHANNEL_COUNT][LINTP_BUFFER_SIZE];

#define LINTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
 *                                         LOCAL FUNCTION PROTOTYPES
 *================================================================================================*/
static uint8 LinTp_GetChannelIdxFromNsdu(PduIdType NsduId);
static uint8 LinTp_GetNsduIdx(PduIdType NsduId);
static void LinTp_ResetChannel(uint8 ChannelIdx);
static void LinTp_SendSingleFrame(uint8 ChannelIdx);
static void LinTp_SendFirstFrame(uint8 ChannelIdx);
static void LinTp_SendConsecutiveFrame(uint8 ChannelIdx);
static PduLengthType LinTp_CalculatePayloadLength(PduLengthType DataLength, LinTp_PciType PciType);

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *================================================================================================*/

/*==================================================================================================
 *  FUNCTION: LinTp_Init
 *  Purpose: Initialize the LinTp module
 *================================================================================================*/
void LinTp_Init(const LinTp_ConfigType *ConfigPtr)
{
    uint8 channelIdx;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_INIT, LINTP_E_PARAM_POINTER);
        return;
    }
#endif

    LinTp_Internal.ConfigPtr = ConfigPtr;

    /* Initialize all channels */
    for (channelIdx = 0U; channelIdx < LINTP_MAX_CHANNEL_COUNT; channelIdx++)
    {
        LinTp_ResetChannel(channelIdx);
        LinTp_Internal.ChannelRuntime[channelIdx].TxBuffer = LinTp_TxBuffer[channelIdx];
        LinTp_Internal.ChannelRuntime[channelIdx].RxBuffer = LinTp_RxBuffer[channelIdx];
    }

    LinTp_Internal.ModuleState = LINTP_STATE_INIT;
}

/*==================================================================================================
 *  FUNCTION: LinTp_DeInit
 *  Purpose: De-initialize the LinTp module
 *================================================================================================*/
void LinTp_DeInit(void)
{
    uint8 channelIdx;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_DEINIT, LINTP_E_UNINIT);
        return;
    }
#endif

    /* Reset all channels */
    for (channelIdx = 0U; channelIdx < LINTP_MAX_CHANNEL_COUNT; channelIdx++)
    {
        LinTp_ResetChannel(channelIdx);
    }

    LinTp_Internal.ConfigPtr = NULL_PTR;
    LinTp_Internal.ModuleState = LINTP_STATE_UNINIT;
}

/*==================================================================================================
 *  FUNCTION: LinTp_Transmit
 *  Purpose: Request transmission of a TP message
 *================================================================================================*/
Std_ReturnType LinTp_Transmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    uint8 channelIdx;
    uint8 nsduIdx;
    LinTp_ChannelRuntimeType *channelPtr;
    const LinTp_ChannelConfigType *chConfigPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRANSMIT, LINTP_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRANSMIT, LINTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (TxPduId >= LINTP_NSDU_COUNT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRANSMIT, LINTP_E_INVALID_TX_ID);
        return E_NOT_OK;
    }
#endif

    /* Find the channel for this N-SDU */
    channelIdx = LinTp_GetChannelIdxFromNsdu(TxPduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRANSMIT, LINTP_E_INVALID_TX_ID);
#endif
        return E_NOT_OK;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];
    chConfigPtr = &LinTp_Internal.ConfigPtr->ChannelConfig[channelIdx];

    /* Check if channel is available */
    if (channelPtr->State != LINTP_CH_IDLE)
    {
        /* Channel busy - cannot transmit */
        return E_NOT_OK;
    }

    /* Get N-SDU index */
    nsduIdx = LinTp_GetNsduIdx(TxPduId);
    if (nsduIdx >= LINTP_NSDU_COUNT)
    {
        return E_NOT_OK;
    }

    /* Check if this is a Tx N-SDU */
    if (!LinTp_Internal.ConfigPtr->NsduConfig[nsduIdx].IsTx)
    {
        return E_NOT_OK;
    }

    /* Store transmission parameters */
    channelPtr->CurrentNsduId = TxPduId;
    channelPtr->DataLength = PduInfoPtr->SduLength;
    channelPtr->DataIndex = 0U;
    channelPtr->BlockCount = 0U;
    channelPtr->WftCount = 0U;
    channelPtr->BufferProvided = FALSE;

    /* Determine frame type based on data length */
    if (PduInfoPtr->SduLength <= LINTP_SF_MAX_DATA_LENGTH)
    {
        /* Single Frame transmission */
        channelPtr->State = LINTP_CH_TX_ACTIVE;
        channelPtr->TxState = LINTP_TX_SF;
    }
    else
    {
        /* Multi-frame transmission */
        channelPtr->State = LINTP_CH_TX_ACTIVE;
        channelPtr->TxState = LINTP_TX_FF;
        channelPtr->BlockSize = chConfigPtr->DefaultBs;
        channelPtr->StMin = chConfigPtr->DefaultStMin;
    }

    /* Set timeout */
    channelPtr->TimeoutCounter = chConfigPtr->N_As;

    return E_OK;
}

/*==================================================================================================
 *  FUNCTION: LinTp_CancelTransmit
 *  Purpose: Cancel an ongoing transmission
 *================================================================================================*/
#if (LINTP_CANCEL_TRANSMIT_API == STD_ON)
Std_ReturnType LinTp_CancelTransmit(PduIdType TxPduId)
{
    uint8 channelIdx;
    const LinTp_ChannelRuntimeType *channelPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CANCEL_TRANSMIT, LINTP_E_UNINIT);
        return E_NOT_OK;
    }

    if (TxPduId >= LINTP_NSDU_COUNT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CANCEL_TRANSMIT, LINTP_E_INVALID_TX_ID);
        return E_NOT_OK;
    }
#endif

    channelIdx = LinTp_GetChannelIdxFromNsdu(TxPduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
        return E_NOT_OK;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];

    /* Check if channel is transmitting the specified N-SDU */
    if ((channelPtr->State != LINTP_CH_TX_ACTIVE) || (channelPtr->CurrentNsduId != TxPduId))
    {
        return E_NOT_OK;
    }

    /* Check if cancellation is allowed */
    if (!LinTp_Internal.ConfigPtr->ChannelConfig[channelIdx].TransmitCancellation)
    {
        return E_NOT_OK;
    }

    /* Abort transmission */
    LinTp_AbortTransmission(channelIdx);

    return E_OK;
}
#endif

/*==================================================================================================
 *  FUNCTION: LinTp_CancelReceive
 *  Purpose: Cancel an ongoing reception
 *================================================================================================*/
#if (LINTP_CANCEL_RECEIVE_API == STD_ON)
Std_ReturnType LinTp_CancelReceive(PduIdType RxPduId)
{
    uint8 channelIdx;
    const LinTp_ChannelRuntimeType *channelPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CANCEL_RECEIVE, LINTP_E_UNINIT);
        return E_NOT_OK;
    }

    if (RxPduId >= LINTP_NSDU_COUNT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CANCEL_RECEIVE, LINTP_E_INVALID_RX_ID);
        return E_NOT_OK;
    }
#endif

    channelIdx = LinTp_GetChannelIdxFromNsdu(RxPduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
        return E_NOT_OK;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];

    /* Check if channel is receiving the specified N-SDU */
    if ((channelPtr->State != LINTP_CH_RX_ACTIVE) || (channelPtr->CurrentNsduId != RxPduId))
    {
        return E_NOT_OK;
    }

    /* Abort reception */
    LinTp_AbortReception(channelIdx);

    return E_OK;
}
#endif

/*==================================================================================================
 *  FUNCTION: LinTp_ChangeParameter
 *  Purpose: Change protocol parameters (BS or STmin)
 *================================================================================================*/
#if (LINTP_CHANGE_PARAMETER_API == STD_ON)
Std_ReturnType LinTp_ChangeParameter(PduIdType PduId, LinTp_ParameterType Parameter, uint16 Value)
{
    uint8 channelIdx;
    LinTp_ChannelRuntimeType *channelPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CHANGE_PARAMETER, LINTP_E_UNINIT);
        return E_NOT_OK;
    }

    if (PduId >= LINTP_NSDU_COUNT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CHANGE_PARAMETER, LINTP_E_INVALD_NSDU_ID);
        return E_NOT_OK;
    }

    if ((Parameter != LINTP_PARAMETER_BS) && (Parameter != LINTP_PARAMETER_STMIN))
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_CHANGE_PARAMETER, LINTP_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif

    channelIdx = LinTp_GetChannelIdxFromNsdu(PduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
        return E_NOT_OK;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];

    /* Can only change parameters when channel is idle */
    if (channelPtr->State != LINTP_CH_IDLE)
    {
        return E_NOT_OK;
    }

    /* Apply parameter change */
    if (Parameter == LINTP_PARAMETER_BS)
    {
        if (Value > 255U)
        {
            return E_NOT_OK;
        }
        channelPtr->BlockSize = (uint8)Value;
    }
    else /* LINTP_PARAMETER_STMIN */
    {
        if (Value > 255U)
        {
            return E_NOT_OK;
        }
        channelPtr->StMin = (uint8)Value;
    }

    return E_OK;
}
#endif

/*==================================================================================================
 *  FUNCTION: LinTp_GetVersionInfo
 *  Purpose: Get version information of the module
 *================================================================================================*/
#if (LINTP_VERSION_INFO_API == STD_ON)
void LinTp_GetVersionInfo(Std_VersionInfoType *VersionInfo)
{
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL_PTR)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_GET_VERSION_INFO, LINTP_E_PARAM_POINTER);
        return;
    }
#endif

    VersionInfo->vendorID = LINTP_VENDOR_ID;
    VersionInfo->moduleID = LINTP_MODULE_ID;
    VersionInfo->sw_major_version = LINTP_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = LINTP_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = LINTP_SW_PATCH_VERSION;
}
#endif

/*==================================================================================================
 *  FUNCTION: LinTp_MainFunction
 *  Purpose: Cyclic processing of the TP state machines
 *================================================================================================*/
void LinTp_MainFunction(void)
{
    uint8 channelIdx;

    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        return;
    }

    /* Process all channels */
    for (channelIdx = 0U; channelIdx < LINTP_MAX_CHANNEL_COUNT; channelIdx++)
    {
        LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];

        /* Process timeouts */
        if (channelPtr->TimeoutCounter > 0U)
        {
            channelPtr->TimeoutCounter--;

            if (channelPtr->TimeoutCounter == 0U)
            {
                /* Timeout occurred */
                if (channelPtr->State == LINTP_CH_TX_ACTIVE)
                {
                    LinTp_AbortTransmission(channelIdx);
                }
                else if (channelPtr->State == LINTP_CH_RX_ACTIVE)
                {
                    LinTp_AbortReception(channelIdx);
                }
                continue;
            }
        }

        /* Process transmission state machine */
        if (channelPtr->State == LINTP_CH_TX_ACTIVE)
        {
            LinTp_ProcessTxStateMachine(channelIdx);
        }

        /* Process reception state machine */
        if (channelPtr->State == LINTP_CH_RX_ACTIVE)
        {
            LinTp_ProcessRxStateMachine(channelIdx);
        }
    }
}

/*==================================================================================================
 *                                         CALLBACK FUNCTIONS
 *================================================================================================*/

/*==================================================================================================
 *  FUNCTION: LinTp_RxIndication
 *  Purpose: Called by LinIf when a LIN frame is received
 *================================================================================================*/
void LinTp_RxIndication(PduIdType RxPduId, const PduInfoType *PduInfoPtr)
{
    uint8 channelIdx;
    uint8 nsduIdx;
    uint8 pciByte;
    uint8 pciType;
    const LinTp_ChannelRuntimeType *channelPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_UNINIT);
        return;
    }

    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_PARAM_POINTER);
        return;
    }

    if (PduInfoPtr->SduDataPtr == NULL_PTR)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_PARAM_POINTER);
        return;
    }

    if (PduInfoPtr->SduLength == 0U)
    {
        return; /* Empty frame - ignore */
    }
#endif

    /* Find channel associated with this PDU */
    channelIdx = LinTp_GetChannelIdxFromNsdu(RxPduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
        return;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];
    nsduIdx = LinTp_GetNsduIdx(RxPduId);

    /* Get PCI type from first byte */
    pciByte = PduInfoPtr->SduDataPtr[0];
    pciType = LINTP_GET_PCI_TYPE(pciByte);

    switch (pciType)
    {
        case LINTP_PCI_SF:
            /* Single Frame */
            LinTp_ProcessSingleFrame(channelIdx, PduInfoPtr);
            break;

        case LINTP_PCI_FF:
            /* First Frame */
            LinTp_ProcessFirstFrame(channelIdx, PduInfoPtr);
            break;

        case LINTP_PCI_CF:
            /* Consecutive Frame */
            LinTp_ProcessConsecutiveFrame(channelIdx, PduInfoPtr);
            break;

        case LINTP_PCI_FC:
            /* Flow Control */
            LinTp_ProcessFlowControl(channelIdx, PduInfoPtr);
            break;

        default:
            /* Unknown PCI type - ignore */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
            Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_INVALID_FRAME);
#endif
            break;
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_TxConfirmation
 *  Purpose: Called by LinIf when a LIN frame transmission is confirmed
 *================================================================================================*/
void LinTp_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    uint8 channelIdx;
    LinTp_ChannelRuntimeType *channelPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (LinTp_Internal.ModuleState != LINTP_STATE_INIT)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TX_CONFIRMATION, LINTP_E_UNINIT);
        return;
    }
#endif

    channelIdx = LinTp_GetChannelIdxFromNsdu(TxPduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
        return;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];

    if (result != E_OK)
    {
        /* Transmission failed */
        if (channelPtr->State == LINTP_CH_TX_ACTIVE)
        {
            LinTp_AbortTransmission(channelIdx);
        }
        return;
    }

    /* Handle successful transmission based on current state */
    if (channelPtr->State == LINTP_CH_TX_ACTIVE)
    {
        switch (channelPtr->TxState)
        {
            case LINTP_TX_SF:
                /* Single Frame completed */
                channelPtr->TxState = LINTP_TX_COMPLETED;
                PduR_LinTpTxConfirmation(channelPtr->CurrentNsduId, E_OK);
                LinTp_ResetChannel(channelIdx);
                break;

            case LINTP_TX_FF:
                /* First Frame completed - wait for FC */
                channelPtr->TxState = LINTP_TX_WAIT_FC;
                channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[channelIdx].N_Cr;
                break;

            case LINTP_TX_CF:
                /* Consecutive Frame completed */
                channelPtr->DataIndex += LinTp_CalculatePayloadLength(
                    channelPtr->DataLength - channelPtr->DataIndex, LINTP_PCI_CF);
                channelPtr->BlockCount++;

                if (channelPtr->DataIndex >= channelPtr->DataLength)
                {
                    /* All data transmitted */
                    channelPtr->TxState = LINTP_TX_COMPLETED;
                    PduR_LinTpTxConfirmation(channelPtr->CurrentNsduId, E_OK);
                    LinTp_ResetChannel(channelIdx);
                }
                else
                {
                    /* More CFs to send */
                    if (channelPtr->BlockCount >= channelPtr->BlockSize)
                    {
                        /* Block complete - wait for next FC */
                        channelPtr->TxState = LINTP_TX_WAIT_FC;
                        channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[channelIdx].N_Cr;
                    }
                    else
                    {
                        /* Continue with next CF */
                        channelPtr->SequenceNumber = (channelPtr->SequenceNumber + 1U) & 0x0FU;
                    }
                }
                break;

            case LINTP_TX_WAIT_TX_CONFIRM:
                /* Flow Control transmitted - update state */
                if (channelPtr->State == LINTP_CH_RX_ACTIVE)
                {
                    channelPtr->RxState = LINTP_RX_WAIT_CF;
                    channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[channelIdx].N_Cr;
                }
                break;

            default:
                /* Do nothing */
                break;
        }
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_TriggerTransmit
 *  Purpose: Called by LinIf to get data for transmission
 *================================================================================================*/
Std_ReturnType LinTp_TriggerTransmit(PduIdType TxPduId, PduInfoType *PduInfoPtr)
{
    uint8 channelIdx;
    LinTp_ChannelRuntimeType *channelPtr;

#if (LINTP_DEV_ERROR_DETECT == STD_ON)
    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(LINTP_MODULE_ID, 0U, LINTP_SID_TRIGGER_TRANSMIT, LINTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    channelIdx = LinTp_GetChannelIdxFromNsdu(TxPduId);
    if (channelIdx >= LINTP_MAX_CHANNEL_COUNT)
    {
        return E_NOT_OK;
    }

    channelPtr = &LinTp_Internal.ChannelRuntime[channelIdx];

    /* Return data based on current transmission state */
    if (channelPtr->State == LINTP_CH_TX_ACTIVE)
    {
        switch (channelPtr->TxState)
        {
            case LINTP_TX_SF:
                /* Copy SF data */
                if (PduInfoPtr->SduLength >= (PduLengthType)(channelPtr->DataLength + 1U))
                {
                    PduInfoPtr->SduDataPtr[0] = LINTP_BUILD_PCI_SF(channelPtr->DataLength);
                    memcpy(&PduInfoPtr->SduDataPtr[1], channelPtr->TxBuffer, channelPtr->DataLength);
                    return E_OK;
                }
                break;

            case LINTP_TX_FF:
                /* Copy FF data */
                if (PduInfoPtr->SduLength >= 8U)
                {
                    PduInfoPtr->SduDataPtr[0] = LINTP_BUILD_PCI_FF(
                        (uint8)((channelPtr->DataLength >> 8) & 0x0FU),
                        (uint8)(channelPtr->DataLength & 0xFFU));
                    memcpy(&PduInfoPtr->SduDataPtr[1], channelPtr->TxBuffer, LINTP_FF_MAX_DATA_LENGTH);
                    channelPtr->DataIndex = LINTP_FF_MAX_DATA_LENGTH;
                    return E_OK;
                }
                break;

            case LINTP_TX_CF:
                /* Copy CF data */
                if (PduInfoPtr->SduLength >= 8U)
                {
                    PduLengthType remaining = channelPtr->DataLength - channelPtr->DataIndex;
                    PduLengthType payloadLen = (remaining > LINTP_CF_MAX_DATA_LENGTH) ? 
                                               LINTP_CF_MAX_DATA_LENGTH : remaining;

                    PduInfoPtr->SduDataPtr[0] = LINTP_BUILD_PCI_CF(channelPtr->SequenceNumber);
                    memcpy(&PduInfoPtr->SduDataPtr[1], &channelPtr->TxBuffer[channelPtr->DataIndex], payloadLen);
                    return E_OK;
                }
                break;

            default:
                break;
        }
    }
    else if (channelPtr->State == LINTP_CH_RX_ACTIVE)
    {
        if (channelPtr->RxState == LINTP_RX_SEND_FC)
        {
            /* Copy FC data */
            if (PduInfoPtr->SduLength >= 3U)
            {
                PduInfoPtr->SduDataPtr[0] = LINTP_BUILD_PCI_FC(LINTP_FC_CTS);
                PduInfoPtr->SduDataPtr[1] = channelPtr->BlockSize;
                PduInfoPtr->SduDataPtr[2] = channelPtr->StMin;
                channelPtr->RxState = LINTP_RX_WAIT_CF;
                return E_OK;
            }
        }
    }

    return E_NOT_OK;
}

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *================================================================================================*/

/*==================================================================================================
 *  FUNCTION: LinTp_GetChannelIdxFromNsdu
 *  Purpose: Get channel index from N-SDU ID
 *================================================================================================*/
static uint8 LinTp_GetChannelIdxFromNsdu(PduIdType NsduId)
{
    uint8 idx;

    for (idx = 0U; idx < LINTP_NSDU_COUNT; idx++)
    {
        if (LinTp_Internal.ConfigPtr->NsduConfig[idx].NsduId == NsduId)
        {
            return (uint8)LinTp_Internal.ConfigPtr->NsduConfig[idx].ChannelId;
        }
    }

    return 0xFFU; /* Invalid channel */
}

/*==================================================================================================
 *  FUNCTION: LinTp_GetNsduIdx
 *  Purpose: Get N-SDU configuration index from N-SDU ID
 *================================================================================================*/
static uint8 LinTp_GetNsduIdx(PduIdType NsduId)
{
    uint8 idx;

    for (idx = 0U; idx < LINTP_NSDU_COUNT; idx++)
    {
        if (LinTp_Internal.ConfigPtr->NsduConfig[idx].NsduId == NsduId)
        {
            return idx;
        }
    }

    return 0xFFU; /* Invalid */
}

/*==================================================================================================
 *  FUNCTION: LinTp_ResetChannel
 *  Purpose: Reset a channel to idle state
 *================================================================================================*/
static void LinTp_ResetChannel(uint8 ChannelIdx)
{
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    channelPtr->State = LINTP_CH_IDLE;
    channelPtr->TxState = LINTP_TX_IDLE;
    channelPtr->RxState = LINTP_RX_IDLE;
    channelPtr->CurrentNsduId = 0xFFFFU;
    channelPtr->DataLength = 0U;
    channelPtr->DataIndex = 0U;
    channelPtr->BufferSize = 0U;
    channelPtr->SequenceNumber = 0U;
    channelPtr->BlockSize = LINTP_BS_DEFAULT;
    channelPtr->BlockCount = 0U;
    channelPtr->StMin = LINTP_STMIN_DEFAULT;
    channelPtr->WftCount = 0U;
    channelPtr->TimeoutCounter = 0U;
    channelPtr->BufferProvided = FALSE;
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProcessTxStateMachine
 *  Purpose: Process transmission state machine
 *================================================================================================*/
static void LinTp_ProcessTxStateMachine(uint8 ChannelIdx)
{
    const LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    switch (channelPtr->TxState)
    {
        case LINTP_TX_SF:
            /* Request copy of Tx data */
            if (LinTp_CopyTxData(ChannelIdx) == E_OK)
            {
                /* Data copied - transmission will be triggered via TriggerTransmit */
            }
            break;

        case LINTP_TX_FF:
            /* Request copy of Tx data */
            if (LinTp_CopyTxData(ChannelIdx) == E_OK)
            {
                /* First frame will be transmitted - wait for confirmation */
            }
            break;

        case LINTP_TX_CF:
            /* Continue transmitting consecutive frames */
            if (channelPtr->BlockCount < channelPtr->BlockSize)
            {
                /* Next CF can be sent immediately or after STmin delay */
                /* For LIN, we rely on the schedule table timing */
            }
            break;

        case LINTP_TX_WAIT_FC:
            /* Waiting for flow control - handled in RxIndication */
            break;

        case LINTP_TX_WAIT_TX_CONFIRM:
            /* Waiting for Tx confirmation */
            break;

        default:
            break;
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProcessRxStateMachine
 *  Purpose: Process reception state machine
 *================================================================================================*/
static void LinTp_ProcessRxStateMachine(uint8 ChannelIdx)
{
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    switch (channelPtr->RxState)
    {
        case LINTP_RX_WAIT_FF:
            /* Waiting for First Frame */
            break;

        case LINTP_RX_WAIT_CF:
            /* Waiting for Consecutive Frame */
            break;

        case LINTP_RX_SEND_FC:
            /* Need to send Flow Control */
            /* Transmission is triggered via TriggerTransmit */
            break;

        case LINTP_RX_COMPLETED:
            /* Reception completed - forward to PduR */
            PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_OK);
            LinTp_ResetChannel(ChannelIdx);
            break;

        case LINTP_RX_ERROR:
            /* Error in reception */
            PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);
            LinTp_ResetChannel(ChannelIdx);
            break;

        default:
            break;
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProcessSingleFrame
 *  Purpose: Process received Single Frame
 *================================================================================================*/
static void LinTp_ProcessSingleFrame(uint8 ChannelIdx, const PduInfoType *PduInfoPtr)
{
    uint8 dataLength;
    PduInfoType pduRInfo;
    BufReq_ReturnType bufStatus;
    PduLengthType bufferSize;
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* Extract data length from PCI */
    dataLength = LINTP_GET_SF_DL(PduInfoPtr->SduDataPtr[0]);

    if (dataLength == 0U)
    {
        /* Invalid SF */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_INVALID_FRAME);
#endif
        return;
    }

    if (dataLength > LINTP_SF_MAX_DATA_LENGTH)
    {
        /* Data length exceeds SF capacity */
        return;
    }

    /* Check if channel is available */
    if (channelPtr->State != LINTP_CH_IDLE)
    {
        /* Channel busy - reject SF */
        return;
    }

    /* Start reception */
    channelPtr->State = LINTP_CH_RX_ACTIVE;
    channelPtr->RxState = LINTP_RX_WAIT_FF; /* Actually receiving SF */
    channelPtr->DataLength = dataLength;
    channelPtr->DataIndex = 0U;

    /* Request buffer from PduR */
    pduRInfo.SduLength = dataLength;
    pduRInfo.SduDataPtr = NULL_PTR;
    bufStatus = PduR_LinTpStartOfReception(channelPtr->CurrentNsduId, &pduRInfo, dataLength, &bufferSize);

    if (bufStatus == BUFREQ_OK)
    {
        /* Copy received data to PduR */
        pduRInfo.SduLength = dataLength;
        pduRInfo.SduDataPtr = &PduInfoPtr->SduDataPtr[1];
        bufStatus = PduR_LinTpCopyRxData(channelPtr->CurrentNsduId, &pduRInfo, &bufferSize);

        if (bufStatus == BUFREQ_OK)
        {
            /* Reception complete */
            channelPtr->RxState = LINTP_RX_COMPLETED;
            PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_OK);
            LinTp_ResetChannel(ChannelIdx);
        }
        else
        {
            /* Buffer error */
            PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);
            LinTp_ResetChannel(ChannelIdx);
        }
    }
    else
    {
        /* No buffer available */
        PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);
        LinTp_ResetChannel(ChannelIdx);
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProcessFirstFrame
 *  Purpose: Process received First Frame
 *================================================================================================*/
static void LinTp_ProcessFirstFrame(uint8 ChannelIdx, const PduInfoType *PduInfoPtr)
{
    uint16 dataLength;
    PduInfoType pduRInfo;
    BufReq_ReturnType bufStatus;
    PduLengthType bufferSize;
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* Extract data length from PCI (12-bit value) */
    dataLength = ((uint16)(PduInfoPtr->SduDataPtr[0] & 0x0FU) << 8) | PduInfoPtr->SduDataPtr[1];

    if (dataLength <= LINTP_SF_MAX_DATA_LENGTH)
    {
        /* Should have been sent as SF */
        return;
    }

    if (dataLength > LINTP_BUFFER_SIZE)
    {
        /* Data too large for buffer - send FC.OVFLW */
        LinTp_SendFlowControl(ChannelIdx, LINTP_FC_OVFLW);
        return;
    }

    /* Check if channel is available */
    if (channelPtr->State != LINTP_CH_IDLE)
    {
        /* Channel busy - reject FF */
        LinTp_SendFlowControl(ChannelIdx, LINTP_FC_OVFLW);
        return;
    }

    /* Start multi-frame reception */
    channelPtr->State = LINTP_CH_RX_ACTIVE;
    channelPtr->RxState = LINTP_RX_SEND_FC;
    channelPtr->DataLength = dataLength;
    channelPtr->DataIndex = LINTP_FF_MAX_DATA_LENGTH;
    channelPtr->SequenceNumber = 1U; /* Expect SN=1 in first CF */
    channelPtr->BlockCount = 0U;

    /* Copy FF payload to local buffer */
    memcpy(channelPtr->RxBuffer, &PduInfoPtr->SduDataPtr[2], LINTP_FF_MAX_DATA_LENGTH);

    /* Request buffer from PduR */
    pduRInfo.SduLength = LINTP_FF_MAX_DATA_LENGTH;
    pduRInfo.SduDataPtr = channelPtr->RxBuffer;
    bufStatus = PduR_LinTpStartOfReception(channelPtr->CurrentNsduId, &pduRInfo, dataLength, &bufferSize);

    if (bufStatus == BUFREQ_OK)
    {
        /* Copy FF data to PduR */
        pduRInfo.SduLength = LINTP_FF_MAX_DATA_LENGTH;
        pduRInfo.SduDataPtr = channelPtr->RxBuffer;
        bufStatus = PduR_LinTpCopyRxData(channelPtr->CurrentNsduId, &pduRInfo, &bufferSize);

        if (bufStatus == BUFREQ_OK)
        {
            /* Send FC.CTS to continue reception */
            channelPtr->BufferProvided = TRUE;
            LinTp_SendFlowControl(ChannelIdx, LINTP_FC_CTS);
            channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[ChannelIdx].N_Cr;
        }
        else
        {
            /* Buffer error - abort */
            PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);
            LinTp_SendFlowControl(ChannelIdx, LINTP_FC_OVFLW);
            LinTp_ResetChannel(ChannelIdx);
        }
    }
    else
    {
        /* No buffer available */
        LinTp_SendFlowControl(ChannelIdx, LINTP_FC_OVFLW);
        LinTp_ResetChannel(ChannelIdx);
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProcessConsecutiveFrame
 *  Purpose: Process received Consecutive Frame
 *================================================================================================*/
static void LinTp_ProcessConsecutiveFrame(uint8 ChannelIdx, const PduInfoType *PduInfoPtr)
{
    uint8 sequenceNumber;
    PduLengthType payloadLength;
    PduLengthType bytesToCopy;
    PduInfoType pduRInfo;
    BufReq_ReturnType bufStatus;
    PduLengthType bufferSize;
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* Check if we're expecting CF */
    if ((channelPtr->State != LINTP_CH_RX_ACTIVE) || 
        (channelPtr->RxState != LINTP_RX_WAIT_CF))
    {
        /* Unexpected CF */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_SEQUENCE_ERROR);
#endif
        return;
    }

    /* Extract sequence number */
    sequenceNumber = LINTP_GET_CF_SN(PduInfoPtr->SduDataPtr[0]);

    /* Check sequence number */
    if (sequenceNumber != channelPtr->SequenceNumber)
    {
        /* Sequence error */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_SEQUENCE_ERROR);
#endif
        PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);
        LinTp_ResetChannel(ChannelIdx);
        return;
    }

    /* Calculate payload length */
    bytesToCopy = channelPtr->DataLength - channelPtr->DataIndex;
    payloadLength = (bytesToCopy > LINTP_CF_MAX_DATA_LENGTH) ? LINTP_CF_MAX_DATA_LENGTH : bytesToCopy;

    /* Copy CF payload to local buffer */
    memcpy(&channelPtr->RxBuffer[channelPtr->DataIndex], &PduInfoPtr->SduDataPtr[1], payloadLength);

    /* Update index and sequence number */
    channelPtr->DataIndex += payloadLength;
    channelPtr->SequenceNumber = (channelPtr->SequenceNumber + 1U) & 0x0FU;
    channelPtr->BlockCount++;

    /* Copy data to PduR */
    pduRInfo.SduLength = payloadLength;
    pduRInfo.SduDataPtr = &channelPtr->RxBuffer[channelPtr->DataIndex - payloadLength];
    bufStatus = PduR_LinTpCopyRxData(channelPtr->CurrentNsduId, &pduRInfo, &bufferSize);

    if (bufStatus != BUFREQ_OK)
    {
        /* Buffer error */
        PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);
        LinTp_ResetChannel(ChannelIdx);
        return;
    }

    /* Check if reception is complete */
    if (channelPtr->DataIndex >= channelPtr->DataLength)
    {
        /* All data received */
        channelPtr->RxState = LINTP_RX_COMPLETED;
        PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_OK);
        LinTp_ResetChannel(ChannelIdx);
    }
    else
    {
        /* More CFs expected */
        /* Reset timeout */
        channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[ChannelIdx].N_Cr;

        /* Check if we need to send another FC */
        if (channelPtr->BlockCount >= channelPtr->BlockSize)
        {
            /* Send next FC.CTS */
            channelPtr->BlockCount = 0U;
            channelPtr->RxState = LINTP_RX_SEND_FC;
            LinTp_SendFlowControl(ChannelIdx, LINTP_FC_CTS);
        }
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProcessFlowControl
 *  Purpose: Process received Flow Control frame
 *================================================================================================*/
static void LinTp_ProcessFlowControl(uint8 ChannelIdx, const PduInfoType *PduInfoPtr)
{
    uint8 flowStatus;
    uint8 blockSize;
    uint8 stMin;
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* Check if we're waiting for FC */
    if ((channelPtr->State != LINTP_CH_TX_ACTIVE) || 
        (channelPtr->TxState != LINTP_TX_WAIT_FC))
    {
        /* Unexpected FC */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_INVALID_FC);
#endif
        return;
    }

    /* Extract FC information */
    flowStatus = LINTP_GET_FC_FS(PduInfoPtr->SduDataPtr[0]);

    if (PduInfoPtr->SduLength >= 3U)
    {
        blockSize = PduInfoPtr->SduDataPtr[1];
        stMin = PduInfoPtr->SduDataPtr[2];
    }
    else
    {
        /* Invalid FC length */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
        Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_INVALID_FC);
#endif
        LinTp_AbortTransmission(ChannelIdx);
        return;
    }

    switch (flowStatus)
    {
        case LINTP_FC_CTS:
            /* Continue To Send - update parameters and start CF transmission */
            channelPtr->BlockSize = blockSize;
            channelPtr->StMin = stMin;
            channelPtr->BlockCount = 0U;
            channelPtr->TxState = LINTP_TX_CF;
            channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[ChannelIdx].N_Cs;
            channelPtr->WftCount = 0U;
            break;

        case LINTP_FC_WAIT:
            /* Wait - reset timeout and continue waiting */
            channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[ChannelIdx].N_Cr;
            channelPtr->WftCount++;

#if (LINTP_SUPPORT_WAIT_FRAMES == STD_ON)
            if (channelPtr->WftCount >= LINTP_MAX_WFT)
            {
                /* Too many Wait frames - abort */
                LinTp_AbortTransmission(ChannelIdx);
            }
#endif
            break;

        case LINTP_FC_OVFLW:
            /* Overflow - abort transmission */
            LinTp_AbortTransmission(ChannelIdx);
            break;

        default:
            /* Invalid flow status */
#if (LINTP_DEV_ERROR_DETECT == STD_ON)
            Det_ReportRuntimeError(LINTP_MODULE_ID, 0U, LINTP_SID_RX_INDICATION, LINTP_E_INVALID_FC);
#endif
            LinTp_AbortTransmission(ChannelIdx);
            break;
    }
}

/*==================================================================================================
 *  FUNCTION: LinTp_SendFlowControl
 *  Purpose: Send a Flow Control frame
 *================================================================================================*/
static void LinTp_SendFlowControl(uint8 ChannelIdx, LinTp_FlowStatusType FlowStatus)
{
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* FC transmission is handled via TriggerTransmit callback from LinIf */
    channelPtr->RxState = LINTP_RX_SEND_FC;
    channelPtr->TimeoutCounter = LinTp_Internal.ConfigPtr->ChannelConfig[ChannelIdx].N_As;
}

/*==================================================================================================
 *  FUNCTION: LinTp_CopyTxData
 *  Purpose: Request Tx data from PduR
 *================================================================================================*/
static Std_ReturnType LinTp_CopyTxData(uint8 ChannelIdx)
{
    PduInfoType pduInfo;
    RetryInfoType retryInfo;
    PduLengthType availableData;
    Std_ReturnType result = E_NOT_OK;
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    retryInfo.TpDataState = TP_CONFPENDING;
    retryInfo.BufSize = 0U;

    switch (channelPtr->TxState)
    {
        case LINTP_TX_SF:
            /* Copy SF data */
            pduInfo.SduLength = channelPtr->DataLength;
            pduInfo.SduDataPtr = channelPtr->TxBuffer;
            break;

        case LINTP_TX_FF:
            /* Copy FF data (first 5 bytes) */
            pduInfo.SduLength = LINTP_FF_MAX_DATA_LENGTH;
            pduInfo.SduDataPtr = channelPtr->TxBuffer;
            break;

        case LINTP_TX_CF:
            /* Copy CF data */
            pduInfo.SduLength = (channelPtr->DataLength - channelPtr->DataIndex);
            if (pduInfo.SduLength > LINTP_CF_MAX_DATA_LENGTH)
            {
                pduInfo.SduLength = LINTP_CF_MAX_DATA_LENGTH;
            }
            pduInfo.SduDataPtr = &channelPtr->TxBuffer[channelPtr->DataIndex];
            break;

        default:
            return E_NOT_OK;
    }

    /* Request data from PduR */
    result = PduR_LinTpCopyTxData(channelPtr->CurrentNsduId, &pduInfo, &retryInfo, &availableData);

    return result;
}

/*==================================================================================================
 *  FUNCTION: LinTp_ProvideRxBuffer
 *  Purpose: Provide buffer for reception
 *================================================================================================*/
static Std_ReturnType LinTp_ProvideRxBuffer(uint8 ChannelIdx)
{
    /* Buffer is statically allocated */
    return E_OK;
}

/*==================================================================================================
 *  FUNCTION: LinTp_AbortTransmission
 *  Purpose: Abort an ongoing transmission
 *================================================================================================*/
static void LinTp_AbortTransmission(uint8 ChannelIdx)
{
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* Notify PduR of transmission failure */
    PduR_LinTpTxConfirmation(channelPtr->CurrentNsduId, E_NOT_OK);

    /* Reset channel */
    LinTp_ResetChannel(ChannelIdx);
}

/*==================================================================================================
 *  FUNCTION: LinTp_AbortReception
 *  Purpose: Abort an ongoing reception
 *================================================================================================*/
static void LinTp_AbortReception(uint8 ChannelIdx)
{
    LinTp_ChannelRuntimeType *channelPtr = &LinTp_Internal.ChannelRuntime[ChannelIdx];

    /* Notify PduR of reception failure */
    PduR_LinTpRxIndication(channelPtr->CurrentNsduId, E_NOT_OK);

    /* Reset channel */
    LinTp_ResetChannel(ChannelIdx);
}

/*==================================================================================================
 *  FUNCTION: LinTp_CalculatePayloadLength
 *  Purpose: Calculate payload length based on PCI type and remaining data
 *================================================================================================*/
static PduLengthType LinTp_CalculatePayloadLength(PduLengthType DataLength, LinTp_PciType PciType)
{
    PduLengthType maxPayload;

    switch (PciType)
    {
        case LINTP_PCI_SF:
            maxPayload = LINTP_SF_MAX_DATA_LENGTH;
            break;
        case LINTP_PCI_FF:
            maxPayload = LINTP_FF_MAX_DATA_LENGTH;
            break;
        case LINTP_PCI_CF:
            maxPayload = LINTP_CF_MAX_DATA_LENGTH;
            break;
        default:
            maxPayload = 0U;
            break;
    }

    return (DataLength < maxPayload) ? DataLength : maxPayload;
}
