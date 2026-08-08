/******************************************************************************
 * @file    heap_protection.h
 * @brief   Heap Protection Module for Automotive Safety
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides comprehensive heap protection:
 * - Heap memory validation
 * - Memory block overlap detection
 * - Zero-on-free (clear memory after release)
 * - Double-free detection
 * - Buffer overflow/underflow detection
 * - Heap integrity checking
 * - Memory leak detection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef HEAP_PROTECTION_H
#define HEAP_PROTECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Module version */
#define HEAP_PROT_VERSION_MAJOR         1U
#define HEAP_PROT_VERSION_MINOR         0U
#define HEAP_PROT_VERSION_PATCH         0U

/* Heap configuration */
#define HEAP_PROT_MAX_BLOCKS            256U
#define HEAP_PROT_MIN_BLOCK_SIZE        8U
#define HEAP_PROT_ALIGNMENT             8U
#define HEAP_PROT_GUARD_SIZE            8U

/* Block header magic numbers */
#define HEAP_BLOCK_MAGIC_ALLOC          0xDEADBEEFU
#define HEAP_BLOCK_MAGIC_FREE           0xFEEEFEEEU
#define HEAP_BLOCK_MAGIC_GUARD          0xCAFEBABEU

/* Fill patterns */
#define HEAP_FILL_PATTERN_ALLOC         0xAAU
#define HEAP_FILL_PATTERN_FREE          0xDDU
#define HEAP_FILL_PATTERN_GUARD         0xDEU

/* Heap states */
#define HEAP_STATE_HEALTHY              0U
#define HEAP_STATE_FRAGMENTED           1U
#define HEAP_STATE_CRITICAL             2U
#define HEAP_STATE_CORRUPTED            3U

/* Block states */
#define HEAP_BLOCK_STATE_FREE           0U
#define HEAP_BLOCK_STATE_ALLOCATED      1U
#define HEAP_BLOCK_STATE_CORRUPTED      2U
#define HEAP_BLOCK_STATE_GUARD_VIOLATED 3U

/* Heap errors */
#define HEAP_ERR_NONE                   0x00U
#define HEAP_ERR_NULL_PTR               0x01U
#define HEAP_ERR_INVALID_PTR            0x02U
#define HEAP_ERR_DOUBLE_FREE            0x03U
#define HEAP_ERR_OVERFLOW               0x04U
#define HEAP_ERR_UNDERFLOW              0x05U
#define HEAP_ERR_CORRUPTION             0x06U
#define HEAP_ERR_OUT_OF_MEMORY          0x07U
#define HEAP_ERR_INVALID_SIZE           0x08U
#define HEAP_ERR_OVERLAP                0x09U
#define HEAP_ERR_GUARD_VIOLATION        0x0AU

/* Heap flags */
#define HEAP_FLAG_ZERO_ON_ALLOC         0x01U
#define HEAP_FLAG_ZERO_ON_FREE          0x02U
#define HEAP_FLAG_CHECK_OVERFLOW        0x04U
#define HEAP_FLAG_CHECK_UNDERFLOW       0x08U
#define HEAP_FLAG_USE_GUARDS            0x10U
#define HEAP_FLAG_LOCK_ON_CORRUPTION    0x20U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Heap block header (tracking structure) */
typedef struct HeapBlockHeader {
    uint32_t magic;                     /* Magic number for validation */
    uint32_t size;                      /* Block size (user data) */
    uint32_t total_size;                /* Total size including overhead */
    uint8_t state;                      /* Block state */
    uint8_t flags;                      /* Allocation flags */
    uint16_t checksum;                  /* Header checksum */
    uint32_t alloc_id;                  /* Allocation ID for tracking */
    uint32_t alloc_time;                /* Allocation timestamp */
    uint8_t owner_task_id;              /* Owner task ID */
    struct HeapBlockHeader *next;       /* Next block in list */
    struct HeapBlockHeader *prev;       /* Previous block in list */
} HeapBlockHeaderType;

/* Heap statistics */
typedef struct {
    uint32_t total_size;                /* Total heap size */
    uint32_t used_size;                 /* Currently used */
    uint32_t free_size;                 /* Currently free */
    uint32_t peak_used;                 /* Peak usage */
    uint32_t alloc_count;               /* Total allocations */
    uint32_t free_count;                /* Total frees */
    uint32_t fail_count;                /* Failed allocations */
    uint32_t block_count;               /* Current block count */
    uint32_t free_block_count;          /* Number of free blocks */
    uint32_t fragmentation;             /* Fragmentation percentage */
    uint32_t corruption_count;          /* Corruption detections */
    uint32_t double_free_count;         /* Double-free detections */
    uint32_t overflow_count;            /* Overflow detections */
    uint32_t underflow_count;           /* Underflow detections */
} HeapStatisticsType;

/* Heap configuration */
typedef struct {
    uint32_t start_addr;                /* Heap start address */
    uint32_t size;                      /* Heap size */
    uint8_t flags;                      /* Heap flags */
    uint32_t min_block_size;            /* Minimum allocation size */
    boolean enable_statistics;          /* Enable statistics tracking */
    boolean enable_locking;             /* Lock heap on corruption */
    uint8_t max_blocks;                 /* Maximum blocks to track */
} HeapConfigType;

/* Heap information */
typedef struct {
    uint8_t state;                      /* Heap state */
    boolean initialized;                /* Initialization flag */
    boolean locked;                     /* Lock state */
    HeapStatisticsType stats;           /* Statistics */
    uint32_t last_error;                /* Last error code */
    uint32_t last_error_addr;           /* Last error address */
} HeapInfoType;

/* Heap violation information */
typedef struct {
    uint32_t block_addr;                /* Block address */
    uint32_t violation_addr;            /* Violation address */
    uint8_t violation_type;             /* Type of violation */
    uint8_t expected_state;             /* Expected block state */
    uint8_t actual_state;               /* Actual block state */
    uint32_t timestamp;                 /* Timestamp */
    uint8_t task_id;                    /* Task ID */
} HeapViolationType;

/* Block validation result */
typedef struct {
    boolean is_valid;                   /* Block is valid */
    uint8_t error_code;                 /* Error if invalid */
    uint32_t header_checksum;           /* Calculated checksum */
    uint32_t stored_checksum;           /* Stored checksum */
    boolean guards_intact;              /* Guard zones intact */
    boolean in_bounds;                  /* Within heap bounds */
} BlockValidationType;

/* Heap violation callback */
typedef void (*HeapViolationCallbackType)(
    const HeapViolationType *violation
);

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize heap protection module
 * @param config Heap configuration
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_Init(const HeapConfigType *config);

/**
 * @brief Deinitialize heap protection module
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_DeInit(void);

/******************************************************************************
 * Function Prototypes - Safe Memory Allocation
 ******************************************************************************/

/**
 * @brief Allocate protected memory block
 * @param size Requested size
 * @param flags Allocation flags
 * @return Pointer to allocated memory, NULL on failure
 */
void* HeapProtection_Malloc(uint32_t size, uint8_t flags);

/**
 * @brief Allocate and zero-initialize memory
 * @param num Number of elements
 * @param size Element size
 * @param flags Allocation flags
 * @return Pointer to allocated memory, NULL on failure
 */
void* HeapProtection_Calloc(uint32_t num, uint32_t size, uint8_t flags);

/**
 * @brief Reallocate memory block
 * @param ptr Existing pointer (NULL for new allocation)
 * @param new_size New size
 * @param flags Allocation flags
 * @return Pointer to reallocated memory, NULL on failure
 */
void* HeapProtection_Realloc(void *ptr, uint32_t new_size, uint8_t flags);

/**
 * @brief Free protected memory block
 * @param ptr Pointer to memory
 * @param flags Free flags
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_Free(void *ptr, uint8_t flags);

/******************************************************************************
 * Function Prototypes - Memory Validation
 ******************************************************************************/

/**
 * @brief Validate heap block
 * @param ptr Pointer to user data
 * @param result Pointer to store validation result
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_ValidateBlock(
    const void *ptr,
    BlockValidationType *result
);

/**
 * @brief Validate entire heap
 * @return Number of corrupted blocks found
 */
uint32_t HeapProtection_ValidateHeap(void);

/**
 * @brief Check for buffer overflow
 * @param ptr Pointer to check
 * @param overflow Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_CheckOverflow(const void *ptr, boolean *overflow);

/**
 * @brief Check for buffer underflow
 * @param ptr Pointer to check
 * @param underflow Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_CheckUnderflow(const void *ptr, boolean *underflow);

/******************************************************************************
 * Function Prototypes - Overlap Detection
 ******************************************************************************/

/**
 * @brief Check for overlapping memory blocks
 * @return Number of overlaps found
 */
uint32_t HeapProtection_CheckOverlaps(void);

/**
 * @brief Check if two regions overlap
 * @param ptr1 First region
 * @param size1 First region size
 * @param ptr2 Second region
 * @param size2 Second region size
 * @return TRUE if overlap
 */
boolean HeapProtection_RegionsOverlap(
    const void *ptr1, uint32_t size1,
    const void *ptr2, uint32_t size2
);

/******************************************************************************
 * Function Prototypes - Double-Free Detection
 ******************************************************************************/

/**
 * @brief Check if pointer was already freed
 * @param ptr Pointer to check
 * @param was_freed Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_WasFreed(const void *ptr, boolean *was_freed);

/**
 * @brief Mark block as freed (for tracking)
 * @param block Block header
 */
void HeapProtection_MarkFreed(HeapBlockHeaderType *block);

/******************************************************************************
 * Function Prototypes - Memory Clearing
 ******************************************************************************/

/**
 * @brief Securely clear memory
 * @param ptr Pointer to memory
 * @param size Size to clear
 */
void HeapProtection_SecureClear(void *ptr, uint32_t size);

/**
 * @brief Fill memory with pattern
 * @param ptr Pointer to memory
 * @param size Size to fill
 * @param pattern Fill pattern
 */
void HeapProtection_FillPattern(void *ptr, uint32_t size, uint8_t pattern);

/**
 * @brief Clear memory on free (zero-on-free)
 * @param ptr Pointer to memory
 * @param size Size to clear
 */
void HeapProtection_ClearOnFree(void *ptr, uint32_t size);

/******************************************************************************
 * Function Prototypes - Guard Zones
 ******************************************************************************/

/**
 * @brief Initialize guard zones for block
 * @param header Block header
 */
void HeapProtection_InitGuards(HeapBlockHeaderType *header);

/**
 * @brief Verify guard zone integrity
 * @param header Block header
 * @param valid Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_CheckGuards(
    const HeapBlockHeaderType *header,
    boolean *valid
);

/******************************************************************************
 * Function Prototypes - Safety Checks
 ******************************************************************************/

/**
 * @brief Perform comprehensive heap check
 * @return Number of errors found
 */
uint32_t HeapProtection_FullCheck(void);

/**
 * @brief Check heap for corruption
 * @param corrupted Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_IsCorrupted(boolean *corrupted);

/**
 * @brief Get heap state
 * @return Current heap state
 */
uint8_t HeapProtection_GetState(void);

/******************************************************************************
 * Function Prototypes - Violation Handling
 ******************************************************************************/

/**
 * @brief Set violation callback
 * @param callback Callback function
 */
void HeapProtection_SetViolationCallback(HeapViolationCallbackType callback);

/**
 * @brief Handle heap violation
 * @param violation Violation information
 */
void HeapProtection_HandleViolation(const HeapViolationType *violation);

/**
 * @brief Get last violation info
 * @param violation Pointer to store violation
 * @return E_OK if violation occurred
 */
Std_ReturnType HeapProtection_GetLastViolation(HeapViolationType *violation);

/******************************************************************************
 * Function Prototypes - Statistics and Info
 ******************************************************************************/

/**
 * @brief Get heap information
 * @param info Pointer to store information
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_GetInfo(HeapInfoType *info);

/**
 * @brief Get heap statistics
 * @param stats Pointer to store statistics
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_GetStats(HeapStatisticsType *stats);

/**
 * @brief Reset heap statistics
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_ResetStats(void);

/**
 * @brief Check for memory leaks
 * @param leak_count Pointer to store leak count
 * @return E_OK on success
 */
Std_ReturnType HeapProtection_CheckLeaks(uint32_t *leak_count);

/******************************************************************************
 * Function Prototypes - Utility Functions
 ******************************************************************************/

/**
 * @brief Get block header from user pointer
 * @param ptr User data pointer
 * @return Block header pointer
 */
HeapBlockHeaderType* HeapProtection_GetHeader(const void *ptr);

/**
 * @brief Calculate header checksum
 * @param header Block header
 * @return Checksum value
 */
uint16_t HeapProtection_CalcChecksum(const HeapBlockHeaderType *header);

/**
 * @brief Get user data pointer from header
 * @param header Block header
 * @return User data pointer
 */
void* HeapProtection_GetUserPtr(const HeapBlockHeaderType *header);

/**
 * @brief Get allocated size
 * @param ptr User data pointer
 * @return Size in bytes, 0 if invalid
 */
uint32_t HeapProtection_GetSize(const void *ptr);

/**
 * @brief Dump heap state for debugging
 */
void HeapProtection_DumpState(void);

/**
 * @brief Lock heap (prevent allocations)
 */
void HeapProtection_Lock(void);

/**
 * @brief Unlock heap
 */
void HeapProtection_Unlock(void);

/**
 * @brief Get error string
 * @param error_code Error code
 * @return Error string
 */
const char* HeapProtection_GetErrorString(uint8_t error_code);

/**
 * @brief Get version information
 * @param version Pointer to version info
 */
void HeapProtection_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* HEAP_PROTECTION_H */
