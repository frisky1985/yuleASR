/**
 * @file Eth_Cfg.h
 * @brief Eth (Ethernet Driver) Configuration
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Eth Module - Configuration Parameters
 * Module ID: 0x11
 *
 * This file contains all configurable parameters for the Eth module.
 * Modify according to your hardware and application requirements.
 */

#ifndef ETH_CFG_H
#define ETH_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Eth_Types.h"

/*============================================================================*
 * General Configuration
 *============================================================================*/

/* Number of Ethernet controllers */
#define ETH_CFG_CONTROLLER_COUNT        2U

/* Main function period in milliseconds */
#define ETH_CFG_MAIN_FUNCTION_PERIOD    10U

/* Development error detection */
#define ETH_CFG_DEV_ERROR_DETECT        STD_ON

/* Version info API */
#define ETH_CFG_VERSION_INFO_API        STD_ON

/* Global time (PTP) support */
#define ETH_CFG_GLOBAL_TIME_SUPPORT     STD_OFF

/* Timestamp support */
#define ETH_CFG_TIMESTAMP_SUPPORT       STD_ON

/* DMA software buffer management */
#define ETH_CFG_DMA_SW_BUFFER           STD_ON

/* Allow runtime configuration changes */
#define ETH_CFG_RUNTIME_CONFIG          STD_OFF

/* Enable statistics counters */
#define ETH_CFG_STATISTICS              STD_ON

/* Enable flow control support */
#define ETH_CFG_FLOW_CONTROL            STD_ON

/* Enable VLAN support */
#define ETH_CFG_VLAN_SUPPORT            STD_ON

/* Enable multicast filtering */
#define ETH_CFG_MULTICAST_FILTER        STD_ON

/*============================================================================*
 * Controller 0 Configuration
 *============================================================================*/

/* Hardware type */
#define ETH_CFG_CTRL0_HW_TYPE           ETH_HW_GENERIC

/* Base addresses */
#define ETH_CFG_CTRL0_BASE_ADDR         0x40000000U
#define ETH_CFG_CTRL0_DMA_BASE_ADDR     0x40001000U

/* MAC address (locally administered) */
#define ETH_CFG_CTRL0_MAC_ADDR          {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U}

/* PHY configuration */
#define ETH_CFG_CTRL0_PHY_TYPE          ETH_PHY_TJA1101
#define ETH_CFG_CTRL0_PHY_INTERFACE     ETH_PHY_IF_RMII
#define ETH_CFG_CTRL0_PHY_ADDRESS       0x01U
#define ETH_CFG_CTRL0_MDC_CLOCK         2500000U    /* 2.5 MHz */
#define ETH_CFG_CTRL0_MDIO_TIMEOUT      1000U       /* 1 ms */

/* Link configuration */
#define ETH_CFG_CTRL0_SPEED             ETH_SPEED_100M
#define ETH_CFG_CTRL0_DUPLEX            ETH_DUPLEX_FULL
#define ETH_CFG_CTRL0_AUTO_NEGOTIATION  STD_ON
#define ETH_CFG_CTRL0_LOOPBACK          STD_OFF

/* DMA configuration */
#define ETH_CFG_CTRL0_RX_DESC_COUNT     16U
#define ETH_CFG_CTRL0_TX_DESC_COUNT     16U
#define ETH_CFG_CTRL0_RX_BUFFER_SIZE    1522U
#define ETH_CFG_CTRL0_TX_BUFFER_SIZE    1522U

/* Interrupt configuration */
#define ETH_CFG_CTRL0_IRQ_MASK          (ETH_IRQ_TX_COMPLETE | \
                                         ETH_IRQ_RX_COMPLETE | \
                                         ETH_IRQ_TX_ERROR | \
                                         ETH_IRQ_RX_ERROR | \
                                         ETH_IRQ_DMA_ERROR | \
                                         ETH_IRQ_PHY_EVENT)
#define ETH_CFG_CTRL0_IRQ_PRIORITY      5U
#define ETH_CFG_CTRL0_IRQ_VECTOR        20U

/* Flow control */
#define ETH_CFG_CTRL0_FLOW_CTRL_ENABLE  STD_ON
#define ETH_CFG_CTRL0_PAUSE_TIME        0xFFFFU

/*============================================================================*
 * Controller 1 Configuration
 *============================================================================*/

/* Hardware type */
#define ETH_CFG_CTRL1_HW_TYPE           ETH_HW_GENERIC

/* Base addresses */
#define ETH_CFG_CTRL1_BASE_ADDR         0x50000000U
#define ETH_CFG_CTRL1_DMA_BASE_ADDR     0x50001000U

/* MAC address (locally administered) */
#define ETH_CFG_CTRL1_MAC_ADDR          {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U}

/* PHY configuration */
#define ETH_CFG_CTRL1_PHY_TYPE          ETH_PHY_DP83825I
#define ETH_CFG_CTRL1_PHY_INTERFACE     ETH_PHY_IF_RMII
#define ETH_CFG_CTRL1_PHY_ADDRESS       0x00U
#define ETH_CFG_CTRL1_MDC_CLOCK         2500000U
#define ETH_CFG_CTRL1_MDIO_TIMEOUT      1000U

/* Link configuration */
#define ETH_CFG_CTRL1_SPEED             ETH_SPEED_100M
#define ETH_CFG_CTRL1_DUPLEX            ETH_DUPLEX_FULL
#define ETH_CFG_CTRL1_AUTO_NEGOTIATION  STD_ON
#define ETH_CFG_CTRL1_LOOPBACK          STD_OFF

/* DMA configuration */
#define ETH_CFG_CTRL1_RX_DESC_COUNT     16U
#define ETH_CFG_CTRL1_TX_DESC_COUNT     16U
#define ETH_CFG_CTRL1_RX_BUFFER_SIZE    1522U
#define ETH_CFG_CTRL1_TX_BUFFER_SIZE    1522U

/* Interrupt configuration */
#define ETH_CFG_CTRL1_IRQ_MASK          (ETH_IRQ_TX_COMPLETE | \
                                         ETH_IRQ_RX_COMPLETE | \
                                         ETH_IRQ_TX_ERROR | \
                                         ETH_IRQ_RX_ERROR | \
                                         ETH_IRQ_DMA_ERROR | \
                                         ETH_IRQ_PHY_EVENT)
#define ETH_CFG_CTRL1_IRQ_PRIORITY      5U
#define ETH_CFG_CTRL1_IRQ_VECTOR        21U

/* Flow control */
#define ETH_CFG_CTRL1_FLOW_CTRL_ENABLE  STD_ON
#define ETH_CFG_CTRL1_PAUSE_TIME        0xFFFFU

/*============================================================================*
 * PHY-Specific Configuration
 *============================================================================*/

/* TJA1101 (NXP 100BASE-T1) specific */
#define ETH_CFG_PHY_TJA1101_ENABLE_WUP  STD_ON   /* Wake-up pulse detection */
#define ETH_CFG_PHY_TJA1101_CONFIG_INI  STD_OFF  /* Configuration via INI pin */

/* DP83825I (TI 100BASE-TX) specific */
#define ETH_CFG_PHY_DP83825I_LED_MODE   0U       /* LED mode configuration */
#define ETH_CFG_PHY_DP83825I_CLK_DELAY  2U       /* Clock delay setting */

/*============================================================================*
 * Standard Macros
 *============================================================================*/

#ifndef STD_ON
#define STD_ON      1U
#endif

#ifndef STD_OFF
#define STD_OFF     0U
#endif

/*============================================================================*
 * Configuration Validation
 *============================================================================*/

/* Validate descriptor counts */
#if (ETH_CFG_CTRL0_RX_DESC_COUNT > ETH_MAX_RX_DESCRIPTORS)
#error "ETH_CFG_CTRL0_RX_DESC_COUNT exceeds ETH_MAX_RX_DESCRIPTORS"
#endif

#if (ETH_CFG_CTRL0_TX_DESC_COUNT > ETH_MAX_TX_DESCRIPTORS)
#error "ETH_CFG_CTRL0_TX_DESC_COUNT exceeds ETH_MAX_TX_DESCRIPTORS"
#endif

#if (ETH_CFG_CTRL1_RX_DESC_COUNT > ETH_MAX_RX_DESCRIPTORS)
#error "ETH_CFG_CTRL1_RX_DESC_COUNT exceeds ETH_MAX_RX_DESCRIPTORS"
#endif

#if (ETH_CFG_CTRL1_TX_DESC_COUNT > ETH_MAX_TX_DESCRIPTORS)
#error "ETH_CFG_CTRL1_TX_DESC_COUNT exceeds ETH_MAX_TX_DESCRIPTORS"
#endif

/* Validate controller count */
#if (ETH_CFG_CONTROLLER_COUNT > ETH_MAX_CONTROLLERS)
#error "ETH_CFG_CONTROLLER_COUNT exceeds ETH_MAX_CONTROLLERS"
#endif

/* Validate buffer sizes */
#if (ETH_CFG_CTRL0_RX_BUFFER_SIZE < ETH_MIN_PAYLOAD_SIZE)
#error "ETH_CFG_CTRL0_RX_BUFFER_SIZE too small"
#endif

#if (ETH_CFG_CTRL0_TX_BUFFER_SIZE < ETH_MIN_PAYLOAD_SIZE)
#error "ETH_CFG_CTRL0_TX_BUFFER_SIZE too small"
#endif

#if (ETH_CFG_CTRL1_RX_BUFFER_SIZE < ETH_MIN_PAYLOAD_SIZE)
#error "ETH_CFG_CTRL1_RX_BUFFER_SIZE too small"
#endif

#if (ETH_CFG_CTRL1_TX_BUFFER_SIZE < ETH_MIN_PAYLOAD_SIZE)
#error "ETH_CFG_CTRL1_TX_BUFFER_SIZE too small"
#endif

/* Validate PHY addresses */
#if (ETH_CFG_CTRL0_PHY_ADDRESS > 31U)
#error "ETH_CFG_CTRL0_PHY_ADDRESS out of range (0-31)"
#endif

#if (ETH_CFG_CTRL1_PHY_ADDRESS > 31U)
#error "ETH_CFG_CTRL1_PHY_ADDRESS out of range (0-31)"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ETH_CFG_H */
