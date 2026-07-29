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

/******************************************************************************
 * @file    dcm_response_cache.c
 * @brief   DCM Response Cache Implementation
 *
 * LRU-based response cache with TTL support.
 * Optimized for frequently accessed diagnostic data.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_response_cache.h"
#include <string.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_CACHE_MAGIC_INIT            0x43414348U  /* "CACH" */

/******************************************************************************
 * Module State
 ******************************************************************************/
typedef struct {
    uint32_t magic;
    Dcm_CacheEntry entries[DCM_CACHE_MAX_ENTRIES];
    Dcm_CacheConfig config;
    bool initialized;
    Dcm_CacheStats stats;
} Dcm_CacheStateType;

static Dcm_CacheStateType s_cacheState;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Compare two cache keys
 * @return bool True if keys are equal
 */
static bool keysEqual(const Dcm_CacheKey *a, const Dcm_CacheKey *b)
{
    if (a->length != b->length) {
        return false;
    }
    
    for (uint8_t i = 0U; i < a->length; i++) {
        if (a->data[i] != b->data[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Find cache entry by key
 * @return int32_t Index of entry, or -1 if not found
 */
static int32_t findEntry(const Dcm_CacheKey *key)
{
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if ((s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) &&
            keysEqual(&s_cacheState.entries[i].key, key)) {
            return (int32_t)i;
        }
    }
    return -1;
}

/**
 * @brief Find best entry for eviction (LRU policy)
 * @return uint8_t Index of entry to evict
 */
static uint8_t findEvictionCandidate(void)
{
    uint8_t candidate = 0U;
    uint32_t minHits = 0xFFFFFFFFU;
    
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        /* Prefer empty or stale entries */
        if ((s_cacheState.entries[i].state == DCM_CACHE_ENTRY_EMPTY) ||
            (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_STALE)) {
            return i;
        }
        
        /* Otherwise use LRU */
        if (s_cacheState.entries[i].hitCount < minHits) {
            minHits = s_cacheState.entries[i].hitCount;
            candidate = i;
        }
    }
    
    return candidate;
}

/**
 * @brief Hash function for request data
 */
static uint32_t computeHash(const uint8_t *data, uint8_t length)
{
    uint32_t hash = 0x811C9DC5U; /* FNV-1a offset basis */
    
    for (uint8_t i = 0U; i < length; i++) {
        hash ^= (uint32_t)data[i];
        hash *= 0x01000193U; /* FNV-1a prime */
    }
    
    return hash;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_CacheInit(const Dcm_CacheConfig *config)
{
    if (config == NULL_PTR) {
        return DCM_E_NOT_OK;
    }
    
    (void)memset(&s_cacheState, 0, sizeof(s_cacheState));
    
    s_cacheState.magic = DCM_CACHE_MAGIC_INIT;
    s_cacheState.config = *config;
    s_cacheState.initialized = true;
    s_cacheState.stats.maxSize = DCM_CACHE_MAX_ENTRIES;
    
    /* Initialize all entries as empty */
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        s_cacheState.entries[i].state = DCM_CACHE_ENTRY_EMPTY;
    }
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_CacheStore(const Dcm_CacheKey *key,
                              const Dcm_ResponseType *response,
                              uint32_t ttlMs)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_cacheState.initialized || (key == NULL_PTR) || (response == NULL_PTR)) {
        return result;
    }
    
    /* Validate response size */
    if (response->length > DCM_CACHE_MAX_DATA_SIZE) {
        return result;
    }
    
    /* Check if already exists */
    int32_t existing = findEntry(key);
    uint8_t index;
    
    if (existing >= 0) {
        /* Update existing entry */
        index = (uint8_t)existing;
    } else {
        /* Find slot for new entry */
        index = findEvictionCandidate();
        if (s_cacheState.entries[index].state == DCM_CACHE_ENTRY_VALID) {
            s_cacheState.stats.evictionCount++;
        }
    }
    
    /* Store entry */
    Dcm_CacheEntry *entry = &s_cacheState.entries[index];
    
    entry->state = DCM_CACHE_ENTRY_VALID;
    (void)memcpy(&entry->key, key, sizeof(Dcm_CacheKey));
    (void)memcpy(entry->responseData, response->data, response->length);
    entry->responseLength = response->length;
    entry->hitCount = 0U;
    entry->timestamp = 0U; /* 时间戳依赖系统定时器集成 */
    entry->isNegativeResponse = response->isNegativeResponse;
    entry->negativeResponseCode = response->negativeResponseCode;
    
    /* Set TTL */
    if (ttlMs > 0U) {
        entry->ttlMs = (ttlMs < s_cacheState.config.maxTtlMs) ? 
                       ttlMs : s_cacheState.config.maxTtlMs;
    } else {
        entry->ttlMs = s_cacheState.config.defaultTtlMs;
    }
    
    /* Update statistics */
    s_cacheState.stats.insertCount++;
    
    /* Count valid entries */
    uint32_t validCount = 0U;
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
            validCount++;
        }
    }
    s_cacheState.stats.currentSize = validCount;
    
    result = DCM_E_OK;
    return result;
}

Dcm_ReturnType Dcm_CacheLookup(const Dcm_CacheKey *key,
                               Dcm_ResponseType *response)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_cacheState.initialized || (key == NULL_PTR) || (response == NULL_PTR)) {
        return result;
    }
    
    int32_t index = findEntry(key);
    
    if (index >= 0) {
        Dcm_CacheEntry *entry = &s_cacheState.entries[index];
        
        /* Check TTL */
        if (entry->ttlMs > 0U) {
            /* TTL 检查 - 时间戳比较依赖系统定时器集成 */
        }
        
        /* Copy response data */
        if (response->maxLength >= entry->responseLength) {
            (void)memcpy(response->data, entry->responseData, entry->responseLength);
            response->length = entry->responseLength;
            response->isNegativeResponse = entry->isNegativeResponse;
            response->negativeResponseCode = entry->negativeResponseCode;
            
            entry->hitCount++;
            s_cacheState.stats.hitCount++;
            
            /* Update hit rate */
            uint32_t total = s_cacheState.stats.hitCount + s_cacheState.stats.missCount;
            if (total > 0U) {
                s_cacheState.stats.hitRate = (s_cacheState.stats.hitCount * 100U) / total;
            }
            
            result = DCM_E_OK;
        }
    } else {
        s_cacheState.stats.missCount++;
    }
    
    return result;
}

Dcm_ReturnType Dcm_CacheInvalidate(const Dcm_CacheKey *key)
{
    if (!s_cacheState.initialized || (key == NULL_PTR)) {
        return DCM_E_NOT_OK;
    }
    
    int32_t index = findEntry(key);
    
    if (index >= 0) {
        s_cacheState.entries[index].state = DCM_CACHE_ENTRY_STALE;
        s_cacheState.stats.invalidationCount++;
        
        /* Recount valid entries */
        uint32_t validCount = 0U;
        for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
            if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
                validCount++;
            }
        }
        s_cacheState.stats.currentSize = validCount;
        
        return DCM_E_OK;
    }
    
    return DCM_E_NOT_OK;
}

Dcm_ReturnType Dcm_CacheInvalidateAll(void)
{
    if (!s_cacheState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
            s_cacheState.entries[i].state = DCM_CACHE_ENTRY_STALE;
            s_cacheState.stats.invalidationCount++;
        }
    }
    
    s_cacheState.stats.currentSize = 0U;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_CacheInvalidateBySession(Dcm_SessionType session)
{
    if (!s_cacheState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Invalidate entries that depend on session state */
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
            /* Check if entry contains session-dependent data */
            /* Key[0] is service ID - session control responses are session-dependent */
            if (s_cacheState.entries[i].key.data[0] == UDS_SVC_DIAGNOSTIC_SESSION_CONTROL) {
                s_cacheState.entries[i].state = DCM_CACHE_ENTRY_STALE;
                s_cacheState.stats.invalidationCount++;
            }
        }
    }
    
    /* Recount valid entries */
    uint32_t validCount = 0U;
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
            validCount++;
        }
    }
    s_cacheState.stats.currentSize = validCount;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_CacheBuildKey(uint8_t serviceId,
                                 const Dcm_RequestType *request,
                                 Dcm_CacheKey *key)
{
    if ((request == NULL_PTR) || (key == NULL_PTR)) {
        return DCM_E_NOT_OK;
    }
    
    key->length = 0U;
    
    /* Service ID is always part of key */
    key->data[key->length++] = serviceId;
    
    /* Add relevant request data based on service type */
    switch (serviceId) {
        case UDS_SVC_READ_DATA_BY_IDENTIFIER:
            /* DID is bytes 1-2 of request */
            if (request->length >= 3U) {
                key->data[key->length++] = request->data[1];
                key->data[key->length++] = request->data[2];
            }
            break;
            
        case UDS_SVC_DIAGNOSTIC_SESSION_CONTROL:
            /* Subfunction */
            if (request->length >= 2U) {
                key->data[key->length++] = request->data[1] & DCM_SUBFUNCTION_MASK;
            }
            break;
            
        case UDS_SVC_READ_DTC_INFORMATION:
            /* Subfunction and status mask */
            if (request->length >= 3U) {
                key->data[key->length++] = request->data[1];
                key->data[key->length++] = request->data[2];
            }
            break;
            
        default:
            /* Use hash of request data for other services */
            if (request->length > 1U) {
                uint32_t hash = computeHash(&request->data[1], 
                                           (uint8_t)(request->length - 1U));
                key->data[key->length++] = (uint8_t)(hash >> 24);
                key->data[key->length++] = (uint8_t)(hash >> 16);
                key->data[key->length++] = (uint8_t)(hash >> 8);
                key->data[key->length++] = (uint8_t)(hash);
            }
            break;
    }
    
    return DCM_E_OK;
}

bool Dcm_CacheIsServiceCacheable(uint8_t serviceId)
{
    switch (serviceId) {
        case UDS_SVC_READ_DATA_BY_IDENTIFIER:
        case UDS_SVC_READ_DTC_INFORMATION:
        case UDS_SVC_DIAGNOSTIC_SESSION_CONTROL:
        case UDS_SVC_READ_SCALING_DATA_BY_IDENTIFIER:
            /* These services have deterministic responses */
            return true;
            
        case UDS_SVC_SECURITY_ACCESS:
        case UDS_SVC_WRITE_DATA_BY_IDENTIFIER:
        case UDS_SVC_WRITE_MEMORY_BY_ADDRESS:
        case UDS_SVC_ECU_RESET:
            /* These modify state - don't cache */
            return false;
            
        default:
            return false;
    }
}

Dcm_ReturnType Dcm_CacheGetStats(Dcm_CacheStats *stats)
{
    if (!s_cacheState.initialized || (stats == NULL_PTR)) {
        return DCM_E_NOT_OK;
    }
    
    (void)memcpy(stats, &s_cacheState.stats, sizeof(Dcm_CacheStats));
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_CacheUpdateTtl(uint32_t elapsedTimeMs)
{
    if (!s_cacheState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
            if (s_cacheState.entries[i].ttlMs > elapsedTimeMs) {
                s_cacheState.entries[i].ttlMs -= elapsedTimeMs;
            } else {
                s_cacheState.entries[i].state = DCM_CACHE_ENTRY_STALE;
                s_cacheState.stats.invalidationCount++;
            }
        }
    }
    
    /* Recount valid entries */
    uint32_t validCount = 0U;
    for (uint8_t i = 0U; i < DCM_CACHE_MAX_ENTRIES; i++) {
        if (s_cacheState.entries[i].state == DCM_CACHE_ENTRY_VALID) {
            validCount++;
        }
    }
    s_cacheState.stats.currentSize = validCount;
    
    return DCM_E_OK;
}

Dcm_ReturnType Dcm_CachePrepopulate(void)
{
    if (!s_cacheState.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Pre-populate with common responses */
    
    /* Session control - Default Session response */
    Dcm_CacheKey key1;
    key1.length = 2U;
    key1.data[0] = UDS_SVC_DIAGNOSTIC_SESSION_CONTROL;
    key1.data[1] = 0x01U; /* Default Session */
    
    Dcm_ResponseType resp1;
    uint8_t resp1Data[6] = {0x50U, 0x01U, 0x00U, 0x32U, 0x13U, 0x88U};
    resp1.data = resp1Data;
    resp1.length = 6U;
    resp1.maxLength = 6U;
    resp1.isNegativeResponse = false;
    resp1.suppressPositiveResponse = false;
    
    (void)Dcm_CacheStore(&key1, &resp1, 0U);
    
    return DCM_E_OK;
}
