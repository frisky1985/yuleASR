/**
 * @file EthSM_Lcfg.c
 * @brief Ethernet State Manager Link-Time Configuration
 */

#include "EthSM.h"
#include "EthSM_Cfg.h"

static const EthSM_ChannelConfigType EthSM_Channels[ETHSM_MAX_NETWORKS] = {
    { ETHSM_NETWORK_ETH0, ETHSM_MAIN_FUNCTION_PERIOD },
    { ETHSM_NETWORK_ETH1, ETHSM_MAIN_FUNCTION_PERIOD },
    { 0xFFU, 0U },  /* Unused */
    { 0xFFU, 0U }   /* Unused */
};

const EthSM_ConfigType EthSM_Config = {
    .NumChannels = 2U,
    .Channels = EthSM_Channels
};
