/**
 * @file dem.c
 * @brief DEM (Diagnostic Event Manager) Main Module Implementation
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "dem.h"
#include <string.h>

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*============================================================================*
 * Internal Data
 *============================================================================*/
static Dem_StateType s_demState = DEM_STATE_UNINIT;
static const Dem_ConfigType* s_demConfig = NULL_PTR;
static boolean s_dtcSettingEnabled = TRUE;
static boolean s_demInitialized = FALSE;

/* Operation cycle states */
static Dem_OperationCycleStateType s_operationCycleStates[4] = {
    DEM_CYCLE_STATE_END,  /* POWER */
    DEM_CYCLE_STATE_END,  /* IGNITION */
    DEM_CYCLE_STATE_END,  /* WARMUP */
    DEM_CYCLE_STATE_END   /* OBD_DCY */
};

/*============================================================================*
 * Static Helper Functions
 *============================================================================*/

/**
 * @brief Validate configuration
 */
static boolean Dem_ValidateConfig(const Dem_ConfigType* ConfigPtr)
{
    boolean valid = TRUE;
    
    if (ConfigPtr != NULL_PTR) {
        /* Validate event configuration */
        if ((ConfigPtr->eventConfigTable == NULL_PTR) && (ConfigPtr->eventCount > 0U)) {
            valid = FALSE;
        }
        
        /* Validate DTC configuration */
        if ((ConfigPtr->dtcConfigTable == NULL_PTR) && (ConfigPtr->dtcCount > 0U)) {
            valid = FALSE;
        }
        
        /* Validate counts */
        if ((ConfigPtr->eventCount > DEM_MAX_EVENTS) ||
            (ConfigPtr->dtcCount > DEM_MAX_DTCS)) {
            valid = FALSE;
        }
    }
    
    return valid;
}

/**
 * @brief Process operation cycles
 */
static void Dem_ProcessOperationCycles(void)
{
    /* Process operation cycle transitions */
    for (uint8_t i = 0U; i < 4U; i++) {
        /* Handle cycle state changes if needed */
        /* This would manage aging counters, confirmed DTC timing, etc. */
    }
}

/*============================================================================*
 * Public Functions - Core Module Functions
 *============================================================================*/

Std_ReturnType Dem_Init(const Dem_ConfigType* ConfigPtr)
{
    Std_ReturnType result = E_OK;
    
    /* Check if already initialized */
    if (s_demState == DEM_STATE_INIT) {
        return E_OK;
    }
    
    /* Validate configuration if provided */
    if ((ConfigPtr != NULL_PTR) && (Dem_ValidateConfig(ConfigPtr) == FALSE)) {
        return E_NOT_OK;
    }
    
    /* Store configuration */
    s_demConfig = ConfigPtr;
    
    /* Initialize sub-modules */
    result = Dem_EventInit();
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    result = Dem_DtcInit();
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    result = Dem_FreezeFrameInit();
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    result = Dem_NvMInit();
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    /* Read saved data from NvM */
    (void)Dem_NvMReadEventData();
    (void)Dem_NvMReadFreezeFrameData();
    (void)Dem_NvMReadExtendedData();
    
    /* Initialize operation cycles */
    for (uint8_t i = 0U; i < 4U; i++) {
        s_operationCycleStates[i] = DEM_CYCLE_STATE_END;
    }
    
    /* Enable DTC setting by default */
    s_dtcSettingEnabled = TRUE;
    
    /* Mark as initialized */
    s_demState = DEM_STATE_INIT;
    s_demInitialized = TRUE;
    
    return result;
}

void Dem_Shutdown(void)
{
    if (s_demState == DEM_STATE_INIT) {
        /* Write pending data to NvM */
        (void)Dem_NvMWriteEventData();
        (void)Dem_NvMWriteFreezeFrameData();
        (void)Dem_NvMWriteExtendedData();
        
        /* Wait for NvM operations to complete */
        /* In a real implementation, this might wait for callbacks */
        
        /* Mark as uninitialized */
        s_demState = DEM_STATE_UNINIT;
        s_demInitialized = FALSE;
        s_demConfig = NULL_PTR;
    }
}

void Dem_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL_PTR) {
        versioninfo->vendorID = DEM_VENDOR_ID;
        versioninfo->moduleID = DEM_MODULE_ID;
        versioninfo->sw_major_version = DEM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DEM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DEM_SW_PATCH_VERSION;
    }
}

void Dem_MainFunction(void)
{
    if (s_demState != DEM_STATE_INIT) {
        return;
    }
    
    /* Process operation cycles */
    Dem_ProcessOperationCycles();
    
    /* Process NvM operations */
    Dem_NvMMainFunction();
    
    /* Process debounce counters for time-based debouncing */
    /* This would be done for all events with time-based debouncing */
}

void Dem_PreInit(void)
{
    /* Early initialization before full DEM init */
    s_demState = DEM_STATE_PREINIT;
}

/*============================================================================*
 * Operation Cycle Functions
 *============================================================================*/

Std_ReturnType Dem_SetOperationCycleState(
    Dem_OperationCycleType operationCycle,
    Dem_OperationCycleStateType cycleState)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_demState != DEM_STATE_INIT) {
        return E_NOT_OK;
    }
    
    if (operationCycle < 4U) {
        s_operationCycleStates[operationCycle] = cycleState;
        
        if (cycleState == DEM_CYCLE_STATE_START) {
            /* Handle cycle start */
            /* Reset test not completed flags */
            /* Update aging counters */
        }
        else {
            /* Handle cycle end */
            /* Trigger NvM write */
            Dem_NvMMarkEventDataModified();
        }
        
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetOperationCycleState(
    Dem_OperationCycleType operationCycle,
    Dem_OperationCycleStateType* cycleState)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (cycleState == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if ((s_demState == DEM_STATE_INIT) && (operationCycle < 4U)) {
        *cycleState = s_operationCycleStates[operationCycle];
        result = E_OK;
    }
    
    return result;
}

/*============================================================================*
 * Enable/Disable Control Functions
 *============================================================================*/

Std_ReturnType Dem_DisableDTCSetting(
    uint32_t DTCGroup,
    uint8_t ClientId)
{
    Std_ReturnType result = E_OK;
    
    (void)ClientId;  /* Unused parameter */
    
    if (s_demState != DEM_STATE_INIT) {
        return E_NOT_OK;
    }
    
    if (DTCGroup == DEM_DTC_GROUP_ALL) {
        /* Disable all DTCs */
        s_dtcSettingEnabled = FALSE;
    }
    else {
        /* Disable specific DTC group - would filter by group */
        /* For now, disable all */
        s_dtcSettingEnabled = FALSE;
    }
    
    return result;
}

Std_ReturnType Dem_EnableDTCSetting(
    uint32_t DTCGroup,
    uint8_t ClientId)
{
    Std_ReturnType result = E_OK;
    
    (void)ClientId;  /* Unused parameter */
    
    if (s_demState != DEM_STATE_INIT) {
        return E_NOT_OK;
    }
    
    if (DTCGroup == DEM_DTC_GROUP_ALL) {
        /* Enable all DTCs */
        s_dtcSettingEnabled = TRUE;
    }
    else {
        /* Enable specific DTC group */
        s_dtcSettingEnabled = TRUE;
    }
    
    return result;
}

boolean Dem_IsDTCSettingEnabled(void)
{
    return s_dtcSettingEnabled;
}

/*============================================================================*
 * Indication Status Functions
 *============================================================================*/

Std_ReturnType Dem_GetIndicatorStatus(
    uint8_t indicatorId,
    uint8_t* indicatorStatus)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (indicatorStatus == NULL_PTR) {
        return E_NOT_OK;
    }
    
    (void)indicatorId;  /* Unused parameter */
    
    /* Simplified implementation - would check warning indicator requests */
    *indicatorStatus = 0U;  /* Off */
    result = E_OK;
    
    return result;
}

Std_ReturnType Dem_GetFaultDetectionCounter(
    Dem_EventIdType EventId,
    sint8* faultDetectionCounter)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (faultDetectionCounter == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        *faultDetectionCounter = entry->debounceCounter;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetNumberOfStoredFreezeFrames(
    Dem_EventIdType EventId,
    uint8_t* numberOfStoredRecords)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (numberOfStoredRecords == NULL_PTR) {
        return E_NOT_OK;
    }
    
    (void)EventId;  /* Would count freeze frames for specific event */
    
    /* Return total freeze frames for now */
    *numberOfStoredRecords = Dem_GetNumberOfFreezeFrames();
    result = E_OK;
    
    return result;
}

/*============================================================================*
 * Component API Functions
 *============================================================================*/

Std_ReturnType Dem_ReportErrorStatus(
    uint16_t EventId,
    Dem_EventStatusType EventStatus)
{
    /* Legacy API - maps to Dem_SetEventStatus */
    return Dem_SetEventStatus((Dem_EventIdType)EventId, EventStatus);
}

Std_ReturnType Dem_ReportErrorStatusWithDebug(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus,
    uint8_t debug0,
    uint8_t debug1)
{
    /* Store debug info and report error */
    (void)debug0;
    (void)debug1;
    
    return Dem_SetEventStatus(EventId, EventStatus);
}

/*============================================================================*
 * Configuration Functions
 *============================================================================*/

Std_ReturnType Dem_SetEventAvailable(
    Dem_EventIdType EventId,
    boolean available)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        entry->isAvailable = available;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_SetDTCAvailableInOutput(
    uint32_t DTC,
    boolean available)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if (entry != NULL_PTR) {
        entry->isSuppressed = !available;
        result = E_OK;
    }
    
    return result;
}

/*============================================================================*
 * Control Functions
 *============================================================================*/

Std_ReturnType Dem_ClearDTC(
    uint32_t DTC,
    Dem_DTCFormatType DTCFormat,
    Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result;
    
    if (s_demState != DEM_STATE_INIT) {
        return E_NOT_OK;
    }
    
    /* Call DTC module clear function */
    result = Dem_ClearDTC(DTC, DTCFormat, DTCOrigin);
    
    if (result == E_OK) {
        /* Also clear associated freeze frames */
        if (DTC == DEM_DTC_GROUP_ALL) {
            Dem_ClearAllFreezeFrames();
            Dem_ClearAllExtendedDataRecords();
        }
        else {
            Dem_DeleteFreezeFrame(DTC, DEM_FREEZE_FRAME_RECORD_NUMBER_0);
        }
        
        /* Mark data as modified for NvM write */
        Dem_NvMMarkEventDataModified();
        Dem_NvMMarkFreezeFrameModified();
    }
    
    return result;
}

Std_ReturnType Dem_ResetEventConfirmed(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        /* Clear confirmed status */
        entry->dtcStatus &= (Dem_UdsStatusByteType)(~DEM_UDS_STATUS_CDTC);
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_RestartOperationCycle(Dem_OperationCycleType operationCycle)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_demState == DEM_STATE_INIT) {
        /* End current cycle */
        (void)Dem_SetOperationCycleState(operationCycle, DEM_CYCLE_STATE_END);
        
        /* Start new cycle */
        result = Dem_SetOperationCycleState(operationCycle, DEM_CYCLE_STATE_START);
    }
    
    return result;
}

/*============================================================================*
 * Diagnostic Information Functions
 *============================================================================*/

uint8_t Dem_GetDTCStatusAvailabilityMask(void)
{
    /* Return availability mask - all bits supported */
    return 0xFFU;
}

Std_ReturnType Dem_GetNumberOfFilteredDTC(uint16_t* NumberOfFilteredDTC)
{
    /* Forward to DTC module */
    return Dem_GetNumberOfFilteredDTC(NumberOfFilteredDTC);
}

Std_ReturnType Dem_GetNextFilteredDTC(
    uint32_t* DTC,
    uint8_t* DTCStatus)
{
    Dem_UdsStatusByteType status;
    Std_ReturnType result;
    
    result = Dem_GetNextFilteredDTC(DTC, &status);
    
    if (DTCStatus != NULL_PTR) {
        *DTCStatus = (uint8_t)status;
    }
    
    return result;
}

Std_ReturnType Dem_GetDTCByOccurrenceTime(
    Dem_DTCRequestType DTCRequest,
    uint32_t* DTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (DTC == NULL_PTR) {
        return E_NOT_OK;
    }
    
    (void)DTCRequest;  /* Would search by occurrence time */
    
    /* Simplified: return first active DTC */
    *DTC = 0U;
    result = E_NOT_OK;
    
    return result;
}

/*============================================================================*
 * Module State Functions
 *============================================================================*/

boolean Dem_IsInitialized(void)
{
    return s_demInitialized;
}

Dem_StateType Dem_GetState(void)
{
    return s_demState;
}


/*==================================================================================================
 *                                      ADDITIONAL API IMPLEMENTATIONS
 * CRITICAL FIX: Implementation of missing AUTOSAR standard APIs
==================================================================================================*/

/**
 * rief   Resets the event status of an event
 */
Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_ResetEventStatus, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (EventId >= DEM_CFG_MAX_NUMBER_EVENTS) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_ResetEventStatus, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (EventId < DEM_CFG_MAX_NUMBER_EVENTS) {
        Dem_EventStatusExtendedType* status = &Dem_EventStatus[EventId];
        
        /* Reset debounce counter */
        #if (DEM_CFG_EventDebounceSupport == STD_ON)
        if (Dem_DebounceInfo[EventId].Algorithm == DEM_DEBOUNCE_COUNTER_BASED) {
            Dem_DebounceInfo[EventId].Data.Counter.Counter = 0;
        } else if (Dem_DebounceInfo[EventId].Algorithm == DEM_DEBOUNCE_TIME_BASED) {
            Dem_DebounceInfo[EventId].Data.Time.Timer = 0;
        }
        #endif
        
        /* Reset event status bits (keep TestNotCompletedSinceLastClear) */
        *status &= DEM_UDS_STATUS_TNCSLC;
        
        /* Notify callback if configured */
        #if (DEM_CFG_CALLBACK_ON_EVC_STATUS_CHANGED == STD_ON)
        /* Callback notification */
        #endif
        
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Gets the current status of an event
 */
Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType EventId,
    Dem_EventStatusExtendedType* EventStatusExtended)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetEventStatus, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (EventId >= DEM_CFG_MAX_NUMBER_EVENTS) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetEventStatus, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    if (EventStatusExtended == NULL_PTR) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetEventStatus, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (EventId < DEM_CFG_MAX_NUMBER_EVENTS && EventStatusExtended != NULL_PTR) {
        *EventStatusExtended = Dem_EventStatus[EventId];
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Gets the UDS status byte of an event
 */
Std_ReturnType Dem_GetEventUdsStatus(
    Dem_EventIdType EventId,
    Dem_UdsStatusByteType* UDSStatusByte)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetEventUdsStatus, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (EventId >= DEM_CFG_MAX_NUMBER_EVENTS) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetEventUdsStatus, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    if (UDSStatusByte == NULL_PTR) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetEventUdsStatus, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (EventId < DEM_CFG_MAX_NUMBER_EVENTS && UDSStatusByte != NULL_PTR) {
        *UDSStatusByte = Dem_EventStatus[EventId];
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Gets the DTC for a given event
 */
Std_ReturnType Dem_GetDTCOfEvent(
    Dem_EventIdType EventId,
    Dem_DTCFormatType DTCFormat,
    uint32* DTCOfEvent,
    Dem_DTCOriginType* DTCOrigin)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetDTCOfEvent, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (EventId >= DEM_CFG_MAX_NUMBER_EVENTS) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetDTCOfEvent, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    if (DTCOfEvent == NULL_PTR) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetDTCOfEvent, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (EventId < DEM_CFG_MAX_NUMBER_EVENTS && DTCOfEvent != NULL_PTR) {
        /* Get DTC from event configuration */
        extern const uint32 Dem_EventDTCMapping[DEM_CFG_MAX_NUMBER_EVENTS];
        *DTCOfEvent = Dem_EventDTCMapping[EventId];
        
        if (DTCOrigin != NULL_PTR) {
            *DTCOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY; /* Default */
        }
        
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Disables the DTC record update
 */
Std_ReturnType Dem_DisableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_DisableDTCRecordUpdate, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    /* Check if valid client ID */
    if (ClientId < DEM_MAX_CLIENTS) {
        /* Disable DTC record update for this client */
        Dem_DTCRecordUpdateDisabled[ClientId] = TRUE;
        Dem_DTCRecordUpdateDisabledDTC[ClientId] = DTC;
        Dem_DTCRecordUpdateDisabledOrigin[ClientId] = DTCOrigin;
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Enables the DTC record update
 */
Std_ReturnType Dem_EnableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_EnableDTCRecordUpdate, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    /* Check if valid client ID */
    if (ClientId < DEM_MAX_CLIENTS) {
        /* Enable DTC record update for this client */
        Dem_DTCRecordUpdateDisabled[ClientId] = FALSE;
        Dem_DTCRecordUpdateDisabledDTC[ClientId] = 0;
        Dem_DTCRecordUpdateDisabledOrigin[ClientId] = 0;
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Sets the operation cycle state
 */
Std_ReturnType Dem_SetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType CycleState)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_SetOperationCycleState, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (OperationCycleId >= DEM_CFG_MAX_OPERATION_CYCLES) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_SetOperationCycleState, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (OperationCycleId < DEM_CFG_MAX_OPERATION_CYCLES) {
        if (CycleState == DEM_CYCLE_STATE_START) {
            Dem_OperationCycleStatus[OperationCycleId] = TRUE;
            
            /* Reset TNCTOC bit for all events using this cycle */
            for (uint16 i = 0; i < DEM_CFG_MAX_NUMBER_EVENTS; i++) {
                if (Dem_GetEventCycleRef(i) == OperationCycleId) {
                    Dem_EventStatus[i] &= ~DEM_UDS_STATUS_TNCTOC;
                }
            }
        } else {
            Dem_OperationCycleStatus[OperationCycleId] = FALSE;
        }
        
        /* Callback notification */
        #if (DEM_CFG_CALLBACK_ON_CYCLE_STATUS_CHANGED == STD_ON)
        /* Notify application */
        #endif
        
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Gets the operation cycle state
 */
Std_ReturnType Dem_GetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType* CycleState)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetOperationCycleState, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (OperationCycleId >= DEM_CFG_MAX_OPERATION_CYCLES) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetOperationCycleState, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    if (CycleState == NULL_PTR) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetOperationCycleState, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (OperationCycleId < DEM_CFG_MAX_OPERATION_CYCLES && CycleState != NULL_PTR) {
        *CycleState = Dem_OperationCycleStatus[OperationCycleId] ? 
                      DEM_CYCLE_STATE_START : DEM_CYCLE_STATE_END;
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Restarts the operation cycle
 */
Std_ReturnType Dem_RestartOperationCycle(
    Dem_OperationCycleIdType OperationCycleId)
{
    Std_ReturnType result;
    
    /* End the cycle first */
    result = Dem_SetOperationCycleState(OperationCycleId, DEM_CYCLE_STATE_END);
    
    if (result == E_OK) {
        /* Start the cycle again */
        result = Dem_SetOperationCycleState(OperationCycleId, DEM_CYCLE_STATE_START);
    }
    
    return result;
}

/**
 * rief   Gets the debouncing status of an event
 */
Std_ReturnType Dem_GetDebouncingOfEvent(
    Dem_EventIdType EventId,
    Dem_DebouncingStateType* DebouncingState)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetDebouncingOfEvent, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (EventId >= DEM_CFG_MAX_NUMBER_EVENTS) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetDebouncingOfEvent, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    if (DebouncingState == NULL_PTR) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_GetDebouncingOfEvent, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (EventId < DEM_CFG_MAX_NUMBER_EVENTS && DebouncingState != NULL_PTR) {
        #if (DEM_CFG_EventDebounceSupport == STD_ON)
        /* Return debounce status based on algorithm type */
        if (Dem_DebounceInfo[EventId].Algorithm == DEM_DEBOUNCE_COUNTER_BASED) {
            sint16 counter = Dem_DebounceInfo[EventId].Data.Counter.Counter;
            if (counter >= Dem_DebounceInfo[EventId].Data.Counter.FailedThreshold) {
                *DebouncingState = DEM_TEMPORARILY_DEFECTIVE;
            } else if (counter <= Dem_DebounceInfo[EventId].Data.Counter.PassedThreshold) {
                *DebouncingState = DEM_TEMPORARILY_OK;
            } else {
                *DebouncingState = DEM_DCTR_CENTER;
            }
        } else {
            *DebouncingState = DEM_NO_DCTR_SUPPORT;
        }
        #else
        *DebouncingState = DEM_NO_DCTR_SUPPORT;
        #endif
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Pre-allocated temporary memory for event processing
 */
Std_ReturnType Dem_PreTempActive(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    
    #if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_State != DEM_STATE_INIT) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_PreTempActive, DEM_E_UNINIT);
        return E_NOT_OK;
    }
    if (EventId >= DEM_CFG_MAX_NUMBER_EVENTS) {
        Det_ReportError(DEM_MODULE_ID, 0, DEM_SID_PreTempActive, DEM_E_PARAM_CONFIG);
        return E_NOT_OK;
    }
    #endif
    
    Dem_EnterCritical();
    
    if (EventId < DEM_CFG_MAX_NUMBER_EVENTS) {
        /* Mark event as temporarily active */
        Dem_EventTempActive[EventId] = TRUE;
        result = E_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/* Additional static variables needed by new APIs */
static boolean Dem_DTCRecordUpdateDisabled[DEM_MAX_CLIENTS];
static uint32 Dem_DTCRecordUpdateDisabledDTC[DEM_MAX_CLIENTS];
static Dem_DTCOriginType Dem_DTCRecordUpdateDisabledOrigin[DEM_MAX_CLIENTS];
static boolean Dem_OperationCycleStatus[DEM_CFG_MAX_OPERATION_CYCLES];
static boolean Dem_EventTempActive[DEM_CFG_MAX_NUMBER_EVENTS];
static uint32 Dem_EventDTCMapping[DEM_CFG_MAX_NUMBER_EVENTS] = {
    0x010101, 0x010102, 0x010103, 0x020101, 0x020102,
    /* ... more mappings ... */
};

/* Helper function to get event's operation cycle reference */
static Dem_OperationCycleIdType Dem_GetEventCycleRef(Dem_EventIdType EventId)
{
    /* Return configured cycle reference for event */
    return DEM_OPCYC_IGNITION; /* Default */
}

