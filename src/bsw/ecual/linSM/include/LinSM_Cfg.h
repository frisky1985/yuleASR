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

/************************************************************************************
 * File: LinSM_Cfg.h
 * Description: LIN State Manager - Configuration Header File
 * AUTOSAR Version: 4.4.0
 *
 * Module: LinSM (LIN State Manager)
 * Purpose: Compile-time configuration parameters
 ************************************************************************************/

#ifndef LINSM_CFG_H
#define LINSM_CFG_H

/*================================================================================
 * General Configuration Parameters
 *===============================================================================*/

/**
 * @brief Enable/Disable version information API
 */
#define LINSM_VERSION_INFO_API              STD_ON

/**
 * @brief Enable/Disable development error detection
 */
#define LINSM_DEV_ERROR_DETECT              STD_ON

/**
 * @brief Enable/Disable wake-up support
 */
#define LINSM_WAKEUP_SUPPORT                STD_ON

/**
 * @brief Enable/Disable sleep mode support
 */
#define LINSM_SLEEP_SUPPORT                 STD_ON

/**
 * @brief Enable/Disable schedule table switching
 */
#define LINSM_SCHEDULE_TABLE_SWITCHING      STD_ON

/*================================================================================
 * Module Configuration Parameters
 *===============================================================================*/

/**
 * @brief Number of configured LIN channels
 */
#define LINSM_CHANNEL_COUNT                 (2U)

/**
 * @brief Number of schedule tables per channel
 */
#define LINSM_SCHEDULE_COUNT_PER_CHANNEL    (4U)

/**
 * @brief Total number of schedule tables
 */
#define LINSM_TOTAL_SCHEDULE_COUNT          (LINSM_CHANNEL_COUNT * LINSM_SCHEDULE_COUNT_PER_CHANNEL)

/**
 * @brief Number of wake-up sources
 */
#define LINSM_WAKEUP_SOURCE_COUNT           (2U)

/*================================================================================
 * Timeout Configuration (in milliseconds)
 *===============================================================================*/

/**
 * @brief Schedule table confirmation timeout
 * Default: 1000ms
 */
#define LINSM_SCHEDULE_CONFIRMATION_TIMEOUT (1000U)

/**
 * @brief Wake-up confirmation timeout
 * Default: 5000ms
 */
#define LINSM_WAKEUP_CONFIRMATION_TIMEOUT   (5000U)

/**
 * @brief Go-to-sleep confirmation timeout
 * Default: 1000ms
 */
#define LINSM_SLEEP_CONFIRMATION_TIMEOUT    (1000U)

/**
 * @brief Mode request repetition time
 * Used for retry in case of busy state
 */
#define LINSM_MODE_REQUEST_REPETITION_TIME  (50U)

/**
 * @brief Main function period in milliseconds
 */
#define LINSM_MAIN_FUNCTION_PERIOD_MS       (10U)

/*================================================================================
 * Channel Identifiers
 *===============================================================================*/

/**
 * @brief LIN Channel 0 - Internal Identifier
 */
#define LINSM_CHANNEL_0                     (0U)

/**
 * @brief LIN Channel 1 - Internal Identifier
 */
#define LINSM_CHANNEL_1                     (1U)

/**
 * @brief Invalid channel identifier
 */
#define LINSM_INVALID_CHANNEL               (0xFFU)

/*================================================================================
 * Schedule Table Identifiers
 *===============================================================================*/

/**
 * @brief Null schedule (no schedule)
 */
#define LINSM_SCHEDULE_NULL                 (0U)

/**
 * @brief Diagnostic request schedule
 */
#define LINSM_SCHEDULE_DIAG_REQUEST         (1U)

/**
 * @brief Diagnostic response schedule
 */
#define LINSM_SCHEDULE_DIAG_RESPONSE        (2U)

/**
 * @brief Normal communication schedule
 */
#define LINSM_SCHEDULE_NORMAL               (3U)

/**
 * @brief Master command schedule
 */
#define LINSM_SCHEDULE_MASTER               (4U)

/**
 * @brief Invalid schedule identifier
 */
#define LINSM_SCHEDULE_INVALID              (0xFFU)

/*================================================================================
 * Wake-up Source Mapping
 *===============================================================================*/

#if (LINSM_WAKEUP_SUPPORT == STD_ON)
/**
 * @brief Wake-up source for LIN Channel 0
 */
#define LINSM_WAKEUP_SOURCE_CH0             (ECUM_WKSOURCE_LIN_CH0)

/**
 * @brief Wake-up source for LIN Channel 1
 */
#define LINSM_WAKEUP_SOURCE_CH1             (ECUM_WKSOURCE_LIN_CH1)
#endif

/*================================================================================
 * ComM Channel Mapping
 *===============================================================================*/

/**
 * @brief ComM channel mapping for LIN Channel 0
 */
#define LINSM_COMM_CHANNEL_0                (COMM_CHANNEL_LIN0)

/**
 * @brief ComM channel mapping for LIN Channel 1
 */
#define LINSM_COMM_CHANNEL_1                (COMM_CHANNEL_LIN1)

/*================================================================================
 * State Machine Configuration
 *===============================================================================*/

/**
 * @brief Maximum number of state machine transitions per main function call
 */
#define LINSM_MAX_TRANSITIONS_PER_CYCLE     (3U)

/**
 * @brief Retry counter for failed transitions
 */
#define LINSM_MAX_RETRY_COUNT               (3U)

/*================================================================================
 * Post-Build Configuration Support
 *===============================================================================*/

/**
 * @brief Enable/Disable post-build configuration
 */
#define LINSM_POSTBUILD_VARIANT_SUPPORT     STD_OFF

/**
 * @brief Configuration variant (Pre-compile/Link-time/Post-build)
 */
#define LINSM_CONFIG_VARIANT                LINSM_VARIANT_PRECOMPILE

/*================================================================================
 * Configuration Constants
 *===============================================================================*/
#define LINSM_VARIANT_PRECOMPILE            (0x01U)
#define LINSM_VARIANT_LINKTIME              (0x02U)
#define LINSM_VARIANT_POSTBUILD             (0x03U)

#endif /* LINSM_CFG_H */
