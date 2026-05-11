/**
 * @file CanTp_Lcfg.c
 * @brief CAN Transport Protocol link-time configuration
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * Link-time configuration for CanTp module.
 * All timing parameters (N_As, N_Bs, N_Cs, N_Ar, N_Br, N_Cr) are configurable
 * per NSDU (Network Service Data Unit).
 */

#include "CanTp.h"
#include "CanTp_Cfg.h"

#define CANTP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    TX NSDU CONFIGURATION TABLE
*==================================================================================================*/
/**
 * @brief Tx NSDU configuration table
 * 
 * Each entry configures timing parameters for a specific Tx NSDU:
 * - CanTpNas: N_As timeout (Tx confirmation timeout) in ms
 * - CanTpNbs: N_Bs timeout (Flow Control reception timeout) in ms  
 * - CanTpNcs: N_Cs timeout (Consecutive Frame transmission timeout) in ms
 */
static const CanTp_TxNsduConfigType CanTp_TxNsduConfigs[CANTP_NUM_TX_NSDU] = {
    /* [0] Diagnostic Physical Tx */
    {
        /* CanTpTxNPduId */                CANTP_CANIF_TX_PDU_ID,
        /* CanTpTxNPduConfirmationId */    CANTP_TX_DIAG_PHYSICAL,
        /* CanTpTxFcNPduId */              CANTP_CANIF_FC_RX_PDU_ID,
        /* CanTpNas */                     25U,    /* N_As: Tx confirmation timeout (ms) */
        /* CanTpNbs */                     75U,    /* N_Bs: FC reception timeout (ms) */
        /* CanTpNcs */                     25U,    /* N_Cs: CF transmission timeout (ms) */
        /* CanTpTxAddressingFormat */      CANTP_ADDRESSING_FORMAT,
        /* CanTpTxPaddingActivation */     CANTP_PADDING_BYTE,
        /* CanTpTxTaType */                CANTP_PHYSICAL,
        /* CanTpTxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpTxAddress */               CANTP_TX_ADDRESS,
        /* CanTpTxPriority */              1U
    },
    /* [1] Diagnostic Functional Tx */
    {
        /* CanTpTxNPduId */                CANTP_CANIF_TX_PDU_ID,
        /* CanTpTxNPduConfirmationId */    CANTP_TX_DIAG_FUNCTIONAL,
        /* CanTpTxFcNPduId */              CANTP_CANIF_FC_RX_PDU_ID,
        /* CanTpNas */                     25U,    /* N_As: Tx confirmation timeout (ms) */
        /* CanTpNbs */                     75U,    /* N_Bs: FC reception timeout (ms) */
        /* CanTpNcs */                     25U,    /* N_Cs: CF transmission timeout (ms) */
        /* CanTpTxAddressingFormat */      CANTP_ADDRESSING_FORMAT,
        /* CanTpTxPaddingActivation */     CANTP_PADDING_BYTE,
        /* CanTpTxTaType */                CANTP_FUNCTIONAL,
        /* CanTpTxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpTxAddress */               CANTP_TX_ADDRESS,
        /* CanTpTxPriority */              2U
    },
    /* [2] UDS Physical Tx */
    {
        /* CanTpTxNPduId */                CANTP_CANIF_TX_PDU_ID,
        /* CanTpTxNPduConfirmationId */    CANTP_TX_UDS_PHYSICAL,
        /* CanTpTxFcNPduId */              CANTP_CANIF_FC_RX_PDU_ID,
        /* CanTpNas */                     25U,    /* N_As: Tx confirmation timeout (ms) */
        /* CanTpNbs */                     75U,    /* N_Bs: FC reception timeout (ms) */
        /* CanTpNcs */                     25U,    /* N_Cs: CF transmission timeout (ms) */
        /* CanTpTxAddressingFormat */      CANTP_ADDRESSING_FORMAT,
        /* CanTpTxPaddingActivation */     CANTP_PADDING_BYTE,
        /* CanTpTxTaType */                CANTP_PHYSICAL,
        /* CanTpTxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpTxAddress */               CANTP_TX_ADDRESS,
        /* CanTpTxPriority */              3U
    },
    /* [3] UDS Functional Tx */
    {
        /* CanTpTxNPduId */                CANTP_CANIF_TX_PDU_ID,
        /* CanTpTxNPduConfirmationId */    CANTP_TX_UDS_FUNCTIONAL,
        /* CanTpTxFcNPduId */              CANTP_CANIF_FC_RX_PDU_ID,
        /* CanTpNas */                     25U,    /* N_As: Tx confirmation timeout (ms) */
        /* CanTpNbs */                     75U,    /* N_Bs: FC reception timeout (ms) */
        /* CanTpNcs */                     25U,    /* N_Cs: CF transmission timeout (ms) */
        /* CanTpTxAddressingFormat */      CANTP_ADDRESSING_FORMAT,
        /* CanTpTxPaddingActivation */     CANTP_PADDING_BYTE,
        /* CanTpTxTaType */                CANTP_FUNCTIONAL,
        /* CanTpTxMaxMessageLength */      CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpTxAddress */               CANTP_TX_ADDRESS,
        /* CanTpTxPriority */              4U
    }
};

/*==================================================================================================
*                                    RX NSDU CONFIGURATION TABLE
*==================================================================================================*/
/**
 * @brief Rx NSDU configuration table
 * 
 * Each entry configures timing parameters for a specific Rx NSDU:
 * - CanTpNar: N_Ar timeout (Rx indication timeout) in ms
 * - CanTpNbr: N_Br timeout (Buffer availability timeout) in ms
 * - CanTpNcr: N_Cr timeout (Consecutive Frame reception timeout) in ms
 */
static const CanTp_RxNsduConfigType CanTp_RxNsduConfigs[CANTP_NUM_RX_NSDU] = {
    /* [0] Diagnostic Physical Rx */
    {
        /* CanTpRxNPduId */                   CANTP_CANIF_RX_PDU_ID,
        /* CanTpRxNSduId */                   CANTP_RX_DIAG_PHYSICAL,
        /* CanTpRxFcNPduConfirmationId */     CANTP_CANIF_FC_TX_PDU_ID,
        /* CanTpNar */                        25U,    /* N_Ar: Rx indication timeout (ms) */
        /* CanTpNbr */                        75U,    /* N_Br: Buffer availability timeout (ms) */
        /* CanTpNcr */                        150U,   /* N_Cr: CF reception timeout (ms) */
        /* CanTpRxAddressingFormat */         CANTP_ADDRESSING_FORMAT,
        /* CanTpRxPaddingActivation */        CANTP_PADDING_BYTE,
        /* CanTpRxTaType */                   CANTP_PHYSICAL,
        /* CanTpRxMaxMessageLength */         CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpRxAddress */                  CANTP_RX_ADDRESS,
        /* CanTpRxWftMax */                   CANTP_WFT_MAX_DEFAULT,
        /* CanTpRxPriority */                 1U,
        /* CanTpBs */                         CANTP_BS_DEFAULT,      /* Block Size */
        /* CanTpSTmin */                      CANTP_STMIN_DEFAULT    /* Min Separation Time */
    },
    /* [1] Diagnostic Functional Rx */
    {
        /* CanTpRxNPduId */                   CANTP_CANIF_RX_PDU_ID,
        /* CanTpRxNSduId */                   CANTP_RX_DIAG_FUNCTIONAL,
        /* CanTpRxFcNPduConfirmationId */     CANTP_CANIF_FC_TX_PDU_ID,
        /* CanTpNar */                        25U,    /* N_Ar: Rx indication timeout (ms) */
        /* CanTpNbr */                        75U,    /* N_Br: Buffer availability timeout (ms) */
        /* CanTpNcr */                        150U,   /* N_Cr: CF reception timeout (ms) */
        /* CanTpRxAddressingFormat */         CANTP_ADDRESSING_FORMAT,
        /* CanTpRxPaddingActivation */        CANTP_PADDING_BYTE,
        /* CanTpRxTaType */                   CANTP_FUNCTIONAL,
        /* CanTpRxMaxMessageLength */         CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpRxAddress */                  CANTP_RX_ADDRESS,
        /* CanTpRxWftMax */                   CANTP_WFT_MAX_DEFAULT,
        /* CanTpRxPriority */                 2U,
        /* CanTpBs */                         CANTP_BS_DEFAULT,
        /* CanTpSTmin */                      CANTP_STMIN_DEFAULT
    },
    /* [2] UDS Physical Rx */
    {
        /* CanTpRxNPduId */                   CANTP_CANIF_RX_PDU_ID,
        /* CanTpRxNSduId */                   CANTP_RX_UDS_PHYSICAL,
        /* CanTpRxFcNPduConfirmationId */     CANTP_CANIF_FC_TX_PDU_ID,
        /* CanTpNar */                        25U,    /* N_Ar: Rx indication timeout (ms) */
        /* CanTpNbr */                        75U,    /* N_Br: Buffer availability timeout (ms) */
        /* CanTpNcr */                        150U,   /* N_Cr: CF reception timeout (ms) */
        /* CanTpRxAddressingFormat */         CANTP_ADDRESSING_FORMAT,
        /* CanTpRxPaddingActivation */        CANTP_PADDING_BYTE,
        /* CanTpRxTaType */                   CANTP_PHYSICAL,
        /* CanTpRxMaxMessageLength */         CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpRxAddress */                  CANTP_RX_ADDRESS,
        /* CanTpRxWftMax */                   CANTP_WFT_MAX_DEFAULT,
        /* CanTpRxPriority */                 3U,
        /* CanTpBs */                         CANTP_BS_DEFAULT,
        /* CanTpSTmin */                      CANTP_STMIN_DEFAULT
    },
    /* [3] UDS Functional Rx */
    {
        /* CanTpRxNPduId */                   CANTP_CANIF_RX_PDU_ID,
        /* CanTpRxNSduId */                   CANTP_RX_UDS_FUNCTIONAL,
        /* CanTpRxFcNPduConfirmationId */     CANTP_CANIF_FC_TX_PDU_ID,
        /* CanTpNar */                        25U,    /* N_Ar: Rx indication timeout (ms) */
        /* CanTpNbr */                        75U,    /* N_Br: Buffer availability timeout (ms) */
        /* CanTpNcr */                        150U,   /* N_Cr: CF reception timeout (ms) */
        /* CanTpRxAddressingFormat */         CANTP_ADDRESSING_FORMAT,
        /* CanTpRxPaddingActivation */        CANTP_PADDING_BYTE,
        /* CanTpRxTaType */                   CANTP_FUNCTIONAL,
        /* CanTpRxMaxMessageLength */         CANTP_MAX_MESSAGE_LENGTH,
        /* CanTpRxAddress */                  CANTP_RX_ADDRESS,
        /* CanTpRxWftMax */                   CANTP_WFT_MAX_DEFAULT,
        /* CanTpRxPriority */                 4U,
        /* CanTpBs */                         CANTP_BS_DEFAULT,
        /* CanTpSTmin */                      CANTP_STMIN_DEFAULT
    }
};

/*==================================================================================================
*                                    CHANNEL CONFIGURATION TABLE
*==================================================================================================*/
static const CanTp_ChannelConfigType CanTp_ChannelConfigs[CANTP_NUM_CHANNELS] = {
    {
        /* ChannelId */       0U,
        /* ChannelMode */     CANTP_MODE_FULL_DUPLEX,
        /* NumTxNsdu */       CANTP_NUM_TX_NSDU,
        /* NumRxNsdu */       CANTP_NUM_RX_NSDU,
        /* TxNsduConfigs */   CanTp_TxNsduConfigs,
        /* RxNsduConfigs */   CanTp_RxNsduConfigs
    },
    {
        /* ChannelId */       1U,
        /* ChannelMode */     CANTP_MODE_FULL_DUPLEX,
        /* NumTxNsdu */       CANTP_NUM_TX_NSDU,
        /* NumRxNsdu */       CANTP_NUM_RX_NSDU,
        /* TxNsduConfigs */   CanTp_TxNsduConfigs,
        /* RxNsduConfigs */   CanTp_RxNsduConfigs
    }
};

/*==================================================================================================
*                                    GENERAL CONFIGURATION
*==================================================================================================*/
static const CanTp_GeneralConfigType CanTp_GeneralConfig = {
    /* DevErrorDetect */              CANTP_DEV_ERROR_DETECT,
    /* VersionInfoApi */              CANTP_VERSION_INFO_API,
    /* CanTpDynamicChannelAllocation */ CANTP_DYNAMIC_CHANNEL_ALLOCATION,
    /* CanTpMaxChannelCnt */          CANTP_MAX_CHANNEL_CNT,
    /* CanTpPaddingByte */            CANTP_PADDING_BYTE,
    /* CanTpPaddingByteValue */       CANTP_PADDING_BYTE_VALUE,
    /* CanTpChangeParameterApi */     CANTP_CHANGE_PARAMETER_API,
    /* CanTpReadParameterApi */       CANTP_READ_PARAMETER_API,
    /* CanTpMainFunctionPeriod */     CANTP_MAIN_FUNCTION_PERIOD_MS
};

/*==================================================================================================
*                                    GLOBAL CONFIG ROOT
*==================================================================================================*/
const CanTp_ConfigType CanTp_Config = {
    /* GeneralConfig */   &CanTp_GeneralConfig,
    /* ChannelConfigs */  CanTp_ChannelConfigs,
    /* NumChannels */     CANTP_NUM_CHANNELS
};

#define CANTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"
