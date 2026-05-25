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
 * @file dem_dtc.h
 * @brief DEM DTC Management Interface
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef DEM_DTC_H
#define DEM_DTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dem_types.h"

/*============================================================================*
 * Macros and Constants
 *============================================================================*/
#define DEM_MAX_DTCS                            100U
#define DEM_DTC_GROUP_ALL                       0xFFFFFFU
#define DEM_DTC_GROUP_EMISSION                  0x0000FFU
#define DEM_DTC_GROUP_POWERTRAIN                0x0000FFU
#define DEM_DTC_GROUP_CHASSIS                   0x0100FFU
#define DEM_DTC_GROUP_BODY                      0x0200FFU
#define DEM_DTC_GROUP_NETWORK                   0x0300FFU

/*============================================================================*
 * DTC Entry Type
 *============================================================================*/
/**
 * @brief DTC Entry (Internal State)
 */
typedef struct {
    uint32_t dtcCode;
    Dem_UdsStatusByteType dtcStatus;
    Dem_DTCSeverityType dtcSeverity;
    Dem_DTCOriginType dtcOrigin;
    uint8_t functionalUnit;
    uint32_t occurrenceCounter;
    uint32_t agingCounter;
    boolean isSuppressed;
    boolean isEnabled;
    const Dem_DtcConfigType* config;
} Dem_DtcEntryType;

/*============================================================================*
 * Function Prototypes
 *============================================================================*/
/**
 * @brief Initialize DTC management
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_DtcInit(void);

/**
 * @brief Get DTC status
 * @param DTC The DTC code
 * @param DTCOrigin The DTC origin
 * @param DTCStatus Pointer to store the DTC status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetStatusOfDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_UdsStatusByteType* DTCStatus
);

/**
 * @brief Get DTC severity
 * @param DTC The DTC code
 * @param DTCOrigin The DTC origin
 * @param DTCSeverity Pointer to store the DTC severity
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetSeverityOfDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_DTCSeverityType* DTCSeverity
);

/**
 * @brief Get DTC of event
 * @param EventId The event ID
 * @param DTCFormat The DTC format
 * @param DTC Pointer to store the DTC code
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetDTCOfEvent(
    Dem_EventIdType EventId,
    Dem_DTCFormatType DTCFormat,
    uint32_t* DTC
);

/**
 * @brief Set DTC filter
 * @param DTCStatusMask The DTC status mask
 * @param DTCKind The DTC kind
 * @param DTCFormat The DTC format
 * @param DTCOrigin The DTC origin
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_SetDTCFilter(
    uint8_t DTCStatusMask,
    Dem_DTCKindType DTCKind,
    Dem_DTCFormatType DTCFormat,
    Dem_DTCOriginType DTCOrigin
);

/**
 * @brief Get number of filtered DTCs
 * @param NumberOfFilteredDTC Pointer to store the count
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_GetNumberOfFilteredDTC(
    uint16_t* NumberOfFilteredDTC
);

/**
 * @brief Get next filtered DTC
 * @param DTC Pointer to store the DTC code
 * @param DTCStatus Pointer to store the DTC status
 * @return E_OK if successful, E_NOT_OK if no more DTCs
 */
extern Std_ReturnType Dem_GetNextFilteredDTC(
    uint32_t* DTC,
    Dem_UdsStatusByteType* DTCStatus
);

/**
 * @brief Get next filtered DTC with severity
 * @param DTC Pointer to store the DTC code
 * @param DTCStatus Pointer to store the DTC status
 * @param DTCSeverity Pointer to store the DTC severity
 * @param DTCFunctionalUnit Pointer to store the functional unit
 * @return E_OK if successful, E_NOT_OK if no more DTCs
 */
extern Std_ReturnType Dem_GetNextFilteredDTCAndSeverity(
    uint32_t* DTC,
    Dem_UdsStatusByteType* DTCStatus,
    Dem_DTCSeverityType* DTCSeverity,
    uint8_t* DTCFunctionalUnit
);

/**
 * @brief Clear DTC
 * @param DTC The DTC to clear (or group)
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
 * @brief Disable DTC recording
 * @param DTC The DTC to disable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_DisableDTCRecording(uint32_t DTC);

/**
 * @brief Enable DTC recording
 * @param DTC The DTC to enable
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_EnableDTCRecording(uint32_t DTC);

/**
 * @brief Disable DTC suppression
 * @param DTC The DTC
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_DisableDTCSuppression(uint32_t DTC);

/**
 * @brief Enable DTC suppression
 * @param DTC The DTC
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_EnableDTCSuppression(uint32_t DTC);

/**
 * @brief Find DTC entry by code
 * @param dtcCode The DTC code to find
 * @return Pointer to DTC entry, or NULL if not found
 */
extern Dem_DtcEntryType* Dem_FindDtcEntry(uint32_t dtcCode);

/**
 * @brief Add or update DTC entry
 * @param dtcCode The DTC code
 * @param status The DTC status
 * @return E_OK if successful, E_NOT_OK otherwise
 */
extern Std_ReturnType Dem_UpdateDtcEntry(
    uint32_t dtcCode,
    Dem_UdsStatusByteType status
);

/**
 * @brief Get the number of active DTCs
 * @return Number of active DTC entries
 */
extern uint16_t Dem_GetNumberOfActiveDTCs(void);

/**
 * @brief Clear all DTCs
 */
extern void Dem_ClearAllDTCs(void);

/**
 * @brief Get DTC status availability mask
 * @return The status availability mask
 */
extern uint8_t Dem_GetDTCStatusAvailabilityMask(void);

/**
 * @brief Get DTC severity availability mask
 * @return The severity availability mask
 */
extern Dem_DTCSeverityType Dem_GetDTCSeverityAvailabilityMask(void);

#ifdef __cplusplus
}
#endif

#endif /* DEM_DTC_H */
