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
 * @file Dem_Int.c
 * @brief Diagnostic Event Manager - Internal Functions
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Separated internal functions from Dem.c
 * - Time-based debounce support
 * - Extended data record support
 * - Improved error handling
 */

/*==================================================================================================
*                                             INCLUDES
==================================================================================================*/
#include "Dem_Int.h"
#include "Det.h"
#include "string.h"

/*==================================================================================================
*                                  LOCAL CONSTANT DEFINITIONS
==================================================================================================*/
#define DEM_TIME_MAX_DELTA_MS           (0xFFFFFFFFU)

/*==================================================================================================
*                                  EXTERNAL VARIABLES
==================================================================================================*/
#define DEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

Dem_InternalStateType Dem_InternalState;

#define DEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Get current timestamp in milliseconds
 * @return Current timestamp
 */
STATIC uint32 Dem_IntGetCurrentTimestamp(void)
{
    /* This should be replaced with actual OS time service */
    /* For now, increment in MainFunction handles timing */
    return Dem_InternalState.LastMainFunctionTimestamp;
}

/**
 * @brief Check if event ID is valid
 */
STATIC boolean Dem_IntIsValidEventId(Dem_EventIdType EventId)
{
    return ((EventId >= DEM_EVENT_ID_MIN) && (EventId <= DEM_EVENT_ID_MAX));
}

/**
 * @brief Check if DTC index is valid
 */
STATIC boolean Dem_IntIsValidDtcIndex(uint8 DtcIndex)
{
    return (DtcIndex < DEM_NUM_DTCS);
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Find event configuration by Event ID
 */
const Dem_EventParameterType* Dem_IntFindEventConfig(Dem_EventIdType EventId)
{
    const Dem_EventParameterType* result = NULL_PTR;
    uint16 i;

    if ((Dem_InternalState.ConfigPtr != NULL_PTR) && Dem_IntIsValidEventId(EventId))
    {
        for (i = 0U; i < Dem_InternalState.ConfigPtr->NumEvents; i++)
        {
            if (Dem_InternalState.ConfigPtr->EventParameters[i].EventId == EventId)
            {
                /* CRITICAL FIX: Use EventParameters instead of Events */
                result = &Dem_InternalState.ConfigPtr->EventParameters[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @brief Find DTC configuration by DTC value
 */
const Dem_DtcParameterType* Dem_IntFindDTCConfig(Dem_DTCType DTC)
{
    const Dem_DtcParameterType* result = NULL_PTR;
    uint16 i;

    if (Dem_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < Dem_InternalState.ConfigPtr->NumDtcs; i++)
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
 * @brief Find DTC index by DTC value
 */
uint8 Dem_IntFindDTCIndex(Dem_DTCType DTC)
{
    uint8 result = DEM_INVALID_DTC_INDEX;
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
 * @brief Find or create DTC entry
 */
uint8 Dem_IntFindOrCreateDTCEntry(Dem_DTCType DTC)
{
    uint8 dtcIndex = Dem_IntFindDTCIndex(DTC);
    uint8 i;

    if (dtcIndex == DEM_INVALID_DTC_INDEX)
    {
        /* Try to find an empty slot */
        for (i = 0U; i < DEM_NUM_DTCS; i++)
        {
            if ((Dem_InternalState.DTCEntries[i].DTC == 0U) || 
                (Dem_InternalState.DTCEntries[i].IsDeleted))
            {
                dtcIndex = i;
                Dem_InternalState.DTCEntries[i].DTC = DTC;
                Dem_InternalState.DTCEntries[i].IsDeleted = FALSE;
                break;
            }
        }
    }

    return dtcIndex;
}

/**
 * @brief Reset debounce counter for an event
 */
void Dem_IntResetDebounceCounter(Dem_EventIdType EventId)
{
    uint16 eventIndex = EventId - 1U;
    
    if (Dem_IntIsValidEventId(EventId))
    {
        Dem_InternalState.EventStates[eventIndex].DebounceCounter = 0;
        Dem_InternalState.EventStates[eventIndex].FaultDetectionCounter = 0;
        Dem_InternalState.EventStates[eventIndex].TimeInCurrentStatus = 0U;
        Dem_InternalState.TimeDebounceStates[eventIndex].State = DEM_TIME_DEBOUNCE_IDLE;
        Dem_InternalState.TimeDebounceStates[eventIndex].ElapsedTimeMs = 0U;
        Dem_InternalState.TimeDebounceStates[eventIndex].ThresholdReached = FALSE;
    }
}

/**
 * @brief Update debounce counter based on event status
 */
void Dem_IntUpdateDebounceCounter(Dem_EventIdType EventId, Dem_EventStatusType EventStatus)
{
    const Dem_EventParameterType* eventConfig;
    Dem_EventStateType* eventState;
    uint16 eventIndex = EventId - 1U;

    if (!Dem_IntIsValidEventId(EventId))
    {
        return;
    }

    eventConfig = Dem_IntFindEventConfig(EventId);
    eventState = &Dem_InternalState.EventStates[eventIndex];

    if (eventConfig == NULL_PTR)
    {
        return;
    }

    /* Process based on debounce algorithm */
    switch (eventConfig->DebounceAlgorithm)
    {
        case DEM_DEBOUNCE_ALGORITHM_NONE:
            /* No debouncing - direct pass/fail */
            if (EventStatus == DEM_EVENT_STATUS_PASSED)
            {
                eventState->DebounceCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
                eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
            }
            else if (EventStatus == DEM_EVENT_STATUS_FAILED)
            {
                eventState->DebounceCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
                eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
            }
            break;

        case DEM_DEBOUNCE_ALGORITHM_COUNTER:
            /* Counter-based debounce */
            switch (EventStatus)
            {
                case DEM_EVENT_STATUS_PASSED:
                    eventState->DebounceCounter = eventConfig->DebounceCounterPassedThreshold;
                    eventState->FaultDetectionCounter = eventConfig->DebounceCounterPassedThreshold;
                    break;

                case DEM_EVENT_STATUS_FAILED:
                    eventState->DebounceCounter = eventConfig->DebounceCounterFailedThreshold;
                    eventState->FaultDetectionCounter = eventConfig->DebounceCounterFailedThreshold;
                    break;

                case DEM_EVENT_STATUS_PREPASSED:
                    if (eventState->DebounceCounter > eventConfig->DebounceCounterPassedThreshold)
                    {
                        eventState->DebounceCounter -= DEM_DEBOUNCE_COUNTER_DECREMENT_STEP;
                    }
                    eventState->FaultDetectionCounter = (Dem_FaultDetectionCounterType)eventState->DebounceCounter;
                    break;

                case DEM_EVENT_STATUS_PREFAILED:
                    if (eventState->DebounceCounter < eventConfig->DebounceCounterFailedThreshold)
                    {
                        eventState->DebounceCounter += DEM_DEBOUNCE_COUNTER_INCREMENT_STEP;
                    }
                    eventState->FaultDetectionCounter = (Dem_FaultDetectionCounterType)eventState->DebounceCounter;
                    break;

                default:
                    /* Do nothing */
                    break;
            }
            break;

        case DEM_DEBOUNCE_ALGORITHM_TIME:
            /* Time-based debounce - handled in MainFunction */
            /* Just update the last report status here */
            break;

        case DEM_DEBOUNCE_ALGORITHM_MONITOR:
            /* Monitor internal - direct status */
            if (EventStatus == DEM_EVENT_STATUS_PASSED)
            {
                eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
            }
            else if (EventStatus == DEM_EVENT_STATUS_FAILED)
            {
                eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
            }
            break;

        default:
            /* Do nothing */
            break;
    }
}

/**
 * @brief Process time-based debounce
 * CRITICAL FIX: Added complete time-based debounce support
 */
void Dem_IntProcessTimeBasedDebounce(Dem_EventIdType EventId,
                                     Dem_EventStatusType EventStatus,
                                     uint32 DeltaTimeMs)
{
    const Dem_EventParameterType* eventConfig;
    Dem_EventStateType* eventState;
    Dem_TimeDebounceStateType* timeState;
    uint16 eventIndex = EventId - 1U;

    if (!Dem_IntIsValidEventId(EventId))
    {
        return;
    }

    eventConfig = Dem_IntFindEventConfig(EventId);
    if ((eventConfig == NULL_PTR) || 
        (eventConfig->DebounceAlgorithm != DEM_DEBOUNCE_ALGORITHM_TIME))
    {
        return;
    }

    eventState = &Dem_InternalState.EventStates[eventIndex];
    timeState = &Dem_InternalState.TimeDebounceStates[eventIndex];

    /* Process based on current event status */
    switch (EventStatus)
    {
        case DEM_EVENT_STATUS_PASSED:
            /* Immediate passed */
            eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
            timeState->State = DEM_TIME_DEBOUNCE_IDLE;
            timeState->ElapsedTimeMs = 0U;
            timeState->ThresholdReached = TRUE;
            break;

        case DEM_EVENT_STATUS_FAILED:
            /* Immediate failed */
            eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
            timeState->State = DEM_TIME_DEBOUNCE_IDLE;
            timeState->ElapsedTimeMs = 0U;
            timeState->ThresholdReached = TRUE;
            break;

        case DEM_EVENT_STATUS_PREFAILED:
            /* Start or continue counting up */
            if (timeState->State != DEM_TIME_DEBOUNCE_COUNTING_UP)
            {
                timeState->State = DEM_TIME_DEBOUNCE_COUNTING_UP;
                timeState->ElapsedTimeMs = 0U;
            }
            else
            {
                timeState->ElapsedTimeMs += DeltaTimeMs;
                
                /* Check if failed threshold reached */
                if (timeState->ElapsedTimeMs >= eventConfig->DebounceTimeFailedThresholdMs)
                {
                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_FAILED_THRESHOLD;
                    timeState->ThresholdReached = TRUE;
                }
            }
            break;

        case DEM_EVENT_STATUS_PREPASSED:
            /* Start or continue counting down */
            if (timeState->State != DEM_TIME_DEBOUNCE_COUNTING_DOWN)
            {
                timeState->State = DEM_TIME_DEBOUNCE_COUNTING_DOWN;
                timeState->ElapsedTimeMs = 0U;
            }
            else
            {
                timeState->ElapsedTimeMs += DeltaTimeMs;
                
                /* Check if passed threshold reached */
                if (timeState->ElapsedTimeMs >= eventConfig->DebounceTimePassedThresholdMs)
                {
                    eventState->FaultDetectionCounter = DEM_DEBOUNCE_COUNTER_PASSED_THRESHOLD;
                    timeState->ThresholdReached = TRUE;
                }
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Store freeze frame for a DTC
 */
void Dem_IntStoreFreezeFrame(uint8 DtcIndex)
{
    uint16 i;
    Dem_FreezeFrameEntryType* freezeFrame;

    if (!Dem_IntIsValidDtcIndex(DtcIndex))
    {
        return;
    }

    /* Find a free freeze frame record or update existing */
    for (i = 0U; i < DEM_NUM_FREEZE_FRAME_RECORDS; i++)
    {
        freezeFrame = &Dem_InternalState.FreezeFrames[i];
        
        if ((!freezeFrame->IsValid) || (freezeFrame->DtcIndex == DtcIndex))
        {
            /* Capture snapshot data */
            freezeFrame->Length = DEM_FREEZE_FRAME_MAX_SIZE;
            
            /* Fill with sample data - in real implementation, this would read from DIDs */
            for (uint16 j = 0U; j < DEM_FREEZE_FRAME_MAX_SIZE; j++)
            {
                freezeFrame->Data[j] = (uint8)(j + DtcIndex + i);
            }
            
            freezeFrame->IsValid = TRUE;
            freezeFrame->Timestamp = Dem_IntGetCurrentTimestamp();
            freezeFrame->DtcIndex = DtcIndex;
            freezeFrame->OccurrenceCounterSnapshot = Dem_InternalState.DTCEntries[DtcIndex].OccurrenceCounter;
            
            break;
        }
    }
}

/**
 * @brief Clear freeze frame for a DTC
 */
void Dem_IntClearFreezeFrame(uint8 DtcIndex)
{
    uint16 i;

    for (i = 0U; i < DEM_NUM_FREEZE_FRAME_RECORDS; i++)
    {
        if (Dem_InternalState.FreezeFrames[i].DtcIndex == DtcIndex)
        {
            Dem_InternalState.FreezeFrames[i].IsValid = FALSE;
            Dem_InternalState.FreezeFrames[i].Length = 0U;
            Dem_InternalState.FreezeFrames[i].DtcIndex = 0xFFFFU;
        }
    }
}

/**
 * @brief Store extended data record
 * CRITICAL FIX: Added extended data support
 */
void Dem_IntStoreExtendedData(uint8 DtcIndex, uint8 RecordNumber)
{
    Dem_ExtendedDataEntryType* extData;
    const Dem_ExtendedDataRecordType* recordConfig;
    uint8 i;

    if (!Dem_IntIsValidDtcIndex(DtcIndex) || (RecordNumber == 0U))
    {
        return;
    }

    /* Find the extended data record configuration */
    recordConfig = NULL_PTR;
    for (i = 0U; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++)
    {
        if (Dem_InternalState.ConfigPtr->ExtendedDataRecords[i].RecordNumber == RecordNumber)
        {
            recordConfig = &Dem_InternalState.ConfigPtr->ExtendedDataRecords[i];
            break;
        }
    }

    if (recordConfig == NULL_PTR)
    {
        return;
    }

    /* Find or allocate extended data entry */
    for (i = 0U; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++)
    {
        extData = &Dem_InternalState.ExtendedDataRecords[i];
        
        if ((!extData->IsValid) || 
            ((extData->DtcIndex == DtcIndex) && (extData->RecordNumber == RecordNumber)))
        {
            /* Store the extended data */
            extData->Length = recordConfig->DataSize;
            extData->IsValid = TRUE;
            extData->Timestamp = Dem_IntGetCurrentTimestamp();
            extData->DtcIndex = DtcIndex;
            extData->RecordNumber = RecordNumber;

            /* Fill with data based on record number */
            switch (RecordNumber)
            {
                case 1U: /* Occurrence Counter */
                    for (uint16 j = 0U; (j < extData->Length) && (j < 4U); j++)
                    {
                        extData->Data[j] = (uint8)(Dem_InternalState.DTCEntries[DtcIndex].OccurrenceCounter >> (j * 8U));
                    }
                    break;

                case 2U: /* Aging Counter */
                    for (uint16 j = 0U; (j < extData->Length) && (j < 4U); j++)
                    {
                        extData->Data[j] = (uint8)(Dem_InternalState.DTCEntries[DtcIndex].AgingCounter >> (j * 8U));
                    }
                    break;

                default:
                    /* Fill with placeholder data */
                    for (uint16 j = 0U; j < extData->Length; j++)
                    {
                        extData->Data[j] = (uint8)(j + RecordNumber);
                    }
                    break;
            }
            
            break;
        }
    }
}

/**
 * @brief Get extended data record
 */
Std_ReturnType Dem_IntGetExtendedData(uint8 DtcIndex,
                                      uint8 RecordNumber,
                                      uint8* DestBuffer,
                                      uint16* BufferSize)
{
    const Dem_ExtendedDataEntryType* extData;
    uint8 i;
    Std_ReturnType result = E_NOT_OK;

    if ((DestBuffer == NULL_PTR) || (BufferSize == NULL_PTR))
    {
        return E_NOT_OK;
    }

    /* Find the extended data entry */
    for (i = 0U; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++)
    {
        extData = &Dem_InternalState.ExtendedDataRecords[i];
        
        if ((extData->IsValid) && 
            (extData->DtcIndex == DtcIndex) && 
            (extData->RecordNumber == RecordNumber))
        {
            if (*BufferSize >= extData->Length)
            {
                (void)memcpy(DestBuffer, extData->Data, extData->Length);
                *BufferSize = extData->Length;
                result = E_OK;
            }
            else
            {
                result = E_NOT_OK; /* Buffer too small */
            }
            break;
        }
    }

    return result;
}

/**
 * @brief Process DTC aging
 */
void Dem_IntProcessAging(void)
{
    uint8 i;
    const Dem_DtcParameterType* dtcConfig;

    for (i = 0U; i < DEM_NUM_DTCS; i++)
    {
        Dem_DTCEntryType* dtcEntry = &Dem_InternalState.DTCEntries[i];

        /* Check if DTC can be aged */
        if ((dtcEntry->Status & DEM_DTC_STATUS_CONFIRMED_DTC) &&
            !(dtcEntry->Status & DEM_DTC_STATUS_TEST_FAILED) &&
            (!dtcEntry->IsAged))
        {
            /* Get DTC configuration */
            dtcConfig = Dem_IntFindDTCConfig(dtcEntry->DTC);
            
            if (dtcConfig != NULL_PTR)
            {
                /* Increment aging counter */
                if (dtcEntry->AgingCounter < dtcConfig->AgingThreshold)
                {
                    dtcEntry->AgingCounter++;
                }

                /* Check if aging threshold reached */
                if (dtcEntry->AgingCounter >= dtcConfig->AgingThreshold)
                {
                    Dem_IntAgeDTCEntry(i);
                }
            }
        }
    }
}

/**
 * @brief Age a single DTC entry
 */
void Dem_IntAgeDTCEntry(uint8 DtcIndex)
{
    if (!Dem_IntIsValidDtcIndex(DtcIndex))
    {
        return;
    }

    Dem_InternalState.DTCEntries[DtcIndex].Status &= ~DEM_DTC_STATUS_CONFIRMED_DTC;
    Dem_InternalState.DTCEntries[DtcIndex].Status &= ~DEM_DTC_STATUS_PENDING_DTC;
    Dem_InternalState.DTCEntries[DtcIndex].IsAged = TRUE;
    
    /* Optionally clear freeze frame */
    Dem_IntClearFreezeFrame(DtcIndex);
}

/**
 * @brief Clear single DTC
 */
void Dem_IntClearSingleDTC(uint8 DtcIndex)
{
    Dem_DTCEntryType* dtcEntry;

    if (!Dem_IntIsValidDtcIndex(DtcIndex))
    {
        return;
    }

    dtcEntry = &Dem_InternalState.DTCEntries[DtcIndex];

    /* Reset DTC status */
    dtcEntry->Status = DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR |
                       DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
    dtcEntry->OccurrenceCounter = 0U;
    dtcEntry->AgingCounter = 0U;
    dtcEntry->IsAged = FALSE;
    dtcEntry->IsDeleted = TRUE;

    /* Clear freeze frames */
    Dem_IntClearFreezeFrame(DtcIndex);

    /* Clear extended data */
    for (uint8 i = 0U; i < DEM_NUM_EXTENDED_DATA_RECORDS; i++)
    {
        if (Dem_InternalState.ExtendedDataRecords[i].DtcIndex == DtcIndex)
        {
            Dem_InternalState.ExtendedDataRecords[i].IsValid = FALSE;
        }
    }
}

/**
 * @brief Clear all DTCs
 */
void Dem_IntClearAllDTCs(void)
{
    uint8 i;

    for (i = 0U; i < DEM_NUM_DTCS; i++)
    {
        Dem_IntClearSingleDTC(i);
    }
}

/**
 * @brief Update DTC status from debounce result
 */
void Dem_IntUpdateDTCStatusFromDebounce(Dem_EventIdType EventId, boolean DebounceResult)
{
    const Dem_EventParameterType* eventConfig;
    const Dem_EventStateType* eventState;
    uint8 dtcIndex;

    eventConfig = Dem_IntFindEventConfig(EventId);
    if (eventConfig == NULL_PTR)
    {
        return;
    }

    eventState = &Dem_InternalState.EventStates[EventId - 1U];
    dtcIndex = Dem_IntFindOrCreateDTCEntry(eventConfig->Dtc);

    if (dtcIndex == DEM_INVALID_DTC_INDEX)
    {
        return;
    }

    Dem_DTCEntryType* dtcEntry = &Dem_InternalState.DTCEntries[dtcIndex];

    if (DebounceResult)
    {
        /* Test Failed */
        dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED;
        dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE;
        dtcEntry->Status |= DEM_DTC_STATUS_TEST_FAILED_SINCE_LAST_CLEAR;
        dtcEntry->Status |= DEM_DTC_STATUS_PENDING_DTC;

        /* Increment occurrence counter */
        if (dtcEntry->OccurrenceCounter < DEM_MAX_OCCURRENCE_COUNTER)
        {
            dtcEntry->OccurrenceCounter++;
        }

        /* Set Confirmed DTC after sufficient occurrences */
        if (dtcEntry->OccurrenceCounter >= eventConfig->EventConfirmationThreshold)
        {
            boolean wasConfirmed = (dtcEntry->Status & DEM_DTC_STATUS_CONFIRMED_DTC) != 0U;
            dtcEntry->Status |= DEM_DTC_STATUS_CONFIRMED_DTC;

            /* Store freeze frame when DTC first becomes confirmed */
            if (!wasConfirmed)
            {
                Dem_IntStoreFreezeFrame(dtcIndex);
            }
        }

        /* Reset aging counter */
        dtcEntry->AgingCounter = 0U;
        dtcEntry->IsAged = FALSE;
    }
    else
    {
        /* Test Passed */
        dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_FAILED;
        
        if (eventState->TestCompletedThisOperationCycle)
        {
            dtcEntry->Status &= ~DEM_DTC_STATUS_PENDING_DTC;
        }
    }

    /* Update Test Not Completed flags */
    dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
    dtcEntry->Status &= ~DEM_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR;
}

/**
 * @brief Process debounce main function
 */
void Dem_IntProcessDebounceMainFunction(uint32 DeltaTimeMs)
{
    uint16 i;
    const Dem_EventParameterType* eventConfig;

    /* Process time-based debounce for all events */
    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        eventConfig = &Dem_InternalState.ConfigPtr->EventParameters[i];
        
        if (eventConfig->DebounceAlgorithm == DEM_DEBOUNCE_ALGORITHM_TIME)
        {
            /* Time-based debounce is handled in Dem_IntProcessTimeBasedDebounce */
            /* But we need to check for threshold timeout here */
            Dem_TimeDebounceStateType* timeState = &Dem_InternalState.TimeDebounceStates[i];
            
            if ((timeState->State == DEM_TIME_DEBOUNCE_COUNTING_UP) ||
                (timeState->State == DEM_TIME_DEBOUNCE_COUNTING_DOWN))
            {
                /* Check if debounce threshold reached */
                if (timeState->ThresholdReached)
                {
                    /* Update DTC status based on debounce result */
                    boolean debounceResult = (timeState->State == DEM_TIME_DEBOUNCE_COUNTING_UP);
                    Dem_IntUpdateDTCStatusFromDebounce(eventConfig->EventId, debounceResult);
                    
                    /* Reset time debounce state */
                    timeState->State = DEM_TIME_DEBOUNCE_IDLE;
                    timeState->ThresholdReached = FALSE;
                }
            }
        }
    }
}

/**
 * @brief Check module initialization
 */
Std_ReturnType Dem_IntCheckInit(void)
{
    if (Dem_InternalState.State != DEM_STATE_INIT)
    {
#if (DEM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID, 
                              DEM_SERVICE_ID_INIT, DEM_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    return E_OK;
}

/**
 * @brief Validate event ID
 */
Std_ReturnType Dem_IntValidateEventId(Dem_EventIdType EventId)
{
    if (!Dem_IntIsValidEventId(EventId))
    {
#if (DEM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DEM_MODULE_ID, DEM_INSTANCE_ID,
                              DEM_SERVICE_ID_SETEVENTSTATUS, DEM_E_PARAM_EVENT_ID);
#endif
        return E_NOT_OK;
    }
    return E_OK;
}

#define DEM_STOP_SEC_CODE
#include "MemMap.h"
