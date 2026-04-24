/******************************************************************************
 * @file    ecc_allocator.h
 * @brief   ECC Memory Allocator - Safe Memory Management with ECC Protection
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * Features:
 * - ECC-protected heap allocation
 * - Memory boundary protection (guard words)
 * - Memory metadata management
 * - Alignment guarantees
 * - Double-free detection
 * - Buffer overflow detection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef ECC_ALLOCATOR_H
#define ECC_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"
#include "ecc_encoder.h"
#include "ecc_checker.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Allocator configuration */
#define ECC_ALLOC_HEAP_SIZE         (256U * 1024U)  /* 256KB default heap */
#define ECC_ALLOC_ALIGN_DEFAULT     8U              /* Default 8-byte alignment */
#define ECC_ALLOC_ALIGN_CACHE       32U             /* Cache line alignment */
#define ECC_ALLOC_MAX_ALLOCATIONS   1024U           /* Maximum concurrent allocations */

/* Guard word pattern for boundary protection */
#define ECC_ALLOC_GUARD_PATTERN     0xDEADBEEFU
#define ECC_ALLOC_FREED_PATTERN     0xFEEEFEEEU

/* Block header magic numbers */
#define ECC_ALLOC_BLOCK_MAGIC       0xECC0U
#define ECC_ALLOC_BLOCK_VALID       0xB10C0000U

/* Allocation flags */
#define ECC_ALLOC_FLAG_NONE         0x00U
#define ECC_ALLOC_FLAG_ZERO         0x01U   /* Zero memory on alloc */
#define ECC_ALLOC_FLAG_ECC          0x02U   /* Enable ECC protection */
#define ECC_ALLOC_FLAG_GUARD        0x04U   /* Enable guard words */
#define ECC_ALLOC_FLAG_CRITICAL     0x08U   /* ASIL-D critical allocation */
#define ECC_ALLOC_FLAG_ALIGNED      0x10U   /* Request aligned address */

/* Error codes specific to allocator */
#define E_ECC_ALLOC_OK              E_OK
#define E_ECC_ALLOC_NO_MEMORY       0x81U
#define E_ECC_ALLOC_INVALID_SIZE    0x82U
#define E_ECC_ALLOC_INVALID_PTR     0x83U
#define E_ECC_ALLOC_DOUBLE_FREE     0x84U
#define E_ECC_ALLOC_BUFFER_OVERFLOW 0x85U
#define E_ECC_ALLOC_HEAP_CORRUPT    0x86U
#define E_ECC_ALLOC_ALIGNMENT_ERR   0x87U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Memory block header - placed before each allocated block */
typedef struct {
    uint32_t magic;             /* Magic number for validation */
    uint32_t size;              /* User-requested size (excluding overhead) */
    uint32_t actual_size;       /* Actual allocated size (with overhead) */
    uint32_t flags;             /* Allocation flags */
    uint32_t sequence;          /* Allocation sequence number */
    uint32_t guard_front;       /* Front guard word */
    uint8_t ecc_code;           /* ECC code for header (optional) */
    uint8_t reserved[3];        /* Padding for alignment */
    
    /* Linked list pointers for heap management */
    struct EccMemBlockType *next;
    struct EccMemBlockType *prev;
} EccMemBlockHeaderType;

/* Memory block structure */
typedef struct EccMemBlockType {
    EccMemBlockHeaderType header;
    /* User data follows header, then: */
    /* - Guard words (if enabled) */
    /* - ECC storage (if enabled) */
    /* - Rear guard word */
} EccMemBlockType;

/* Allocation statistics */
typedef struct {
    uint32_t total_allocations;     /* Total successful allocations */
    uint32_t total_frees;           /* Total successful frees */
    uint32_t current_used;          /* Currently allocated bytes */
    uint32_t peak_used;             /* Peak allocated bytes */
    uint32_t total_bytes_allocated; /* Cumulative bytes allocated */
    uint32_t free_blocks;           /* Number of free blocks */
    uint32_t used_blocks;           /* Number of used blocks */
    uint32_t failed_allocations;    /* Failed allocation attempts */
    uint32_t double_frees;          /* Double-free attempts detected */
    uint32_t overflows_detected;    /* Buffer overflows detected */
    uint32_t heap_corruptions;      /* Heap corruption events */
    uint32_t ecc_errors;            /* ECC errors detected */
    uint32_t largest_free_block;    /* Size of largest free block */
    uint32_t fragmentation_percent; /* Fragmentation percentage (x100) */
} EccAllocStatsType;

/* Heap configuration */
typedef struct {
    uint8_t *heap_base;             /* Base address of heap */
    uint32_t heap_size;             /* Total heap size */
    uint32_t min_block_size;        /* Minimum allocation size */
    uint32_t alignment;             /* Default alignment */
    boolean enable_ecc;             /* Enable ECC on all allocations */
    boolean enable_guard;           /* Enable guard words */
    boolean auto_scrub;             /* Auto-scrub freed memory */
    uint32_t scrub_pattern;         /* Pattern to use for scrubbing */
    uint8_t asil_level;             /* ASIL level for safety checks */
} EccHeapConfigType;

/* Heap state */
typedef struct {
    boolean initialized;
    EccHeapConfigType config;
    EccAllocStatsType stats;
    
    /* Heap management */
    uint8_t *heap_start;
    uint8_t *heap_end;
    EccMemBlockType *free_list;     /* Head of free block list */
    EccMemBlockType *used_list;     /* Head of used block list */
    uint32_t sequence_counter;      /* Allocation sequence counter */
    
    /* ECC storage management */
    uint8_t *ecc_pool;              /* Pool for ECC codes */
    uint32_t ecc_pool_size;         /* Size of ECC pool */
    uint32_t ecc_pool_used;         /* Used bytes in ECC pool */
} EccHeapStateType;

/* Allocation info (for debugging) */
typedef struct {
    void *ptr;                      /* User pointer */
    uint32_t size;                  /* Allocated size */
    uint32_t sequence;              /* Allocation sequence */
    uint32_t flags;                 /* Allocation flags */
    boolean is_valid;               /* Block is valid */
    boolean has_ecc;                /* ECC enabled */
    uint8_t ecc_code;               /* ECC code */
} EccAllocInfoType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize ECC memory allocator
 * @param config Pointer to heap configuration
 * @return E_OK on success, error code on failure
 */
Std_ReturnType EccAllocator_Init(const EccHeapConfigType *config);

/**
 * @brief Deinitialize ECC memory allocator
 * @return E_OK on success
 */
Std_ReturnType EccAllocator_DeInit(void);

/**
 * @brief Allocate ECC-protected memory
 * @param size Number of bytes to allocate
 * @param flags Allocation flags (ECC_ALLOC_FLAG_*)
 * @return Pointer to allocated memory, NULL on failure
 */
void* EccAllocator_Malloc(uint32_t size, uint32_t flags);

/**
 * @brief Allocate zeroed ECC-protected memory
 * @param size Number of bytes to allocate
 * @param flags Allocation flags
 * @return Pointer to allocated memory, NULL on failure
 */
void* EccAllocator_Calloc(uint32_t num, uint32_t size, uint32_t flags);

/**
 * @brief Resize allocated memory
 * @param ptr Pointer to existing allocation (NULL = new allocation)
 * @param new_size New size in bytes
 * @param flags Allocation flags
 * @return Pointer to resized memory, NULL on failure
 */
void* EccAllocator_Realloc(void *ptr, uint32_t new_size, uint32_t flags);

/**
 * @brief Free ECC-protected memory
 * @param ptr Pointer to allocated memory
 * @return E_OK on success, error code on failure
 */
Std_ReturnType EccAllocator_Free(void *ptr);

/**
 * @brief Allocate aligned memory
 * @param size Number of bytes to allocate
 * @param alignment Required alignment (must be power of 2)
 * @param flags Allocation flags
 * @return Pointer to aligned memory, NULL on failure
 */
void* EccAllocator_AlignedAlloc(uint32_t size, uint32_t alignment, uint32_t flags);

/**
 * @brief Verify memory block integrity
 * @param ptr Pointer to allocated memory
 * @param result Pointer to store check result (can be NULL)
 * @return E_OK if valid, error code if corrupted
 */
Std_ReturnType EccAllocator_Verify(void *ptr, EccCheckResultType *result);

/**
 * @brief Check all allocated blocks for corruption
 * @return Number of corrupted blocks found
 */
uint32_t EccAllocator_CheckAll(void);

/**
 * @brief Get allocation statistics
 * @return Pointer to statistics structure
 */
const EccAllocStatsType* EccAllocator_GetStats(void);

/**
 * @brief Reset allocation statistics
 * @return E_OK on success
 */
Std_ReturnType EccAllocator_ResetStats(void);

/**
 * @brief Get information about an allocation
 * @param ptr Pointer to allocated memory
 * @param info Pointer to store allocation info
 * @return E_OK on success
 */
Std_ReturnType EccAllocator_GetInfo(void *ptr, EccAllocInfoType *info);

/**
 * @brief Get heap state
 * @return Pointer to heap state
 */
const EccHeapStateType* EccAllocator_GetHeapState(void);

/**
 * @brief Get total heap size
 * @return Total heap size in bytes
 */
uint32_t EccAllocator_GetHeapSize(void);

/**
 * @brief Get free heap space
 * @return Free bytes available
 */
uint32_t EccAllocator_GetFreeSize(void);

/**
 * @brief Get largest contiguous free block
 * @return Size of largest free block
 */
uint32_t EccAllocator_GetLargestFree(void);

/**
 * @brief Dump heap statistics (debug)
 */
void EccAllocator_DumpStats(void);

/**
 * @brief Set allocation failure callback
 * @param callback Function to call on allocation failure
 */
void EccAllocator_SetFailureCallback(void (*callback)(uint32_t size, uint32_t flags));

/**
 * @brief Get string description of error code
 * @param error Error code
 * @return Error description string
 */
const char* EccAllocator_ErrorString(Std_ReturnType error);

/**
 * @brief Get version information
 * @param version Pointer to version info structure
 */
void EccAllocator_GetVersionInfo(Std_VersionInfoType *version);

/******************************************************************************
 * Inline Helper Functions
 ******************************************************************************/

/**
 * @brief Get user pointer from block pointer
 */
static inline void* EccAllocator_BlockToPtr(EccMemBlockType *block)
{
    if (block == NULL) {
        return NULL;
    }
    return (void*)((uint8_t*)block + sizeof(EccMemBlockHeaderType));
}

/**
 * @brief Get block pointer from user pointer
 */
static inline EccMemBlockType* EccAllocator_PtrToBlock(void *ptr)
{
    if (ptr == NULL) {
        return NULL;
    }
    return (EccMemBlockType*)((uint8_t*)ptr - sizeof(EccMemBlockHeaderType));
}

/**
 * @brief Check if pointer is from ECC heap
 */
static inline boolean EccAllocator_IsHeapPtr(void *ptr)
{
    extern EccHeapStateType g_heap_state;
    if (ptr == NULL || !g_heap_state.initialized) {
        return FALSE;
    }
    return ((uint8_t*)ptr >= g_heap_state.heap_start &&
            (uint8_t*)ptr < g_heap_state.heap_end) ? TRUE : FALSE;
}

#ifdef __cplusplus
}
#endif

#endif /* ECC_ALLOCATOR_H */
