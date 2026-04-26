/**
 * @file ota_memory_pool.h
 * @brief OTA Static Memory Pool - MISRA Rule 21.3 Compliant
 * @version 1.0
 * @note Replaces malloc/free with fixed-size memory pools
 */

#ifndef OTA_MEMORY_POOL_H
#define OTA_MEMORY_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include "ota_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Memory Pool Configuration
 * ============================================================================ */
#define OTA_POOL_NUM_BLOCKS         4U
#define OTA_POOL_BLOCK_SIZE         4096U  /* 4KB blocks for OTA chunks */
#define OTA_POOL_ALIGNMENT          4U

/* ============================================================================
 * Memory Pool Types
 * ============================================================================ */
typedef enum {
    OTA_POOL_OK = 0,
    OTA_POOL_ERROR_NULL_PTR = 1,
    OTA_POOL_ERROR_NO_MEMORY = 2,
    OTA_POOL_ERROR_INVALID_SIZE = 3,
    OTA_POOL_ERROR_NOT_INITIALIZED = 4
} ota_pool_error_t;

typedef struct {
    uint8_t buffer[OTA_POOL_NUM_BLOCKS][OTA_POOL_BLOCK_SIZE];
    bool blockUsed[OTA_POOL_NUM_BLOCKS];
    uint32_t allocCount;
    uint32_t freeCount;
    bool initialized;
} ota_memory_pool_t;

/* ============================================================================
 * API Functions
 * ============================================================================ */

/**
 * @brief Initialize the OTA memory pool
 * @return OTA error code
 */
ota_error_t ota_memory_pool_init(void);

/**
 * @brief Allocate a block from the pool
 * @param size Requested size (must be <= OTA_POOL_BLOCK_SIZE)
 * @return Pointer to allocated block, or NULL if no memory
 */
void* ota_memory_pool_alloc(uint32_t size);

/**
 * @brief Free a block back to the pool
 * @param ptr Pointer to allocated block
 */
void ota_memory_pool_free(void *ptr);

/**
 * @brief Check if pool is initialized
 * @return true if initialized
 */
bool ota_memory_pool_is_initialized(void);

/**
 * @brief Get available blocks count
 * @return Number of free blocks
 */
uint32_t ota_memory_pool_get_free_blocks(void);

/**
 * @brief Get statistics
 * @param allocCount Output: total allocations
 * @param freeCount Output: total frees
 */
void ota_memory_pool_get_stats(uint32_t *allocCount, uint32_t *freeCount);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MEMORY_POOL_H */
