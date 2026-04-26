/** @file Dem.c
 * @brief Diagnostic Event Manager (Dem) implementation
 * @details AUTOSAR R22-11 compliant Dem module
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "Dem.h"
#include <string.h>

/*=============================================================================
 * Internal Event Status Storage
 *=============================================================================*/
#define DEM_MAX_EVENTS 64U

typedef struct {
    uint16 eventId;
    uint8 status;
    uint8 errorCounter;
    boolean isActive;
} Dem_EventStatusType_Internal;

static Dem_EventStatusType_Internal Dem_EventStatus[DEM_MAX_EVENTS];
static boolean Dem_Initialized = FALSE;

/*=============================================================================
 * Helper Functions
 *=============================================================================*/
static sint16 Dem_FindEventIndex(uint16 EventId) {
    for (uint16 i = 0U; i < DEM_MAX_EVENTS; i++) {
        if (Dem_EventStatus[i].isActive && Dem_EventStatus[i].eventId == EventId) {
            return (sint16)i;
        }
    }
    return -1;
}

static sint16 Dem_GetFreeIndex(void) {
    for (uint16 i = 0U; i < DEM_MAX_EVENTS; i++) {
        if (!Dem_EventStatus[i].isActive) {
            return (sint16)i;
        }
    }
    return -1;
}

/*=============================================================================
 * Dem Module Functions
 *=============================================================================*/
Std_ReturnType Dem_ReportErrorStatus(
    uint16 EventId,
    Dem_EventStatusType EventStatus)
{
    Std_ReturnType ret = E_OK;
    
    if (!Dem_Initialized) {
        memset(Dem_EventStatus, 0, sizeof(Dem_EventStatus));
        Dem_Initialized = TRUE;
    }
    
    sint16 index = Dem_FindEventIndex(EventId);
    if (index < 0) {
        index = Dem_GetFreeIndex();
        if (index < 0) {
            return E_NOT_OK; /* No free slots */
        }
        Dem_EventStatus[index].eventId = EventId;
        Dem_EventStatus[index].isActive = TRUE;
    }
    
    Dem_EventStatus[index].status = (uint8)EventStatus;
    
    /* Update error counter based on status */
    switch (EventStatus) {
        case DEM_EVENT_STATUS_FAILED:
            if (Dem_EventStatus[index].errorCounter < 255U) {
                Dem_EventStatus[index].errorCounter++;
            }
            break;
        case DEM_EVENT_STATUS_PASSED:
            Dem_EventStatus[index].errorCounter = 0U;
            break;
        case DEM_EVENT_STATUS_PREFAILED:
            /* Do nothing, wait for confirmed failure */
            break;
        case DEM_EVENT_STATUS_PREPASSED:
            /* Do nothing, wait for confirmed pass */
            break;
        default:
            ret = E_NOT_OK;
            break;
    }
    
    return ret;
}

Std_ReturnType Dem_GetEventStatus(
    uint16 EventId,
    uint8* EventStatusExtended)
{
    if (EventStatusExtended == NULL_PTR) {
        return E_NOT_OK;
    }
    
    sint16 index = Dem_FindEventIndex(EventId);
    if (index < 0) {
        return E_NOT_OK;
    }
    
    *EventStatusExtended = Dem_EventStatus[index].status;
    return E_OK;
}

Std_ReturnType Dem_ResetEventStatus(uint16 EventId)
{
    sint16 index = Dem_FindEventIndex(EventId);
    if (index < 0) {
        return E_NOT_OK;
    }
    
    Dem_EventStatus[index].status = DEM_EVENT_STATUS_PASSED;
    Dem_EventStatus[index].errorCounter = 0U;
    
    return E_OK;
}
