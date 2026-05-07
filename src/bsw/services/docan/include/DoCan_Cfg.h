/**
 * @file DoCan_Cfg.h
 * @brief Diagnostic over CAN configuration header
 * @version 1.0.0
 * @date 2026-04-24
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef DOCAN_CFG_H
#define DOCAN_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define DOCAN_DEV_ERROR_DETECT          (STD_ON)
#define DOCAN_VERSION_INFO_API          (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define DOCAN_MAX_CHANNELS              (4U)
#define DOCAN_MAX_PDU_MAPPINGS          (8U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define DOCAN_MAIN_FUNCTION_PERIOD_MS   (10U)

/*==================================================================================================
*                                    DCM PDU IDs
==================================================================================================*/
#define DOCAN_DCM_TX_DIAG_PHYSICAL      ((PduIdType)0U)
#define DOCAN_DCM_TX_DIAG_FUNCTIONAL    ((PduIdType)1U)
#define DOCAN_DCM_RX_DIAG_PHYSICAL      ((PduIdType)2U)
#define DOCAN_DCM_RX_DIAG_FUNCTIONAL    ((PduIdType)3U)

/*==================================================================================================
*                                    CANTP PDU IDs
==================================================================================================*/
#define DOCAN_CANTP_TX_DIAG_PHYSICAL    ((PduIdType)0U)
#define DOCAN_CANTP_TX_DIAG_FUNCTIONAL  ((PduIdType)1U)
#define DOCAN_CANTP_RX_DIAG_PHYSICAL    ((PduIdType)0U)
#define DOCAN_CANTP_RX_DIAG_FUNCTIONAL  ((PduIdType)1U)

/*==================================================================================================
*                                    CHANNEL IDs
==================================================================================================*/
#define DOCAN_CHANNEL_0                 (0U)
#define DOCAN_CHANNEL_1                 (1U)
#define DOCAN_CHANNEL_2                 (2U)
#define DOCAN_CHANNEL_3                 (3U)

#endif /* DOCAN_CFG_H */
