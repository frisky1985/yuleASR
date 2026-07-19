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
 * @file Com_Lcfg.c
 * @brief COM Configuration Tables
 */

#include "Com.h"
#include "Com_Cfg.h"

/* Signal Configurations */
static const Com_SignalConfigType Com_Signals[COM_MAX_SIGNALS] = {
    {
        .SignalId = 0U,
        .BitPosition = 0U,
        .BitSize = 8U,
        .Endianness = COM_LITTLE_ENDIAN,
        .TransferProperty = COM_TX_MODE_DIRECT,
        .FilterAlgorithm = 0U,
        .FilterMask = 0U,
        .FilterX = 0U,
        .SignalGroupRef = 0U
    },
    {
        .SignalId = 1U,
        .BitPosition = 9U,
        .BitSize = 16U,
        .Endianness = COM_LITTLE_ENDIAN,
        .TransferProperty = COM_TX_MODE_PERIODIC,
        .FilterAlgorithm = 0U,
        .FilterMask = 0U,
        .FilterX = 0U,
        .SignalGroupRef = 0U
    }
};

/* I-PDU Configurations */
static const Com_IPduConfigType Com_IPdus[COM_MAX_IPDUS] = {
    {
        .PduId = 0U,
        .DataLength = 8U,
        .RepeatingEnabled = FALSE,
        .NumRepetitions = 0U,
        .TimeBetweenRepetitions = 0U,
        .TimePeriod = 100U
    }
};

/* Configuration */
const Com_ConfigType Com_Config = {
    .Signals = Com_Signals,
    .NumSignals = 2U,
    .IPdus = Com_IPdus,
    .NumIPdus = 1U
};
