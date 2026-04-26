/******************************************************************************
 * @file    EthTrcv_Cfg.h
 * @brief   Ethernet Transceiver (EthTrcv) Configuration - AUTOSAR R22-11
 *
 * This file contains the configuration parameters for the EthTrcv module.
 * It should be generated based on the AUTOSAR configuration (ARXML).
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x35 (EthTrcv)
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * Configuration for: TJA1101 100BASE-T1 Automotive Ethernet PHY
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ETHTRCV_CFG_H
#define ETHTRCV_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Pre-compile Configuration
 ******************************************************************************/

/* Development error detection */
#ifndef ETHTRCV_DEV_ERROR_DETECT
#define ETHTRCV_DEV_ERROR_DETECT        STD_ON
#endif

/* Version info API */
#ifndef ETHTRCV_VERSION_INFO_API
#define ETHTRCV_VERSION_INFO_API        STD_ON
#endif

/* Wake-up support */
#ifndef ETHTRCV_WAKEUP_SUPPORT
#define ETHTRCV_WAKEUP_SUPPORT          STD_ON
#endif

/* Link monitoring support */
#ifndef ETHTRCV_LINK_MONITORING_SUPPORT
#define ETHTRCV_LINK_MONITORING_SUPPORT STD_ON
#endif

/* PHY test mode support */
#ifndef ETHTRCV_PHY_TEST_MODE_SUPPORT
#define ETHTRCV_PHY_TEST_MODE_SUPPORT   STD_ON
#endif

/* PHY loopback mode support */
#ifndef ETHTRCV_PHY_LOOPBACK_SUPPORT
#define ETHTRCV_PHY_LOOPBACK_SUPPORT    STD_ON
#endif

/* PHY register access via MII/MDIO */
#ifndef ETHTRCV_PHY_MII_MDIO_SUPPORT
#define ETHTRCV_PHY_MII_MDIO_SUPPORT    STD_ON
#endif

/******************************************************************************
 * Number of Transceivers
 ******************************************************************************/
#define ETHTRCV_CFG_NUM_TRANSCEIVERS    2U

/******************************************************************************
 * Transceiver Configuration
 ******************************************************************************/

/* Transceiver 0: Primary 100BASE-T1 PHY (TJA1101) */
#define ETHTRCV_CFG_TRCV_0_IDX          0U
#define ETHTRCV_CFG_TRCV_0_PHY_ADDR     0x00U   /* PHY address = 0 */
#define ETHTRCV_CFG_TRCV_0_PHY_TYPE     ETHTRCV_PHY_TYPE_TJA1101
#define ETHTRCV_CFG_TRCV_0_ENABLED      STD_ON
#define ETHTRCV_CFG_TRCV_0_WAKEUP       STD_ON
#define ETHTRCV_CFG_TRCV_0_AUTONEG      STD_OFF /* 100BASE-T1 does not use autoneg */

/* Transceiver 1: Secondary 100BASE-T1 PHY (TJA1101) - optional */
#define ETHTRCV_CFG_TRCV_1_IDX          1U
#define ETHTRCV_CFG_TRCV_1_PHY_ADDR     0x01U   /* PHY address = 1 */
#define ETHTRCV_CFG_TRCV_1_PHY_TYPE     ETHTRCV_PHY_TYPE_TJA1101
#define ETHTRCV_CFG_TRCV_1_ENABLED      STD_OFF /* Disabled by default */
#define ETHTRCV_CFG_TRCV_1_WAKEUP       STD_ON
#define ETHTRCV_CFG_TRCV_1_AUTONEG      STD_OFF

/******************************************************************************
 * Timing Configuration
 ******************************************************************************/

/* Main function period in ms */
#define ETHTRCV_CFG_MAIN_FUNCTION_PERIOD_MS     10U

/* Link check period in ms */
#define ETHTRCV_CFG_LINK_CHECK_PERIOD_MS        100U

/* PHY reset timeout in ms */
#define ETHTRCV_CFG_PHY_RESET_TIMEOUT_MS        1000U

/* Link up timeout in ms */
#define ETHTRCV_CFG_LINK_UP_TIMEOUT_MS          5000U

/* PHY register access timeout in ms */
#define ETHTRCV_CFG_PHY_ACCESS_TIMEOUT_MS       100U

/* Mode transition timeout in ms */
#define ETHTRCV_CFG_MODE_TRANSITION_TIMEOUT_MS  500U

/******************************************************************************
 * PHY Configuration (TJA1101 specific)
 ******************************************************************************/

/* TJA1101 PHY ID */
#define ETHTRCV_CFG_TJA1101_ID1         0x0180U /* OUI bits 3-18 */
#define ETHTRCV_CFG_TJA1101_ID2         0xDC00U /* OUI bits 19-24 + Model + Revision */
#define ETHTRCV_CFG_TJA1101_ID2_MASK    0xFFF0U /* Mask for model/revision check */

/* Extended Control Register default value */
#define ETHTRCV_CFG_TJA1101_ECTRL_DEFAULT   0x8000U /* Link control enabled */

/* Configuration Register 1 default value */
#define ETHTRCV_CFG_TJA1101_CONFIG1_DEFAULT 0x0000U

/* Interrupt Enable Register default value */
#define ETHTRCV_CFG_TJA1101_INT_ENABLE_DEFAULT  0x0000U

/******************************************************************************
 * Wake-up Configuration
 ******************************************************************************/

/* Wake-up source mask */
#define ETHTRCV_CFG_WAKEUP_SOURCE_MASK  ETH_WKSRC_WAKEUP_PIN

/* Wake-up filter configuration */
#define ETHTRCV_CFG_WAKEUP_FILTER_ENABLED   STD_OFF

/******************************************************************************
 * Link Monitoring Configuration
 ******************************************************************************/

/* Number of consecutive link down events before reporting */
#define ETHTRCV_CFG_LINK_DOWN_THRESHOLD     3U

/* Number of consecutive link up events before reporting */
#define ETHTRCV_CFG_LINK_UP_THRESHOLD       3U

/* Link debounce time in ms */
#define ETHTRCV_CFG_LINK_DEBOUNCE_MS        50U

/******************************************************************************
 * Error Handling Configuration
 ******************************************************************************/

/* DEM Event IDs for error reporting */
#define ETHTRCV_E_DEM_PHY_ACCESS_FAILED     0x01U
#define ETHTRCV_E_DEM_LINK_FAILURE          0x02U
#define ETHTRCV_E_DEM_WAKEUP_FAILURE        0x03U

/******************************************************************************
 * Port Configuration (for MCAL integration)
 ******************************************************************************/

/* MDIO interface port/pin configuration (platform specific) */
#define ETHTRCV_CFG_MDIO_PORT           0U
#define ETHTRCV_CFG_MDIO_PIN            0U
#define ETHTRCV_CFG_MDC_PORT            0U
#define ETHTRCV_CFG_MDC_PIN             1U

/* PHY reset pin configuration */
#define ETHTRCV_CFG_PHY_RESET_PORT      0U
#define ETHTRCV_CFG_PHY_RESET_PIN       2U

/* PHY interrupt pin configuration */
#define ETHTRCV_CFG_PHY_INT_PORT        0U
#define ETHTRCV_CFG_PHY_INT_PIN         3U

/******************************************************************************
 * Debug and Trace Configuration
 ******************************************************************************/

/* Enable debug logging */
#ifndef ETHTRCV_CFG_DEBUG_ENABLED
#define ETHTRCV_CFG_DEBUG_ENABLED       STD_OFF
#endif

/* Enable statistics collection */
#ifndef ETHTRCV_CFG_STATS_ENABLED
#define ETHTRCV_CFG_STATS_ENABLED       STD_ON
#endif

/******************************************************************************
 * Derived Configuration
 ******************************************************************************/

/* Maximum number of transceivers */
#if (ETHTRCV_CFG_NUM_TRANSCEIVERS > ETHTRCV_MAX_TRANSCEIVERS)
#error "ETHTRCV_CFG_NUM_TRANSCEIVERS exceeds ETHTRCV_MAX_TRANSCEIVERS"
#endif

/* Validate transceiver indices */
#if (ETHTRCV_CFG_TRCV_0_IDX >= ETHTRCV_MAX_TRANSCEIVERS)
#error "ETHTRCV_CFG_TRCV_0_IDX out of range"
#endif

#if (ETHTRCV_CFG_TRCV_1_IDX >= ETHTRCV_MAX_TRANSCEIVERS)
#error "ETHTRCV_CFG_TRCV_1_IDX out of range"
#endif

/* PHY address validation (5-bit address) */
#if (ETHTRCV_CFG_TRCV_0_PHY_ADDR > 0x1FU)
#error "ETHTRCV_CFG_TRCV_0_PHY_ADDR out of range (must be 0-31)"
#endif

#if (ETHTRCV_CFG_TRCV_1_PHY_ADDR > 0x1FU)
#error "ETHTRCV_CFG_TRCV_1_PHY_ADDR out of range (must be 0-31)"
#endif

#ifdef __cplusplus
}
#endif

#endif /* ETHTRCV_CFG_H */
