/**
 * @file EthSM_Cfg.h
 * @brief Ethernet State Manager (EthSM) Configuration Header
 * @version 1.0.0
 * @date 2026-05-05
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Ethernet State Manager (ETHSM)
 * Layer: ECU Abstraction Layer (ECUAL)
 * AUTOSAR Version: 4.4.0
 */

#ifndef ETHSM_CFG_H
#define ETHSM_CFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define ETHSM_CFG_VENDOR_ID                     (0x01U)
#define ETHSM_CFG_MODULE_ID                     (0x43U)
#define ETHSM_CFG_AR_RELEASE_MAJOR_VERSION      (0x04U)
#define ETHSM_CFG_AR_RELEASE_MINOR_VERSION      (0x04U)
#define ETHSM_CFG_AR_RELEASE_REVISION_VERSION   (0x00U)
#define ETHSM_CFG_SW_MAJOR_VERSION              (0x01U)
#define ETHSM_CFG_SW_MINOR_VERSION              (0x00U)
#define ETHSM_CFG_SW_PATCH_VERSION              (0x00U)

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION OPTIONS
==================================================================================================*/

/**
 * @brief Enable/Disable Version Info API
 * @details STD_ON: EthSM_GetVersionInfo is available
 *          STD_OFF: EthSM_GetVersionInfo is not available
 */
#define ETHSM_VERSION_INFO_API                  STD_ON

/**
 * @brief Enable/Disable Development Error Detection
 * @details STD_ON: DET error reporting is enabled
 *          STD_OFF: DET error reporting is disabled
 */
#define ETHSM_DEV_ERROR_DETECT                  STD_ON

/**
 * @brief Enable/Disable Wake-up Support
 * @details STD_ON: Wake-up functionality is enabled
 *          STD_OFF: Wake-up functionality is disabled
 * @note If enabled, EthSM handles wake-up from bus sleep
 */
#define ETHSM_WAKEUP_SUPPORT                    STD_ON

/**
 * @brief Enable/Disable Transceiver Link Change Notification
 * @details STD_ON: EthSM is notified on link state changes
 *          STD_OFF: Link state is polled only
 */
#define ETHSM_TRCVLINK_CHANGE_NOTIFICATION      STD_ON

/**
 * @brief Enable/Disable State Change Callback
 * @details STD_ON: ComM_BusSM_ModeIndication is called on state changes
 *          STD_OFF: No callback on state changes
 */
#define ETHSM_STATE_CHANGE_CALLBACK             STD_ON

/*==================================================================================================
*                                    NETWORK CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of Ethernet networks managed by EthSM
 * @details This value defines the size of internal state arrays
 */
#define ETHSM_MAX_NETWORKS                      (2U)

/**
 * @brief Network Handle 0 - Primary Ethernet Network
 */
#define ETHSM_NETWORK_0                         (0U)

/**
 * @brief Network Handle 1 - Secondary Ethernet Network (optional)
 */
#define ETHSM_NETWORK_1                         (1U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION (in milliseconds)
==================================================================================================*/

/**
 * @brief Timeout for WAIT_TRCVLINK state
 * @details Maximum time to wait for transceiver link to become active
 *          If exceeded, state machine transitions to NO_COM
 */
#define ETHSM_TIMEOUT_WAIT_TRCVLINK             (100U)  /* 100ms */

/**
 * @brief Timeout for WAIT_ONLINE state
 * @details Maximum time to wait for TcpIp to become online
 *          If exceeded, state machine transitions to NO_COM
 */
#define ETHSM_TIMEOUT_WAIT_ONLINE               (5000U) /* 5 seconds */

/**
 * @brief Timeout for transceiver wake-up sequence
 * @details Maximum time to wait for transceiver to complete wake-up
 * @note Only used if ETHSM_WAKEUP_SUPPORT is STD_ON
 */
#define ETHSM_TIMEOUT_TRCV_WAKEUP               (50U)   /* 50ms */

/**
 * @brief Debounce time for link state changes
 * @details Minimum time link must be stable before state change is accepted
 */
#define ETHSM_LINK_DEBOUNCE_TIME                (20U)   /* 20ms */

/*==================================================================================================
*                                    MAIN FUNCTION CYCLE TIME
==================================================================================================*/

/**
 * @brief Cycle time of EthSM_MainFunction in milliseconds
 * @details This value is used to calculate timeout counters
 */
#define ETHSM_MAIN_FUNCTION_CYCLE_MS            (10U)   /* 10ms */

/*==================================================================================================
*                                    CONTROLLER CONFIGURATION
==================================================================================================*/

/**
 * @brief Ethernet Controller Index for Network 0
 * @details Maps EthSM network to EthIf controller index
 */
#define ETHSM_CTRL_IDX_NETWORK_0                (0U)

/**
 * @brief Ethernet Controller Index for Network 1
 * @details Maps EthSM network to EthIf controller index
 */
#define ETHSM_CTRL_IDX_NETWORK_1                (1U)

/*==================================================================================================
*                                    TRANSCEIVER CONFIGURATION
==================================================================================================*/

/**
 * @brief Ethernet Transceiver Index for Network 0
 * @details Maps EthSM network to EthIf transceiver index
 */
#define ETHSM_TRCV_IDX_NETWORK_0                (0U)

/**
 * @brief Ethernet Transceiver Index for Network 1
 * @details Maps EthSM network to EthIf transceiver index
 */
#define ETHSM_TRCV_IDX_NETWORK_1                (1U)

/*==================================================================================================
*                                    TCP/IP CONFIGURATION
==================================================================================================*/

/**
 * @brief TcpIp Controller Index for Network 0
 * @details Maps EthSM network to TcpIp controller index
 */
#define ETHSM_TCPIP_CTRL_IDX_NETWORK_0          (0U)

/**
 * @brief TcpIp Controller Index for Network 1
 * @details Maps EthSM network to TcpIp controller index
 */
#define ETHSM_TCPIP_CTRL_IDX_NETWORK_1          (1U)

/*==================================================================================================
*                                    WAKE-UP CONFIGURATION
==================================================================================================*/

#if (ETHSM_WAKEUP_SUPPORT == STD_ON)
/**
 * @brief Wake-up Source for Network 0
 * @details EcuM wake-up source identifier
 */
#define ETHSM_WAKEUP_SOURCE_NETWORK_0           (0x01U)

/**
 * @brief Wake-up Source for Network 1
 * @details EcuM wake-up source identifier
 */
#define ETHSM_WAKEUP_SOURCE_NETWORK_1           (0x02U)

/**
 * @brief Enable/Disable Wake-up by Bus for Network 0
 */
#define ETHSM_WAKEUP_BY_BUS_NETWORK_0           STD_ON

/**
 * @brief Enable/Disable Wake-up by Bus for Network 1
 */
#define ETHSM_WAKEUP_BY_BUS_NETWORK_1           STD_ON

#endif /* ETHSM_WAKEUP_SUPPORT */

/*==================================================================================================
*                                    RETRY CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of retries for mode requests
 * @details If a mode request fails, it will be retried up to this many times
 */
#define ETHSM_MAX_RETRIES                       (3U)

/**
 * @brief Delay between retries in milliseconds
 */
#define ETHSM_RETRY_DELAY_MS                    (100U)

/*==================================================================================================
*                                    POST-BUILD CONFIGURATION
==================================================================================================*/

/**
 * @brief Post-build configuration variant
 * @details SELECTABLE: Configuration can be selected at runtime
 *          VARIANT-POST-BUILD: Post-build configuration support
 */
#define ETHSM_CONFIG_VARIANT                    ETHSM_CONFIG_VARIANT_PRECOMPILE

#define ETHSM_CONFIG_VARIANT_PRECOMPILE         (0U)
#define ETHSM_CONFIG_VARIANT_SELECTABLE         (1U)
#define ETHSM_CONFIG_VARIANT_POSTBUILD          (2U)

#endif /* ETHSM_CFG_H */
