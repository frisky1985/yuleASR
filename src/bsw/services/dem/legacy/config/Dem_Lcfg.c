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

/***********************************************************************************************************************
 * File:        Dem_Lcfg.c
 * Description: Dem link-time configuration (RAM configuration)
 *              This file contains the static configuration tables for Dem module
 **********************************************************************************************************************/

#include "Dem.h"
#include "Dem_Cfg.h"

/*==================================================================================================
 *                                      EVENT CONFIGURATION TABLE
==================================================================================================*/

/* Event configuration structure */
typedef struct {
    Dem_EventIdType EventId;
    uint32 DTC;
    Dem_DTCOriginType DTCOrigin;
    Dem_DTCKindType DTCKind;
    uint8 Priority;
    boolean ImmediateStorage;
    uint8 DebounceAlgorithmRef;
    uint8 OperationCycleRef;
    uint8 IndicatorAttributeRef;
    uint8 FreezeFrameRecNumStart;
    uint8 FreezeFrameRecNumEnd;
    uint8 ExtendedDataRef;
} Dem_EventConfigType;

/* Debounce algorithm configuration */
typedef struct {
    uint8 AlgorithmId;
    Dem_DebounceAlgorithmClassType AlgorithmClass;
    sint16 CounterFailedThreshold;
    sint16 CounterPassedThreshold;
    sint16 CounterIncrementStep;
    sint16 CounterDecrementStep;
    uint16 TimeFailedThreshold;
    uint16 TimePassedThreshold;
} Dem_DebounceAlgorithmConfigType;

/* Operation cycle configuration */
typedef struct {
    Dem_OperationCycleIdType CycleId;
    Dem_OperationCycleType CycleType;
    boolean AutoStart;
} Dem_OperationCycleConfigType;

/* Indicator configuration */
typedef struct {
    uint8 IndicatorId;
    uint8 Behavior;
    uint8 FailureCycleCounterThreshold;
    uint8 HealingCycleCounterThreshold;
} Dem_IndicatorConfigType;

/* Freeze frame record configuration */
typedef struct {
    uint8 RecordNumber;
    uint8 DidCount;
    const uint16* DidIds;
} Dem_FreezeFrameRecordConfigType;

/* Extended data record configuration */
typedef struct {
    uint8 RecordNumber;
    uint16 DataSize;
    uint8 UpdateRule;
} Dem_ExtendedDataRecordConfigType;

/*==================================================================================================
 *                                      CONFIGURATION TABLES
==================================================================================================*/

#if (DEM_CFG_EventDebounceSupport == STD_ON)
/* Debounce algorithm configuration table */
static const Dem_DebounceAlgorithmConfigType Dem_DebounceAlgorithmConfig[DEM_CFG_MAX_DEBOUNCE_ALGORITHMS] = {
    /* AlgorithmId, Class,                     FailedThr, PassedThr, IncStep, DecStep, TimeFailed, TimePassed */
    {0, DEM_DEBOUNCE_COUNTER_BASED,             127,       -128,      1,       -1,      0,          0},
    {1, DEM_DEBOUNCE_COUNTER_BASED,             64,        -64,       2,       -2,      0,          0},
    {2, DEM_DEBOUNCE_TIME_BASED,                0,         0,         0,       0,       100,        100},
    /* Add more debounce algorithms as needed */
};
#endif

#if (DEM_CFG_OperationCycleSupport == STD_ON)
/* Operation cycle configuration table */
static const Dem_OperationCycleConfigType Dem_OperationCycleConfig[DEM_CFG_MAX_OPERATION_CYCLES] = {
    /* CycleId,                    CycleType,                        AutoStart */
    {DEM_OPCYC_IGNITION,           DEM_OPCYC_IGNITION,               TRUE},
    {DEM_OPCYC_OBD_DCY,            DEM_OPCYC_OBD_DCY,                TRUE},
    {DEM_OPCYC_WARMUP,             DEM_OPCYC_WARMUP,                 FALSE},
    {DEM_OPCYC_POWER,              DEM_OPCYC_POWER,                  TRUE},
};
#endif

#if (DEM_CFG_IndicatorSupport == STD_ON)
/* Indicator configuration table */
static const Dem_IndicatorConfigType Dem_IndicatorConfig[DEM_CFG_MAX_INDICATORS] = {
    /* IndicatorId, Behavior,                  FailureThr, HealingThr */
    {0,             DEM_INDICATOR_BLINKING,    3,          3},  /* MIL */
    {1,             DEM_INDICATOR_CONTINUOUS,  1,          1},  /* WIF */
    {2,             DEM_INDICATOR_BLINKING,    2,          2},  /* RSL */
    {3,             DEM_INDICATOR_CONTINUOUS,  1,          1},  /* AWL */
};
#endif

/* Event configuration table */
static const Dem_EventConfigType Dem_EventConfig[DEM_CFG_MAX_NUMBER_EVENTS] = {
    /* EventId, DTC,        Origin,                   Kind,                    Priority, ImmStorage, DebounceRef, CycleRef, IndRef, FFStart, FFEnd, ExtRef */
    {0,         0x010101,   DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_DTC_KIND_EMISSION_REL_DTCS, 1, TRUE,       0,           0,        0,      1,       2,     0},
    {1,         0x010102,   DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_DTC_KIND_EMISSION_REL_DTCS, 2, FALSE,      0,           0,        0,      1,       2,     0},
    {2,         0x010103,   DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_DTC_KIND_EMISSION_REL_DTCS, 1, TRUE,       1,           0,        0,      1,       2,     0},
    {3,         0x020101,   DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_DTC_KIND_ALL_DTCS,          3, FALSE,      0,           1,        1,      1,       1,     0},
    {4,         0x020102,   DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_DTC_KIND_ALL_DTCS,          2, FALSE,      2,           1,        1,      1,       2,     0},
    /* Add more events as needed */
};

/* DTC configuration table */
typedef struct {
    uint32 DTC;
    Dem_DTCGroupType DTCGroup;
    uint8 FunctionalUnit;
    boolean ImmediateStorage;
    uint8 Priority;
} Dem_DTCConfigType;

static const Dem_DTCConfigType Dem_DTCConfig[DEM_CFG_MAX_NUMBER_DTCS] = {
    /* DTC,       Group,                   FunctionalUnit, ImmStorage, Priority */
    {0x010101,   DEM_DTC_GROUP_EMISSION_DTCS, 0x01,           TRUE,       1},
    {0x010102,   DEM_DTC_GROUP_EMISSION_DTCS, 0x01,           FALSE,      2},
    {0x010103,   DEM_DTC_GROUP_EMISSION_DTCS, 0x01,           TRUE,       1},
    {0x020101,   DEM_DTC_GROUP_CHASSIS_DTCS,  0x02,           FALSE,      3},
    {0x020102,   DEM_DTC_GROUP_CHASSIS_DTCS,  0x02,           FALSE,      2},
    /* Add more DTCs as needed */
};

/* Freeze frame DID configuration */
static const uint16 Dem_FreezeFrameDids[] = {
    0x0100,  /* Vehicle Speed */
    0x0101,  /* Engine Speed */
    0x0105,  /* Engine Coolant Temperature */
    0x015C,  /* Battery Voltage */
    0xF400,  /* Odometer */
};

/* Freeze frame record configuration */
static const Dem_FreezeFrameRecordConfigType Dem_FreezeFrameRecordConfig[DEM_CFG_MAX_FREEZEFRAME_RECORDS] = {
    {1, 5, Dem_FreezeFrameDids},
    {2, 5, Dem_FreezeFrameDids},
    {3, 5, Dem_FreezeFrameDids},
};

/* Extended data record configuration */
static const Dem_ExtendedDataRecordConfigType Dem_ExtendedDataRecordConfig[DEM_CFG_MAX_EXTENDED_DATA_RECORDS] = {
    {1, 4, DEM_UPDATE_RECORD_UPDATE},   /* Odometer at fault */
    {2, 2, DEM_UPDATE_RECORD_UPDATE},   /* Time at fault */
};

/*==================================================================================================
 *                                      EXTERNAL ACCESS
==================================================================================================*/

/* Configuration access macros */
#define DEM_CFG_GET_EVENT_CONFIG(idx)          (&Dem_EventConfig[idx])
#define DEM_CFG_GET_DTC_CONFIG(idx)            (&Dem_DTCConfig[idx])
#define DEM_CFG_GET_DEBOUNCE_CONFIG(idx)       (&Dem_DebounceAlgorithmConfig[idx])
#define DEM_CFG_GET_OPERATION_CYCLE_CONFIG(idx) (&Dem_OperationCycleConfig[idx])
#define DEM_CFG_GET_INDICATOR_CONFIG(idx)      (&Dem_IndicatorConfig[idx])
#define DEM_CFG_GET_FF_RECORD_CONFIG(idx)      (&Dem_FreezeFrameRecordConfig[idx])
#define DEM_CFG_GET_EXT_DATA_RECORD_CONFIG(idx) (&Dem_ExtendedDataRecordConfig[idx])

/* Configuration size macros */
#define DEM_CFG_GET_NUM_EVENTS()               (sizeof(Dem_EventConfig) / sizeof(Dem_EventConfigType))
#define DEM_CFG_GET_NUM_DTCS()                 (sizeof(Dem_DTCConfig) / sizeof(Dem_DTCConfigType))
#define DEM_CFG_GET_NUM_DEBOUNCE_ALGORITHMS()  (sizeof(Dem_DebounceAlgorithmConfig) / sizeof(Dem_DebounceAlgorithmConfigType))
#define DEM_CFG_GET_NUM_OPERATION_CYCLES()     (sizeof(Dem_OperationCycleConfig) / sizeof(Dem_OperationCycleConfigType))
#define DEM_CFG_GET_NUM_INDICATORS()           (sizeof(Dem_IndicatorConfig) / sizeof(Dem_IndicatorConfigType))

/*==================================================================================================
 *                                      VALIDATION
==================================================================================================*/

/* Static assertions for configuration consistency */
#if (DEM_CFG_GET_NUM_EVENTS() > DEM_CFG_MAX_NUMBER_EVENTS)
    #error "Event configuration exceeds maximum allowed events"
#endif

#if (DEM_CFG_GET_NUM_DTCS() > DEM_CFG_MAX_NUMBER_DTCS)
    #error "DTC configuration exceeds maximum allowed DTCs"
#endif

#if (DEM_DEV_ERROR_DETECT == STD_ON)
/* Development error checking for configuration */
    #define DEM_VALIDATE_CONFIG_PTR(ptr, apiId) \
        do { \
            if ((ptr) == NULL_PTR) { \
                Det_ReportError(DEM_MODULE_ID, 0, (apiId), DEM_E_PARAM_POINTER); \
            } \
        } while(0)
#else
    #define DEM_VALIDATE_CONFIG_PTR(ptr, apiId)
#endif

