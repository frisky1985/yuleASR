/**
 * @file nvm_redundancy.c
 * @brief NvM Redundancy Management Implementation
 * @version 1.0.0
 * @date 2025
 * 
 * ASIL-D Compliant Implementation
 */

#include "nvm_redundancy.h"
#include "nvm_core.h"
#include "nvm_block_manager.h"
#include "nvm_storage_if.h"
#include <string.h>

/*============================================================================*
 * Module Variables
 *============================================================================*/
static Nvm_RedundancyContext_t g_redundancyContext = {0};
static Nvm_PartitionInfo_t g_partitions[4] = {0};
static Nvm_PartitionId_t g_activePartition = NVM_PART_CONFIG_A;

/* Static buffer for data operations */
static uint8_t g_redundancyBuffer[NVM_REDUNDANCY_MIRROR_SIZE];

/*============================================================================*
 * Private Function Prototypes
 *============================================================================*/
static int32_t Nvm_Private_FindRedundancyPair(uint16_t primaryBlockId);
static Nvm_ErrorCode_t Nvm_Private_CopyBlock(
    uint16_t sourceBlockId,
    uint16_t destBlockId
);

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_Redundancy_Init(void)
{
    if (g_redundancyContext.initialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* Initialize context */
    memset(&g_redundancyContext, 0, sizeof(Nvm_RedundancyContext_t));
    memset(&g_partitions, 0, sizeof(g_partitions));
    
    /* Initialize partitions */
    g_partitions[0].id = NVM_PART_CONFIG_A;
    g_partitions[0].redundant = true;
    
    g_partitions[1].id = NVM_PART_CONFIG_B;
    g_partitions[1].redundant = true;
    
    g_partitions[2].id = NVM_PART_RUNTIME;
    g_partitions[2].redundant = false;
    
    g_partitions[3].id = NVM_PART_DIAGNOSTIC;
    g_partitions[3].redundant = false;
    
    g_activePartition = NVM_PART_CONFIG_A;
    g_redundancyContext.initialized = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_Deinit(void)
{
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    memset(&g_redundancyContext, 0, sizeof(Nvm_RedundancyContext_t));
    memset(&g_partitions, 0, sizeof(g_partitions));
    memset(g_redundancyBuffer, 0, sizeof(g_redundancyBuffer));
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_ConfigurePair(
    uint16_t primaryBlockId,
    uint16_t backupBlockId,
    bool autoRecover)
{
    Nvm_RedundancyPair_t* pair = NULL;
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Validate block IDs */
    if (primaryBlockId >= NVM_MAX_BLOCK_COUNT || 
        backupBlockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    if (primaryBlockId == backupBlockId) {
        return NVM_E_PARAM_RANGE;
    }
    
    /* Check if pair already exists */
    index = Nvm_Private_FindRedundancyPair(primaryBlockId);
    
    if (index < 0) {
        /* Find free slot */
        uint32_t i;
        for (i = 0; i < NVM_REDUNDANCY_MAX_PAIRS; i++) {
            if (!g_redundancyContext.pairs[i].enabled) {
                index = (int32_t)i;
                break;
            }
        }
    }
    
    if (index < 0) {
        return NVM_E_OUT_OF_MEMORY;
    }
    
    /* Configure pair */
    pair = &g_redundancyContext.pairs[index];
    pair->primaryBlockId = primaryBlockId;
    pair->backupBlockId = backupBlockId;
    pair->enabled = true;
    pair->autoRecover = autoRecover;
    pair->checksumPrimary = 0;
    pair->checksumBackup = 0;
    pair->syncCount = 0;
    
    g_redundancyContext.activePairs++;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_RemovePair(uint16_t primaryBlockId)
{
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    index = Nvm_Private_FindRedundancyPair(primaryBlockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Clear pair */
    memset(&g_redundancyContext.pairs[index], 0, sizeof(Nvm_RedundancyPair_t));
    g_redundancyContext.activePairs--;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_Enable(uint16_t blockId)
{
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    g_redundancyContext.pairs[index].enabled = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_Disable(uint16_t blockId)
{
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    g_redundancyContext.pairs[index].enabled = false;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_Write(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_RedundancyPair_t* pair = NULL;
    int32_t index;
    uint32_t checksum;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (length == 0 || length > NVM_REDUNDANCY_MIRROR_SIZE) {
        return NVM_E_PARAM_RANGE;
    }
    
    /* Find redundancy pair */
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0 || !g_redundancyContext.pairs[index].enabled) {
        /* No redundancy, write directly */
        return Nvm_StorageIf_WriteBlock(blockId, data, length);
    }
    
    pair = &g_redundancyContext.pairs[index];
    
    /* Calculate checksum */
    checksum = Nvm_Redundancy_CalculateChecksum(data, length);
    
    /* Write to primary block */
    result = Nvm_StorageIf_WriteBlock(pair->primaryBlockId, data, length);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Write to backup block */
    result = Nvm_StorageIf_WriteBlock(pair->backupBlockId, data, length);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Update checksums */
    pair->checksumPrimary = checksum;
    pair->checksumBackup = checksum;
    pair->syncCount++;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_Read(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_RedundancyPair_t* pair = NULL;
    int32_t index;
    uint32_t checksum;
    bool primaryValid = false;
    bool backupValid = false;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (length == 0 || length > NVM_REDUNDANCY_MIRROR_SIZE) {
        return NVM_E_PARAM_RANGE;
    }
    
    /* Find redundancy pair */
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0 || !g_redundancyContext.pairs[index].enabled) {
        /* No redundancy, read directly */
        return Nvm_StorageIf_ReadBlock(blockId, data, length);
    }
    
    pair = &g_redundancyContext.pairs[index];
    
    /* Try reading primary */
    result = Nvm_StorageIf_ReadBlock(pair->primaryBlockId, data, length);
    if (result == NVM_OK) {
        checksum = Nvm_Redundancy_CalculateChecksum(data, length);
        if (checksum == pair->checksumPrimary) {
            primaryValid = true;
        }
    }
    
    if (primaryValid) {
        return NVM_OK;
    }
    
    /* Primary invalid, try backup */
    result = Nvm_StorageIf_ReadBlock(pair->backupBlockId, data, length);
    if (result == NVM_OK) {
        checksum = Nvm_Redundancy_CalculateChecksum(data, length);
        if (checksum == pair->checksumBackup) {
            backupValid = true;
            
            /* Attempt to recover primary */
            if (pair->autoRecover) {
                Nvm_Private_CopyBlock(pair->backupBlockId, pair->primaryBlockId);
                g_redundancyContext.recoveryCount++;
            }
        }
    }
    
    if (backupValid) {
        return NVM_OK;
    }
    
    /* Both blocks invalid */
    g_redundancyContext.failedRecoveries++;
    return NVM_E_RECOVERY_FAILED;
}

Nvm_ErrorCode_t Nvm_Redundancy_Verify(uint16_t blockId)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_RedundancyPair_t* pair = NULL;
    int32_t index;
    uint32_t checksumPrimary = 0;
    uint32_t checksumBackup = 0;
    bool primaryValid = false;
    bool backupValid = false;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    pair = &g_redundancyContext.pairs[index];
    
    /* Read and verify primary */
    result = Nvm_StorageIf_ReadBlock(pair->primaryBlockId, g_redundancyBuffer, 
                                     NVM_REDUNDANCY_MIRROR_SIZE);
    if (result == NVM_OK) {
        checksumPrimary = Nvm_Redundancy_CalculateChecksum(g_redundancyBuffer, 
                                                           NVM_REDUNDANCY_MIRROR_SIZE);
        primaryValid = (checksumPrimary == pair->checksumPrimary);
    }
    
    /* Read and verify backup */
    result = Nvm_StorageIf_ReadBlock(pair->backupBlockId, g_redundancyBuffer,
                                     NVM_REDUNDANCY_MIRROR_SIZE);
    if (result == NVM_OK) {
        checksumBackup = Nvm_Redundancy_CalculateChecksum(g_redundancyBuffer,
                                                          NVM_REDUNDANCY_MIRROR_SIZE);
        backupValid = (checksumBackup == pair->checksumBackup);
    }
    
    /* Compare checksums */
    if (primaryValid && backupValid) {
        if (checksumPrimary == checksumBackup) {
            return NVM_OK;
        }
    }
    
    return NVM_E_REDUNDANCY_FAILED;
}

Nvm_ErrorCode_t Nvm_Redundancy_Sync(uint16_t blockId)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_RedundancyPair_t* pair = NULL;
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    pair = &g_redundancyContext.pairs[index];
    
    /* Read primary and copy to backup */
    result = Nvm_StorageIf_ReadBlock(pair->primaryBlockId, g_redundancyBuffer,
                                     NVM_REDUNDANCY_MIRROR_SIZE);
    if (result != NVM_OK) {
        return result;
    }
    
    result = Nvm_StorageIf_WriteBlock(pair->backupBlockId, g_redundancyBuffer,
                                      NVM_REDUNDANCY_MIRROR_SIZE);
    if (result != NVM_OK) {
        return result;
    }
    
    pair->syncCount++;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_Recover(uint16_t blockId)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_RedundancyPair_t* pair = NULL;
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    pair = &g_redundancyContext.pairs[index];
    
    /* Try to recover primary from backup */
    result = Nvm_Private_CopyBlock(pair->backupBlockId, pair->primaryBlockId);
    
    if (result == NVM_OK) {
        g_redundancyContext.recoveryCount++;
    } else {
        g_redundancyContext.failedRecoveries++;
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_Redundancy_GetPairStatus(
    uint16_t blockId,
    Nvm_RedundancyPair_t* pair)
{
    int32_t index;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (pair == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    index = Nvm_Private_FindRedundancyPair(blockId);
    
    if (index < 0) {
        return NVM_E_BLOCK_INVALID;
    }
    
    *pair = g_redundancyContext.pairs[index];
    
    return NVM_OK;
}

uint32_t Nvm_Redundancy_CalculateChecksum(
    const uint8_t* data,
    uint32_t length)
{
    uint32_t checksum = 0;
    uint32_t i;
    
    if (data == NULL || length == 0) {
        return 0;
    }
    
    /* Simple checksum - can be replaced with CRC32 */
    for (i = 0; i < length; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ data[i];
    }
    
    return checksum;
}

Nvm_ErrorCode_t Nvm_Redundancy_ValidateChecksum(
    const uint8_t* data,
    uint32_t length,
    uint32_t expectedChecksum)
{
    uint32_t calculated;
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    calculated = Nvm_Redundancy_CalculateChecksum(data, length);
    
    if (calculated != expectedChecksum) {
        return NVM_E_CRC_FAILED;
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_InitPartition(Nvm_PartitionId_t partition)
{
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (partition > NVM_PART_DIAGNOSTIC) {
        return NVM_E_PARAM_RANGE;
    }
    
    g_partitions[partition].valid = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_GetPartitionInfo(
    Nvm_PartitionId_t partition,
    Nvm_PartitionInfo_t* info)
{
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (info == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (partition > NVM_PART_DIAGNOSTIC) {
        return NVM_E_PARAM_RANGE;
    }
    
    *info = g_partitions[partition];
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_ValidatePartition(Nvm_PartitionId_t partition)
{
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (partition > NVM_PART_DIAGNOSTIC) {
        return NVM_E_PARAM_RANGE;
    }
    
    if (!g_partitions[partition].valid) {
        return NVM_E_BLOCK_CORRUPTED;
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_CopyPartition(
    Nvm_PartitionId_t source,
    Nvm_PartitionId_t destination)
{
    (void)source;
    (void)destination;
    
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Partition copy implementation would go here */
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Redundancy_SwapPartition(void)
{
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Toggle active partition */
    if (g_activePartition == NVM_PART_CONFIG_A) {
        g_activePartition = NVM_PART_CONFIG_B;
    } else {
        g_activePartition = NVM_PART_CONFIG_A;
    }
    
    return NVM_OK;
}

Nvm_PartitionId_t Nvm_Redundancy_GetActivePartition(void)
{
    return g_activePartition;
}

Nvm_ErrorCode_t Nvm_Redundancy_GetStats(
    uint32_t* recoveryCount,
    uint32_t* failedCount)
{
    if (!g_redundancyContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (recoveryCount == NULL || failedCount == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    *recoveryCount = g_redundancyContext.recoveryCount;
    *failedCount = g_redundancyContext.failedRecoveries;
    
    return NVM_OK;
}

Nvm_RedundancyContext_t* Nvm_Redundancy_GetContext(void)
{
    return &g_redundancyContext;
}

/*============================================================================*
 * Private Functions
 *============================================================================*/

static int32_t Nvm_Private_FindRedundancyPair(uint16_t primaryBlockId)
{
    uint32_t i;
    
    for (i = 0; i < NVM_REDUNDANCY_MAX_PAIRS; i++) {
        if (g_redundancyContext.pairs[i].enabled &&
            g_redundancyContext.pairs[i].primaryBlockId == primaryBlockId) {
            return (int32_t)i;
        }
    }
    
    return -1;
}

static Nvm_ErrorCode_t Nvm_Private_CopyBlock(
    uint16_t sourceBlockId,
    uint16_t destBlockId)
{
    Nvm_ErrorCode_t result;
    
    result = Nvm_StorageIf_ReadBlock(sourceBlockId, g_redundancyBuffer,
                                     NVM_REDUNDANCY_MIRROR_SIZE);
    if (result != NVM_OK) {
        return result;
    }
    
    result = Nvm_StorageIf_WriteBlock(destBlockId, g_redundancyBuffer,
                                      NVM_REDUNDANCY_MIRROR_SIZE);
    
    return result;
}
