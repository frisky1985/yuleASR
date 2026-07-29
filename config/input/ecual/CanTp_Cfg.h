/**
 * @file CanTp_Cfg.h
 * @brief CAN Transport Protocol configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef CANTP_CFG_H
#define CANTP_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define CANTP_DEV_ERROR_DETECT          (STD_ON)
#define CANTP_VERSION_INFO_API          (STD_ON)
#define CANTP_DYNAMIC_CHANNEL_ALLOCATION (STD_OFF)
#define CANTP_PADDING_BYTE              (STD_ON)
#define CANTP_PADDING_BYTE_VALUE        (0xCCU)
#define CANTP_CHANGE_PARAMETER_API      (STD_ON)
#define CANTP_READ_PARAMETER_API        (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define CANTP_MAX_CHANNEL_CNT           (4U)
#define CANTP_NUM_CHANNELS              (2U)

/*==================================================================================================
*                                    NSDU CONFIGURATION
==================================================================================================*/
#define CANTP_NUM_TX_NSDU               (4U)
#define CANTP_NUM_RX_NSDU               (4U)

/*==================================================================================================
*                                    TX SDU IDs
==================================================================================================*/
#define CANTP_TX_DIAG_PHYSICAL          ((PduIdType)0U)
#define CANTP_TX_DIAG_FUNCTIONAL        ((PduIdType)1U)
#define CANTP_TX_UDS_PHYSICAL           ((PduIdType)2U)
#define CANTP_TX_UDS_FUNCTIONAL         ((PduIdType)3U)

/*==================================================================================================
*                                    RX SDU IDs
==================================================================================================*/
#define CANTP_RX_DIAG_PHYSICAL          ((PduIdType)0U)
#define CANTP_RX_DIAG_FUNCTIONAL        ((PduIdType)1U)
#define CANTP_RX_UDS_PHYSICAL           ((PduIdType)2U)
#define CANTP_RX_UDS_FUNCTIONAL         ((PduIdType)3U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION (ms)
==================================================================================================*/
#define CANTP_NAS_DEFAULT               (25U)   /* N_As timeout */
#define CANTP_NBS_DEFAULT               (75U)   /* N_Bs timeout */
#define CANTP_NCS_DEFAULT               (25U)   /* N_Cs timeout */
#define CANTP_NAR_DEFAULT               (25U)   /* N_Ar timeout */
#define CANTP_NBR_DEFAULT               (75U)   /* N_Br timeout */
#define CANTP_NCR_DEFAULT               (150U)  /* N_Cr timeout */

/*==================================================================================================
*                                    FLOW CONTROL DEFAULTS
==================================================================================================*/
#define CANTP_BS_DEFAULT                (8U)    /* Block Size */
#define CANTP_STMIN_DEFAULT             (20U)   /* Minimum Separation Time (ms) */
#define CANTP_WFT_MAX_DEFAULT           (8U)    /* Max Wait Frames */

/*==================================================================================================
*                                    ADDRESSING CONFIGURATION
==================================================================================================*/
#define CANTP_ADDRESSING_FORMAT         (CANTP_STANDARD)
#define CANTP_TX_ADDRESS                (0x00U)
#define CANTP_RX_ADDRESS                (0x00U)

/*==================================================================================================
*                                    MAX MESSAGE LENGTH
==================================================================================================*/
#define CANTP_MAX_MESSAGE_LENGTH        (4095U) /* ISO-TP max */
#define CANTP_CANFD_MAX_MESSAGE_LENGTH  (4095U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define CANTP_MAIN_FUNCTION_PERIOD_MS   (5U)

/*==================================================================================================
*                                    PDU MAPPING
==================================================================================================*/
#define CANTP_CANIF_TX_PDU_ID           ((PduIdType)0U)
#define CANTP_CANIF_RX_PDU_ID           ((PduIdType)0U)
#define CANTP_CANIF_FC_TX_PDU_ID        ((PduIdType)1U)
#define CANTP_CANIF_FC_RX_PDU_ID        ((PduIdType)1U)

#endif /* CANTP_CFG_H */
