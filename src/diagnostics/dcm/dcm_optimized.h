/******************************************************************************
 * @file    dcm_optimized.h
 * @brief   DCM Optimized Main Interface
 *
 * High-performance DCM implementation with:
 * - Priority queue for pending operations
 * - Response caching
 * - Fast paths for common services
 * - Zero-copy optimizations
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_OPTIMIZED_H
#define DCM_OPTIMIZED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"
#include "dcm_session.h"
#include "dcm_ecu_reset.h"
#include "dcm_security.h"
#include "dcm_communication.h"
#include "dcm_dynamic_did.h"
#include "dcm_memory.h"
#include "dcm_routine.h"
#include "dcm_priority_queue.h"
#include "dcm_response_cache.h"

/******************************************************************************
 * Optimized DCM Configuration
 ******************************************************************************/
typedef struct {
    /* Base configuration */
    const Dcm_ProtocolConfigType *protocolConfigs;
    uint8_t numProtocols;
    const Dcm_SessionControlConfigType *sessionConfig;
    const Dcm_EcuResetConfigType *ecuResetConfig;
    const Dcm_SecurityAccessConfigType *securityConfig;
    const Dcm_CommunicationControlConfigType *commControlConfig;
    const Dcm_DynamicDidConfigType *dynamicDidConfig;
    const Dcm_MemoryWriteConfigType *memoryWriteConfig;
    const Dcm_RoutineConfigType *routineConfigs;
    uint8_t numRoutines;
    
    /* Optimization settings */
    bool enablePriorityQueue;           /* Enable priority queue processing */
    bool enableResponseCache;           /* Enable response caching */
    bool enableFastPath;                /* Enable fast path for common services */
    bool enableZeroCopy;                /* Enable zero-copy optimizations */
    uint8_t maxPendingRequests;         /* Maximum pending requests */
    uint32_t fastPathMask;              /* Bitmask of fast-path services */
    
    /* Cache configuration */
    Dcm_CacheConfig cacheConfig;
} Dcm_OptimizedConfigType;

/******************************************************************************
 * Performance Statistics
 ******************************************************************************/
typedef struct {
    /* Request statistics */
    uint32_t totalRequests;
    uint32_t fastPathHits;
    uint32_t cacheHits;
    uint32_t priorityQueueInserts;
    uint32_t priorityQueueRemoves;
    
    /* Timing statistics */
    uint32_t avgProcessingTime;         /* Average request processing time (us) */
    uint32_t maxProcessingTime;         /* Maximum processing time observed */
    uint32_t minProcessingTime;         /* Minimum processing time observed */
    
    /* Memory statistics */
    uint32_t copyOperationsSaved;       /* Number of copy operations avoided */
    uint32_t bytesCopied;               /* Total bytes copied */
    uint32_t bytesAvoided;              /* Total bytes not copied (zero-copy) */
    
    /* Service statistics */
    uint32_t serviceCounts[32];         /* Per-service request counts */
    uint32_t serviceTimes[32];          /* Per-service processing times */
} Dcm_PerformanceStats;

/******************************************************************************
 * Optimized DCM API
 ******************************************************************************/

/**
 * @brief Initialize optimized DCM module
 * @param config Pointer to optimized configuration
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_OptimizedInit(const Dcm_OptimizedConfigType *config);

/**
 * @brief Deinitialize optimized DCM module
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_OptimizedDeInit(void);

/**
 * @brief Main function - optimized processing
 * @param elapsedTimeMs Time elapsed since last call in milliseconds
 */
void Dcm_OptimizedMainFunction(uint32_t elapsedTimeMs);

/**
 * @brief Process incoming diagnostic request (optimized)
 * @param request Pointer to request message
 * @param response Pointer to response message buffer
 * @return Dcm_ReturnType Processing result
 */
Dcm_ReturnType Dcm_OptimizedProcessRequest(const Dcm_RequestType *request,
                                           Dcm_ResponseType *response);

/**
 * @brief Process request via priority queue
 * @return Dcm_ReturnType DCM_E_OK if request processed
 */
Dcm_ReturnType Dcm_OptimizedProcessQueue(void);

/**
 * @brief Get optimized DCM status
 * @return bool True if initialized
 */
bool Dcm_OptimizedIsInitialized(void);

/**
 * @brief Get performance statistics
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_OptimizedGetStats(Dcm_PerformanceStats *stats);

/**
 * @brief Reset performance statistics
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_OptimizedResetStats(void);

/**
 * @brief Enable/disable fast path for a service
 * @param serviceId Service ID
 * @param enable True to enable, false to disable
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_OptimizedSetFastPath(uint8_t serviceId, bool enable);

/**
 * @brief Flush priority queue and process all pending requests
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_OptimizedFlushQueue(void);

/**
 * @brief Get queue depth
 * @return uint8_t Current number of pending requests
 */
uint8_t Dcm_OptimizedGetQueueDepth(void);

/**
 * @brief Check if queue has pending high-priority requests
 * @return bool True if high-priority requests pending
 */
bool Dcm_OptimizedHasUrgentRequests(void);

#ifdef __cplusplus
}
#endif

#endif /* DCM_OPTIMIZED_H */
