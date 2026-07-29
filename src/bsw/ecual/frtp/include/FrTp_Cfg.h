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
 * @file FrTp_Cfg.h
 * @brief FlexRay Transport Protocol configuration header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef FRTP_CFG_H
#define FRTP_CFG_H

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define FRTP_CFG_VENDOR_ID              (0x01U)
#define FRTP_CFG_MODULE_ID              (0x2DU)
#define FRTP_CFG_AR_RELEASE_MAJOR_VERSION   (0x04U)
#define FRTP_CFG_AR_RELEASE_MINOR_VERSION   (0x04U)
#define FRTP_CFG_SW_MAJOR_VERSION       (0x01U)
#define FRTP_CFG_SW_MINOR_VERSION       (0x00U)
#define FRTP_CFG_SW_PATCH_VERSION       (0x00U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/* Development error detection */
#ifndef FRTP_DEV_ERROR_DETECT
#define FRTP_DEV_ERROR_DETECT           STD_ON
#endif

/* Version info API */
#ifndef FRTP_VERSION_INFO_API
#define FRTP_VERSION_INFO_API           STD_ON
#endif

/* Maximum number of connections */
#ifndef FRTP_MAX_CONNECTIONS
#define FRTP_MAX_CONNECTIONS            (4U)
#endif

/* Maximum payload size per frame (FlexRay typical: 254 bytes) */
#ifndef FRTP_MAX_PAYLOAD_PER_FRAME
#define FRTP_MAX_PAYLOAD_PER_FRAME      (254U)
#endif

/* Maximum SF payload (Single Frame) */
#ifndef FRTP_MAX_SF_PAYLOAD
#define FRTP_MAX_SF_PAYLOAD             (254U)
#endif

/* Maximum FF payload (First Frame) */
#ifndef FRTP_MAX_FF_PAYLOAD
#define FRTP_MAX_FF_PAYLOAD             (254U)
#endif

/* Maximum CF payload (Consecutive Frame) */
#ifndef FRTP_MAX_CF_PAYLOAD
#define FRTP_MAX_CF_PAYLOAD             (254U)
#endif

/* Default N_As timeout (Tx confirmation) in ms */
#ifndef FRTP_DEFAULT_NAS_TIMEOUT
#define FRTP_DEFAULT_NAS_TIMEOUT        (100U)
#endif

/* Default N_Bs timeout (Rx FC) in ms */
#ifndef FRTP_DEFAULT_NBS_TIMEOUT
#define FRTP_DEFAULT_NBS_TIMEOUT        (100U)
#endif

/* Default N_Cs timeout (CF transmission) in ms */
#ifndef FRTP_DEFAULT_NCS_TIMEOUT
#define FRTP_DEFAULT_NCS_TIMEOUT        (100U)
#endif

/* Default N_Ar timeout (Rx confirmation) in ms */
#ifndef FRTP_DEFAULT_NAR_TIMEOUT
#define FRTP_DEFAULT_NAR_TIMEOUT        (100U)
#endif

/* Default N_Br timeout (Buffer request) in ms */
#ifndef FRTP_DEFAULT_NBR_TIMEOUT
#define FRTP_DEFAULT_NBR_TIMEOUT        (100U)
#endif

/* Default N_Cr timeout (CF reception) in ms */
#ifndef FRTP_DEFAULT_NCR_TIMEOUT
#define FRTP_DEFAULT_NCR_TIMEOUT        (100U)
#endif

/* Default block size for flow control */
#ifndef FRTP_DEFAULT_BLOCK_SIZE
#define FRTP_DEFAULT_BLOCK_SIZE         (8U)
#endif

/* Default STmin (separation time minimum) in ms */
#ifndef FRTP_DEFAULT_STMIN
#define FRTP_DEFAULT_STMIN              (10U)
#endif

/* Maximum retry count for failed transmissions */
#ifndef FRTP_MAX_RETRY_COUNT
#define FRTP_MAX_RETRY_COUNT            (3U)
#endif

/* Enable flow control by default */
#ifndef FRTP_FLOW_CONTROL_ENABLED
#define FRTP_FLOW_CONTROL_ENABLED       STD_ON
#endif

/* Main function period in ms */
#ifndef FRTP_MAIN_FUNCTION_PERIOD
#define FRTP_MAIN_FUNCTION_PERIOD       (5U)
#endif

/*==================================================================================================
*                                    PDU TYPE ENCODING
==================================================================================================*/
/* PCI (Protocol Control Information) byte layout:
 * Bit 7-6: PDU Type (00=SF, 01=FF, 10=CF, 11=FC)
 * Bit 5-0: Additional info (length, sequence number, etc.)
 */

#define FRTP_PCI_TYPE_MASK              (0xC0U)
#define FRTP_PCI_TYPE_SF                (0x00U)  /* Single Frame: 00 xxxxxx */
#define FRTP_PCI_TYPE_FF                (0x40U)  /* First Frame: 01 xxxxxx */
#define FRTP_PCI_TYPE_CF                (0x80U)  /* Consecutive Frame: 10 xxxxxx */
#define FRTP_PCI_TYPE_FC                (0xC0U)  /* Flow Control: 11 xxxxxx */

#define FRTP_PCI_SF_LENGTH_MASK         (0x3FU)  /* SF length in lower 6 bits */
#define FRTP_PCI_CF_SEQ_MASK            (0x0FU)  /* CF sequence number in lower 4 bits */
#define FRTP_PCI_FC_STATUS_MASK         (0x0FU)  /* FC status in lower 4 bits */

/* Flow Control Status */
#define FRTP_FC_STATUS_CTS              (0x00U)  /* Clear To Send */
#define FRTP_FC_STATUS_WAIT             (0x01U)  /* Wait */
#define FRTP_FC_STATUS_OVFLW            (0x02U)  /* Overflow/Abort */

/*==================================================================================================
*                                    STATE DEFINITIONS
==================================================================================================*/
/* Connection state flags */
#define FRTP_FLAG_NONE                  (0x00U)
#define FRTP_FLAG_TX_ACTIVE             (0x01U)
#define FRTP_FLAG_RX_ACTIVE             (0x02U)
#define FRTP_FLAG_FC_SENT               (0x04U)
#define FRTP_FLAG_WAIT_CONFIRM          (0x08U)
#define FRTP_FLAG_LAST_CF               (0x10U)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/
/* Notification callbacks enable */
#define FRTP_TX_CONFIRMATION_ENABLED    STD_ON
#define FRTP_RX_INDICATION_ENABLED      STD_ON

#endif /* FRTP_CFG_H */
