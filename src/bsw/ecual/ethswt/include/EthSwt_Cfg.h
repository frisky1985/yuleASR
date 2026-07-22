/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file EthSwt_Cfg.h
 * @brief Ethernet Switch Pre-Compile Configuration
 * @version 1.0.0
 */

#ifndef ETHSWT_CFG_H
#define ETHSWT_CFG_H

/*==================================================================================================
 *                                    PRE-COMPILE SWITCHES
 *==================================================================================================*/
#define ETHSWT_DEV_ERROR_DETECT                 (STD_ON)
#define ETHSWT_VERSION_INFO_API                 (STD_ON)

/*==================================================================================================
 *                                    PORT CONFIGURATION
 *==================================================================================================*/
#define ETHSWT_MAX_PORTS                        (8U)
#define ETHSWT_MAX_VLANS                        (16U)
#define ETHSWT_MAX_MAC_FILTERS                  (32U)

/*==================================================================================================
 *                                    BUFFER CONFIGURATION
 *==================================================================================================*/
#define ETHSWT_FRAME_BUFFER_SIZE                (1600U)
#define ETHSWT_TX_QUEUE_DEPTH                   (64U)
#define ETHSWT_RX_QUEUE_DEPTH                   (64U)

/*==================================================================================================
 *                                    TIMING CONFIGURATION
 *==================================================================================================*/
#define ETHSWT_MAIN_FUNCTION_PERIOD_MS          (10U)
#define ETHSWT_LINK_POLL_INTERVAL_MS            (100U)

/*==================================================================================================
 *                                    FEATURE FLAGS
 *==================================================================================================*/
#define ETHSWT_ENABLE_VLAN                      (STD_ON)
#define ETHSWT_ENABLE_MAC_FILTERING             (STD_ON)
#define ETHSWT_ENABLE_PORT_STATS                (STD_ON)
#define ETHSWT_ENABLE_AUTO_NEGOTIATION          (STD_ON)

/*==================================================================================================
 *                                    DEFAULT VALUES
 *==================================================================================================*/
#define ETHSWT_DEFAULT_SPEED                    ETHSWT_SPEED_AUTO
#define ETHSWT_DEFAULT_DUPLEX                   ETHSWT_DUPLEX_FULL
#define ETHSWT_DEFAULT_MTU                      (1500U)

#endif /* ETHSWT_CFG_H */
