/******************************************************************************
 * @file    ecc_allocator.c
 * @brief   ECC Memory Allocator Implementation
 *
 * Memory layout for each block:
 * [Header][User Data][Guard Words (opt)][ECC Storage (opt)][Rear Guard]
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ecc_allocator.h"
#include <string.h>

/******************************************************************************
 * Local Constants
 ******************************************************************************/

#define ECC_ALLOCATOR_VENDOR_ID     0x0001U
#define ECC_ALLOCATOR_MODULE_ID     0x0103U
#define ECC_ALLOCATOR_SW_MAJOR      1U
#define ECC_ALLOCATOR_SW_MINOR      0U
#define ECC_ALLOCATOR_SW_PATCH      0U

/* Minimum block size (must fit header + at least some data) */
#define MIN_BLOCK_SIZE              (sizeof(EccMemBlockHeaderType) + 16U)

/* Rear guard word offset from end of block */
#define REAR_GUARD_SIZE             sizeof(uint32_t)

/******************************************************************************
 * Local Variables
 ******************************************************************************/

EccHeapStateType g_heap_state;
static boolean g_initialized = FALSE;
static void (*g_failure_callback)(uint32_t size, uint32_t flags) = NULL;

/* Default heap buffer (can be overridden) */
static uint8_t g_default_heap[ECC_ALLOC_HEAP_SIZE];
static uint8_t g_ecc_pool[ECC_ALLOC_HEAP_SIZE / 4U];  /* ECC code pool */

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

static EccMemBlockType* EccAllocator_FindFreeBlock(uint32_t size, uint32_t alignment);
static void EccAllocator_SplitBlock(EccMemBlockType *block, uint32_t size);
static void EccAllocator_CoalesceFreeBlocks(void);
static void EccAllocator_RemoveFromFreeList(EccMemBlockType *block);
static void EccAllocator_AddToFreeList(EccMemBlockType *block);
static void EccAllocator_RemoveFromUsedList(EccMemBlockType *block);
static void EccAllocator_AddToUsedList(EccMemBlockType *block);
static Std_ReturnType EccAllocator_ValidateBlock(EccMemBlockType *block);
static void EccAllocator_SetGuards(EccMemBlockType *block);
static Std_ReturnType EccAllocator_CheckGuards(EccMemBlockType *block);
static uint32_t EccAllocator_CalculateBlockSize(uint32_t user_size, uint32_t flags);
static void* EccAllocator_DoAlloc(uint32_t size, uint32_t alignment, uint32_t flags);

/******************************************************************************
 * Local Function Implementations
 ******************************************************************************/

/**
 * @brief Calculate total block size including overhead
 */
static uint32_t EccAllocator_CalculateBlockSize(uint32_t user_size, uint32_t flags)
{
    uint32_t block_size = sizeof(EccMemBlockHeaderType) + user_size + REAR_GUARD_SIZE;
    
    /* Add space for front guard if enabled */
    if (flags & ECC_ALLOC_FLAG_GUARD) {
        block_size += sizeof(uint32_t);
    }
    
    /* Add space for ECC codes if enabled (one per 4 bytes) */
    if (flags & ECC_ALLOC_FLAG_ECC) {
        block_size += (user_size + 3U) / 4U;  /* One ECC byte per word */
    }
    
    /* Ensure minimum size */
    if (block_size < MIN_BLOCK_SIZE) {
        block_size = MIN_BLOCK_SIZE;
    }
    
    /* Round up to alignment */
    block_size = (block_size + ECC_ALLOC_ALIGN_DEFAULT - 1U) & 
                 ~(ECC_ALLOC_ALIGN_DEFAULT - 1U);
    
    return block_size;
}

/**
 * @brief Find a suitable free block
 */
static EccMemBlockType* EccAllocator_FindFreeBlock(uint32_t size, uint32_t alignment)
{
    EccMemBlockType *current = g_heap_state.free_list;
    EccMemBlockType *best_fit = NULL;
    uint32_t best_size = 0xFFFFFFFFU;
    
    while (current != NULL) {
        /* Check if block is large enough */
        if (current->header.actual_size >= size) {
            /* Check alignment */
            uint32_t user_ptr = (uint32_t)EccAllocator_BlockToPtr(current);
            uint32_t aligned_ptr = (user_ptr + alignment - 1U) & ~(alignment - 1U);
            uint32_t padding = aligned_ptr - user_ptr;
            
            if (current->header.actual_size >= size + padding) {
                /* Use best-fit strategy */
                if (current->header.actual_size < best_size) {
                    best_size = current->header.actual_size;
                    best_fit = current;
                }
            }
        }
        current = current->header.next;
    }
    
    return best_fit;
}

/**
 * @brief Split a block if it's significantly larger than needed
 */
static void EccAllocator_SplitBlock(EccMemBlockType *block, uint32_t needed_size)
{
    uint32_t remaining;
    EccMemBlockType *new_block;
    
    if (block == NULL) {
        return;
    }
    
    /* Only split if there's enough space for another minimum block + overhead */
    remaining = block->header.actual_size - needed_size;
    if (remaining < MIN_BLOCK_SIZE + sizeof(EccMemBlockHeaderType)) {
        return;
    }
    
    /* Create new free block from remaining space */
    new_block = (EccMemBlockType*)((uint8_t*)block + needed_size);
    new_block->header.magic = ECC_ALLOC_BLOCK_MAGIC;
    new_block->header.size = 0U;  /* Free block */
    new_block->header.actual_size = remaining;
    new_block->header.flags = 0U;
    new_block->header.sequence = 0U;
    new_block->header.guard_front = ECC_ALLOC_FREED_PATTERN;
    
    /* Update original block */
    block->header.actual_size = needed_size;
    
    /* Add new block to free list */
    EccAllocator_AddToFreeList(new_block);
}

/**
 * @brief Remove block from free list
 */
static void EccAllocator_RemoveFromFreeList(EccMemBlockType *block)
{
    if (block == NULL) {
        return;
    }
    
    if (block->header.prev != NULL) {
        block->header.prev->header.next = block->header.next;
    } else {
        g_heap_state.free_list = block->header.next;
    }
    
    if (block->header.next != NULL) {
        block->header.next->header.prev = block->header.prev;
    }
    
    block->header.prev = NULL;
    block->header.next = NULL;
}

/**
 * @brief Add block to free list (sorted by address for coalescing)
 */
static void EccAllocator_AddToFreeList(EccMemBlockType *block)
{
    EccMemBlockType *current;
    
    if (block == NULL) {
        return;
    }
    
    block->header.prev = NULL;
    block->header.next = NULL;
    
    /* Empty list */
    if (g_heap_state.free_list == NULL) {
        g_heap_state.free_list = block;
        return;
    }
    
    /* Insert in address order */
    current = g_heap_state.free_list;
    while (current != NULL && current < block) {
        if (current->header.next == NULL || current->header.next > block) {
            break;
        }
        current = current->header.next;
    }
    
    if (current > block) {
        /* Insert at head */
        block->header.next = g_heap_state.free_list;
        if (g_heap_state.free_list != NULL) {
            g_heap_state.free_list->header.prev = block;
        }
        g_heap_state.free_list = block;
    } else {
        /* Insert after current */
        block->header.next = current->header.next;
        block->header.prev = current;
        current->header.next = block;
        if (block->header.next != NULL) {
            block->header.next->header.prev = block;
        }
    }
}

/**
 * @brief Remove block from used list
 */
static void EccAllocator_RemoveFromUsedList(EccMemBlockType *block)
{
    if (block == NULL) {
        return;
    }
    
    if (block->header.prev != NULL) {
        block->header.prev->header.next = block->header.next;
    } else {
        g_heap_state.used_list = block->header.next;
    }
    
    if (block->header.next != NULL) {
        block->header.next->header.prev = block->header.prev;
    }
    
    block->header.prev = NULL;
    block->header.next = NULL;
}

/**
 * @brief Add block to used list
 */
static void EccAllocator_AddToUsedList(EccMemBlockType *block)
{
    if (block == NULL) {
        return;
    }
    
    block->header.prev = NULL;
    block->header.next = g_heap_state.used_list;
    
    if (g_heap_state.used_list != NULL) {
        g_heap_state.used_list->header.prev = block;
    }
    
    g_heap_state.used_list = block;
}

/**
 * @brief Coalesce adjacent free blocks
 */
static void EccAllocator_CoalesceFreeBlocks(void)
{
    EccMemBlockType *current = g_heap_state.free_list;
    
    while (current != NULL && current->header.next != NULL) {
        EccMemBlockType *next = current->header.next;
        uint8_t *current_end = (uint8_t*)current + current->header.actual_size;
        
        /* Check if blocks are adjacent */
        if (current_end == (uint8_t*)next) {
            /* Merge blocks */
            current->header.actual_size += next->header.actual_size;
            current->header.next = next->header.next;
            if (next->header.next != NULL) {
                next->header.next->header.prev = current;
            }
        } else {
            current = next;
        }
    }
}

/**
 * @brief Validate a memory block
 */
static Std_ReturnType EccAllocator_ValidateBlock(EccMemBlockType *block)
{
    if (block == NULL) {
        return E_ECC_ALLOC_INVALID_PTR;
    }
    
    /* Check magic number */
    if (block->header.magic != ECC_ALLOC_BLOCK_MAGIC) {
        return E_ECC_ALLOC_HEAP_CORRUPT;
    }
    
    /* Check if already freed */
    if (block->header.size == 0U && block->header.guard_front == ECC_ALLOC_FREED_PATTERN) {
        return E_ECC_ALLOC_DOUBLE_FREE;
    }
    
    return E_OK;
}

/**
 * @brief Set guard words for a block
 */
static void EccAllocator_SetGuards(EccMemBlockType *block)
{
    uint8_t *user_data;
    uint32_t *front_guard;
    uint32_t *rear_guard;
    
    if (block == NULL) {
        return;
    }
    
    user_data = (uint8_t*)EccAllocator_BlockToPtr(block);
    
    /* Front guard (before user data) */
    if (block->header.flags & ECC_ALLOC_FLAG_GUARD) {
        front_guard = (uint32_t*)user_data;
        *front_guard = ECC_ALLOC_GUARD_PATTERN;
        user_data += sizeof(uint32_t);
    }
    
    /* ECC storage (after user data) */
    if (block->header.flags & ECC_ALLOC_FLAG_ECC) {
        uint32_t num_words = (block->header.size + 3U) / 4U;
        uint8_t *ecc_storage = user_data + block->header.size;
        uint32_t i;
        uint32_t *data_words = (uint32_t*)user_data;
        
        for (i = 0U; i < num_words; i++) {
            EccEncoder_Encode32(data_words[i], &ecc_storage[i]);
        }
        
        /* Rear guard after ECC storage */
        rear_guard = (uint32_t*)(ecc_storage + num_words);
    } else {
        /* Rear guard after user data */
        rear_guard = (uint32_t*)(user_data + block->header.size);
    }
    
    *rear_guard = ECC_ALLOC_GUARD_PATTERN;
}

/**
 * @brief Check guard words for corruption
 */
static Std_ReturnType EccAllocator_CheckGuards(EccMemBlockType *block)
{
    uint8_t *user_data;
    uint32_t *front_guard = NULL;
    uint32_t *rear_guard;
    
    if (block == NULL) {
        return E_ECC_ALLOC_INVALID_PTR;
    }
    
    user_data = (uint8_t*)EccAllocator_BlockToPtr(block);
    
    /* Check front guard */
    if (block->header.flags & ECC_ALLOC_FLAG_GUARD) {
        front_guard = (uint32_t*)user_data;
        if (*front_guard != ECC_ALLOC_GUARD_PATTERN) {
            return E_ECC_ALLOC_BUFFER_OVERFLOW;
        }
        user_data += sizeof(uint32_t);
    }
    
    /* Determine rear guard location */
    if (block->header.flags & ECC_ALLOC_FLAG_ECC) {
        uint32_t num_words = (block->header.size + 3U) / 4U;
        uint8_t *ecc_storage = user_data + block->header.size;
        rear_guard = (uint32_t*)(ecc_storage + num_words);
    } else {
        rear_guard = (uint32_t*)(user_data + block->header.size);
    }
    
    /* Check rear guard */
    if (*rear_guard != ECC_ALLOC_GUARD_PATTERN) {
        return E_ECC_ALLOC_BUFFER_OVERFLOW;
    }
    
    return E_OK;
}

/**
 * @brief Core allocation function
 */
static void* EccAllocator_DoAlloc(uint32_t size, uint32_t alignment, uint32_t flags)
{
    EccMemBlockType *block;
    uint32_t block_size;
    void *user_ptr;
    uint32_t padding = 0U;
    
    /* Check size */
    if (size == 0U || size > g_heap_state.config.heap_size) {
        g_heap_state.stats.failed_allocations++;
        if (g_failure_callback != NULL) {
            g_failure_callback(size, flags);
        }
        return NULL;
    }
    
    /* Calculate required block size */
    block_size = EccAllocator_CalculateBlockSize(size, flags);
    
    /* Find free block */
    block = EccAllocator_FindFreeBlock(block_size, alignment);
    if (block == NULL) {
        /* Try coalescing free blocks */
        EccAllocator_CoalesceFreeBlocks();
        block = EccAllocator_FindFreeBlock(block_size, alignment);
        
        if (block == NULL) {
            g_heap_state.stats.failed_allocations++;
            if (g_failure_callback != NULL) {
                g_failure_callback(size, flags);
            }
            return NULL;
        }
    }
    
    /* Split block if too large */
    EccAllocator_SplitBlock(block, block_size);
    
    /* Remove from free list */
    EccAllocator_RemoveFromFreeList(block);
    
    /* Calculate alignment padding */
    user_ptr = EccAllocator_BlockToPtr(block);
    if (alignment > 0U) {
        uint32_t aligned_addr = ((uint32_t)user_ptr + alignment - 1U) & ~(alignment - 1U);
        padding = aligned_addr - (uint32_t)user_ptr;
        if (padding > 0U) {
            user_ptr = (void*)aligned_addr;
        }
    }
    
    /* Initialize header */
    block->header.size = size;
    block->header.actual_size = block_size;
    block->header.flags = flags;
    block->header.sequence = g_heap_state.sequence_counter++;
    block->header.guard_front = ECC_ALLOC_GUARD_PATTERN;
    
    /* Zero memory if requested */
    if (flags & ECC_ALLOC_FLAG_ZERO) {
        memset(user_ptr, 0, size);
    }
    
    /* Set guard words and ECC codes */
    EccAllocator_SetGuards(block);
    
    /* Add to used list */
    EccAllocator_AddToUsedList(block);
    
    /* Update statistics */
    g_heap_state.stats.total_allocations++;
    g_heap_state.stats.current_used += block_size;
    g_heap_state.stats.total_bytes_allocated += size;
    g_heap_state.stats.used_blocks++;
    
    if (g_heap_state.stats.current_used > g_heap_state.stats.peak_used) {
        g_heap_state.stats.peak_used = g_heap_state.stats.current_used;
    }
    
    return user_ptr;
}

/******************************************************************************
 * Public API Implementation
 ******************************************************************************/

/**
 * @brief Initialize ECC allocator
 */
Std_ReturnType EccAllocator_Init(const EccHeapConfigType *config)
{
    uint8_t *heap_base;
    uint32_t heap_size;
    EccMemBlockType *initial_block;
    
    /* Use default config if none provided */
    if (config != NULL) {
        g_heap_state.config = *config;
    } else {
        memset(&g_heap_state.config, 0, sizeof(g_heap_state.config));
        g_heap_state.config.heap_base = g_default_heap;
        g_heap_state.config.heap_size = ECC_ALLOC_HEAP_SIZE;
        g_heap_state.config.alignment = ECC_ALLOC_ALIGN_DEFAULT;
        g_heap_state.config.enable_ecc = TRUE;
        g_heap_state.config.enable_guard = TRUE;
    }
    
    heap_base = g_heap_state.config.heap_base;
    heap_size = g_heap_state.config.heap_size;
    
    if (heap_base == NULL || heap_size < MIN_BLOCK_SIZE) {
        return E_NOT_OK;
    }
    
    /* Clear heap */
    memset(heap_base, 0, heap_size);
    
    /* Initialize heap state */
    g_heap_state.heap_start = heap_base;
    g_heap_state.heap_end = heap_base + heap_size;
    g_heap_state.free_list = NULL;
    g_heap_state.used_list = NULL;
    g_heap_state.sequence_counter = 1U;
    g_heap_state.ecc_pool = g_ecc_pool;
    g_heap_state.ecc_pool_size = sizeof(g_ecc_pool);
    g_heap_state.ecc_pool_used = 0U;
    
    memset(&g_heap_state.stats, 0, sizeof(g_heap_state.stats));
    
    /* Create initial free block covering entire heap */
    initial_block = (EccMemBlockType*)heap_base;
    initial_block->header.magic = ECC_ALLOC_BLOCK_MAGIC;
    initial_block->header.size = 0U;  /* Free block */
    initial_block->header.actual_size = heap_size;
    initial_block->header.flags = 0U;
    initial_block->header.sequence = 0U;
    initial_block->header.guard_front = ECC_ALLOC_FREED_PATTERN;
    initial_block->header.prev = NULL;
    initial_block->header.next = NULL;
    
    g_heap_state.free_list = initial_block;
    g_heap_state.stats.free_blocks = 1U;
    g_heap_state.stats.largest_free_block = heap_size;
    
    /* Initialize ECC encoder */
    {
        EccEncoderConfigType enc_config;
        memset(&enc_config, 0, sizeof(enc_config));
        enc_config.mode = ECC_MODE_32BIT;
        enc_config.asil_level = g_heap_state.config.asil_level;
        EccEncoder_Init(&enc_config);
    }
    
    g_heap_state.initialized = TRUE;
    g_initialized = TRUE;
    
    return E_OK;
}

/**
 * @brief Deinitialize ECC allocator
 */
Std_ReturnType EccAllocator_DeInit(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Clear heap if scrub enabled */
    if (g_heap_state.config.auto_scrub) {
        memset(g_heap_state.config.heap_base, 
               (int)(g_heap_state.config.scrub_pattern & 0xFF),
               g_heap_state.config.heap_size);
    }
    
    memset(&g_heap_state, 0, sizeof(g_heap_state));
    g_initialized = FALSE;
    
    EccEncoder_DeInit();
    
    return E_OK;
}

/**
 * @brief Allocate memory
 */
void* EccAllocator_Malloc(uint32_t size, uint32_t flags)
{
    uint32_t alignment = ECC_ALLOC_ALIGN_DEFAULT;
    
    if (!g_initialized) {
        return NULL;
    }
    
    /* Apply default flags from config */
    if (g_heap_state.config.enable_ecc) {
        flags |= ECC_ALLOC_FLAG_ECC;
    }
    if (g_heap_state.config.enable_guard) {
        flags |= ECC_ALLOC_FLAG_GUARD;
    }
    
    if (flags & ECC_ALLOC_FLAG_ALIGNED) {
        alignment = ECC_ALLOC_ALIGN_CACHE;
    }
    
    return EccAllocator_DoAlloc(size, alignment, flags);
}

/**
 * @brief Allocate zeroed memory
 */
void* EccAllocator_Calloc(uint32_t num, uint32_t size, uint32_t flags)
{
    uint32_t total_size;
    
    /* Check for overflow */
    if (num > 0U && size > 0xFFFFFFFFU / num) {
        return NULL;
    }
    
    total_size = num * size;
    flags |= ECC_ALLOC_FLAG_ZERO;
    
    return EccAllocator_Malloc(total_size, flags);
}

/**
 * @brief Reallocate memory
 */
void* EccAllocator_Realloc(void *ptr, uint32_t new_size, uint32_t flags)
{
    void *new_ptr;
    EccMemBlockType *old_block;
    uint32_t old_size;
    
    if (!g_initialized) {
        return NULL;
    }
    
    /* If ptr is NULL, behave like malloc */
    if (ptr == NULL) {
        return EccAllocator_Malloc(new_size, flags);
    }
    
    /* If size is 0, behave like free */
    if (new_size == 0U) {
        EccAllocator_Free(ptr);
        return NULL;
    }
    
    /* Validate old block */
    old_block = EccAllocator_PtrToBlock(ptr);
    if (EccAllocator_ValidateBlock(old_block) != E_OK) {
        return NULL;
    }
    
    old_size = old_block->header.size;
    
    /* Allocate new block */
    new_ptr = EccAllocator_Malloc(new_size, flags);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* Copy data (min of old and new size) */
    memcpy(new_ptr, ptr, (new_size < old_size) ? new_size : old_size);
    
    /* Free old block */
    EccAllocator_Free(ptr);
    
    return new_ptr;
}

/**
 * @brief Free allocated memory
 */
Std_ReturnType EccAllocator_Free(void *ptr)
{
    EccMemBlockType *block;
    Std_ReturnType status;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* NULL free is valid (no-op) */
    if (ptr == NULL) {
        return E_OK;
    }
    
    /* Validate pointer is from our heap */
    if (!EccAllocator_IsHeapPtr(ptr)) {
        return E_ECC_ALLOC_INVALID_PTR;
    }
    
    block = EccAllocator_PtrToBlock(ptr);
    
    /* Validate block */
    status = EccAllocator_ValidateBlock(block);
    if (status != E_OK) {
        if (status == E_ECC_ALLOC_DOUBLE_FREE) {
            g_heap_state.stats.double_frees++;
        } else if (status == E_ECC_ALLOC_HEAP_CORRUPT) {
            g_heap_state.stats.heap_corruptions++;
        }
        return status;
    }
    
    /* Check for buffer overflow */
    status = EccAllocator_CheckGuards(block);
    if (status != E_OK) {
        g_heap_state.stats.overflows_detected++;
        return status;
    }
    
    /* Remove from used list */
    EccAllocator_RemoveFromUsedList(block);
    
    /* Update statistics */
    g_heap_state.stats.total_frees++;
    g_heap_state.stats.current_used -= block->header.actual_size;
    g_heap_state.stats.used_blocks--;
    
    /* Scrub memory if enabled */
    if (g_heap_state.config.auto_scrub) {
        memset(ptr, (int)(g_heap_state.config.scrub_pattern & 0xFF), 
               block->header.size);
    }
    
    /* Mark as freed */
    block->header.size = 0U;
    block->header.flags = 0U;
    block->header.guard_front = ECC_ALLOC_FREED_PATTERN;
    
    /* Add to free list */
    EccAllocator_AddToFreeList(block);
    
    /* Try to coalesce with neighbors */
    EccAllocator_CoalesceFreeBlocks();
    
    return E_OK;
}

/**
 * @brief Allocate aligned memory
 */
void* EccAllocator_AlignedAlloc(uint32_t size, uint32_t alignment, uint32_t flags)
{
    if (!g_initialized) {
        return NULL;
    }
    
    /* Alignment must be power of 2 */
    if (alignment == 0U || (alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }
    
    flags |= ECC_ALLOC_FLAG_ALIGNED;
    
    return EccAllocator_DoAlloc(size, alignment, flags);
}

/**
 * @brief Verify memory block integrity
 */
Std_ReturnType EccAllocator_Verify(void *ptr, EccCheckResultType *result)
{
    EccMemBlockType *block;
    Std_ReturnType status;
    
    if (!g_initialized || ptr == NULL) {
        return E_ECC_ALLOC_INVALID_PTR;
    }
    
    block = EccAllocator_PtrToBlock(ptr);
    
    /* Validate block structure */
    status = EccAllocator_ValidateBlock(block);
    if (status != E_OK) {
        return status;
    }
    
    /* Check guards */
    status = EccAllocator_CheckGuards(block);
    if (status != E_OK) {
        return status;
    }
    
    /* Check ECC if enabled */
    if ((block->header.flags & ECC_ALLOC_FLAG_ECC) && result != NULL) {
        uint8_t *user_data = (uint8_t*)ptr;
        uint32_t num_words = (block->header.size + 3U) / 4U;
        uint8_t *ecc_storage = user_data + block->header.size + sizeof(uint32_t);
        uint32_t i;
        
        for (i = 0U; i < num_words; i++) {
            uint32_t *data_words = (uint32_t*)user_data;
            if (EccChecker_CheckAndCorrect32(data_words[i], ecc_storage[i], 
                                              result) != E_OK) {
                g_heap_state.stats.ecc_errors++;
                return E_NOT_OK;
            }
        }
    }
    
    return E_OK;
}

/**
 * @brief Check all allocated blocks
 */
uint32_t EccAllocator_CheckAll(void)
{
    EccMemBlockType *current;
    uint32_t error_count = 0U;
    
    if (!g_initialized) {
        return 0U;
    }
    
    current = g_heap_state.used_list;
    while (current != NULL) {
        if (EccAllocator_Verify(EccAllocator_BlockToPtr(current), NULL) != E_OK) {
            error_count++;
        }
        current = current->header.next;
    }
    
    return error_count;
}

/**
 * @brief Get allocation statistics
 */
const EccAllocStatsType* EccAllocator_GetStats(void)
{
    if (!g_initialized) {
        return NULL;
    }
    
    /* Calculate fragmentation */
    {
        uint32_t total_free = 0U;
        EccMemBlockType *current = g_heap_state.free_list;
        
        while (current != NULL) {
            total_free += current->header.actual_size;
            current = current->header.next;
        }
        
        if (total_free > 0U) {
            g_heap_state.stats.fragmentation_percent = 
                10000U - ((g_heap_state.stats.largest_free_block * 10000U) / total_free);
        }
    }
    
    return &g_heap_state.stats;
}

/**
 * @brief Reset statistics
 */
Std_ReturnType EccAllocator_ResetStats(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    memset(&g_heap_state.stats, 0, sizeof(g_heap_state.stats));
    
    return E_OK;
}

/**
 * @brief Get allocation info
 */
Std_ReturnType EccAllocator_GetInfo(void *ptr, EccAllocInfoType *info)
{
    EccMemBlockType *block;
    
    if (!g_initialized || ptr == NULL || info == NULL) {
        return E_NOT_OK;
    }
    
    block = EccAllocator_PtrToBlock(ptr);
    
    if (EccAllocator_ValidateBlock(block) != E_OK) {
        return E_NOT_OK;
    }
    
    info->ptr = ptr;
    info->size = block->header.size;
    info->sequence = block->header.sequence;
    info->flags = block->header.flags;
    info->is_valid = TRUE;
    info->has_ecc = (block->header.flags & ECC_ALLOC_FLAG_ECC) ? TRUE : FALSE;
    info->ecc_code = block->header.ecc_code;
    
    return E_OK;
}

/**
 * @brief Get heap state
 */
const EccHeapStateType* EccAllocator_GetHeapState(void)
{
    return &g_heap_state;
}

/**
 * @brief Get total heap size
 */
uint32_t EccAllocator_GetHeapSize(void)
{
    if (!g_initialized) {
        return 0U;
    }
    
    return g_heap_state.config.heap_size;
}

/**
 * @brief Get free heap space
 */
uint32_t EccAllocator_GetFreeSize(void)
{
    uint32_t free_size = 0U;
    EccMemBlockType *current;
    
    if (!g_initialized) {
        return 0U;
    }
    
    current = g_heap_state.free_list;
    while (current != NULL) {
        free_size += current->header.actual_size;
        current = current->header.next;
    }
    
    return free_size;
}

/**
 * @brief Get largest free block
 */
uint32_t EccAllocator_GetLargestFree(void)
{
    uint32_t largest = 0U;
    EccMemBlockType *current;
    
    if (!g_initialized) {
        return 0U;
    }
    
    current = g_heap_state.free_list;
    while (current != NULL) {
        if (current->header.actual_size > largest) {
            largest = current->header.actual_size;
        }
        current = current->header.next;
    }
    
    g_heap_state.stats.largest_free_block = largest;
    
    return largest;
}

/**
 * @brief Dump heap statistics (debug)
 */
void EccAllocator_DumpStats(void)
{
    const EccAllocStatsType *stats;
    
    if (!g_initialized) {
        return;
    }
    
    stats = EccAllocator_GetStats();
    if (stats == NULL) {
        return;
    }
    
    /* Debug output would go here in real implementation */
    (void)stats;
}

/**
 * @brief Set failure callback
 */
void EccAllocator_SetFailureCallback(void (*callback)(uint32_t size, uint32_t flags))
{
    g_failure_callback = callback;
}

/**
 * @brief Get error string
 */
const char* EccAllocator_ErrorString(Std_ReturnType error)
{
    switch (error) {
        case E_OK:
            return "OK";
        case E_ECC_ALLOC_NO_MEMORY:
            return "Out of memory";
        case E_ECC_ALLOC_INVALID_SIZE:
            return "Invalid size";
        case E_ECC_ALLOC_INVALID_PTR:
            return "Invalid pointer";
        case E_ECC_ALLOC_DOUBLE_FREE:
            return "Double free detected";
        case E_ECC_ALLOC_BUFFER_OVERFLOW:
            return "Buffer overflow detected";
        case E_ECC_ALLOC_HEAP_CORRUPT:
            return "Heap corruption detected";
        case E_ECC_ALLOC_ALIGNMENT_ERR:
            return "Alignment error";
        default:
            return "Unknown error";
    }
}

/**
 * @brief Get version information
 */
void EccAllocator_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = ECC_ALLOCATOR_VENDOR_ID;
        version->moduleID = ECC_ALLOCATOR_MODULE_ID;
        version->sw_major_version = ECC_ALLOCATOR_SW_MAJOR;
        version->sw_minor_version = ECC_ALLOCATOR_SW_MINOR;
        version->sw_patch_version = ECC_ALLOCATOR_SW_PATCH;
    }
}
