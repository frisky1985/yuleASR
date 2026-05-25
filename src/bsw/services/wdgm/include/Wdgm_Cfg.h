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

/** @file Wdgm_Cfg.h
 * @brief Watchdog Manager configuration header
 */

#ifndef WDGM_CFG_H
#define WDGM_CFG_H

/*============================================================================
 *  GENERAL CONFIGURATION
 *===========================================================================*/

/** @brief Development error detection */
#define WDGM_DEV_ERROR_DETECT               STD_ON

/** @brief Version info API */
#define WDGM_VERSION_INFO_API               STD_ON

/** @brief Deinitialization allowed */
#define WDGM_DEINIT_API                     STD_OFF

/** @brief Off mode allowed (for debugging) */
#define WDGM_OFF_MODE_ENABLED               STD_OFF

/** @brief Deadline monitoring support */
#define WDGM_DEADLINE_MONITORING            STD_ON

/** @brief Alive counter monitoring */
#define WDGM_ALIVE_MONITORING               STD_ON

/** @brief Logical supervision support */
#define WDGM_LOGICAL_MONITORING             STD_ON

/*============================================================================
 *  SUPERVISION CONFIGURATION
 *===========================================================================*/

/** @brief Maximum number of supervised entities */
#define WDGM_MAX_SUPERVISED_ENTITIES        4

/** @brief Maximum checkpoints per entity */
#define WDGM_MAX_CHECKPOINTS_PER_SE         16

/** @brief Maximum total checkpoints */
#define WDGM_MAX_CHECKPOINTS                32

/** @brief Expiration tolerance (in supervision cycles) */
#define WDGM_EXPIRATION_TOLERANCE           3

/** @brief Alive counter threshold */
#define WDGM_ALIVE_THRESHOLD                5

/*============================================================================
 *  SUPERVISION ENTITY IDs
 *===========================================================================*/

/** @brief ECU Manager supervision entity */
#define WDGM_SEID_ECUM                      0

/** @brief BSW Manager supervision entity */
#define WDGM_SEID_BSWM                      1

/** @brief RTE supervision entity */
#define WDGM_SEID_RTE                       2

/** @brief Application supervision entity */
#define WDGM_SEID_APP                       3

/*============================================================================
 *  TIMEOUT CONFIGURATION
 *===========================================================================*/

/** @brief Supervision cycle period (ms) */
#define WDGM_SUPERVISION_CYCLE_MS           10

/** @brief Deadline timeout (in supervision cycles) */
#define WDGM_DEADLINE_TIMEOUT_CYCLES        50

/** @brief Fast mode supervision cycle */
#define WDGM_FAST_MODE_CYCLE_MS             5

/** @brief Slow mode supervision cycle */
#define WDGM_SLOW_MODE_CYCLE_MS             100

#endif /* WDGM_CFG_H */
