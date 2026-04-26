/**
 * @file fee_wl.h
 * @brief Fee Wear Leveling Header
 * @version 1.0.0
 * @date 2026
 *
 * Wear Leveling for Flash EEPROM Emulation
 */

#ifndef FEE_WL_H
#define FEE_WL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fee_types.h"

/*============================================================================*
 * Wear Leveling Configuration
 *============================================================================*/
#define FEE_WL_THRESHOLD_DEFAULT    1000u   /* Default erase count threshold */
#define FEE_WL_MAX_THRESHOLD        5000u   /* Maximum threshold */
#define FEE_WL_MIN_THRESHOLD        100u    /* Minimum threshold */

/*============================================================================*
 * Wear Leveling Statistics
 *============================================================================*/
typedef struct {
    uint32_t minEraseCount;                     /* Minimum erase count */
    uint32_t maxEraseCount;                     /* Maximum erase count */
    uint32_t avgEraseCount;                     /* Average erase count */
    uint32_t maxDifference;                     /* Max difference between clusters */
    uint32_t rebalancingCount;                  /* Rebalancing operations */
    uint32_t thresholdExceeded;                 /* Times threshold was exceeded */
} Fee_WlStatistics_t;

/*============================================================================*
 * Wear Leveling API
 *============================================================================*/

/**
 * @brief Initialize wear leveling module
 *
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_Init(void);

/**
 * @brief Set wear leveling threshold
 *
 * @param threshold Erase count difference threshold
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_SetThreshold(uint32_t threshold);

/**
 * @brief Get wear leveling threshold
 *
 * @return Current threshold
 */
uint32_t Fee_Wl_GetThreshold(void);

/**
 * @brief Check if rebalancing is needed
 *
 * @return true if rebalancing should be triggered
 */
bool Fee_Wl_IsRebalancingNeeded(void);

/**
 * @brief Perform wear leveling rebalancing
 *
 * Swaps data between most worn and least worn clusters
 * to equalize erase counts.
 *
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_Rebalance(void);

/**
 * @brief Update erase count for cluster
 *
 * @param clusterIndex Cluster index
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_UpdateEraseCount(uint32_t clusterIndex);

/**
 * @brief Get cluster with minimum erase count
 *
 * @return Cluster index, 0xFFFFFFFF if error
 */
uint32_t Fee_Wl_GetMinWearCluster(void);

/**
 * @brief Get cluster with maximum erase count
 *
 * @return Cluster index, 0xFFFFFFFF if error
 */
uint32_t Fee_Wl_GetMaxWearCluster(void);

/**
 * @brief Get erase count difference between min and max
 *
 * @return Erase count difference
 */
uint32_t Fee_Wl_GetWearDifference(void);

/**
 * @brief Get wear leveling statistics
 *
 * @param stats Pointer to statistics structure
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_GetStatistics(Fee_WlStatistics_t* stats);

/**
 * @brief Reset wear leveling statistics
 *
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_ResetStatistics(void);

/**
 * @brief Get estimated cluster lifetime
 *
 * @param clusterIndex Cluster index
 * @param estimatedWrites Pointer to store estimated remaining writes
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Wl_GetEstimatedLifetime(
    uint32_t clusterIndex,
    uint32_t* estimatedWrites
);

/**
 * @brief Check for cluster health
 *
 * @param clusterIndex Cluster index
 * @return true if cluster is healthy
 */
bool Fee_Wl_IsClusterHealthy(uint32_t clusterIndex);

/**
 * @brief Get recommended write cluster
 *
 * Selects cluster based on wear leveling policy.
 *
 * @return Recommended cluster index
 */
uint32_t Fee_Wl_GetRecommendedWriteCluster(void);

#ifdef __cplusplus
}
#endif

#endif /* FEE_WL_H */
