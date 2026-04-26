/**
 * @file MemIf.c
 * @brief AUTOSAR MemIf (Memory Interface) Module Implementation
 * @version 4.4.0
 * @date 2025
 * 
 * AUTOSAR Classic Platform - MemIf Implementation (Module ID: 0x0F)
 * 
 * This module provides uniform access to memory devices (FEE, EA)
 * with support for:
 * - 1-2 memory devices
 * - Read/Write/Erase/Invalidate operations
 * - Asynchronous operation support
 * - Device selection logic
 * 
 * Copyright (c) 2025
 */

#include "MemIf.h"
#include <string.h>

/*============================================================================*
 * Local Types
 *============================================================================*/

typedef struct {
    boolean Initialized;
    MemIf_StatusType DeviceStatus[MEMIF_MAX_DEVICES];
    MemIf_JobResultType JobResult[MEMIF_MAX_DEVICES];
    MemIf_ModeType DeviceMode[MEMIF_MAX_DEVICES];
    boolean JobPending[MEMIF_MAX_DEVICES];
} MemIf_GlobalType;

/*============================================================================*
 * Local Variables
 *============================================================================*/
static MemIf_GlobalType MemIf_Global = {
    .Initialized = FALSE,
    .DeviceStatus = {MEMIF_UNINIT, MEMIF_UNINIT},
    .JobResult = {MEMIF_JOB_OK, MEMIF_JOB_OK},
    .DeviceMode = {MEMIF_MODE_FAST, MEMIF_MODE_FAST},
    .JobPending = {FALSE, FALSE}
};

/*============================================================================*
 * Internal Function Prototypes
 *============================================================================*/
static boolean MemIf_IsDeviceValid(uint8_t DeviceIndex);
static Std_ReturnType MemIf_CheckDeviceReady(uint8_t DeviceIndex, uint8_t ServiceId);

/*============================================================================*
 * Development Error Reporting
 *============================================================================*/
#define MEMIF_SID_SET_MODE              0x08u

#ifndef E_OK
#define E_OK                            0x00u
#endif

#ifndef E_NOT_OK
#define E_NOT_OK                        0x01u
#endif

#ifndef NULL_PTR
#define NULL_PTR                        ((void*)0)
#endif

typedef unsigned char boolean;

#if (MEMIF_DEV_ERROR_DETECT == STD_ON)
    /* Det.h not available in this environment - error reporting disabled */
    #define MEMIF_REPORT_ERROR(ApiId, ErrorId) /* Empty */
#else
    #define MEMIF_REPORT_ERROR(ApiId, ErrorId)
#endif

/*============================================================================*
 * Local Functions
 *============================================================================*/

/**
 * @brief Check if device index is valid
 */
static boolean MemIf_IsDeviceValid(uint8_t DeviceIndex)
{
    if (DeviceIndex >= MEMIF_NUMBER_OF_DEVICES) {
        return FALSE;
    }
    
    if (MemIf_DeviceDrivers[DeviceIndex].Enabled == FALSE) {
        return FALSE;
    }
    
    return TRUE;
}

/**
 * @brief Check if device is ready for operation
 */
static Std_ReturnType MemIf_CheckDeviceReady(uint8_t DeviceIndex, uint8_t ServiceId)
{
    if (MemIf_Global.Initialized == FALSE) {
        MEMIF_REPORT_ERROR(ServiceId, MEMIF_E_NO_ERROR);
        return E_NOT_OK;
    }
    
    if (MemIf_IsDeviceValid(DeviceIndex) == FALSE) {
        MEMIF_REPORT_ERROR(ServiceId, MEMIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    
    return E_OK;
}

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

/**
 * @brief Initializes the MemIf module
 */
void MemIf_Init(void)
{
    uint8_t deviceIdx;
    
    /* Initialize all configured devices */
    for (deviceIdx = 0u; deviceIdx < MEMIF_NUMBER_OF_DEVICES; deviceIdx++) {
        if (MemIf_DeviceDrivers[deviceIdx].Enabled == TRUE) {
            /* Initialize underlying device */
            if (MemIf_DeviceDrivers[deviceIdx].Init != NULL_PTR) {
                MemIf_DeviceDrivers[deviceIdx].Init();
            }
            
            MemIf_Global.DeviceStatus[deviceIdx] = MEMIF_IDLE;
            MemIf_Global.JobResult[deviceIdx] = MEMIF_JOB_OK;
            MemIf_Global.JobPending[deviceIdx] = FALSE;
        }
    }
    
    MemIf_Global.Initialized = TRUE;
}

/**
 * @brief Sets the mode for the specified device
 */
void MemIf_SetMode(uint8_t DeviceIndex, MemIf_ModeType Mode)
{
    if (MemIf_CheckDeviceReady(DeviceIndex, MEMIF_SID_SET_MODE) != E_OK) {
        return;
    }
    
    if (Mode > MEMIF_MODE_FAST) {
        MEMIF_REPORT_ERROR(MEMIF_SID_SET_MODE, MEMIF_E_PARAM_DEVICE);
        return;
    }
    
    MemIf_Global.DeviceMode[DeviceIndex] = Mode;
    
    /* Pass mode to underlying device */
    if (MemIf_DeviceDrivers[DeviceIndex].SetMode != NULL_PTR) {
        MemIf_DeviceDrivers[DeviceIndex].SetMode((uint8_t)Mode);
    }
}

/**
 * @brief Reads data from the memory device
 */
Std_ReturnType MemIf_Read(
    uint8_t DeviceIndex,
    uint16_t BlockNumber,
    uint16_t BlockOffset,
    uint8_t* DataBufferPtr,
    uint16_t Length)
{
    Std_ReturnType result;
    
    if (MemIf_CheckDeviceReady(DeviceIndex, MEMIF_SID_READ) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if (DataBufferPtr == NULL_PTR) {
        MEMIF_REPORT_ERROR(MEMIF_SID_READ, MEMIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (BlockNumber >= MemIf_DeviceDrivers[DeviceIndex].NumOfBlocks) {
        MEMIF_REPORT_ERROR(MEMIF_SID_READ, MEMIF_E_INVALID_BLOCK);
        return E_NOT_OK;
    }
    
    /* Check if device is busy */
    if (MemIf_Global.DeviceStatus[DeviceIndex] == MEMIF_BUSY) {
        return E_NOT_OK;
    }
    
    /* Forward to device driver */
    if (MemIf_DeviceDrivers[DeviceIndex].Read != NULL_PTR) {
        result = MemIf_DeviceDrivers[DeviceIndex].Read(
            BlockNumber + MemIf_DeviceDrivers[DeviceIndex].BlockOffset,
            BlockOffset,
            DataBufferPtr,
            Length
        );
        
        if (result == E_OK) {
            MemIf_Global.DeviceStatus[DeviceIndex] = MEMIF_BUSY;
            MemIf_Global.JobPending[DeviceIndex] = TRUE;
            MemIf_Global.JobResult[DeviceIndex] = MEMIF_JOB_PENDING;
        }
        
        return result;
    }
    
    return E_NOT_OK;
}

/**
 * @brief Writes data to the memory device
 */
Std_ReturnType MemIf_Write(
    uint8_t DeviceIndex,
    uint16_t BlockNumber,
    uint8_t* DataBufferPtr)
{
    Std_ReturnType result;
    
    if (MemIf_CheckDeviceReady(DeviceIndex, MEMIF_SID_WRITE) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if (DataBufferPtr == NULL_PTR) {
        MEMIF_REPORT_ERROR(MEMIF_SID_WRITE, MEMIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (BlockNumber >= MemIf_DeviceDrivers[DeviceIndex].NumOfBlocks) {
        MEMIF_REPORT_ERROR(MEMIF_SID_WRITE, MEMIF_E_INVALID_BLOCK);
        return E_NOT_OK;
    }
    
    /* Check if device is busy */
    if (MemIf_Global.DeviceStatus[DeviceIndex] == MEMIF_BUSY) {
        return E_NOT_OK;
    }
    
    /* Forward to device driver */
    if (MemIf_DeviceDrivers[DeviceIndex].Write != NULL_PTR) {
        result = MemIf_DeviceDrivers[DeviceIndex].Write(
            BlockNumber + MemIf_DeviceDrivers[DeviceIndex].BlockOffset,
            DataBufferPtr
        );
        
        if (result == E_OK) {
            MemIf_Global.DeviceStatus[DeviceIndex] = MEMIF_BUSY;
            MemIf_Global.JobPending[DeviceIndex] = TRUE;
            MemIf_Global.JobResult[DeviceIndex] = MEMIF_JOB_PENDING;
        }
        
        return result;
    }
    
    return E_NOT_OK;
}

/**
 * @brief Cancels an ongoing operation
 */
void MemIf_Cancel(uint8_t DeviceIndex)
{
    if (MemIf_CheckDeviceReady(DeviceIndex, MEMIF_SID_CANCEL) != E_OK) {
        return;
    }
    
    if (MemIf_DeviceDrivers[DeviceIndex].Cancel != NULL_PTR) {
        MemIf_DeviceDrivers[DeviceIndex].Cancel();
    }
    
    MemIf_Global.DeviceStatus[DeviceIndex] = MEMIF_IDLE;
    MemIf_Global.JobPending[DeviceIndex] = FALSE;
    MemIf_Global.JobResult[DeviceIndex] = MEMIF_JOB_CANCELED;
}

/**
 * @brief Gets the current status of a memory device
 */
MemIf_StatusType MemIf_GetStatus(uint8_t DeviceIndex)
{
    if (MemIf_Global.Initialized == FALSE) {
        return MEMIF_UNINIT;
    }
    
    if (DeviceIndex >= MEMIF_NUMBER_OF_DEVICES) {
        MEMIF_REPORT_ERROR(MEMIF_SID_GET_STATUS, MEMIF_E_PARAM_DEVICE);
        return MEMIF_UNINIT;
    }
    
    return MemIf_Global.DeviceStatus[DeviceIndex];
}

/**
 * @brief Gets the result of the last job
 */
MemIf_JobResultType MemIf_GetJobResult(uint8_t DeviceIndex)
{
    if (MemIf_Global.Initialized == FALSE) {
        return MEMIF_JOB_FAILED;
    }
    
    if (DeviceIndex >= MEMIF_NUMBER_OF_DEVICES) {
        MEMIF_REPORT_ERROR(MEMIF_SID_GET_JOB_RESULT, MEMIF_E_PARAM_DEVICE);
        return MEMIF_JOB_FAILED;
    }
    
    return MemIf_Global.JobResult[DeviceIndex];
}

/**
 * @brief Invalidates a block
 */
Std_ReturnType MemIf_Invalidate(uint8_t DeviceIndex, uint16_t BlockNumber)
{
    Std_ReturnType result;
    
    if (MemIf_CheckDeviceReady(DeviceIndex, MEMIF_SID_INVALIDATE) != E_OK) {
        return E_NOT_OK;
    }
    
    if (BlockNumber >= MemIf_DeviceDrivers[DeviceIndex].NumOfBlocks) {
        MEMIF_REPORT_ERROR(MEMIF_SID_INVALIDATE, MEMIF_E_INVALID_BLOCK);
        return E_NOT_OK;
    }
    
    /* Check if device is busy */
    if (MemIf_Global.DeviceStatus[DeviceIndex] == MEMIF_BUSY) {
        return E_NOT_OK;
    }
    
    #if (MEMIF_INVALIDATE_ENABLED == STD_ON)
    if (MemIf_DeviceDrivers[DeviceIndex].Invalidate != NULL_PTR) {
        result = MemIf_DeviceDrivers[DeviceIndex].Invalidate(
            BlockNumber + MemIf_DeviceDrivers[DeviceIndex].BlockOffset
        );
        
        if (result == E_OK) {
            MemIf_Global.DeviceStatus[DeviceIndex] = MEMIF_BUSY;
            MemIf_Global.JobPending[DeviceIndex] = TRUE;
            MemIf_Global.JobResult[DeviceIndex] = MEMIF_JOB_PENDING;
        }
        
        return result;
    }
    #endif
    
    return E_NOT_OK;
}

/**
 * @brief Erases a block or sector
 */
Std_ReturnType MemIf_Erase(uint8_t DeviceIndex, uint16_t BlockNumber)
{
    Std_ReturnType result;
    
    if (MemIf_CheckDeviceReady(DeviceIndex, MEMIF_SID_ERASE) != E_OK) {
        return E_NOT_OK;
    }
    
    if (BlockNumber >= MemIf_DeviceDrivers[DeviceIndex].NumOfBlocks) {
        MEMIF_REPORT_ERROR(MEMIF_SID_ERASE, MEMIF_E_INVALID_BLOCK);
        return E_NOT_OK;
    }
    
    /* Check if device is busy */
    if (MemIf_Global.DeviceStatus[DeviceIndex] == MEMIF_BUSY) {
        return E_NOT_OK;
    }
    
    #if (MEMIF_ERASE_ENABLED == STD_ON)
    if (MemIf_DeviceDrivers[DeviceIndex].Erase != NULL_PTR) {
        result = MemIf_DeviceDrivers[DeviceIndex].Erase(
            BlockNumber + MemIf_DeviceDrivers[DeviceIndex].BlockOffset
        );
        
        if (result == E_OK) {
            MemIf_Global.DeviceStatus[DeviceIndex] = MEMIF_BUSY;
            MemIf_Global.JobPending[DeviceIndex] = TRUE;
            MemIf_Global.JobResult[DeviceIndex] = MEMIF_JOB_PENDING;
        }
        
        return result;
    }
    #endif
    
    return E_NOT_OK;
}

/**
 * @brief Main function for MemIf
 */
void MemIf_MainFunction(void)
{
    uint8_t deviceIdx;
    MemIf_JobResultType jobResult;
    
    if (MemIf_Global.Initialized == FALSE) {
        return;
    }
    
    /* Process each device */
    for (deviceIdx = 0u; deviceIdx < MEMIF_NUMBER_OF_DEVICES; deviceIdx++) {
        if (MemIf_DeviceDrivers[deviceIdx].Enabled == FALSE) {
            continue;
        }
        
        /* Call device main function */
        if (MemIf_DeviceDrivers[deviceIdx].MainFunction != NULL_PTR) {
            MemIf_DeviceDrivers[deviceIdx].MainFunction();
        }
        
        /* Update job status if a job is pending */
        if (MemIf_Global.JobPending[deviceIdx] == TRUE) {
            if (MemIf_DeviceDrivers[deviceIdx].GetJobResult != NULL_PTR) {
                jobResult = (MemIf_JobResultType)MemIf_DeviceDrivers[deviceIdx].GetJobResult();
                MemIf_Global.JobResult[deviceIdx] = jobResult;
                
                /* Update device status based on job result */
                if (jobResult != MEMIF_JOB_PENDING) {
                    MemIf_Global.JobPending[deviceIdx] = FALSE;
                    MemIf_Global.DeviceStatus[deviceIdx] = MEMIF_IDLE;
                }
            }
        }
    }
}

/*============================================================================*
 * Configuration - Default Device Drivers (Stub implementations)
 * These should be replaced with actual FEE or EA driver references
 *============================================================================*/

/* Stub device 0 functions */
static void MemIf_Device0_Init(void) { }
static void MemIf_Device0_SetMode(uint8_t Mode) { }
static Std_ReturnType MemIf_Device0_Read(
    uint16_t BlockNumber,
    uint16_t BlockOffset,
    uint8_t* DataBufferPtr,
    uint16_t Length) { return E_OK; }
static Std_ReturnType MemIf_Device0_Write(
    uint16_t BlockNumber,
    uint8_t* DataBufferPtr) { return E_OK; }
static void MemIf_Device0_Cancel(void) { }
static uint8_t MemIf_Device0_GetStatus(void) { return MEMIF_IDLE; }
static uint8_t MemIf_Device0_GetJobResult(void) { return MEMIF_JOB_OK; }
static Std_ReturnType MemIf_Device0_Invalidate(uint16_t BlockNumber) { return E_OK; }
static Std_ReturnType MemIf_Device0_Erase(uint16_t BlockNumber) { return E_OK; }
static void MemIf_Device0_MainFunction(void) { }

/* Stub device 1 functions */
static void MemIf_Device1_Init(void) { }
static void MemIf_Device1_SetMode(uint8_t Mode) { }
static Std_ReturnType MemIf_Device1_Read(
    uint16_t BlockNumber,
    uint16_t BlockOffset,
    uint8_t* DataBufferPtr,
    uint16_t Length) { return E_OK; }
static Std_ReturnType MemIf_Device1_Write(
    uint16_t BlockNumber,
    uint8_t* DataBufferPtr) { return E_OK; }
static void MemIf_Device1_Cancel(void) { }
static uint8_t MemIf_Device1_GetStatus(void) { return MEMIF_IDLE; }
static uint8_t MemIf_Device1_GetJobResult(void) { return MEMIF_JOB_OK; }
static Std_ReturnType MemIf_Device1_Invalidate(uint16_t BlockNumber) { return E_OK; }
static Std_ReturnType MemIf_Device1_Erase(uint16_t BlockNumber) { return E_OK; }
static void MemIf_Device1_MainFunction(void) { }

/* Device driver table - Configuration */
const MemIf_DeviceDriverType MemIf_DeviceDrivers[MEMIF_NUMBER_OF_DEVICES] = {
    /* Device 0 - Primary FEE */
    {
        .Init = MemIf_Device0_Init,
        .SetMode = MemIf_Device0_SetMode,
        .Read = MemIf_Device0_Read,
        .Write = MemIf_Device0_Write,
        .Cancel = MemIf_Device0_Cancel,
        .GetStatus = MemIf_Device0_GetStatus,
        .GetJobResult = MemIf_Device0_GetJobResult,
        .Invalidate = MemIf_Device0_Invalidate,
        .Erase = MemIf_Device0_Erase,
        .MainFunction = MemIf_Device0_MainFunction,
        .IsFee = TRUE,
        .BlockOffset = MEMIF_DEVICE_0_BLOCK_OFFSET,
        .NumOfBlocks = MEMIF_DEVICE_0_BLOCK_COUNT,
        .Enabled = (boolean)MEMIF_DEVICE_0_ENABLED
    },
    /* Device 1 - Secondary/Redundant */
    {
        .Init = MemIf_Device1_Init,
        .SetMode = MemIf_Device1_SetMode,
        .Read = MemIf_Device1_Read,
        .Write = MemIf_Device1_Write,
        .Cancel = MemIf_Device1_Cancel,
        .GetStatus = MemIf_Device1_GetStatus,
        .GetJobResult = MemIf_Device1_GetJobResult,
        .Invalidate = MemIf_Device1_Invalidate,
        .Erase = MemIf_Device1_Erase,
        .MainFunction = MemIf_Device1_MainFunction,
        .IsFee = TRUE,
        .BlockOffset = MEMIF_DEVICE_1_BLOCK_OFFSET,
        .NumOfBlocks = MEMIF_DEVICE_1_BLOCK_COUNT,
        .Enabled = (boolean)MEMIF_DEVICE_1_ENABLED
    }
};
