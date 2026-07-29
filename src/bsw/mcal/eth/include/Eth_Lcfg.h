/**
 * @file Eth_Lcfg.h
 * @brief Ethernet Driver Link-Time Configuration Header
 * @version 1.0.0
 * 
 * Link-time configuration for Eth module.
 * 
 * @copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ETH_LCFG_H
#define ETH_LCFG_H

#include "Eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                              LINK-TIME CONFIGURATION MACROS
==================================================================================================*/

/* Number of configured controllers */
#define ETH_CFG_NUM_CONTROLLERS            ETH_MAX_CONTROLLERS

/* Controller 0 Configuration */
#if (ETH_MAX_CONTROLLERS > 0)
#define ETH_CFG_CTRL0_INDEX                0x00u
#define ETH_CFG_CTRL0_MAC_ADDR0            0x00u
#define ETH_CFG_CTRL0_MAC_ADDR1            0x01u
#define ETH_CFG_CTRL0_MAC_ADDR2            0x02u
#define ETH_CFG_CTRL0_MAC_ADDR3            0x03u
#define ETH_CFG_CTRL0_MAC_ADDR4            0x04u
#define ETH_CFG_CTRL0_MAC_ADDR5            0x05u
#define ETH_CFG_CTRL0_SPEED                ETH_CTRL0_SPEED
#define ETH_CFG_CTRL0_FULL_DUPLEX          ETH_CTRL0_FULL_DUPLEX
#define ETH_CFG_CTRL0_RX_CHECKSUM_OFFLOAD  ETH_CTRL0_RX_CHECKSUM_OFFLOAD
#define ETH_CFG_CTRL0_TX_CHECKSUM_OFFLOAD  ETH_CTRL0_TX_CHECKSUM_OFFLOAD
#define ETH_CFG_CTRL0_PHY_ADDRESS          0x00u
#define ETH_CFG_CTRL0_TX_BUF_COUNT         ETH_MAX_TX_BUFS
#define ETH_CFG_CTRL0_RX_BUF_COUNT         ETH_MAX_RX_BUFS
#define ETH_CFG_CTRL0_BUF_SIZE             ETH_MAX_FRAME_SIZE
#endif

/*==================================================================================================
*                              EXTERNAL CONFIGURATION DECLARATIONS
==================================================================================================*/

#define ETH_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Eth_ConfigType Eth_Config;
extern const Eth_ControllerConfigType Eth_ControllerConfig[ETH_MAX_CONTROLLERS];

#define ETH_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* ETH_LCFG_H */
