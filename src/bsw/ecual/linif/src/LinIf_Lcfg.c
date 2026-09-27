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
    },
    {
        .FrameIdx = 2U,
        .Pid = 0x3EU,
        .Dlc = 4U,
        .FrameType = LINIF_EVENT_TRIGGERED_FRAME,
        .IsPublish = TRUE
    }
};

/* Schedule Entries */
static const LinIf_ScheduleEntryType LinIf_NormalScheduleEntries[] = {
    { 5U, 0U },
    { 10U, 1U },
    { 10U, 2U }
};

static const LinIf_ScheduleEntryType LinIf_DiagRequestScheduleEntries[] = {
    { 20U, 0U }
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
        .EntryCount = 3U,
        .Entries = LinIf_NormalScheduleEntries
    },
    {
        .Schedule = LINIF_SCHEDULE_DIAG_REQUEST,
        .EntryCount = 1U,
        .Entries = LinIf_DiagRequestScheduleEntries
    }
};

/* Channel Configurations */
static const LinIf_ChannelConfigType LinIf_Channels[LINIF_MAX_CHANNELS] = {
    {
        .ChannelId = 0U,
        .NumFrames = 3U,
        .NumSchedules = 3U,
        .Frames = LinIf_Frames,
        .Schedules = LinIf_Schedules
    }
};

/* TX PDU to frame mapping (PduId used by LinIf_Transmit) */
static const LinIf_TxPduMapType LinIf_TxPduMap[LINIF_MAX_TX_PDUS] = {
    { 0U, 0U },
    { 0U, 2U }
};

/* Configuration */
const LinIf_ConfigType LinIf_Config = {
    .NumChannels = 1U,
    .Channels = LinIf_Channels,
    .NumTxPdus = 2U,
    .TxPduMap = LinIf_TxPduMap
};
