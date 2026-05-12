/**
 * @file EthIf_Cfg.h
 * @brief Ethernet Interface configuration header
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef ETHIF_CFG_H
#define ETHIF_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define ETHIF_DEV_ERROR_DETECT          (STD_ON)
#define ETHIF_VERSION_INFO_API          (STD_ON)
#define ETHIF_ENABLE_WAKEUP_MODE_API    (STD_ON)
#define ETHIF_GET_WAKEUP_MODE_API       (STD_ON)
#define ETHIF_GET_CTRL_IDX_API          (STD_ON)
#define ETHIF_GET_VLAN_IDX_API          (STD_ON)
#define ETHIF_GET_AND_RESET_MEASUREMENT_DATA_API (STD_ON)
#define ETHIF_GET_CURRENT_TIME_API      (STD_ON)
#define ETHIF_ENABLE_EGRESS_TIMESTAMP_API (STD_ON)
#define ETHIF_GET_EGRESS_TIMESTAMP_API  (STD_ON)
#define ETHIF_GET_INGRESS_TIMESTAMP_API (STD_ON)

/*==================================================================================================
*                                    NUMBER OF CONTROLLERS
==================================================================================================*/
#define ETHIF_NUM_CONTROLLERS           (2U)
#define ETHIF_NUM_FRAME_OWNERS          (8U)
#define ETHIF_NUM_VLANS                 (4U)

/*==================================================================================================
*                                    CONTROLLER DEFINITIONS
==================================================================================================*/
#define ETHIF_CONTROLLER_0              (0U)
#define ETHIF_CONTROLLER_1              (1U)

/*==================================================================================================
*                                    MAC ADDRESS
==================================================================================================*/
#define ETHIF_DEFAULT_MAC_ADDR          {0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U}

/*==================================================================================================
*                                    MTU CONFIGURATION
==================================================================================================*/
#define ETHIF_MTU_DEFAULT               (1500U)
#define ETHIF_MTU_JUMBO                 (9000U)

/*==================================================================================================
*                                    FRAME TYPES
==================================================================================================*/
#define ETHIF_FRAMETYPE_IPV4            ((EthIf_FrameType)0x0800U)
#define ETHIF_FRAMETYPE_IPV6            ((EthIf_FrameType)0x86DDU)
#define ETHIF_FRAMETYPE_ARP             ((EthIf_FrameType)0x0806U)
#define ETHIF_FRAMETYPE_VLAN            ((EthIf_FrameType)0x8100U)
#define ETHIF_FRAMETYPE_SOMEIP          ((EthIf_FrameType)0x88E0U)
#define ETHIF_FRAMETYPE_TSN             ((EthIf_FrameType)0x88F7U)

/*==================================================================================================
*                                    VLAN CONFIGURATION
==================================================================================================*/
#define ETHIF_VLAN_ID_DEFAULT           (1U)
#define ETHIF_VLAN_ID_DIAG              (100U)
#define ETHIF_VLAN_ID_ADAS              (200U)
#define ETHIF_VLAN_ID_INFOTAINMENT      (300U)

/*==================================================================================================
*                                    TIME SYNC
==================================================================================================*/
#define ETHIF_TIME_SYNC_ENABLED         (STD_ON)
#define ETHIF_GPTP_SUPPORT              (STD_ON)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define ETHIF_MAIN_FUNCTION_PERIOD_MS   (5U)

/*==================================================================================================
*                                    WAKEUP SUPPORT
==================================================================================================*/
#define ETHIF_WAKEUP_SUPPORT            (STD_ON)

/*==================================================================================================
*                                    SWITCH SUPPORT
==================================================================================================*/
#define ETHIF_SWITCH_SUPPORT            (STD_ON)
#define ETHIF_NUM_SWITCH_PORTS          (4U)

#endif /* ETHIF_CFG_H */
