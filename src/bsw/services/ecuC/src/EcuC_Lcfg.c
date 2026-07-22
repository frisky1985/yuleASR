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
 * @file EcuC_Lcfg.c
 * @brief EcuC Link-Time Configuration
 * @version 1.0.0
 * @date 2024-05-05
 */

#include "EcuC.h"
#include "EcuC_Cfg.h"

/*==================[Signal Configurations]=================================*/

/* Engine Control Signals */
extern const EcuC_ConfigType EcuC_Config;
static const EcuC_SignalConfigType EcuC_Signals_Engine[] = {
    {
        .SignalId = 0U,
        .SignalSize = 16U,      /* RPM */
        .SignalStartBit = 0U,
        .SignalBitOrder = 0U,
        .TransferProperty = ECUC_SIGNAL_TRIGGERED_ON_CHANGE,
        .Direction = ECUC_SEND,
        .RelatedPduId = 0U
    },
    {
        .SignalId = 1U,
        .SignalSize = 8U,       /* Throttle Position */
        .SignalStartBit = 16U,
        .SignalBitOrder = 0U,
        .TransferProperty = ECUC_SIGNAL_TRIGGERED,
        .Direction = ECUC_SEND,
        .RelatedPduId = 0U
    },
    {
        .SignalId = 2U,
        .SignalSize = 16U,      /* Coolant Temp */
        .SignalStartBit = 24U,
        .SignalBitOrder = 0U,
        .TransferProperty = ECUC_SIGNAL_TRIGGERED_ON_CHANGE,
        .Direction = ECUC_SEND,
        .RelatedPduId = 0U
    }
};

/* Vehicle Status Signals */
static const EcuC_SignalConfigType EcuC_Signals_Vehicle[] = {
    {
        .SignalId = 10U,
        .SignalSize = 16U,      /* Vehicle Speed */
        .SignalStartBit = 0U,
        .SignalBitOrder = 0U,
        .TransferProperty = ECUC_SIGNAL_TRIGGERED_ON_CHANGE,
        .Direction = ECUC_SEND,
        .RelatedPduId = 1U
    },
    {
        .SignalId = 11U,
        .SignalSize = 8U,       /* Gear Position */
        .SignalStartBit = 16U,
        .SignalBitOrder = 0U,
        .TransferProperty = ECUC_SIGNAL_TRIGGERED_ON_CHANGE,
        .Direction = ECUC_SEND,
        .RelatedPduId = 1U
    }
};

/* Diagnostic Signals */
static const EcuC_SignalConfigType EcuC_Signals_Diag[] = {
    {
        .SignalId = 20U,
        .SignalSize = 8U,       /* DTC Status */
        .SignalStartBit = 0U,
        .SignalBitOrder = 0U,
        .TransferProperty = ECUC_SIGNAL_TRIGGERED,
        .Direction = ECUC_RECEIVE,
        .RelatedPduId = 2U
    }
};

/* PDU Configurations */
static const EcuC_SignalConfigType* const EcuC_Pdu0_Signals[] = {
    &EcuC_Signals_Engine[0],
    &EcuC_Signals_Engine[1],
    &EcuC_Signals_Engine[2]
};

static const EcuC_PduConfigType EcuC_Pdus[] = {
    {
        .PduId = 0U,
        .PduLength = 8U,
        .SignalCount = 3U,
        .Signals = EcuC_Signals_Engine
    },
    {
        .PduId = 1U,
        .PduLength = 8U,
        .SignalCount = 2U,
        .Signals = EcuC_Signals_Vehicle
    },
    {
        .PduId = 2U,
        .PduLength = 8U,
        .SignalCount = 1U,
        .Signals = EcuC_Signals_Diag
    }
};

/* All Signals Array */
static const EcuC_SignalConfigType EcuC_AllSignals[] = {
    /* Engine Signals */
    { 0U, 16U, 0U, 0U, ECUC_SIGNAL_TRIGGERED_ON_CHANGE, ECUC_SEND, 0U },
    { 1U, 8U, 16U, 0U, ECUC_SIGNAL_TRIGGERED, ECUC_SEND, 0U },
    { 2U, 16U, 24U, 0U, ECUC_SIGNAL_TRIGGERED_ON_CHANGE, ECUC_SEND, 0U },
    /* Vehicle Signals */
    { 10U, 16U, 0U, 0U, ECUC_SIGNAL_TRIGGERED_ON_CHANGE, ECUC_SEND, 1U },
    { 11U, 8U, 16U, 0U, ECUC_SIGNAL_TRIGGERED_ON_CHANGE, ECUC_SEND, 1U },
    /* Diagnostic Signals */
    { 20U, 8U, 0U, 0U, ECUC_SIGNAL_TRIGGERED, ECUC_RECEIVE, 2U }
};

/* Routing Paths for Gateway */
static const uint16 EcuC_Routing0_SignalMapping[] = { 0U, 1U, 2U };

static const EcuC_RoutingPathType EcuC_RoutingPaths[] = {
    {
        .SourcePduId = 0U,      /* CAN Engine PDU */
        .DestinationPduId = 10U, /* Ethernet Engine PDU */
        .SignalCount = 3U,
        .SignalMapping = EcuC_Routing0_SignalMapping
    },
    {
        .SourcePduId = 1U,      /* CAN Vehicle PDU */
        .DestinationPduId = 11U, /* Ethernet Vehicle PDU */
        .SignalCount = 2U,
        .SignalMapping = NULL_PTR
    }
};

/*==================[Module Configuration]==================================*/

static const EcuC_ConfigType EcuC_Config = {
    .PduCount = 3U,
    .SignalCount = 6U,
    .RoutingPathCount = 2U,
    .Pdus = EcuC_Pdus,
    .Signals = EcuC_AllSignals,
    .RoutingPaths = EcuC_RoutingPaths
};

/*==================[End of File]===========================================*/
