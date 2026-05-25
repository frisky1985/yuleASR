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
 * @file EthSM_Cfg.h
 * @brief Ethernet State Manager Configuration
 */

#ifndef ETHSM_CFG_H
#define ETHSM_CFG_H

#define ETHSM_DEV_ERROR_DETECT      STD_ON
#define ETHSM_VERSION_INFO_API      STD_ON

#define ETHSM_MAX_NETWORKS          4U
#define ETHSM_MAIN_FUNCTION_PERIOD  10U  /* ms */

/* Network Handles */
#define ETHSM_NETWORK_ETH0          0U
#define ETHSM_NETWORK_ETH1          1U

#endif
