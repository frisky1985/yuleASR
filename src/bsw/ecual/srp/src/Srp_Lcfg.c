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
/* @req SWS_Srp_00001 @req SWS_Srp_00002 @req SWS_Srp_00005 */


/**
 * @file Srp_Lcfg.c
 * @brief SRP Configuration Tables
 */

#include "Srp.h"
#include "Srp_Cfg.h"

/* Stream Configurations */
extern const uint16 Srp_StreamCount;
static const Srp_StreamConfigType Srp_Streams[] = {
    {
        .StreamId = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07},
        .StreamVlanId = 100U,
        .Priority = 3U,
        .FrameSize = 1522U,
        .IntervalFrames = 1U,
        .Role = SRP_RESERVE_TALKER
    },
    {
        .StreamId = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08},
        .StreamVlanId = 101U,
        .Priority = 2U,
        .FrameSize = 512U,
        .IntervalFrames = 1U,
        .Role = SRP_RESERVE_LISTENER
    }
};

const uint16 Srp_StreamCount = 2U;
