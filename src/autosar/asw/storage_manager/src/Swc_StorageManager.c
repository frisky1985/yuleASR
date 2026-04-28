/**
 * @file Swc_StorageManager.c
 * @brief Storage Manager Software Component Implementation
 * @version 1.0.0
 * @date 2026-04-19
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Classic Platform 4.x - Application Software Component
 * Purpose: Non-volatile data storage management at application layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Swc_StorageManager.h"
#include "Rte.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define SWC_STORAGEMANAGER_MODULE_ID        0x84
#define SWC_STORAGEMANAGER_INSTANCE_ID      0x00

/* Maximum blocks */
#define STORAGE_MAX_BLOCKS                  32

/* Block data size */
#define STORAGE_BLOCK_DATA_SIZE             256

/* Write cycle threshold for maintenance */
#define STORAGE_WRITE_CYCLE_THRESHOLD       100000

/*==================================================================================================
*                                    LOCAL TYPES
==================================================================================================*/
typedef struct {
    uint8 data[STORAGE_BLOCK_DATA_SIZE];
    Swc_StorageBlockStatusType status;
    boolean writeProtected;
    boolean isConfigured;
} Swc_StorageBlockType;

typedef struct {
    Swc_StorageBlockType blocks[STORAGE_MAX_BLOCKS];
    Swc_StorageStatisticsType statistics;
    uint8 numBlocks;
    boolean isInitialized;
} Swc_StorageManagerInternalType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define RTE_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC Swc_StorageManagerInternalType swcStorageManager = {
    .blocks = {{0}},
    .statistics = {0},
    .numBlocks = 0,
    .isInitialized = FALSE
};

#define RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC sint16 Swc_StorageManager_FindBlock(uint16 blockId);
STATIC uint16 Swc_StorageManager_CalculateCrc(const uint8* data, uint16 length);
STATIC void Swc_StorageManager_UpdateStatistics(void);
STATIC void Swc_StorageManager_CheckWriteCycles(void);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Finds block by ID
 */
STATIC sint16 Swc_StorageManager_FindBlock(uint16 blockId)
{
    uint8 i;

    for (i = 0; i < swcStorageManager.numBlocks; i++) {
        if (swcStorageManager.blocks[i].status.blockId == blockId) {
            return (sint16)i;
        }
    }

    return -1;
}

/**
 * @brief Calculates CRC16
 */
STATIC uint16 Swc_StorageManager_CalculateCrc(const uint8* data, uint16 length)
{
    uint16 crc = 0xFFFF;
    uint16 i;
    uint8 j;

    for (i = 0; i < length; i++) {
        crc ^= (uint16)data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief Updates storage statistics
 */
STATIC void Swc_StorageManager_UpdateStatistics(void)
{
    uint8 i;
    uint32 usedMemory = 0;

    for (i = 0; i < swcStorageManager.numBlocks; i++) {
        if (swcStorageManager.blocks[i].status.state != STORAGE_BLOCK_EMPTY) {
            usedMemory += STORAGE_BLOCK_DATA_SIZE;
        }
    }

    swcStorageManager.statistics.memoryUsed = usedMemory;
    swcStorageManager.statistics.memoryTotal = STORAGE_MAX_BLOCKS * STORAGE_BLOCK_DATA_SIZE;
}

/**
 * @brief Checks write cycles for maintenance
 */
STATIC void Swc_StorageManager_CheckWriteCycles(void)
{
    uint8 i;

    for (i = 0; i < swcStorageManager.numBlocks; i++) {
        if (swcStorageManager.blocks[i].status.writeCycleCounter > STORAGE_WRITE_CYCLE_THRESHOLD) {
            /* Trigger maintenance - in real implementation, would trigger block relocation */
            Det_ReportError(SWC_STORAGEMANAGER_MODULE_ID, SWC_STORAGEMANAGER_INSTANCE_ID,
                            0x50, RTE_E_OK);
        }
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the Storage Manager component
 */
void Swc_StorageManager_Init(void)
{
    uint8 i;

    if (swcStorageManager.isInitialized) {
        return;
    }

    /* Initialize blocks */
    for (i = 0; i < STORAGE_MAX_BLOCKS; i++) {
        swcStorageManager.blocks[i].status.blockId = 0;
        swcStorageManager.blocks[i].status.state = STORAGE_BLOCK_EMPTY;
        swcStorageManager.blocks[i].status.writeCycleCounter = 0;
        swcStorageManager.blocks[i].status.lastWriteTime = 0;
        swcStorageManager.blocks[i].status.dataLength = 0;
        swcStorageManager.blocks[i].status.crc = 0;
        swcStorageManager.blocks[i].writeProtected = FALSE;
        swcStorageManager.blocks[i].isConfigured = FALSE;
    }
    swcStorageManager.numBlocks = 0;

    /* Initialize statistics */
    swcStorageManager.statistics.readOperations = 0;
    swcStorageManager.statistics.writeOperations = 0;
    swcStorageManager.statistics.eraseOperations = 0;
    swcStorageManager.statistics.readErrors = 0;
    swcStorageManager.statistics.writeErrors = 0;
    swcStorageManager.statistics.memoryUsed = 0;
    swcStorageManager.statistics.memoryTotal = STORAGE_MAX_BLOCKS * STORAGE_BLOCK_DATA_SIZE;

    swcStorageManager.isInitialized = TRUE;

    Det_ReportError(SWC_STORAGEMANAGER_MODULE_ID, SWC_STORAGEMANAGER_INSTANCE_ID,
                    0x01, RTE_E_OK);
}

/**
 * @brief 100ms cyclic runnable
 */
void Swc_StorageManager_100ms(void)
{
    if (!swcStorageManager.isInitialized) {
        return;
    }

    /* Update statistics */
    Swc_StorageManager_UpdateStatistics();

    /* Check write cycles */
    Swc_StorageManager_CheckWriteCycles();
}

/**
 * @brief Write cycle management runnable
 */
void Swc_StorageManager_WriteCycle(void)
{
    uint8 i;

    if (!swcStorageManager.isInitialized) {
        return;
    }

    /* Monitor write cycles and perform maintenance if needed */
    for (i = 0; i < swcStorageManager.numBlocks; i++) {
        if (swcStorageManager.blocks[i].status.writeCycleCounter > 0) {
            /* Log write cycle information */
            /* In real implementation, would trigger wear leveling */
        }
    }
}

/**
 * @brief Reads data from storage block
 */
Swc_StorageResultType Swc_StorageManager_ReadBlock(uint16 blockId,
                                                    void* data,
                                                    uint16 length)
{
    sint16 blockIndex;

    if (data == NULL || length == 0) {
        return STORAGE_RESULT_INVALID_DATA;
    }

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    /* Find block */
    blockIndex = Swc_StorageManager_FindBlock(blockId);

    if (blockIndex < 0) {
        return STORAGE_RESULT_INVALID_BLOCK;
    }

    /* Check block state */
    if (swcStorageManager.blocks[blockIndex].status.state != STORAGE_BLOCK_VALID) {
        swcStorageManager.statistics.readErrors++;
        return STORAGE_RESULT_NOT_OK;
    }

    /* Validate length */
    if (length > swcStorageManager.blocks[blockIndex].status.dataLength) {
        length = swcStorageManager.blocks[blockIndex].status.dataLength;
    }

    /* Copy data */
    memcpy(data, swcStorageManager.blocks[blockIndex].data, length);

    /* Update statistics */
    swcStorageManager.statistics.readOperations++;

    return STORAGE_RESULT_OK;
}

/**
 * @brief Writes data to storage block
 */
Swc_StorageResultType Swc_StorageManager_WriteBlock(uint16 blockId,
                                                     const void* data,
                                                     uint16 length)
{
    sint16 blockIndex;
    uint16 crc;

    if (data == NULL || length == 0 || length > STORAGE_BLOCK_DATA_SIZE) {
        return STORAGE_RESULT_INVALID_DATA;
    }

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    /* Find or create block */
    blockIndex = Swc_StorageManager_FindBlock(blockId);

    if (blockIndex < 0) {
        /* Create new block */
        if (swcStorageManager.numBlocks >= STORAGE_MAX_BLOCKS) {
            return STORAGE_RESULT_MEMORY_FULL;
        }
        blockIndex = swcStorageManager.numBlocks;
        swcStorageManager.blocks[blockIndex].status.blockId = blockId;
        swcStorageManager.blocks[blockIndex].isConfigured = TRUE;
        swcStorageManager.numBlocks++;
    }

    /* Check write protection */
    if (swcStorageManager.blocks[blockIndex].writeProtected) {
        return STORAGE_RESULT_WRITE_PROTECTED;
    }

    /* Set state to writing */
    swcStorageManager.blocks[blockIndex].status.state = STORAGE_BLOCK_WRITING;

    /* Copy data */
    memcpy(swcStorageManager.blocks[blockIndex].data, data, length);
    swcStorageManager.blocks[blockIndex].status.dataLength = length;

    /* Calculate CRC */
    crc = Swc_StorageManager_CalculateCrc(data, length);
    swcStorageManager.blocks[blockIndex].status.crc = crc;

    /* Update write cycle counter */
    swcStorageManager.blocks[blockIndex].status.writeCycleCounter++;
    swcStorageManager.blocks[blockIndex].status.lastWriteTime = Rte_GetTime();

    /* Set state to valid */
    swcStorageManager.blocks[blockIndex].status.state = STORAGE_BLOCK_VALID;

    /* Update statistics */
    swcStorageManager.statistics.writeOperations++;

    /* Write block status via RTE */
    (void)Rte_Write_BlockStatus(&swcStorageManager.blocks[blockIndex].status);

    return STORAGE_RESULT_OK;
}

/**
 * @brief Gets block status
 */
Swc_StorageResultType Swc_StorageManager_GetBlockStatus(uint16 blockId,
                                                         Swc_StorageBlockStatusType* status)
{
    sint16 blockIndex;

    if (status == NULL) {
        return STORAGE_RESULT_INVALID_DATA;
    }

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    /* Find block */
    blockIndex = Swc_StorageManager_FindBlock(blockId);

    if (blockIndex < 0) {
        return STORAGE_RESULT_INVALID_BLOCK;
    }

    *status = swcStorageManager.blocks[blockIndex].status;
    return STORAGE_RESULT_OK;
}

/**
 * @brief Invalidates storage block
 */
Swc_StorageResultType Swc_StorageManager_InvalidateBlock(uint16 blockId)
{
    sint16 blockIndex;

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    /* Find block */
    blockIndex = Swc_StorageManager_FindBlock(blockId);

    if (blockIndex < 0) {
        return STORAGE_RESULT_INVALID_BLOCK;
    }

    /* Check write protection */
    if (swcStorageManager.blocks[blockIndex].writeProtected) {
        return STORAGE_RESULT_WRITE_PROTECTED;
    }

    /* Invalidate block */
    swcStorageManager.blocks[blockIndex].status.state = STORAGE_BLOCK_INVALID;

    return STORAGE_RESULT_OK;
}

/**
 * @brief Erases storage block
 */
Swc_StorageResultType Swc_StorageManager_EraseBlock(uint16 blockId)
{
    sint16 blockIndex;

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    /* Find block */
    blockIndex = Swc_StorageManager_FindBlock(blockId);

    if (blockIndex < 0) {
        return STORAGE_RESULT_INVALID_BLOCK;
    }

    /* Check write protection */
    if (swcStorageManager.blocks[blockIndex].writeProtected) {
        return STORAGE_RESULT_WRITE_PROTECTED;
    }

    /* Clear block data */
    memset(swcStorageManager.blocks[blockIndex].data, 0xFF, STORAGE_BLOCK_DATA_SIZE);

    /* Reset block status */
    swcStorageManager.blocks[blockIndex].status.state = STORAGE_BLOCK_EMPTY;
    swcStorageManager.blocks[blockIndex].status.dataLength = 0;
    swcStorageManager.blocks[blockIndex].status.crc = 0;

    /* Update statistics */
    swcStorageManager.statistics.eraseOperations++;

    return STORAGE_RESULT_OK;
}

/**
 * @brief Gets storage statistics
 */
Swc_StorageResultType Swc_StorageManager_GetStatistics(Swc_StorageStatisticsType* stats)
{
    if (stats == NULL) {
        return STORAGE_RESULT_INVALID_DATA;
    }

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    *stats = swcStorageManager.statistics;
    return STORAGE_RESULT_OK;
}

/**
 * @brief Sets write protection
 */
Swc_StorageResultType Swc_StorageManager_SetWriteProtection(uint16 blockId,
                                                             boolean writeProtected)
{
    sint16 blockIndex;

    if (!swcStorageManager.isInitialized) {
        return STORAGE_RESULT_NOT_OK;
    }

    /* Find block */
    blockIndex = Swc_StorageManager_FindBlock(blockId);

    if (blockIndex < 0) {
        return STORAGE_RESULT_INVALID_BLOCK;
    }

    swcStorageManager.blocks[blockIndex].writeProtected = writeProtected;

    return STORAGE_RESULT_OK;
}

#define RTE_STOP_SEC_CODE
#include "MemMap.h"
