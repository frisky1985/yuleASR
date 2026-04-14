/**
 * @file CanIf_Cfg.h
 * @brief CAN Interface configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef CANIF_CFG_H
#define CANIF_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define CANIF_DEV_ERROR_DETECT          (STD_ON)
#define CANIF_VERSION_INFO_API          (STD_ON)
#define CANIF_DLC_CHECK                 (STD_ON)
#define CANIF_SOFTWARE_FILTER_TYPE      (STD_ON)

/*==================================================================================================
*                                    API ENABLE/DISABLE
==================================================================================================*/
#define CANIF_READRXPDUDATA_API         (STD_ON)
#define CANIF_READTXPDUNOTIFYSTATUS_API (STD_ON)
#define CANIF_READRXPDUNOTIFYSTATUS_API (STD_ON)
#define CANIF_SETDYNAMICTXID_API        (STD_ON)
#define CANIF_CANCELTRANSMIT_SUPPORT    (STD_OFF)

/*==================================================================================================
*                                    NUMBER OF CONFIGURATIONS
==================================================================================================*/
#define CANIF_NUM_CONTROLLERS           (2U)
#define CANIF_NUM_TX_PDUS               (32U)
#define CANIF_NUM_RX_PDUS               (32U)
#define CANIF_NUM_HRH                   (8U)
#define CANIF_NUM_HTH                   (8U)
#define CANIF_NUM_TRANSCEIVERS          (2U)

/*==================================================================================================
*                                    CONTROLLER DEFINITIONS
==================================================================================================*/
#define CANIF_CONTROLLER_0              (0U)
#define CANIF_CONTROLLER_1              (1U)

/*==================================================================================================
*                                    TX PDU DEFINITIONS
==================================================================================================*/
#define CANIF_TXPDU_ENGINE_STATUS       ((PduIdType)0U)
#define CANIF_TXPDU_VEHICLE_SPEED       ((PduIdType)1U)
#define CANIF_TXPDU_BATTERY_STATUS      ((PduIdType)2U)
#define CANIF_TXPDU_DIAG_REQUEST        ((PduIdType)3U)
#define CANIF_TXPDU_DIAG_RESPONSE       ((PduIdType)4U)

/*==================================================================================================
*                                    RX PDU DEFINITIONS
==================================================================================================*/
#define CANIF_RXPDU_ENGINE_CMD          ((PduIdType)0U)
#define CANIF_RXPDU_VEHICLE_CMD         ((PduIdType)1U)
#define CANIF_RXPDU_DIAG_REQUEST        ((PduIdType)2U)
#define CANIF_RXPDU_DIAG_RESPONSE       ((PduIdType)3U)

/*==================================================================================================
*                                    CAN ID DEFINITIONS
==================================================================================================*/
#define CANIF_CANID_ENGINE_STATUS       ((Can_IdType)0x100U)
#define CANIF_CANID_VEHICLE_SPEED       ((Can_IdType)0x200U)
#define CANIF_CANID_BATTERY_STATUS      ((Can_IdType)0x300U)
#define CANIF_CANID_ENGINE_CMD          ((Can_IdType)0x101U)
#define CANIF_CANID_VEHICLE_CMD         ((Can_IdType)0x201U)
#define CANIF_CANID_DIAG_REQUEST        ((Can_IdType)0x700U)
#define CANIF_CANID_DIAG_RESPONSE       ((Can_IdType)0x708U)

/*==================================================================================================
*                                    BAUDRATE CONFIGURATION
==================================================================================================*/
#define CANIF_DEFAULT_BAUDRATE          (500U)  /* 500 kbps */

/*==================================================================================================
*                                    MAIN FUNCTION PERIODS
==================================================================================================*/
#define CANIF_MAIN_FUNCTION_PERIOD_MS   (10U)

#endif /* CANIF_CFG_H */
