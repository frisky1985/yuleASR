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
 * @file CanSm_Cfg.h
 * @brief CAN State Management configuration header
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef CANSM_CFG_H
#define CANSM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
/**
 * @brief Development error detection
 */
#define CANSM_DEV_ERROR_DETECT                  (STD_ON)

/**
 * @brief Version info API
 */
#define CANSM_VERSION_INFO_API                  (STD_ON)

/**
 * @brief Set baudrate API
 */
#define CANSM_SET_BAUDRATE_API                  (STD_ON)

/**
 * @brief Get baudrate API
 */
#define CANSM_GET_BAUDRATE_API                  (STD_ON)

/**
 * @brief BusOff check enabled
 */
#define CANSM_BUSOFF_CHECK_ENABLED              (STD_ON)

/**
 * @brief Transceiver management enabled
 */
#define CANSM_TRANSCEIVER_SUPPORT               (STD_ON)

/**
 * @brief Wakeup validation enabled
 */
#define CANSM_WAKEUP_VALIDATION_ENABLED         (STD_ON)

/**
 * @brief Mode change request timeout in milliseconds
 */
#define CANSM_MODE_CHANGE_REQUEST_TIMEOUT_MS    (100U)

/**
 * @brief BusOff recovery level 1 time in milliseconds
 */
#define CANSM_BUSOFF_RECOVERY_L1_MS             (100U)

/**
 * @brief BusOff recovery level 2 time in milliseconds
 */
#define CANSM_BUSOFF_RECOVERY_L2_MS             (1000U)

/**
 * @brief BusOff counter threshold before recovery
 */
#define CANSM_BUSOFF_THRESHOLD                  (10U)

/**
 * @brief Maximum number of networks
 */
#define CANSM_MAX_NETWORKS                      (4U)

/**
 * @brief Number of configured networks
 */
#define CANSM_NUM_NETWORKS                      (2U)

/*==================================================================================================
*                                    NETWORK CONFIGURATION
==================================================================================================*/
/**
 * @brief Network Handle definitions
 */
#define CANSM_NETWORK_CAN0                      (0U)
#define CANSM_NETWORK_CAN1                      (1U)
#define CANSM_NETWORK_CAN2                      (2U)
#define CANSM_NETWORK_CAN3                      (3U)

/**
 * @brief CAN Controller mapping
 */
#define CANSM_CONTROLLER_CAN0                   (0U)
#define CANSM_CONTROLLER_CAN1                   (1U)

/**
 * @brief Transceiver mapping
 */
#define CANSM_TRANSCEIVER_CAN0                  (0U)
#define CANSM_TRANSCEIVER_CAN1                  (1U)

/**
 * @brief Network specific configurations
 */
#define CANSM_NETWORK0_BUSOFF_RECOVERY_TIME_MS  (1000U)
#define CANSM_NETWORK1_BUSOFF_RECOVERY_TIME_MS  (1000U)

#define CANSM_NETWORK0_MAIN_FUNCTION_PERIOD_MS  (10U)
#define CANSM_NETWORK1_MAIN_FUNCTION_PERIOD_MS  (10U)

#define CANSM_NETWORK0_WAKEUP_SUPPORT           (STD_ON)
#define CANSM_NETWORK1_WAKEUP_SUPPORT           (STD_ON)

#define CANSM_NETWORK0_BUSOFF_RECOVERY_ENABLED  (STD_ON)
#define CANSM_NETWORK1_BUSOFF_RECOVERY_ENABLED  (STD_ON)

/*==================================================================================================
*                                    BAUDRATE CONFIGURATION
==================================================================================================*/
/**
 * @brief Supported baudrates
 */
#define CANSM_BAUDRATE_125K                     (125U)
#define CANSM_BAUDRATE_250K                     (250U)
#define CANSM_BAUDRATE_500K                     (500U)
#define CANSM_BAUDRATE_1000K                    (1000U)

/**
 * @brief Default baudrate
 */
#define CANSM_DEFAULT_BAUDRATE                  (CANSM_BAUDRATE_500K)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/
/**
 * @brief BusOff notification callback
 */
#define CANSM_BUSOFF_NOTIFICATION_ENABLED       (STD_ON)

/**
 * @brief Mode change notification callback
 */
#define CANSM_MODE_CHANGE_NOTIFICATION_ENABLED  (STD_ON)

#endif /* CANSM_CFG_H */
