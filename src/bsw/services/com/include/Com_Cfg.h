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
 * @file Com_Cfg.h
 * @brief COM Configuration
 */

#ifndef COM_CFG_H
#define COM_CFG_H

#define COM_DEV_ERROR_DETECT            STD_ON
#define COM_VERSION_INFO_API            STD_ON

/* Maximum Counts */
#define COM_MAX_SIGNALS                 256U
#define COM_MAX_IPDUS                   64U
#define COM_MAX_GROUPS                  16U

/* Signal Limits */
#define COM_MAX_SIGNAL_LENGTH           64U

/* Transmission Modes */
#define COM_TX_MODE_DIRECT              0x00U
#define COM_TX_MODE_PERIODIC            0x01U
#define COM_TX_MODE_MIXED               0x02U

/* Byte Order */
#define COM_LITTLE_ENDIAN               0x00U
#define COM_BIG_ENDIAN                  0x01U

/* Runtime Count Aliases (matching actual usage) */
#define COM_NUM_OF_IPDUS                COM_MAX_IPDUS
#define COM_NUM_OF_SIGNALS              COM_MAX_SIGNALS
#define COM_NUM_OF_IPDU_GROUPS          COM_MAX_GROUPS
#define COM_NUM_IPDU_GROUPS             COM_NUM_OF_IPDU_GROUPS
#define COM_NUM_OF_SIGNAL_GROUPS        COM_MAX_GROUPS
#define COM_MAX_IPDU_BUFFER_SIZE        128U
#define COM_MAX_IPDU_LENGTH             64U

#endif
