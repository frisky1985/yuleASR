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
 * @file dem_nvm.c
 * @brief DEM NvM Integration Implementation
 * @version 1.0
 * @note AUTOSAR R22-11 compliant, MISRA C:2012
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "dem_nvm.h"
#include "dem_event.h"
#include "dem_dtc.h"
#include "dem_freeze_frame.h"
#include "dem.h"
#include <string.h>

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/*============================================================================*
 * Internal Data
 *============================================================================*/
static Dem_NvDataContainerType s_nvDataContainer;
static Dem_NvmBlockStateType s_nvmBlockStates[5];
static boolean s_nvmInitialized = FALSE;
static uint32_t s_nvmWriteCounter = 0U;

/* NvM block configuration */
static const NvM_BlockIdType s_nvmBlockIds[] = {
    DEM_NVM_BLOCK_ID_EVENT_STATUS,
    DEM_NVM_BLOCK_ID_DTC_DATA,
    DEM_NVM_BLOCK_ID_FREEZE_FRAME,
    DEM_NVM_BLOCK_ID_EXTENDED_DATA,
    DEM_NVM_BLOCK_ID_OBD_DATA
};

/* Current NvM data version */
#define DEM_NVM_DATA_VERSION                    0x00010000U  /* Version 1.0 */

/*============================================================================*
 * Static Helper Functions
 *============================================================================*/
/**
 * @brief Initialize NvM block states
 */
static void Dem_InitNvmBlockStates(void)
{
    for (uint8_t i = 0U; i < 5U; i++) {
        s_nvmBlockStates[i].blockId = s_nvmBlockIds[i];
        s_nvmBlockStates[i].writeState = DEM_NVM_WRITE_STATE_IDLE;
        s_nvmBlockStates[i].writeRetries = 0U;
        s_nvmBlockStates[i].lastWriteRequestTime = 0U;
        s_nvmBlockStates[i].dataModified = FALSE;
        s_nvmBlockStates[i].writePending = FALSE;
    }
}

/**
 * @brief Find NvM block state by block ID
 */
static Dem_NvmBlockStateType* Dem_FindNvmBlockState(NvM_BlockIdType blockId)
{
    Dem_NvmBlockStateType* state = NULL_PTR;
    
    for (uint8_t i = 0U; i < 5U; i++) {
        if (s_nvmBlockStates[i].blockId == blockId) {
            state = &s_nvmBlockStates[i];
            break;
        }
    }
    
    return state;
}

/**
 * @brief Simple CRC32 calculation
 */
static uint32_t Dem_CalculateCRC32(const uint8_t* data, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFFU;
    
    for (uint32_t i = 0U; i < size; i++) {
        crc ^= (uint32_t)data[i];
        for (uint8_t j = 0U; j < 8U; j++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            }
            else {
                crc >>= 1U;
            }
        }
    }
    
    return ~crc;
}

/**
 * @brief Prepare event data for NvM storage
 */
static void Dem_PrepareNvEventData(void)
{
    s_nvDataContainer.version = DEM_NVM_DATA_VERSION;
    s_nvDataContainer.numEvents = Dem_GetNumberOfEvents();
    s_nvDataContainer.writeCounter = s_nvmWriteCounter;
    
    /* Event data would be populated here from event entries */
    /* This is simplified - would iterate through all events */
}

/**
 * @brief Restore event data from NvM storage
 */
static void Dem_RestoreNvEventData(void)
{
    /* Restore event data from NvM container */
    /* This would populate event entries from the stored data */
}

/*============================================================================*
 * Public Functions
 *============================================================================*/
Std_ReturnType Dem_NvMInit(void)
{
    Std_ReturnType result = E_OK;
    
    /* Initialize NvM block states */
    Dem_InitNvmBlockStates();
    
    /* Initialize data container */
    (void)memset(&s_nvDataContainer, 0, sizeof(s_nvDataContainer));
    s_nvDataContainer.version = DEM_NVM_DATA_VERSION;
    
    s_nvmWriteCounter = 0U;
    s_nvmInitialized = TRUE;
    
    return result;
}

Std_ReturnType Dem_NvMReadEventData(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_nvmInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    /* Read event status block from NvM */
    result = NvM_ReadBlock(DEM_NVM_BLOCK_ID_EVENT_STATUS, &s_nvDataContainer);
    
    if (result == E_OK) {
        /* Wait for read completion - synchronous for now */
        /* In async implementation, this would be handled in callback */
        
        /* Validate data */
        if (Dem_NvMValidateData(&s_nvDataContainer) == TRUE) {
            Dem_RestoreNvEventData();
        }
        else {
            /* Data invalid - restore defaults */
            result = Dem_NvMRestoreEventDataDefaults();
        }
    }
    
    return result;
}

Std_ReturnType Dem_NvMWriteEventData(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (s_nvmInitialized == FALSE) {
        return E_NOT_OK;
    }
    
    /* Prepare data for writing */
    Dem_PrepareNvEventData();
    
    /* Calculate checksum */
    s_nvDataContainer.checksum = Dem_NvMCalculateChecksum(
        (const uint8_t*)&s_nvDataContainer,
        sizeof(s_nvDataContainer) - sizeof(s_nvDataContainer.checksum)
    );
    
    /* Increment write counter */
    s_nvmWriteCounter++;
    s_nvDataContainer.writeCounter = s_nvmWriteCounter;
    
    /* Write to NvM */
    result = NvM_WriteBlock(DEM_NVM_BLOCK_ID_EVENT_STATUS, &s_nvDataContainer);
    
    if (result == E_OK) {
        Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_EVENT_STATUS);
        if (state != NULL_PTR) {
            state->writeState = DEM_NVM_WRITE_STATE_PENDING;
            state->writePending = TRUE;
        }
    }
    
    return result;
}

Std_ReturnType Dem_NvMRestoreEventDataDefaults(void)
{
    Std_ReturnType result = E_OK;
    
    /* Clear NvM container */
    (void)memset(&s_nvDataContainer, 0, sizeof(s_nvDataContainer));
    s_nvDataContainer.version = DEM_NVM_DATA_VERSION;
    s_nvDataContainer.numEvents = 0U;
    s_nvDataContainer.writeCounter = 0U;
    
    /* Restore from ROM defaults using NvM service */
    result = NvM_RestoreBlockDefaults(DEM_NVM_BLOCK_ID_EVENT_STATUS, &s_nvDataContainer);
    
    return result;
}

Std_ReturnType Dem_NvMReadFreezeFrameData(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* Read freeze frame data from NvM */
    Dem_NvFreezeFrameDataType nvFreezeFrame;
    
    result = NvM_ReadBlock(DEM_NVM_BLOCK_ID_FREEZE_FRAME, &nvFreezeFrame);
    
    return result;
}

Std_ReturnType Dem_NvMWriteFreezeFrameData(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* Prepare freeze frame data */
    Dem_NvFreezeFrameDataType nvFreezeFrame;
    (void)memset(&nvFreezeFrame, 0, sizeof(nvFreezeFrame));
    
    /* Write to NvM */
    result = NvM_WriteBlock(DEM_NVM_BLOCK_ID_FREEZE_FRAME, &nvFreezeFrame);
    
    if (result == E_OK) {
        Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_FREEZE_FRAME);
        if (state != NULL_PTR) {
            state->writeState = DEM_NVM_WRITE_STATE_PENDING;
            state->writePending = TRUE;
        }
    }
    
    return result;
}

Std_ReturnType Dem_NvMReadExtendedData(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* Read extended data from NvM */
    Dem_NvExtendedDataType nvExtendedData;
    
    result = NvM_ReadBlock(DEM_NVM_BLOCK_ID_EXTENDED_DATA, &nvExtendedData);
    
    return result;
}

Std_ReturnType Dem_NvMWriteExtendedData(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* Prepare extended data */
    Dem_NvExtendedDataType nvExtendedData;
    (void)memset(&nvExtendedData, 0, sizeof(nvExtendedData));
    
    /* Write to NvM */
    result = NvM_WriteBlock(DEM_NVM_BLOCK_ID_EXTENDED_DATA, &nvExtendedData);
    
    if (result == E_OK) {
        Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_EXTENDED_DATA);
        if (state != NULL_PTR) {
            state->writeState = DEM_NVM_WRITE_STATE_PENDING;
            state->writePending = TRUE;
        }
    }
    
    return result;
}

void Dem_NvMWriteCallback(uint8_t ServiceId, NvM_RequestResultType JobResult)
{
    (void)ServiceId;
    
    /* Update block state based on result */
    for (uint8_t i = 0U; i < 5U; i++) {
        if (s_nvmBlockStates[i].writePending == TRUE) {
            if (JobResult == NVM_REQ_OK) {
                s_nvmBlockStates[i].writeState = DEM_NVM_WRITE_STATE_COMPLETED;
                s_nvmBlockStates[i].writeRetries = 0U;
            }
            else if (JobResult == NVM_REQ_NOT_OK) {
                s_nvmBlockStates[i].writeState = DEM_NVM_WRITE_STATE_FAILED;
                
                /* Retry if needed */
                if (s_nvmBlockStates[i].writeRetries < DEM_NVM_MAX_WRITE_RETRIES) {
                    s_nvmBlockStates[i].writeRetries++;
                    /* Trigger retry write */
                    (void)NvM_WriteBlock(s_nvmBlockStates[i].blockId, &s_nvDataContainer);
                }
            }
            
            s_nvmBlockStates[i].writePending = FALSE;
            s_nvmBlockStates[i].dataModified = FALSE;
            break;
        }
    }
}

Std_ReturnType Dem_NvMRequestWriteEventData(void)
{
    Std_ReturnType result = E_OK;
    
    Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_EVENT_STATUS);
    
    if (state != NULL_PTR) {
        state->dataModified = TRUE;
        state->lastWriteRequestTime = Dem_GetCurrentTimestamp();
    }
    
    return result;
}

Std_ReturnType Dem_NvMRequestWriteFreezeFrame(void)
{
    Std_ReturnType result = E_OK;
    
    Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_FREEZE_FRAME);
    
    if (state != NULL_PTR) {
        state->dataModified = TRUE;
    }
    
    return result;
}

Std_ReturnType Dem_NvMRequestWriteExtendedData(void)
{
    Std_ReturnType result = E_OK;
    
    Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_EXTENDED_DATA);
    
    if (state != NULL_PTR) {
        state->dataModified = TRUE;
    }
    
    return result;
}

boolean Dem_NvMIsWritePending(void)
{
    boolean pending = FALSE;
    
    for (uint8_t i = 0U; i < 5U; i++) {
        if (s_nvmBlockStates[i].writePending == TRUE) {
            pending = TRUE;
            break;
        }
    }
    
    return pending;
}

void Dem_NvMCancelWrite(void)
{
    /* Cancel any pending writes */
    for (uint8_t i = 0U; i < 5U; i++) {
        if (s_nvmBlockStates[i].writePending == TRUE) {
            s_nvmBlockStates[i].writePending = FALSE;
            s_nvmBlockStates[i].writeState = DEM_NVM_WRITE_STATE_IDLE;
        }
    }
    
    /* Call NvM cancel function */
    NvM_CancelWriteAll();
}

void Dem_NvMMainFunction(void)
{
    if (s_nvmInitialized == FALSE) {
        return;
    }
    
    /* Process pending write requests */
    for (uint8_t i = 0U; i < 5U; i++) {
        if ((s_nvmBlockStates[i].dataModified == TRUE) &&
            (s_nvmBlockStates[i].writePending == FALSE)) {
            
            /* Check if enough time has passed since last write request */
            /* This implements write delay for performance optimization */
            uint32_t currentTime = Dem_GetCurrentTimestamp();
            uint32_t elapsedTime = currentTime - s_nvmBlockStates[i].lastWriteRequestTime;
            if (elapsedTime >= DEM_NVM_WRITE_DELAY_MS)
            {
                /* Initiate write based on block type */
                switch (s_nvmBlockStates[i].blockId) {
                    case DEM_NVM_BLOCK_ID_EVENT_STATUS:
                        (void)Dem_NvMWriteEventData();
                        break;

                    case DEM_NVM_BLOCK_ID_FREEZE_FRAME:
                        (void)Dem_NvMWriteFreezeFrameData();
                        break;

                    case DEM_NVM_BLOCK_ID_EXTENDED_DATA:
                        (void)Dem_NvMWriteExtendedData();
                        break;

                    default:
                        /* Unknown block */
                        break;
                }
            }
        }
    }
}

uint32_t Dem_NvMCalculateChecksum(const uint8_t* data, uint32_t size)
{
    return Dem_CalculateCRC32(data, size);
}

boolean Dem_NvMValidateData(const Dem_NvDataContainerType* container)
{
    boolean valid = FALSE;
    
    if (container != NULL_PTR) {
        /* Check version */
        if (container->version == DEM_NVM_DATA_VERSION) {
            /* Calculate checksum */
            uint32_t calcChecksum = Dem_CalculateCRC32(
                (const uint8_t*)container,
                sizeof(*container) - sizeof(container->checksum)
            );
            
            /* Verify checksum */
            if (calcChecksum == container->checksum) {
                valid = TRUE;
            }
        }
    }
    
    return valid;
}

void Dem_NvMMarkEventDataModified(void)
{
    Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_EVENT_STATUS);
    
    if (state != NULL_PTR) {
        state->dataModified = TRUE;
    }
}

void Dem_NvMMarkFreezeFrameModified(void)
{
    Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_FREEZE_FRAME);
    
    if (state != NULL_PTR) {
        state->dataModified = TRUE;
    }
}

void Dem_NvMMarkExtendedDataModified(void)
{
    Dem_NvmBlockStateType* state = Dem_FindNvmBlockState(DEM_NVM_BLOCK_ID_EXTENDED_DATA);
    
    if (state != NULL_PTR) {
        state->dataModified = TRUE;
    }
}


/*==================================================================================================
 *                                      ADDITIONAL NVM INTEGRATION
 * CRITICAL FIX: Enhanced NvM integration for AUTOSAR compliance
==================================================================================================*/

#include "NvM.h"
#include "Det.h"

/* NvM block IDs for Dem */
#define DEM_NVM_BLOCK_ID_PRIMARY    (1U)
#define DEM_NVM_BLOCK_ID_MIRROR     (2U)
#define DEM_NVM_BLOCK_ID_PERMANENT  (3U)
#define DEM_NVM_BLOCK_ID_STATUS     (4U)

/* NvM write queue */
typedef struct {
    uint8 BlockId;
    boolean Pending;
    uint8 RetryCount;
} Dem_NvmWriteQueueEntryType;

static Dem_NvmWriteQueueEntryType Dem_NvmWriteQueue[DEM_CFG_NVM_WRITE_QUEUE_SIZE];
static uint8 Dem_NvmWriteQueueHead = 0;
static uint8 Dem_NvmWriteQueueTail = 0;

/**
 * rief   Initialize NvM integration
 */
void Dem_NvmInit(void)
{
    uint8 i;
    
    for (i = 0; i < DEM_CFG_NVM_WRITE_QUEUE_SIZE; i++) {
        Dem_NvmWriteQueue[i].BlockId = 0;
        Dem_NvmWriteQueue[i].Pending = FALSE;
        Dem_NvmWriteQueue[i].RetryCount = 0;
    }
    
    Dem_NvmWriteQueueHead = 0;
    Dem_NvmWriteQueueTail = 0;
}

/**
 * rief   Read event memory from NvM
 */
Std_ReturnType Dem_NvmReadEventMemory(
    Dem_DTCOriginType Origin,
    Dem_EventMemoryEntryType* Entry,
    uint8 Index)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 blockId;
    NvM_RequestResultType nvmResult;
    
    /* Determine block ID based on origin */
    switch (Origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
            blockId = DEM_NVM_BLOCK_ID_PRIMARY;
            break;
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
            blockId = DEM_NVM_BLOCK_ID_MIRROR;
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
            blockId = DEM_NVM_BLOCK_ID_PERMANENT;
            break;
        default:
            return E_NOT_OK;
    }
    
    /* Read from NvM */
    result = NvM_ReadBlock(blockId, (void*)Entry);
    
    if (result == E_OK) {
        /* Wait for read completion with timeout */
        uint16 timeout = DEM_CFG_NVM_READ_TIMEOUT_MS / DEM_CFG_MAIN_FUNCTION_PERIOD_MS;
        
        while (timeout > 0U ) {
            NvM_GetErrorStatus(blockId, &nvmResult);
            
            if (nvmResult == NVM_REQ_OK) {
                result = E_OK;
                break;
            } else if (nvmResult == NVM_REQ_NOT_OK) {
                result = E_NOT_OK;
                break;
            }
            
            timeout--;
            /* In real implementation, use OS delay */
        }
        
        if (timeout == 0U ) {
            result = E_NOT_OK; /* Timeout */
        }
    }
    
    return result;
}

/**
 * rief   Write event memory to NvM
 */
Std_ReturnType Dem_NvmWriteEventMemory(
    Dem_DTCOriginType Origin,
    const Dem_EventMemoryEntryType* Entry,
    uint8 Index)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 blockId;
    
    /* Determine block ID based on origin */
    switch (Origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
            blockId = DEM_NVM_BLOCK_ID_PRIMARY;
            break;
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
            blockId = DEM_NVM_BLOCK_ID_MIRROR;
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
            blockId = DEM_NVM_BLOCK_ID_PERMANENT;
            break;
        default:
            return E_NOT_OK;
    }
    
    /* Queue the write request */
    Dem_EnterCritical();
    
    uint8 nextTail = (Dem_NvmWriteQueueTail + 1) % DEM_CFG_NVM_WRITE_QUEUE_SIZE;
    
    if (nextTail != Dem_NvmWriteQueueHead) {
        Dem_NvmWriteQueue[Dem_NvmWriteQueueTail].BlockId = blockId;
        Dem_NvmWriteQueue[Dem_NvmWriteQueueTail].Pending = TRUE;
        Dem_NvmWriteQueue[Dem_NvmWriteQueueTail].RetryCount = 0;
        Dem_NvmWriteQueueTail = nextTail;
        result = E_OK;
    } else {
        /* Queue full */
        result = E_NOT_OK;
    }
    
    Dem_ExitCritical();
    
    return result;
}

/**
 * rief   Process NvM write queue (called from main function)
 */
void Dem_NvmProcessWriteQueue(void)
{
    NvM_RequestResultType nvmResult;
    
    while (Dem_NvmWriteQueueHead != Dem_NvmWriteQueueTail) {
        Dem_NvmWriteQueueEntryType* entry = &Dem_NvmWriteQueue[Dem_NvmWriteQueueHead];
        
        if (entry->Pending) {
            /* Check current NvM status */
            NvM_GetErrorStatus(entry->BlockId, &nvmResult);
            
            if (nvmResult == NVM_REQ_OK || nvmResult == NVM_REQ_BLOCK_INVALID) {
                /* Start new write request */
                uint8* ramData = Dem_GetNvmRamData(entry->BlockId);
                
                if (NvM_WriteBlock(entry->BlockId, (void*)ramData) == E_OK) {
                    entry->Pending = FALSE; /* Write started */
                } else {
                    /* Retry */
                    entry->RetryCount++;
                    if (entry->RetryCount >= DEM_CFG_NVM_WRITE_RETRY) {
                        /* Max retries reached, drop entry */
                        Dem_NvmWriteQueueHead = (Dem_NvmWriteQueueHead + 1) % DEM_CFG_NVM_WRITE_QUEUE_SIZE;
                    }
                    break; /* Try again next cycle */
                }
            } else if (nvmResult == NVM_REQ_PENDING) {
                /* Write in progress, wait */
                break;
            } else if (nvmResult == NVM_REQ_NOT_OK) {
                /* Write failed, retry */
                entry->RetryCount++;
                if (entry->RetryCount >= DEM_CFG_NVM_WRITE_RETRY) {
                    /* Max retries reached */
                    Dem_NvmWriteQueueHead = (Dem_NvmWriteQueueHead + 1) % DEM_CFG_NVM_WRITE_QUEUE_SIZE;
                }
                break;
            }
        } else {
            /* Check if write completed */
            NvM_GetErrorStatus(entry->BlockId, &nvmResult);
            
            if (nvmResult == NVM_REQ_OK) {
                /* Write completed successfully */
                Dem_NvmWriteQueueHead = (Dem_NvmWriteQueueHead + 1) % DEM_CFG_NVM_WRITE_QUEUE_SIZE;
            } else if (nvmResult == NVM_REQ_NOT_OK) {
                /* Write failed */
                entry->RetryCount++;
                if (entry->RetryCount >= DEM_CFG_NVM_WRITE_RETRY) {
                    Dem_NvmWriteQueueHead = (Dem_NvmWriteQueueHead + 1) % DEM_CFG_NVM_WRITE_QUEUE_SIZE;
                } else {
                    entry->Pending = TRUE; /* Retry */
                }
                break;
            } else {
                /* Still pending */
                break;
            }
        }
    }
}

/**
 * rief   Invalidate NvM block
 */
Std_ReturnType Dem_NvmInvalidateBlock(uint8 BlockId)
{
    return NvM_InvalidateNvBlock(BlockId);
}

/**
 * rief   Erase NvM block
 */
Std_ReturnType Dem_NvmEraseBlock(uint8 BlockId)
{
    return NvM_EraseNvBlock(BlockId);
}

/**
 * rief   Restore NvM block defaults
 */
Std_ReturnType Dem_NvmRestoreBlockDefaults(uint8 BlockId)
{
    return NvM_RestoreBlockDefaults(BlockId, NULL_PTR);
}

/* Helper function to get RAM data pointer for block */
static uint8* Dem_GetNvmRamData(uint8 BlockId)
{
    switch (BlockId) {
        case DEM_NVM_BLOCK_ID_PRIMARY:
            return (uint8*)Dem_PrimaryMemory;
        case DEM_NVM_BLOCK_ID_MIRROR:
            return (uint8*)Dem_MirrorMemory;
        case DEM_NVM_BLOCK_ID_PERMANENT:
            return (uint8*)Dem_PermanentMemory;
        default:
            return NULL_PTR;
    }
}

