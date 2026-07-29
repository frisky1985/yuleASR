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
 * @file CanTp.c
 * @brief CAN Transport Protocol implementation (ISO 15765-2)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "CanTp.h"
#include "CanTp_Cfg.h"
#include "CanIf.h"
#include "PduR.h"
#include "Det.h"

#define CANTP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/* Channel state definitions */
typedef enum {
    CANTP_CH_IDLE = 0,
    CANTP_CH_TX_SF,         /* Transmitting Single Frame */
    CANTP_CH_TX_FF,         /* Transmitting First Frame */
    CANTP_CH_TX_CF,         /* Transmitting Consecutive Frames */
    CANTP_CH_RX_SF,         /* Receiving Single Frame */
    CANTP_CH_RX_FF,         /* Receiving First Frame */
    CANTP_CH_RX_CF,         /* Receiving Consecutive Frames */
    CANTP_CH_TX_WAIT_FC,    /* Waiting for Flow Control */
    CANTP_CH_RX_WAIT_FC     /* Waiting to send Flow Control */
} CanTp_ChannelStateType;

/* Channel runtime structure */
typedef struct {
    CanTp_ChannelStateType State;
    PduIdType ActiveNsduId;
    uint16 DataLength;
    uint16 DataIndex;
    uint8 SequenceNumber;
    uint8 BlockSize;
    uint8 STmin;
    uint8 WftCounter;
    uint16 Timer;
    uint8 Buffer[CANTP_CHANNEL_BUFFER_SIZE];       /* Temporary buffer for frame data */
    boolean TxConfirmed;
    boolean RxIndicated;
    const CanTp_TxNsduConfigType* TxNsduConfig;  /* Pointer to Tx NSDU config (for Tx operations) */
    const CanTp_RxNsduConfigType* RxNsduConfig;  /* Pointer to Rx NSDU config (for Rx operations) */
} CanTp_ChannelRuntimeType;

static boolean CanTp_Initialized = FALSE;
static const CanTp_ConfigType* CanTp_ConfigPtr = NULL_PTR;
static CanTp_ChannelRuntimeType CanTp_ChannelRuntime[CANTP_MAX_CHANNEL_CNT];

/* ISO-TP Frame Constants */
#define CANTP_PCI_TYPE_MASK             (0xF0U)
#define CANTP_PCI_TYPE_SF               (0x00U)  /* Single Frame */
#define CANTP_PCI_TYPE_FF               (0x10U)  /* First Frame */
#define CANTP_PCI_TYPE_CF               (0x20U)  /* Consecutive Frame */
#define CANTP_PCI_TYPE_FC               (0x30U)  /* Flow Control */
#define CANTP_PCI_SF_DL_MASK            (0x0FU)
#define CANTP_PCI_FF_DL_MASK            (0x0FU)
#define CANTP_PCI_CF_SN_MASK            (0x0FU)
#define CANTP_PCI_FC_FS_MASK            (0x0FU)
#define CANTP_MAX_SF_DATA_LEN           (7U)     /* 7 bytes for standard CAN */
#define CANTP_MAX_FF_DATA_LEN           (6U)     /* 6 bytes in First Frame */
#define CANTP_MAX_CF_DATA_LEN           (7U)     /* 7 bytes per Consecutive Frame */

#define CANTP_START_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    CONFIGURATION ACCESS HELPERS
*==================================================================================================*/
/**
 * @brief Get Tx NSDU configuration for a given Tx SDU ID
 * @param txSduId Tx SDU ID
 * @return Pointer to Tx NSDU configuration, or NULL_PTR if invalid
 */
static const CanTp_TxNsduConfigType* CanTp_GetTxNsduConfig(PduIdType txSduId)
{
    if (CanTp_ConfigPtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    /* Search through all channels for the matching Tx NSDU config */
    for (uint8 ch = 0U; ch < CanTp_ConfigPtr->NumChannels; ch++) {
        const CanTp_ChannelConfigType* chConfig = &CanTp_ConfigPtr->ChannelConfigs[ch];
        for (uint8 i = 0U; i < chConfig->NumTxNsdu; i++) {
            if (chConfig->TxNsduConfigs[i].CanTpTxNPduConfirmationId == txSduId) {
                return &chConfig->TxNsduConfigs[i];
            }
        }
    }
    return NULL_PTR;
}

/**
 * @brief Get Rx NSDU configuration for a given Rx SDU ID
 * @param rxSduId Rx SDU ID
 * @return Pointer to Rx NSDU configuration, or NULL_PTR if invalid
 */
static const CanTp_RxNsduConfigType* CanTp_GetRxNsduConfig(PduIdType rxSduId)
{
    if (CanTp_ConfigPtr == NULL_PTR) {
        return NULL_PTR;
    }
    
    /* Search through all channels for the matching Rx NSDU config */
    for (uint8 ch = 0U; ch < CanTp_ConfigPtr->NumChannels; ch++) {
        const CanTp_ChannelConfigType* chConfig = &CanTp_ConfigPtr->ChannelConfigs[ch];
        for (uint8 i = 0U; i < chConfig->NumRxNsdu; i++) {
            if (chConfig->RxNsduConfigs[i].CanTpRxNPduId == rxSduId) {
                return &chConfig->RxNsduConfigs[i];
            }
        }
    }
    return NULL_PTR;
}

/**
 * @brief CanTp_ResetChannel - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_ResetChannel function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
static void CanTp_ResetChannel(CanTp_ChannelType Channel)
{
    if (Channel < CANTP_MAX_CHANNEL_CNT) {
        CanTp_ChannelRuntime[Channel].State = CANTP_CH_IDLE;
        CanTp_ChannelRuntime[Channel].ActiveNsduId = CANTP_INVALID_CHANNEL_ID;
        CanTp_ChannelRuntime[Channel].DataLength = 0U;
        CanTp_ChannelRuntime[Channel].DataIndex = 0U;
        CanTp_ChannelRuntime[Channel].SequenceNumber = 0U;
        CanTp_ChannelRuntime[Channel].BlockSize = 0U;
        CanTp_ChannelRuntime[Channel].STmin = 0U;
        CanTp_ChannelRuntime[Channel].WftCounter = 0U;
        CanTp_ChannelRuntime[Channel].Timer = 0U;
        CanTp_ChannelRuntime[Channel].TxConfirmed = FALSE;
        CanTp_ChannelRuntime[Channel].RxIndicated = FALSE;
        CanTp_ChannelRuntime[Channel].TxNsduConfig = NULL_PTR;
        CanTp_ChannelRuntime[Channel].RxNsduConfig = NULL_PTR;
    }
}

static CanTp_ChannelType CanTp_FindFreeChannel(void)
{
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        if (CanTp_ChannelRuntime[i].State == CANTP_CH_IDLE) {
            return (CanTp_ChannelType)i;
        }
    }
    return CANTP_INVALID_CHANNEL_ID;  /* No free channel */
}

/**
 * @brief CanTp_SendFlowControl - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_SendFlowControl function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
static void CanTp_SendFlowControl(CanTp_ChannelType Channel, CanTp_FlowStatusType Fs, uint8 Bs, uint8 Stmin)
{
    uint8 fcFrame[CANTP_CAN_FRAME_LENGTH];

    fcFrame[0] = (uint8)(CANTP_PCI_TYPE_FC | (uint8)Fs);
    fcFrame[1] = Bs;
    fcFrame[2] = Stmin;

    /* Pad remaining bytes */
    for (uint8 i = 3U; i < CANTP_CAN_FRAME_LENGTH; i++) {
        fcFrame[i] = CANTP_PADDING_BYTE_VALUE;
    }

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = fcFrame;
    pduInfo.SduLength = CANTP_CAN_FRAME_LENGTH;
    pduInfo.MetaDataPtr = NULL_PTR;

    /* Send via CAN Interface */
    (void)CanIf_Transmit(CANTP_CANIF_FC_TX_PDU_ID, &pduInfo);
}

/**
 * @brief CanTp_SendSingleFrame - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_SendSingleFrame function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
static void CanTp_SendSingleFrame(PduIdType TxSduId, const uint8* Data, uint8 Length)
{
    uint8 sfFrame[CANTP_CAN_FRAME_LENGTH];

    sfFrame[0] = (uint8)(CANTP_PCI_TYPE_SF | (Length & CANTP_PCI_SF_DL_MASK));

    for (uint8 i = 0U; i < Length; i++) {
        sfFrame[i + 1U] = Data[i];
    }

    /* Pad remaining bytes */
    for (uint8 i = (Length + 1U); i < CANTP_CAN_FRAME_LENGTH; i++) {
        sfFrame[i] = CANTP_PADDING_BYTE_VALUE;
    }

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = sfFrame;
    pduInfo.SduLength = CANTP_CAN_FRAME_LENGTH;
    pduInfo.MetaDataPtr = NULL_PTR;

    (void)CanIf_Transmit(CANTP_CANIF_TX_PDU_ID, &pduInfo);
}

/**
 * @brief CanTp_SendFirstFrame - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_SendFirstFrame function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
static void CanTp_SendFirstFrame(CanTp_ChannelType Channel, uint16 MessageLength)
{
    uint8 ffFrame[CANTP_CAN_FRAME_LENGTH];

    ffFrame[0] = (uint8)(CANTP_PCI_TYPE_FF | ((MessageLength >> 8) & CANTP_PCI_FF_DL_MASK));
    ffFrame[1] = (uint8)(MessageLength & 0xFFU);

    CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[Channel];

    /* Copy first 6 bytes of data */
    for (uint8 i = 0U; i < CANTP_MAX_FF_DATA_LEN; i++) {
        ffFrame[i + 2U] = runtime->Buffer[i];
    }

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = ffFrame;
    pduInfo.SduLength = CANTP_CAN_FRAME_LENGTH;
    pduInfo.MetaDataPtr = NULL_PTR;

    runtime->DataIndex = CANTP_MAX_FF_DATA_LEN;
    runtime->SequenceNumber = 1U;

    (void)CanIf_Transmit(CANTP_CANIF_TX_PDU_ID, &pduInfo);
}

/**
 * @brief CanTp_SendConsecutiveFrame - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_SendConsecutiveFrame function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
static void CanTp_SendConsecutiveFrame(CanTp_ChannelType Channel)
{
    uint8 cfFrame[CANTP_CAN_FRAME_LENGTH];
    CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[Channel];

    cfFrame[0] = (uint8)(CANTP_PCI_TYPE_CF | (runtime->SequenceNumber & CANTP_PCI_CF_SN_MASK));

    uint16 remainingBytes = runtime->DataLength - runtime->DataIndex;
    uint8 bytesToSend = (remainingBytes > CANTP_MAX_CF_DATA_LEN) ? CANTP_MAX_CF_DATA_LEN : (uint8)remainingBytes;

    for (uint8 i = 0U; i < bytesToSend; i++) {
        cfFrame[i + 1U] = runtime->Buffer[runtime->DataIndex + i];
    }

    /* Pad remaining bytes */
    for (uint8 i = (bytesToSend + 1U); i < CANTP_CAN_FRAME_LENGTH; i++) {
        cfFrame[i] = CANTP_PADDING_BYTE_VALUE;
    }

    PduInfoType pduInfo;
    pduInfo.SduDataPtr = cfFrame;
    pduInfo.SduLength = CANTP_CAN_FRAME_LENGTH;
    pduInfo.MetaDataPtr = NULL_PTR;

    runtime->DataIndex += bytesToSend;
    runtime->SequenceNumber = (runtime->SequenceNumber + 1U) & 0x0FU;

    (void)CanIf_Transmit(CANTP_CANIF_TX_PDU_ID, &pduInfo);
}

/**
 * @brief CanTp_Init - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_Init function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
void CanTp_Init(const CanTp_ConfigType* CfgPtr)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CfgPtr == NULL_PTR) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_INIT, CANTP_E_PARAM_CONFIG);
        return;
    }
    #endif

    CanTp_ConfigPtr = CfgPtr;

    /* Reset all channels */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        CanTp_ResetChannel((CanTp_ChannelType)i);
    }

    CanTp_Initialized = TRUE;
}

/**
 * @brief CanTp_Shutdown - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_Shutdown function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
void CanTp_Shutdown(void)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CanTp_Initialized == FALSE) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_SHUTDOWN, CANTP_E_UNINIT);
        return;
    }
    #endif

    /* Reset all channels */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        CanTp_ResetChannel((CanTp_ChannelType)i);
    }

    CanTp_ConfigPtr = NULL_PTR;
    CanTp_Initialized = FALSE;
}

/**
 * @brief CanTp_Transmit - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_Transmit function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
Std_ReturnType CanTp_Transmit(PduIdType CanTpTxSduId, const PduInfoType* CanTpTxInfoPtr)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CanTp_Initialized == FALSE) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_TRANSMIT, CANTP_E_UNINIT);
        return E_NOT_OK;
    }
    if (CanTpTxSduId >= CANTP_NUM_TX_NSDU) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_TRANSMIT, CANTP_E_INVALID_TX_ID);
        return E_NOT_OK;
    }
    if (CanTpTxInfoPtr == NULL_PTR) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_TRANSMIT, CANTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    uint16 dataLength = CanTpTxInfoPtr->SduLength;

    if (dataLength == 0U) {
        return E_NOT_OK;
    }

    /* Find free channel */
    CanTp_ChannelType channel = CanTp_FindFreeChannel();
    if (channel == CANTP_INVALID_CHANNEL_ID) {
        return E_NOT_OK;  /* No free channel */
    }

    /* Get Tx NSDU configuration for timing parameters */
    const CanTp_TxNsduConfigType* txNsduConfig = CanTp_GetTxNsduConfig(CanTpTxSduId);
    if (txNsduConfig == NULL_PTR) {
        return E_NOT_OK;  /* Invalid configuration */
    }

    CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[channel];
    runtime->ActiveNsduId = CanTpTxSduId;
    runtime->TxNsduConfig = txNsduConfig;  /* Store config pointer for timer access */
    runtime->DataLength = dataLength;

    /* Copy data to internal buffer */
    for (uint16 i = 0U; i < dataLength; i++) {
        runtime->Buffer[i] = CanTpTxInfoPtr->SduDataPtr[i];
    }

    if (dataLength <= CANTP_MAX_SF_DATA_LEN) {
        /* Single Frame transmission */
        runtime->State = CANTP_CH_TX_SF;
        CanTp_SendSingleFrame(CanTpTxSduId, runtime->Buffer, (uint8)dataLength);
        runtime->Timer = txNsduConfig->CanTpNas;  /* N_As from config table */
    } else {
        /* Multi-frame transmission */
        runtime->State = CANTP_CH_TX_FF;
        CanTp_SendFirstFrame(channel, dataLength);
        runtime->State = CANTP_CH_TX_WAIT_FC;
        runtime->Timer = txNsduConfig->CanTpNbs;  /* N_Bs from config table */
    }

    return E_OK;
}

/**
 * @brief CanTp_CancelTransmit - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_CancelTransmit function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
Std_ReturnType CanTp_CancelTransmit(PduIdType CanTpTxSduId)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CanTp_Initialized == FALSE) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_CANCELTRANSMIT, CANTP_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    /* Find channel with matching Tx SDU ID */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        if (CanTp_ChannelRuntime[i].ActiveNsduId == CanTpTxSduId &&
            (CanTp_ChannelRuntime[i].State == CANTP_CH_TX_SF ||
             CanTp_ChannelRuntime[i].State == CANTP_CH_TX_FF ||
             CanTp_ChannelRuntime[i].State == CANTP_CH_TX_CF ||
             CanTp_ChannelRuntime[i].State == CANTP_CH_TX_WAIT_FC)) {

            CanTp_ResetChannel((CanTp_ChannelType)i);
            return E_OK;
        }
    }

    return E_NOT_OK;
}

/**
 * @brief CanTp_CancelReceive - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_CancelReceive function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
Std_ReturnType CanTp_CancelReceive(PduIdType CanTpRxSduId)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CanTp_Initialized == FALSE) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_CANCELRECEIVE, CANTP_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    /* Find channel with matching Rx SDU ID */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        if (CanTp_ChannelRuntime[i].ActiveNsduId == CanTpRxSduId &&
            (CanTp_ChannelRuntime[i].State == CANTP_CH_RX_SF ||
             CanTp_ChannelRuntime[i].State == CANTP_CH_RX_FF ||
             CanTp_ChannelRuntime[i].State == CANTP_CH_RX_CF)) {

            CanTp_ResetChannel((CanTp_ChannelType)i);
            return E_OK;
        }
    }

    return E_NOT_OK;
}

/**
 * @brief CanTp_ChangeParameter - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_ChangeParameter function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
Std_ReturnType CanTp_ChangeParameter(PduIdType id, TPParameterType parameter, uint16 value)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CanTp_Initialized == FALSE) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_CHANGEPARAMETER, CANTP_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    #if (CANTP_CHANGE_PARAMETER_API == STD_ON)
    /* Find channel and update parameter */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        if (CanTp_ChannelRuntime[i].ActiveNsduId == id) {
            if (parameter == TP_STMIN) {
                CanTp_ChannelRuntime[i].STmin = (uint8)value;
                return E_OK;
            } else if (parameter == TP_BS) {
                CanTp_ChannelRuntime[i].BlockSize = (uint8)value;
                return E_OK;
            }
        }
    }
    #else
    (void)id;
    (void)parameter;
    (void)value;
    #endif

    return E_NOT_OK;
}

/**
 * @brief CanTp_ReadParameter - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_ReadParameter function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
Std_ReturnType CanTp_ReadParameter(PduIdType id, TPParameterType parameter, uint16* value)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (CanTp_Initialized == FALSE) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_READPARAMETER, CANTP_E_UNINIT);
        return E_NOT_OK;
    }
    if (value == NULL_PTR) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_READPARAMETER, CANTP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif

    #if (CANTP_READ_PARAMETER_API == STD_ON)
    /* Find channel and read parameter */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        if (CanTp_ChannelRuntime[i].ActiveNsduId == id) {
            if (parameter == TP_STMIN) {
                *value = CanTp_ChannelRuntime[i].STmin;
                return E_OK;
            } else if (parameter == TP_BS) {
                *value = CanTp_ChannelRuntime[i].BlockSize;
                return E_OK;
            }
        }
    }
    #else
    (void)id;
    (void)parameter;
    (void)value;
    #endif

    return E_NOT_OK;
}

/**
 * @brief CanTp_GetVersionInfo - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_GetVersionInfo function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
void CanTp_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (CANTP_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(CANTP_MODULE_ID, 0U, CANTP_SID_GETVERSIONINFO, CANTP_E_PARAM_POINTER);
        return;
    }
    #endif

    versioninfo->vendorID = CANTP_VENDOR_ID;
    versioninfo->moduleID = CANTP_MODULE_ID;
    versioninfo->sw_major_version = CANTP_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CANTP_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CANTP_SW_PATCH_VERSION;
}

/**
 * @brief CanTp_RxIndication - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_RxIndication function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
void CanTp_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    if (CanTp_Initialized == FALSE) {
        return;
    }

    if (PduInfoPtr == NULL_PTR || PduInfoPtr->SduDataPtr == NULL_PTR) {
        return;
    }

    uint8 pci = PduInfoPtr->SduDataPtr[0];
    uint8 frameType = pci & CANTP_PCI_TYPE_MASK;

    switch (frameType) {
        case CANTP_PCI_TYPE_SF: {
            /* Single Frame received */
            uint8 sfDl = pci & CANTP_PCI_SF_DL_MASK;

            if (sfDl > 0U && sfDl <= CANTP_MAX_SF_DATA_LEN) {
                /* Find free channel */
                CanTp_ChannelType channel = CanTp_FindFreeChannel();
                if (channel != 0xFFU) {
                    CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[channel];
                    runtime->State = CANTP_CH_RX_SF;
                    runtime->DataLength = sfDl;

                    /* Copy data */
                    for (uint8 i = 0U; i < sfDl; i++) {
                        runtime->Buffer[i] = PduInfoPtr->SduDataPtr[i + 1U];
                    }

                    /* Forward to PduR */
                    PduInfoType pduInfo;
                    pduInfo.SduDataPtr = runtime->Buffer;
                    pduInfo.SduLength = sfDl;
                    pduInfo.MetaDataPtr = NULL_PTR;

                    PduR_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pduInfo);

                    CanTp_ResetChannel(channel);
                }
            }
            break;
        }

        case CANTP_PCI_TYPE_FF: {
            /* First Frame received */
            uint16 ffDl = ((uint16)(pci & CANTP_PCI_FF_DL_MASK) << 8) | PduInfoPtr->SduDataPtr[1];

            if (ffDl > CANTP_MAX_SF_DATA_LEN && ffDl <= CANTP_MAX_MESSAGE_LENGTH) {
                /* Find free channel */
                CanTp_ChannelType channel = CanTp_FindFreeChannel();
                if (channel != 0xFFU) {
                    /* Get Rx NSDU configuration based on RxPduId */
                    const CanTp_RxNsduConfigType* rxNsduConfig = CanTp_GetRxNsduConfig(RxPduId);
                    if (rxNsduConfig == NULL_PTR) {
                        break;  /* Invalid configuration */
                    }

                    CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[channel];
                    runtime->State = CANTP_CH_RX_FF;
                    runtime->DataLength = ffDl;
                    runtime->DataIndex = CANTP_MAX_FF_DATA_LEN;
                    runtime->SequenceNumber = 1U;
                    runtime->RxNsduConfig = rxNsduConfig;  /* Store config pointer */

                    /* Copy first 6 bytes */
                    for (uint8 i = 0U; i < CANTP_MAX_FF_DATA_LEN; i++) {
                        runtime->Buffer[i] = PduInfoPtr->SduDataPtr[i + 2U];
                    }

                    /* Send Flow Control - Continue To Send */
                    CanTp_SendFlowControl(channel, CANTP_FLOWSTATUS_CTS, CANTP_BS_DEFAULT, CANTP_STMIN_DEFAULT);
                    runtime->State = CANTP_CH_RX_CF;
                    runtime->Timer = rxNsduConfig->CanTpNcr;  /* N_Cr from config table */
                }
            }
            break;
        }

        case CANTP_PCI_TYPE_CF: {
            /* Consecutive Frame received */
            uint8 sn = pci & CANTP_PCI_CF_SN_MASK;

            /* Find active receive channel */
            for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
                CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[i];

                if (runtime->State == CANTP_CH_RX_CF && runtime->SequenceNumber == sn) {
                    uint16 remainingBytes = runtime->DataLength - runtime->DataIndex;
                    uint8 bytesToCopy = (remainingBytes > CANTP_MAX_CF_DATA_LEN) ?
                                        CANTP_MAX_CF_DATA_LEN : (uint8)remainingBytes;

                    /* Copy data */
                    for (uint8 j = 0U; j < bytesToCopy; j++) {
                        runtime->Buffer[runtime->DataIndex + j] = PduInfoPtr->SduDataPtr[j + 1U];
                    }

                    runtime->DataIndex += bytesToCopy;
                    runtime->SequenceNumber = (runtime->SequenceNumber + 1U) & 0x0FU;
                    /* Reset N_Cr timer from config if available, otherwise use default */
                    if (runtime->RxNsduConfig != NULL_PTR) {
                        runtime->Timer = runtime->RxNsduConfig->CanTpNcr;
                    } else {
                        runtime->Timer = CANTP_NCR_DEFAULT;
                    }

                    /* Check if reception complete */
                    if (runtime->DataIndex >= runtime->DataLength) {
                        /* Forward to PduR */
                        PduInfoType pduInfo;
                        pduInfo.SduDataPtr = runtime->Buffer;
                        pduInfo.SduLength = runtime->DataLength;
                        pduInfo.MetaDataPtr = NULL_PTR;

                        PduR_RxIndication(CANTP_RX_DIAG_PHYSICAL, &pduInfo);

                        CanTp_ResetChannel((CanTp_ChannelType)i);
                    }
                    break;
                }
            }
            break;
        }

        case CANTP_PCI_TYPE_FC: {
            /* Flow Control received */
            uint8 fs = pci & CANTP_PCI_FC_FS_MASK;
            uint8 bs = PduInfoPtr->SduDataPtr[1];
            uint8 stmin = PduInfoPtr->SduDataPtr[2];

            /* Find active transmit channel waiting for FC */
            for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
                CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[i];

                if (runtime->State == CANTP_CH_TX_WAIT_FC) {
                    if (fs == CANTP_FLOWSTATUS_CTS) {
                        /* Continue To Send */
                        runtime->BlockSize = bs;
                        runtime->STmin = stmin;
                        runtime->State = CANTP_CH_TX_CF;
                        /* Use N_Cs from config if available */
                        if (runtime->TxNsduConfig != NULL_PTR) {
                            runtime->Timer = runtime->TxNsduConfig->CanTpNcs;
                        } else {
                            runtime->Timer = CANTP_NCS_DEFAULT;
                        }

                        /* Send first Consecutive Frame */
                        CanTp_SendConsecutiveFrame((CanTp_ChannelType)i);
                    } else if (fs == CANTP_FLOWSTATUS_WT) {
                        /* Wait */
                        /* Use N_Bs from config if available */
                        if (runtime->TxNsduConfig != NULL_PTR) {
                            runtime->Timer = runtime->TxNsduConfig->CanTpNbs;
                        } else {
                            runtime->Timer = CANTP_NBS_DEFAULT;
                        }
                    } else if (fs == CANTP_FLOWSTATUS_OVFLW) {
                        /* Overflow - abort transmission */
                        CanTp_ResetChannel((CanTp_ChannelType)i);
                    }
                    break;
                }
            }
            break;
        }

        default:
            /* Invalid frame type */
            break;
    }

    (void)RxPduId;
}

/**
 * @brief CanTp_TxConfirmation - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_TxConfirmation function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
void CanTp_TxConfirmation(PduIdType TxPduId)
{
    if (CanTp_Initialized == FALSE) {
        return;
    }

    /* Handle Tx confirmation for active channels */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[i];

        if (runtime->State == CANTP_CH_TX_SF) {
            /* Single Frame transmission complete */
            PduR_TxConfirmation(runtime->ActiveNsduId, E_OK);
            CanTp_ResetChannel((CanTp_ChannelType)i);
        } else if (runtime->State == CANTP_CH_TX_FF) {
            /* First Frame sent, waiting for FC */
            runtime->State = CANTP_CH_TX_WAIT_FC;
            /* Use N_Bs from config if available */
            if (runtime->TxNsduConfig != NULL_PTR) {
                runtime->Timer = runtime->TxNsduConfig->CanTpNbs;
            } else {
                runtime->Timer = CANTP_NBS_DEFAULT;
            }
        } else if (runtime->State == CANTP_CH_TX_CF) {
            /* Consecutive Frame sent */
            if (runtime->DataIndex >= runtime->DataLength) {
                /* All data sent */
                PduR_TxConfirmation(runtime->ActiveNsduId, E_OK);
                CanTp_ResetChannel((CanTp_ChannelType)i);
            } else {
                /* More frames to send */
                /* Use N_Cs from config if available */
                if (runtime->TxNsduConfig != NULL_PTR) {
                    runtime->Timer = runtime->TxNsduConfig->CanTpNcs;
                } else {
                    runtime->Timer = CANTP_NCS_DEFAULT;
                }
            }
        }
    }

    (void)TxPduId;
}

/**
 * @brief CanTp_MainFunction - AUTOSAR CAN Transport Layer API
 * @details Implements the AUTOSAR CanTp_MainFunction function for CAN TP segmentation/reassembly
 * @return Std_ReturnType or void per AUTOSAR specification
 */
void CanTp_MainFunction(void)
{
    if (CanTp_Initialized == FALSE) {
        return;
    }

    /* Process all channels */
    for (uint8 i = 0U; i < CANTP_MAX_CHANNEL_CNT; i++) {
        CanTp_ChannelRuntimeType* runtime = &CanTp_ChannelRuntime[i];

        if (runtime->State == CANTP_CH_IDLE) {
            continue;
        }

        /* Decrement timer */
        if (runtime->Timer > 0U) {
            runtime->Timer--;
        }

        /* Check for timeouts */
        if (runtime->Timer == 0U) {
            switch (runtime->State) {
                case CANTP_CH_TX_SF:
                case CANTP_CH_TX_FF:
                    /* N_As timeout */
                    CanTp_ResetChannel((CanTp_ChannelType)i);
                    break;

                case CANTP_CH_TX_WAIT_FC:
                    /* N_Bs timeout */
                    CanTp_ResetChannel((CanTp_ChannelType)i);
                    break;

                case CANTP_CH_TX_CF:
                    /* N_Cs timeout - send next frame */
                    CanTp_SendConsecutiveFrame((CanTp_ChannelType)i);
                    /* Use N_Cs from config if available */
                    if (runtime->TxNsduConfig != NULL_PTR) {
                        runtime->Timer = runtime->TxNsduConfig->CanTpNcs;
                    } else {
                        runtime->Timer = CANTP_NCS_DEFAULT;
                    }
                    break;

                case CANTP_CH_RX_CF:
                    /* N_Cr timeout */
                    CanTp_ResetChannel((CanTp_ChannelType)i);
                    break;

                default:
                    break;
            }
        }

        /* Handle Consecutive Frame transmission with STmin */
        if (runtime->State == CANTP_CH_TX_CF && runtime->DataIndex < runtime->DataLength) {
            /* Check if ready to send next CF based on STmin */
            /* In a real implementation, this would use a separate STmin timer */
        }
    }
}

#define CANTP_STOP_SEC_CODE
#include "MemMap.h"
