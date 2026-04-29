/******************************************************************************
 * @file    dcm_response_cache.h
 * @brief   DCM Response Cache Implementation
 *
 * Caches frequently used responses to avoid recomputation.
 * Particularly effective for ReadDataByIdentifier and session responses.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_RESPONSE_CACHE_H
#define DCM_RESPONSE_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Cache Configuration
 ******************************************************************************/
#define DCM_CACHE_MAX_ENTRIES           16U
#define DCM_CACHE_MAX_DATA_SIZE         64U
#define DCM_CACHE_MAX_KEY_SIZE          8U

/******************************************************************************
 * Cache Entry State
 ******************************************************************************/
typedef enum {
    DCM_CACHE_ENTRY_EMPTY = 0,
    DCM_CACHE_ENTRY_VALID,
    DCM_CACHE_ENTRY_STALE
} Dcm_CacheEntryState;

/******************************************************************************
 * Cache Key Structure
 ******************************************************************************/
typedef struct {
    uint8_t data[DCM_CACHE_MAX_KEY_SIZE];
    uint8_t length;
} Dcm_CacheKey;

/******************************************************************************
 * Cache Entry Structure
 ******************************************************************************/
typedef struct {
    Dcm_CacheEntryState state;
    Dcm_CacheKey key;
    uint8_t responseData[DCM_CACHE_MAX_DATA_SIZE];
    uint32_t responseLength;
    uint32_t hitCount;                  /* For LRU eviction */
    uint32_t timestamp;                 /* For TTL expiration */
    uint32_t ttlMs;                     /* Time to live in milliseconds */
    bool isNegativeResponse;
    uint8_t negativeResponseCode;
} Dcm_CacheEntry;

/******************************************************************************
 * Cache Statistics
 ******************************************************************************/
typedef struct {
    uint32_t hitCount;                  /* Cache hits */
    uint32_t missCount;                 /* Cache misses */
    uint32_t evictionCount;             /* Entries evicted */
    uint32_t insertCount;               /* Entries inserted */
    uint32_t invalidationCount;         /* Entries invalidated */
    uint32_t currentSize;               /* Current number of entries */
    uint32_t maxSize;                   /* Maximum entries */
    uint32_t hitRate;                   /* Hit rate percentage (0-100) */
    uint32_t avgLookupTime;             /* Average lookup time in microseconds */
} Dcm_CacheStats;

/******************************************************************************
 * Cache Configuration Type
 ******************************************************************************/
typedef struct {
    uint32_t defaultTtlMs;              /* Default TTL for entries */
    uint32_t maxTtlMs;                  /* Maximum allowed TTL */
    bool enableAutoInvalidate;          /* Auto-invalidate on session change */
    uint32_t maxEntries;                /* Maximum cache entries */
} Dcm_CacheConfig;

/******************************************************************************
 * Response Cache API
 ******************************************************************************/

/**
 * @brief Initialize response cache
 * @param config Pointer to cache configuration
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheInit(const Dcm_CacheConfig *config);

/**
 * @brief Store response in cache
 * @param key Cache key
 * @param response Pointer to response data
 * @param ttlMs Time to live in milliseconds (0 for default)
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheStore(const Dcm_CacheKey *key,
                              const Dcm_ResponseType *response,
                              uint32_t ttlMs);

/**
 * @brief Lookup response in cache
 * @param key Cache key
 * @param response Pointer to store cached response
 * @return Dcm_ReturnType DCM_E_OK if found, DCM_E_NOT_OK if not found
 */
Dcm_ReturnType Dcm_CacheLookup(const Dcm_CacheKey *key,
                               Dcm_ResponseType *response);

/**
 * @brief Invalidate cache entry
 * @param key Cache key
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheInvalidate(const Dcm_CacheKey *key);

/**
 * @brief Invalidate all cache entries
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheInvalidateAll(void);

/**
 * @brief Invalidate entries by session
 * @param session Session type
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheInvalidateBySession(Dcm_SessionType session);

/**
 * @brief Build cache key from request
 * @param serviceId Service ID
 * @param request Request data
 * @param key Pointer to store key
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheBuildKey(uint8_t serviceId,
                                 const Dcm_RequestType *request,
                                 Dcm_CacheKey *key);

/**
 * @brief Check if service is cacheable
 * @param serviceId Service ID
 * @return bool True if cacheable
 */
bool Dcm_CacheIsServiceCacheable(uint8_t serviceId);

/**
 * @brief Get cache statistics
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheGetStats(Dcm_CacheStats *stats);

/**
 * @brief Update cache TTLs (call periodically)
 * @param elapsedTimeMs Time elapsed since last call
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CacheUpdateTtl(uint32_t elapsedTimeMs);

/**
 * @brief Pre-populate cache with common responses
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_CachePrepopulate(void);

#ifdef __cplusplus
}
#endif

#endif /* DCM_RESPONSE_CACHE_H */
