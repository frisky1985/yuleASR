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
 * @file Xcp_Lcfg.c
 * @brief XCP Link-Time Configuration
 * @version 1.0.0
 * @date 2024-05-05
 */

#include "Xcp.h"
#include "Xcp_Cfg.h"

/*==================[Global Constants]======================================*/

/* Event Channel Configurations */
static const Xcp_EventChannelType Xcp_EventChannels[XCP_MAX_EVENT_CHANNELS] = {
    {
        .EventChannelName = "ECU_Cyclic_10ms",
        .EventChannelTimeCycle = 10,
        .EventChannelTimeUnit = XCP_UNIT_1MS,
        .EventChannelPriority = 1,
        .EventChannelMaxDaqList = 4
    },
    {
        .EventChannelName = "ECU_Cyclic_100ms",
        .EventChannelTimeCycle = 100,
        .EventChannelTimeUnit = XCP_UNIT_1MS,
        .EventChannelPriority = 2,
        .EventChannelMaxDaqList = 2
    }
};

/* DAQ List Configurations */
Xcp_DaqListType Xcp_DaqLists[XCP_MAX_DAQ] = {
    {
        .DaqListNumber = 0,
        .Mode = XCP_DAQ_MODE_ALTERNATING,
        .Prescaler = 1,
        .EventChannel = 0,
        .FirstOdt = 0,
        .OdtCount = 2,
        .IsRunning = XCP_FALSE
    },
    {
        .DaqListNumber = 1,
        .Mode = XCP_DAQ_MODE_ALTERNATING,
        .Prescaler = 1,
        .EventChannel = 0,
        .FirstOdt = 2,
        .OdtCount = 1,
        .IsRunning = XCP_FALSE
    },
    {
        .DaqListNumber = 2,
        .Mode = XCP_DAQ_MODE_ALTERNATING,
        .Prescaler = 10,
        .EventChannel = 1,
        .FirstOdt = 3,
        .OdtCount = 1,
        .IsRunning = XCP_FALSE
    }
};

/* ODT Configurations */
static Xcp_OdtType Xcp_Odts[XCP_MAX_ODT] = {
    { .OdtNumber = 0, .Entries = {{0}}, .EntryCount = 0 },
    { .OdtNumber = 1, .Entries = {{0}}, .EntryCount = 0 },
    { .OdtNumber = 2, .Entries = {{0}}, .EntryCount = 0 },
    { .OdtNumber = 3, .Entries = {{0}}, .EntryCount = 0 }
};

/* STIM List Configurations */
static Xcp_StimListType Xcp_StimLists[XCP_MAX_STIM] = {
    {
        .StimListNumber = 0,
        .Mode = XCP_STIM_MODE_ALTERNATING,
        .EventChannel = 0,
        .FirstOdt = 4,
        .OdtCount = 1,
        .IsRunning = XCP_FALSE
    }
};

/* Calibration Segment Configuration */
static const Xcp_SegmentType Xcp_Segments[XCP_MAX_SEGMENTS] = {
    {
        .SegmentName = "CalRAM",
        .SegmentNumber = 0,
        .Address = (uint32)0x20000000,
        .Length = 0x10000,
        .Pages = {
            {
                .PageNumber = 0,
                .PageName = "Working",
                .Address = (uint32)0x20000000,
                .Length = 0x10000,
                .Access = XCP_PAGE_READ_WRITE,
                .IsInit = XCP_TRUE
            },
            {
                .PageNumber = 1,
                .PageName = "Reference",
                .Address = (uint32)0x20010000,
                .Length = 0x10000,
                .Access = XCP_PAGE_READ_ONLY,
                .IsInit = XCP_FALSE
            }
        },
        .PageCount = 2
    }
};

/* Session Configuration */
static const Xcp_SessionType Xcp_SessionConfig = {
    .SessionConfigurationId = 0x0001,
    .MaxCto = XCP_MAX_CTO,
    .MaxDto = XCP_MAX_DTO,
    .MaxWriteDaqMultipleElements = 8,
    .CommunicationMode = XCP_COMM_MODE_POLLING,
    .Timing = {
        .T1 = 1000,  /* Response pending timeout [ms] */
        .T2 = 100,   /* Standard timeout [ms] */
        .T3 = 50,    /* Start timeout [ms] */
        .T4 = 10000, /* Session timeout [ms] */
        .T5 = 100,   /* Status timeout [ms] */
        .T6 = 50,    /* Sync timeout [ms] */
        .T7 = 50     /* Transport layer specific timeout [ms] */
    }
};

/* Seed & Key Configuration */
const uint8 Xcp_Seed[XCP_MAX_SEED_SIZE] = {0x01, 0x02, 0x03, 0x04};
const uint8 Xcp_Key[XCP_MAX_KEY_SIZE] = {0xAB, 0xCD, 0xEF, 0x12};

/* Resource Protection */
static const Xcp_ResourceProtectionType Xcp_ResourceProtection = {
    .CalPag = XCP_PROTECT_NONE,
    .Daq = XCP_PROTECT_NONE,
    .Stim = XCP_PROTECT_NONE,
    .Pgm = XCP_PROTECT_NONE
};

/* Transport Layer Configuration */
static const Xcp_TransportLayerType Xcp_TransportConfig = {
    .TransportLayerType = XCP_TRANSPORT_CAN,
    .Can = {
        .CanIdRequest = 0x500,
        .CanIdResponse = 0x501,
        .CanIdBroadcast = 0x502,
        .Baudrate = 500000
    }
};

/* Module Configuration */
static const Xcp_ConfigType Xcp_Config = {
    .General = {
        .MaxCto = XCP_MAX_CTO,
        .MaxDto = XCP_MAX_DTO,
        .MaxEventChannels = XCP_MAX_EVENT_CHANNELS,
        .MaxDaq = XCP_MAX_DAQ,
        .MaxOdt = XCP_MAX_ODT,
        .MaxOdtEntries = XCP_MAX_ODT_ENTRIES,
        .MaxStim = XCP_MAX_STIM,
        .MaxSegments = XCP_MAX_SEGMENTS,
        .MaxPages = XCP_MAX_PAGES,
        .MaxSeedKeySize = XCP_MAX_SEED_SIZE,
        .TimestampSupport = XCP_TRUE,
        .TimestampSize = XCP_TS_4BYTE,
        .TimestampResolution = XCP_TS_UNIT_1US,
        .IdentificationField = XCP_IDF_ABSOLUTE_ODT,
        .AddressGranularity = XCP_AG_DWORD
    },
    .EventChannels = Xcp_EventChannels,
    .DaqLists = Xcp_DaqLists,
    .Odts = Xcp_Odts,
    .StimLists = Xcp_StimLists,
    .Segments = Xcp_Segments,
    .Session = &Xcp_SessionConfig,
    .ResourceProtection = &Xcp_ResourceProtection,
    .TransportLayer = &Xcp_TransportConfig
};

/* Version Info */
static const Xcp_VersionInfoType Xcp_VersionInfo = {
    .VendorID = XCP_VENDOR_ID,
    .ModuleID = XCP_MODULE_ID,
    .SwMajorVersion = XCP_SW_MAJOR_VERSION,
    .SwMinorVersion = XCP_SW_MINOR_VERSION,
    .SwPatchVersion = XCP_SW_PATCH_VERSION
};

/*==================[End of File]===========================================*/
