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
 * @file Lin_Cfg.h
 * @brief LIN Driver Configuration
 */

#ifndef LIN_CFG_H
#define LIN_CFG_H

#include "Lin.h"

/* Development Error Detection */
#define LIN_DEV_ERROR_DETECT               STD_ON
#define LIN_VERSION_INFO_API               STD_ON
#define LIN_WAKEUP_SUPPORT                 STD_ON

/* Number of Channels */
#define LIN_MAX_CHANNELS                   2

/* Channel IDs */
#define LIN_CHANNEL_0                      0x00
#define LIN_CHANNEL_1                      0x01

/* Baud Rates */
#define LIN_BAUDRATE_9600                  9600
#define LIN_BAUDRATE_19200                 19200

/* Timeouts (in ms) */
#define LIN_TIMEOUT                        100
#define LIN_WAKEUP_TIMEOUT                 50

/* Frame Configuration */
#define LIN_MAX_FRAME_LENGTH               8
#define LIN_MAX_PID                        0x3F

/* Channel 0 Configuration */
#define LIN_CH0_BAUDRATE                   LIN_BAUDRATE_19200
#define LIN_CH0_WAKEUP_SUPPORT             STD_ON

/* Channel 1 Configuration */
#define LIN_CH1_BAUDRATE                   LIN_BAUDRATE_19200
#define LIN_CH1_WAKEUP_SUPPORT             STD_ON

#endif /* LIN_CFG_H */
