/**
 * @file DoCan_Lcfg.c
 * @brief Diagnostic over CAN Link-Time Configuration
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic over CAN (DoCan)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "DoCan.h"
#include "DoCan_Cfg.h"

/*==================================================================================================
*                                  GLOBAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DOCAN_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*--- PDU Mapping configurations ---*/
static const DoCan_PduMappingType DoCan_PduMappings[DOCAN_MAX_PDU_MAPPINGS] = {
    /* Mapping 0: DCM TX Physical -> CanTp TX Physical */
    {
        .DoCanPduId = DOCAN_DCM_TX_DIAG_PHYSICAL,
        .CanTpPduId = DOCAN_CANTP_TX_DIAG_PHYSICAL,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxConfirmationEnabled = TRUE,
        .RxIndicationEnabled = FALSE
    },
    /* Mapping 1: DCM TX Functional -> CanTp TX Functional */
    {
        .DoCanPduId = DOCAN_DCM_TX_DIAG_FUNCTIONAL,
        .CanTpPduId = DOCAN_CANTP_TX_DIAG_FUNCTIONAL,
        .ChannelType = DOCAN_CHANNEL_FUNCTIONAL,
        .TxConfirmationEnabled = TRUE,
        .RxIndicationEnabled = FALSE
    },
    /* Mapping 2: DCM RX Physical <- CanTp RX Physical */
    {
        .DoCanPduId = DOCAN_DCM_RX_DIAG_PHYSICAL,
        .CanTpPduId = DOCAN_CANTP_RX_DIAG_PHYSICAL,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxConfirmationEnabled = FALSE,
        .RxIndicationEnabled = TRUE
    },
    /* Mapping 3: DCM RX Functional <- CanTp RX Functional */
    {
        .DoCanPduId = DOCAN_DCM_RX_DIAG_FUNCTIONAL,
        .CanTpPduId = DOCAN_CANTP_RX_DIAG_FUNCTIONAL,
        .ChannelType = DOCAN_CHANNEL_FUNCTIONAL,
        .TxConfirmationEnabled = FALSE,
        .RxIndicationEnabled = TRUE
    },
    /* Mapping 4-7: Reserved */
    {
        .DoCanPduId = (PduIdType)0xFFFFU,
        .CanTpPduId = (PduIdType)0xFFFFU,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxConfirmationEnabled = FALSE,
        .RxIndicationEnabled = FALSE
    },
    {
        .DoCanPduId = (PduIdType)0xFFFFU,
        .CanTpPduId = (PduIdType)0xFFFFU,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxConfirmationEnabled = FALSE,
        .RxIndicationEnabled = FALSE
    },
    {
        .DoCanPduId = (PduIdType)0xFFFFU,
        .CanTpPduId = (PduIdType)0xFFFFU,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxConfirmationEnabled = FALSE,
        .RxIndicationEnabled = FALSE
    },
    {
        .DoCanPduId = (PduIdType)0xFFFFU,
        .CanTpPduId = (PduIdType)0xFFFFU,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxConfirmationEnabled = FALSE,
        .RxIndicationEnabled = FALSE
    }
};

/*--- Channel configurations ---*/
static const DoCan_ChannelConfigType DoCan_ChannelConfigs[DOCAN_MAX_CHANNELS] = {
    {
        .ChannelId = DOCAN_CHANNEL_0,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxPduId = DOCAN_DCM_TX_DIAG_PHYSICAL,
        .RxPduId = DOCAN_DCM_RX_DIAG_PHYSICAL,
        .TimeoutMs = 1000U
    },
    {
        .ChannelId = DOCAN_CHANNEL_1,
        .ChannelType = DOCAN_CHANNEL_FUNCTIONAL,
        .TxPduId = DOCAN_DCM_TX_DIAG_FUNCTIONAL,
        .RxPduId = DOCAN_DCM_RX_DIAG_FUNCTIONAL,
        .TimeoutMs = 500U
    },
    {
        .ChannelId = DOCAN_CHANNEL_2,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxPduId = (PduIdType)0xFFFFU,
        .RxPduId = (PduIdType)0xFFFFU,
        .TimeoutMs = 0U
    },
    {
        .ChannelId = DOCAN_CHANNEL_3,
        .ChannelType = DOCAN_CHANNEL_PHYSICAL,
        .TxPduId = (PduIdType)0xFFFFU,
        .RxPduId = (PduIdType)0xFFFFU,
        .TimeoutMs = 0U
    }
};

/*--- Global DoCan configuration ---*/
const DoCan_ConfigType DoCan_Config = {
    .PduMappings = DoCan_PduMappings,
    .NumPduMappings = 4U,
    .ChannelConfigs = DoCan_ChannelConfigs,
    .NumChannels = 2U,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE
};

#define DOCAN_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
