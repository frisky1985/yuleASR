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
 * @file    dcm_memory_pool.h
 * @brief   DCM Memory Pool - Pool-based Memory Allocator
 *
 * Reduces heap fragmentation and allocation overhead by using fixed-size
 * memory pools for DCM runtime objects.
 *
 * Features:
 * - Fixed-size block pools (O(1) alloc/free)
 * - Static or dynamic pool backing
 * - Thread-safe operations (optional)
 * - Memory alignment support
 * - Fragmentation-free allocation
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_MEMORY_POOL_H
#define DCM_MEMORY_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"

/******************************************************************************
 * Memory Pool Configuration
 ******************************************************************************/

/* Pool sizes - configurable */
#define DCM_POOL_SMALL_BLOCK_SIZE       32U     /* Small objects: headers, states */
#define DCM_POOL_MEDIUM_BLOCK_SIZE      128U    /* Medium objects: requests */
#define DCM_POOL_LARGE_BLOCK_SIZE       512U    /* Large objects: buffers */
#define DCM_POOL_XL_BLOCK_SIZE          2048U   /* Extra large: response buffers */

/* Pool counts - configurable */
#define DCM_POOL_SMALL_COUNT            32U     /* 32 * 32 = 1KB */
#define DCM_POOL_MEDIUM_COUNT           16U     /* 16 * 128 = 2KB */
#define DCM_POOL_LARGE_COUNT            8U      /* 8 * 512 = 4KB */
#define DCM_POOL_XL_COUNT               4U      /* 4 * 2048 = 8KB */

/* Total static pool size: ~15KB */
#define DCM_POOL_TOTAL_SIZE             ((DCM_POOL_SMALL_COUNT * DCM_POOL_SMALL_BLOCK_SIZE) + \
                                         (DCM_POOL_MEDIUM_COUNT * DCM_POOL_MEDIUM_BLOCK_SIZE) + \
                                         (DCM_POOL_LARGE_COUNT * DCM_POOL_LARGE_BLOCK_SIZE) + \
                                         (DCM_POOL_XL_COUNT * DCM_POOL_XL_BLOCK_SIZE))

/* Alignment - 4 bytes for 32-bit systems, 8 for 64-bit */
#if defined(__x86_64__) || defined(__aarch64__) || defined(__LP64__)
    #define DCM_POOL_ALIGNMENT          8U
#else
    #define DCM_POOL_ALIGNMENT          4U
#endif

/* Magic numbers for debugging */
#define DCM_POOL_MAGIC_FREE             0xF4EEU  /* "FREE" */
#define DCM_POOL_MAGIC_ALLOC            0xA110U  /* "ALLOC" */
#define DCM_POOL_MAGIC_GUARD            0xDEADU  /* Guard pattern */

/******************************************************************************
 * Memory Pool Types
 ******************************************************************************/

/* Pool block states */
typedef enum {
    DCM_BLOCK_FREE = 0,                 /* Block is free */
    DCM_BLOCK_ALLOCATED,                /* Block is allocated */
    DCM_BLOCK_CORRUPTED                 /* Block corruption detected */
} Dcm_PoolBlockState;

/* Pool block header - bit-packed for minimal overhead */
typedef struct {
    uint16_t magic : 16;                /* Magic number for validation */
    uint16_t state : 2;                 /* Block state (FREE/ALLOCATED) */
    uint16_t poolId : 3;                /* Pool identifier (0-7) */
    uint16_t seqNum : 11;               /* Sequence number for debugging */
} Dcm_PoolBlockHeader;

/* Pool statistics */
typedef struct {
    uint32_t allocCount;                /* Total allocations */
    uint32_t freeCount;                 /* Total frees */
    uint32_t failCount;                 /* Allocation failures */
    uint32_t currentUsed;               /* Currently allocated blocks */
    uint32_t peakUsed;                  /* Peak allocation */
    uint32_t corruptionCount;           /* Corruption detections */
} Dcm_PoolStats;

/* Individual pool definition */
typedef struct {
    uint8_t *buffer;                    /* Pool memory buffer */
    uint32_t blockSize;                 /* Size of each block */
    uint32_t blockCount;                /* Total number of blocks */
    uint32_t usedCount;                 /* Currently used blocks */
    uint8_t *freeList;                  /* Free list head */
    Dcm_PoolStats stats;                /* Pool statistics */
    const char *name;                   /* Pool name for debugging */
} Dcm_Pool;

/* Memory pool manager */
typedef struct {
    bool initialized;
    bool useStaticPools;                /* Use static or dynamic backing */
    Dcm_Pool smallPool;                 /* Small block pool */
    Dcm_Pool mediumPool;                /* Medium block pool */
    Dcm_Pool largePool;                 /* Large block pool */
    Dcm_Pool xlPool;                    /* Extra large pool */
    uint32_t totalAllocations;
    uint32_t totalFrees;
    uint32_t totalBytesAllocated;
} Dcm_PoolManager;

/* Pool allocation mode */
typedef enum {
    DCM_ALLOC_MODE_STATIC = 0,          /* Use static memory pools */
    DCM_ALLOC_MODE_DYNAMIC,             /* Use dynamic heap allocation */
    DCM_ALLOC_MODE_HYBRID               /* Try static first, fallback to dynamic */
} Dcm_AllocMode;

/* Memory allocation attributes */
typedef struct {
    Dcm_AllocMode mode;                 /* Allocation mode */
    uint8_t alignment;                  /* Required alignment */
    bool zeroInit;                      /* Zero-initialize memory */
    const char *file;                   /* Source file (debug) */
    uint32_t line;                      /* Source line (debug) */
} Dcm_MemAttr;

/******************************************************************************
 * Default Allocation Attributes
 ******************************************************************************/
#define DCM_MEM_ATTR_DEFAULT    { DCM_ALLOC_MODE_HYBRID, DCM_POOL_ALIGNMENT, true, NULL, 0U }
#define DCM_MEM_ATTR_STATIC     { DCM_ALLOC_MODE_STATIC, DCM_POOL_ALIGNMENT, true, NULL, 0U }
#define DCM_MEM_ATTR_DYNAMIC    { DCM_ALLOC_MODE_DYNAMIC, DCM_POOL_ALIGNMENT, true, NULL, 0U }
#define DCM_MEM_ATTR_NOINIT     { DCM_ALLOC_MODE_HYBRID, DCM_POOL_ALIGNMENT, false, NULL, 0U }

/******************************************************************************
 * Memory Pool API
 ******************************************************************************/

/**
 * @brief Initialize memory pool system
 *
 * @param useStaticPools If true, use static memory backing
 * @param staticBuffer Optional static buffer (NULL to use internal static)
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolInit(bool useStaticPools, uint8_t *staticBuffer);

/**
 * @brief Deinitialize memory pool system
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolDeInit(void);

/**
 * @brief Allocate memory from pool
 *
 * @param size Number of bytes to allocate
 * @param attr Allocation attributes
 * @return void* Allocated memory or NULL
 */
void* Dcm_PoolAlloc(uint32_t size, const Dcm_MemAttr *attr);

/**
 * @brief Free memory back to pool
 *
 * @param ptr Pointer to allocated memory
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolFree(void *ptr);

/**
 * @brief Allocate and zero memory
 *
 * @param size Number of bytes to allocate
 * @param attr Allocation attributes
 * @return void* Allocated and zeroed memory or NULL
 */
void* Dcm_PoolAllocZero(uint32_t size, const Dcm_MemAttr *attr);

/**
 * @brief Reallocate memory
 *
 * @param ptr Existing pointer (NULL for new allocation)
 * @param oldSize Current size
 * @param newSize New size
 * @param attr Allocation attributes
 * @return void* Reallocated memory or NULL
 */
void* Dcm_PoolRealloc(void *ptr, uint32_t oldSize, uint32_t newSize, 
                      const Dcm_MemAttr *attr);

/**
 * @brief Get pool statistics
 *
 * @param poolId Pool identifier (0-3)
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolGetStats(uint8_t poolId, Dcm_PoolStats *stats);

/**
 * @brief Get total pool statistics
 *
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolGetTotalStats(Dcm_PoolStats *stats);

/**
 * @brief Check if pointer is from pool
 *
 * @param ptr Pointer to check
 * @return bool True if from pool
 */
bool Dcm_PoolContains(const void *ptr);

/**
 * @brief Get block size for allocation
 *
 * @param ptr Allocated pointer
 * @return uint32_t Block size or 0
 */
uint32_t Dcm_PoolGetBlockSize(const void *ptr);

/**
 * @brief Validate pool integrity
 *
 * @param poolId Pool to validate (0xFF for all)
 * @return Dcm_ReturnType DCM_E_OK if valid
 */
Dcm_ReturnType Dcm_PoolValidate(uint8_t poolId);

/**
 * @brief Defragment pool (if supported)
 *
 * @param poolId Pool to defragment
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolDefragment(uint8_t poolId);

/**
 * @brief Get pool usage percentage
 *
 * @param poolId Pool identifier
 * @return uint8_t Usage percentage (0-100)
 */
uint8_t Dcm_PoolGetUsage(uint8_t poolId);

/**
 * @brief Check if pools are initialized
 *
 * @return bool True if initialized
 */
bool Dcm_PoolIsInitialized(void);

/**
 * @brief Emergency cleanup - free all allocations
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_PoolEmergencyCleanup(void);

/******************************************************************************
 * Convenience Macros
 ******************************************************************************/

/* Simple allocation macros */
#define DCM_ALLOC(size)         Dcm_PoolAlloc((size), NULL)
#define DCM_ALLOC_ZERO(size)    Dcm_PoolAllocZero((size), NULL)
#define DCM_FREE(ptr)           Dcm_PoolFree(ptr)
#define DCM_REALLOC(ptr, old, new) Dcm_PoolRealloc((ptr), (old), (new), NULL)

/* Static allocation macros */
#define DCM_ALLOC_STATIC(size)  Dcm_PoolAlloc((size), &((Dcm_MemAttr)DCM_MEM_ATTR_STATIC))
#define DCM_ALLOC_DYNAMIC(size) Dcm_PoolAlloc((size), &((Dcm_MemAttr)DCM_MEM_ATTR_DYNAMIC))

/* Debug allocation with file/line */
#if defined(DCM_DEBUG_ALLOC)
    #define DCM_ALLOC_DBG(size) Dcm_PoolAlloc((size), \
        &((Dcm_MemAttr){ DCM_ALLOC_MODE_HYBRID, DCM_POOL_ALIGNMENT, true, __FILE__, __LINE__ }))
#else
    #define DCM_ALLOC_DBG(size) DCM_ALLOC(size)
#endif

/******************************************************************************
 * Pool Size Helpers
 ******************************************************************************/

/* Get recommended pool for size */
static inline uint8_t Dcm_PoolGetRecommendedId(uint32_t size) {
    if (size <= DCM_POOL_SMALL_BLOCK_SIZE) {
        return 0U;
    } else if (size <= DCM_POOL_MEDIUM_BLOCK_SIZE) {
        return 1U;
    } else if (size <= DCM_POOL_LARGE_BLOCK_SIZE) {
        return 2U;
    } else if (size <= DCM_POOL_XL_BLOCK_SIZE) {
        return 3U;
    }
    return 0xFFU; /* Too large for pools */
}

/* Get block size for pool ID */
static inline uint32_t Dcm_PoolGetSizeForId(uint8_t poolId) {
    switch (poolId) {
        case 0U: return DCM_POOL_SMALL_BLOCK_SIZE;
        case 1U: return DCM_POOL_MEDIUM_BLOCK_SIZE;
        case 2U: return DCM_POOL_LARGE_BLOCK_SIZE;
        case 3U: return DCM_POOL_XL_BLOCK_SIZE;
        default: return 0U;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* DCM_MEMORY_POOL_H */
