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
 * @file SomeIpIf_Cfg.h
 * @brief SOME/IP Interface Configuration
 */

#ifndef SOMEIPIF_CFG_H
#define SOMEIPIF_CFG_H

#define SOMEIPIF_DEV_ERROR_DETECT       STD_ON
#define SOMEIPIF_VERSION_INFO_API       STD_ON

/* Protocol Constants */
#define SOMEIP_PROTOCOL_VERSION         0x01U
#define SOMEIP_INTERFACE_VERSION        0x01U

/* SOME/IP Header Size (16 bytes) */
#define SOMEIP_HEADER_SIZE              16U

/* Maximum Message Length */
#define SOMEIP_MAX_MESSAGE_LENGTH       4095U

/* Maximum Number of Services */
#define SOMEIPIF_MAX_SERVICES           32U

/* Maximum Number of Endpoints */
#define SOMEIPIF_MAX_ENDPOINTS          16U

/* Connection Types */
#define SOMEIP_CONNECTION_TCP           0x00U
#define SOMEIP_CONNECTION_UDP           0x01U

#endif
