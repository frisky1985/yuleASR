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
 * @file dem.h
 * @brief DEM (Diagnostic Event Manager) Main Module Interface
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_H
#define DEM_H

#ifdef __cplusplus
extern "C" {


/*==================================================================================================
 *                                      ADDITIONAL API DECLARATIONS
 * CRITICAL FIX: Added missing AUTOSAR standard APIs
==================================================================================================*/

/**
 * rief   Resets the event status of an event
 * \param   EventId: Identification of an event by configured EventId
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 * \post    Event status reset to passed
 */
extern Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId);

/**
 * rief   Gets the current status of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   EventStatusExtended: Pointer to receive the extended event status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType EventId,
    Dem_EventStatusExtendedType* EventStatusExtended
);

/**
 * rief   Gets the UDS status byte of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   UDSStatusByte: Pointer to receive the UDS status byte
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventUdsStatus(
    Dem_EventIdType EventId,
    Dem_UdsStatusByteType* UDSStatusByte
);

/**
 * rief   Gets the DTC for a given event
 * \param   EventId: Identification of an event by configured EventId
 * \param   DTCOfEvent: Pointer to receive the DTC value
 * \param   DTCFormat: Format of the DTC (OBD/UDS)
 * \param   DTCOrigin: Origin of the DTC (Primary/Mirror/Permanent)
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetDTCOfEvent(
    Dem_EventIdType EventId,
    Dem_DTCFormatType DTCFormat,
    uint32* DTCOfEvent,
    Dem_DTCOriginType* DTCOrigin
);

/**
 * rief   Disables the DTC record update
 * \param   DTC: Diagnostic Trouble Code
 * \param   DTCOrigin: Origin of the DTC
 * \param   ClientId: Unique client ID
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_DisableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId
);

/**
 * rief   Enables the DTC record update
 * \param   DTC: Diagnostic Trouble Code
 * \param   DTCOrigin: Origin of the DTC
 * \param   ClientId: Unique client ID
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_EnableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId
);

/**
 * rief   Sets an enable condition
 * \param   EnableCondition: Identification of the enable condition
 * \param   ConditionFulfilled: TRUE if condition is fulfilled, FALSE otherwise
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetEnableCondition(
    uint8 EnableCondition,
    boolean ConditionFulfilled
);

/**
 * rief   Sets a storage condition
 * \param   StorageCondition: Identification of the storage condition
 * \param   ConditionFulfilled: TRUE if condition is fulfilled, FALSE otherwise
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetStorageCondition(
    uint8 StorageCondition,
    boolean ConditionFulfilled
);

/**
 * rief   Gets the number of filtered DTCs
 * \param   ClientId: Unique client ID
 * \param   NumberOfFilteredDTC: Pointer to receive the number of filtered DTCs
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized, filter set
 */
extern Std_ReturnType Dem_GetNumberOfFilteredDTC(
    Dem_ClientIdType ClientId,
    uint16* NumberOfFilteredDTC
);

/**
 * rief   Gets the next filtered DTC
 * \param   ClientId: Unique client ID
 * \param   DTC: Pointer to receive the DTC value
 * \param   DTCStatus: Pointer to receive the DTC status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK if no more DTCs
 * \pre     Dem initialized, filter set
 */
extern Std_ReturnType Dem_GetNextFilteredDTC(
    Dem_ClientIdType ClientId,
    uint32* DTC,
    Dem_DTCStatusMaskType* DTCStatus
);

/**
 * rief   Sets the operation cycle state
 * \param   OperationCycleId: Identification of the operation cycle
 * \param   CycleState: New state of the operation cycle (START/END)
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType CycleState
);

/**
 * rief   Gets the operation cycle state
 * \param   OperationCycleId: Identification of the operation cycle
 * \param   CycleState: Pointer to receive the current cycle state
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType* CycleState
);

/**
 * rief   Restarts the operation cycle
 * \param   OperationCycleId: Identification of the operation cycle
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_RestartOperationCycle(
    Dem_OperationCycleIdType OperationCycleId
);

/**
 * rief   Gets the debouncing status of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   DebouncingState: Pointer to receive the debouncing state
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetDebouncingOfEvent(
    Dem_EventIdType EventId,
    Dem_DebouncingStateType* DebouncingState
);

/**
 * rief   Sets the indicator status
 * \param   IndicatorId: Identification of the indicator
 * \param   IndicatorStatus: New status of the indicator
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetIndicatorStatus(
    uint8 IndicatorId,
    Dem_IndicatorStatusType IndicatorStatus
);

/**
 * rief   Gets the indicator status
 * \param   IndicatorId: Identification of the indicator
 * \param   IndicatorStatus: Pointer to receive the indicator status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetIndicatorStatus(
    uint8 IndicatorId,
    Dem_IndicatorStatusType* IndicatorStatus
);

/**
 * rief   Gets the event memory overflow status
 * \param   DTCOrigin: Origin of the event memory
 * \param   OverflowIndication: Pointer to receive the overflow status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventMemoryOverflow(
    Dem_DTCOriginType DTCOrigin,
    boolean* OverflowIndication
);

/**
 * rief   Gets the number of event memory entries
 * \param   DTCOrigin: Origin of the event memory
 * \param   NumberOfEventMemoryEntries: Pointer to receive the number of entries
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetNumberOfEventMemoryEntries(
    Dem_DTCOriginType DTCOrigin,
    uint8* NumberOfEventMemoryEntries
);

/**
 * rief   Pre-allocated temporary memory for event processing
 * \param   EventId: Identification of an event by configured EventId
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_PreTempActive(Dem_EventIdType EventId);


#endif

/*============================================================================*
 * Includes
 *============================================================================*/
#include "dem_types.h"
#include "dem_event.h"
#include "dem_dtc.h"
#include "dem_freeze_frame.h"
#include "dem_nvm.h"

/*============================================================================*
 * Module Information
 *============================================================================*/
#define DEM_MODULE_NAME                         "Dem"
#define DEM_VENDOR_ID                           0x00U
#define DEM_MODULE_ID                           0x0DU

/*============================================================================*
 * Configuration Constants
 *============================================================================*/
#define DEM_VERSION_INFO_API                    STD_ON
#define DEM_DEV_ERROR_DETECT                    STD_ON
#define DEM_DEBUG_PRINT                         STD_OFF

/*============================================================================*
 * Function Prototypes - Core Module Functions
 *============================================================================*/

/**
 * @brief Initializes the DEM module
 * @details Must be called before using any other DEM services
 * @param ConfigPtr Pointer to DEM configuration (NULL for default)
 * @return E_OK if successful, E_NOT_OK otherwise
 * @pre None
 * @post DEM module initialized and ready for operation
 */
extern Std_ReturnType Dem_Init(const Dem_ConfigType* ConfigPtr);

/**
 * @brief Shuts down the DEM module
 * @details Saves all pending data to non-volatile memory
 * @pre DEM module must be initialized
 * @post DEM module shut down
 */
extern void Dem_Shutdown(void);

/**
 * @brief Gets the version information of the DEM module
 * @param versioninfo Pointer to version info structure
 */
extern void Dem_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Main function - should be called periodically by the BSW scheduler
 * @details Processes debounce counters, handles NvM operations
 * @note Call period typically 10ms
 */
extern void Dem_MainFunction(void);

/**
 * @brief Pre-init function for early initialization
 * @details Can be called before full DEM initialization
 */
extern void Dem_PreInit(void);

/*============================================================================*
 * Operation Cycle Functions
 *============================================================================*/

/**
 * @brief Set operation cycle state
 * @param operationCycle The operation cycle type
 * @param cycleState The new cycle state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_SetOperationCycleState(
    Dem_OperationCycleType operationCycle,
    Dem_OperationCycleStateType cycleState
);

/**
 * @brief Get operation cycle state
 * @param operationCycle The operation cycle type
 * @param cycleState Pointer to store the cycle state
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetOperationCycleState(
    Dem_OperationCycleType operationCycle,
    Dem_OperationCycleStateType* cycleState
);

/*============================================================================*
 * Enable/Disable Control Functions
 *============================================================================*/

/**
 * @brief Disable DTC setting (control DTC setting)
 * @param DTCGroup The DTC group to disable (0xFFFFFF for all)
 * @param ClientId The client ID requesting the disable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_DisableDTCSetting(
    uint32_t DTCGroup,
    uint8_t ClientId
);

/**
 * @brief Enable DTC setting
 * @param DTCGroup The DTC group to enable (0xFFFFFF for all)
 * @param ClientId The client ID requesting the enable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_EnableDTCSetting(
    uint32_t DTCGroup,
    uint8_t ClientId
);

/**
 * @brief Check if DTC setting is enabled
 * @return TRUE if DTC setting is enabled, FALSE otherwise
 */
extern boolean Dem_IsDTCSettingEnabled(void);

/*============================================================================*
 * Indication Status Functions
 *============================================================================*/

/**
 * @brief Get the indicator status
 * @param indicatorId The indicator ID
 * @param indicatorStatus Pointer to store the status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetIndicatorStatus(
    uint8_t indicatorId,
    uint8_t* indicatorStatus
);

/**
 * @brief Get the fault detection counter
 * @param EventId The event ID
 * @param faultDetectionCounter Pointer to store the counter
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetFaultDetectionCounter(
    Dem_EventIdType EventId,
    sint8* faultDetectionCounter
);

/**
 * @brief Get the number of stored freeze frames for an event
 * @param EventId The event ID
 * @param numberOfStoredRecords Pointer to store the count
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetNumberOfStoredFreezeFrames(
    Dem_EventIdType EventId,
    uint8_t* numberOfStoredRecords
);

/*============================================================================*
 * Component API Functions (For other BSW modules)
 *============================================================================*/

/**
 * @brief Report error status (legacy API for compatibility)
 * @param EventId The event ID
 * @param EventStatus The event status
 * @return E_OK if successful, E_NOT_OK otherwise
 * @note This is a wrapper around Dem_SetEventStatus
 */
extern Std_ReturnType Dem_ReportErrorStatus(
    uint16_t EventId,
    Dem_EventStatusType EventStatus
);

/**
 * @brief Report error status with extended parameter
 * @param EventId The event ID
 * @param EventStatus The event status
 * @param debug0 Debug parameter 0
 * @param debug1 Debug parameter 1
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_ReportErrorStatusWithDebug(
    Dem_EventIdType EventId,
    Dem_EventStatusType EventStatus,
    uint8_t debug0,
    uint8_t debug1
);

/*============================================================================*
 * Configuration Functions
 *============================================================================*/

/**
 * @brief Set event available
 * @param EventId The event ID
 * @param available TRUE to set available, FALSE to disable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_SetEventAvailable(
    Dem_EventIdType EventId,
    boolean available
);

/**
 * @brief Set DTC available in output
 * @param DTC The DTC code
 * @param available TRUE to set available, FALSE to disable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_SetDTCAvailableInOutput(
    uint32_t DTC,
    boolean available
);

/*============================================================================*
 * Control Functions
 *============================================================================*/

/**
 * @brief Clear DTC
 * @param DTC The DTC to clear
 * @param DTCFormat The DTC format
 * @param DTCOrigin The DTC origin
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_ClearDTC(
    uint32_t DTC,
    Dem_DTCFormatType DTCFormat,
    Dem_DTCOriginType DTCOrigin
);

/**
 * @brief Reset event confirmed status
 * @param EventId The event ID
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_ResetEventConfirmed(
    Dem_EventIdType EventId
);

/**
 * @brief Restart operation cycle
 * @param operationCycle The operation cycle
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_RestartOperationCycle(
    Dem_OperationCycleType operationCycle
);

/*============================================================================*
 * Diagnostic Information Functions
 *============================================================================*/

/**
 * @brief Get DTC status availability mask
 * @return The status availability mask
 */
extern uint8_t Dem_GetDTCStatusAvailabilityMask(void);

/**
 * @brief Get number of filtered DTC
 * @param NumberOfFilteredDTC Pointer to store the count
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetNumberOfFilteredDTC(
    uint16_t* NumberOfFilteredDTC
);

/**
 * @brief Get next filtered DTC
 * @param DTC Pointer to store the DTC
 * @param DTCStatus Pointer to store the status
 * @return E_OK if successful, E_NOT_OK if no more DTCs
 */
extern Std_ReturnType Dem_GetNextFilteredDTC(
    uint32_t* DTC,
    uint8_t* DTCStatus
);

/**
 * @brief Get DTC by occurrence time
 * @param DTCRequest The request type (first/most recent)
 * @param DTC Pointer to store the DTC
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetDTCByOccurrenceTime(
    Dem_DTCRequestType DTCRequest,
    uint32_t* DTC
);

/*============================================================================*
 * Module State Functions
 *============================================================================*/

/**
 * @brief Check if DEM is initialized
 * @return TRUE if initialized, FALSE otherwise
 */
extern boolean Dem_IsInitialized(void);

/**
 * @brief Get DEM module state
 * @return The current DEM state
 */
extern Dem_StateType Dem_GetState(void);

#ifdef __cplusplus
}


/*==================================================================================================
 *                                      ADDITIONAL API DECLARATIONS
 * CRITICAL FIX: Added missing AUTOSAR standard APIs
==================================================================================================*/

/**
 * rief   Resets the event status of an event
 * \param   EventId: Identification of an event by configured EventId
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 * \post    Event status reset to passed
 */
extern Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId);

/**
 * rief   Gets the current status of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   EventStatusExtended: Pointer to receive the extended event status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType EventId,
    Dem_EventStatusExtendedType* EventStatusExtended
);

/**
 * rief   Gets the UDS status byte of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   UDSStatusByte: Pointer to receive the UDS status byte
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventUdsStatus(
    Dem_EventIdType EventId,
    Dem_UdsStatusByteType* UDSStatusByte
);

/**
 * rief   Gets the DTC for a given event
 * \param   EventId: Identification of an event by configured EventId
 * \param   DTCOfEvent: Pointer to receive the DTC value
 * \param   DTCFormat: Format of the DTC (OBD/UDS)
 * \param   DTCOrigin: Origin of the DTC (Primary/Mirror/Permanent)
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetDTCOfEvent(
    Dem_EventIdType EventId,
    Dem_DTCFormatType DTCFormat,
    uint32* DTCOfEvent,
    Dem_DTCOriginType* DTCOrigin
);

/**
 * rief   Disables the DTC record update
 * \param   DTC: Diagnostic Trouble Code
 * \param   DTCOrigin: Origin of the DTC
 * \param   ClientId: Unique client ID
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_DisableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId
);

/**
 * rief   Enables the DTC record update
 * \param   DTC: Diagnostic Trouble Code
 * \param   DTCOrigin: Origin of the DTC
 * \param   ClientId: Unique client ID
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_EnableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId
);

/**
 * rief   Sets an enable condition
 * \param   EnableCondition: Identification of the enable condition
 * \param   ConditionFulfilled: TRUE if condition is fulfilled, FALSE otherwise
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetEnableCondition(
    uint8 EnableCondition,
    boolean ConditionFulfilled
);

/**
 * rief   Sets a storage condition
 * \param   StorageCondition: Identification of the storage condition
 * \param   ConditionFulfilled: TRUE if condition is fulfilled, FALSE otherwise
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetStorageCondition(
    uint8 StorageCondition,
    boolean ConditionFulfilled
);

/**
 * rief   Gets the number of filtered DTCs
 * \param   ClientId: Unique client ID
 * \param   NumberOfFilteredDTC: Pointer to receive the number of filtered DTCs
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized, filter set
 */
extern Std_ReturnType Dem_GetNumberOfFilteredDTC(
    Dem_ClientIdType ClientId,
    uint16* NumberOfFilteredDTC
);

/**
 * rief   Gets the next filtered DTC
 * \param   ClientId: Unique client ID
 * \param   DTC: Pointer to receive the DTC value
 * \param   DTCStatus: Pointer to receive the DTC status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK if no more DTCs
 * \pre     Dem initialized, filter set
 */
extern Std_ReturnType Dem_GetNextFilteredDTC(
    Dem_ClientIdType ClientId,
    uint32* DTC,
    Dem_DTCStatusMaskType* DTCStatus
);

/**
 * rief   Sets the operation cycle state
 * \param   OperationCycleId: Identification of the operation cycle
 * \param   CycleState: New state of the operation cycle (START/END)
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType CycleState
);

/**
 * rief   Gets the operation cycle state
 * \param   OperationCycleId: Identification of the operation cycle
 * \param   CycleState: Pointer to receive the current cycle state
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType* CycleState
);

/**
 * rief   Restarts the operation cycle
 * \param   OperationCycleId: Identification of the operation cycle
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_RestartOperationCycle(
    Dem_OperationCycleIdType OperationCycleId
);

/**
 * rief   Gets the debouncing status of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   DebouncingState: Pointer to receive the debouncing state
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetDebouncingOfEvent(
    Dem_EventIdType EventId,
    Dem_DebouncingStateType* DebouncingState
);

/**
 * rief   Sets the indicator status
 * \param   IndicatorId: Identification of the indicator
 * \param   IndicatorStatus: New status of the indicator
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetIndicatorStatus(
    uint8 IndicatorId,
    Dem_IndicatorStatusType IndicatorStatus
);

/**
 * rief   Gets the indicator status
 * \param   IndicatorId: Identification of the indicator
 * \param   IndicatorStatus: Pointer to receive the indicator status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetIndicatorStatus(
    uint8 IndicatorId,
    Dem_IndicatorStatusType* IndicatorStatus
);

/**
 * rief   Gets the event memory overflow status
 * \param   DTCOrigin: Origin of the event memory
 * \param   OverflowIndication: Pointer to receive the overflow status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventMemoryOverflow(
    Dem_DTCOriginType DTCOrigin,
    boolean* OverflowIndication
);

/**
 * rief   Gets the number of event memory entries
 * \param   DTCOrigin: Origin of the event memory
 * \param   NumberOfEventMemoryEntries: Pointer to receive the number of entries
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetNumberOfEventMemoryEntries(
    Dem_DTCOriginType DTCOrigin,
    uint8* NumberOfEventMemoryEntries
);

/**
 * rief   Pre-allocated temporary memory for event processing
 * \param   EventId: Identification of an event by configured EventId
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_PreTempActive(Dem_EventIdType EventId);


#endif



/*==================================================================================================
 *                                      ADDITIONAL API DECLARATIONS
 * CRITICAL FIX: Added missing AUTOSAR standard APIs
==================================================================================================*/

/**
 * rief   Resets the event status of an event
 * \param   EventId: Identification of an event by configured EventId
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 * \post    Event status reset to passed
 */
extern Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType EventId);

/**
 * rief   Gets the current status of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   EventStatusExtended: Pointer to receive the extended event status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventStatus(
    Dem_EventIdType EventId,
    Dem_EventStatusExtendedType* EventStatusExtended
);

/**
 * rief   Gets the UDS status byte of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   UDSStatusByte: Pointer to receive the UDS status byte
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventUdsStatus(
    Dem_EventIdType EventId,
    Dem_UdsStatusByteType* UDSStatusByte
);

/**
 * rief   Gets the DTC for a given event
 * \param   EventId: Identification of an event by configured EventId
 * \param   DTCOfEvent: Pointer to receive the DTC value
 * \param   DTCFormat: Format of the DTC (OBD/UDS)
 * \param   DTCOrigin: Origin of the DTC (Primary/Mirror/Permanent)
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetDTCOfEvent(
    Dem_EventIdType EventId,
    Dem_DTCFormatType DTCFormat,
    uint32* DTCOfEvent,
    Dem_DTCOriginType* DTCOrigin
);

/**
 * rief   Disables the DTC record update
 * \param   DTC: Diagnostic Trouble Code
 * \param   DTCOrigin: Origin of the DTC
 * \param   ClientId: Unique client ID
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_DisableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId
);

/**
 * rief   Enables the DTC record update
 * \param   DTC: Diagnostic Trouble Code
 * \param   DTCOrigin: Origin of the DTC
 * \param   ClientId: Unique client ID
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_EnableDTCRecordUpdate(
    uint32 DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ClientIdType ClientId
);

/**
 * rief   Sets an enable condition
 * \param   EnableCondition: Identification of the enable condition
 * \param   ConditionFulfilled: TRUE if condition is fulfilled, FALSE otherwise
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetEnableCondition(
    uint8 EnableCondition,
    boolean ConditionFulfilled
);

/**
 * rief   Sets a storage condition
 * \param   StorageCondition: Identification of the storage condition
 * \param   ConditionFulfilled: TRUE if condition is fulfilled, FALSE otherwise
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetStorageCondition(
    uint8 StorageCondition,
    boolean ConditionFulfilled
);

/**
 * rief   Gets the number of filtered DTCs
 * \param   ClientId: Unique client ID
 * \param   NumberOfFilteredDTC: Pointer to receive the number of filtered DTCs
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized, filter set
 */
extern Std_ReturnType Dem_GetNumberOfFilteredDTC(
    Dem_ClientIdType ClientId,
    uint16* NumberOfFilteredDTC
);

/**
 * rief   Gets the next filtered DTC
 * \param   ClientId: Unique client ID
 * \param   DTC: Pointer to receive the DTC value
 * \param   DTCStatus: Pointer to receive the DTC status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK if no more DTCs
 * \pre     Dem initialized, filter set
 */
extern Std_ReturnType Dem_GetNextFilteredDTC(
    Dem_ClientIdType ClientId,
    uint32* DTC,
    Dem_DTCStatusMaskType* DTCStatus
);

/**
 * rief   Sets the operation cycle state
 * \param   OperationCycleId: Identification of the operation cycle
 * \param   CycleState: New state of the operation cycle (START/END)
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType CycleState
);

/**
 * rief   Gets the operation cycle state
 * \param   OperationCycleId: Identification of the operation cycle
 * \param   CycleState: Pointer to receive the current cycle state
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetOperationCycleState(
    Dem_OperationCycleIdType OperationCycleId,
    Dem_OperationCycleStateType* CycleState
);

/**
 * rief   Restarts the operation cycle
 * \param   OperationCycleId: Identification of the operation cycle
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_RestartOperationCycle(
    Dem_OperationCycleIdType OperationCycleId
);

/**
 * rief   Gets the debouncing status of an event
 * \param   EventId: Identification of an event by configured EventId
 * \param   DebouncingState: Pointer to receive the debouncing state
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetDebouncingOfEvent(
    Dem_EventIdType EventId,
    Dem_DebouncingStateType* DebouncingState
);

/**
 * rief   Sets the indicator status
 * \param   IndicatorId: Identification of the indicator
 * \param   IndicatorStatus: New status of the indicator
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_SetIndicatorStatus(
    uint8 IndicatorId,
    Dem_IndicatorStatusType IndicatorStatus
);

/**
 * rief   Gets the indicator status
 * \param   IndicatorId: Identification of the indicator
 * \param   IndicatorStatus: Pointer to receive the indicator status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetIndicatorStatus(
    uint8 IndicatorId,
    Dem_IndicatorStatusType* IndicatorStatus
);

/**
 * rief   Gets the event memory overflow status
 * \param   DTCOrigin: Origin of the event memory
 * \param   OverflowIndication: Pointer to receive the overflow status
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetEventMemoryOverflow(
    Dem_DTCOriginType DTCOrigin,
    boolean* OverflowIndication
);

/**
 * rief   Gets the number of event memory entries
 * \param   DTCOrigin: Origin of the event memory
 * \param   NumberOfEventMemoryEntries: Pointer to receive the number of entries
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_GetNumberOfEventMemoryEntries(
    Dem_DTCOriginType DTCOrigin,
    uint8* NumberOfEventMemoryEntries
);

/**
 * rief   Pre-allocated temporary memory for event processing
 * \param   EventId: Identification of an event by configured EventId
 * 
eturn  Std_ReturnType: E_OK if operation successful, E_NOT_OK otherwise
 * \pre     Dem initialized
 */
extern Std_ReturnType Dem_PreTempActive(Dem_EventIdType EventId);

/**
 * @brief   Get the current system timestamp (milliseconds)
 * @return  Current timestamp value from the DEM global tick counter
 * @note    The tick counter is incremented each Dem_MainFunction cycle
 */
extern uint32_t Dem_GetCurrentTimestamp(void);

#endif /* DEM_H */
