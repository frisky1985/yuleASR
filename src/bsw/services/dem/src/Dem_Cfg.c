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
/* @req SWS_Dem_00001 @req SWS_Dem_00002 @req SWS_Dem_00003 */


/**
 * @file Dem_Cfg.c
 * @brief Diagnostic Event Manager - Link-Time Configuration
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Separated configuration data from test file
 * This file provides the actual production configuration for Dem module
 */

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Dem_Types.h"
#include "Dem_Cfg.h"
#include "Compiler.h"
#include "MemMap.h"

/*==================================================================================================
*                                    EVENT PARAMETERS CONFIGURATION
==================================================================================================*/
/**
 * @brief Event parameter configurations
 * 
 * This table defines all diagnostic events in the system.
 * Each event is mapped to a DTC and configured with debounce parameters.
 */
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* Example event configurations - customize for your application */
STATIC const Dem_EventParameterType Dem_EventParameters[DEM_NUM_EVENTS] = {
    /* Event 1: ECU Internal Watchdog Timeout */
    {
        1U,                             /* EventId */
        0x010101U,                      /* DTC */
        1U,                             /* EventPriority */
        TRUE,                           /* EventAvailable */
        TRUE,                           /* EventReporting */
        1U,                             /* EventFailureCycleCounterThreshold */
        2U,                             /* EventConfirmationThreshold */
        DEM_DEBOUNCE_ALGORITHM_COUNTER, /* DebounceAlgorithm */
        TRUE,                           /* EventCounterBased */
        FALSE,                          /* EventTimeBased */
        FALSE,                          /* EventMonitorInternal */
        127,                            /* DebounceCounterFailedThreshold */
        -128,                           /* DebounceCounterPassedThreshold */
        100U,                           /* DebounceTimeFailedThresholdMs */
        100U                            /* DebounceTimePassedThresholdMs */
    },
    /* Event 2: Voltage Too High */
    {
        2U,
        0x010102U,
        2U,
        TRUE,
        TRUE,
        1U,
        2U,
        DEM_DEBOUNCE_ALGORITHM_TIME,
        FALSE,
        TRUE,
        FALSE,
        127,
        -128,
        1000U,  /* 1 second */
        1000U
    },
    /* Event 3: CAN Bus Off */
    {
        3U,
        0x010301U,
        1U,
        TRUE,
        TRUE,
        1U,
        1U,  /* Confirm immediately */
        DEM_DEBOUNCE_ALGORITHM_MONITOR,
        FALSE,
        FALSE,
        TRUE,
        127,
        -128,
        0U,
        0U
    },
    /* Event 4: Communication Timeout */
    {
        4U,
        0x010401U,
        3U,
        TRUE,
        TRUE,
        2U,
        3U,
        DEM_DEBOUNCE_ALGORITHM_COUNTER,
        TRUE,
        FALSE,
        FALSE,
        127,
        -128,
        100U,
        100U
    },
    /* Event 5: Sensor Signal Range Check */
    {
        5U,
        0x010501U,
        2U,
        TRUE,
        TRUE,
        1U,
        2U,
        DEM_DEBOUNCE_ALGORITHM_COUNTER,
        TRUE,
        FALSE,
        FALSE,
        64,
        -64,
        100U,
        100U
    },
    /* Add more events as needed - placeholder for remaining entries */
    /* Events 6-128: Reserved for future use */
};

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    DTC PARAMETERS CONFIGURATION
==================================================================================================*/
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* DID IDs for freeze frames */
STATIC const uint16 Dem_FreezeFrameDids_01[] = {0xF100U, 0xF101U, 0xF102U, 0xF103U};
STATIC const uint16 Dem_FreezeFrameDids_02[] = {0xF100U, 0xF101U, 0xF400U, 0xF401U, 0xF402U};

/* Freeze frame record configurations */
STATIC const Dem_FreezeFrameRecordType Dem_FreezeFrameRecords[DEM_NUM_FREEZE_FRAME_RECORDS] = {
    {1U, 4U, Dem_FreezeFrameDids_01, TRUE},
    {2U, 5U, Dem_FreezeFrameDids_02, TRUE},
    {3U, 0U, NULL_PTR, FALSE},  /* Reserved */
    {4U, 0U, NULL_PTR, FALSE},
    {5U, 0U, NULL_PTR, FALSE},
    {6U, 0U, NULL_PTR, FALSE},
    {7U, 0U, NULL_PTR, FALSE},
    {8U, 0U, NULL_PTR, FALSE}
};

/* Extended data record configurations */
STATIC const Dem_ExtendedDataRecordType Dem_ExtendedDataRecords[DEM_NUM_EXTENDED_DATA_RECORDS] = {
    {1U, 4U, TRUE, FALSE},   /* Occurrence Counter (4 bytes) */
    {2U, 4U, TRUE, FALSE},   /* Aging Counter (4 bytes) */
    {3U, 8U, TRUE, TRUE},    /* Timestamp (8 bytes) */
    {4U, 32U, TRUE, FALSE}   /* Additional environmental data */
};

/* DTC parameter configurations */
STATIC const Dem_DtcParameterType Dem_DtcParameters[DEM_NUM_DTCS] = {
    /* DTC 0x010101: ECU Internal Watchdog Timeout */
    {
        0x010101U,                      /* Dtc */
        DEM_SEVERITY_CHECK_IMMEDIATELY, /* DtcSeverity */
        0x01U,                          /* DtcFunctionalUnit */
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,  /* DtcOrigin */
        TRUE,                           /* DtcAvailable */
        TRUE,                           /* DtcReporting */
        40U,                            /* AgingThreshold */
        FALSE                           /* MemoryEntryOverflow */
    },
    /* DTC 0x010102: Voltage Too High */
    {
        0x010102U,
        DEM_SEVERITY_CHECK_AT_NEXT_HALT,
        0x01U,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE,
        TRUE,
        40U,
        FALSE
    },
    /* DTC 0x010301: CAN Bus Off */
    {
        0x010301U,
        DEM_SEVERITY_CHECK_IMMEDIATELY,
        0x03U,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE,
        TRUE,
        40U,
        FALSE
    },
    /* DTC 0x010401: Communication Timeout */
    {
        0x010401U,
        DEM_SEVERITY_MAINTENANCE_ONLY,
        0x04U,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE,
        TRUE,
        40U,
        FALSE
    },
    /* DTC 0x010501: Sensor Signal Range Check */
    {
        0x010501U,
        DEM_SEVERITY_CHECK_AT_NEXT_HALT,
        0x05U,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE,
        TRUE,
        40U,
        FALSE
    },
    /* DTC 0x123456: User defined example */
    {
        0x123456U,
        DEM_SEVERITY_NO_SEVERITY,
        0x12U,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE,
        TRUE,
        40U,
        FALSE
    },
    /* DTC 0x123457: User defined example */
    {
        0x123457U,
        DEM_SEVERITY_NO_SEVERITY,
        0x12U,
        DEM_DTC_ORIGIN_PRIMARY_MEMORY,
        TRUE,
        TRUE,
        40U,
        FALSE
    },
    /* Reserved DTCs for future use */
    {0, 0, 0, DEM_DTC_ORIGIN_PRIMARY_MEMORY, FALSE, FALSE, 40U, FALSE},
    {0, 0, 0, DEM_DTC_ORIGIN_PRIMARY_MEMORY, FALSE, FALSE, 40U, FALSE},
    {0, 0, 0, DEM_DTC_ORIGIN_PRIMARY_MEMORY, FALSE, FALSE, 40U, FALSE},
    /* ... continue for DEM_NUM_DTCS entries */
};

/* Indicator configurations */
STATIC const Dem_IndicatorType Dem_Indicators[DEM_NUM_INDICATORS] = {
    {DEM_INDICATOR_MIL, DEM_INDICATOR_CONTINUOUS, 3U},
    {DEM_INDICATOR_SVS, DEM_INDICATOR_CONTINUOUS, 3U},
    {DEM_INDICATOR_AWLS, DEM_INDICATOR_BLINKING, 1U},
    {DEM_INDICATOR_PL, DEM_INDICATOR_CONTINUOUS, 2U},
    {DEM_INDICATOR_SBL, DEM_INDICATOR_ON_DEMAND, 1U},
    {DEM_INDICATOR_AWLS2, DEM_INDICATOR_BLINKING, 1U},
    {DEM_INDICATOR_RSL, DEM_INDICATOR_BLINKING_CONT, 1U},
    {DEM_INDICATOR_ESL, DEM_INDICATOR_CONTINUOUS, 2U}
};

/* Notification callback declarations (to be implemented by integrator) */
extern void Dem_ClearDTCLambdaNotification(void);
extern void Dem_ClearDTCStartNotification(void);
extern void Dem_ClearDTCFinishNotification(void);

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    GLOBAL CONFIGURATION
==================================================================================================*/
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief DEM module configuration
 * 
 * This is the main configuration structure used by Dem_Init().
 */
const Dem_ConfigType Dem_Config = {
    Dem_EventParameters,          /* EventParameters */
    5U,                           /* NumEvents - actual configured events */
    Dem_DtcParameters,            /* DtcParameters */
    7U,                           /* NumDtcs - actual configured DTCs */
    Dem_FreezeFrameRecords,       /* FreezeFrameRecords */
    DEM_NUM_FREEZE_FRAME_RECORDS, /* NumFreezeFrameRecords */
    Dem_ExtendedDataRecords,      /* ExtendedDataRecords */
    DEM_NUM_EXTENDED_DATA_RECORDS,/* NumExtendedDataRecords */
    Dem_Indicators,               /* Indicators */
    DEM_NUM_INDICATORS,           /* NumIndicators */
    (boolean)DEM_DEV_ERROR_DETECT,       /* DevErrorDetect */
    (boolean)DEM_VERSION_INFO_API,       /* VersionInfoApi */
    (boolean)DEM_CLEAR_DTC_SUPPORTED,    /* ClearDtcSupported */
    (boolean)DEM_CLEAR_DTC_LIMITATION,   /* ClearDtcLimitation */
    DEM_DTC_STATUS_AVAILABILITY_MASK,    /* DtcStatusAvailabilityMask */
    (boolean)DEM_OBD_RELEVANT_SUPPORT,   /* OBDRelevantSupport */
    (boolean)DEM_J1939_SUPPORT,          /* J1939Support */
    FALSE,                        /* TriggerFimReports */
    TRUE,                         /* TriggerMonitorInitBeforeClearOk */
#if defined(DEM_CLEAR_DTC_LAMBDA_NOTIFICATION)
    Dem_ClearDTCLambdaNotification, /* ClearDTCLambdaNotification */
#else
    NULL_PTR,
#endif
#if defined(DEM_CLEAR_DTC_START_NOTIFICATION)
    Dem_ClearDTCStartNotification,  /* ClearDTCStartNotification */
#else
    NULL_PTR,
#endif
#if defined(DEM_CLEAR_DTC_FINISH_NOTIFICATION)
    Dem_ClearDTCFinishNotification, /* ClearDTCFinishNotification */
#else
    NULL_PTR,
#endif
};

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    VALIDATION CHECKS
==================================================================================================*/
/* Ensure configuration consistency */
#if (DEM_NUM_EVENTS < 1)
#error "DEM_NUM_EVENTS must be at least 1"
#endif

#if (DEM_NUM_DTCS < 1)
#error "DEM_NUM_DTCS must be at least 1"
#endif

#if (DEM_NUM_FREEZE_FRAME_RECORDS < 1)
#error "DEM_NUM_FREEZE_FRAME_RECORDS must be at least 1"
#endif

#if (DEM_NUM_EXTENDED_DATA_RECORDS < 1)
#error "DEM_NUM_EXTENDED_DATA_RECORDS must be at least 1"
#endif
