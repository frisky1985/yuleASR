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
 * @file Eth_Cfg.h
 * @brief Ethernet Driver Configuration
 */

#ifndef ETH_CFG_H
#define ETH_CFG_H

#include "Eth.h"

/* Development Error Detection */
#define ETH_DEV_ERROR_DETECT               STD_ON
#define ETH_VERSION_INFO_API               STD_ON

/* Number of Controllers */
#define ETH_MAX_CONTROLLERS                1U

/* Controller IDs */
#define ETH_CONTROLLER_0                   0x00

/* MAC Address for Controller 0 */
#define ETH_CTRL0_MAC_ADDR                 {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}

/* Speed Configuration */
#define ETH_CTRL0_SPEED                    ETH_RATE_100MBPS
#define ETH_CTRL0_FULL_DUPLEX              STD_ON

/* Offload Features */
#define ETH_CTRL0_RX_CHECKSUM_OFFLOAD      STD_OFF
#define ETH_CTRL0_TX_CHECKSUM_OFFLOAD      STD_OFF

/* Buffer Configuration */
#define ETH_MAX_TX_BUFS                    8U
#define ETH_MAX_RX_BUFS                    8U
#define ETH_CFG_BUF_SIZE                   1536

/* Timeouts */
#define ETH_TIMEOUT                        1000
#define ETH_LINK_TIMEOUT                   5000

/* Features */
#define ETH_MULTICAST_SUPPORT              STD_ON
#define ETH_PROMISCUOUS_MODE               STD_OFF

#endif /* ETH_CFG_H */
