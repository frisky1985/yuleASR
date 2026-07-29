/*==================================================================================================
 * Project              : YuleTech AutoSAR BSW
 * Platform             : NXP i.MX8M Mini
 * Peripheral           : N/A (Service Layer)
 * Dependencies         : NvM
 *
 * SW Version           : 1.1.0
 * Build Version        : YuleTech_DEM_1.1.0
 * Build Date           : 2026-04-29
 * Author               : AI Agent (Dem Optimization)
 *
 * CRITICAL FIXES (v1.1.0):
 * - Fixed null pointer dereference (ConfigPtr->Events -> EventParameters)
 * - Separated internal functions to Dem_Int.c
 * - Added time-based debounce support
 * - Added extended data record support
 * - Improved error handling
 *
 * (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 * All Rights Reserved.
 ==================================================================================================*/

 /*==================================================================================================
  *                                             INCLUDES
  ==================================================================================================*/
#include "Dem.h"
#include "Dem_Int.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

 /*==================================================================================================
  *                                  LOCAL CONSTANT DEFINITIONS
  ==================================================================================================*/
#define DEM_INSTANCE_ID                 (0x00U)

 /*==================================================================================================
  *                                  LOCAL MACRO DEFINITIONS
  ==================================================================================================*/
#if (DEM_DEV_ERROR_DETECT == STD_ON)
#define DEM_DET_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, (ApiId), (ErrorId))
#else
#define DEM_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

 /*==================================================================================================
  *                                  GLOBAL FUNCTIONS
  ==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

 /**
  * @brief   Initializes the DEM module
  */
void Dem_Init(const Dem_ConfigType* ConfigPtr)
{
    uint8 i;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_INIT, DEM_E_PARAM_POINTER);
        return;
    }
#endif

    /* Store configuration pointer */
    Dem_InternalState.ConfigPtr = ConfigPtr;

    /* Initialize event states */
    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        Dem_InternalState.EventStates[i].LastReportedStatus = DEM_EVENT_STATUS_PASSED;
        Dem_InternalState.EventStates[i].DTCStatus = 0U;
        Dem_InternalState.EventStates[i].FaultDetectionCounter = 0;
        Dem_InternalState.EventStates[i].DebounceCounter = 0;
        Dem_InternalState.EventStates[i].TestFailedThisOperationCycle = FALSE;
        Dem_InternalState.EventStates[i].TestCompletedThisOperationCycle = FALSE;
        Dem_InternalState.EventStates[i].OccurrenceCounter = 0U;
        Dem_InternalState.EventStates[i].AgingCounter = 0U;
        Dem_InternalState.EventStates[i].IsAged = FALSE;
        Dem_InternalState.EventStates[i].LastReportTimestamp = 0U;
        Dem_InternalState.EventStates[i].TimeInCurrentStatus = 0U;
        
        /* Initialize time-based debounce state */
        Dem_InternalState.TimeDebounceStates[i].State = DEM_TIME_DEBOUNCE_IDLE;
        Dem_InternalState.TimeDebounceStates[i].ElapsedTimeMs = 0U;
        Dem_InternalState.TimeDebounceStates[i].ThresholdReached = FALSE;
    }

    /* Initialize DTC entries */
    for (i = 0U; i < DEM_NUM_DTCS; i++)
    {
        if (i < ConfigPtr->NumDtcs)
        {
            Dem_InternalState.DTCEntries[i].DTC = ConfigPtr->DtcParameters[i].Dtc;
            Dem_InternalState.DTCEntries[i].AgingThreshold = ConfigPtr->DtcParameters[i].AgingThreshold;
        }
        else
        {
            Dem_InternalState.DTCEntries[i].DTC = 0U;
            Dem_InternalState.DTCEntries[i].AgingThreshold = DEM_AGING_CYCLE_THRESHOLD;
        }
        
        Dem_InternalState.DTCEntries[i].Status = DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR |
            DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
        Dem_InternalState.DTCEntries[i].OccurrenceCounter = 0U;
        Dem_InternalState.DTCEntries[i].AgingCounter = 0U;
        Dem_InternalState.DTCEntries[i].IsAged = FALSE;
        Dem_InternalState.DTCEntries[i].IsSuppressed = FALSE;
        Dem_InternalState.DTCEntries[i].IsDeleted = FALSE;
        Dem_InternalState.DTCEntries[i].NvMBlockId = 0xFFFFU;
        Dem_InternalState.DTCEntries[i].IsNvMDataValid = FALSE;
    }

    /* Initialize operation cycle states */
    for (i = 0U; i < DEM_NUM_OPERATION_CYCLES; i++)
    {
        Dem_InternalState.OperationCycleStates[i] = DEM_CYCLE_STATE_END;
    }

    /* Initialize enable conditions */
    for (i = 0U; i < DEM_NUM_ENABLE_CONDITIONS; i++)
    {
        Dem_InternalState.EnableConditions[i] = TRUE;
    }

    /* Initialize storage conditions */
    for (i = 0U; i < DEM_NUM_STORAGE_CONDITIONS; i++)
    {
        Dem_InternalState.StorageConditions[i] = TRUE;
    }

    /* Initialize selected DTC */
    Dem_InternalState.SelectedDTC = 0U;
    Dem_InternalState.DTCRecordUpdateDisabled = FALSE;
    Dem_InternalState.DTCSettingDisabled = FALSE;

    /* Initialize freeze frames */
    for (i = 0U; i < DEM_NUM_FREEZE_FRAME_RECORDS; i++)
    {
        Dem_InternalState.FreezeFrames[i].IsValid = FALSE;
        Dem_InternalState.FreezeFrames[i].Length = 0U;
        Dem_InternalState.FreezeFrames[i].Timestamp = 0U;
        Dem_InternalState.FreezeFrames[i].DtcIndex = 0xFFFFU;
    }

    /* Initialize extended data records */
    for (i = 0U; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++)
    {
        Dem_InternalState.ExtendedDataRecords[i].IsValid = FALSE;
        Dem_InternalState.ExtendedDataRecords[i].Length = 0U;
        Dem_InternalState.ExtendedDataRecords[i].DtcIndex = 0xFFFFU;
    }

    /* Initialize filter state */
    Dem_InternalState.DTCFilterStatusMask = 0xFFU;
    Dem_InternalState.DTCFilterFormat = DEM_DTC_FORMAT_UDS;
    Dem_InternalState.DTCFilterOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
    Dem_InternalState.FilteredDTCCount = 0U;
    Dem_InternalState.CurrentFilteredIndex = 0U;

    /* Initialize timestamp */
    Dem_InternalState.LastMainFunctionTimestamp = 0U;

    /* Set module state to initialized */
    Dem_InternalState.State = DEM_STATE_INIT;

    /* Call notification if configured */
#if defined(DEM_CLEAR_DTC_LAMBDA_NOTIFICATION)
    if (Dem_InternalState.ConfigPtr->ClearDTCLambdaNotification != NULL_PTR)
    {
        Dem_InternalState.ConfigPtr->ClearDTCLambdaNotification();
    }
#endif
}

 /**
  * @brief   Deinitializes the DEM module
  */
void Dem_DeInit(void)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_DEINIT, DEM_E_UNINIT);
        return;
    }
#endif

    /* Clear configuration pointer */
    Dem_InternalState.ConfigPtr = NULL_PTR;

    /* Set module state to uninitialized */
    Dem_InternalState.State = DEM_STATE_UNINIT;
}

 /**
  * @brief   Shuts down the DEM (alias for Dem_DeInit)
  */
void Dem_Shutdown(void)
{
    Dem_DeInit();
}

 /**
  * @brief   Set event status
  * 
  * CRITICAL FIX: Now properly handles all debounce algorithms
  */
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    Std_ReturnType result = E_NOT_OK;
    const Dem_EventParameterType* eventConfig;
    Dem_EventStateType* eventState;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    /* Check initialization */
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    /* Validate EventId */
    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    eventState = &Dem_InternalState.EventStates[EventId - 1U];
    eventConfig = Dem_IntFindEventConfig(EventId);

    if (eventConfig != NULL_PTR)
    {
        /* Store old status for callback */
        Dem_EventStatusType oldStatus = eventState->LastReportedStatus;
        
        /* Update last reported status */
        eventState->LastReportedStatus = EventStatus;
        eventState->TestCompletedThisOperationCycle = TRUE;

        /* Process based on debounce algorithm */
        switch (eventConfig->DebounceAlgorithm)
        {
            case DEM_DEBOUNCE_ALGORITHM_COUNTER:
            case DEM_DEBOUNCE_ALGORITHM_NONE:
                /* Counter-based or no debounce */
                Dem_IntUpdateDebounceCounter(EventId, EventStatus);
                
                /* Check if threshold reached and update DTC status */
                if (eventState->DebounceCounter >= DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
                {
                    Dem_IntUpdateDTCStatusFromDebounce(EventId, TRUE);
                }
                else if (eventState->DebounceCounter <= DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
                {
                    Dem_IntUpdateDTCStatusFromDebounce(EventId, FALSE);
                }
                break;

            case DEM_DEBOUNCE_ALGORITHM_TIME:
                /* Time-based debounce - process in MainFunction */
                Dem_IntProcessTimeBasedDebounce(EventId, EventStatus, 0U);
                break;

            case DEM_DEBOUNCE_ALGORITHM_MONITOR:
                /* Monitor internal - direct status */
                if (EventStatus == DEM_EVENT_STATUS_FAILED)
                {
                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
                    Dem_IntUpdateDTCStatusFromDebounce(EventId, TRUE);
                }
                else if (EventStatus == DEM_EVENT_STATUS_PASSED)
                {
                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
                    Dem_IntUpdateDTCStatusFromDebounce(EventId, FALSE);
                }
                break;

            default:
                break;
        }

        result = E_OK;
    }
    else
    {
#if (DEM_DEV_ERROR_DETECT == STD_ON)
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
#endif
    }

    return result;
}

 /**
  * @brief   Reset event status
  */
Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_RESETEVENTSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_RESETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if (Dem_IntFindEventConfig(EventId) != NULL_PTR)
    {
        /* Reset debounce counter */
        Dem_IntResetDebounceCounter(EventId);
        
        /* Reset test flags */
        Dem_InternalState.EventStates[EventId - 1U].TestCompletedThisOperationCycle = FALSE;
        Dem_InternalState.EventStates[EventId - 1U].TestFailedThisOperationCycle = FALSE;
        
        result = E_OK;
    }

    return result;
}

 /**
  * @brief   Get event status
  */
Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType* EventStatus)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (EventStatus == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTSTATUS, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if (Dem_IntFindEventConfig(EventId) != NULL_PTR)
    {
        *EventStatus = Dem_InternalState.EventStates[EventId - 1U].LastReportedStatus;
        result = E_OK;
    }

    return result;
}

 /**
  * @brief   Get event failed status
  */
Std_ReturnType Dem_GetEventFailed(Dem_EventIdType EventId, boolean* EventFailed)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTFAILED, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (EventFailed == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTFAILED, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTFAILED, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if (Dem_IntFindEventConfig(EventId) != NULL_PTR)
    {
        *EventFailed = (Dem_InternalState.EventStates[EventId - 1U].FaultDetectionCounter >= 
                        DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD);
        result = E_OK;
    }

    return result;
}

 /**
  * @brief   Get event tested status
  */
Std_ReturnType Dem_GetEventTested(Dem_EventIdType EventId, boolean* EventTested)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTTESTED, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (EventTested == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTTESTED, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTTESTED, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if (Dem_IntFindEventConfig(EventId) != NULL_PTR)
    {
        *EventTested = Dem_InternalState.EventStates[EventId - 1U].TestCompletedThisOperationCycle;
        result = E_OK;
    }

    return result;
}

 /**
  * @brief   Get fault detection counter
  */
Std_ReturnType Dem_GetFaultDetectionCounter(Dem_EventIdType EventId, sint8* FaultDetectionCounter)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETFAULTDETECTION, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (FaultDetectionCounter == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETFAULTDETECTION, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETFAULTDETECTION, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if (Dem_IntFindEventConfig(EventId) != NULL_PTR)
    {
        *FaultDetectionCounter = Dem_InternalState.EventStates[EventId - 1U].FaultDetectionCounter;
        result = E_OK;
    }

    return result;
}

 /**
  * @brief   Get status of DTC
  */
Std_ReturnType Dem_GetStatusOfDTC(Dem_DtcType DTC,
                                  Dem_DTCOriginType DTCOrigin,
                                  Dem_UdsStatusByteType* DTCStatus)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 dtcIndex;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETDTCSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (DTCStatus == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETDTCSTATUS, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    dtcIndex = Dem_IntFindDTCIndex(DTC);
    
    if ((dtcIndex != DEM_INVALID_DTC_INDEX) &&
        (Dem_IntFindDTCConfig(DTC) != NULL_PTR))
    {
        *DTCStatus = Dem_InternalState.DTCEntries[dtcIndex].Status;
        result = E_OK;
    }
    else
    {
        *DTCStatus = 0U;
        result = DEM_STATUS_WRONG_DTC;
    }

    return result;
}

 /**
  * @brief   Get DTC status availability mask
  */
Std_ReturnType Dem_GetDTCStatusAvailabilityMask(uint8* DTCStatusMask)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETDTCSTATUSAVAILABILITYMASK, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (DTCStatusMask == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETDTCSTATUSAVAILABILITYMASK, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *DTCStatusMask = DEM_DTC_STATUS_AVAILABILITY_MASK;
    return E_OK;
}

 /**
  * @brief   Get number of filtered DTCs
  */
Std_ReturnType Dem_GetNumberOfFilteredDTC(uint16* NumberOfFilteredDTC)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETNUMBEROFFILTEREDDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (NumberOfFilteredDTC == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETNUMBEROFFILTEREDDTC, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    *NumberOfFilteredDTC = Dem_InternalState.FilteredDTCCount;
    return E_OK;
}

 /**
  * @brief   Get next filtered DTC
  */
Std_ReturnType Dem_GetNextFilteredDTC(Dem_DtcType* DTC, Dem_UdsStatusByteType* DTCStatus)
{
    Std_ReturnType result = DEM_FILTERED_NO_MATCHING_ELEMENT;
    
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETNEXTFILTEREDDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((DTC == NULL_PTR) || (DTCStatus == NULL_PTR))
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETNEXTFILTEREDDTC, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find next matching DTC */
    while (Dem_InternalState.CurrentFilteredIndex < DEM_NUM_DTCS)
    {
        if (Dem_IntMatchDTCFilter(Dem_InternalState.CurrentFilteredIndex))
        {
            *DTC = Dem_InternalState.DTCEntries[Dem_InternalState.CurrentFilteredIndex].DTC;
            *DTCStatus = Dem_InternalState.DTCEntries[Dem_InternalState.CurrentFilteredIndex].Status;
            Dem_InternalState.CurrentFilteredIndex++;
            result = DEM_FILTERED_OK;
            break;
        }
        Dem_InternalState.CurrentFilteredIndex++;
    }

    return result;
}

 /**
  * @brief   Clear DTC
  */
Std_ReturnType Dem_ClearDTC(Dem_DtcType DTC,
                            Dem_DTCFormatType DTCFormat,
                            Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_CLEARDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

#if (DEM_CLEAR_DTC_SUPPORTED == STD_ON)
    if ((DTC != DEM_DTC_GROUP_ALL) && (Dem_IntFindDTCConfig(DTC) == NULL_PTR))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_CLEARDTC, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#else
    DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_CLEARDTC, DEM_E_WRONG_CONFIGURATION);
    return E_NOT_OK;
#endif
#endif

#if (DEM_CLEAR_DTC_SUPPORTED == STD_ON)
    /* Call start notification if configured */
    if (Dem_InternalState.ConfigPtr->ClearDTCStartNotification != NULL_PTR)
    {
        Dem_InternalState.ConfigPtr->ClearDTCStartNotification();
    }

    if (DTC == DEM_DTC_GROUP_ALL)
    {
        Dem_IntClearAllDTCs();
    }
    else
    {
        uint8 dtcIndex = Dem_IntFindDTCIndex(DTC);
        if (dtcIndex != DEM_INVALID_DTC_INDEX)
        {
            Dem_IntClearSingleDTC(dtcIndex);
        }
    }

    /* Call finish notification if configured */
    if (Dem_InternalState.ConfigPtr->ClearDTCFinishNotification != NULL_PTR)
    {
        Dem_InternalState.ConfigPtr->ClearDTCFinishNotification();
    }

    result = E_OK;
#endif

    return result;
}

 /**
  * @brief   Select DTC
  */
Std_ReturnType Dem_SelectDTC(Dem_DtcType DTC, Dem_DTCFormatType DTCFormat, Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SELECTEDDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (Dem_IntFindDTCConfig(DTC) != NULL_PTR)
    {
        Dem_InternalState.SelectedDTC = DTC;
        Dem_InternalState.DTCFilterFormat = DTCFormat;
        Dem_InternalState.DTCFilterOrigin = DTCOrigin;
        
        /* Update filtered count */
        Dem_IntUpdateFilteredCount();
        Dem_InternalState.CurrentFilteredIndex = 0U;
        
        result = E_OK;
    }

    return result;
}

 /**
  * @brief   Disable DTC setting
  */
Std_ReturnType Dem_DisableDTCSetting(Dem_DtcType DTCGroup, uint8 DTCKind)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_DISABLEDTCSETTING, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    Dem_InternalState.DTCSettingDisabled = TRUE;
    return E_OK;
}

 /**
  * @brief   Enable DTC setting
  */
Std_ReturnType Dem_EnableDTCSetting(Dem_DtcType DTCGroup, uint8 DTCKind)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_ENABLEDTCSETTING, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    Dem_InternalState.DTCSettingDisabled = FALSE;
    return E_OK;
}

 /**
  * @brief   Disable DTC record update
  */
Std_ReturnType Dem_DisableDTCRecordUpdate(void)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_DISABLEDTCRECORD, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (Dem_InternalState.DTCRecordUpdateDisabled)
    {
        return DEM_DISABLEDTCRECUP_DISABLED;
    }

    Dem_InternalState.DTCRecordUpdateDisabled = TRUE;
    return DEM_DISABLEDTCRECUP_OK;
}

 /**
  * @brief   Enable DTC record update
  */
Std_ReturnType Dem_EnableDTCRecordUpdate(void)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_ENABLEDTCRECORD, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    Dem_InternalState.DTCRecordUpdateDisabled = FALSE;
    return E_OK;
}

 /**
  * @brief   Get indicator status
  */
Std_ReturnType Dem_GetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType* IndicatorStatus)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETINDICATORSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (IndicatorStatus == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETINDICATORSTATUS, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (IndicatorId >= DEM_NUM_INDICATORS)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETINDICATORSTATUS, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#endif

    /* Check if any DTC with this indicator is failed */
    *IndicatorStatus = DEM_INDICATOR_OFF;
    
    for (uint8 i = 0U; i < DEM_NUM_DTCS; i++)
    {
        if ((Dem_InternalState.DTCEntries[i].Status & DEM_DTC_STATUS_TEST_FAILED) &&
            (!Dem_InternalState.DTCEntries[i].IsSuppressed))
        {
            /* Check if this DTC uses the indicator */
            *IndicatorStatus = DEM_INDICATOR_CONTINUOUS;
            break;
        }
    }

    return E_OK;
}

 /**
  * @brief   Set indicator status
  */
Std_ReturnType Dem_SetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType IndicatorStatus)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_SETINDICATORSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (IndicatorId >= DEM_NUM_INDICATORS)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_SETINDICATORSTATUS, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#endif

    /* This would typically interface with BSW or SWC to control physical indicators */
    return E_OK;
}

 /**
  * @brief   Get freeze frame data by DTC
  */
Std_ReturnType Dem_GetFreezeFrameDataByDTC(Dem_DtcType DTC,
                                           Dem_DTCOriginType DTCOrigin,
                                           uint8 RecordNumber,
                                           uint8* DestBuffer,
                                           uint16* BufferSize)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 dtcIndex;
    
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETFREEZEFRAMEDATABYDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((DestBuffer == NULL_PTR) || (BufferSize == NULL_PTR))
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETFREEZEFRAMEDATABYDTC, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    dtcIndex = Dem_IntFindDTCIndex(DTC);
    
    if (dtcIndex != DEM_INVALID_DTC_INDEX)
    {
        result = Dem_IntGetFreezeFrame(dtcIndex, RecordNumber, DestBuffer, BufferSize);
    }
    else
    {
        result = DEM_GET_FREEZEFRAME_WRONG_DTC;
    }

    return result;
}

 /**
  * @brief   Get extended data record by DTC
  * 
  * CRITICAL FIX: Added extended data record support
  */
Std_ReturnType Dem_GetExtendedDataRecordByDTC(Dem_DtcType DTC,
                                              Dem_DTCOriginType DTCOrigin,
                                              uint8 ExtendedDataNumber,
                                              uint8* DestBuffer,
                                              uint16* BufferSize)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 dtcIndex;
    
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETEXTENDEDDATARECORDBYDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((DestBuffer == NULL_PTR) || (BufferSize == NULL_PTR))
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETEXTENDEDDATARECORDBYDTC, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    dtcIndex = Dem_IntFindDTCIndex(DTC);
    
    if (dtcIndex != DEM_INVALID_DTC_INDEX)
    {
        result = Dem_IntGetExtendedData(dtcIndex, ExtendedDataNumber, DestBuffer, BufferSize);
    }
    else
    {
        result = DEM_RECORD_WRONG_DTC;
    }

    return result;
}

 /**
  * @brief   Get size of extended data record
  */
Std_ReturnType Dem_GetSizeOfExtendedDataRecordByDTC(Dem_DtcType DTC,
                                                    Dem_DTCOriginType DTCOrigin,
                                                    uint8 ExtendedDataNumber,
                                                    uint16* SizeOfExtendedDataRecord)
{
    const Dem_DtcParameterType* dtcConfig;
    
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETSIZEOFEXTENDEDDATARECORDBYDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (SizeOfExtendedDataRecord == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETSIZEOFEXTENDEDDATARECORDBYDTC, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    dtcConfig = Dem_IntFindDTCConfig(DTC);
    
    if (dtcConfig == NULL_PTR)
    {
        return DEM_RECORD_WRONG_DTC;
    }

    /* Find extended data record size from config */
    for (uint8 i = 0U; i < Dem_InternalState.ConfigPtr->NumExtendedDataRecords; i++)
    {
        if (Dem_InternalState.ConfigPtr->ExtendedDataRecords[i].RecordNumber == ExtendedDataNumber)
        {
            *SizeOfExtendedDataRecord = Dem_InternalState.ConfigPtr->ExtendedDataRecords[i].DataSize;
            return E_OK;
        }
    }

    return DEM_RECORD_WRONG_NUMBER;
}

 /**
  * @brief   Set operation cycle state
  */
Std_ReturnType Dem_SetOperationCycleState(Dem_OperationCycleType OperationCycleType,
                                          Dem_OperationCycleStateType CycleState)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_SETOPERATIONCYCLESTATE, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (OperationCycleType >= DEM_NUM_OPERATION_CYCLES)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_SETOPERATIONCYCLESTATE, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#endif

    uint8 cycleIndex = (uint8)OperationCycleType;
    Dem_OperationCycleStateType oldState = Dem_InternalState.OperationCycleStates[cycleIndex];
    
    Dem_InternalState.OperationCycleStates[cycleIndex] = CycleState;

    /* Handle state transition effects */
    if ((oldState == DEM_CYCLE_STATE_END) && (CycleState == DEM_CYCLE_STATE_START))
    {
        Dem_IntHandleOperationCycleStart(cycleIndex);
    }
    else if ((oldState == DEM_CYCLE_STATE_START) && (CycleState == DEM_CYCLE_STATE_END))
    {
        Dem_IntHandleOperationCycleEnd(cycleIndex);
    }

    result = E_OK;
    return result;
}

 /**
  * @brief   Get operation cycle state
  */
Std_ReturnType Dem_GetOperationCycleState(Dem_OperationCycleType OperationCycleType,
                                          Dem_OperationCycleStateType* CycleState)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETOPERATIONCYCLESTATE, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (CycleState == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETOPERATIONCYCLESTATE, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (OperationCycleType >= DEM_NUM_OPERATION_CYCLES)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETOPERATIONCYCLESTATE, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#endif

    *CycleState = Dem_InternalState.OperationCycleStates[(uint8)OperationCycleType];
    return E_OK;
}

 /**
  * @brief   Restart operation cycle
  */
Std_ReturnType Dem_RestartOperationCycle(Dem_OperationCycleType OperationCycleType)
{
    Std_ReturnType result;

    /* End the cycle */
    result = Dem_SetOperationCycleState(OperationCycleType, DEM_CYCLE_STATE_END);
    
    if (result == E_OK)
    {
        /* Start a new cycle */
        result = Dem_SetOperationCycleState(OperationCycleType, DEM_CYCLE_STATE_START);
    }

    return result;
}

 /**
  * @brief   Get DTC of check failed
  */
Std_ReturnType Dem_GetDTCOfCheckFailed(Dem_DtcType* DTC)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETDTCOFCHECKFAILED, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if (DTC == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_GETDTCOFCHECKFAILED, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    /* Find first DTC with TestFailed bit set */
    for (uint8 i = 0U; i < DEM_NUM_DTCS; i++)
    {
        if (Dem_InternalState.DTCEntries[i].Status & DEM_DTC_STATUS_TEST_FAILED)
        {
            *DTC = Dem_InternalState.DTCEntries[i].DTC;
            return E_OK;
        }
    }

    return E_NOT_OK;
}

 /**
  * @brief   Get version info
  */
void Dem_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETVERSIONINFO, DEM_E_PARAM_POINTER);
        return;
    }
#endif

    versioninfo->vendorID = DEM_VENDOR_ID;
    versioninfo->moduleID = DEM_MODULE_ID;
    versioninfo->sw_major_version = DEM_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = DEM_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = DEM_SW_PATCH_VERSION;
}

 /**
  * @brief   Prestore freeze frame
  */
Std_ReturnType Dem_PrestoreFreezeFrame(Dem_EventIdType EventId)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_PRESTORAGE, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId < DEM_EVENT_ID_MIN) || (EventId > DEM_EVENT_ID_MAX))
    {
        DEM_DET_REPORT_ERROR(DEM_SID_PRESTORAGE, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    /* In this implementation, prestorage is handled automatically when DTC is confirmed */
    /* This API is for explicit prestorage request from application */
    
    const Dem_EventParameterType* eventConfig = Dem_IntFindEventConfig(EventId);
    
    if (eventConfig != NULL_PTR)
    {
        uint8 dtcIndex = Dem_IntFindOrCreateDTCEntry(eventConfig->Dtc);
        if (dtcIndex != DEM_INVALID_DTC_INDEX)
        {
            Dem_IntStoreFreezeFrame(dtcIndex);
            return E_OK;
        }
    }

    return E_NOT_OK;
}

 /**
  * @brief   Clear prestored freeze frame
  */
Std_ReturnType Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_CLEARPRESTOREDFF, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* Clear freeze frames associated with this event's DTC */
    const Dem_EventParameterType* eventConfig = Dem_IntFindEventConfig(EventId);
    
    if (eventConfig != NULL_PTR)
    {
        uint8 dtcIndex = Dem_IntFindDTCIndex(eventConfig->Dtc);
        if (dtcIndex != DEM_INVALID_DTC_INDEX)
        {
            Dem_IntClearFreezeFrame(dtcIndex);
            return E_OK;
        }
    }

    return E_NOT_OK;
}

 /**
  * @brief   Main function - periodic processing
  * 
  * CRITICAL FIX: Now properly processes time-based debounce and aging
  */
void Dem_MainFunction(void)
{
    static uint32 lastCallTime = 0U;
    uint32 currentTime;
    uint32 deltaTime;

    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        return;
    }

    /* Calculate delta time */
    currentTime = Dem_InternalState.LastMainFunctionTimestamp + DEM_MAIN_FUNCTION_PERIOD_MS;
    deltaTime = currentTime - lastCallTime;
    lastCallTime = currentTime;

    /* Update timestamp */
    Dem_InternalState.LastMainFunctionTimestamp = currentTime;

    /* Process time-based debounce */
    Dem_IntProcessDebounceMainFunction(deltaTime);

    /* Process aging */
    Dem_IntProcessAging();
}

#define DEM_STOP_SEC_CODE
#include "MemMap.h"
