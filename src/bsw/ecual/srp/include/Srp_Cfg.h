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
 * @file Srp_Cfg.h
 * @brief SRP Configuration
 */

#ifndef SRP_CFG_H
#define SRP_CFG_H

#define SRP_DEV_ERROR_DETECT    STD_ON
#define SRP_VERSION_INFO_API    STD_ON

/* Maximum Streams */
#define SRP_MAX_STREAMS         32U

/* Maximum Domain */
#define SRP_MAX_DOMAINS         4U

/* Protocol Constants */
#define SRP_ETHERTYPE           0x22EAU

/* Default Priority */
#define SRP_DEFAULT_PRIORITY    3U

/* Talker Advertise */
#define SRP_ATTRIBUTE_TYPE_TALKER_ADVERTISE     1U
#define SRP_ATTRIBUTE_TYPE_TALKER_FAILED        2U
#define SRP_ATTRIBUTE_TYPE_LISTENER_READY       3U
#define SRP_ATTRIBUTE_TYPE_LISTENER_READY_FAILED 4U
#define SRP_ATTRIBUTE_TYPE_LISTENER_ASKING_FAILED 5U

#endif
