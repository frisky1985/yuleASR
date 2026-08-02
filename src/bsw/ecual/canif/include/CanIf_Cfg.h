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

/*
 * CanIf_Cfg.h
 * CAN Interface Configuration Header
 * AUTOSAR-compliant implementation
 */

#ifndef CANIF_CFG_H
#define CANIF_CFG_H

#include "Std_Types.h"

/*=============================================================================
 * Pre-compile configuration parameters
 *=============================================================================*/

/* Number of CAN controllers configured */
#define CANIF_CONTROLLER_CNT        1U

/* Num aliases used by CanIf.c (AUTOSAR naming) */
#define CANIF_NUM_CONTROLLERS       CANIF_CONTROLLER_CNT
#define CANIF_NUM_TRANSCEIVERS      CANIF_CONTROLLER_CNT

/* Number of Hardware Object Handles (HOH) */
#define CANIF_HOH_CNT               4U

/* Number of Hardware Transmit Handles (HTH) */
#define CANIF_HTH_CNT               2U

/* Number of L-PDUs configured */
#define CANIF_LPDU_CNT              8U

/* Number of Tx L-PDUs */
#define CANIF_TX_LPDU_CNT           4U
#define CANIF_NUM_TX_PDUS           CANIF_TX_LPDU_CNT

/* Number of Rx L-PDUs */
#define CANIF_RX_LPDU_CNT           4U
#define CANIF_NUM_RX_PDUS           CANIF_RX_LPDU_CNT

/*=============================================================================
 * Switches
 *=============================================================================*/

/* Development error detection enable/disable */
#define CANIF_DEV_ERROR_DETECT      STD_ON

/* Version info API enable/disable */
#define CANIF_VERSION_INFO_API      STD_ON

/* Transmit cancellation enable/disable */
#define CANIF_TRANSMIT_CANCELLATION STD_OFF

/* Receive indication enable/disable */
#define CANIF_RX_INDICATION         STD_ON

/* Transmit confirmation enable/disable */
#define CANIF_TX_CONFIRMATION       STD_ON

/* Controller Wakeup support */
#define CANIF_WAKEUP_SUPPORT        STD_ON

/*=============================================================================
 * Controller Configuration
 *=============================================================================*/

/* Controller IDs */
#define CANIF_CONTROLLER_0          0U

/* Default baudrate (kbps) used by CanIf_GetBaudrate */
#define CANIF_DEFAULT_BAUDRATE      500U

/*=============================================================================
 * HOH Configuration Indices
 *=============================================================================*/

/* Hardware Transmit Handles (HTH) - indices into HOH table */
#define CANIF_HTH_0                 0U
#define CANIF_HTH_1                 1U

/* Hardware Receive Handles (HRH) - indices into HOH table */
#define CANIF_HRH_0                 2U
#define CANIF_HRH_1                 3U

/*=============================================================================
 * L-PDU IDs (User configured)
 *=============================================================================*/

/* Tx L-PDU IDs */
#define CANIF_TX_LPDU_0             0U
#define CANIF_TX_LPDU_1             1U
#define CANIF_TX_LPDU_2             2U
#define CANIF_TX_LPDU_3             3U

/* Rx L-PDU IDs */
#define CANIF_RX_LPDU_0             4U
#define CANIF_RX_LPDU_1             5U
#define CANIF_RX_LPDU_2             6U
#define CANIF_RX_LPDU_3             7U

/*=============================================================================
 * PDU Mode Types
 *=============================================================================*/

/*=============================================================================
 * Configuration Structures (Link-time configuration)
 *=============================================================================*/

/* CAN ID type */
typedef uint32 CanIf_CanIdType;
typedef uint32 CanIf_CanIdTypeType;

/* Hardware Object Handle type */
typedef uint8 CanIf_HohType;

/* Hardware Transmit Handle type */
typedef uint8 CanIf_HthType;

/* L-PDU ID type */
typedef uint16 CanIf_PduIdType;

/* HOH configuration type */
typedef struct
{
    uint8 controllerId;      /* Associated CAN controller */
    boolean isTx;            /* TRUE = Tx HOH, FALSE = Rx HOH */
    uint8 driverObjId;       /* Driver-specific object ID */
} CanIf_HohCfgType;

/* Tx L-PDU configuration type */
typedef struct
{
    CanIf_PduIdType pduId;           /* L-PDU ID */
    CanIf_CanIdType canId;           /* CAN Identifier */
    CanIf_HthType hthId;             /* Associated HTH */
    uint8 controllerId;              /* Associated controller */
    uint8 dlc;                       /* Data Length Code (0-8) */
} CanIf_TxPduCfgType;

/* Rx L-PDU configuration type */
typedef struct
{
    CanIf_PduIdType pduId;           /* L-PDU ID */
    CanIf_CanIdType canId;           /* CAN Identifier */
    CanIf_CanIdType canIdMask;       /* CAN ID mask for filtering */
    CanIf_HohType hohId;             /* Associated HRH */
    uint8 controllerId;              /* Associated controller */
    uint8 dlc;                       /* Data Length Code (0-8) */
} CanIf_RxPduCfgType;

/* Controller Mode type (redefined as enum in CanIf.h) */
typedef uint8 CanIf_ControllerModeType;
typedef uint8 CanIf_PduModeType;

/* Controller configuration type */
typedef struct
{
    uint8 controllerId;              /* Controller ID */
    CanIf_ControllerModeType initMode; /* Initial mode */
} CanIf_ControllerCfgType;

/*=============================================================================
 * External Configuration References (defined in CanIf_Lcfg.c)
 *=============================================================================*/

extern const CanIf_HohCfgType CanIf_HohCfg[CANIF_HOH_CNT];
extern const CanIf_TxPduCfgType CanIf_TxPduCfg[CANIF_TX_LPDU_CNT];
extern const CanIf_RxPduCfgType CanIf_RxPduCfg[CANIF_RX_LPDU_CNT];
extern const CanIf_ControllerCfgType CanIf_ControllerCfg[CANIF_CONTROLLER_CNT];

/* Rx L-PDU to HOH mapping table (for fast lookup) */
extern const CanIf_PduIdType CanIf_RxPduHohMap[CANIF_HOH_CNT][CANIF_RX_LPDU_CNT];

/*=============================================================================
 * Error Codes
 *=============================================================================*/

#define CANIF_E_PARAM_CANID         0x01U
#define CANIF_E_PARAM_DLC           0x02U

/* Controller / PDU counts */
#define CANIF_NUM_CONTROLLERS              (1U)
#define CANIF_NUM_TRANSCEIVERS             (1U)
#define CANIF_NUM_TX_PDUS                  (8U)
#define CANIF_NUM_RX_PDUS                  (8U)

#define CANIF_DEFAULT_BAUDRATE        (500000U)

#endif /* CANIF_CFG_H */
