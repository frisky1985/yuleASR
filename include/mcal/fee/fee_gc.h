/**
 * @file fee_gc.h
 * @brief Fee Garbage Collection Header
 * @version 1.0.0
 * @date 2026
 *
 * Garbage Collection for Flash EEPROM Emulation
 */

#ifndef FEE_GC_H
#define FEE_GC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fee_types.h"

/*============================================================================*
 * GC State Machine States
 *============================================================================*/
typedef enum {
    FEE_GC_STATE_IDLE               = 0x00u,    /* GC not active */
    FEE_GC_STATE_START              = 0x01u,    /* Starting GC */
    FEE_GC_STATE_SCAN_SOURCE        = 0x02u,    /* Scanning source cluster */
    FEE_GC_STATE_COPY_BLOCK         = 0x03u,    /* Copying valid block */
    FEE_GC_STATE_ERASE_SOURCE       = 0x04u,    /* Erasing source cluster */
    FEE_GC_STATE_FINALIZE           = 0x05u,    /* Finalizing GC */
    FEE_GC_STATE_ERROR              = 0xFFu     /* GC error */
} Fee_GcState_t;

/*============================================================================*
 * GC Statistics
 *============================================================================*/
typedef struct {
    uint32_t gcCount;                           /* Total GC runs */
    uint32_t blocksCopied;                      /* Total blocks copied */
    uint32_t blocksDiscarded;                   /* Total blocks discarded */
    uint32_t bytesReclaimed;                    /* Total bytes reclaimed */
    uint32_t lastGcDuration;                    /* Last GC duration (ms) */
    uint32_t maxGcDuration;                     /* Max GC duration (ms) */
    uint32_t gcFailures;                        /* GC failure count */
} Fee_GcStatistics_t;

/*============================================================================*
 * GC API
 *============================================================================*/

/**
 * @brief Initialize garbage collection module
 *
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Gc_Init(void);

/**
 * @brief Start garbage collection
 *
 * @param sourceCluster Source cluster index
 * @param targetCluster Target cluster index
 * @return FEE_OK if GC started, FEE_E_GC_BUSY if already running
 */
Fee_ErrorCode_t Fee_Gc_Start(uint32_t sourceCluster, uint32_t targetCluster);

/**
 * @brief Process one GC step (non-blocking)
 *
 * Call this function repeatedly until GC completes.
 *
 * @return FEE_OK if step processed, FEE_E_NOT_OK on error
 */
Fee_ErrorCode_t Fee_Gc_ProcessStep(void);

/**
 * @brief Get current GC state
 *
 * @return Current GC state
 */
Fee_GcState_t Fee_Gc_GetState(void);

/**
 * @brief Check if GC is running
 *
 * @return true if GC is active
 */
bool Fee_Gc_IsActive(void);

/**
 * @brief Cancel ongoing GC
 *
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Gc_Cancel(void);

/**
 * @brief Check if GC is needed
 *
 * @return true if GC should be triggered
 */
bool Fee_Gc_IsNeeded(void);

/**
 * @brief Check if emergency GC is needed
 *
 * @return true if emergency GC should be triggered
 */
bool Fee_Gc_IsEmergency(void);

/**
 * @brief Get GC progress percentage
 *
 * @return Progress (0-100)
 */
uint32_t Fee_Gc_GetProgress(void);

/**
 * @brief Get GC statistics
 *
 * @param stats Pointer to statistics structure
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Gc_GetStatistics(Fee_GcStatistics_t* stats);

/**
 * @brief Reset GC statistics
 *
 * @return FEE_OK on success
 */
Fee_ErrorCode_t Fee_Gc_ResetStatistics(void);

/**
 * @brief Get recommended source cluster for GC
 *
 * @return Cluster index with most invalid blocks
 */
uint32_t Fee_Gc_GetRecommendedSource(void);

/**
 * @brief Get recommended target cluster for GC
 *
 * @return Cluster index (least worn or empty)
 */
uint32_t Fee_Gc_GetRecommendedTarget(void);

#ifdef __cplusplus
}
#endif

#endif /* FEE_GC_H */
