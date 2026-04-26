/******************************************************************************
 * @file    EthIf_Cfg.h
 * @brief   Ethernet Interface Configuration
 *
 * Configuration parameters for the Ethernet Interface module.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-B Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef ETHIF_CFG_H
#define ETHIF_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Module Version Information (from configuration)
 ******************************************************************************/
#define ETHIF_CFG_VENDOR_ID             0x01U
#define ETHIF_CFG_MODULE_ID             0x34U
#define ETHIF_CFG_SW_MAJOR_VERSION      1U
#define ETHIF_CFG_SW_MINOR_VERSION      0U
#define ETHIF_CFG_SW_PATCH_VERSION      0U

/******************************************************************************
 * General Configuration
 ******************************************************************************/

/* Development error detection */
#define ETHIF_DEV_ERROR_DETECT          STD_ON

/* Version info API */
#define ETHIF_VERSION_INFO_API          STD_ON

/* API to get controller mode */
#define ETHIF_GET_CONTROLLER_MODE_API   STD_ON

/* API to get controller index */
#define ETHIF_GET_CTRL_IDX_API          STD_ON

/* API to get/set physical address */
#define ETHIF_PHYS_ADDR_API             STD_ON

/* API for broadcast address */
#define ETHIF_GET_BROADCAST_API         STD_ON

/* Transmit buffering support */
#define ETHIF_ENABLE_TX_BUFFERING       STD_ON

/* Receive buffering support */
#define ETHIF_ENABLE_RX_BUFFERING       STD_ON

/* VLAN processing support */
#define ETHIF_ENABLE_VLAN_PROCESSING    STD_ON

/* Time synchronization support (TSN/gPTP) */
#define ETHIF_ENABLE_TIMESTAMP          STD_ON

/* Wake-up support */
#define ETHIF_WAKEUP_SUPPORT            STD_ON

/* Main function period in milliseconds */
#define ETHIF_MAIN_FUNCTION_PERIOD_MS   10U

/******************************************************************************
 * Controller Configuration
 ******************************************************************************/

/* Number of configured Ethernet controllers */
#define ETHIF_NUM_CONTROLLERS           2U

/* Number of virtual controllers */
#define ETHIF_NUM_VIRT_CTRLS            4U

/* Total number of PDUs */
#define ETHIF_NUM_PDUS                  16U

/* Controller 0 - Main Ethernet */
#define ETHIF_CTRL0_ENABLED             STD_ON
#define ETHIF_CTRL0_VIRT_CTRLS          2U
#define ETHIF_CTRL0_MAC_ADDR            {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U}
#define ETHIF_CTRL0_MTU                 1500U
#define ETHIF_CTRL0_SPEED               1000U  /* 1 Gbps */
#define ETHIF_CTRL0_PROMISCUOUS         STD_OFF
#define ETHIF_CTRL0_ACCEPT_BROADCAST    STD_ON
#define ETHIF_CTRL0_ACCEPT_MULTICAST    STD_ON

/* Controller 1 - Redundant/Secondary */
#define ETHIF_CTRL1_ENABLED             STD_OFF
#define ETHIF_CTRL1_VIRT_CTRLS          0U
#define ETHIF_CTRL1_MAC_ADDR            {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x56U}
#define ETHIF_CTRL1_MTU                 1500U
#define ETHIF_CTRL1_SPEED               1000U
#define ETHIF_CTRL1_PROMISCUOUS         STD_OFF
#define ETHIF_CTRL1_ACCEPT_BROADCAST    STD_ON
#define ETHIF_CTRL1_ACCEPT_MULTICAST    STD_OFF

/******************************************************************************
 * Virtual Controller Configuration
 ******************************************************************************/

/* Virtual Controller 0 - SoAd Communication */
#define ETHIF_VIRTCTRL0_ENABLED         STD_ON
#define ETHIF_VIRTCTRL0_PHYS_CTRL       0U
#define ETHIF_VIRTCTRL0_PRIORITY        3U
#define ETHIF_VIRTCTRL0_VLAN_ENABLED    STD_ON
#define ETHIF_VIRTCTRL0_VLAN_ID         100U
#define ETHIF_VIRTCTRL0_NUM_PDUS        8U

/* Virtual Controller 1 - Diagnostics (DoIP) */
#define ETHIF_VIRTCTRL1_ENABLED         STD_ON
#define ETHIF_VIRTCTRL1_PHYS_CTRL       0U
#define ETHIF_VIRTCTRL1_PRIORITY        6U
#define ETHIF_VIRTCTRL1_VLAN_ENABLED    STD_ON
#define ETHIF_VIRTCTRL1_VLAN_ID         200U
#define ETHIF_VIRTCTRL1_NUM_PDUS        4U

/******************************************************************************
 * PDU Configuration
 ******************************************************************************/

/* PDU 0-7: SoAd Communication */
#define ETHIF_PDU0_ENABLED              STD_ON
#define ETHIF_PDU0_VIRT_CTRL            0U
#define ETHIF_PDU0_FRAME_TYPE           0x0800U  /* IPv4 */
#define ETHIF_PDU0_PRIORITY             3U
#define ETHIF_PDU0_BUFFER_SIZE          1536U

#define ETHIF_PDU1_ENABLED              STD_ON
#define ETHIF_PDU1_VIRT_CTRL            0U
#define ETHIF_PDU1_FRAME_TYPE           0x86DDU  /* IPv6 */
#define ETHIF_PDU1_PRIORITY             3U
#define ETHIF_PDU1_BUFFER_SIZE          1536U

/* PDU 8-11: Diagnostics */
#define ETHIF_PDU8_ENABLED              STD_ON
#define ETHIF_PDU8_VIRT_CTRL            1U
#define ETHIF_PDU8_FRAME_TYPE           0x0800U  /* IPv4 */
#define ETHIF_PDU8_PRIORITY             6U
#define ETHIF_PDU8_BUFFER_SIZE          1536U

/******************************************************************************
 * Transmit Queue Configuration
 ******************************************************************************/

/* Transmit queue depth per controller */
#define ETHIF_TX_QUEUE_DEPTH            16U

/* Maximum pending transmissions */
#define ETHIF_MAX_PENDING_TX            32U

/* Enable priority queuing */
#define ETHIF_PRIORITY_QUEUING          STD_ON

/* Number of priority queues */
#define ETHIF_NUM_PRIORITY_QUEUES       8U

/******************************************************************************
 * Buffer Configuration
 ******************************************************************************/

/* Receive buffer size per controller */
#define ETHIF_RX_BUFFER_SIZE            1536U

/* Number of receive buffers */
#define ETHIF_NUM_RX_BUFFERS            8U

/* Transmit buffer size */
#define ETHIF_TX_BUFFER_SIZE            1536U

/* Number of transmit buffers */
#define ETHIF_NUM_TX_BUFFERS            16U

/******************************************************************************
 * VLAN Configuration
 ******************************************************************************/

/* Default VLAN ID for untagged frames */
#define ETHIF_DEFAULT_VLAN_ID           1U

/* VLAN priority mask */
#define ETHIF_VLAN_PRIORITY_MASK        0xE000U

/* VLAN ID mask */
#define ETHIF_VLAN_ID_MASK              0x0FFFU

/******************************************************************************
 * Timeout Configuration
 ******************************************************************************/

/* Transmission timeout in milliseconds */
#define ETHIF_TX_TIMEOUT_MS             100U

/* Buffer request timeout in milliseconds */
#define ETHIF_BUFREQ_TIMEOUT_MS         50U

/* Controller mode change timeout */
#define ETHIF_MODE_CHANGE_TIMEOUT_MS    1000U

/******************************************************************************
 * DEM Event IDs
 ******************************************************************************/

/* DEM Event IDs - to be configured with actual Dem IDs */
#define ETHIF_DEM_EVENT_INIT_FAILED     0x3401U
#define ETHIF_DEM_EVENT_TX_FAILED       0x3402U
#define ETHIF_DEM_EVENT_RX_FAILED       0x3403U
#define ETHIF_DEM_EVENT_CTRL_ERROR      0x3404U
#define ETHIF_DEM_EVENT_BUFFER_OVERFLOW 0x3405U
#define ETHIF_DEM_EVENT_TIMESTAMP_ERROR 0x3406U

/******************************************************************************
 * Callback Configuration
 ******************************************************************************/

/* Enable upper layer RxIndication callback */
#define ETHIF_UL_RXINDICATION_CALLBACK  STD_ON

/* Enable upper layer TxConfirmation callback */
#define ETHIF_UL_TXCONFIRMATION_CALLBACK    STD_ON

/* Enable controller mode indication callback */
#define ETHIF_UL_MODE_INDICATION_CALLBACK   STD_ON

/******************************************************************************
 * Hardware Platform Selection
 ******************************************************************************/

/* Select target hardware platform */
/* #define ETHIF_TARGET_AURIX_TC3XX */
/* #define ETHIF_TARGET_AURIX_TC4XX */
/* #define ETHIF_TARGET_S32K3XX */
/* #define ETHIF_TARGET_S32G3 */
#define ETHIF_TARGET_EMULATOR           /* For testing */

/******************************************************************************
 * Feature Switches
 ******************************************************************************/

/* Enable TSN features */
#define ETHIF_TSN_SUPPORT               STD_ON

/* Enable CBS (Credit Based Shaper) */
#define ETHIF_CBS_SUPPORT               STD_ON

/* Enable TAS (Time Aware Shaper) */
#define ETHIF_TAS_SUPPORT               STD_ON

/* Enable Frame Preemption */
#define ETHIF_FRAME_PREEMPTION_SUPPORT  STD_ON

/* Enable Cut-Through forwarding */
#define ETHIF_CUT_THROUGH_SUPPORT       STD_OFF

/******************************************************************************
 * Multicast Configuration
 ******************************************************************************/

/* Maximum multicast addresses per controller */
#define ETHIF_MAX_MULTICAST_ADDRS       16U

/* Enable IGMP snooping */
#define ETHIF_IGMP_SNOOPING             STD_ON

/******************************************************************************
 * Statistics Configuration
 ******************************************************************************/

/* Enable statistics collection */
#define ETHIF_STATISTICS_ENABLED        STD_ON

/* Statistics update period */
#define ETHIF_STATS_PERIOD_MS           1000U

#ifdef __cplusplus
}
#endif

#endif /* ETHIF_CFG_H */
