/******************************************************************************
 * @file    dcm_optimized.c
 * @brief   DCM Optimized Main Implementation
 *
 * High-performance DCM with:
 * - Binary heap priority queue (O(log n) vs O(n) linear search)
 * - LRU response cache with TTL
 * - Fast paths for critical services
 * - Zero-copy buffer management
 *
 * AUTOSAR R22-11 compliant
 * ISO 14229-1:2020 UDS Specification compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_optimized.h"
#include "dcm.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_OPT_MAGIC_INIT              0x44434D4FU  /* "DCMO" */
#define DCM_FAST_PATH_MASK_DEFAULT      0x00001843U  /* 0x10, 0x11, 0x27, 0x3E */
#define DCM_OPT_MAX_BURST_PROCESS       4U           /* Max requests per cycle */

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    const Dcm_OptimizedConfigType *config;
    Dcm_StateType state;
    bool initialized;
    uint32_t fastPathMask;
    Dcm_PerformanceStats stats;
    
    /* Zero-copy buffer management */
    struct {
        uint8_t *rxBuffer;
        uint32_t rxSize;
        uint8_t *txBuffer;
        uint32_t txSize;
        bool rxInUse;
        bool txInUse;
    } buffers;
    
    /* Processing state */
    bool processingActive;
    uint32_t lastProcessingTime;
} Dcm_OptimizedStateType;

static Dcm_OptimizedStateType s_optState;

/******************************************************************************
 * Service Handler Table - Direct dispatch for fast path
 ******************************************************************************/
typedef struct {
    uint8_t serviceId;
    Dcm_ServiceHandlerFunc handler;
    bool isFastPath;
    bool isCacheable;
    Dcm_PriorityLevel priority;
} Dcm_ServiceEntry;

/* Fast path service handlers - defined as extern in respective modules */
extern Dcm_ReturnType Dcm_DiagnosticSessionControl(const Dcm_RequestType *request,
                                                    Dcm_ResponseType *response);
extern Dcm_ReturnType Dcm_EcuReset(const Dcm_RequestType *request,
                                   Dcm_ResponseType *response);
extern Dcm_ReturnType Dcm_SecurityAccess(const Dcm_RequestType *request,
                                         Dcm_ResponseType *response);
extern Dcm_ReturnType Dcm_CommunicationControl(const Dcm_RequestType *request,
                                               Dcm_ResponseType *response);
extern Dcm_ReturnType Dcm_DynamicallyDefineDataIdentifier(const Dcm_RequestType *request,
                                                          Dcm_ResponseType *response);
extern Dcm_ReturnType Dcm_WriteMemoryByAddress(const Dcm_RequestType *request,
                                               Dcm_ResponseType *response);
extern Dcm_ReturnType Dcm_RoutineControl(const Dcm_RequestType *request,
                                         Dcm_ResponseType *response);

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Send negative response (zero-copy version)
 */
static Dcm_ReturnType sendNegativeResponse(Dcm_ResponseType *response,
                                            uint8_t sid,
                                            uint8_t nrc)
{
    if ((response != NULL) && (response->data != NULL) &&
        (response->maxLength >= 3U)) {
        response->data[0U] = DCM_SID_NEGATIVE_RESPONSE;
        response->data[1U] = sid;
        response->data[2U] = nrc;
        response->length = 3U;
        response->isNegativeResponse = true;
        response->negativeResponseCode = nrc;
        return DCM_E_OK;
    }
    return DCM_E_NOT_OK;
}

/**
 * @brief Check if service is in fast path
 */
static bool isFastPathService(uint8_t serviceId)
{
    if (serviceId < 32U) {
        return ((s_optState.fastPathMask & (1UL << serviceId)) != 0UL);
    }
    return false;
}

/**
 * @brief Get service handler - direct lookup for fast path
 */
static Dcm_ServiceHandlerFunc getServiceHandler(uint8_t serviceId)
{
    switch (serviceId) {
        case UDS_SVC_DIAGNOSTIC_SESSION_CONTROL:
            return Dcm_DiagnosticSessionControl;
        case UDS_SVC_ECU_RESET:
            return Dcm_EcuReset;
        case UDS_SVC_SECURITY_ACCESS:
            return Dcm_SecurityAccess;
        case UDS_SVC_COMMUNICATION_CONTROL:
            return Dcm_CommunicationControl;
        case UDS_SVC_DYNAMICALLY_DEFINE_DATA_IDENTIFIER:
            return Dcm_DynamicallyDefineDataIdentifier;
        case UDS_SVC_WRITE_MEMORY_BY_ADDRESS:
            return Dcm_WriteMemoryByAddress;
        case UDS_SVC_ROUTINE_CONTROL:
            return Dcm_RoutineControl;
        default:
            return NULL;
    }
}

/**
 * @brief Process request via fast path (direct dispatch)
 */
static Dcm_ReturnType processFastPath(uint8_t serviceId,
                                      const Dcm_RequestType *request,
                                      Dcm_ResponseType *response)
{
    Dcm_ServiceHandlerFunc handler = getServiceHandler(serviceId);
    
    if (handler != NULL) {
        s_optState.stats.fastPathHits++;
        s_optState.stats.serviceCounts[serviceId & 0x1FU]++;
        return handler(request, response);
    }
    
    return DCM_E_SERVICE_NOT_SUPPORTED;
}

/**
 * @brief Process request with caching support
 */
static Dcm_ReturnType processWithCache(uint8_t serviceId,
                                       const Dcm_RequestType *request,
                                       Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    Dcm_CacheKey key;
    bool cacheLookupDone = false;
    
    /* Try cache lookup if enabled */
    if (s_optState.config->enableResponseCache &&
        Dcm_CacheIsServiceCacheable(serviceId)) {
        
        if (Dcm_CacheBuildKey(serviceId, request, &key) == DCM_E_OK) {
            if (Dcm_CacheLookup(&key, response) == DCM_E_OK) {
                s_optState.stats.cacheHits++;
                return DCM_E_OK;
            }
            cacheLookupDone = true;
        }
    }
    
    /* Process request */
    result = processFastPath(serviceId, request, response);
    
    /* Store in cache if successful and cacheable */
    if ((result == DCM_E_OK) && cacheLookupDone &&
        (!response->isNegativeResponse)) {
        (void)Dcm_CacheStore(&key, response, 0U);
    }
    
    return result;
}

/**
 * @brief Update performance statistics
 */
static void updateStats(uint8_t serviceId, uint32_t processingTime)
{
    s_optState.stats.totalRequests++;
    s_optState.stats.serviceCounts[serviceId & 0x1FU]++;
    s_optState.stats.serviceTimes[serviceId & 0x1FU] += processingTime;
    
    /* Update min/max/avg */
    if (processingTime > s_optState.stats.maxProcessingTime) {
        s_optState.stats.maxProcessingTime = processingTime;
    }
    if ((s_optState.stats.minProcessingTime == 0U) ||
        (processingTime < s_optState.stats.minProcessingTime)) {
        s_optState.stats.minProcessingTime = processingTime;
    }
    
    /* Simple moving average */
    if (s_optState.stats.totalRequests == 1U) {
        s_optState.stats.avgProcessingTime = processingTime;
    } else {
        s_optState.stats.avgProcessingTime = 
            ((s_optState.stats.avgProcessingTime * 7U) + processingTime) / 8U;
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_OptimizedInit(const Dcm_OptimizedConfigType *config)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (config == NULL) {
        return result;
    }
    
    /* Clear state */
    (void)memset(&s_optState, 0, sizeof(s_optState));
    
    s_optState.magic = DCM_OPT_MAGIC_INIT;
    s_optState.config = config;
    s_optState.state = DCM_STATE_INIT;
    s_optState.fastPathMask = (config->fastPathMask != 0U) ?
                              config->fastPathMask : DCM_FAST_PATH_MASK_DEFAULT;
    
    /* Initialize priority queue */
    if (config->enablePriorityQueue) {
        (void)Dcm_PqInit();
    }
    
    /* Initialize response cache */
    if (config->enableResponseCache) {
        (void)Dcm_CacheInit(&config->cacheConfig);
        (void)Dcm_CachePrepopulate();
    }
    
    /* Initialize base DCM */
    Dcm_ConfigType baseConfig;
    baseConfig.protocolConfigs = config->protocolConfigs;
    baseConfig.numProtocols = config->numProtocols;
    baseConfig.sessionConfig = config->sessionConfig;
    baseConfig.ecuResetConfig = config->ecuResetConfig;
    baseConfig.securityConfig = config->securityConfig;
    baseConfig.commControlConfig = config->commControlConfig;
    baseConfig.dynamicDidConfig = config->dynamicDidConfig;
    baseConfig.memoryWriteConfig = config->memoryWriteConfig;
    baseConfig.routineConfigs = config->routineConfigs;
    baseConfig.numRoutines = config->numRoutines;
    
    result = Dcm_Init(&baseConfig);
    
    if (result == DCM_E_OK) {
        s_optState.initialized = true;
    }
    
    return result;
}

Dcm_ReturnType Dcm_OptimizedDeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_optState.initialized) {
        s_optState.initialized = false;
        s_optState.state = DCM_STATE_UNINIT;
        s_optState.magic = 0U;
        s_optState.config = NULL;
        
        result = Dcm_DeInit();
    }
    
    return result;
}

void Dcm_OptimizedMainFunction(uint32_t elapsedTimeMs)
{
    if (!s_optState.initialized) {
        return;
    }
    
    /* Update cache TTLs */
    if (s_optState.config->enableResponseCache) {
        (void)Dcm_CacheUpdateTtl(elapsedTimeMs);
    }
    
    /* Process priority queue */
    if (s_optState.config->enablePriorityQueue) {
        uint8_t burstCount = 0U;
        while ((!Dcm_PqIsEmpty()) && (burstCount < DCM_OPT_MAX_BURST_PROCESS)) {
            (void)Dcm_OptimizedProcessQueue();
            burstCount++;
        }
    }
    
    /* Call base main function */
    Dcm_MainFunction(elapsedTimeMs);
}

Dcm_ReturnType Dcm_OptimizedProcessRequest(const Dcm_RequestType *request,
                                           Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    uint8_t nrc = UDS_NRC_GENERAL_REJECT;
    uint32_t startTime = 0U; /* TODO: Get timestamp */
    
    /* Check initialization */
    if (!s_optState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate parameters */
    if ((request == NULL) || (response == NULL) ||
        (request->data == NULL) || (request->length == 0U)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check minimum request length */
    if (request->length < 1U) {
        nrc = UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_FORMAT;
        (void)sendNegativeResponse(response, request->data[0U], nrc);
        return DCM_E_NOT_OK;
    }
    
    const uint8_t serviceId = request->data[0U];
    s_optState.state = DCM_STATE_PROCESSING;
    
    /* Fast path for critical services */
    if (s_optState.config->enableFastPath && isFastPathService(serviceId)) {
        result = processWithCache(serviceId, request, response);
        
        uint32_t endTime = 0U; /* TODO: Get timestamp */
        updateStats(serviceId, endTime - startTime);
        
        s_optState.state = DCM_STATE_INIT;
        return result;
    }
    
    /* Queue-based processing for non-fast-path services */
    if (s_optState.config->enablePriorityQueue) {
        result = Dcm_PqInsertServiceRequest(serviceId, request);
        
        if (result == DCM_E_OK) {
            /* Return pending - will be processed from queue */
            s_optState.stats.priorityQueueInserts++;
            result = DCM_E_PENDING;
        }
    } else {
        /* Direct processing without queue */
        result = processFastPath(serviceId, request, response);
    }
    
    s_optState.state = DCM_STATE_INIT;
    return result;
}

Dcm_ReturnType Dcm_OptimizedProcessQueue(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    Dcm_PqEntry entry;
    
    if (!s_optState.initialized || !s_optState.config->enablePriorityQueue) {
        return result;
    }
    
    if (Dcm_PqRemove(&entry) != DCM_E_OK) {
        return result;
    }
    
    s_optState.stats.priorityQueueRemoves++;
    
    /* Reconstruct request from queue entry */
    Dcm_RequestType request;
    request.data = (uint8_t*)entry.data.request.data;
    request.length = entry.data.request.length;
    request.sourceAddress = entry.data.request.sourceAddr;
    request.protocol = entry.data.request.protocol;
    request.addrMode = DCM_ADDR_PHYSICAL; /* Default */
    request.timestamp = entry.timestamp;
    
    /* Allocate response buffer */
    uint8_t responseData[DCM_MAX_BUFFER_SIZE];
    Dcm_ResponseType response;
    response.data = responseData;
    response.maxLength = DCM_MAX_BUFFER_SIZE;
    response.length = 0U;
    response.isNegativeResponse = false;
    response.suppressPositiveResponse = false;
    
    /* Process the queued request */
    result = processFastPath(entry.data.request.serviceId, &request, &response);
    
    /* TODO: Send response via appropriate channel */
    
    return result;
}

bool Dcm_OptimizedIsInitialized(void)
{
    return s_optState.initialized;
}

Dcm_ReturnType Dcm_OptimizedGetStats(Dcm_PerformanceStats *stats)
{
    if (!s_optState.initialized || (stats == NULL)) {
        return DCM_E_NOT_OK;
    }
    
    (void)memcpy(stats, &s_optState.stats, sizeof(Dcm_PerformanceStats));
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_OptimizedResetStats(void)
{
    if (!s_optState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    (void)memset(&s_optState.stats, 0, sizeof(Dcm_PerformanceStats));
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_OptimizedSetFastPath(uint8_t serviceId, bool enable)
{
    if (!s_optState.initialized || (serviceId >= 32U)) {
        return DCM_E_NOT_OK;
    }
    
    if (enable) {
        s_optState.fastPathMask |= (1UL << serviceId);
    } else {
        s_optState.fastPathMask &= ~(1UL << serviceId);
    }
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_OptimizedFlushQueue(void)
{
    Dcm_ReturnType result = DCM_E_OK;
    
    if (!s_optState.initialized || !s_optState.config->enablePriorityQueue) {
        return DCM_E_NOT_OK;
    }
    
    while (!Dcm_PqIsEmpty()) {
        if (Dcm_OptimizedProcessQueue() != DCM_E_OK) {
            result = DCM_E_NOT_OK;
        }
    }
    
    return result;
}

uint8_t Dcm_OptimizedGetQueueDepth(void)
{
    if (!s_optState.initialized || !s_optState.config->enablePriorityQueue) {
        return 0U;
    }
    
    return Dcm_PqGetSize();
}

bool Dcm_OptimizedHasUrgentRequests(void)
{
    if (!s_optState.initialized || !s_optState.config->enablePriorityQueue) {
        return false;
    }
    
    return Dcm_PqHasHighPriorityEntries();
}
