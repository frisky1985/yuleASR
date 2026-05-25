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
 * File:        Dem_PBcfg.c
 * Description: Dem post-build configuration (Flash configuration)
 *              Contains configuration that can be modified after build time
 **********************************************************************************************************************/

#include "Dem.h"
#include "Dem_Cfg.h"

/*==================================================================================================
 *                                      POST-BUILD CONFIGURATION
==================================================================================================*/

/* Post-build configuration version */
#define DEM_PBCFG_MAJOR_VERSION     (1U)
#define DEM_PBCFG_MINOR_VERSION     (0U)
#define DEM_PBCFG_PATCH_VERSION     (0U)

/*==================================================================================================
 *                                      CALLBACK CONFIGURATION
==================================================================================================*/

#if (DEM_CFG_CALLBACK_ON_EVC_STATUS_CHANGED == STD_ON)
/* Event status changed callback configuration */
typedef void (*Dem_CallbackOnEventStatusChangedType)(Dem_EventIdType EventId, Dem_EventStatusExtendedType EventStatusOld, Dem_EventStatusExtendedType EventStatusNew);

static const Dem_CallbackOnEventStatusChangedType Dem_CallbackOnEventStatusChangedTable[DEM_CFG_MAX_NUMBER_EVENTS] = {
    NULL_PTR,   /* Event 0 - no callback */
    NULL_PTR,   /* Event 1 - no callback */
    NULL_PTR,   /* Event 2 - no callback */
    /* Add more callbacks as needed */
};
#endif

#if (DEM_CFG_CALLBACK_ON_DTC_STATUS_CHANGED == STD_ON)
/* DTC status changed callback configuration */
typedef void (*Dem_CallbackOnDTCStatusChangedType)(uint32 DTC, Dem_DTCStatusMaskType DTCStatusOld, Dem_DTCStatusMaskType DTCStatusNew);

static const Dem_CallbackOnDTCStatusChangedType Dem_CallbackOnDTCStatusChangedTable[DEM_CFG_MAX_NUMBER_DTCS] = {
    NULL_PTR,   /* DTC 0x010101 - no callback */
    NULL_PTR,   /* DTC 0x010102 - no callback */
    NULL_PTR,   /* DTC 0x010103 - no callback */
    /* Add more callbacks as needed */
};
#endif

#if (DEM_CFG_CALLBACK_ON_CYCLE_STATUS_CHANGED == STD_ON)
/* Operation cycle status changed callback configuration */
typedef void (*Dem_CallbackOnOperationCycleStatusChangedType)(Dem_OperationCycleIdType OperationCycleId, Dem_OperationCycleStateType CycleState);

static const Dem_CallbackOnOperationCycleStatusChangedType Dem_CallbackOnCycleStatusChangedTable[DEM_CFG_MAX_OPERATION_CYCLES] = {
    NULL_PTR,   /* Ignition cycle - no callback */
    NULL_PTR,   /* OBD driving cycle - no callback */
    NULL_PTR,   /* Warmup cycle - no callback */
    NULL_PTR,   /* Power cycle - no callback */
};
#endif

/*==================================================================================================
 *                                      MONITOR CONFIGURATION
==================================================================================================*/

/* Monitor initialization function configuration */
typedef Std_ReturnType (*Dem_InitMonitorForEventType)(Dem_InitMonitorReasonType InitMonitorReason);

static const Dem_InitMonitorForEventType Dem_InitMonitorForEventTable[DEM_CFG_MAX_NUMBER_EVENTS] = {
    NULL_PTR,   /* Event 0 - no initialization */
    NULL_PTR,   /* Event 1 - no initialization */
    NULL_PTR,   /* Event 2 - no initialization */
    /* Add more initialization functions as needed */
};

/*==================================================================================================
 *                                      DTC GROUP CONFIGURATION
==================================================================================================*/

/* DTC group configuration */
typedef struct {
    Dem_DTCGroupType DTCGroup;
    uint32 DTCLow;
    uint32 DTCHigh;
} Dem_DTCGroupInfoType;

static const Dem_DTCGroupInfoType Dem_DTCGroupConfig[] = {
    {DEM_DTC_GROUP_ALL_DTCS,         0x000000, 0xFFFFFF},
    {DEM_DTC_GROUP_EMISSION_DTCS,    0x010000, 0x01FFFF},
    {DEM_DTC_GROUP_CHASSIS_DTCS,     0x020000, 0x02FFFF},
    {DEM_DTC_GROUP_POWERTRAIN_DTCS,  0x030000, 0x03FFFF},
    {DEM_DTC_GROUP_NETWORK_DTCS,     0x040000, 0x04FFFF},
    {DEM_DTC_GROUP_BODY_DTCS,        0x050000, 0x05FFFF},
    {DEM_DTC_GROUP_UDS_DTCS,         0x060000, 0x06FFFF},
};

#define DEM_NUM_DTC_GROUPS    (sizeof(Dem_DTCGroupConfig) / sizeof(Dem_DTCGroupInfoType))

/*==================================================================================================
 *                                      CLEAR DTC CONFIGURATION
==================================================================================================*/

/* Clear DTC behavior configuration */
typedef struct {
    boolean ClearAllDTCsAllowed;
    boolean ClearEmissionDTConly;
    uint32  ClearDTCMaxTimeMs;
} Dem_ClearDTCConfigType;

static const Dem_ClearDTCConfigType Dem_ClearDTCConfig = {
    TRUE,       /* ClearAllDTCsAllowed */
    FALSE,      /* ClearEmissionDTConly */
    10000U,     /* ClearDTCMaxTimeMs */
};

/*==================================================================================================
 *                                      MEMORY CONFIGURATION
==================================================================================================*/

/* Memory entry configuration */
typedef struct {
    Dem_DTCOriginType Origin;
    uint8 MaxNumEntries;
    boolean OverflowIndication;
} Dem_MemoryConfigType;

static const Dem_MemoryConfigType Dem_MemoryConfig[] = {
    {DEM_DTC_ORIGIN_PRIMARY_MEMORY,   DEM_CFG_PRIMARY_MEMORY_MAX_ENTRIES,   TRUE},
    {DEM_DTC_ORIGIN_MIRROR_MEMORY,    DEM_CFG_MIRROR_MEMORY_MAX_ENTRIES,    TRUE},
    {DEM_DTC_ORIGIN_PERMANENT_MEMORY, DEM_CFG_PERMANENT_MEMORY_MAX_ENTRIES, FALSE},
};

#define DEM_NUM_MEMORIES    (sizeof(Dem_MemoryConfig) / sizeof(Dem_MemoryConfigType))

/*==================================================================================================
 *                                      AGING CONFIGURATION
==================================================================================================*/

#if (DEM_CFG_AgingSupport == STD_ON)
/* Aging configuration */
typedef struct {
    boolean AgingAllowed;
    uint8 AgingCycleThreshold;
    Dem_OperationCycleIdType AgingCycle;
} Dem_AgingConfigType;

static const Dem_AgingConfigType Dem_AgingConfig[DEM_CFG_MAX_NUMBER_DTCS] = {
    {TRUE,  40, DEM_OPCYC_IGNITION},  /* DTC 0x010101 */
    {TRUE,  40, DEM_OPCYC_IGNITION},  /* DTC 0x010102 */
    {TRUE,  40, DEM_OPCYC_IGNITION},  /* DTC 0x010103 */
    {FALSE, 0,  0},                   /* DTC 0x020101 - no aging */
    {FALSE, 0,  0},                   /* DTC 0x020102 - no aging */
};
#endif

/*==================================================================================================
 *                                      PID CONFIGURATION
==================================================================================================*/

#if (DEM_CFG_OBDSupport == STD_ON)
/* PID configuration for OBD */
typedef struct {
    uint8 PID;
    uint8 DataSize;
    uint8 (*GetPIDDataFunc)(uint8* Buffer);
} Dem_PIDConfigType;

/* Placeholder PID functions - should be implemented by application */
static uint8 Dem_GetPID01Data(uint8* Buffer) { (void)Buffer; return 0; }
static uint8 Dem_GetPID41Data(uint8* Buffer) { (void)Buffer; return 0; }
static uint8 Dem_GetPID1CData(uint8* Buffer) { (void)Buffer; return 0; }

static const Dem_PIDConfigType Dem_PIDConfig[] = {
    {0x01, 4, Dem_GetPID01Data},  /* Monitor status */
    {0x41, 4, Dem_GetPID41Data},  /* Monitor status this drive cycle */
    {0x1C, 1, Dem_GetPID1CData},  /* OBD standards conformance */
};

#define DEM_NUM_PIDS    (sizeof(Dem_PIDConfig) / sizeof(Dem_PIDConfigType))
#endif

/*==================================================================================================
 *                                      POST-BUILD CONFIG ROOT
==================================================================================================*/

typedef struct {
    uint16 ConfigId;
    uint16 ConfigMajorVersion;
    uint16 ConfigMinorVersion;
    uint16 ConfigPatchVersion;
    
#if (DEM_CFG_CALLBACK_ON_EVC_STATUS_CHANGED == STD_ON)
    const Dem_CallbackOnEventStatusChangedType* EventStatusChangedCallbacks;
#endif
    
#if (DEM_CFG_CALLBACK_ON_DTC_STATUS_CHANGED == STD_ON)
    const Dem_CallbackOnDTCStatusChangedType* DTCStatusChangedCallbacks;
#endif
    
#if (DEM_CFG_CALLBACK_ON_CYCLE_STATUS_CHANGED == STD_ON)
    const Dem_CallbackOnOperationCycleStatusChangedType* CycleStatusChangedCallbacks;
#endif
    
    const Dem_InitMonitorForEventType* InitMonitorCallbacks;
    const Dem_DTCGroupInfoType* DTCGroupConfig;
    uint8 NumDTCGroups;
    const Dem_ClearDTCConfigType* ClearDTCConfig;
    const Dem_MemoryConfigType* MemoryConfig;
    uint8 NumMemories;
    
#if (DEM_CFG_AgingSupport == STD_ON)
    const Dem_AgingConfigType* AgingConfig;
#endif
    
#if (DEM_CFG_OBDSupport == STD_ON)
    const Dem_PIDConfigType* PIDConfig;
    uint8 NumPIDs;
#endif
} Dem_PBConfigRootType;

/* Post-build configuration root */
static const Dem_PBConfigRootType Dem_PBConfigRoot = {
    0x0001,     /* ConfigId */
    DEM_PBCFG_MAJOR_VERSION,
    DEM_PBCFG_MINOR_VERSION,
    DEM_PBCFG_PATCH_VERSION,
    
#if (DEM_CFG_CALLBACK_ON_EVC_STATUS_CHANGED == STD_ON)
    Dem_CallbackOnEventStatusChangedTable,
#endif
    
#if (DEM_CFG_CALLBACK_ON_DTC_STATUS_CHANGED == STD_ON)
    Dem_CallbackOnDTCStatusChangedTable,
#endif
    
#if (DEM_CFG_CALLBACK_ON_CYCLE_STATUS_CHANGED == STD_ON)
    Dem_CallbackOnOperationCycleStatusChangedTable,
#endif
    
    Dem_InitMonitorForEventTable,
    Dem_DTCGroupConfig,
    DEM_NUM_DTC_GROUPS,
    &Dem_ClearDTCConfig,
    Dem_MemoryConfig,
    DEM_NUM_MEMORIES,
    
#if (DEM_CFG_AgingSupport == STD_ON)
    Dem_AgingConfig,
#endif
    
#if (DEM_CFG_OBDSupport == STD_ON)
    Dem_PIDConfig,
    DEM_NUM_PIDS,
#endif
};

/* External reference to post-build configuration */
const Dem_PBConfigRootType* const Dem_PBConfig = &Dem_PBConfigRoot;

