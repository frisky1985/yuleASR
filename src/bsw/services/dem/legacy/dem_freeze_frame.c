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
 * @file dem_freeze_frame.c
 * @brief DEM Freeze Frame Management Implementation
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "dem_freeze_frame.h"
#include "dem_dtc.h"
#include "dem.h"
#include <string.h>

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*============================================================================*
 * Internal Data
 *============================================================================*/
static Dem_FreezeFrameEntryType s_freezeFrameEntries[DEM_MAX_FREEZE_FRAME_RECORDS];
static Dem_ExtendedDataRecordType s_extendedDataEntries[DEM_MAX_FREEZE_FRAME_RECORDS];
static boolean s_freezeFrameInitialized = FALSE;

/*============================================================================*
 * Static Helper Functions
 *============================================================================*/
/**
 * @brief Get free freeze frame entry
 */
static Dem_FreezeFrameEntryType* Dem_GetFreeFreezeFrameEntry(void)
{
    Dem_FreezeFrameEntryType* entry = NULL_PTR;
    
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if (s_freezeFrameEntries[i].isValid == FALSE) {
            entry = &s_freezeFrameEntries[i];
            break;
        }
    }
    
    return entry;
}

/**
 * @brief Get oldest freeze frame entry for replacement
 */
static Dem_FreezeFrameEntryType* Dem_GetOldestFreezeFrameEntry(void)
{
    Dem_FreezeFrameEntryType* oldest = NULL_PTR;
    uint32_t oldestTime = 0xFFFFFFFFU;
    
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if (s_freezeFrameEntries[i].isValid == TRUE) {
            if (s_freezeFrameEntries[i].timestamp < oldestTime) {
                oldestTime = s_freezeFrameEntries[i].timestamp;
                oldest = &s_freezeFrameEntries[i];
            }
        }
    }
    
    return oldest;
}

/**
 * @brief Initialize freeze frame entry
 */
static void Dem_InitFreezeFrameEntry(Dem_FreezeFrameEntryType* entry)
{
    if (entry != NULL_PTR) {
        entry->dtcCode = 0U;
        entry->recordNumber = DEM_FREEZE_FRAME_RECORD_NUMBER_INVALID;
        entry->timestamp = 0U;
        entry->dataSize = 0U;
        entry->isValid = FALSE;
        (void)memset(entry->data, 0, DEM_MAX_FREEZE_FRAME_SIZE);
    }
}

/**
 * @brief Initialize extended data entry
 */
static void Dem_InitExtendedDataEntry(Dem_ExtendedDataRecordType* entry)
{
    if (entry != NULL_PTR) {
        entry->recordNumber = DEM_EXTENDED_DATA_RECORD_NUMBER_INVALID;
        entry->dataSize = 0U;
        entry->timestamp = 0U;
        entry->recordValid = FALSE;
        (void)memset(entry->data, 0, 128U);
    }
}

/*============================================================================*
 * Public Functions
 *============================================================================*/
Std_ReturnType Dem_FreezeFrameInit(void)
{
    Std_ReturnType result = E_OK;
    
    /* Initialize all freeze frame entries */
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        Dem_InitFreezeFrameEntry(&s_freezeFrameEntries[i]);
        Dem_InitExtendedDataEntry(&s_extendedDataEntries[i]);
    }
    
    s_freezeFrameInitialized = TRUE;
    
    return result;
}

Std_ReturnType Dem_StoreFreezeFrame(
    uint32_t dtcCode,
    Dem_FreezeFrameRecordNumberType recordNumber,
    const uint8_t* data,
    uint16_t dataSize)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_freezeFrameInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    if ((data == NULL_PTR) || (dataSize == 0U) || (dataSize > DEM_MAX_FREEZE_FRAME_SIZE)) {
        return E_NOT_OK;
    }
    
    /* Check if DTC exists */
    Dem_DtcEntryType* dtcEntry = Dem_FindDtcEntry(dtcCode);
    if (dtcEntry == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Check if freeze frame already exists for this DTC/record */
    Dem_FreezeFrameEntryType* entry = Dem_FindFreezeFrameEntry(dtcCode, recordNumber);
    
    if (entry == NULL_PTR) {
        /* Get free entry or replace oldest */
        entry = Dem_GetFreeFreezeFrameEntry();
        
        if (entry == NULL_PTR) {
            /* No free entry - replace oldest */
            entry = Dem_GetOldestFreezeFrameEntry();
        }
    }
    
    if (entry != NULL_PTR) {
        /* Store freeze frame data */
        entry->dtcCode = dtcCode;
        entry->recordNumber = recordNumber;
        entry->timestamp = Dem_GetCurrentTimestamp();
        entry->dataSize = dataSize;
        (void)memcpy(entry->data, data, dataSize);
        entry->isValid = TRUE;
        
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetFreezeFrameDataByDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_FreezeFrameRecordNumberType RecordNumber,
    uint8_t* DestBuffer,
    uint16_t* BufSize)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((DestBuffer == NULL_PTR) || (BufSize == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (s_freezeFrameInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    (void)DTCOrigin; /* Unused in this implementation */
    
    Dem_FreezeFrameEntryType* entry = Dem_FindFreezeFrameEntry(DTC, RecordNumber);
    
    if ((entry != NULL_PTR) && (entry->isValid == TRUE)) {
        uint16_t copySize = entry->dataSize;
        
        if (copySize > *BufSize) {
            copySize = *BufSize;
        }
        
        (void)memcpy(DestBuffer, entry->data, copySize);
        *BufSize = copySize;
        result = E_OK;
    }
    else {
        *BufSize = 0U;
    }
    
    return result;
}

Std_ReturnType Dem_GetSizeOfFreezeFrameSelection(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_FreezeFrameRecordNumberType RecordNumber,
    uint16_t* SizeOfFreezeFrame)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (SizeOfFreezeFrame == NULL_PTR) {
        return E_NOT_OK;
    }
    
    (void)DTCOrigin; /* Unused parameter */
    
    Dem_FreezeFrameEntryType* entry = Dem_FindFreezeFrameEntry(DTC, RecordNumber);
    
    if ((entry != NULL_PTR) && (entry->isValid == TRUE)) {
        *SizeOfFreezeFrame = entry->dataSize;
        result = E_OK;
    }
    else {
        *SizeOfFreezeFrame = 0U;
    }
    
    return result;
}

Std_ReturnType Dem_GetOBDFreezeFrameData(
    uint8_t PID,
    uint8_t* DestBuffer,
    uint16_t* BufSize)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((DestBuffer == NULL_PTR) || (BufSize == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    /* OBD freeze frame is stored with record number 0 */
    /* Find OBD freeze frame (typically for DTC P0xxx) */
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if ((s_freezeFrameEntries[i].isValid == TRUE) &&
            (s_freezeFrameEntries[i].recordNumber == DEM_OBD_FREEZE_FRAME_RECORD_NUMBER)) {
            
            /* PID data is typically at specific offsets in the freeze frame */
            /* Simplified: return the entire freeze frame */
            uint16_t copySize = s_freezeFrameEntries[i].dataSize;
            
            if (copySize > *BufSize) {
                copySize = *BufSize;
            }
            
            (void)memcpy(DestBuffer, s_freezeFrameEntries[i].data, copySize);
            *BufSize = copySize;
            result = E_OK;
            break;
        }
    }
    
    (void)PID; /* PID would be used to extract specific data from freeze frame */
    
    return result;
}

Std_ReturnType Dem_DeleteFreezeFrame(
    uint32_t dtcCode,
    Dem_FreezeFrameRecordNumberType recordNumber)
{
    Std_ReturnType result = E_NOT_OK;
    
    Dem_FreezeFrameEntryType* entry = Dem_FindFreezeFrameEntry(dtcCode, recordNumber);
    
    if (entry != NULL_PTR) {
        Dem_InitFreezeFrameEntry(entry);
        result = E_OK;
    }
    
    return result;
}

void Dem_ClearAllFreezeFrames(void)
{
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        Dem_InitFreezeFrameEntry(&s_freezeFrameEntries[i]);
    }
}

Dem_FreezeFrameEntryType* Dem_FindFreezeFrameEntry(
    uint32_t dtcCode,
    Dem_FreezeFrameRecordNumberType recordNumber)
{
    Dem_FreezeFrameEntryType* entry = NULL_PTR;
    
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if ((s_freezeFrameEntries[i].isValid == TRUE) &&
            (s_freezeFrameEntries[i].dtcCode == dtcCode) &&
            (s_freezeFrameEntries[i].recordNumber == recordNumber)) {
            entry = &s_freezeFrameEntries[i];
            break;
        }
    }
    
    return entry;
}

Std_ReturnType Dem_CaptureFreezeFrameData(Dem_FreezeFrameEntryType* entry)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (entry == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Capture typical freeze frame data */
    /* This would typically read from various sensors/modules */
    
    /* Simplified: Fill with dummy data representing typical freeze frame contents */
    /* Byte 0-1: Engine RPM */
    entry->data[0] = 0x1A;  /* High byte */
    entry->data[1] = 0x0A;  /* Low byte (approx 6650 RPM) */
    
    /* Byte 2: Vehicle Speed */
    entry->data[2] = 0x50;  /* 80 km/h */
    
    /* Byte 3: Coolant Temperature */
    entry->data[3] = 0x5A;  /* 90 degrees C (0x5A = 90) */
    
    /* Byte 4-5: Intake Air Pressure */
    entry->data[4] = 0x03;
    entry->data[5] = 0xE8;  /* 1000 hPa */
    
    entry->dataSize = 6U;
    result = E_OK;
    
    return result;
}

uint8_t Dem_GetNumberOfFreezeFrames(void)
{
    uint8_t count = 0U;
    
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if (s_freezeFrameEntries[i].isValid == TRUE) {
            count++;
        }
    }
    
    return count;
}

/*============================================================================*
 * Extended Data Record Functions
 *============================================================================*/
Std_ReturnType Dem_StoreExtendedDataRecord(
    uint32_t dtcCode,
    Dem_ExtendedDataRecordNumberType recordNumber,
    const uint8_t* data,
    uint16_t dataSize)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((data == NULL_PTR) || (dataSize == 0U) || (dataSize > 128U)) {
        return E_NOT_OK;
    }
    
    /* Find existing entry or free slot */
    Dem_ExtendedDataRecordType* entry = NULL_PTR;
    
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if ((s_extendedDataEntries[i].recordValid == TRUE) &&
            (s_extendedDataEntries[i].recordNumber == recordNumber)) {
            entry = &s_extendedDataEntries[i];
            break;
        }
    }
    
    if (entry == NULL_PTR) {
        /* Find free slot */
        for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
            if (s_extendedDataEntries[i].recordValid == FALSE) {
                entry = &s_extendedDataEntries[i];
                break;
            }
        }
    }
    
    if (entry != NULL_PTR) {
        entry->recordNumber = recordNumber;
        entry->dataSize = dataSize;
        entry->timestamp = Dem_GetCurrentTimestamp();
        (void)memcpy(entry->data, data, dataSize);
        entry->recordValid = TRUE;
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType Dem_GetExtendedDataRecordByDTC(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ExtendedDataRecordNumberType ExtendedDataNumber,
    uint8_t* DestBuffer,
    uint16_t* BufSize)
{
    Std_ReturnType result = E_NOT_OK;
    
    if ((DestBuffer == NULL_PTR) || (BufSize == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    (void)DTC;        /* Would be used to find DTC-specific extended data */
    (void)DTCOrigin;  /* Unused parameter */
    
    Dem_ExtendedDataRecordType* entry = NULL_PTR;
    
    /* Find extended data record by number */
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if ((s_extendedDataEntries[i].recordValid == TRUE) &&
            (s_extendedDataEntries[i].recordNumber == ExtendedDataNumber)) {
            entry = &s_extendedDataEntries[i];
            break;
        }
    }
    
    if (entry != NULL_PTR) {
        uint16_t copySize = entry->dataSize;
        
        if (copySize > *BufSize) {
            copySize = *BufSize;
        }
        
        (void)memcpy(DestBuffer, entry->data, copySize);
        *BufSize = copySize;
        result = E_OK;
    }
    else {
        *BufSize = 0U;
    }
    
    return result;
}

Std_ReturnType Dem_GetSizeOfExtendedDataRecordSelection(
    uint32_t DTC,
    Dem_DTCOriginType DTCOrigin,
    Dem_ExtendedDataRecordNumberType ExtendedDataNumber,
    uint16_t* SizeOfExtendedDataRecord)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (SizeOfExtendedDataRecord == NULL_PTR) {
        return E_NOT_OK;
    }
    
    (void)DTC;
    (void)DTCOrigin;
    
    Dem_ExtendedDataRecordType* entry = NULL_PTR;
    
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        if ((s_extendedDataEntries[i].recordValid == TRUE) &&
            (s_extendedDataEntries[i].recordNumber == ExtendedDataNumber)) {
            entry = &s_extendedDataEntries[i];
            break;
        }
    }
    
    if (entry != NULL_PTR) {
        *SizeOfExtendedDataRecord = entry->dataSize;
        result = E_OK;
    }
    else {
        *SizeOfExtendedDataRecord = 0U;
    }
    
    return result;
}

void Dem_ClearAllExtendedDataRecords(void)
{
    for (uint8_t i = 0U; i < DEM_MAX_FREEZE_FRAME_RECORDS; i++) {
        Dem_InitExtendedDataEntry(&s_extendedDataEntries[i]);
    }
}
