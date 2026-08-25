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

/*
 * FiM.c - Function Inhibition Manager Implementation
 * AUTOSAR_SWS_FunctionInhibitionManager
 *
 * This module manages function permissions based on diagnostic event status.
 * It maps Dem events to Function IDs (FIDs) and calculates permissions
 * based on configured inhibition masks.
 */

#include "FiM.h"
#include "Dem.h"
#include "Det.h"

/*============================================================================
 * Local Macros
 *===========================================================================*/
#define FIM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "FiM_MemMap.h"

/*============================================================================
 * Internal State Variables
 *===========================================================================*/

/* Module initialization state */
static FiM_StateType FiM_State = FIM_UNINIT;

/* Permission state for each FID
 * TRUE = Function allowed
 * FALSE = Function inhibited */
static boolean FiM_FidPermission[FIM_CFG_NUMBER_OF_FIDS];

/* Availability state for each FID
 * Set by FiM_SetFunctionAvailable() */
static boolean FiM_FidAvailable[FIM_CFG_NUMBER_OF_FIDS];

/* Event status cache for quick lookup */
static uint8 FiM_EventStatus[FIM_CFG_NUMBER_OF_EVENTS];

/* Dirty flags for events that need recalculation */
static boolean FiM_EventDirty[FIM_CFG_NUMBER_OF_EVENTS];

/* Global recalculation required flag */
static boolean FiM_GlobalRecalculationRequired = FALSE;

#define FIM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "FiM_MemMap.h"

/*============================================================================
 * External Configuration Data (from FiM_Lcfg.c)
 *===========================================================================*/
extern const FiM_InhibitionConfigType FiM_InhibitionConfigTable[];
extern const uint16 FiM_NumInhibitionConfigs;
extern const FiM_EventFidMappingType FiM_EventFidMapTable[];
extern const uint16 FiM_NumEventFidMappings;
extern const FiM_FidConfigType FiM_FidConfigTable[];
extern const FiM_EventConfigType FiM_EventConfigTable[];

/*============================================================================
 * Local Function Prototypes
 *===========================================================================*/
static void FiM_RecalculateFidPermission(FiM_FunctionIdType Fid);
static boolean FiM_CalculateInhibition(uint16 EventId, uint8 InhibitionMask);
static boolean FiM_CheckEventInhibitsFid(uint16 EventId, FiM_FunctionIdType Fid, uint8 InhibitionMask);
static void FiM_MarkAffectedFidsDirty(uint16 EventId);
static Std_ReturnType FiM_ValidateFid(FiM_FunctionIdType FID);
static Std_ReturnType FiM_ValidateEventId(uint16 EventId);
static void FiM_UpdateEventStatus(uint16 EventId);

/*============================================================================
 * Local Function Implementations
 *===========================================================================*/

/**
 * @brief Validate FID parameter
 * @param FID Function ID to validate
 * @return E_OK if valid, E_NOT_OK otherwise
 */
/** @req SWS_FiM_00001 */
static Std_ReturnType FiM_ValidateFid(FiM_FunctionIdType FID)
{
    if (FID >= FIM_CFG_NUMBER_OF_FIDS) {
#if (FIM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_GETFUNCTIONPERMISSION,
            FIM_E_FID_OUT_OF_RANGE
        );
#endif
        return E_NOT_OK;
    }
    return E_OK;
}

/**
 * @brief Validate Event ID parameter
 * @param EventId Event ID to validate
 * @return E_OK if valid, E_NOT_OK otherwise
 */
/** @req SWS_FiM_00002 */
static Std_ReturnType FiM_ValidateEventId(uint16 EventId)
{
    if (EventId >= FIM_CFG_NUMBER_OF_EVENTS) {
#if (FIM_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_DEMTRIGGERONEVENTSTATUS,
            FIM_E_EVENTID_OUT_OF_RANGE
        );
#endif
        return E_NOT_OK;
    }
    return E_OK;
}

/**
 * @brief Update cached event status from Dem
 * @param EventId Event ID to update
 */
/** @req SWS_FiM_00003 */
static void FiM_UpdateEventStatus(uint16 EventId)
{
    uint8 eventStatus = 0u;
    
    /* Get current event status from Dem */
    if (Dem_GetEventStatus(EventId, &eventStatus) == E_OK) {
        FiM_EventStatus[EventId] = eventStatus;
    }
}

/**
 * @brief Check if an event inhibits a FID based on mask
 * @param EventId The event ID
 * @param InhibitionMask The inhibition mask to apply
 * @return TRUE if function should be inhibited
 */
/** @req SWS_FiM_00004 */
static boolean FiM_CalculateInhibition(uint16 EventId, uint8 InhibitionMask)
{
    uint8 eventStatus = FiM_EventStatus[EventId];
    boolean inhibit = FALSE;
    
    /* Extract relevant status bits from Dem event status byte */
    boolean testFailed = (eventStatus & DEM_UDS_STATUS_TF) != 0u;
    boolean testFailedThisCycle = (eventStatus & DEM_UDS_STATUS_TFTOC) != 0u;
    boolean testNotCompleted = (eventStatus & DEM_UDS_STATUS_TNCTOC) != 0u;
    boolean testFailedSinceLastClear = (eventStatus & DEM_UDS_STATUS_TFSLC) != 0u;
    boolean testNotCompletedSinceLastClear = (eventStatus & DEM_UDS_STATUS_TNCSLC) != 0u;
    
    /* Apply inhibition mask logic */
    if (InhibitionMask & FIM_LAST_FAILED) {
        /* Inhibit if event has failed since last clear or this cycle */
        if (testFailed || testFailedSinceLastClear || testFailedThisCycle) {
            inhibit = TRUE;
        }
    }
    
    if (InhibitionMask & FIM_NOT_TESTED) {
        /* Inhibit if event not completed this cycle or since last clear */
        if (testNotCompleted || testNotCompletedSinceLastClear) {
            inhibit = TRUE;
        }
    }
    
    if (InhibitionMask & FIM_TESTED_FAULTY) {
        /* Inhibit if tested AND (failed now OR failed this cycle) */
        if (!testNotCompleted && (testFailed || testFailedThisCycle)) {
            inhibit = TRUE;
        }
    }
    
    return inhibit;
}

/**
 * @brief Check specific event-to-FID inhibition
 * @param EventId The event ID
 * @param Fid The function ID
 * @param InhibitionMask The inhibition mask
 * @return TRUE if the event inhibits this FID
 */
/** @req SWS_FiM_00005 */
static boolean FiM_CheckEventInhibitsFid(uint16 EventId, FiM_FunctionIdType Fid, uint8 InhibitionMask)
{
    (void)Fid; /* Parameter not needed in this implementation but kept for API consistency */
    return FiM_CalculateInhibition(EventId, InhibitionMask);
}

/**
 * @brief Mark all FIDs affected by an event as dirty
 * @param EventId The event that changed
 */
/** @req SWS_FiM_00006 */
static void FiM_MarkAffectedFidsDirty(uint16 EventId)
{
    uint16 i;
    
    /* Find all FID mappings for this event */
    for (i = 0u; i < FiM_NumEventFidMappings; i++) {
        if (FiM_EventFidMapTable[i].EventId == EventId) {
            /* Mark all FIDs associated with this event as needing recalculation */
            FiM_EventDirty[EventId] = TRUE;
            break;
        }
    }
    
    /* Set global flag if we have dirty events */
    if (i < FiM_NumEventFidMappings) {
        FiM_GlobalRecalculationRequired = TRUE;
    }
}

/**
 * @brief Recalculate permission for a single FID
 * @param Fid Function ID to recalculate
 */
/** @req SWS_FiM_00007 */
static void FiM_RecalculateFidPermission(FiM_FunctionIdType Fid)
{
    uint16 i;
    boolean permission = TRUE; /* Default: allow */
    
    /* Check if FID is available (can be inhibited) */
    if (FiM_FidAvailable[Fid] == FALSE) {
        /* Function is marked unavailable - deny permission */
        FiM_FidPermission[Fid] = FALSE;
        return;
    }
    
    /* Check all inhibition configurations for this FID */
    for (i = 0u; i < FiM_NumInhibitionConfigs; i++) {
        if (FiM_InhibitionConfigTable[i].Fid == Fid) {
            uint16 eventId = FiM_InhibitionConfigTable[i].EventId;
            uint8 mask = FiM_InhibitionConfigTable[i].InhibitionMask;
            
            /* Check if this event inhibits the function */
            if (FiM_CalculateInhibition(eventId, mask) == TRUE) {
                permission = FALSE;
                break; /* One inhibiting event is enough */
            }
        }
    }
    
    FiM_FidPermission[Fid] = permission;
}

/*============================================================================
 * API Function Implementations
 *===========================================================================*/

/**
 * @brief Initialize FiM module
 * @param configPtr Configuration pointer (not used in post-build variant)
 */
/** @req SWS_FiM_00008 */
void FiM_Init(const void* configPtr)
{
    uint16 i;
    
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_INIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_INIT,
            FIM_E_NOT_INITIALIZED
        );
        return;
    }
    
    if (configPtr != NULL_PTR) {
        /* Post-build configuration not supported in this implementation */
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_INIT,
            FIM_E_PARAM_CONFIG
        );
        return;
    }
#endif

    /* Initialize FID permissions to default */
    for (i = 0u; i < FIM_CFG_NUMBER_OF_FIDS; i++) {
        FiM_FidPermission[i] = TRUE;  /* Default: allow all */
        FiM_FidAvailable[i] = TRUE;   /* Default: available */
    }
    
    /* Clear event status cache */
    for (i = 0u; i < FIM_CFG_NUMBER_OF_EVENTS; i++) {
        FiM_EventStatus[i] = 0u;
        FiM_EventDirty[i] = FALSE;
    }
    
    /* Initial recalculation of all FIDs based on current Dem status */
    for (i = 0u; i < FIM_CFG_NUMBER_OF_EVENTS; i++) {
        FiM_UpdateEventStatus((uint16)i);
    }
    
    /* Calculate initial permissions */
    for (i = 0u; i < FIM_CFG_NUMBER_OF_FIDS; i++) {
        FiM_RecalculateFidPermission((FiM_FunctionIdType)i);
    }
    
    FiM_GlobalRecalculationRequired = FALSE;
    FiM_State = FIM_INIT;
}

/**
 * @brief Deinitialize FiM module
 */
/** @req SWS_FiM_00009 */
void FiM_DeInit(void)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_UNINIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_DEINIT,
            FIM_E_UNINIT
        );
        return;
    }
#endif

    FiM_State = FIM_UNINIT;
}

/**
 * @brief Get permission state for a function
 * @param FID Function ID
 * @param Permission Output: TRUE if function is allowed
 * @return E_OK if successful, E_NOT_OK if error
 */
/** @req SWS_FiM_00010 */
Std_ReturnType FiM_GetFunctionPermission(FiM_FunctionIdType FID, boolean* Permission)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_UNINIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_GETFUNCTIONPERMISSION,
            FIM_E_UNINIT
        );
        return E_NOT_OK;
    }
    
    if (Permission == NULL_PTR) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_GETFUNCTIONPERMISSION,
            FIM_E_PARAM_POINTER
        );
        return E_NOT_OK;
    }
#endif

    if (FiM_ValidateFid(FID) != E_OK) {
        return E_NOT_OK;
    }

    *Permission = FiM_FidPermission[FID];
    return E_OK;
}

/**
 * @brief Set availability state for a function
 * @param FID Function ID
 * @param Availability TRUE to make function available, FALSE to inhibit
 * @return E_OK if successful, E_NOT_OK if error
 */
/** @req SWS_FiM_00011 */
Std_ReturnType FiM_SetFunctionAvailable(FiM_FunctionIdType FID, boolean Availability)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_UNINIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_SETFUNCTIONAVAILABLE,
            FIM_E_UNINIT
        );
        return E_NOT_OK;
    }
#endif

    if (FiM_ValidateFid(FID) != E_OK) {
        return E_NOT_OK;
    }

#if (FIM_FUNCTION_INHIBITION_AVAILABLE == STD_ON)
    FiM_FidAvailable[FID] = Availability;
    /* Recalculate permission immediately */
    FiM_RecalculateFidPermission(FID);
    return E_OK;
#else
    (void)Availability;
    return E_NOT_OK;
#endif
}

/**
 * @brief Callback from Dem when monitor status changes
 * @param EventId The event whose monitor status changed
 */
/** @req SWS_FiM_00012 */
void FiM_DemTriggerOnMonitorStatus(uint16 EventId)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_UNINIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_DEMTRIGGERONMONITORSTATUS,
            FIM_E_UNINIT
        );
        return;
    }
    
    if (FiM_ValidateEventId(EventId) != E_OK) {
        return;
    }
#endif

    /* Update cached status */
    FiM_UpdateEventStatus(EventId);
    
    /* Mark affected FIDs for recalculation */
    FiM_MarkAffectedFidsDirty(EventId);
}

/**
 * @brief Callback from Dem when event status changes
 * @param EventId The event whose status changed
 */
/** @req SWS_FiM_00013 */
void FiM_DemTriggerOnEventStatus(uint16 EventId)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_UNINIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_DEMTRIGGERONEVENTSTATUS,
            FIM_E_UNINIT
        );
        return;
    }
    
    if (FiM_ValidateEventId(EventId) != E_OK) {
        return;
    }
#endif

    /* Update cached status */
    FiM_UpdateEventStatus(EventId);
    
    /* Mark affected FIDs for recalculation */
    FiM_MarkAffectedFidsDirty(EventId);
}

/**
 * @brief Detailed callback from Dem with old and new status
 * @param EventId The event
 * @param EventStatusByteOld Previous status
 * @param EventStatusByteNew Current status
 */
/** @req SWS_FiM_00014 */
void FiM_DemTriggerOnEventStatusUds(uint16 EventId, uint8 EventStatusByteOld, uint8 EventStatusByteNew)
{
    (void)EventStatusByteOld;
    
    /* Update cached status directly from callback */
    if (FiM_ValidateEventId(EventId) == E_OK) {
        FiM_EventStatus[EventId] = EventStatusByteNew;
        FiM_MarkAffectedFidsDirty(EventId);
    }
}

/**
 * @brief Main function - performs deferred recalculation
 * Called cyclically by the scheduler
 */
/** @req SWS_FiM_00015 */
void FiM_MainFunction(void)
{
    uint16 i;
    
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (FiM_State == FIM_UNINIT) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_MAINFUNCTION,
            FIM_E_UNINIT
        );
        return;
    }
#endif

    /* If global recalculation is required, update all FIDs */
    if (FiM_GlobalRecalculationRequired == TRUE) {
        /* Recalculate permissions for FIDs with dirty events */
        for (i = 0u; i < FIM_CFG_NUMBER_OF_FIDS; i++) {
            FiM_RecalculateFidPermission((FiM_FunctionIdType)i);
        }
        
        /* Clear dirty flags */
        for (i = 0u; i < FIM_CFG_NUMBER_OF_EVENTS; i++) {
            FiM_EventDirty[i] = FALSE;
        }
        
        FiM_GlobalRecalculationRequired = FALSE;
    }
    
#if (FIM_MAIN_FUNCTION_CALLOUT_SUPPORTED == STD_ON)
    /* User callout hook */
    FiM_MainFunctionCallout();
#endif
}

/**
 * @brief Get version information
 * @param versioninfo Pointer to version info structure
 */
#if (FIM_VERSION_INFO_API == STD_ON)
/** @req SWS_FiM_00016 */
void FiM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (FIM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(
            FIM_MODULE_ID,
            FIM_INSTANCE_ID,
            FIM_SID_GETVERSIONINFO,
            FIM_E_PARAM_POINTER
        );
        return;
    }
#endif

    versioninfo->vendorID = FIM_VENDOR_ID;
    versioninfo->moduleID = FIM_MODULE_ID;
    versioninfo->sw_major_version = FIM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = FIM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = FIM_SW_PATCH_VERSION;
}
#endif /* FIM_VERSION_INFO_API */
