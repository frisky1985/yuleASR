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
        .ByteOrder = COM_LITTLE_ENDIAN,
        .TransferProperty = COM_DIRECT,
        .UpdateBitPosition = 8U,
        .HasUpdateBit = TRUE
    },
    {
        .SignalId = 1U,
        .BitPosition = 9U,
        .BitSize = 16U,
        .ByteOrder = COM_LITTLE_ENDIAN,
        .TransferProperty = COM_PERIODIC,
        .UpdateBitPosition = 25U,
        .HasUpdateBit = TRUE
    }
};

/* I-PDU Group 0 Signals */
static const Com_SignalConfigType* Com_IPdu0_Signals[] = {
    &Com_Signals[0],
    &Com_Signals[1]
};

/* I-PDU Configurations */
static const Com_IPduConfigType Com_IPdus[COM_MAX_IPDUS] = {
    {
        .PduId = 0U,
        .Length = 8U,
        .NumSignals = 2U,
        .Signals = Com_IPdu0_Signals,
        .TransferProperty = COM_DIRECT,
        .Period = 100U
    }
};

/* I-PDU Group 0 I-PDUs */
static const PduIdType Com_Group0_IPdus[] = { 0U };

/* I-PDU Groups */
static const Com_IPduGroupType Com_Groups[COM_MAX_GROUPS] = {
    {
        .GroupId = 0U,
        .NumIPdus = 1U,
        .IPdus = Com_Group0_IPdus,
        .IsStarted = FALSE
    }
};

/* Configuration */
const Com_ConfigType Com_Config = {
    .NumSignals = 2U,
    .NumIPdus = 1U,
    .NumGroups = 1U,
    .Signals = Com_Signals,
    .IPdus = Com_IPdus,
    .Groups = Com_Groups
};
