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
 * @file dem_event.h
 * @brief DEM Event Management Interface
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_EVENT_H
#define DEM_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dem_types.h"

/*============================================================================*
 * Macros and Constants
 *============================================================================*/
#define DEM_MAX_EVENTS                          128U
#define DEM_MAX_EVENT_NAME_LENGTH               32U

/* Debounce counter limits */
#define DEM_DEBOUNCE_COUNTER_MIN                (-128)
#define DEM_DEBOUNCE_COUNTER_MAX                127

/*============================================================================*
 * Callback Function Types
 *============================================================================*/
/**
 * @brief Event Status Changed Callback Type
 * @param EventId The event ID
 * @param EventStatusOld Old event status
 * @param EventStatusNew New event status
 */
typedef void (*Dem_EventStatusChangedCallbackType)(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatusOld,
    Dem_EventStatusType EventStatusNew
);

/*============================================================================*
 * Event Entry Type
 *============================================================================*/
/**
 * @brief Event Entry (Internal State)
 */
typedef struct {
    Dem_EventIdType eventId;
    Dem_EventStatusType eventStatus;
    Dem_UdsStatusByteType dtcStatus;
    sint8 debounceCounter;
    uint32_t occurrenceCounter;
    uint32_t faultDetectionCounter;
    uint16_t failureCounter;
    uint32_t lastReportTimestamp;
    boolean isAvailable;
    boolean isEnabled;
    boolean isSuppressed;
    boolean isActive;
    const Dem_EventConfigType* config;
} Dem_EventEntryType;

/*============================================================================*
 * Function Prototypes
 *============================================================================*/
/**
 * @brief Initialize event management
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_EventInit(void);

/**
 * @brief Set event status
 * @param EventId The event ID
 * @param EventStatus The new event status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_SetEventStatus(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus
);

/**
 * @brief Get event status
 * @param EventId The event ID
 * @param EventStatusExtended Pointer to store the extended status (UDS status byte)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType EventId,
    Dem_UdsStatusByteType* EventStatusExtended
);

/**
 * @brief Reset event status
 * @param EventId The event ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId);

/**
 * @brief Get event debounce status
 * @param EventId The event ID
 * @param DebounceStatus Pointer to store the debounce status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetEventDebouncingStatus(
    Dem_EventIdType EventId,
    Dem_DebounceStateType* DebounceStatus
);

/**
 * @brief Get event failed status
 * @param EventId The event ID
 * @param EventFailed Pointer to store the failed status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetEventFailed(
    Dem_EventIdType EventId,
    boolean* EventFailed
);

/**
 * @brief Get event tested status
 * @param EventId The event ID
 * @param EventTested Pointer to store the tested status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetEventTested(
    Dem_EventIdType EventId,
    boolean* EventTested
);

/**
 * @brief Get the number of event entries
 * @return Number of active event entries
 */
extern uint16_t Dem_GetNumberOfEvents(void);

/**
 * @brief Find event entry by ID
 * @param EventId The event ID to find
 * @return Pointer to event entry, or NULL if not found
 */
extern Dem_EventEntryType* Dem_FindEventEntry(Dem_EventIdType EventId);

/**
 * @brief Process debounce for counter-based algorithm
 * @param eventEntry Pointer to event entry
 * @param EventStatus The reported event status
 */
extern void Dem_ProcessDebounceCounter(
    Dem_EventEntryType* eventEntry,
    Dem_EventStatusType EventStatus
);

/**
 * @brief Process debounce for time-based algorithm
 * @param eventEntry Pointer to event entry
 * @param EventStatus The reported event status
 */
extern void Dem_ProcessDebounceTime(
    Dem_EventEntryType* eventEntry,
    Dem_EventStatusType EventStatus
);

/**
 * @brief Update DTC status based on event status change
 * @param eventEntry Pointer to event entry
 * @param EventStatusNew The new event status
 */
extern void Dem_UpdateDtcStatus(
    Dem_EventEntryType* eventEntry,
    Dem_EventStatusType EventStatusNew
);

/*============================================================================*
 * Callback Registration Functions
 *============================================================================*/
/**
 * @brief Register event status changed callback
 * @param callback Callback function pointer
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_RegisterEventStatusChangedCallback(
    Dem_EventStatusChangedCallbackType callback
);

/**
 * @brief Unregister event status changed callback
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_UnregisterEventStatusChangedCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* DEM_EVENT_H */
