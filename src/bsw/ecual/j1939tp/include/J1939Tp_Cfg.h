/**
 * @file J1939Tp_Cfg.h
 * @brief J1939 Transport Protocol configuration header
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef J1939TP_CFG_H
#define J1939TP_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define J1939TP_DEV_ERROR_DETECT          (STD_ON)
#define J1939TP_VERSION_INFO_API          (STD_ON)
#define J1939TP_DYNAMIC_CHANNEL_ALLOCATION (STD_OFF)
#define J1939TP_CHANGE_PARAMETER_API      (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define J1939TP_MAX_CHANNEL_CNT           (4U)
#define J1939TP_NUM_CHANNELS              (2U)

/*==================================================================================================
*                                    NSDU CONFIGURATION
==================================================================================================*/
#define J1939TP_NUM_TX_NSDU               (4U)
#define J1939TP_NUM_RX_NSDU               (4U)

/*==================================================================================================
*                                    TX SDU IDs
==================================================================================================*/
#define J1939TP_TX_BAM_ENGINE_BROADCAST   ((PduIdType)0U)
#define J1939TP_TX_RTS_TCU_PHYSICAL       ((PduIdType)1U)
#define J1939TP_TX_BAM_DASHBOARD          ((PduIdType)2U)
#define J1939TP_TX_RTS_BCM_PHYSICAL       ((PduIdType)3U)

/*==================================================================================================
*                                    RX SDU IDs
==================================================================================================*/
#define J1939TP_RX_BAM_ENGINE_BROADCAST   ((PduIdType)0U)
#define J1939TP_RX_RTS_TCU_PHYSICAL       ((PduIdType)1U)
#define J1939TP_RX_BAM_DASHBOARD          ((PduIdType)2U)
#define J1939TP_RX_RTS_BCM_PHYSICAL       ((PduIdType)3U)

/*==================================================================================================
*                                    J1939 PGN DEFINITIONS
==================================================================================================*/
/* Engine Temperature (ET1) - used for BAM broadcast */
#define J1939TP_PGN_ET1                   (0x00FEEEU)  /* 65262 */

/* Electronic Engine Controller 3 (EEC3) - used for RTS/CTS */
#define J1939TP_PGN_EEC3                  (0x00FED4U)  /* 65236 */

/* Dash Display (DD) - used for BAM broadcast */
#define J1939TP_PGN_DD                    (0x00FEFCU)  /* 65276 */

/* Body Controller Messages */
#define J1939TP_PGN_BCM1                  (0x00FF00U)  /* 65280 - proprietary */

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION (ms)
==================================================================================================*/
#define J1939TP_T1_TIMEOUT_DEFAULT        (200U)  /* Response timeout */
#define J1939TP_T2_TIMEOUT_DEFAULT        (50U)   /* Sender timeout */
#define J1939TP_T3_TIMEOUT_DEFAULT        (200U)  /* Receiver timeout */
#define J1939TP_T4_TIMEOUT_DEFAULT        (50U)   /* BAM inter-packet time */
#define J1939TP_TH_TIMEOUT_DEFAULT        (750U)  /* Hold timeout */

/*==================================================================================================
*                                    PROTOCOL CONFIGURATION
==================================================================================================*/
#define J1939TP_PROTOCOL_DEFAULT          (J1939TP_PROTOCOL_BAM)
#define J1939TP_COMM_TYPE_DEFAULT         (J1939TP_COMM_BROADCAST)

/*==================================================================================================
*                                    ADDRESS CONFIGURATION
==================================================================================================*/
#define J1939TP_DEFAULT_SOURCE_ADDRESS    (0x0AU)   /* Engine #1 (example) */
#define J1939TP_DEFAULT_DEST_ADDRESS      (0xFFU)   /* Global */
#define J1939TP_TCU_ADDRESS               (0x03U)   /* Transmission */
#define J1939TP_BCM_ADDRESS               (0x21U)   /* Body Controller */
#define J1939TP_DASHBOARD_ADDRESS         (0x23U)   /* Instrument Cluster */

/*==================================================================================================
*                                    MAX MESSAGE LENGTH
==================================================================================================*/
#define J1939TP_MAX_MESSAGE_LENGTH        (1785U)  /* 255 packets * 7 bytes */

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define J1939TP_MAIN_FUNCTION_PERIOD_MS   (5U)

/*==================================================================================================
*                                    CAN INTERFACE PDU MAPPING
==================================================================================================*/
/* TP.CM Tx/Rx PDU IDs */
#define J1939TP_CANIF_CM_TX_PDU_ID        ((PduIdType)0U)
#define J1939TP_CANIF_CM_RX_PDU_ID        ((PduIdType)0U)

/* TP.DT Tx/Rx PDU IDs */
#define J1939TP_CANIF_DT_TX_PDU_ID        ((PduIdType)1U)
#define J1939TP_CANIF_DT_RX_PDU_ID        ((PduIdType)1U)

/* Additional PDU IDs for multi-channel */
#define J1939TP_CANIF_CM_TX_PDU_ID_2      ((PduIdType)2U)
#define J1939TP_CANIF_CM_RX_PDU_ID_2      ((PduIdType)2U)
#define J1939TP_CANIF_DT_TX_PDU_ID_2      ((PduIdType)3U)
#define J1939TP_CANIF_DT_RX_PDU_ID_2      ((PduIdType)3U)

/*==================================================================================================
*                                    MAX CTS PACKETS
==================================================================================================*/
#define J1939TP_MAX_CTS_PACKETS_DEFAULT   (16U)

/*==================================================================================================
*                                    RX BUFFER CONFIGURATION
==================================================================================================*/
#define J1939TP_RX_BUFFER_SIZE            (1785U)  /* Max J1939 message size */

#endif /* J1939TP_CFG_H */
