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

/*==================================================================================================
 * File: EthTrcv_Cfg.h
 * Module: EthTrcv (Ethernet Transceiver Driver)
 * AUTOSAR Version: 4.4.0
 *==================================================================================================*/

#ifndef ETHTRCV_CFG_H
#define ETHTRCV_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                  MODULE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Pre-compile option for Version Info API
 * STD_ON: Version Info API enabled
 * STD_OFF: Version Info API disabled
 */
#define ETHTRCV_VERSION_INFO_API            STD_ON

/**
 * @brief Pre-compile option for Development Error Detection
 * STD_ON: Development error detection enabled
 * STD_OFF: Development error detection disabled
 */
#define ETHTRCV_DEV_ERROR_DETECT            STD_ON

/**
 * @brief Pre-compile option for Wake-up support
 * STD_ON: Wake-up support enabled
 * STD_OFF: Wake-up support disabled
 */
#define ETHTRCV_WAKEUP_SUPPORT              STD_ON

/**
 * @brief Pre-compile option for Cable Diagnostics support
 * STD_ON: Cable diagnostics enabled
 * STD_OFF: Cable diagnostics disabled
 */
#define ETHTRCV_CABLE_DIAGNOSTICS_SUPPORT   STD_ON

/**
 * @brief Pre-compile option for Signal Quality support
 * STD_ON: Signal quality measurement enabled
 * STD_OFF: Signal quality measurement disabled
 */
#define ETHTRCV_SIGNAL_QUALITY_SUPPORT      STD_ON

/**
 * @brief Pre-compile option for Auto-Negotiation
 * STD_ON: Auto-negotiation enabled by default
 * STD_OFF: Auto-negotiation disabled
 */
#define ETHTRCV_AUTO_NEGOTIATION_SUPPORT    STD_ON

/**
 * @brief Pre-compile option for MII/RMII/RGMII switching
 * STD_ON: Interface type switching supported
 * STD_OFF: Interface type switching not supported
 */
#define ETHTRCV_DYNAMIC_INTERFACE_SWITCH    STD_OFF

/*==================================================================================================
 *                                    GENERAL DEFINES
 *==================================================================================================*/

/**
 * @brief Maximum number of transceivers supported
 */
#define ETHTRCV_MAX_TRCV_SUPPORTED          (2U)

/**
 * @brief Number of configured transceivers
 */
#define ETHTRCV_NUMBER_OF_TRCVS             (2U)

/**
 * @brief Instance ID for DET reporting
 */
#define ETHTRCV_INSTANCE_ID                 (0U)

/**
 * @brief Main Function period in ms
 */
#define ETHTRCV_MAIN_FUNCTION_PERIOD_MS     (10U)

/**
 * @brief PHY access timeout in ms
 */
#define ETHTRCV_PHY_ACCESS_TIMEOUT_MS       (100U)

/**
 * @brief Link detection debounce count
 */
#define ETHTRCV_LINK_DEBOUNCE_COUNT         (3U)

/*==================================================================================================
 *                                TRANSCEIVER CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Transceiver 0 Configuration
 */
/* Transceiver Type */
#define ETHTRCV_TRCV0_TYPE                  (ETHTRCV_TYPE_TJA1100)
/* PHY Address (SMI address) */
#define ETHTRCV_TRCV0_PHY_ADDRESS           (0U)
/* Interface Type */
#define ETHTRCV_TRCV0_INTERFACE             (ETHTRCV_INTERFACE_RMII)
/* Default Mode after Init */
#define ETHTRCV_TRCV0_DEFAULT_MODE          (ETHTRCV_MODE_ACTIVE)
/* Auto-Negotiation Enable */
#define ETHTRCV_TRCV0_AUTO_NEG_ENABLE       (STD_ON)
/* Link Speed (when ANEG disabled) */
#define ETHTRCV_TRCV0_FIXED_SPEED           (ETHTRCV_BAUD_RATE_100MBIT)
/* Duplex Mode (when ANEG disabled) */
#define ETHTRCV_TRCV0_FIXED_DUPLEX          (ETHTRCV_DUPLEX_MODE_FULL)
/* Wake-up support */
#define ETHTRCV_TRCV0_WAKEUP_SUPPORT        (STD_ON)
/* Wake-up Mode (0=Line, 1=Frame, 2=Both) */
#define ETHTRCV_TRCV0_WAKEUP_MODE           (2U)
/* Cable Diagnostics support */
#define ETHTRCV_TRCV0_CABLE_DIAG_ENABLE     (STD_ON)
/* Signal Quality support */
#define ETHTRCV_TRCV0_SIGNAL_QUALITY_ENABLE (STD_ON)

/**
 * @brief Transceiver 1 Configuration
 */
#define ETHTRCV_TRCV1_TYPE                  (ETHTRCV_TYPE_RTL8211)
#define ETHTRCV_TRCV1_PHY_ADDRESS           (1U)
#define ETHTRCV_TRCV1_INTERFACE             (ETHTRCV_INTERFACE_RGMII)
#define ETHTRCV_TRCV1_DEFAULT_MODE          (ETHTRCV_MODE_ACTIVE)
#define ETHTRCV_TRCV1_AUTO_NEG_ENABLE       (STD_ON)
#define ETHTRCV_TRCV1_FIXED_SPEED           (ETHTRCV_BAUD_RATE_1000MBIT)
#define ETHTRCV_TRCV1_FIXED_DUPLEX          (ETHTRCV_DUPLEX_MODE_FULL)
#define ETHTRCV_TRCV1_WAKEUP_SUPPORT        (STD_OFF)
#define ETHTRCV_TRCV1_WAKEUP_MODE           (0U)
#define ETHTRCV_TRCV1_CABLE_DIAG_ENABLE     (STD_OFF)
#define ETHTRCV_TRCV1_SIGNAL_QUALITY_ENABLE (STD_OFF)

/*==================================================================================================
 *                                PHY-SPECIFIC DEFINES
 *==================================================================================================*/

/* TJA1100 Specific Registers */
#define ETHTRCV_TJA1100_REG_BASIC_CTRL      (0x00U)
#define ETHTRCV_TJA1100_REG_BASIC_STATUS    (0x01U)
#define ETHTRCV_TJA1100_REG_PHY_ID1         (0x02U)
#define ETHTRCV_TJA1100_REG_PHY_ID2         (0x03U)
#define ETHTRCV_TJA1100_REG_EXTENDED_CTRL   (0x11U)
#define ETHTRCV_TJA1100_REG_CONFIG1         (0x12U)
#define ETHTRCV_TJA1100_REG_CONFIG2         (0x13U)
#define ETHTRCV_TJA1100_REG_SYM_ERR_COUNTER (0x14U)
#define ETHTRCV_TJA1100_REG_INT_SOURCE      (0x15U)
#define ETHTRCV_TJA1100_REG_INT_ENABLE      (0x16U)
#define ETHTRCV_TJA1100_REG_COMM_STATUS     (0x17U)
#define ETHTRCV_TJA1100_REG_GENERAL_STATUS  (0x18U)
#define ETHTRCV_TJA1100_REG_EXTERNAL_STATUS (0x19U)
#define ETHTRCV_TJA1100_REG_LINK_FAIL_COUNTER (0x1AU)

/* TJA1100 Extended Control Bits */
#define ETHTRCV_TJA1100_EXT_CTRL_RESET      (0x8000U)
#define ETHTRCV_TJA1100_EXT_CTRL_PWR_MODE_MASK (0x7000U)
#define ETHTRCV_TJA1100_EXT_CTRL_PWR_NORMAL (0x0000U)
#define ETHTRCV_TJA1100_EXT_CTRL_PWR_SLEEP  (0x1000U)
#define ETHTRCV_TJA1100_EXT_CTRL_PWR_STANDBY (0x2000U)
#define ETHTRCV_TJA1100_EXT_CTRL_PWR_DISABLE (0x4000U)
#define ETHTRCV_TJA1100_EXT_CTRL_SLAVE_JITTER_TEST (0x0800U)
#define ETHTRCV_TJA1100_EXT_CTRL_TRAINING_RESTART (0x0400U)
#define ETHTRCV_TJA1100_EXT_CTRL_CABLE_TEST (0x0200U)
#define ETHTRCV_TJA1100_EXT_CTRL_LOOPBACK_MODE (0x0100U)
#define ETHTRCV_TJA1100_EXT_CTRL_CONFIG_INH (0x0080U)
#define ETHTRCV_TJA1100_EXT_CTRL_AUTO_PWD   (0x0040U)

/* TJA1100 Communication Status */
#define ETHTRCV_TJA1100_COMM_LOC_REMFAULT   (0x1000U)
#define ETHTRCV_TJA1100_COMM_REM_WUR        (0x0800U)
#define ETHTRCV_TJA1100_COMM_REM_LWU        (0x0400U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_MASK (0x0380U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_IDLE_ERR (0x0080U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_TRAINING (0x0100U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_SLEEP_FAIL (0x0180U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_SLEEP (0x0200U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_ACTIVE (0x0280U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_SLEEP_ACK (0x0300U)
#define ETHTRCV_TJA1100_COMM_PHY_STATE_LWU  (0x0380U)
#define ETHTRCV_TJA1100_COMM_LINK_UP        (0x0040U)
#define ETHTRCV_TJA1100_COMM_TX_MODE        (0x0020U)
#define ETHTRCV_TJA1100_COMM_RX_MODE        (0x0010U)
#define ETHTRCV_TJA1100_COMM_POLARITY_DETECT (0x0008U)
#define ETHTRCV_TJA1100_COMM_PHY_CONFIG     (0x0004U)
#define ETHTRCV_TJA1100_COMM_TWEVENT        (0x0001U)

/* RTL8211 Specific Registers */
#define ETHTRCV_RTL8211_REG_PHYIDR1         (0x02U)
#define ETHTRCV_RTL8211_REG_PHYIDR2         (0x03U)
#define ETHTRCV_RTL8211_REG_ANAEG           (0x04U)
#define ETHTRCV_RTL8211_REG_ANLPA           (0x05U)
#define ETHTRCV_RTL8211_REG_ANE             (0x06U)
#define ETHTRCV_RTL8211_REG_GBECSR          (0x09U)
#define ETHTRCV_RTL8211_REG_GBESR           (0x0AU)
#define ETHTRCV_RTL8211_REG_PHYCR1          (0x10U)
#define ETHTRCV_RTL8211_REG_PHYCR2          (0x11U)
#define ETHTRCV_RTL8211_REG_PHYSR           (0x11U)
#define ETHTRCV_RTL8211_REG_PHYSCR          (0x12U)
#define ETHTRCV_RTL8211_REG_INSR            (0x13U)
#define ETHTRCV_RTL8211_REG_PAGSR           (0x1FU)

/* RTL8211 Page Select */
#define ETHTRCV_RTL8211_PAGE_PCC            (0xA43U)  /* Page Control Common */
#define ETHTRCV_RTL8211_PAGE_PCG            (0xD08U)  /* Page Control Green */

/* RTL8211 PHYSR Register Bits */
#define ETHTRCV_RTL8211_PHYSR_SPEED_MASK    (0xC000U)
#define ETHTRCV_RTL8211_PHYSR_SPEED_10      (0x0000U)
#define ETHTRCV_RTL8211_PHYSR_SPEED_100     (0x8000U)
#define ETHTRCV_RTL8211_PHYSR_SPEED_1000    (0xC000U)
#define ETHTRCV_RTL8211_PHYSR_DUPLEX        (0x2000U)
#define ETHTRCV_RTL8211_PHYSR_LINK          (0x0400U)
#define ETHTRCV_RTL8211_PHYSR_MDI_PLUG      (0x0020U)
#define ETHTRCV_RTL8211_PHYSR_ALDPS         (0x0010U)

/* KSZ8081 Specific Registers */
#define ETHTRCV_KSZ8081_REG_PC1R            (0x1EU)
#define ETHTRCV_KSZ8081_REG_PC2R            (0x1FU)

/* LAN8720 Specific Registers */
#define ETHTRCV_LAN8720_REG_SMR             (0x12U)
#define ETHTRCV_LAN8720_REG_SCSR            (0x1FU)

/* LAN8720 SCSR Bits */
#define ETHTRCV_LAN8720_SCSR_SPEED          (0x0004U)
#define ETHTRCV_LAN8720_SCSR_DUPLEX         (0x0010U)
#define ETHTRCV_LAN8720_SCSR_LINK           (0x0040U)

/*==================================================================================================
 *                               INTERFACE CONFIGURATION
 *==================================================================================================*/

/**
 * @brief MII Interface Settings
 */
#define ETHTRCV_MII_TX_CLK_DIV              (2U)
#define ETHTRCV_MII_RX_CLK_DIV              (2U)

/**
 * @brief RMII Interface Settings
 */
#define ETHTRCV_RMII_REF_CLK_FREQ_MHZ       (50U)
#define ETHTRCV_RMII_CRS_DV_ENABLE          (STD_ON)

/**
 * @brief RGMII Interface Settings
 */
#define ETHTRCV_RGMII_TX_CLK_DELAY_NS       (2U)
#define ETHTRCV_RGMII_RX_CLK_DELAY_NS       (2U)
#define ETHTRCV_RGMII_ID_ENABLE             (STD_ON)

/*==================================================================================================
 *                                  CALLBACK CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Link state change callback enable
 */
#define ETHTRCV_LINK_STATE_CHG_CALLBACK     STD_ON

/**
 * @brief Wake-up indication callback enable
 */
#define ETHTRCV_WAKEUP_IND_CALLBACK         STD_ON

/*==================================================================================================
 *                                   DEBUG CONFIGURATION
 *==================================================================================================*/

/**
 * @brief Enable internal debug traces
 */
#define ETHTRCV_DEBUG_ENABLE                STD_ON

/**
 * @brief Enable statistics counters
 */
#define ETHTRCV_STATS_ENABLE                STD_ON

#ifdef __cplusplus
}
#endif

#endif /* ETHTRCV_CFG_H */
