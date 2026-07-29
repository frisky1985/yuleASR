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

/* Forward type definitions needed before CanIf.h is included */
typedef uint8 CanIf_ControllerModeType;
#define CANIF_CS_UNINIT         0x00u
#define CANIF_CS_STARTED        0x01u
#define CANIF_CS_STOPPED        0x02u
#define CANIF_CS_SLEEP          0x03u
#define CANIF_NUM_CONTROLLERS   2u

/*=============================================================================
 * Pre-compile configuration parameters
 *=============================================================================*/

/* Number of CAN controllers configured */
#define CANIF_CONTROLLER_CNT        1U

/* Number of Hardware Object Handles (HOH) */
#define CANIF_HOH_CNT               4U

/* Number of Hardware Transmit Handles (HTH) */
#define CANIF_HTH_CNT               2U

/* Number of L-PDUs configured */
#define CANIF_LPDU_CNT              8U

/* Number of Tx L-PDUs */
#define CANIF_TX_LPDU_CNT           4U

/* Number of Rx L-PDUs */
#define CANIF_RX_LPDU_CNT           4U

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
 * Configuration Structures (Link-time configuration)
 *=============================================================================*/

/* Hardware Object Handle type */
typedef uint8 CanIf_HohType;

/* Hardware Transmit Handle type */
typedef uint8 CanIf_HthType;

/* CAN Identifier type */
typedef uint32 CanIf_CanIdType;

/* CAN Identifier Type (standard/extended) */
typedef enum {
    CANIF_CANID_TYPE_STANDARD = 0,
    CANIF_CANID_TYPE_EXTENDED
} CanIf_CanIdTypeType;

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
#define CANIF_E_PARAM_HOH           0x03U
#define CANIF_E_PARAM_HTH           0x04U
#define CANIF_E_PARAM_CONTROLLER    0x05U
#define CANIF_E_PARAM_POINTER       0x06U
#define CANIF_E_UNINIT              0x07U
#define CANIF_E_INVALID_TXPDUID     0x08U
#define CANIF_E_INVALID_RXPDUID     0x09U
#define CANIF_E_INIT_FAILED         0x0AU
#define CANIF_E_PARAM_TRCV          0x0BU
#define CANIF_E_PARAM_TRCVMODE      0x0CU
#define CANIF_E_PARAM_WAKEUPSOURCE  0x0DU
#define CANIF_E_PARAM_CTRLMODE      0x0EU
#define CANIF_E_PARAM_PDUMODE       0x0FU
#define CANIF_E_INVALID_DATA_LENGTH 0x10U

/* Service IDs for error reporting */
#define CANIF_SID_INIT              0x01U
#define CANIF_SID_SETCONTROLLERMODE 0x03U
#define CANIF_SID_GETCONTROLLERMODE 0x04U
#define CANIF_SID_TRANSMIT          0x09U
#define CANIF_SID_RXINDICATION      0x14U
#define CANIF_SID_TXCONFIRMATION    0x15U
#define CANIF_SID_SETPDUMODE        0x1AU
#define CANIF_SID_GETPDUMODE        0x1BU

#endif /* CANIF_CFG_H */
