/**
 * @file dem_event.c
 * @brief DEM Event Management Implementation
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "dem_types.h"
#include <string.h>

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#include "dem_event.h"

/*============================================================================*
 * Internal Data
 *============================================================================*/
static Dem_EventEntryType s_eventEntries[DEM_MAX_EVENTS];
static uint16_t s_eventCount = 0U;
static boolean s_eventInitialized = FALSE;
static Dem_EventStatusChangedCallbackType s_eventStatusCallback = NULL_PTR;

/*============================================================================*
 * Static Helper Functions
 *============================================================================*/
/**
 * @brief Get free event entry
 */
static Dem_EventEntryType* Dem_GetFreeEventEntry(void)
{
    Dem_EventEntryType* entry = NULL_PTR;
    
    for (uint16_t i = 0U; i < DEM_MAX_EVENTS; i++) {
        if (s_eventEntries[i].isActive == FALSE) {
            entry = &s_eventEntries[i];
            break;
        }
    }
    
    return entry;
}

/**
 * @brief Initialize event entry
 */
static void Dem_InitEventEntry(Dem_EventEntryType* entry)
{
    if (entry != NULL_PTR) {
        entry->eventId = DEM_EVENT_ID_INVALID;
        entry->eventStatus = DEM_EVENT_STATUS_PASSED;
        entry->dtcStatus = DEM_UDS_STATUS_TNCSLC | DEM_UDS_STATUS_TNCTOC;
        entry->debounceCounter = 0;
        entry->occurrenceCounter = 0U;
        entry->faultDetectionCounter = 0U;
        entry->failureCounter = 0U;
        entry->lastReportTimestamp = 0U;
        entry->isAvailable = TRUE;
        entry->isEnabled = TRUE;
        entry->isSuppressed = FALSE;
        entry->isActive = FALSE;
        entry->config = NULL_PTR;
    }
}

/**
 * @brief Set UDS status bit
 */
static void Dem_SetDtcStatusBit(
    Dem_EventEntryType* entry,
    Dem_UdsStatusByteType bit)
{
    if (entry != NULL_PTR) {
        entry->dtcStatus |= bit;
    }
}

/**
 * @brief Clear UDS status bit
 */
static void Dem_ClearDtcStatusBit(
    Dem_EventEntryType* entry,
    Dem_UdsStatusByteType bit)
{
    if (entry != NULL_PTR) {
        entry->dtcStatus &= (Dem_UdsStatusByteType)(~bit);
    }
}

/*============================================================================*
 * Public Functions
 *============================================================================*/
Std_ReturnType Dem_EventInit(void)
{
    Std_ReturnType result = E_OK;
    
    /* Initialize all event entries */
    for (uint16_t i = 0U; i < DEM_MAX_EVENTS; i++) {
        Dem_InitEventEntry(&s_eventEntries[i]);
    }
    
    s_eventCount = 0U;
    s_eventInitialized = TRUE;
    
    return result;
}

Std_ReturnType Dem_SetEventStatus(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_eventInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    if ((EventId == DEM_EVENT_ID_INVALID) || 
        (EventId > DEM_EVENT_ID_MAX)) {
        return E_NOT_OK;
    }
    
    /* Find or create event entry */
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry == NULL_PTR) {
        entry = Dem_GetFreeEventEntry();
        if (entry == NULL_PTR) {
            return E_NOT_OK; /* No free slots */
        }
        entry->eventId = EventId;
        entry->isActive = TRUE;
        s_eventCount++;
    }
    
    /* Check if event is available and enabled */
    if ((entry->isAvailable == FALSE) || (entry->isEnabled == FALSE)) {
        return E_OK; /* Silently ignore */
    }
    
    /* Store old status for callback */
    Dem_EventStatusType oldStatus = entry->eventStatus;
    
    /* Process based on debounce algorithm */
    if (entry->config != NULL_PTR) {
        switch (entry->config->debounceAlgorithm) {
            case DEM_DEBOUNCE_ALGORITHM_COUNTER_BASED:
                Dem_ProcessDebounceCounter(entry, EventStatus);
                break;
                
            case DEM_DEBOUNCE_ALGORITHM_TIME_BASED:
                Dem_ProcessDebounceTime(entry, EventStatus);
                break;
                
            case DEM_DEBOUNCE_ALGORITHM_MONITOR_BASED:
            case DEM_DEBOUNCE_ALGORITHM_NONE:
            default:
                /* No debouncing - use status directly */
                entry->eventStatus = EventStatus;
                if ((EventStatus == DEM_EVENT_STATUS_FAILED) ||
                    (EventStatus == DEM_EVENT_STATUS_PASSED)) {
                    entry->debounceCounter = (EventStatus == DEM_EVENT_STATUS_FAILED) ? 
                        DEM_DEBOUNCE_COUNTER_MAX : DEM_DEBOUNCE_COUNTER_MIN;
                }
                break;
        }
    }
    else {
        /* No config - use status directly */
        entry->eventStatus = EventStatus;
    }
    
    /* Update DTC status */
    Dem_UpdateDtcStatus(entry, entry->eventStatus);

    /* Notify callback if registered */
    if (s_eventStatusCallback != NULL_PTR) {
        s_eventStatusCallback(EventId, oldStatus, entry->eventStatus);
    }
    
    /* Update timestamp */
    entry->lastReportTimestamp = 0U; /* TODO: Get system timestamp */
    
    result = E_OK;
    
    return result;
}

Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType EventId,
    Dem_UdsStatusByteType* EventStatusExtended)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (EventStatusExtended == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (s_eventInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        *EventStatusExtended = entry->dtcStatus;
        result = E_OK;
    }
    else {
        *EventStatusExtended = 0U;
    }
    
    return result;
}

Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_eventInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        /* Reset event status */
        entry->eventStatus = DEM_EVENT_STATUS_PASSED;
        entry->debounceCounter = 0;
        
        /* Reset UDS status bits */
        Dem_ClearDtcStatusBit(entry, DEM_UDS_STATUS_TF);
        Dem_ClearDtcStatusBit(entry, DEM_UDS_STATUS_TFTOC);
        Dem_ClearDtcStatusBit(entry, DEM_UDS_STATUS_CDTC);
        
        /* Set test not completed bits */
        Dem_SetDtcStatusBit(entry, DEM_UDS_STATUS_TNCSLC);
        Dem_SetDtcStatusBit(entry, DEM_UDS_STATUS_TNCTOC);
        
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetEventDebouncingStatus(
    Dem_EventIdType EventId,
    Dem_DebounceStateType* DebounceStatus)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (DebounceStatus == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        /* Map debounce counter to status */
        if (entry->debounceCounter <= DEM_DEBOUNCE_COUNTER_MIN) {
            *DebounceStatus = DEM_DEBOUNCE_STATUS_PASSED;
        }
        else if (entry->debounceCounter >= DEM_DEBOUNCE_COUNTER_MAX) {
            *DebounceStatus = DEM_DEBOUNCE_STATUS_FAILED;
        }
        else if (entry->debounceCounter > 0) {
            *DebounceStatus = DEM_DEBOUNCE_STATUS_PREFAILED;
        }
        else if (entry->debounceCounter < 0) {
            *DebounceStatus = DEM_DEBOUNCE_STATUS_PREPASSED;
        }
        else {
            *DebounceStatus = DEM_DEBOUNCE_STATUS_NONE;
        }
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetEventFailed(
    Dem_EventIdType EventId,
    boolean* EventFailed)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (EventFailed == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        *EventFailed = (boolean)((entry->dtcStatus & DEM_UDS_STATUS_TF) != 0U);
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetEventTested(
    Dem_EventIdType EventId,
    boolean* EventTested)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (EventTested == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_EventEntryType* entry = Dem_FindEventEntry(EventId);
    
    if (entry != NULL_PTR) {
        /* Event is tested if TNCTOC bit is 0 */
        *EventTested = (boolean)((entry->dtcStatus & DEM_UDS_STATUS_TNCTOC) == 0U);
        result = E_OK;
    }
    
    return result;
}

uint16_t Dem_GetNumberOfEvents(void)
{
    return s_eventCount;
}

Dem_EventEntryType* Dem_FindEventEntry(Dem_EventIdType EventId)
{
    Dem_EventEntryType* entry = NULL_PTR;
    
    for (uint16_t i = 0U; i < DEM_MAX_EVENTS; i++) {
        if ((s_eventEntries[i].isActive == TRUE) &&
            (s_eventEntries[i].eventId == EventId)) {
            entry = &s_eventEntries[i];
            break;
        }
    }
    
    return entry;
}

void Dem_ProcessDebounceCounter(
    Dem_EventEntryType* eventEntry,
    Dem_EventStatusType EventStatus)
{
    if ((eventEntry == NULL_PTR) || (eventEntry->config == NULL_PTR)) {
        return;
    }
    
    const Dem_DebounceCounterBasedConfigType* config = 
        &eventEntry->config->debounceConfig.counterConfig;
    
    switch (EventStatus) {
        case DEM_EVENT_STATUS_FAILED:
            /* Jump to failed if configured */
            if (config->jumpUp == TRUE) {
                eventEntry->debounceCounter = DEM_DEBOUNCE_COUNTER_MAX;
            }
            else {
                eventEntry->debounceCounter += config->debounceCounterIncrementStepSize;
                if (eventEntry->debounceCounter > DEM_DEBOUNCE_COUNTER_MAX) {
                    eventEntry->debounceCounter = DEM_DEBOUNCE_COUNTER_MAX;
                }
            }
            
            /* Check threshold */
            if (eventEntry->debounceCounter >= (sint8)config->debounceCounterFailedThreshold) {
                eventEntry->eventStatus = DEM_EVENT_STATUS_FAILED;
            }
            break;
            
        case DEM_EVENT_STATUS_PASSED:
            /* Jump to passed if configured */
            if (config->jumpDown == TRUE) {
                eventEntry->debounceCounter = DEM_DEBOUNCE_COUNTER_MIN;
            }
            else {
                eventEntry->debounceCounter -= config->debounceCounterDecrementStepSize;
                if (eventEntry->debounceCounter < DEM_DEBOUNCE_COUNTER_MIN) {
                    eventEntry->debounceCounter = DEM_DEBOUNCE_COUNTER_MIN;
                }
            }
            
            /* Check threshold */
            if (eventEntry->debounceCounter <= (sint8)config->debounceCounterPassedThreshold) {
                eventEntry->eventStatus = DEM_EVENT_STATUS_PASSED;
            }
            break;
            
        case DEM_EVENT_STATUS_PREFAILED:
            eventEntry->debounceCounter += config->debounceCounterIncrementStepSize;
            if (eventEntry->debounceCounter > DEM_DEBOUNCE_COUNTER_MAX) {
                eventEntry->debounceCounter = DEM_DEBOUNCE_COUNTER_MAX;
            }
            break;
            
        case DEM_EVENT_STATUS_PREPASSED:
            eventEntry->debounceCounter -= config->debounceCounterDecrementStepSize;
            if (eventEntry->debounceCounter < DEM_DEBOUNCE_COUNTER_MIN) {
                eventEntry->debounceCounter = DEM_DEBOUNCE_COUNTER_MIN;
            }
            break;
            
        default:
            /* No action */
            break;
    }
}

void Dem_ProcessDebounceTime(
    Dem_EventEntryType* eventEntry,
    Dem_EventStatusType EventStatus)
{
    /* Time-based debouncing would require a timer */
    /* For now, treat similar to counter-based with time as counter */
    if (eventEntry != NULL_PTR) {
        /* Simplified implementation - treat as counter-based */
        Dem_ProcessDebounceCounter(eventEntry, EventStatus);
    }
}

void Dem_UpdateDtcStatus(
    Dem_EventEntryType* eventEntry,
    Dem_EventStatusType EventStatusNew)
{
    if (eventEntry == NULL_PTR) {
        return;
    }
    
    /* Store old status for callback */
    Dem_UdsStatusByteType oldDtcStatus = eventEntry->dtcStatus;
    
    /* Update TF (Test Failed) bit */
    if (EventStatusNew == DEM_EVENT_STATUS_FAILED) {
        Dem_SetDtcStatusBit(eventEntry, DEM_UDS_STATUS_TF);
        Dem_SetDtcStatusBit(eventEntry, DEM_UDS_STATUS_TFTOC);
        Dem_SetDtcStatusBit(eventEntry, DEM_UDS_STATUS_TFSLC);
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_TNCTOC);
        
        /* Update occurrence counter */
        if (eventEntry->occurrenceCounter < 0xFFFFFFFFU) {
            eventEntry->occurrenceCounter++;
        }
        
        /* Update pending and confirmed status */
        Dem_SetDtcStatusBit(eventEntry, DEM_UDS_STATUS_PDTC);
        
        /* Confirmed DTC after 40/80 cycles (simplified - immediate) */
        if (eventEntry->occurrenceCounter >= 1U) {
            Dem_SetDtcStatusBit(eventEntry, DEM_UDS_STATUS_CDTC);
        }
    }
    else if (EventStatusNew == DEM_EVENT_STATUS_PASSED) {
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_TF);
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_TFTOC);
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_PDTC);
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_TNCTOC);
    }
    else {
        /* PREPASSED or PREFAILED - mark as tested */
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_TNCTOC);
        Dem_ClearDtcStatusBit(eventEntry, DEM_UDS_STATUS_TNCSLC);
    }
    
    /* Notify DCM callback if DTC status changed */
    if ((oldDtcStatus != eventEntry->dtcStatus) && (eventEntry->config != NULL_PTR)) {
        /* Forward to DCM-DEM integration */
        extern void Dcm_DemIntegration_NotifyDtcStatusChanged(
            uint32_t dtc,
            Dem_UdsStatusByteType statusOld,
            Dem_UdsStatusByteType statusNew
        );
        Dcm_DemIntegration_NotifyDtcStatusChanged(
            eventEntry->config->dtcCode,
            oldDtcStatus,
            eventEntry->dtcStatus
        );
    }
}

/*============================================================================*
 * Callback Registration Functions
 *============================================================================*/
Std_ReturnType Dem_RegisterEventStatusChangedCallback(
    Dem_EventStatusChangedCallbackType callback)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (callback != NULL_PTR) {
        s_eventStatusCallback = callback;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_UnregisterEventStatusChangedCallback(void)
{
    s_eventStatusCallback = NULL_PTR;
    return E_OK;
}
