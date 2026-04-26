/**
 * @file ota_memory_pool.c
 * @brief OTA Static Memory Pool Implementation
 * @note MISRA C:2012 Compliant - replaces malloc/free
 */

#include "ota_memory_pool.h"
#include <string.h>

/* ============================================================================
 * Static Memory Pool Instance
 * ============================================================================ */
static ota_memory_pool_t s_otaPool;

/* ============================================================================
 * Initialize Memory Pool
 * ============================================================================ */
ota_error_t ota_memory_pool_init(void)
{
    ota_error_t result = OTA_ERR_OK;
    
    if (!s_otaPool.initialized) {
        /* Clear all blocks */
        (void)memset(s_otaPool.buffer, 0, sizeof(s_otaPool.buffer));
        
        /* Mark all blocks as free */
        for (uint8_t i = 0U; i < OTA_POOL_NUM_BLOCKS; i++) {
            s_otaPool.blockUsed[i] = false;
        }
        
        s_otaPool.allocCount = 0U;
        s_otaPool.freeCount = 0U;
        s_otaPool.initialized = true;
    }
    
    return result;
}

/* ============================================================================
 * Allocate Block from Pool
 * ============================================================================ */
void* ota_memory_pool_alloc(uint32_t size)
{
    void *ptr = NULL;
    
    if (s_otaPool.initialized && (size <= OTA_POOL_BLOCK_SIZE)) {
        /* Find first free block */
        for (uint8_t i = 0U; i < OTA_POOL_NUM_BLOCKS; i++) {
            if (!s_otaPool.blockUsed[i]) {
                s_otaPool.blockUsed[i] = true;
                ptr = s_otaPool.buffer[i];
                s_otaPool.allocCount++;
                break;
            }
        }
    }
    
    return ptr;
}

/* ============================================================================
 * Free Block back to Pool
 * ============================================================================ */
void ota_memory_pool_free(void *ptr)
{
    if ((ptr != NULL) && s_otaPool.initialized) {
        /* Calculate which block this is */
        uint8_t *bytePtr = (uint8_t *)ptr;
        uint8_t *basePtr = (uint8_t *)s_otaPool.buffer;
        
        if ((bytePtr >= basePtr) && 
            (bytePtr < (basePtr + (OTA_POOL_NUM_BLOCKS * OTA_POOL_BLOCK_SIZE)))) {
            uint32_t offset = (uint32_t)(bytePtr - basePtr);
            uint8_t blockIndex = (uint8_t)(offset / OTA_POOL_BLOCK_SIZE);
            
            if (blockIndex < OTA_POOL_NUM_BLOCKS) {
                /* Clear the block */
                (void)memset(s_otaPool.buffer[blockIndex], 0, OTA_POOL_BLOCK_SIZE);
                s_otaPool.blockUsed[blockIndex] = false;
                s_otaPool.freeCount++;
            }
        }
    }
}

/* ============================================================================
 * Check if Pool is Initialized
 * ============================================================================ */
bool ota_memory_pool_is_initialized(void)
{
    return s_otaPool.initialized;
}

/* ============================================================================
 * Get Free Block Count
 * ============================================================================ */
uint32_t ota_memory_pool_get_free_blocks(void)
{
    uint32_t freeCount = 0U;
    
    if (s_otaPool.initialized) {
        for (uint8_t i = 0U; i < OTA_POOL_NUM_BLOCKS; i++) {
            if (!s_otaPool.blockUsed[i]) {
                freeCount++;
            }
        }
    }
    
    return freeCount;
}

/* ============================================================================
 * Get Pool Statistics
 * ============================================================================ */
void ota_memory_pool_get_stats(uint32_t *allocCount, uint32_t *freeCount)
{
    if (allocCount != NULL) {
        *allocCount = s_otaPool.allocCount;
    }
    
    if (freeCount != NULL) {
        *freeCount = s_otaPool.freeCount;
    }
}
