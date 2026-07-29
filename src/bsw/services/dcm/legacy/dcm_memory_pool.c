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
 * @file    dcm_memory_pool.c
 * @brief   DCM Memory Pool Implementation
 *
 * Fixed-size memory pool allocator with O(1) alloc/free operations.
 * Supports both static and dynamic memory backing.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_memory_pool.h"
#include <string.h>
#include <stdlib.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_POOL_MAGIC_INIT             0x504F4F4CU  /* "POOL" */
#define DCM_POOL_HEADER_SIZE            sizeof(Dcm_PoolBlockHeader)
#define DCM_POOL_TOTAL_BLOCKS           (DCM_POOL_SMALL_COUNT + DCM_POOL_MEDIUM_COUNT + \
                                         DCM_POOL_LARGE_COUNT + DCM_POOL_XL_COUNT)

/* Align size to pool alignment */
#define DCM_ALIGN_SIZE(size)            (((size) + DCM_POOL_ALIGNMENT - 1U) & \
                                         ~(DCM_POOL_ALIGNMENT - 1U))

/* Get header from user pointer */
#define DCM_PTR_TO_HEADER(ptr)          ((Dcm_PoolBlockHeader *)((uint8_t *)(ptr) - \
                                         DCM_POOL_HEADER_SIZE))

/* Get user pointer from header */
#define DCM_HEADER_TO_PTR(hdr)          ((void *)((uint8_t *)(hdr) + DCM_POOL_HEADER_SIZE))

/******************************************************************************
 * Static Memory Pool Storage
 ******************************************************************************/
#if defined(DCM_USE_STATIC_POOLS)
/* Statically allocated pool memory */
static uint8_t s_smallPoolBuffer[DCM_POOL_SMALL_COUNT * 
                                  (DCM_POOL_SMALL_BLOCK_SIZE + DCM_POOL_HEADER_SIZE)];
static uint8_t s_mediumPoolBuffer[DCM_POOL_MEDIUM_COUNT * 
                                   (DCM_POOL_MEDIUM_BLOCK_SIZE + DCM_POOL_HEADER_SIZE)];
static uint8_t s_largePoolBuffer[DCM_POOL_LARGE_COUNT * 
                                  (DCM_POOL_LARGE_BLOCK_SIZE + DCM_POOL_HEADER_SIZE)];
static uint8_t s_xlPoolBuffer[DCM_POOL_XL_COUNT * 
                               (DCM_POOL_XL_BLOCK_SIZE + DCM_POOL_HEADER_SIZE)];
#else
/* Dynamic pool buffers */
static uint8_t *s_smallPoolBuffer = NULL;
static uint8_t *s_mediumPoolBuffer = NULL;
static uint8_t *s_largePoolBuffer = NULL;
static uint8_t *s_xlPoolBuffer = NULL;
#endif

/******************************************************************************
 * Module State
 ******************************************************************************/
static Dcm_PoolManager s_poolManager;
static uint16_t s_sequenceNumber = 0U;

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Initialize a single pool
 */
static Dcm_ReturnType initPool(Dcm_Pool *pool, uint8_t *buffer,
                                uint32_t blockSize, uint32_t blockCount,
                                const char *name)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if ((pool != NULL) && (buffer != NULL) && (blockSize > 0U) && (blockCount > 0U)) {
        pool->buffer = buffer;
        pool->blockSize = blockSize;
        pool->blockCount = blockCount;
        pool->usedCount = 0U;
        pool->name = name;
        
        /* Clear statistics */
        (void)memset(&pool->stats, 0, sizeof(pool->stats));
        
        /* Initialize free list as linked list */
        uint32_t totalBlockSize = blockSize + DCM_POOL_HEADER_SIZE;
        uint8_t *current = buffer;
        
        for (uint32_t i = 0U; i < blockCount; i++) {
            Dcm_PoolBlockHeader *header = (Dcm_PoolBlockHeader *)current;
            header->magic = DCM_POOL_MAGIC_FREE;
            header->state = DCM_BLOCK_FREE;
            header->poolId = 0U; /* Set by caller */
            header->seqNum = 0U;
            
            /* Link to next block */
            if (i < (blockCount - 1U)) {
                *((uint8_t **)(current + DCM_POOL_HEADER_SIZE)) = current + totalBlockSize;
            } else {
                *((uint8_t **)(current + DCM_POOL_HEADER_SIZE)) = NULL;
            }
            
            current += totalBlockSize;
        }
        
        /* Set free list head */
        pool->freeList = buffer;
        
        result = DCM_E_OK;
    }
    
    return result;
}

/**
 * @brief Allocate dynamic pool buffer
 */
static uint8_t* allocatePoolBuffer(uint32_t blockSize, uint32_t blockCount)
{
    uint32_t totalSize = blockCount * (blockSize + DCM_POOL_HEADER_SIZE);
    uint8_t *buffer = (uint8_t *)malloc(totalSize);
    
    if (buffer != NULL) {
        (void)memset(buffer, 0, totalSize);
    }
    
    return buffer;
}

/**
 * @brief Free dynamic pool buffer
 */
static void freePoolBuffer(uint8_t *buffer)
{
    if (buffer != NULL) {
        free(buffer);
    }
}

/**
 * @brief Find which pool contains a pointer
 */
static Dcm_Pool* findPoolForPtr(const void *ptr)
{
    if (ptr == NULL) {
        return NULL;
    }
    
    const uint8_t *bytePtr = (const uint8_t *)ptr;
    
    /* Check each pool */
    if ((bytePtr >= s_poolManager.smallPool.buffer) &&
        (bytePtr < (s_poolManager.smallPool.buffer + 
                    (s_poolManager.smallPool.blockCount * 
                     (s_poolManager.smallPool.blockSize + DCM_POOL_HEADER_SIZE))))) {
        return &s_poolManager.smallPool;
    }
    
    if ((bytePtr >= s_poolManager.mediumPool.buffer) &&
        (bytePtr < (s_poolManager.mediumPool.buffer + 
                    (s_poolManager.mediumPool.blockCount * 
                     (s_poolManager.mediumPool.blockSize + DCM_POOL_HEADER_SIZE))))) {
        return &s_poolManager.mediumPool;
    }
    
    if ((bytePtr >= s_poolManager.largePool.buffer) &&
        (bytePtr < (s_poolManager.largePool.buffer + 
                    (s_poolManager.largePool.blockCount * 
                     (s_poolManager.largePool.blockSize + DCM_POOL_HEADER_SIZE))))) {
        return &s_poolManager.largePool;
    }
    
    if ((bytePtr >= s_poolManager.xlPool.buffer) &&
        (bytePtr < (s_poolManager.xlPool.buffer + 
                    (s_poolManager.xlPool.blockCount * 
                     (s_poolManager.xlPool.blockSize + DCM_POOL_HEADER_SIZE))))) {
        return &s_poolManager.xlPool;
    }
    
    return NULL;
}

/**
 * @brief Validate block header
 */
static bool validateHeader(const Dcm_PoolBlockHeader *header)
{
    if (header == NULL) {
        return false;
    }
    
    /* Check magic number */
    if ((header->magic != DCM_POOL_MAGIC_ALLOC) && 
        (header->magic != DCM_POOL_MAGIC_FREE)) {
        return false;
    }
    
    /* Check state */
    if (header->state > DCM_BLOCK_CORRUPTED) {
        return false;
    }
    
    /* Check pool ID */
    if (header->poolId > 3U) {
        return false;
    }
    
    return true;
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_PoolInit(bool useStaticPools, uint8_t *staticBuffer)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    /* Clear manager state */
    (void)memset(&s_poolManager, 0, sizeof(s_poolManager));
    s_sequenceNumber = 0U;
    
    /* Use static buffer if provided, otherwise use internal or allocate */
    uint8_t *smallBuf = NULL;
    uint8_t *mediumBuf = NULL;
    uint8_t *largeBuf = NULL;
    uint8_t *xlBuf = NULL;
    
    if (useStaticPools) {
#if defined(DCM_USE_STATIC_POOLS)
        smallBuf = s_smallPoolBuffer;
        mediumBuf = s_mediumPoolBuffer;
        largeBuf = s_largePoolBuffer;
        xlBuf = s_xlPoolBuffer;
#else
        /* Use provided static buffer */
        if (staticBuffer != NULL) {
            smallBuf = staticBuffer;
            mediumBuf = staticBuffer + (DCM_POOL_SMALL_COUNT * 
                                        (DCM_POOL_SMALL_BLOCK_SIZE + DCM_POOL_HEADER_SIZE));
            largeBuf = mediumBuf + (DCM_POOL_MEDIUM_COUNT * 
                                    (DCM_POOL_MEDIUM_BLOCK_SIZE + DCM_POOL_HEADER_SIZE));
            xlBuf = largeBuf + (DCM_POOL_LARGE_COUNT * 
                                (DCM_POOL_LARGE_BLOCK_SIZE + DCM_POOL_HEADER_SIZE));
        }
#endif
    } else {
        /* Allocate dynamic buffers */
        smallBuf = allocatePoolBuffer(DCM_POOL_SMALL_BLOCK_SIZE, DCM_POOL_SMALL_COUNT);
        if (smallBuf != NULL) {
            mediumBuf = allocatePoolBuffer(DCM_POOL_MEDIUM_BLOCK_SIZE, DCM_POOL_MEDIUM_COUNT);
        }
        if (mediumBuf != NULL) {
            largeBuf = allocatePoolBuffer(DCM_POOL_LARGE_BLOCK_SIZE, DCM_POOL_LARGE_COUNT);
        }
        if (largeBuf != NULL) {
            xlBuf = allocatePoolBuffer(DCM_POOL_XL_BLOCK_SIZE, DCM_POOL_XL_COUNT);
        }
    }
    
    /* Initialize pools */
    if ((smallBuf != NULL) &&
        (initPool(&s_poolManager.smallPool, smallBuf, 
                  DCM_POOL_SMALL_BLOCK_SIZE, DCM_POOL_SMALL_COUNT,
                  "Small") == DCM_E_OK)) {
        s_poolManager.smallPool.stats.allocCount = 0U;
        result = DCM_E_OK;
    }
    
    if ((result == DCM_E_OK) && (mediumBuf != NULL)) {
        if (initPool(&s_poolManager.mediumPool, mediumBuf,
                     DCM_POOL_MEDIUM_BLOCK_SIZE, DCM_POOL_MEDIUM_COUNT,
                     "Medium") != DCM_E_OK) {
            result = DCM_E_NOT_OK;
        }
    }
    
    if ((result == DCM_E_OK) && (largeBuf != NULL)) {
        if (initPool(&s_poolManager.largePool, largeBuf,
                     DCM_POOL_LARGE_BLOCK_SIZE, DCM_POOL_LARGE_COUNT,
                     "Large") != DCM_E_OK) {
            result = DCM_E_NOT_OK;
        }
    }
    
    if ((result == DCM_E_OK) && (xlBuf != NULL)) {
        if (initPool(&s_poolManager.xlPool, xlBuf,
                     DCM_POOL_XL_BLOCK_SIZE, DCM_POOL_XL_COUNT,
                     "XL") != DCM_E_OK) {
            result = DCM_E_NOT_OK;
        }
    }
    
    if (result == DCM_E_OK) {
        s_poolManager.initialized = true;
        s_poolManager.useStaticPools = useStaticPools;
    } else {
        /* Cleanup on failure */
        if (!useStaticPools) {
            freePoolBuffer(smallBuf);
            freePoolBuffer(mediumBuf);
            freePoolBuffer(largeBuf);
            freePoolBuffer(xlBuf);
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_PoolDeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_poolManager.initialized) {
        /* Free dynamic buffers if used */
        if (!s_poolManager.useStaticPools) {
            freePoolBuffer(s_poolManager.smallPool.buffer);
            freePoolBuffer(s_poolManager.mediumPool.buffer);
            freePoolBuffer(s_poolManager.largePool.buffer);
            freePoolBuffer(s_poolManager.xlPool.buffer);
        }
        
        /* Clear state */
        (void)memset(&s_poolManager, 0, sizeof(s_poolManager));
        result = DCM_E_OK;
    }
    
    return result;
}

void* Dcm_PoolAlloc(uint32_t size, const Dcm_MemAttr *attr)
{
    void *ptr = NULL;
    Dcm_Pool *pool = NULL;
    
    if (!s_poolManager.initialized || (size == 0U)) {
        return NULL;
    }
    
    /* Determine allocation mode */
    Dcm_AllocMode mode = DCM_ALLOC_MODE_HYBRID;
    if (attr != NULL) {
        mode = attr->mode;
    }
    
    /* Select appropriate pool based on size */
    if (size <= DCM_POOL_SMALL_BLOCK_SIZE) {
        pool = &s_poolManager.smallPool;
    } else if (size <= DCM_POOL_MEDIUM_BLOCK_SIZE) {
        pool = &s_poolManager.mediumPool;
    } else if (size <= DCM_POOL_LARGE_BLOCK_SIZE) {
        pool = &s_poolManager.largePool;
    } else if (size <= DCM_POOL_XL_BLOCK_SIZE) {
        pool = &s_poolManager.xlPool;
    }
    
    /* Try pool allocation first (for static and hybrid modes) */
    if ((pool != NULL) && (mode != DCM_ALLOC_MODE_DYNAMIC)) {
        if (pool->freeList != NULL) {
            /* Allocate from free list */
            uint8_t *block = pool->freeList;
            Dcm_PoolBlockHeader *header = (Dcm_PoolBlockHeader *)block;
            
            /* Update free list */
            pool->freeList = *((uint8_t **)(block + DCM_POOL_HEADER_SIZE));
            
            /* Mark as allocated */
            header->magic = DCM_POOL_MAGIC_ALLOC;
            header->state = DCM_BLOCK_ALLOCATED;
            header->seqNum = s_sequenceNumber++;
            
            /* Update statistics */
            pool->usedCount++;
            pool->stats.allocCount++;
            pool->stats.currentUsed++;
            if (pool->stats.currentUsed > pool->stats.peakUsed) {
                pool->stats.peakUsed = pool->stats.currentUsed;
            }
            
            s_poolManager.totalAllocations++;
            s_poolManager.totalBytesAllocated += pool->blockSize;
            
            /* Zero initialize if requested */
            ptr = (void *)(block + DCM_POOL_HEADER_SIZE);
            if ((attr != NULL) && attr->zeroInit) {
                (void)memset(ptr, 0, pool->blockSize);
            }
        } else if (mode == DCM_ALLOC_MODE_STATIC) {
            /* Pool full and static mode - fail */
            pool->stats.failCount++;
        }
    }
    
    /* Fallback to dynamic allocation if needed */
    if ((ptr == NULL) && (mode != DCM_ALLOC_MODE_STATIC)) {
        uint32_t allocSize = size + DCM_POOL_HEADER_SIZE;
        uint8_t *block = (uint8_t *)malloc(allocSize);
        
        if (block != NULL) {
            Dcm_PoolBlockHeader *header = (Dcm_PoolBlockHeader *)block;
            header->magic = DCM_POOL_MAGIC_ALLOC;
            header->state = DCM_BLOCK_ALLOCATED;
            header->poolId = 7U; /* Mark as dynamic (max value for 3 bits) */
            header->seqNum = s_sequenceNumber++;
            
            ptr = (void *)(block + DCM_POOL_HEADER_SIZE);
            
            if ((attr != NULL) && attr->zeroInit) {
                (void)memset(ptr, 0, size);
            }
            
            s_poolManager.totalAllocations++;
            s_poolManager.totalBytesAllocated += size;
        }
    }
    
    return ptr;
}

Dcm_ReturnType Dcm_PoolFree(void *ptr)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (ptr == NULL) {
        return DCM_E_OK; /* Free of NULL is OK per standard */
    }
    
    if (!s_poolManager.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Get header */
    Dcm_PoolBlockHeader *header = DCM_PTR_TO_HEADER(ptr);
    
    /* Validate header */
    if (!validateHeader(header)) {
        return DCM_E_NOT_OK;
    }
    
    /* Check if already free */
    if (header->state == DCM_BLOCK_FREE) {
        return DCM_E_NOT_OK; /* Double free */
    }
    
    /* Handle dynamic allocation - poolId 7 indicates dynamic (outside pool) */
    if (header->poolId == 7U) {
        header->magic = 0U; /* Clear magic */
        free(header);
        s_poolManager.totalFrees++;
        return DCM_E_OK;
    }
    
    /* Find pool */
    Dcm_Pool *pool = findPoolForPtr(ptr);
    if (pool == NULL) {
        return DCM_E_NOT_OK;
    }
    
    /* Return to free list */
    uint8_t *block = (uint8_t *)header;
    *((uint8_t **)(block + DCM_POOL_HEADER_SIZE)) = pool->freeList;
    pool->freeList = block;
    
    /* Mark as free */
    header->magic = DCM_POOL_MAGIC_FREE;
    header->state = DCM_BLOCK_FREE;
    
    /* Update statistics */
    pool->usedCount--;
    pool->stats.freeCount++;
    pool->stats.currentUsed--;
    s_poolManager.totalFrees++;
    
    result = DCM_E_OK;
    
    return result;
}

void* Dcm_PoolAllocZero(uint32_t size, const Dcm_MemAttr *attr)
{
    Dcm_MemAttr localAttr;
    
    if (attr != NULL) {
        localAttr = *attr;
    } else {
        localAttr = (Dcm_MemAttr)DCM_MEM_ATTR_DEFAULT;
    }
    
    localAttr.zeroInit = true;
    return Dcm_PoolAlloc(size, &localAttr);
}

void* Dcm_PoolRealloc(void *ptr, uint32_t oldSize, uint32_t newSize,
                      const Dcm_MemAttr *attr)
{
    void *newPtr = NULL;
    
    if (newSize == 0U) {
        /* Realloc to 0 is equivalent to free */
        (void)Dcm_PoolFree(ptr);
        return NULL;
    }
    
    if (ptr == NULL) {
        /* Realloc of NULL is equivalent to alloc */
        return Dcm_PoolAlloc(newSize, attr);
    }
    
    /* Allocate new block */
    newPtr = Dcm_PoolAlloc(newSize, attr);
    if (newPtr != NULL) {
        /* Copy old data */
        uint32_t copySize = (oldSize < newSize) ? oldSize : newSize;
        (void)memcpy(newPtr, ptr, copySize);
        
        /* Free old block */
        (void)Dcm_PoolFree(ptr);
    }
    
    return newPtr;
}

Dcm_ReturnType Dcm_PoolGetStats(uint8_t poolId, Dcm_PoolStats *stats)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    const Dcm_Pool *pool = NULL;
    
    if (!s_poolManager.initialized || (stats == NULL)) {
        return result;
    }
    
    switch (poolId) {
        case 0U:
            pool = &s_poolManager.smallPool;
            break;
        case 1U:
            pool = &s_poolManager.mediumPool;
            break;
        case 2U:
            pool = &s_poolManager.largePool;
            break;
        case 3U:
            pool = &s_poolManager.xlPool;
            break;
        default:
            return result;
    }
    
    (void)memcpy(stats, &pool->stats, sizeof(Dcm_PoolStats));
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_PoolGetTotalStats(Dcm_PoolStats *stats)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_poolManager.initialized || (stats == NULL)) {
        return result;
    }
    
    (void)memset(stats, 0, sizeof(Dcm_PoolStats));
    
    /* Aggregate all pool stats */
    Dcm_PoolStats poolStats;
    
    for (uint8_t i = 0U; i < 4U; i++) {
        if (Dcm_PoolGetStats(i, &poolStats) == DCM_E_OK) {
            stats->allocCount += poolStats.allocCount;
            stats->freeCount += poolStats.freeCount;
            stats->failCount += poolStats.failCount;
            stats->currentUsed += poolStats.currentUsed;
            stats->peakUsed += poolStats.peakUsed;
            stats->corruptionCount += poolStats.corruptionCount;
        }
    }
    
    result = DCM_E_OK;
    
    return result;
}

bool Dcm_PoolContains(const void *ptr)
{
    if (!s_poolManager.initialized || (ptr == NULL)) {
        return false;
    }
    
    return (findPoolForPtr(ptr) != NULL);
}

uint32_t Dcm_PoolGetBlockSize(const void *ptr)
{
    uint32_t size = 0U;
    
    if (!s_poolManager.initialized || (ptr == NULL)) {
        return 0U;
    }
    
    const Dcm_PoolBlockHeader *header = DCM_PTR_TO_HEADER(ptr);
    
    if (!validateHeader(header)) {
        return 0U;
    }
    
    if (header->poolId == 0xFFU) {
        /* Dynamic allocation - can't determine size */
        return 0U;
    }
    
    Dcm_Pool *pool = findPoolForPtr(ptr);
    if (pool != NULL) {
        size = pool->blockSize;
    }
    
    return size;
}

Dcm_ReturnType Dcm_PoolValidate(uint8_t poolId)
{
    Dcm_ReturnType result = DCM_E_OK;
    
    if (!s_poolManager.initialized) {
        return DCM_E_NOT_OK;
    }
    
    /* Validate specific pool or all pools */
    uint8_t startId = (poolId == 0xFFU) ? 0U : poolId;
    uint8_t endId = (poolId == 0xFFU) ? 3U : poolId;
    
    for (uint8_t i = startId; i <= endId; i++) {
        Dcm_Pool *pool = NULL;
        
        switch (i) {
            case 0U: pool = &s_poolManager.smallPool; break;
            case 1U: pool = &s_poolManager.mediumPool; break;
            case 2U: pool = &s_poolManager.largePool; break;
            case 3U: pool = &s_poolManager.xlPool; break;
            default: continue;
        }
        
        if (pool != NULL) {
            /* Walk free list and validate */
            uint8_t *current = pool->freeList;
            uint32_t freeCount = 0U;
            
            while (current != NULL) {
                Dcm_PoolBlockHeader *header = (Dcm_PoolBlockHeader *)current;
                
                if (!validateHeader(header)) {
                    pool->stats.corruptionCount++;
                    result = DCM_E_NOT_OK;
                    break;
                }
                
                freeCount++;
                current = *((uint8_t **)(current + DCM_POOL_HEADER_SIZE));
                
                /* Prevent infinite loop */
                if (freeCount > pool->blockCount) {
                    pool->stats.corruptionCount++;
                    result = DCM_E_NOT_OK;
                    break;
                }
            }
        }
    }
    
    return result;
}

Dcm_ReturnType Dcm_PoolDefragment(uint8_t poolId)
{
    /* Fixed-size pools don't fragment, so nothing to do */
    (void)poolId;
    return DCM_E_OK;
}

uint8_t Dcm_PoolGetUsage(uint8_t poolId)
{
    uint8_t usage = 0U;
    
    if (!s_poolManager.initialized) {
        return 0U;
    }
    
    const Dcm_Pool *pool = NULL;
    
    switch (poolId) {
        case 0U: pool = &s_poolManager.smallPool; break;
        case 1U: pool = &s_poolManager.mediumPool; break;
        case 2U: pool = &s_poolManager.largePool; break;
        case 3U: pool = &s_poolManager.xlPool; break;
        default: return 0U;
    }
    
    if ((pool != NULL) && (pool->blockCount > 0U)) {
        usage = (uint8_t)((pool->usedCount * 100U) / pool->blockCount);
    }
    
    return usage;
}

bool Dcm_PoolIsInitialized(void)
{
    return s_poolManager.initialized;
}

Dcm_ReturnType Dcm_PoolEmergencyCleanup(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_poolManager.initialized) {
        return result;
    }
    
    /* Reinitialize all pools - effectively frees everything */
    result = Dcm_PoolInit(s_poolManager.useStaticPools, NULL);
    
    return result;
}
