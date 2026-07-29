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
 * @file LinTp_Cfg.h
 * @brief LIN Transport Layer configuration header
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef LINTP_CFG_H
#define LINTP_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define LINTP_DEV_ERROR_DETECT              (STD_ON)
#define LINTP_VERSION_INFO_API              (STD_ON)
#define LINTP_CHANGE_PARAMETER_API          (STD_ON)
#define LINTP_CANCEL_TRANSMIT_API           (STD_ON)
#define LINTP_CANCEL_RECEIVE_API            (STD_ON)
#define LINTP_TC                            (STD_ON)    /*!< Transport Protocol for Class C */

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define LINTP_NUMBER_OF_CHANNELS            (1U)
#define LINTP_NUMBER_OF_CONNECTIONS         (2U)
#define LINTP_NUMBER_OF_PDUS                (4U)

/*==================================================================================================
*                                    CHANNEL IDs
==================================================================================================*/
#define LINTP_CHANNEL_0                     (0U)

/*==================================================================================================
*                                    CONNECTION IDs
==================================================================================================*/
#define LINTP_CONNECTION_0                  (0U)
#define LINTP_CONNECTION_1                  (1U)

/*==================================================================================================
*                                    PDU IDs
==================================================================================================*/
#define LINTP_PDU_TX_DIAGNOSTIC             (0U)
#define LINTP_PDU_RX_DIAGNOSTIC             (1U)
#define LINTP_PDU_TX_FUNCTIONAL             (2U)
#define LINTP_PDU_RX_FUNCTIONAL             (3U)

/*==================================================================================================
*                                    NAD (NODE ADDRESS) CONFIGURATION
==================================================================================================*/
#define LINTP_NAD_BROADCAST                 (0x7FU)
#define LINTP_NAD_DIAGNOSTIC                (0x01U)
#define LINTP_NAD_FUNCTIONAL                (0x7EU)
#define LINTP_NAD_DEFAULT                   (0x00U)

/*==================================================================================================
*                                    TIMING CONFIGURATION (CONFIGURABLE)
==================================================================================================*/
#define LINTP_DEFAULT_N_AS_MS               (100U)
#define LINTP_DEFAULT_N_CR_MS               (100U)
#define LINTP_DEFAULT_STMIN_MS              (10U)
#define LINTP_N_AS_TIMEOUT_COUNT            (LINTP_DEFAULT_N_AS_MS / 5U)
#define LINTP_N_CR_TIMEOUT_COUNT            (LINTP_DEFAULT_N_CR_MS / 5U)

/*==================================================================================================
*                                    MESSAGE SIZE CONFIGURATION
==================================================================================================*/
#define LINTP_MAX_MESSAGE_LENGTH            (4095U)
#define LINTP_SF_MAX_DATA_LENGTH            (6U)
#define LINTP_FF_DATA_LENGTH                (5U)
#define LINTP_CF_DATA_LENGTH                (6U)
#define LINTP_FRAME_SIZE                    (8U)
#define LINTP_PCI_SIZE                      (1U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define LINTP_MAIN_FUNCTION_PERIOD_MS       (5U)

/*==================================================================================================
*                                    SF/FF/CF MASKS
==================================================================================================*/
#define LINTP_PCI_MASK                      (0xF0U)
#define LINTP_PCI_SF                        (0x00U)
#define LINTP_PCI_FF                        (0x10U)
#define LINTP_PCI_CF                        (0x20U)
#define LINTP_PCI_TYPE_MASK                 (0xF0U)
#define LINTP_PCI_DL_MASK                   (0x0FU)
#define LINTP_PCI_SN_MASK                   (0x0FU)

/*==================================================================================================
*                                    SN (SEQUENCE NUMBER)
==================================================================================================*/
#define LINTP_SN_MAX                        (0x0FU)
#define LINTP_SN_FIRST_CF                   (0x01U)

#endif /* LINTP_CFG_H */
