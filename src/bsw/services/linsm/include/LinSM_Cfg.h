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
 * @file LinSM_Cfg.h
 * @brief LIN State Manager configuration header
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef LINSM_CFG_H
#define LINSM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define LINSM_DEV_ERROR_DETECT              (STD_ON)
#define LINSM_VERSION_INFO_API              (STD_ON)
#define LINSM_WAKEUP_SUPPORT                (STD_ON)
#define LINSM_COMMUNICATION_CONTROL_SUPPORT (STD_ON)
#define LINSM_SLAVE_SUPPORT                 (STD_ON)
#define LINSM_MASTER_NODE_SUPPORT           (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define LINSM_NUMBER_OF_CHANNELS            (1U)
#define LINSM_NUMBER_OF_SCHEDULES           (4U)

/*==================================================================================================
*                                    CHANNEL IDs
==================================================================================================*/
#define LINSM_CHANNEL_0                     (0U)

/*==================================================================================================
*                                    SCHEDULE IDs
==================================================================================================*/
#define LINSM_SCHEDULE_NULL                 (0U)
#define LINSM_SCHEDULE_MASTER               (1U)
#define LINSM_SCHEDULE_DIAGNOSTIC           (2U)
#define LINSM_SCHEDULE_TABLE_0              (3U)

/*==================================================================================================
*                                    INITIAL SCHEDULE
==================================================================================================*/
#define LINSM_INITIAL_SCHEDULE              (LINSM_SCHEDULE_MASTER)

/*==================================================================================================
*                                    TIMING CONFIGURATION
==================================================================================================*/
#define LINSM_REQUEST_TIMEOUT_MS            (100U)
#define LINSM_CONFIRMATION_TIMEOUT_MS       (50U)
#define LINSM_MAIN_FUNCTION_PERIOD_MS       (5U)
#define LINSM_WAKEUP_TIMEOUT_MS             (50U)
#define LINSM_GOTOSLEEP_TIMEOUT_MS          (100U)
#define LINSM_MAX_SCHEDULE_SWITCHES         (10U)

/*==================================================================================================
*                                    MODE DEFINITIONS
==================================================================================================*/
/* Mode aliases — values must match LinSM_ModeType enum */
#define LINSM_MODE_FULL_COM                 (1U)
#define LINSM_MODE_NO_COM                   (0U)
#define LINSM_MODE_SILENT_COM               (2U)

/*==================================================================================================
*                                    TIMEOUT COUNTERS
==================================================================================================*/
#define LINSM_REQUEST_TIMEOUT_COUNT         (LINSM_REQUEST_TIMEOUT_MS / LINSM_MAIN_FUNCTION_PERIOD_MS)
#define LINSM_CONFIRMATION_TIMEOUT_COUNT    (LINSM_CONFIRMATION_TIMEOUT_MS / LINSM_MAIN_FUNCTION_PERIOD_MS)

#endif /* LINSM_CFG_H */
