/******************************************************************************
 * @file    heap_protection.c
 * @brief   Heap Protection Module Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#include "heap_protection.h"
#include <string.h>

/******************************************************************************
 * Private Constants
 ******************************************************************************/

#define HEAP_HEADER_SIZE        sizeof(HeapBlockHeaderType)
#define HEAP_OVERHEAD           (HEAP_HEADER_SIZE + 2 * HEAP_PROT_GUARD_SIZE)

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static HeapConfigType g_config;
static HeapInfoType g_heap_info;
static HeapBlockHeaderType *g_block_list = NULL;
static HeapBlockHeaderType *g_free_list = NULL;
static HeapViolationType g_last_violation;
static HeapViolationCallbackType g_violation_callback = NULL;
static uint32_t g_alloc_counter = 0;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static void InitBlockHeader(HeapBlockHeaderType *header, uint32_t size, uint8_t flags);
static void UpdateBlockChecksum(HeapBlockHeaderType *header);
static boolean VerifyBlockChecksum(const HeapBlockHeaderType *header);
static void LinkBlock(HeapBlockHeaderType *block);
static void UnlinkBlock(HeapBlockHeaderType *block);
static void ReportViolation(const HeapViolationType *violation);
static void FillGuardZone(uint32_t addr, uint32_t size);
static boolean CheckGuardZone(uint32_t addr, uint32_t size);
static HeapBlockHeaderType* FindFreeBlock(uint32_t size);
static boolean IsValidHeapPtr(const void *ptr);

/******************************************************************************
 * Function Implementations - Initialization
 ******************************************************************************/

Std_ReturnType HeapProtection_Init(const HeapConfigType *config)
{
    if (g_initialized) {
        return E_OK;
    }
    
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    /* Validate configuration */
    if (config->size == 0 || config->start_addr == 0) {
        return E_NOT_OK;
    }
    
    /* Copy configuration */
    memcpy(&g_config, config, sizeof(HeapConfigType));
    
    /* Initialize heap info */
    memset(&g_heap_info, 0, sizeof(HeapInfoType));
    g_heap_info.state = HEAP_STATE_HEALTHY;
    g_heap_info.initialized = TRUE;
    g_heap_info.locked = FALSE;
    g_heap_info.stats.total_size = config->size;
    g_heap_info.stats.free_size = config->size;
    
    /* Initialize lists */
    g_block_list = NULL;
    g_free_list = NULL;
    
    /* Clear violation record */
    memset(&g_last_violation, 0, sizeof(HeapViolationType));
    
    g_alloc_counter = 1;
    g_initialized = TRUE;
    
    return E_OK;
}

Std_ReturnType HeapProtection_DeInit(void)
{
    HeapBlockHeaderType *block;
    HeapBlockHeaderType *next;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Free all allocated blocks */
    block = g_block_list;
    while (block != NULL) {
        next = block->next;
        
        if (block->state == HEAP_BLOCK_STATE_ALLOCATED) {
            /* Clear user data */
            void *user_ptr = HeapProtection_GetUserPtr(block);
            if (user_ptr != NULL && g_config.flags & HEAP_FLAG_ZERO_ON_FREE) {
                HeapProtection_SecureClear(user_ptr, block->size);
            }
        }
        
        block = next;
    }
    
    /* Clear all state */
    memset(&g_config, 0, sizeof(HeapConfigType));
    memset(&g_heap_info, 0, sizeof(HeapInfoType));
    g_block_list = NULL;
    g_free_list = NULL;
    g_violation_callback = NULL;
    g_initialized = FALSE;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Safe Memory Allocation
 ******************************************************************************/

void* HeapProtection_Malloc(uint32_t size, uint8_t flags)
{
    HeapBlockHeaderType *block;
    HeapBlockHeaderType *new_block;
    uint32_t alloc_size;
    uint32_t remaining;
    void *user_ptr;
    
    if (!g_initialized || size == 0 || g_heap_info.locked) {
        g_heap_info.stats.fail_count++;
        return NULL;
    }
    
    /* Check if size exceeds heap */
    alloc_size = HEAP_OVERHEAD + ((size + HEAP_PROT_ALIGNMENT - 1) & ~(HEAP_PROT_ALIGNMENT - 1));
    if (alloc_size > g_heap_info.stats.free_size) {
        g_heap_info.stats.fail_count++;
        return NULL;
    }
    
    /* Try to find free block */
    block = FindFreeBlock(size);
    if (block != NULL) {
        /* Reuse existing free block */
        block->state = HEAP_BLOCK_STATE_ALLOCATED;
        block->magic = HEAP_BLOCK_MAGIC_ALLOC;
        block->flags = flags;
        block->alloc_id = g_alloc_counter++;
        block->alloc_time = 0;  /* Should get from OS */
        
        /* Update checksum */
        UpdateBlockChecksum(block);
        
        /* Initialize guard zones if enabled */
        if (g_config.flags & HEAP_FLAG_USE_GUARDS) {
            HeapProtection_InitGuards(block);
        }
        
        /* Zero memory if requested */
        user_ptr = HeapProtection_GetUserPtr(block);
        if (flags & HEAP_FLAG_ZERO_ON_ALLOC) {
            memset(user_ptr, 0, block->size);
        } else {
            /* Fill with allocation pattern */
            HeapProtection_FillPattern(user_ptr, block->size, HEAP_FILL_PATTERN_ALLOC);
        }
        
        /* Update statistics */
        g_heap_info.stats.alloc_count++;
        g_heap_info.stats.used_size += block->total_size;
        g_heap_info.stats.free_size -= block->total_size;
        g_heap_info.stats.block_count++;
        
        if (g_heap_info.stats.used_size > g_heap_info.stats.peak_used) {
            g_heap_info.stats.peak_used = g_heap_info.stats.used_size;
        }
        
        return user_ptr;
    }
    
    /* Allocate from heap space */
    if (g_heap_info.stats.free_size < alloc_size) {
        g_heap_info.stats.fail_count++;
        return NULL;
    }
    
    /* Calculate new block address */
    block = (HeapBlockHeaderType *)(g_config.start_addr + 
             (g_config.size - g_heap_info.stats.free_size));
    
    /* Initialize block header */
    InitBlockHeader(block, size, flags);
    block->alloc_id = g_alloc_counter++;
    block->alloc_time = 0;
    
    /* Link into block list */
    LinkBlock(block);
    
    /* Initialize guard zones if enabled */
    if (g_config.flags & HEAP_FLAG_USE_GUARDS) {
        HeapProtection_InitGuards(block);
    }
    
    /* Get user pointer */
    user_ptr = HeapProtection_GetUserPtr(block);
    
    /* Initialize memory */
    if (flags & HEAP_FLAG_ZERO_ON_ALLOC) {
        memset(user_ptr, 0, size);
    } else {
        HeapProtection_FillPattern(user_ptr, size, HEAP_FILL_PATTERN_ALLOC);
    }
    
    /* Update statistics */
    g_heap_info.stats.alloc_count++;
    g_heap_info.stats.used_size += block->total_size;
    g_heap_info.stats.free_size -= block->total_size;
    g_heap_info.stats.block_count++;
    
    if (g_heap_info.stats.used_size > g_heap_info.stats.peak_used) {
        g_heap_info.stats.peak_used = g_heap_info.stats.used_size;
    }
    
    return user_ptr;
}

void* HeapProtection_Calloc(uint32_t num, uint32_t size, uint8_t flags)
{
    uint32_t total_size;
    void *ptr;
    
    if (num == 0 || size == 0) {
        return NULL;
    }
    
    /* Check for overflow */
    total_size = num * size;
    if (total_size / num != size) {
        return NULL;  /* Overflow */
    }
    
    /* Allocate with zero flag */
    ptr = HeapProtection_Malloc(total_size, flags | HEAP_FLAG_ZERO_ON_ALLOC);
    
    return ptr;
}

void* HeapProtection_Realloc(void *ptr, uint32_t new_size, uint8_t flags)
{
    void *new_ptr;
    HeapBlockHeaderType *header;
    uint32_t old_size;
    
    if (ptr == NULL) {
        /* Behave like malloc */
        return HeapProtection_Malloc(new_size, flags);
    }
    
    if (new_size == 0) {
        /* Behave like free */
        HeapProtection_Free(ptr, flags);
        return NULL;
    }
    
    /* Get current block info */
    header = HeapProtection_GetHeader(ptr);
    if (header == NULL || header->state != HEAP_BLOCK_STATE_ALLOCATED) {
        return NULL;
    }
    
    old_size = header->size;
    
    /* Allocate new block */
    new_ptr = HeapProtection_Malloc(new_size, flags);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* Copy data */
    memcpy(new_ptr, ptr, (old_size < new_size) ? old_size : new_size);
    
    /* Free old block */
    HeapProtection_Free(ptr, flags);
    
    return new_ptr;
}

Std_ReturnType HeapProtection_Free(void *ptr, uint8_t flags)
{
    HeapBlockHeaderType *header;
    boolean was_freed;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    if (ptr == NULL) {
        return E_OK;  /* Free of NULL is allowed */
    }
    
    /* Validate pointer */
    if (!IsValidHeapPtr(ptr)) {
        g_heap_info.stats.corruption_count++;
        return E_NOT_OK;
    }
    
    /* Get block header */
    header = HeapProtection_GetHeader(ptr);
    
    /* Check for double-free */
    if (HeapProtection_WasFreed(ptr, &was_freed) == E_OK && was_freed) {
        HeapViolationType violation;
        violation.block_addr = (uint32_t)header;
        violation.violation_addr = (uint32_t)ptr;
        violation.violation_type = HEAP_ERR_DOUBLE_FREE;
        violation.expected_state = HEAP_BLOCK_STATE_ALLOCATED;
        violation.actual_state = header->state;
        violation.timestamp = 0;
        violation.task_id = 0;
        
        g_heap_info.stats.double_free_count++;
        ReportViolation(&violation);
        
        if (g_config.flags & HEAP_FLAG_LOCK_ON_CORRUPTION) {
            HeapProtection_Lock();
        }
        
        return E_NOT_OK;
    }
    
    /* Validate block */
    if (header->magic != HEAP_BLOCK_MAGIC_ALLOC ||
        header->state != HEAP_BLOCK_STATE_ALLOCATED) {
        g_heap_info.stats.corruption_count++;
        return E_NOT_OK;
    }
    
    /* Verify checksum */
    if (!VerifyBlockChecksum(header)) {
        HeapViolationType violation;
        violation.block_addr = (uint32_t)header;
        violation.violation_addr = (uint32_t)ptr;
        violation.violation_type = HEAP_ERR_CORRUPTION;
        violation.expected_state = 0;
        violation.actual_state = 0;
        violation.timestamp = 0;
        violation.task_id = 0;
        
        g_heap_info.stats.corruption_count++;
        ReportViolation(&violation);
        
        return E_NOT_OK;
    }
    
    /* Clear user data if requested */
    if (g_config.flags & HEAP_FLAG_ZERO_ON_FREE) {
        HeapProtection_ClearOnFree(ptr, header->size);
    } else {
        /* Fill with free pattern */
        HeapProtection_FillPattern(ptr, header->size, HEAP_FILL_PATTERN_FREE);
    }
    
    /* Mark as free */
    header->state = HEAP_BLOCK_STATE_FREE;
    header->magic = HEAP_BLOCK_MAGIC_FREE;
    UpdateBlockChecksum(header);
    
    /* Update statistics */
    g_heap_info.stats.free_count++;
    g_heap_info.stats.used_size -= header->total_size;
    g_heap_info.stats.free_size += header->total_size;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Memory Validation
 ******************************************************************************/

Std_ReturnType HeapProtection_ValidateBlock(
    const void *ptr,
    BlockValidationType *result)
{
    HeapBlockHeaderType *header;
    boolean guards_ok;
    
    if (!g_initialized || result == NULL) {
        return E_NOT_OK;
    }
    
    result->is_valid = FALSE;
    result->error_code = HEAP_ERR_NONE;
    result->guards_intact = TRUE;
    result->in_bounds = FALSE;
    
    if (ptr == NULL) {
        result->error_code = HEAP_ERR_NULL_PTR;
        return E_OK;
    }
    
    if (!IsValidHeapPtr(ptr)) {
        result->error_code = HEAP_ERR_INVALID_PTR;
        return E_OK;
    }
    
    header = HeapProtection_GetHeader(ptr);
    
    /* Check bounds */
    result->in_bounds = ((uint32_t)header >= g_config.start_addr &&
                         (uint32_t)header < (g_config.start_addr + g_config.size));
    
    if (!result->in_bounds) {
        result->error_code = HEAP_ERR_INVALID_PTR;
        return E_OK;
    }
    
    /* Check magic */
    if (header->magic != HEAP_BLOCK_MAGIC_ALLOC) {
        result->error_code = HEAP_ERR_CORRUPTION;
        return E_OK;
    }
    
    /* Check state */
    if (header->state != HEAP_BLOCK_STATE_ALLOCATED) {
        result->error_code = HEAP_ERR_INVALID_PTR;
        return E_OK;
    }
    
    /* Verify checksum */
    result->stored_checksum = header->checksum;
    result->header_checksum = HeapProtection_CalcChecksum(header);
    
    if (result->stored_checksum != result->header_checksum) {
        result->error_code = HEAP_ERR_CORRUPTION;
        return E_OK;
    }
    
    /* Check guards if enabled */
    if (g_config.flags & HEAP_FLAG_USE_GUARDS) {
        if (HeapProtection_CheckGuards(header, &guards_ok) == E_OK) {
            result->guards_intact = guards_ok;
            if (!guards_ok) {
                result->error_code = HEAP_ERR_GUARD_VIOLATION;
                return E_OK;
            }
        }
    }
    
    result->is_valid = TRUE;
    return E_OK;
}

uint32_t HeapProtection_ValidateHeap(void)
{
    HeapBlockHeaderType *block;
    uint32_t corruption_count = 0;
    
    if (!g_initialized) {
        return 0;
    }
    
    block = g_block_list;
    while (block != NULL) {
        if (block->state == HEAP_BLOCK_STATE_ALLOCATED) {
            if (!VerifyBlockChecksum(block)) {
                corruption_count++;
                block->state = HEAP_BLOCK_STATE_CORRUPTED;
            }
        }
        block = block->next;
    }
    
    return corruption_count;
}

Std_ReturnType HeapProtection_CheckOverflow(const void *ptr, boolean *overflow)
{
    HeapBlockHeaderType *header;
    boolean guards_ok;
    
    if (!g_initialized || overflow == NULL) {
        return E_NOT_OK;
    }
    
    *overflow = FALSE;
    
    if (!IsValidHeapPtr(ptr)) {
        return E_NOT_OK;
    }
    
    header = HeapProtection_GetHeader(ptr);
    
    if (!(g_config.flags & HEAP_FLAG_USE_GUARDS)) {
        return E_OK;  /* Cannot check without guards */
    }
    
    /* Check top guard zone */
    guards_ok = CheckGuardZone(
        (uint32_t)ptr + header->size, HEAP_PROT_GUARD_SIZE);
    
    if (!guards_ok) {
        *overflow = TRUE;
        g_heap_info.stats.overflow_count++;
        
        HeapViolationType violation;
        violation.block_addr = (uint32_t)header;
        violation.violation_addr = (uint32_t)ptr + header->size;
        violation.violation_type = HEAP_ERR_OVERFLOW;
        violation.expected_state = 0;
        violation.actual_state = 0;
        violation.timestamp = 0;
        violation.task_id = 0;
        
        ReportViolation(&violation);
    }
    
    return E_OK;
}

Std_ReturnType HeapProtection_CheckUnderflow(const void *ptr, boolean *underflow)
{
    HeapBlockHeaderType *header;
    boolean guards_ok;
    
    if (!g_initialized || underflow == NULL) {
        return E_NOT_OK;
    }
    
    *underflow = FALSE;
    
    if (!IsValidHeapPtr(ptr)) {
        return E_NOT_OK;
    }
    
    header = HeapProtection_GetHeader(ptr);
    
    if (!(g_config.flags & HEAP_FLAG_USE_GUARDS)) {
        return E_OK;  /* Cannot check without guards */
    }
    
    /* Check bottom guard zone (between header and user data) */
    guards_ok = CheckGuardZone(
        (uint32_t)header + HEAP_HEADER_SIZE, HEAP_PROT_GUARD_SIZE);
    
    if (!guards_ok) {
        *underflow = TRUE;
        g_heap_info.stats.underflow_count++;
        
        HeapViolationType violation;
        violation.block_addr = (uint32_t)header;
        violation.violation_addr = (uint32_t)ptr - 1;
        violation.violation_type = HEAP_ERR_UNDERFLOW;
        violation.expected_state = 0;
        violation.actual_state = 0;
        violation.timestamp = 0;
        violation.task_id = 0;
        
        ReportViolation(&violation);
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Overlap Detection
 ******************************************************************************/

uint32_t HeapProtection_CheckOverlaps(void)
{
    HeapBlockHeaderType *block1;
    HeapBlockHeaderType *block2;
    uint32_t overlap_count = 0;
    uint32_t start1, end1, start2, end2;
    
    if (!g_initialized) {
        return 0;
    }
    
    block1 = g_block_list;
    while (block1 != NULL) {
        if (block1->state == HEAP_BLOCK_STATE_ALLOCATED) {
            start1 = (uint32_t)block1;
            end1 = start1 + block1->total_size;
            
            block2 = block1->next;
            while (block2 != NULL) {
                if (block2->state == HEAP_BLOCK_STATE_ALLOCATED) {
                    start2 = (uint32_t)block2;
                    end2 = start2 + block2->total_size;
                    
                    if ((start1 < end2) && (start2 < end1)) {
                        overlap_count++;
                    }
                }
                block2 = block2->next;
            }
        }
        block1 = block1->next;
    }
    
    return overlap_count;
}

boolean HeapProtection_RegionsOverlap(
    const void *ptr1, uint32_t size1,
    const void *ptr2, uint32_t size2)
{
    uint32_t start1, end1, start2, end2;
    
    if (ptr1 == NULL || ptr2 == NULL) {
        return FALSE;
    }
    
    start1 = (uint32_t)ptr1;
    end1 = start1 + size1;
    start2 = (uint32_t)ptr2;
    end2 = start2 + size2;
    
    return (start1 < end2) && (start2 < end1);
}

/******************************************************************************
 * Function Implementations - Double-Free Detection
 ******************************************************************************/

Std_ReturnType HeapProtection_WasFreed(const void *ptr, boolean *was_freed)
{
    HeapBlockHeaderType *header;
    
    if (!g_initialized || was_freed == NULL) {
        return E_NOT_OK;
    }
    
    *was_freed = FALSE;
    
    if (!IsValidHeapPtr(ptr)) {
        return E_NOT_OK;
    }
    
    header = HeapProtection_GetHeader(ptr);
    
    /* Check if marked as free */
    if (header->state == HEAP_BLOCK_STATE_FREE ||
        header->magic == HEAP_BLOCK_MAGIC_FREE) {
        *was_freed = TRUE;
    }
    
    return E_OK;
}

void HeapProtection_MarkFreed(HeapBlockHeaderType *block)
{
    if (block == NULL) {
        return;
    }
    
    block->state = HEAP_BLOCK_STATE_FREE;
    block->magic = HEAP_BLOCK_MAGIC_FREE;
    UpdateBlockChecksum(block);
}

/******************************************************************************
 * Function Implementations - Memory Clearing
 ******************************************************************************/

void HeapProtection_SecureClear(void *ptr, uint32_t size)
{
    volatile uint8_t *p = ptr;
    uint32_t i;
    
    if (ptr == NULL || size == 0) {
        return;
    }
    
    /* Multiple passes for security */
    for (i = 0; i < size; i++) {
        p[i] = 0x00;
    }
    
    for (i = 0; i < size; i++) {
        p[i] = 0xFF;
    }
    
    for (i = 0; i < size; i++) {
        p[i] = 0x00;
    }
}

void HeapProtection_FillPattern(void *ptr, uint32_t size, uint8_t pattern)
{
    if (ptr == NULL || size == 0) {
        return;
    }
    
    memset(ptr, pattern, size);
}

void HeapProtection_ClearOnFree(void *ptr, uint32_t size)
{
    HeapProtection_SecureClear(ptr, size);
}

/******************************************************************************
 * Function Implementations - Guard Zones
 ******************************************************************************/

void HeapProtection_InitGuards(HeapBlockHeaderType *header)
{
    uint32_t bottom_guard;
    uint32_t top_guard;
    
    if (header == NULL) {
        return;
    }
    
    /* Bottom guard (after header, before user data) */
    bottom_guard = (uint32_t)header + HEAP_HEADER_SIZE;
    FillGuardZone(bottom_guard, HEAP_PROT_GUARD_SIZE);
    
    /* Top guard (after user data) */
    top_guard = (uint32_t)HeapProtection_GetUserPtr(header) + header->size;
    FillGuardZone(top_guard, HEAP_PROT_GUARD_SIZE);
}

Std_ReturnType HeapProtection_CheckGuards(
    const HeapBlockHeaderType *header,
    boolean *valid)
{
    uint32_t bottom_guard;
    uint32_t top_guard;
    boolean bottom_ok, top_ok;
    
    if (!g_initialized || header == NULL || valid == NULL) {
        return E_NOT_OK;
    }
    
    /* Bottom guard */
    bottom_guard = (uint32_t)header + HEAP_HEADER_SIZE;
    bottom_ok = CheckGuardZone(bottom_guard, HEAP_PROT_GUARD_SIZE);
    
    /* Top guard */
    top_guard = (uint32_t)HeapProtection_GetUserPtr(header) + header->size;
    top_ok = CheckGuardZone(top_guard, HEAP_PROT_GUARD_SIZE);
    
    *valid = bottom_ok && top_ok;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Safety Checks
 ******************************************************************************/

uint32_t HeapProtection_FullCheck(void)
{
    uint32_t errors = 0;
    
    if (!g_initialized) {
        return 0;
    }
    
    /* Validate all blocks */
    errors += HeapProtection_ValidateHeap();
    
    /* Check for overlaps */
    errors += HeapProtection_CheckOverlaps();
    
    /* Check guard zones for all allocated blocks */
    if (g_config.flags & HEAP_FLAG_USE_GUARDS) {
        HeapBlockHeaderType *block = g_block_list;
        boolean overflow, underflow;
        
        while (block != NULL) {
            if (block->state == HEAP_BLOCK_STATE_ALLOCATED) {
                void *user_ptr = HeapProtection_GetUserPtr(block);
                
                HeapProtection_CheckOverflow(user_ptr, &overflow);
                if (overflow) errors++;
                
                HeapProtection_CheckUnderflow(user_ptr, &underflow);
                if (underflow) errors++;
            }
            block = block->next;
        }
    }
    
    /* Update heap state */
    if (errors > 0) {
        g_heap_info.state = HEAP_STATE_CORRUPTED;
    } else if (g_heap_info.stats.free_size < (g_config.size / 10)) {
        g_heap_info.state = HEAP_STATE_CRITICAL;
    } else if (g_heap_info.stats.fragmentation > 30) {
        g_heap_info.state = HEAP_STATE_FRAGMENTED;
    } else {
        g_heap_info.state = HEAP_STATE_HEALTHY;
    }
    
    return errors;
}

Std_ReturnType HeapProtection_IsCorrupted(boolean *corrupted)
{
    if (!g_initialized || corrupted == NULL) {
        return E_NOT_OK;
    }
    
    *corrupted = (g_heap_info.state == HEAP_STATE_CORRUPTED);
    
    return E_OK;
}

uint8_t HeapProtection_GetState(void)
{
    if (!g_initialized) {
        return HEAP_STATE_CORRUPTED;
    }
    
    return g_heap_info.state;
}

/******************************************************************************
 * Function Implementations - Violation Handling
 ******************************************************************************/

void HeapProtection_SetViolationCallback(HeapViolationCallbackType callback)
{
    g_violation_callback = callback;
}

void HeapProtection_HandleViolation(const HeapViolationType *violation)
{
    if (violation == NULL) {
        return;
    }
    
    /* Store last violation */
    memcpy(&g_last_violation, violation, sizeof(HeapViolationType));
    
    /* Lock heap if configured */
    if (g_config.flags & HEAP_FLAG_LOCK_ON_CORRUPTION) {
        HeapProtection_Lock();
    }
    
    /* Call registered callback */
    if (g_violation_callback != NULL) {
        g_violation_callback(violation);
    }
}

Std_ReturnType HeapProtection_GetLastViolation(HeapViolationType *violation)
{
    if (violation == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(violation, &g_last_violation, sizeof(HeapViolationType));
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Statistics and Info
 ******************************************************************************/

Std_ReturnType HeapProtection_GetInfo(HeapInfoType *info)
{
    if (!g_initialized || info == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(info, &g_heap_info, sizeof(HeapInfoType));
    
    return E_OK;
}

Std_ReturnType HeapProtection_GetStats(HeapStatisticsType *stats)
{
    if (!g_initialized || stats == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(stats, &g_heap_info.stats, sizeof(HeapStatisticsType));
    
    return E_OK;
}

Std_ReturnType HeapProtection_ResetStats(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    g_heap_info.stats.alloc_count = 0;
    g_heap_info.stats.free_count = 0;
    g_heap_info.stats.fail_count = 0;
    g_heap_info.stats.corruption_count = 0;
    g_heap_info.stats.double_free_count = 0;
    g_heap_info.stats.overflow_count = 0;
    g_heap_info.stats.underflow_count = 0;
    g_heap_info.stats.peak_used = g_heap_info.stats.used_size;
    
    return E_OK;
}

Std_ReturnType HeapProtection_CheckLeaks(uint32_t *leak_count)
{
    HeapBlockHeaderType *block;
    
    if (!g_initialized || leak_count == NULL) {
        return E_NOT_OK;
    }
    
    *leak_count = 0;
    
    block = g_block_list;
    while (block != NULL) {
        if (block->state == HEAP_BLOCK_STATE_ALLOCATED) {
            (*leak_count)++;
        }
        block = block->next;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Utility Functions
 ******************************************************************************/

HeapBlockHeaderType* HeapProtection_GetHeader(const void *ptr)
{
    if (ptr == NULL) {
        return NULL;
    }
    
    /* Header is just before user data, with guard zone in between */
    return (HeapBlockHeaderType *)((uint32_t)ptr - HEAP_HEADER_SIZE - HEAP_PROT_GUARD_SIZE);
}

uint16_t HeapProtection_CalcChecksum(const HeapBlockHeaderType *header)
{
    uint16_t checksum = 0;
    const uint8_t *data;
    uint32_t i;
    uint32_t size;
    
    if (header == NULL) {
        return 0;
    }
    
    /* Calculate over all fields except checksum */
    data = (const uint8_t *)header;
    size = sizeof(HeapBlockHeaderType) - sizeof(header->checksum);
    
    for (i = 0; i < size; i++) {
        checksum += data[i];
    }
    
    return checksum;
}

void* HeapProtection_GetUserPtr(const HeapBlockHeaderType *header)
{
    if (header == NULL) {
        return NULL;
    }
    
    /* User data starts after header and bottom guard zone */
    return (void *)((uint32_t)header + HEAP_HEADER_SIZE + HEAP_PROT_GUARD_SIZE);
}

uint32_t HeapProtection_GetSize(const void *ptr)
{
    HeapBlockHeaderType *header;
    
    if (!IsValidHeapPtr(ptr)) {
        return 0;
    }
    
    header = HeapProtection_GetHeader(ptr);
    
    if (header->state != HEAP_BLOCK_STATE_ALLOCATED) {
        return 0;
    }
    
    return header->size;
}

void HeapProtection_DumpState(void)
{
    HeapBlockHeaderType *block;
    
    if (!g_initialized) {
        return;
    }
    
    /* Platform-specific debug output would go here */
    
    block = g_block_list;
    while (block != NULL) {
        block = block->next;
    }
}

void HeapProtection_Lock(void)
{
    g_heap_info.locked = TRUE;
}

void HeapProtection_Unlock(void)
{
    g_heap_info.locked = FALSE;
}

const char* HeapProtection_GetErrorString(uint8_t error_code)
{
    switch (error_code) {
        case HEAP_ERR_NONE: return "No Error";
        case HEAP_ERR_NULL_PTR: return "Null Pointer";
        case HEAP_ERR_INVALID_PTR: return "Invalid Pointer";
        case HEAP_ERR_DOUBLE_FREE: return "Double Free";
        case HEAP_ERR_OVERFLOW: return "Buffer Overflow";
        case HEAP_ERR_UNDERFLOW: return "Buffer Underflow";
        case HEAP_ERR_CORRUPTION: return "Heap Corruption";
        case HEAP_ERR_OUT_OF_MEMORY: return "Out of Memory";
        case HEAP_ERR_INVALID_SIZE: return "Invalid Size";
        case HEAP_ERR_OVERLAP: return "Memory Overlap";
        case HEAP_ERR_GUARD_VIOLATION: return "Guard Zone Violation";
        default: return "Unknown Error";
    }
}

void HeapProtection_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = 0;
        version->moduleID = 0;
        version->sw_major_version = HEAP_PROT_VERSION_MAJOR;
        version->sw_minor_version = HEAP_PROT_VERSION_MINOR;
        version->sw_patch_version = HEAP_PROT_VERSION_PATCH;
    }
}

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static void InitBlockHeader(HeapBlockHeaderType *header, uint32_t size, uint8_t flags)
{
    if (header == NULL) {
        return;
    }
    
    header->magic = HEAP_BLOCK_MAGIC_ALLOC;
    header->size = size;
    header->total_size = HEAP_OVERHEAD + ((size + HEAP_PROT_ALIGNMENT - 1) & ~(HEAP_PROT_ALIGNMENT - 1));
    header->state = HEAP_BLOCK_STATE_ALLOCATED;
    header->flags = flags;
    header->alloc_id = 0;
    header->alloc_time = 0;
    header->owner_task_id = 0;
    header->next = NULL;
    header->prev = NULL;
    
    UpdateBlockChecksum(header);
}

static void UpdateBlockChecksum(HeapBlockHeaderType *header)
{
    if (header == NULL) {
        return;
    }
    
    header->checksum = HeapProtection_CalcChecksum(header);
}

static boolean VerifyBlockChecksum(const HeapBlockHeaderType *header)
{
    uint16_t calc_checksum;
    
    if (header == NULL) {
        return FALSE;
    }
    
    calc_checksum = HeapProtection_CalcChecksum(header);
    return (calc_checksum == header->checksum);
}

static void LinkBlock(HeapBlockHeaderType *block)
{
    if (block == NULL) {
        return;
    }
    
    block->next = g_block_list;
    block->prev = NULL;
    
    if (g_block_list != NULL) {
        g_block_list->prev = block;
    }
    
    g_block_list = block;
}

static void UnlinkBlock(HeapBlockHeaderType *block)
{
    if (block == NULL) {
        return;
    }
    
    if (block->prev != NULL) {
        block->prev->next = block->next;
    } else {
        g_block_list = block->next;
    }
    
    if (block->next != NULL) {
        block->next->prev = block->prev;
    }
}

static void ReportViolation(const HeapViolationType *violation)
{
    if (violation == NULL) {
        return;
    }
    
    /* Store last violation */
    memcpy(&g_last_violation, violation, sizeof(HeapViolationType));
    
    /* Update heap state */
    g_heap_info.state = HEAP_STATE_CORRUPTED;
    g_heap_info.last_error = violation->violation_type;
    g_heap_info.last_error_addr = violation->violation_addr;
    
    /* Call callback if registered */
    if (g_violation_callback != NULL) {
        g_violation_callback(violation);
    }
}

static void FillGuardZone(uint32_t addr, uint32_t size)
{
    HeapProtection_FillPattern((void *)addr, size, HEAP_FILL_PATTERN_GUARD);
}

static boolean CheckGuardZone(uint32_t addr, uint32_t size)
{
    uint8_t *ptr = (uint8_t *)addr;
    uint32_t i;
    
    for (i = 0; i < size; i++) {
        if (ptr[i] != HEAP_FILL_PATTERN_GUARD) {
            return FALSE;
        }
    }
    
    return TRUE;
}

static HeapBlockHeaderType* FindFreeBlock(uint32_t size)
{
    HeapBlockHeaderType *block = g_free_list;
    
    while (block != NULL) {
        if (block->state == HEAP_BLOCK_STATE_FREE && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    
    return NULL;
}

static boolean IsValidHeapPtr(const void *ptr)
{
    if (ptr == NULL) {
        return FALSE;
    }
    
    /* Check if within heap bounds */
    if ((uint32_t)ptr < g_config.start_addr ||
        (uint32_t)ptr >= (g_config.start_addr + g_config.size)) {
        return FALSE;
    }
    
    return TRUE;
}
