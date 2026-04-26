/** @file Dem.h
 * @brief Diagnostic Event Manager (Dem) interface header
 * @details AUTOSAR R22-11 compliant Dem module for WdgM error reporting
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_H
#define DEM_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Dem Event IDs for WdgM (generated from configuration)
 *=============================================================================*/
#ifndef DEM_EVENT_ID_WDGM_ALIVE_SUPERVISION
#define DEM_EVENT_ID_WDGM_ALIVE_SUPERVISION     0x1001U
#endif

#ifndef DEM_EVENT_ID_WDGM_DEADLINE_SUPERVISION
#define DEM_EVENT_ID_WDGM_DEADLINE_SUPERVISION  0x1002U
#endif

#ifndef DEM_EVENT_ID_WDGM_LOGICAL_SUPERVISION
#define DEM_EVENT_ID_WDGM_LOGICAL_SUPERVISION   0x1003U
#endif

#ifndef DEM_EVENT_ID_WDGM_WATCHDOG_RESET
#define DEM_EVENT_ID_WDGM_WATCHDOG_RESET        0x1004U
#endif

#ifndef DEM_EVENT_ID_WDGM_SET_MODE_FAILED
#define DEM_EVENT_ID_WDGM_SET_MODE_FAILED       0x1005U
#endif

/*=============================================================================
 * Dem Event Status Types
 *=============================================================================*/
typedef uint8 Dem_EventStatusType;

#define DEM_EVENT_STATUS_PASSED                 0x00U
#define DEM_EVENT_STATUS_FAILED                 0x01U
#define DEM_EVENT_STATUS_PREPASSED              0x02U
#define DEM_EVENT_STATUS_PREFAILED              0x03U

/*=============================================================================
 * Function Prototypes
 *=============================================================================*/
/**
 * @brief Reports the status of a diagnostic event
 * @param EventId The event ID
 * @param EventStatus The new status of the event
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Dem_ReportErrorStatus(
    uint16 EventId,
    Dem_EventStatusType EventStatus
);

/**
 * @brief Gets the event status
 * @param EventId The event ID
 * @param EventStatusExtended Pointer to store the extended status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Dem_GetEventStatus(
    uint16 EventId,
    uint8* EventStatusExtended
);

/**
 * @brief Resets the event status
 * @param EventId The event ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType Dem_ResetEventStatus(uint16 EventId);

#ifdef __cplusplus
}
#endif

#endif /* DEM_H */
