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
 * @file EthSM_Lcfg.c
 * @brief Ethernet State Manager Link-Time Configuration
 */

#include "EthSM.h"
#include "EthSM_Cfg.h"

extern const EthSM_ConfigType EthSM_Config;
static const EthSM_ChannelConfigType EthSM_Channels[ETHSM_MAX_NETWORKS] = {
    { ETHSM_NETWORK_ETH0, ETHSM_MAIN_FUNCTION_PERIOD },
    { ETHSM_NETWORK_ETH1, ETHSM_MAIN_FUNCTION_PERIOD },
    { 0xFFU, 0U },  /* Unused */
    { 0xFFU, 0U }   /* Unused */
};

static const EthSM_ConfigType EthSM_Config = {
    .NumChannels = 2U,
    .Channels = EthSM_Channels
};
