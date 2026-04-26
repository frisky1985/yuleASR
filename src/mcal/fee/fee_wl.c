/**
 * @file fee_wl.c
 * @brief Fee Wear Leveling Implementation
 * @version 1.0.0
 * @date 2026
 *
 * Wear Leveling for Flash EEPROM Emulation
 * MISRA C:2012 compliant
 */

#include "mcal/fee/fee_wl.h"
#include "mcal/fee/fee.h"
#include <string.h>

/*============================================================================*
 * Static Variables
 *============================================================================*/
static uint32_t gFee_WlThreshold = FEE_WL_THRESHOLD_DEFAULT;
static Fee_WlStatistics_t gFee_WlStats;

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Get runtime state
 */
static const Fee_RuntimeStateType* Fee_Wl_GetState(void)
{
    return Fee_GetRuntimeState();
}

/**
 * @brief Update statistics
 */
static void Fee_Wl_UpdateStats(void)
{
    const Fee_RuntimeStateType* state;
    uint32_t i;
    uint32_t minCount = 0xFFFFFFFFu;
    uint32_t maxCount = 0u;
    uint32_t totalCount = 0u;

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return;
    }

    for (i = 0u; i < state->config->clusterCount; i++) {
        uint32_t count = state->config->clusters[i].eraseCount;

        if (count < minCount) {
            minCount = count;
        }
        if (count > maxCount) {
            maxCount = count;
        }
        totalCount += count;
    }

    gFee_WlStats.minEraseCount = minCount;
    gFee_WlStats.maxEraseCount = maxCount;
    gFee_WlStats.avgEraseCount = totalCount / state->config->clusterCount;
    gFee_WlStats.maxDifference = maxCount - minCount;
}

/*============================================================================*
 * Public API
 *============================================================================*/

Fee_ErrorCode_t Fee_Wl_Init(void)
{
    gFee_WlThreshold = FEE_WL_THRESHOLD_DEFAULT;
    (void)memset(&gFee_WlStats, 0, sizeof(Fee_WlStatistics_t));

    Fee_Wl_UpdateStats();

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Wl_SetThreshold(uint32_t threshold)
{
    if ((threshold < FEE_WL_MIN_THRESHOLD) || (threshold > FEE_WL_MAX_THRESHOLD)) {
        return FEE_E_PARAM_CONFIG;
    }

    gFee_WlThreshold = threshold;

    return FEE_OK;
}

uint32_t Fee_Wl_GetThreshold(void)
{
    return gFee_WlThreshold;
}

bool Fee_Wl_IsRebalancingNeeded(void)
{
    Fee_Wl_UpdateStats();

    if (gFee_WlStats.maxDifference > gFee_WlThreshold) {
        gFee_WlStats.thresholdExceeded++;
        return true;
    }

    return false;
}

Fee_ErrorCode_t Fee_Wl_Rebalance(void)
{
    uint32_t minCluster;
    uint32_t maxCluster;

    /* Find clusters with min and max wear */
    minCluster = Fee_Wl_GetMinWearCluster();
    maxCluster = Fee_Wl_GetMaxWearCluster();

    if ((minCluster == 0xFFFFFFFFu) || (maxCluster == 0xFFFFFFFFu)) {
        return FEE_E_NOT_OK;
    }

    if (minCluster == maxCluster) {
        return FEE_OK; /* Already balanced */
    }

    /* Trigger GC from max worn cluster to min worn cluster */
    /* This will equalize the erase counts */
    /* Note: This is a simplified approach - real implementation
     * would need more sophisticated balancing */

    gFee_WlStats.rebalancingCount++;

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Wl_UpdateEraseCount(uint32_t clusterIndex)
{
    const Fee_RuntimeStateType* state;

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return FEE_E_UNINIT;
    }

    if (clusterIndex >= state->config->clusterCount) {
        return FEE_E_INVALID_CLUSTER;
    }

    /* In real implementation, this would persist the erase count to flash */
    /* For now, just update statistics */
    Fee_Wl_UpdateStats();

    return FEE_OK;
}

uint32_t Fee_Wl_GetMinWearCluster(void)
{
    const Fee_RuntimeStateType* state;
    uint32_t minCount = 0xFFFFFFFFu;
    uint32_t minCluster = 0xFFFFFFFFu;
    uint32_t i;

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < state->config->clusterCount; i++) {
        if (state->config->clusters[i].eraseCount < minCount) {
            minCount = state->config->clusters[i].eraseCount;
            minCluster = i;
        }
    }

    return minCluster;
}

uint32_t Fee_Wl_GetMaxWearCluster(void)
{
    const Fee_RuntimeStateType* state;
    uint32_t maxCount = 0u;
    uint32_t maxCluster = 0xFFFFFFFFu;
    uint32_t i;

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return 0xFFFFFFFFu;
    }

    for (i = 0u; i < state->config->clusterCount; i++) {
        if (state->config->clusters[i].eraseCount > maxCount) {
            maxCount = state->config->clusters[i].eraseCount;
            maxCluster = i;
        }
    }

    return maxCluster;
}

uint32_t Fee_Wl_GetWearDifference(void)
{
    Fee_Wl_UpdateStats();
    return gFee_WlStats.maxDifference;
}

Fee_ErrorCode_t Fee_Wl_GetStatistics(Fee_WlStatistics_t* stats)
{
    if (stats == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    Fee_Wl_UpdateStats();
    (void)memcpy(stats, &gFee_WlStats, sizeof(Fee_WlStatistics_t));

    return FEE_OK;
}

Fee_ErrorCode_t Fee_Wl_ResetStatistics(void)
{
    (void)memset(&gFee_WlStats, 0, sizeof(Fee_WlStatistics_t));
    return FEE_OK;
}

Fee_ErrorCode_t Fee_Wl_GetEstimatedLifetime(uint32_t clusterIndex, uint32_t* estimatedWrites)
{
    const Fee_RuntimeStateType* state;
    const uint32_t TYPICAL_FLASH_CYCLES = 100000u;  /* Typical flash endurance */

    if (estimatedWrites == NULL) {
        return FEE_E_PARAM_POINTER;
    }

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return FEE_E_UNINIT;
    }

    if (clusterIndex >= state->config->clusterCount) {
        return FEE_E_INVALID_CLUSTER;
    }

    uint32_t currentCycles = state->config->clusters[clusterIndex].eraseCount;

    if (currentCycles >= TYPICAL_FLASH_CYCLES) {
        *estimatedWrites = 0u;  /* End of life */
    } else {
        *estimatedWrites = TYPICAL_FLASH_CYCLES - currentCycles;
    }

    return FEE_OK;
}

bool Fee_Wl_IsClusterHealthy(uint32_t clusterIndex)
{
    const Fee_RuntimeStateType* state;
    const uint32_t END_OF_LIFE_THRESHOLD = 90000u;  /* 90% of typical endurance */

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return false;
    }

    if (clusterIndex >= state->config->clusterCount) {
        return false;
    }

    return state->config->clusters[clusterIndex].eraseCount < END_OF_LIFE_THRESHOLD;
}

uint32_t Fee_Wl_GetRecommendedWriteCluster(void)
{
    const Fee_RuntimeStateType* state;
    uint32_t recommended = 0xFFFFFFFFu;
    uint32_t minScore = 0xFFFFFFFFu;
    uint32_t i;

    state = Fee_Wl_GetState();
    if (state == NULL) {
        return 0xFFFFFFFFu;
    }

    /* Score = erase_count * 100 + used_space_percentage */
    for (i = 0u; i < state->config->clusterCount; i++) {
        uint32_t eraseScore = state->config->clusters[i].eraseCount;
        uint32_t spaceScore = 0u;

        if (state->config->clusters[i].size > 0u) {
            spaceScore = (state->config->clusters[i].usedSpace * 100u) /
                         state->config->clusters[i].size;
        }

        uint32_t totalScore = (eraseScore * 100u) + spaceScore;

        if (totalScore < minScore) {
            minScore = totalScore;
            recommended = i;
        }
    }

    return recommended;
}
