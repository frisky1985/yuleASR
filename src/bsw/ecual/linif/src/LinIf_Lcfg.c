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
/* @req SWS_LinIf_00001 @req SWS_LinIf_00002 @req SWS_LinIf_00003 */


/**
 * @file LinIf_Lcfg.c
 * @brief LIN Interface Configuration Tables
 */

#include "LinIf.h"
#include "LinIf_Cfg.h"

/* Frame Configurations */
static const LinIf_FrameConfigType LinIf_Frames[LINIF_MAX_FRAMES] = {
    {
        .FrameIdx = 0U,
        .Pid = 0x3CU,
        .Dlc = 8U,
        .FrameType = LINIF_UNCONDITIONAL_FRAME,
        .IsPublish = TRUE
    },
    {
        .FrameIdx = 1U,
        .Pid = 0x3DU,
        .Dlc = 8U,
        .FrameType = LINIF_UNCONDITIONAL_FRAME,
        .IsPublish = FALSE
    }
};

/* Schedule Entries */
static const LinIf_ScheduleEntryType LinIf_NormalScheduleEntries[] = {
    { 5U, 0U },
    { 10U, 1U }
};

/* Schedule Tables */
static const LinIf_ScheduleTableConfigType LinIf_Schedules[LINIF_MAX_SCHEDULES] = {
    {
        .Schedule = LINIF_NULL_SCHEDULE,
        .EntryCount = 0U,
        .Entries = NULL_PTR
    },
    {
        .Schedule = LINIF_Normal,
        .EntryCount = 2U,
        .Entries = LinIf_NormalScheduleEntries
    }
};

/* Channel Configurations */
static const LinIf_ChannelConfigType LinIf_Channels[LINIF_MAX_CHANNELS] = {
    {
        .ChannelId = 0U,
        .NumFrames = 2U,
        .NumSchedules = 2U,
        .Frames = LinIf_Frames,
        .Schedules = LinIf_Schedules
    }
};

/* Configuration */
static const LinIf_ConfigType LinIf_Config = {
    .NumChannels = 1U,
    .Channels = LinIf_Channels
};
