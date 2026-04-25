/*
 * cache_manager.c
 * NXP S32G3 L1/L2 Cache Management
 * ARM Cortex-A53 Cache Operations
 */

#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 * S32G3 Cache Architecture (Cortex-A53)
 *============================================================================*/

/* L1 Cache characteristics */
#define L1_CACHE_LINE_SIZE      64          /* 64 bytes per line */
#define L1_CACHE_SETS           256         /* 256 sets */
#define L1_CACHE_WAYS           4           /* 4-way set associative */
#define L1_CACHE_SIZE           (L1_CACHE_LINE_SIZE * L1_CACHE_SETS * L1_CACHE_WAYS)

/* L2 Cache characteristics */
#define L2_CACHE_LINE_SIZE      64
#define L2_CACHE_SIZE           0x80000     /* 512 KB */

/* Cache Type Register values */
#define CTR_DMINLINE_SHIFT      16
#define CTR_IMINLINE_SHIFT      0
#define CTR_ERG_SHIFT           20
#define CTR_CWG_SHIFT           24

/*==============================================================================
 * ARMv8 Cache Maintenance Operations (using inline assembly)
 *============================================================================*/

/* Data Cache Line Invalidate by VA to PoC */
static inline void dc_ivac(uint64_t va)
{
    __asm__ volatile ("dc ivac, %0" :: "r"(va) : "memory");
}

/* Data Cache Line Clean by VA to PoC */
static inline void dc_cvac(uint64_t va)
{
    __asm__ volatile ("dc cvac, %0" :: "r"(va) : "memory");
}

/* Data Cache Line Clean and Invalidate by VA to PoC */
static inline void dc_civac(uint64_t va)
{
    __asm__ volatile ("dc civac, %0" :: "r"(va) : "memory");
}

/* Data Synchronization Barrier */
static inline void dsb(void)
{
    __asm__ volatile ("dsb sy" ::: "memory");
}

/* Instruction Synchronization Barrier */
static inline void isb(void)
{
    __asm__ volatile ("isb" ::: "memory");
}

/* Data Memory Barrier */
static inline void dmb(void)
{
    __asm__ volatile ("dmb sy" ::: "memory");
}

/* Instruction Cache Invalidate All */
static inline void ic_iallu(void)
{
    __asm__ volatile ("ic iallu" ::: "memory");
}

/* Instruction Cache Line Invalidate by VA */
static inline void ic_ivau(uint64_t va)
{
    __asm__ volatile ("ic ivau, %0" :: "r"(va) : "memory");
}

/* Data Cache Zero by VA */
static inline void dc_zva(uint64_t va)
{
    __asm__ volatile ("dc zva, %0" :: "r"(va) : "memory");
}

/* Read Cache Type Register */
static inline uint64_t read_ctr(void)
{
    uint64_t ctr;
    __asm__ volatile ("mrs %0, ctr_el0" : "=r"(ctr));
    return ctr;
}

/* Read Cache Level ID Register */
static inline uint64_t read_clidr(void)
{
    uint64_t clidr;
    __asm__ volatile ("mrs %0, clidr_el1" : "=r"(clidr));
    return clidr;
}

/* Read Cache Size Selection Register */
static inline void write_csselr(uint64_t csselr)
{
    __asm__ volatile ("msr csselr_el1, %0" :: "r"(csselr));
    isb();
}

/* Read Current Cache Size ID Register */
static inline uint64_t read_ccsidr(void)
{
    uint64_t ccsidr;
    __asm__ volatile ("mrs %0, ccsidr_el1" : "=r"(ccsidr));
    return ccsidr;
}

/*==============================================================================
 * Public Cache Management API
 *============================================================================*/

/*
 * cache_invalidate_dcache_range
 * Invalidate data cache lines for memory range
 * Used before DMA read operations (memory <- device)
 */
void cache_invalidate_dcache_range(uintptr_t addr, size_t size)
{
    uint64_t end;
    uint64_t va;
    
    if (size == 0) {
        return;
    }
    
    /* Align to cache line boundary */
    va = (uint64_t)(addr & ~(L1_CACHE_LINE_SIZE - 1));
    end = (uint64_t)(addr + size);
    
    /* Invalidate each cache line */
    while (va < end) {
        dc_ivac(va);
        va += L1_CACHE_LINE_SIZE;
    }
    
    /* Ensure completion */
    dsb();
}

/*
 * cache_clean_dcache_range
 * Clean (write back) data cache lines for memory range
 * Used after CPU writes before DMA read
 */
void cache_clean_dcache_range(uintptr_t addr, size_t size)
{
    uint64_t end;
    uint64_t va;
    
    if (size == 0) {
        return;
    }
    
    /* Align to cache line boundary */
    va = (uint64_t)(addr & ~(L1_CACHE_LINE_SIZE - 1));
    end = (uint64_t)(addr + size);
    
    /* Clean each cache line */
    while (va < end) {
        dc_cvac(va);
        va += L1_CACHE_LINE_SIZE;
    }
    
    /* Ensure completion */
    dsb();
}

/*
 * cache_clean_invalidate_dcache_range
 * Clean and invalidate data cache lines for memory range
 * Used for cache coherency operations
 */
void cache_clean_invalidate_dcache_range(uintptr_t addr, size_t size)
{
    uint64_t end;
    uint64_t va;
    
    if (size == 0) {
        return;
    }
    
    /* Align to cache line boundary */
    va = (uint64_t)(addr & ~(L1_CACHE_LINE_SIZE - 1));
    end = (uint64_t)(addr + size);
    
    /* Clean and invalidate each cache line */
    while (va < end) {
        dc_civac(va);
        va += L1_CACHE_LINE_SIZE;
    }
    
    /* Ensure completion */
    dsb();
}

/*
 * cache_zero_dcache_range
 * Zero and invalidate data cache lines (ARMv8 feature)
 * Efficient zeroing of memory with cache allocation
 */
void cache_zero_dcache_range(uintptr_t addr, size_t size)
{
    uint64_t end;
    uint64_t va;
    
    if (size == 0) {
        return;
    }
    
    /* Align to cache line boundary */
    va = (uint64_t)(addr & ~(L1_CACHE_LINE_SIZE - 1));
    end = (uint64_t)(addr + size);
    
    /* Zero each cache line */
    while (va < end) {
        dc_zva(va);
        va += L1_CACHE_LINE_SIZE;
    }
    
    /* Ensure completion */
    dsb();
}

/*
 * cache_invalidate_icache_range
 * Invalidate instruction cache lines for memory range
 * Used after code modifications
 */
void cache_invalidate_icache_range(uintptr_t addr, size_t size)
{
    uint64_t end;
    uint64_t va;
    
    if (size == 0) {
        return;
    }
    
    /* Align to cache line boundary */
    va = (uint64_t)(addr & ~(L1_CACHE_LINE_SIZE - 1));
    end = (uint64_t)(addr + size);
    
    /* Invalidate each cache line */
    while (va < end) {
        ic_ivau(va);
        va += L1_CACHE_LINE_SIZE;
    }
    
    /* Ensure completion and synchronize instructions */
    dsb();
    isb();
}

/*
 * cache_invalidate_icache_all
 * Invalidate entire instruction cache
 */
void cache_invalidate_icache_all(void)
{
    ic_iallu();
    dsb();
    isb();
}

/*
 * cache_sync_before_dma_tx
 * Prepare memory for DMA transmit (CPU -> Device)
 * Clean cache to ensure DMA sees latest data
 */
void cache_sync_before_dma_tx(uintptr_t addr, size_t size)
{
    /* Clean D-cache to ensure DMA sees data written by CPU */
    cache_clean_dcache_range(addr, size);
}

/*
 * cache_sync_after_dma_rx
 * Prepare memory after DMA receive (Device -> CPU)
 * Invalidate cache to prevent stale data reads
 */
void cache_sync_after_dma_rx(uintptr_t addr, size_t size)
{
    /* Invalidate D-cache to prevent reading stale cached data */
    cache_invalidate_dcache_range(addr, size);
}

/*
 * cache_sync_before_dma_rx
 * Prepare memory before DMA receive (Device -> CPU)
 * Clean and invalidate to handle buffer reuse
 */
void cache_sync_before_dma_rx(uintptr_t addr, size_t size)
{
    /* Clean and invalidate for buffer reuse scenarios */
    cache_clean_invalidate_dcache_range(addr, size);
}

/*
 * cache_sync_after_dma_tx
 * Called after DMA transmit completes
 * May be needed for cache coherency in some scenarios
 */
void cache_sync_after_dma_tx(uintptr_t addr, size_t size)
{
    /* After TX, cache may be stale but will be overwritten before next use */
    (void)addr;
    (void)size;
    /* No operation needed in most cases */
}

/*
 * cache_enable
 * Enable caches (called during startup)
 * Note: On S32G3/A53, caches are typically enabled by boot code
 */
void cache_enable(void)
{
    uint64_t sctlr;
    
    /* Read System Control Register */
    __asm__ volatile ("mrs %0, sctlr_el1" : "=r"(sctlr));
    
    /* Enable I-cache and D-cache bits */
    sctlr |= (1UL << 12);   /* I-bit: Instruction cache */
    sctlr |= (1UL << 2);    /* C-bit: Data cache */
    
    /* Write back */
    __asm__ volatile ("msr sctlr_el1, %0" :: "r"(sctlr));
    isb();
}

/*
 * cache_disable
 * Disable caches (typically for debugging only)
 */
void cache_disable(void)
{
    uint64_t sctlr;
    
    /* Clean and invalidate D-cache before disabling */
    /* Note: Full D-cache clean would require iterating all sets/ways */
    
    /* Read System Control Register */
    __asm__ volatile ("mrs %0, sctlr_el1" : "=r"(sctlr));
    
    /* Disable I-cache and D-cache bits */
    sctlr &= ~(1UL << 12);  /* Clear I-bit */
    sctlr &= ~(1UL << 2);   /* Clear C-bit */
    
    /* Write back */
    __asm__ volatile ("msr sctlr_el1, %0" :: "r"(sctlr));
    isb();
}

/*
 * cache_get_line_size
 * Get cache line size
 */
size_t cache_get_line_size(void)
{
    return L1_CACHE_LINE_SIZE;
}

/*
 * cache_get_info
 * Get cache information
 */
typedef struct {
    size_t l1_dcache_size;
    size_t l1_icache_size;
    size_t l2_cache_size;
    size_t line_size;
    uint32_t l1_ways;
    uint32_t l1_sets;
} cache_info_t;

void cache_get_info(cache_info_t *info)
{
    if (!info) return;
    
    info->l1_dcache_size = L1_CACHE_SIZE;
    info->l1_icache_size = L1_CACHE_SIZE;
    info->l2_cache_size = L2_CACHE_SIZE;
    info->line_size = L1_CACHE_LINE_SIZE;
    info->l1_ways = L1_CACHE_WAYS;
    info->l1_sets = L1_CACHE_SETS;
}

/*
 * cache_lockdown_range
 * Lock cache lines in memory range (prevent eviction)
 * Platform-specific implementation
 */
int cache_lockdown_range(uintptr_t addr, size_t size)
{
    /* S32G3/Cortex-A53 cache lockdown is complex and typically
     * not used in automotive applications.
     * Would require L2 cache controller programming.
     */
    (void)addr;
    (void)size;
    return -1;  /* Not implemented */
}

/*
 * cache_unlockdown_all
 * Release all cache lockdown
 */
void cache_unlockdown_all(void)
{
    /* Not implemented - see cache_lockdown_range */
}
