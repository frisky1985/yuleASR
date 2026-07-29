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
 * @file dem_dtc.c
 * @brief DEM DTC Management Implementation
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "dem_dtc.h"
#include "dem_event.h"
#include <string.h>

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*============================================================================*
 * Internal Data
 *============================================================================*/
static Dem_DtcEntryType s_dtcEntries[DEM_MAX_DTCS];
static uint16_t s_dtcCount = 0U;
static boolean s_dtcInitialized = FALSE;

/* DTC Filter */
static Dem_DtcFilterType s_dtcFilter;

/* Status availability mask - all bits available */
static const uint8_t s_dtcStatusAvailabilityMask = 0xFFU;

/*============================================================================*
 * Static Helper Functions
 *============================================================================*/
/**
 * @brief Get free DTC entry
 */
static Dem_DtcEntryType* Dem_GetFreeDtcEntry(void)
{
    Dem_DtcEntryType* entry = NULL_PTR;
    
    for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
        if (s_dtcEntries[i].dtcCode == 0U) {
            entry = &s_dtcEntries[i];
            break;
        }
    }
    
    return entry;
}

/**
 * @brief Initialize DTC entry
 */
static void Dem_InitDtcEntry(Dem_DtcEntryType* entry)
{
    if (entry != NULL_PTR) {
        entry->dtcCode = 0U;
        entry->dtcStatus = DEM_UDS_STATUS_TNCSLC | DEM_UDS_STATUS_TNCTOC;
        entry->dtcSeverity = DEM_SEVERITY_NO_SEVERITY;
        entry->dtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
        entry->functionalUnit = 0U;
        entry->occurrenceCounter = 0U;
        entry->agingCounter = 0U;
        entry->isSuppressed = FALSE;
        entry->isEnabled = TRUE;
        entry->config = NULL_PTR;
    }
}

/**
 * @brief Check if DTC matches filter criteria
 */
static boolean Dem_DtcMatchesFilter(const Dem_DtcEntryType* entry)
{
    if (entry == NULL_PTR) {
        return FALSE;
    }
    
    /* Check if DTC code is valid */
    if (entry->dtcCode == 0U) {
        return FALSE;
    }
    
    /* Check if suppressed */
    if (entry->isSuppressed == TRUE) {
        return FALSE;
    }
    
    /* Check origin */
    if (s_dtcFilter.dtcOrigin != entry->dtcOrigin) {
        return FALSE;
    }
    
    /* Check status mask */
    if ((entry->dtcStatus & s_dtcFilter.statusMask) == 0U) {
        return FALSE;
    }
    
    /* Check DTC kind (emission related) */
    if (s_dtcFilter.dtcKind == DEM_DTC_KIND_EMISSION_REL_DTCS) {
        /* DTCs 0x00xxxx are emission related */
        if ((entry->dtcCode & 0xFF0000U) != 0U) {
            return FALSE;
        }
    }
    
    return TRUE;
}

/**
 * @brief Check if DTC matches group
 */
static boolean Dem_DtcMatchesGroup(uint32_t dtcCode, uint32_t dtcGroup)
{
    if (dtcGroup == DEM_DTC_GROUP_ALL) {
        return TRUE;
    }
    
    /* Check group mask */
    uint8_t groupPrefix = (uint8_t)((dtcGroup >> 16) & 0xFFU);
    uint8_t dtcPrefix = (uint8_t)((dtcCode >> 16) & 0xFFU);
    
    return (dtcPrefix == groupPrefix);
}

/*============================================================================*
 * Public Functions
 *============================================================================*/
Std_ReturnType Dem_DtcInit(void)
{
    Std_ReturnType result = E_OK;
    
    /* Initialize all DTC entries */
    for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
        Dem_InitDtcEntry(&s_dtcEntries[i]);
    }
    
    /* Initialize filter */
    s_dtcFilter.statusMask = 0x00U;
    s_dtcFilter.dtcKind = DEM_DTC_KIND_ALL_DTCS;
    s_dtcFilter.dtcFormat = DEM_DTC_FORMAT_UDS;
    s_dtcFilter.dtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
    s_dtcFilter.filterIsSet = FALSE;
    s_dtcFilter.currentIndex = 0U;
    
    s_dtcCount = 0U;
    s_dtcInitialized = TRUE;
    
    return result;
}

Std_ReturnType Dem_GetStatusOfDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_UdsStatusByteType* DTCStatus)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (DTCStatus == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (s_dtcInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if ((entry != NULL_PTR) && (entry->dtcOrigin == DTCOrigin)) {
        *DTCStatus = entry->dtcStatus;
        result = E_OK;
    }
    else {
        *DTCStatus = 0U;
    }
    
    return result;
}

Std_ReturnType Dem_GetSeverityOfDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_DTCSeverityType* DTCSeverity)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (DTCSeverity == NULL_PTR) {
        return E_NOT_OK;
    }
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if ((entry != NULL_PTR) && (entry->dtcOrigin == DTCOrigin)) {
        *DTCSeverity = entry->dtcSeverity;
        result = E_OK;
    }
    else {
        *DTCSeverity = DEM_SEVERITY_NO_SEVERITY;
    }
    
    return result;
}

Std_ReturnType Dem_GetDTCOfEvent(
    Dem_EventIdType EventId,
    Dem_DTCFormatType DTCFormat,
    uint32_t* DTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (DTC == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Find event entry and get its DTC */
    Dem_EventEntryType* eventEntry = Dem_FindEventEntry(EventId);
    
    if ((eventEntry != NULL_PTR) && (eventEntry->config != NULL_PTR)) {
        *DTC = eventEntry->config->dtcCode;
        result = E_OK;
    }
    else {
        *DTC = 0U;
    }
    
    (void)DTCFormat; /* Unused - all formats represent same DTC value */
    
    return result;
}

Std_ReturnType Dem_SetDTCFilter(
    uint8_t DTCStatusMask,
    Dem_DTCKindType DTCKind,
    Dem_DTCFormatType DTCFormat,
    Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result = E_OK;
    
    if (s_dtcInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    /* Set filter criteria */
    s_dtcFilter.statusMask = DTCStatusMask;
    s_dtcFilter.dtcKind = DTCKind;
    s_dtcFilter.dtcFormat = DTCFormat;
    s_dtcFilter.dtcOrigin = DTCOrigin;
    s_dtcFilter.filterIsSet = TRUE;
    s_dtcFilter.currentIndex = 0U;
    
    return result;
}

Std_ReturnType Dem_GetNumberOfFilteredDTC(uint16_t* NumberOfFilteredDTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (NumberOfFilteredDTC == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (s_dtcInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    if (s_dtcFilter.filterIsSet == FALSE) {
        return E_NOT_OK;
    }
    
    /* Count matching DTCs */
    uint16_t count = 0U;
    for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
        if (Dem_DtcMatchesFilter(&s_dtcEntries[i]) == TRUE) {
            count++;
        }
    }
    
    *NumberOfFilteredDTC = count;
    result = E_OK;
    
    return result;
}

Std_ReturnType Dem_GetNextFilteredDTC(
    uint32_t* DTC,
    Dem_UdsStatusByteType* DTCStatus)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((DTC == NULL_PTR) || (DTCStatus == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (s_dtcFilter.filterIsSet == FALSE) {
        return E_NOT_OK;
    }
    
    /* Search for next matching DTC */
    while (s_dtcFilter.currentIndex < DEM_MAX_DTCS) {
        Dem_DtcEntryType* entry = &s_dtcEntries[s_dtcFilter.currentIndex];
        s_dtcFilter.currentIndex++;
        
        if (Dem_DtcMatchesFilter(entry) == TRUE) {
            *DTC = entry->dtcCode;
            *DTCStatus = entry->dtcStatus;
            result = E_OK;
            break;
        }
    }
    
    return result;
}

Std_ReturnType Dem_GetNextFilteredDTCAndSeverity(
    uint32_t* DTC,
    Dem_UdsStatusByteType* DTCStatus,
    Dem_DTCSeverityType* DTCSeverity,
    uint8_t* DTCFunctionalUnit)
{
    Std_ReturnType result = Dem_GetNextFilteredDTC(DTC, DTCStatus);
    
    if (result == E_OK) {
        Dem_DtcEntryType* entry = Dem_FindDtcEntry(*DTC);
        
        if (entry != NULL_PTR) {
            if (DTCSeverity != NULL_PTR) {
                *DTCSeverity = entry->dtcSeverity;
            }
            if (DTCFunctionalUnit != NULL_PTR) {
                *DTCFunctionalUnit = entry->functionalUnit;
            }
        }
    }
    
    return result;
}

Std_ReturnType Dem_ClearDTC(
    uint32_t DTC,
    Dem_DTCFormatType DTCFormat,
    Dem_DTCOriginType DTCOrigin)
{
    Std_ReturnType result = E_OK;
    
    if (s_dtcInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    (void)DTCFormat; /* Unused parameter */
    
    if (DTC == DEM_DTC_GROUP_ALL) {
        /* Clear all DTCs for the specified origin */
        for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
            if ((s_dtcEntries[i].dtcCode != 0U) &&
                (s_dtcEntries[i].dtcOrigin == DTCOrigin)) {
                Dem_InitDtcEntry(&s_dtcEntries[i]);
            }
        }
        s_dtcCount = 0U;
    }
    else {
        /* Clear specific DTC */
        Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
        
        if ((entry != NULL_PTR) && (entry->dtcOrigin == DTCOrigin)) {
            Dem_InitDtcEntry(entry);
            if (s_dtcCount > 0U) {
                s_dtcCount--;
            }
        }
        else {
            result = E_NOT_OK;
        }
    }
    
    return result;
}

Std_ReturnType Dem_DisableDTCRecording(uint32_t DTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if (entry != NULL_PTR) {
        entry->isEnabled = FALSE;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_EnableDTCRecording(uint32_t DTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if (entry != NULL_PTR) {
        entry->isEnabled = TRUE;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_DisableDTCSuppression(uint32_t DTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if (entry != NULL_PTR) {
        entry->isSuppressed = FALSE;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_EnableDTCSuppression(uint32_t DTC)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(DTC);
    
    if (entry != NULL_PTR) {
        entry->isSuppressed = TRUE;
        result = E_OK;
    }
    
    return result;
}

Dem_DtcEntryType* Dem_FindDtcEntry(uint32_t dtcCode)
{
    Dem_DtcEntryType* entry = NULL_PTR;
    
    for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
        if (s_dtcEntries[i].dtcCode == dtcCode) {
            entry = &s_dtcEntries[i];
            break;
        }
    }
    
    return entry;
}

Std_ReturnType Dem_UpdateDtcEntry(
    uint32_t dtcCode,
    Dem_UdsStatusByteType status)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_dtcInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    Dem_DtcEntryType* entry = Dem_FindDtcEntry(dtcCode);
    
    if (entry == NULL_PTR) {
        /* Create new entry */
        entry = Dem_GetFreeDtcEntry();
        if (entry != NULL_PTR) {
            entry->dtcCode = dtcCode;
            entry->dtcStatus = status;
            entry->dtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;
            s_dtcCount++;
            result = E_OK;
        }
    }
    else {
        /* Update existing entry */
        entry->dtcStatus = status;
        result = E_OK;
    }
    
    return result;
}

uint16_t Dem_GetNumberOfActiveDTCs(void)
{
    uint16_t count = 0U;
    
    for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
        if ((s_dtcEntries[i].dtcCode != 0U) &&
            ((s_dtcEntries[i].dtcStatus & DEM_UDS_STATUS_CDTC) != 0U)) {
            count++;
        }
    }
    
    return count;
}

void Dem_ClearAllDTCs(void)
{
    for (uint16_t i = 0U; i < DEM_MAX_DTCS; i++) {
        Dem_InitDtcEntry(&s_dtcEntries[i]);
    }
    s_dtcCount = 0U;
}

uint8_t Dem_GetDTCStatusAvailabilityMask(void)
{
    return s_dtcStatusAvailabilityMask;
}

Dem_DTCSeverityType Dem_GetDTCSeverityAvailabilityMask(void)
{
    return (Dem_DTCSeverityType)(DEM_SEVERITY_MAINTENANCE_ONLY | 
                                  DEM_SEVERITY_CHECK_AT_NEXT_HALT | 
                                  DEM_SEVERITY_CHECK_IMMEDIATELY);
}
