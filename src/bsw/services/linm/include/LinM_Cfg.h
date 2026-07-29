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
 * @file LinM_Cfg.h
 * @brief LIN Master Management configuration header
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef LINM_CFG_H
#define LINM_CFG_H

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/
#define LINM_DEV_ERROR_DETECT               (STD_ON)
#define LINM_VERSION_INFO_API               (STD_ON)
#define LINM_WAKEUP_SUPPORT                 (STD_ON)
#define LINM_SLEEP_SUPPORT                  (STD_ON)
#define LINM_MASTER_NODE_SUPPORT            (STD_ON)

/*==================================================================================================
*                                    CHANNEL CONFIGURATION
==================================================================================================*/
#define LINM_NUMBER_OF_CHANNELS             (1U)
#define LINM_NUMBER_OF_SCHEDULES            (4U)
#define LINM_NUMBER_OF_ENTRIES              (16U)

/*==================================================================================================
*                                    CHANNEL IDs
==================================================================================================*/
#define LINM_CHANNEL_0                      (0U)

/*==================================================================================================
*                                    SCHEDULE IDs
==================================================================================================*/
#define LINM_SCHEDULE_NULL                  (0U)
#define LINM_SCHEDULE_MASTER                (1U)
#define LINM_SCHEDULE_DIAGNOSTIC            (2U)
#define LINM_SCHEDULE_TABLE_0               (3U)

/*==================================================================================================
*                                    SCHEDULE ENTRY TYPES
==================================================================================================*/
#define LINM_ENTRY_TYPE_UNCONDITIONAL       (0U)
#define LINM_ENTRY_TYPE_EVENT_TRIGGERED     (1U)
#define LINM_ENTRY_TYPE_SPORADIC            (2U)
#define LINM_ENTRY_TYPE_DIAGNOSTIC          (3U)
#define LINM_ENTRY_TYPE_SLAVE_TO_SLAVE      (4U)
#define LINM_ENTRY_TYPE_EMPTY               (0xFFU)

/*==================================================================================================
*                                    FRAME TYPES
==================================================================================================*/
#define LINM_FRAME_TYPE_PUBLISHER           (0U)
#define LINM_FRAME_TYPE_SUBSCRIBER          (1U)
#define LINM_FRAME_TYPE_SUBSCRIBER_AUTO     (2U)

/*==================================================================================================
*                                    TIMING CONFIGURATION
==================================================================================================*/
#define LINM_SCHEDULE_BASE_TIME_MS          (10U)
#define LINM_WAKEUP_TIMEOUT_MS              (50U)
#define LINM_SLEEP_TIMEOUT_MS               (100U)
#define LINM_MASTER_REQUEST_DELAY_MS        (5U)
#define LINM_SLAVE_RESPONSE_TIMEOUT_MS      (20U)

/*==================================================================================================
*                                    SCHEDULE ENTRY MASKS
==================================================================================================*/
#define LINM_ENTRY_DELAY_MASK               (0x00FFU)
#define LINM_ENTRY_FRAME_MASK               (0xFF00U)
#define LINM_ENTRY_TYPE_MASK                (0xFF0000U)

#define LINM_ENTRY_DELAY_SHIFT              (0U)
#define LINM_ENTRY_FRAME_SHIFT              (8U)
#define LINM_ENTRY_TYPE_SHIFT               (16U)

/*==================================================================================================
*                                    SCHEDULE PRIORITIES
==================================================================================================*/
#define LINM_SCHEDULE_PRIORITY_HIGH         (0U)
#define LINM_SCHEDULE_PRIORITY_NORMAL       (1U)
#define LINM_SCHEDULE_PRIORITY_LOW          (2U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIOD
==================================================================================================*/
#define LINM_MAIN_FUNCTION_PERIOD_MS        (5U)

/*==================================================================================================
*                                    NULL SCHEDULE ENTRY
==================================================================================================*/
#define LINM_NULL_ENTRY                     (0xFFFFFFFFU)

#endif /* LINM_CFG_H */
