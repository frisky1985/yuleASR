/**
 * @file fee_gc.c
 * @brief Fee Garbage Collection Implementation
 * @version 1.0.0
 * @date 2026
 *
 * Garbage Collection for Flash EEPROM Emulation
 * MISRA C:2012 compliant
 */

#include "mcal/fee/fee_gc.h"
#include "mcal/fee/fee.h"
#include "mcal/fls/fls.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*
 * Static Variables
 *============================================================================*/
static Fee_GcState_t gFee_GcState = FEE_GC_STATE_IDLE;
static Fee_GcStatistics_t gFee_GcStats;

/* GC context */
static uint32_t gFee_GcSourceCluster = 0u;
static uint32_t gFee_GcTargetCluster = 0u;
static uint32_t gFee_GcScanAddress = 0u;
static uint32_t gFee_GcWriteAddress = 0u;
static uint32_t gFee_GcTotalBlocks = 0u;
static uint32_t gFee_GcProcessedBlocks = 0u;
static uint32_t gFee_GcCopiedBlocks = 0u;
static uint32_t gFee_GcStartTime = 0u;

/* Block buffer for copying */
static uint8_t gFee_GcBlockBuffer[FEE_MAX_BLOCK_SIZE + FEE_BLOCK_HEADER_SIZE + 4u];

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Get cluster config
 */
static const Fee_ClusterConfigType* Fee_Gc_GetCluster(uint32_t index)
{
    const Fee_RuntimeStateType* state = Fee_GetRuntimeState();
    if ((state == NULL) || (state->config == NULL)) {
        return NULL;
    }
    if (index >= state->config->clusterCount) {
        return NULL;
    }
    return &state->config->clusters[index];
}

/**
 * @brief Check if block is valid (newest version)
 */
static bool Fee_Gc_IsBlockValid(uint16_t blockNumber, uint32_t sequence)
{
    Fee_BlockInfoType* info;

    info = (Fee_BlockInfoType*)Fee_FindBlockInfo(blockNumber);
    if (info == NULL) {
        return false;
    }

    return (info->sequence == sequence) && (info->status == FEE_BLOCK_VALID);
}

/**
 * @brief Count invalid blocks in cluster
 */
static uint32_t Fee_Gc_CountInvalidBlocks(uint32_t clusterIndex)
{
    const Fee_ClusterConfigType* cluster;
    uint32_t scanAddr;
    Fee_BlockHeader_t header;
    uint32_t invalidCount = 0u;
    Fls_ErrorCode_t result;

    cluster = Fee_Gc_GetCluster(clusterIndex);
    if (cluster == NULL) {
        return 0u;
    }

    scanAddr = cluster->startAddress;

    while (scanAddr < (cluster->startAddress + cluster->size)) {
        result = Fls_Read(scanAddr, sizeof(Fee_BlockHeader_t), (uint8_t*)&header);
        if (result != FLS_OK) {
            break;
        }

        if (header.magic == FEE_BLOCK_HEADER_ERASED) {
            break;
        }

        if (header.magic != FEE_BLOCK_HEADER_MAGIC) {
            scanAddr += 4u;
            continue;
        }

        /* Check if this is the current valid version */
        if (!Fee_Gc_IsBlockValid(header.blockNumber, header.sequence)) {
            invalidCount++;
        }

        scanAddr += FEE_BLOCK_HEADER_SIZE + header.dataLength + 4u;
        scanAddr = (scanAddr + 3u) & ~3u;
    }

    return invalidCount;
}

/**
 * @brief Calculate GC progress
 */
static uint32_t Fee_Gc_CalculateProgress(void)
{
    if (gFee_GcTotalBlocks == 0u) {
        return 0u;
    }
    return (gFee_GcProcessedBlocks * 100u) / gFee_GcTotalBlocks;
}

/*============================================================================*
 * GC State Machine
 *============================================================================*/

/**
 * @brief GC State: START
 */
static Fee_ErrorCode_t Fee_Gc_StateStart(void)
{
    const Fee_ClusterConfigType* source;
    const Fee_ClusterConfigType* target;

    source = Fee_Gc_GetCluster(gFee_GcSourceCluster);
    target = Fee_Gc_GetCluster(gFee_GcTargetCluster);

    if ((source == NULL) || (target == NULL)) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        gFee_GcStats.gcFailures++;
        return FEE_E_INVALID_CLUSTER;
    }

    if (gFee_GcSourceCluster == gFee_GcTargetCluster) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        gFee_GcStats.gcFailures++;
        return FEE_E_INVALID_CLUSTER;
    }

    /* Initialize context */
    gFee_GcScanAddress = source->startAddress;
    gFee_GcWriteAddress = target->startAddress;
    gFee_GcTotalBlocks = 0u;
    gFee_GcProcessedBlocks = 0u;
    gFee_GcCopiedBlocks = 0u;
    gFee_GcStartTime = 0u; /* TODO: Get system time */

    /* Mark target cluster as erasing */
    ((Fee_ClusterConfigType*)target)->status = FEE_CLUSTER_ERASING;

    /* Erase target cluster */
    Fls_Erase(target->startAddress, target->size);

    gFee_GcState = FEE_GC_STATE_SCAN_SOURCE;

    return FEE_OK;
}

/**
 * @brief GC State: SCAN_SOURCE
 */
static Fee_ErrorCode_t Fee_Gc_StateScanSource(void)
{
    const Fee_ClusterConfigType* source;
    Fee_BlockHeader_t header;
    Fls_ErrorCode_t result;

    source = Fee_Gc_GetCluster(gFee_GcSourceCluster);
    if (source == NULL) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_INVALID_CLUSTER;
    }

    /* Check if reached end */
    if (gFee_GcScanAddress >= (source->startAddress + source->size)) {
        gFee_GcState = FEE_GC_STATE_ERASE_SOURCE;
        return FEE_OK;
    }

    /* Read header */
    result = Fls_Read(gFee_GcScanAddress, sizeof(Fee_BlockHeader_t), (uint8_t*)&header);
    if (result != FLS_OK) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_READ_FAILED;
    }

    /* Check if empty */
    if (header.magic == FEE_BLOCK_HEADER_ERASED) {
        gFee_GcState = FEE_GC_STATE_ERASE_SOURCE;
        return FEE_OK;
    }

    /* Check if valid header */
    if (header.magic != FEE_BLOCK_HEADER_MAGIC) {
        gFee_GcScanAddress += 4u;
        return FEE_OK;
    }

    /* Count block */
    gFee_GcTotalBlocks++;

    /* Check if this is current valid version */
    if (Fee_Gc_IsBlockValid(header.blockNumber, header.sequence)) {
        /* Copy this block */
        gFee_GcState = FEE_GC_STATE_COPY_BLOCK;
    } else {
        /* Skip this block (old version) */
        gFee_GcProcessedBlocks++;
        gFee_GcStats.blocksDiscarded++;
        gFee_GcScanAddress += FEE_BLOCK_HEADER_SIZE + header.dataLength + 4u;
        gFee_GcScanAddress = (gFee_GcScanAddress + 3u) & ~3u;
    }

    return FEE_OK;
}

/**
 * @brief GC State: COPY_BLOCK
 */
static Fee_ErrorCode_t Fee_Gc_StateCopyBlock(void)
{
    Fee_BlockHeader_t header;
    Fls_ErrorCode_t result;
    uint32_t totalSize;
    uint32_t targetSize;
    const Fee_ClusterConfigType* target;

    /* Read header */
    result = Fls_Read(gFee_GcScanAddress, sizeof(Fee_BlockHeader_t), (uint8_t*)&header);
    if (result != FLS_OK) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_READ_FAILED;
    }

    totalSize = FEE_BLOCK_HEADER_SIZE + header.dataLength + 4u;

    /* Check target space */
    target = Fee_Gc_GetCluster(gFee_GcTargetCluster);
    if (target == NULL) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_INVALID_CLUSTER;
    }

    if ((gFee_GcWriteAddress + totalSize) > (target->startAddress + target->size)) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_OUT_OF_MEMORY;
    }

    /* Read entire block */
    if (totalSize > sizeof(gFee_GcBlockBuffer)) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_INVALID_BLOCK_LEN;
    }

    result = Fls_Read(gFee_GcScanAddress, totalSize, gFee_GcBlockBuffer);
    if (result != FLS_OK) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_READ_FAILED;
    }

    /* Write to target */
    result = Fls_Write(gFee_GcWriteAddress, totalSize, gFee_GcBlockBuffer);
    if (result != FLS_OK) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_WRITE_FAILED;
    }

    /* Update write address */
    gFee_GcWriteAddress += totalSize;
    gFee_GcWriteAddress = (gFee_GcWriteAddress + 3u) & ~3u;

    /* Update stats */
    gFee_GcCopiedBlocks++;
    gFee_GcStats.blocksCopied++;
    gFee_GcStats.bytesReclaimed += totalSize;

    /* Move to next block */
    gFee_GcProcessedBlocks++;
    gFee_GcScanAddress += totalSize;
    gFee_GcScanAddress = (gFee_GcScanAddress + 3u) & ~3u;

    gFee_GcState = FEE_GC_STATE_SCAN_SOURCE;

    return FEE_OK;
}

/**
 * @brief GC State: ERASE_SOURCE
 */
static Fee_ErrorCode_t Fee_Gc_StateEraseSource(void)
{
    const Fee_ClusterConfigType* source;
    Fls_ErrorCode_t result;

    source = Fee_Gc_GetCluster(gFee_GcSourceCluster);
    if (source == NULL) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_INVALID_CLUSTER;
    }

    /* Erase source cluster */
    result = Fls_Erase(source->startAddress, source->size);
    if (result != FLS_OK) {
        gFee_GcState = FEE_GC_STATE_ERROR;
        return FEE_E_ERASE_FAILED;
    }

    /* Update erase count */
    ((Fee_ClusterConfigType*)source)->eraseCount++;

    gFee_GcState = FEE_GC_STATE_FINALIZE;

    return FEE_OK;
}

/**
 * @brief GC State: FINALIZE
 */
static Fee_ErrorCode_t Fee_Gc_StateFinalize(void)
{
    const Fee_ClusterConfigType* source;
    const Fee_ClusterConfigType* target;

    source = Fee_Gc_GetCluster(gFee_GcSourceCluster);
    target = Fee_Gc_GetCluster(gFee_GcTargetCluster);

    if ((source != NULL) && (target != NULL)) {
        /* Swap cluster roles */
        ((Fee_ClusterConfigType*)source)->status = FEE_CLUSTER_EMPTY;
        ((Fee_ClusterConfigType*)source)->usedSpace = 0u;

        ((Fee_ClusterConfigType*)target)->status = FEE_CLUSTER_ACTIVE;
        ((Fee_ClusterConfigType*)target)->usedSpace = gFee_GcWriteAddress - target->startAddress;
    }

    /* Update stats */
    gFee_GcStats.gcCount++;
    gFee_GcStats.lastGcDuration = 0u; /* TODO: Calculate duration */

    gFee_GcState = FEE_GC_STATE_IDLE;

    return FEE_OK;
}

/*============================================================================*
 * Public API
 *============================================================================*/

Fee_ErrorCode_t Fee_Gc_Init(void)
{
    gFee_GcState = FEE_GC_STATE_IDLE;
    (void)memset(&gFee_GcStats, 0, sizeof(Fee_GcStatistics_t));

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Gc_Start(uint32_t sourceCluster, uint32_t targetCluster)
{
    if (gFee_GcState != FEE_GC_STATE_IDLE) {
        return FEE_E_GC_BUSY;
    }

    gFee_GcSourceCluster = sourceCluster;
    gFee_GcTargetCluster = targetCluster;
    gFee_GcState = FEE_GC_STATE_START;

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Gc_ProcessStep(void)
{
    Fee_ErrorCode_t result = FEE_OK;

    switch (gFee_GcState) {
        case FEE_GC_STATE_IDLE:
        case FEE_GC_STATE_ERROR:
            /* Nothing to do */
            break;

        case FEE_GC_STATE_START:
            result = Fee_Gc_StateStart();
            break;

        case FEE_GC_STATE_SCAN_SOURCE:
            result = Fee_Gc_StateScanSource();
            break;

        case FEE_GC_STATE_COPY_BLOCK:
            result = Fee_Gc_StateCopyBlock();
            break;

        case FEE_GC_STATE_ERASE_SOURCE:
            result = Fee_Gc_StateEraseSource();
            break;

        case FEE_GC_STATE_FINALIZE:
            result = Fee_Gc_StateFinalize();
            break;

        default:
            gFee_GcState = FEE_GC_STATE_ERROR;
            result = FEE_E_NOT_OK;
            break;
    }

    return result;
}

Fee_GcState_t Fee_Gc_GetState(void)
{
    return gFee_GcState;
}

bool Fee_Gc_IsActive(void)
{
    return (gFee_GcState != FEE_GC_STATE_IDLE) && (gFee_GcState != FEE_GC_STATE_ERROR);
}

Fee_ErrorCode_t Fee_Gc_Cancel(void)
{
    if (gFee_GcState == FEE_GC_STATE_IDLE) {
        return FEE_OK;
    }

    /* Can only cancel during scan/copy */
    if ((gFee_GcState == FEE_GC_STATE_SCAN_SOURCE) ||
        (gFee_GcState == FEE_GC_STATE_COPY_BLOCK)) {
        gFee_GcState = FEE_GC_STATE_IDLE;
        return FEE_OK;
    }

    return FEE_E_NOT_OK;
}

bool Fee_Gc_IsNeeded(void)
{
    return (Fee_GetUsedSpacePercent() >= FEE_GC_THRESHOLD_PERCENT);
}

bool Fee_Gc_IsEmergency(void)
{
    return (Fee_GetUsedSpacePercent() >= FEE_GC_EMERGENCY_PERCENT);
}

uint32_t Fee_Gc_GetProgress(void)
{
    return Fee_Gc_CalculateProgress();
}

Fee_ErrorCode_t Fee_Gc_GetStatistics(Fee_GcStatistics_t* stats)
{
    if (stats == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    (void)memcpy(stats, &gFee_GcStats, sizeof(Fee_GcStatistics_t));

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Gc_ResetStatistics(void)
{
    (void)memset(&gFee_GcStats, 0, sizeof(Fee_GcStatistics_t));
    return FEE_OK;
}

uint32_t Fee_Gc_GetRecommendedSource(void)
{
    const Fee_RuntimeStateType* state = Fee_GetRuntimeState();
    uint32_t maxInvalid = 0u;
    uint32_t sourceIndex = 0xFFFFFFFFu;
    uint32_t i;
    uint32_t invalidCount;

    if (state == NULL) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < state->config->clusterCount; i++) {
        if (state->config->clusters[i].status == FEE_CLUSTER_ACTIVE) {
            invalidCount = Fee_Gc_CountInvalidBlocks(i);
            if (invalidCount > maxInvalid) {
                maxInvalid = invalidCount;
                sourceIndex = i;
            }
        }
    }

    return sourceIndex;
}

uint32_t Fee_Gc_GetRecommendedTarget(void)
{
    return Fee_GetLeastWornCluster();
}
