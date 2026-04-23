/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Peripheral           : N/A (Service Layer)
* Dependencies         : NvM
*
* SW Version           : 1.0.0
* Build Version        : S32K3XXS32K3XX_MCAL_1.0.0
* Build Date           : 2026-04-15
* Author               : AI Agent (Dem Development)
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Dem.h"
#include "Dem_Cfg.h"
#include "NvM.h"
#include "Det.h"
#include "MemMap.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DEM_INSTANCE_ID                 (0x00U)

/* Module state */
#define DEM_STATE_UNINIT                (0x00U)
#define DEM_STATE_INIT                  (0x01U)

/* Debounce algorithm types */
#define DEM_DEBOUNCE_ALGORITHM_NONE     (0x00U)
#define DEM_DEBOUNCE_ALGORITHM_COUNTER  (0x01U)
#define DEM_DEBOUNCE_ALGORITHM_TIME     (0x02U)
#define DEM_DEBOUNCE_ALGORITHM_MONITOR  (0x03U)

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
*                                  LOCAL TYPE DEFINITIONS
==================================================================================================*/
/* DTC entry type */
typedef struct
{
    Dem_DTCType DTC;
    Dem_DTCStatusType Status;
    uint32 OccurrenceCounter;
    uint32 AgingCounter;
    boolean IsAged;
    boolean IsSuppressed;
} Dem_DTCEntryType;

/* Freeze frame entry */
typedef struct
{
    uint8 Data[DEM_FREEZE_FRAME_MAX_SIZE];
    uint16 Length;
    boolean IsValid;
    uint32 Timestamp;
} Dem_FreezeFrameEntryType;

/* Module internal state */
typedef struct
{
    uint8 State;
    const Dem_ConfigType* ConfigPtr;
    Dem_EventStateType EventStates[DEM_NUM_EVENTS];
    Dem_DTCEntryType DTCEntries[DEM_NUM_DTCS];
    uint8 OperationCycleStates[DEM_NUM_OPERATION_CYCLES];
    boolean EnableConditions[DEM_NUM_ENABLE_CONDITIONS];
    boolean StorageConditions[DEM_NUM_STORAGE_CONDITIONS];
    Dem_DTCType SelectedDTC;
    boolean DTCRecordUpdateDisabled;
    Dem_FreezeFrameEntryType FreezeFrames[DEM_NUM_FREEZE_FRAME_RECORDS];
    boolean DTCSettingDisabled;
} Dem_InternalStateType;

/*==================================================================================================
*                                  LOCAL VARIABLE DECLARATIONS
==================================================================================================*/
#define DEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC Dem_InternalStateType Dem_InternalState;

#define DEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                  LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC const Dem_EventParameterType* Dem_FindEventConfig(Dem_EventIdType EventId);
STATIC const Dem_DTCParameterType* Dem_FindDTCConfig(Dem_DTCType DTC);
STATIC uint8 Dem_FindDTCIndex(Dem_DTCType DTC);
STATIC void Dem_UpdateDebounceCounter(Dem_EventIdType EventId, Dem_EventStatusType EventStatus);
STATIC void Dem_UpdateDTCStatus(Dem_EventIdType EventId);
STATIC void Dem_ProcessAging(void);
STATIC void Dem_ResetDebounceCounter(Dem_EventIdType EventId);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Find event configuration by Event ID
 */
STATIC const Dem_EventParameterType* Dem_FindEventConfig(Dem_EventIdType EventId)
{
    const Dem_EventParameterType* result = NULL_PTR;
    uint8 i;

    if ((Dem_InternalState.ConfigPtr != NULL_PTR) && (EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
    {
        for (i = 0U; i < Dem_InternalState.ConfigPtr->NumEvents; i++)
        {
            if (Dem_InternalState.ConfigPtr->EventParameters[i].EventId == EventId)
            {
                result = &Dem_InternalState.ConfigPtr->Events[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find DTC configuration by DTC value
 */
STATIC const Dem_DTCParameterType* Dem_FindDTCConfig(Dem_DTCType DTC)
{
    const Dem_DTCParameterType* result = NULL_PTR;
    uint8 i;

    if (Dem_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < Dem_InternalState.ConfigPtr->NumDTCs; i++)
        {
            if (Dem_InternalState.ConfigPtr->DtcParameters[i].Dtc == DTC)
            {
                result = &Dem_InternalState.ConfigPtr->DtcParameters[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief   Find DTC index by DTC value
 */
STATIC uint8 Dem_FindDTCIndex(Dem_DTCType DTC)
{
    uint8 result = 0xFFU;
    uint8 i;

    for (i = 0U; i < DEM_NUM_DTCS; i++)
    {
        if (Dem_InternalState.DTCEntries[i].DTC == DTC)
        {
            result = i;
            break;
        }
    }

    return result;
}

/**
 * @brief   Reset debounce counter for an event
 */
STATIC void Dem_ResetDebounceCounter(Dem_EventIdType EventId)
{
    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
    {
        Dem_InternalState.EventStates[EventId - 1U].DebounceCounter = 0;
    }
}

/**
 * @brief   Update debounce counter based on event status
 */
STATIC void Dem_UpdateDebounceCounter(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    const Dem_EventParameterType* eventConfig;
    Dem_EventStateType* eventState;

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
    {
        eventConfig = Dem_FindEventConfig(EventId);
        eventState = &Dem_InternalState.EventStates[EventId - 1U];

        if (eventConfig != NULL_PTR)
        {
            switch (EventStatus)
            {
                case DEM_EVENT_STATUS_PASSED:
                    /* Reset debounce counter */
                    eventState->DebounceCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
                    break;

                case DEM_EVENT_STATUS_FAILED:
                    /* Set debounce counter to failed threshold */
                    eventState->DebounceCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
                    break;

                case DEM_EVENT_STATUS_PREPASSED:
                    /* Decrement debounce counter */
                    if (eventState->DebounceCounter > DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
                    {
                        eventState->DebounceCounter -= DEM_DEBOUNCE_COUNTER_DECREMENT_STEP;
                    }
                    eventState->FaultDetectionCounter = (Dem_FaultDetectionCounterType)eventState->DebounceCounter;
                    break;

                case DEM_EVENT_STATUS_PREFAILED:
                    /* Increment debounce counter */
                    if (eventState->DebounceCounter < DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
                    {
                        eventState->DebounceCounter += DEM_DEBOUNCE_COUNTER_INCREMENT_STEP;
                    }
                    eventState->FaultDetectionCounter = (Dem_FaultDetectionCounterType)eventState->DebounceCounter;
                    break;

                default:
                    /* Do nothing */
                    break;
            }
        }
    }
}

/**
 * @brief   Update DTC status based on event state
 */
/**
 * @brief   Store freeze frame data for a DTC
 */
STATIC void Dem_StoreFreezeFrame(uint8 dtcIndex)
{
    uint8 i;

    if (dtcIndex < DEM_NUM_FREEZE_FRAME_RECORDS)
    {
        Dem_FreezeFrameEntryType* freezeFrame = &Dem_InternalState.FreezeFrames[dtcIndex];

        if (!freezeFrame->IsValid)
        {
            /* Capture snapshot data (simplified - would read actual DID data) */
            freezeFrame->Length = DEM_FREEZE_FRAME_MAX_SIZE;
            for (i = 0U; i < DEM_FREEZE_FRAME_MAX_SIZE; i++)
            {
                freezeFrame->Data[i] = (uint8)(i + dtcIndex);
            }
            freezeFrame->IsValid = TRUE;
            freezeFrame->Timestamp = 0U; /* Would use actual timestamp */
        }
    }
}

STATIC void Dem_UpdateDTCStatus(Dem_EventIdType EventId)
{
    const Dem_EventParameterType* eventConfig;
    Dem_EventStateType* eventState;
    uint8 dtcIndex;

    eventConfig = Dem_FindEventConfig(EventId);

    if (eventConfig != NULL_PTR)
    {
        eventState = &Dem_InternalState.EventStates[EventId - 1U];
        dtcIndex = Dem_FindDTCIndex(eventConfig->Dtc);

        if (dtcIndex != 0xFFU)
        {
            Dem_DTCEntryType* dtcEntry = &Dem_InternalState.DTCEntries[dtcIndex];

            /* Update Test Failed status */
            if (eventState->DebounceCounter >= DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD)
            {
                /* Set Test Failed */
                dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED;
                dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE;
                dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR;

                /* Set Pending DTC */
                dtcEntry->Status |= DEM_DTC_STATUS_PENDING_DTC;

                /* Increment occurrence counter */
                if (dtcEntry->OccurrenceCounter < DEM_MAX_OCCURRENCE_COUNTER)
                {
                    dtcEntry->OccurrenceCounter++;
                }

                /* Set Confirmed DTC after sufficient occurrences */
                if (dtcEntry->OccurrenceCounter >= 2U)
                {
                    boolean wasConfirmed = (dtcEntry->Status & DEM_DTC_STATUS_CONFIRMED_DTC) != 0U;
                    dtcEntry->Status |= DEM_DTC_STATUS_CONFIRMED_DTC;

                    /* Store freeze frame when DTC first becomes confirmed */
                    if (!wasConfirmed)
                    {
                        Dem_StoreFreezeFrame(dtcIndex);
                    }
                }

                /* Reset aging counter */
                dtcEntry->AgingCounter = 0U;
                dtcEntry->IsAged = FALSE;
            }
            else if (eventState->DebounceCounter <= DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD)
            {
                /* Clear Test Failed */
                dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_FAILED;

                /* Clear Pending DTC if test passed this cycle */
                if (eventState->TestCompletedThisOperationCycle)
                {
                    dtcEntry->Status &= ~DEM_DTC_STATUS_PENDING_DTC;
                }
            }

            /* Update Test Not Completed This Operation Cycle */
            dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;

            /* Update Test Not Completed Since Last Clear */
            dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR;
        }
    }
}

/**
 * @brief   Process DTC aging
 */
STATIC void Dem_ProcessAging(void)
{
    uint8 i;

    for (i = 0U; i < DEM_NUM_DTCS; i++)
    {
        Dem_DTCEntryType* dtcEntry = &Dem_InternalState.DTCEntries[i];

        /* Check if DTC can be aged */
        if ((dtcEntry->Status & DEM_DTC_STATUS_CONFIRMED_DTC) &&
            !(dtcEntry->Status & DEM_DTC_STATUS_TEST_FAILED))
        {
            /* Increment aging counter */
            if (dtcEntry->AgingCounter < DEM_AGING_CYCLE_COUNTER_THRESHOLD)
            {
                dtcEntry->AgingCounter++;
            }

            /* Check if aging threshold reached */
            if (dtcEntry->AgingCounter >= DEM_AGING_THRESHOLD)
            {
                /* Age the DTC */
                dtcEntry->Status &= ~DEM_DTC_STATUS_CONFIRMED_DTC;
                dtcEntry->Status &= ~DEM_DTC_STATUS_PENDING_DTC;
                dtcEntry->IsAged = TRUE;
            }
        }
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

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
    }

    /* Initialize DTC entries */
    for (i = 0U; i < DEM_NUM_DTCS; i++)
    {
        if (i < ConfigPtr->NumDTCs)
        {
            Dem_InternalState.DTCEntries[i].DTC = ConfigPtr->DtcParameters[i].Dtc;
        }
        else
        {
            Dem_InternalState.DTCEntries[i].DTC = 0U;
        }
        Dem_InternalState.DTCEntries[i].Status = DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR |
                                                  DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
        Dem_InternalState.DTCEntries[i].OccurrenceCounter = 0U;
        Dem_InternalState.DTCEntries[i].AgingCounter = 0U;
        Dem_InternalState.DTCEntries[i].IsAged = FALSE;
        Dem_InternalState.DTCEntries[i].IsSuppressed = FALSE;
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
    }

    /* Set module state to initialized */
    Dem_InternalState.State = DEM_STATE_INIT;
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
 * @brief   Set event status
 */
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    Std_ReturnType result = E_NOT_OK;
    Dem_EventStateType* eventState;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
    {
        eventState = &Dem_InternalState.EventStates[EventId - 1U];

        /* Store last reported status */
        eventState->LastReportedStatus = EventStatus;

        /* Update debounce counter */
        Dem_UpdateDebounceCounter(EventId, EventStatus);

        /* Mark test as completed this operation cycle */
        eventState->TestCompletedThisOperationCycle = TRUE;

        /* Update DTC status */
        Dem_UpdateDTCStatus(EventId);

        result = E_OK;
    }

    return result;
}

/**
 * @brief   Reset event status
 */
Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    Dem_EventStateType* eventState;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_RESETEVENTSTATUS, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_RESETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }
#endif

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS))
    {
        eventState = &Dem_InternalState.EventStates[EventId - 1U];

        /* Reset debounce counter */
        Dem_ResetDebounceCounter(EventId);

        /* Reset test completed flag */
        eventState->TestCompletedThisOperationCycle = FALSE;

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

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }

    if (EventStatus == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTSTATUS, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS) && (EventStatus != NULL_PTR))
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

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTFAILED, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }

    if (EventFailed == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTFAILED, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS) && (EventFailed != NULL_PTR))
    {
        *EventFailed = (Dem_InternalState.EventStates[EventId - 1U].DebounceCounter >=
                        DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD) ? TRUE : FALSE;
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

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTTESTED, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }

    if (EventTested == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETEVENTTESTED, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS) && (EventTested != NULL_PTR))
    {
        *EventTested = Dem_InternalState.EventStates[EventId - 1U].TestCompletedThisOperationCycle;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Get fault detection counter
 */
Std_ReturnType Dem_GetFaultDetectionCounter(Dem_EventIdType EventId, Dem_FaultDetectionCounterType* FaultDetectionCounter)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETFAULTDETECTION, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETFAULTDETECTION, DEM_E_PARAM_EVENT_ID);
        return E_NOT_OK;
    }

    if (FaultDetectionCounter == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETFAULTDETECTION, DEM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if ((EventId > 0U) && (EventId <= DEM_NUM_EVENTS) && (FaultDetectionCounter != NULL_PTR))
    {
        *FaultDetectionCounter = Dem_InternalState.EventStates[EventId - 1U].FaultDetectionCounter;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Get DTC status
 */
Std_ReturnType Dem_GetDTCStatus(Dem_DTCType DTC, Dem_DTCOriginType DTCOrigin, Dem_DTCStatusType* DTCStatus)
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

    (void)DTCOrigin; /* Unused parameter */

    dtcIndex = Dem_FindDTCIndex(DTC);

    if ((dtcIndex != 0xFFU) && (DTCStatus != NULL_PTR))
    {
        *DTCStatus = Dem_InternalState.DTCEntries[dtcIndex].Status;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Clear DTC
 */
Std_ReturnType Dem_ClearDTC(Dem_DTCType DTC, Dem_DTCFormatType DTCFormat, Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 dtcIndex;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_CLEARDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    (void)DTCFormat; /* Unused parameter */
    (void)DTCOrigin; /* Unused parameter */

    if (DTC == DEM_DTC_GROUP_ALL)
    {
        /* Clear all DTCs */
        uint8 i;
        for (i = 0U; i < DEM_NUM_DTCS; i++)
        {
            Dem_InternalState.DTCEntries[i].Status = DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR |
                                                      DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
            Dem_InternalState.DTCEntries[i].OccurrenceCounter = 0U;
            Dem_InternalState.DTCEntries[i].AgingCounter = 0U;
            Dem_InternalState.DTCEntries[i].IsAged = FALSE;
        }
        result = E_OK;
    }
    else
    {
        /* Clear specific DTC */
        dtcIndex = Dem_FindDTCIndex(DTC);

        if (dtcIndex != 0xFFU)
        {
            Dem_InternalState.DTCEntries[dtcIndex].Status = DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR |
                                                             DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
            Dem_InternalState.DTCEntries[dtcIndex].OccurrenceCounter = 0U;
            Dem_InternalState.DTCEntries[dtcIndex].AgingCounter = 0U;
            Dem_InternalState.DTCEntries[dtcIndex].IsAged = FALSE;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Select DTC
 */
Std_ReturnType Dem_SelectDTC(Dem_DTCType DTC, Dem_DTCFormatType DTCFormat, Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_SELECTEDDTC, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    (void)DTCFormat; /* Unused parameter */
    (void)DTCOrigin; /* Unused parameter */

    Dem_InternalState.SelectedDTC = DTC;
    result = E_OK;

    return result;
}

/**
 * @brief   Disable DTC record update
 */
Std_ReturnType Dem_DisableDTCRecordUpdate(void)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_DISABLEDTCRECORD, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    Dem_InternalState.DTCRecordUpdateDisabled = TRUE;
    result = E_OK;

    return result;
}

/**
 * @brief   Enable DTC record update
 */
Std_ReturnType Dem_EnableDTCRecordUpdate(void)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_ENABLEDTCRECORD, DEM_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    Dem_InternalState.DTCRecordUpdateDisabled = FALSE;
    result = E_OK;

    return result;
}

/**
 * @brief   Set operation cycle state
 */
Std_ReturnType Dem_SetOperationCycleState(uint8 OperationCycleId, uint8 CycleState)
{
    Std_ReturnType result = E_NOT_OK;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        return E_NOT_OK;
    }

    if (OperationCycleId >= DEM_NUM_OPERATION_CYCLES)
    {
        return E_NOT_OK;
    }
#endif

    if (OperationCycleId < DEM_NUM_OPERATION_CYCLES)
    {
        uint8 oldState = Dem_InternalState.OperationCycleStates[OperationCycleId];
        Dem_InternalState.OperationCycleStates[OperationCycleId] = CycleState;

        /* Handle cycle end */
        if ((oldState == DEM_CYCLE_STATE_START) && (CycleState == DEM_CYCLE_STATE_END))
        {
            uint8 i;

            /* Reset test completed flags for all events */
            for (i = 0U; i < DEM_NUM_EVENTS; i++)
            {
                Dem_InternalState.EventStates[i].TestCompletedThisOperationCycle = FALSE;
            }

            /* Process aging */
            Dem_ProcessAging();
        }

        result = E_OK;
    }

    return result;
}

/**
 * @brief   Get operation cycle state
 */
Std_ReturnType Dem_GetOperationCycleState(uint8 OperationCycleId, uint8* CycleState)
{
    Std_ReturnType result = E_NOT_OK;

    if ((OperationCycleId < DEM_NUM_OPERATION_CYCLES) && (CycleState != NULL_PTR))
    {
        *CycleState = Dem_InternalState.OperationCycleStates[OperationCycleId];
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Main function for DEM processing
 */
void Dem_MainFunction(void)
{
    /* Main function processing is handled in operation cycle state changes */
}

/**
 * @brief   Get version information
 */
#if (DEM_VERSION_INFO_API == STD_ON)
void Dem_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        DEM_DET_REPORT_ERROR(DEM_SERVICE_ID_GETVERSIONINFO, DEM_E_PARAM_POINTER);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = DEM_VENDOR_ID;
        versioninfo->moduleID = DEM_MODULE_ID;
        versioninfo->sw_major_version = DEM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DEM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DEM_SW_PATCH_VERSION;
    }
}
#endif

/**
 * @brief   Pre-store freeze frame data
 */
Std_ReturnType Dem_PrestoreFreezeFrame(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    const Dem_EventParameterType* eventConfig;
    uint8 dtcIndex;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_PRESTORAGE, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SID_PRESTORAGE, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#endif

    eventConfig = Dem_FindEventConfig(EventId);
    if (eventConfig != NULL_PTR)
    {
        dtcIndex = Dem_FindDTCIndex(eventConfig->Dtc);
        if (dtcIndex != 0xFFU)
        {
            Dem_StoreFreezeFrame(dtcIndex);
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Clear pre-stored freeze frame data
 */
Std_ReturnType Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    const Dem_EventParameterType* eventConfig;
    uint8 dtcIndex;

#if (DEM_DEV_ERROR_DETECT == STD_ON)
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
        DEM_DET_REPORT_ERROR(DEM_SID_CLEARPRESTOREDFF, DEM_E_UNINIT);
        return E_NOT_OK;
    }

    if ((EventId == 0U) || (EventId > DEM_NUM_EVENTS))
    {
        DEM_DET_REPORT_ERROR(DEM_SID_CLEARPRESTOREDFF, DEM_E_PARAM_DATA);
        return E_NOT_OK;
    }
#endif

    eventConfig = Dem_FindEventConfig(EventId);
    if (eventConfig != NULL_PTR)
    {
        dtcIndex = Dem_FindDTCIndex(eventConfig->Dtc);
        if ((dtcIndex != 0xFFU) && (dtcIndex < DEM_NUM_FREEZE_FRAME_RECORDS))
        {
            Dem_InternalState.FreezeFrames[dtcIndex].IsValid = FALSE;
            result = E_OK;
        }
    }

    return result;
}

/**
 * @brief   Disable DTC setting
 */
Std_ReturnType Dem_DisableDTCSetting(Dem_DTCType DTCGroup, uint8 DTCKind)
{
    (void)DTCGroup;
    (void)DTCKind;

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
Std_ReturnType Dem_EnableDTCSetting(Dem_DTCType DTCGroup, uint8 DTCKind)
{
    (void)DTCGroup;
    (void)DTCKind;

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
 * @brief   Get freeze frame data by DTC
 */
Std_ReturnType Dem_GetFreezeFrameDataByDTC(Dem_DTCType DTC, Dem_DTCOriginType DTCOrigin,
                                            uint8 RecordNumber, uint8* DestBuffer,
                                            uint16* BufferSize)
{
    uint8 dtcIndex;
    Std_ReturnType result = E_NOT_OK;

    (void)DTCOrigin;
    (void)RecordNumber;

    dtcIndex = Dem_FindDTCIndex(DTC);

    if ((dtcIndex != 0xFFU) && (dtcIndex < DEM_NUM_FREEZE_FRAME_RECORDS) &&
        (DestBuffer != NULL_PTR) && (BufferSize != NULL_PTR))
    {
        Dem_FreezeFrameEntryType* freezeFrame = &Dem_InternalState.FreezeFrames[dtcIndex];

        if (freezeFrame->IsValid)
        {
            uint16 copyLength = (*BufferSize < freezeFrame->Length) ? *BufferSize : freezeFrame->Length;
            uint16 i;

            for (i = 0U; i < copyLength; i++)
            {
                DestBuffer[i] = freezeFrame->Data[i];
            }
            *BufferSize = copyLength;
            result = E_OK;
        }
    }

    return result;
}

/* Stub functions for unimplemented features */
Std_ReturnType Dem_GetNumberOfFilteredDTC(uint16* NumberOfFilteredDTC)
{
    (void)NumberOfFilteredDTC;
    return E_NOT_OK;
}

Std_ReturnType Dem_GetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType* IndicatorStatus)
{
    (void)IndicatorId;
    (void)IndicatorStatus;
    return E_NOT_OK;
}

Std_ReturnType Dem_SetIndicatorStatus(uint8 IndicatorId, Dem_IndicatorStatusType IndicatorStatus)
{
    (void)IndicatorId;
    (void)IndicatorStatus;
    return E_NOT_OK;
}

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                       END OF FILE
==================================================================================================*/
