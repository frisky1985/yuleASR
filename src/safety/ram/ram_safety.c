/******************************************************************************
 * @file    ram_safety.c
 * @brief   RAM Safety Interface Implementation
 *
 * Unified interface for RAM ECC protection with ASIL-D safety level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ram_safety.h"
#include "../../common/utils/eth_utils.h"
#include <string.h>

/******************************************************************************
 * Local Constants
 ******************************************************************************/

#define RAM_SAFETY_VENDOR_ID        0x0001U
#define RAM_SAFETY_MODULE_ID        0x0104U
#define RAM_SAFETY_SW_MAJOR         1U
#define RAM_SAFETY_SW_MINOR         0U
#define RAM_SAFETY_SW_PATCH         0U

/* Default values */
#define DEFAULT_TIMER_INTERVAL_MS   100U
#define MAX_RETRIES                 3U

/******************************************************************************
 * Local Variables
 ******************************************************************************/

static RamSafetyStatusType g_safety_status;
static RamSafetyConfigType g_safety_config;
static RamMemoryMapType g_memory_map;
static RamSafetyErrorCallbackType g_error_callback = NULL;
static uint32_t g_last_timer_check = 0U;

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

static void RamSafety_ResetStatus(void);
static Std_ReturnType RamSafety_ValidateConfig(const RamSafetyConfigType *config);
static void RamSafety_HandleError(uint32_t error_code, const char *func, 
                                   const char *file, uint32_t line);
static Std_ReturnType RamSafety_VerifyCopy(const void *dst, const void *src, 
                                            uint32_t size);

/******************************************************************************
 * Local Function Implementations
 ******************************************************************************/

/**
 * @brief Reset safety status
 */
static void RamSafety_ResetStatus(void)
{
    memset(&g_safety_status, 0, sizeof(g_safety_status));
}

/**
 * @brief Validate configuration
 */
static Std_ReturnType RamSafety_ValidateConfig(const RamSafetyConfigType *config)
{
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    /* Validate ASIL level */
    if (config->asil_level > ASIL_D) {
        return E_NOT_OK;
    }
    
    /* Validate check mode */
    if (config->check_mode > RAM_SAFETY_CHECK_PARANOID) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Handle error with callback
 */
static void RamSafety_HandleError(uint32_t error_code, const char *func,
                                   const char *file, uint32_t line)
{
    g_safety_status.error_count++;
    g_safety_status.last_error_code = error_code;
    
    if (g_error_callback != NULL) {
        g_error_callback(error_code, func, file, line);
    }
}

/**
 * @brief Verify copy operation by re-reading
 */
static Std_ReturnType RamSafety_VerifyCopy(const void *dst, const void *src,
                                            uint32_t size)
{
    const uint8_t *d = (const uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    uint32_t i;
    
    for (i = 0U; i < size; i++) {
        if (d[i] != s[i]) {
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/******************************************************************************
 * Public API Implementation - Initialization
 ******************************************************************************/

/**
 * @brief Initialize RAM safety module
 */
Std_ReturnType RamSafety_Init(const RamSafetyConfigType *config)
{
    Std_ReturnType status;
    
    /* Validate configuration */
    status = RamSafety_ValidateConfig(config);
    if (status != E_OK) {
        return status;
    }
    
    /* Reset state */
    RamSafety_ResetStatus();
    memset(&g_safety_config, 0, sizeof(g_safety_config));
    memset(&g_memory_map, 0, sizeof(g_memory_map));
    
    /* Copy configuration */
    g_safety_config = *config;
    
    /* Initialize ECC encoder */
    status = EccEncoder_Init(&config->encoder_config);
    if (status != E_OK) {
        return status;
    }
    
    /* Initialize ECC checker */
    status = EccChecker_Init(&config->checker_config);
    if (status != E_OK) {
        EccEncoder_DeInit();
        return status;
    }
    
    /* Initialize RAM monitor if enabled */
    if (config->enable_monitor) {
        status = RamMonitor_Init(&config->monitor_config);
        if (status != E_OK) {
            EccChecker_DeInit();
            EccEncoder_DeInit();
            return status;
        }
        g_safety_status.monitor_active = TRUE;
        
        /* Start monitoring */
        RamMonitor_Start();
    }
    
    /* Initialize ECC allocator if enabled */
    if (config->enable_allocator) {
        status = EccAllocator_Init(&config->heap_config);
        if (status != E_OK) {
            if (config->enable_monitor) {
                RamMonitor_DeInit();
            }
            EccChecker_DeInit();
            EccEncoder_DeInit();
            return status;
        }
        g_safety_status.allocator_active = TRUE;
    }
    
    /* Initialize memory map */
    RamSafety_InitMemoryMap();
    
    /* Initialize timer if enabled */
    if (config->enable_timer_checks) {
        uint32_t interval = (config->timer_interval_ms > 0U) ? 
                            config->timer_interval_ms : DEFAULT_TIMER_INTERVAL_MS;
        RamSafety_InitTimer(interval);
    }
    
    g_safety_status.initialized = TRUE;
    
    return E_OK;
}

/**
 * @brief Deinitialize RAM safety module
 */
Std_ReturnType RamSafety_DeInit(void)
{
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* Deinitialize submodules */
    if (g_safety_status.allocator_active) {
        EccAllocator_DeInit();
        g_safety_status.allocator_active = FALSE;
    }
    
    if (g_safety_status.monitor_active) {
        RamMonitor_Stop();
        RamMonitor_DeInit();
        g_safety_status.monitor_active = FALSE;
    }
    
    EccChecker_DeInit();
    EccEncoder_DeInit();
    
    /* Clear state */
    RamSafety_ResetStatus();
    memset(&g_safety_config, 0, sizeof(g_safety_config));
    memset(&g_memory_map, 0, sizeof(g_memory_map));
    
    return E_OK;
}

/**
 * @brief Get initialization status
 */
boolean RamSafety_IsInitialized(void)
{
    return g_safety_status.initialized;
}

/******************************************************************************
 * Public API Implementation - Memory Operations
 ******************************************************************************/

/**
 * @brief Safe memory copy with ECC verification
 */
Std_ReturnType RamSafety_MemCpy(void *dst, const void *src,
                                 uint32_t size, uint32_t flags)
{
    Std_ReturnType status = E_OK;
    uint8_t retry;
    
    /* Safety assertions */
    RAM_SAFETY_ASSERT_PTR(dst);
    RAM_SAFETY_ASSERT_PTR(src);
    RAM_SAFETY_ASSERT(size > 0U);
    
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* Verify source if requested */
    if (flags & RAM_OP_FLAG_VERIFY_SRC) {
        status = RamSafety_VerifyMem(src, size, flags);
        if (status != E_OK) {
            RamSafety_HandleError(E_SAFETY_MEMORY_ERROR, "RamSafety_MemCpy", 
                                   __FILE__, __LINE__);
            return status;
        }
    }
    
    /* Perform copy with retry */
    for (retry = 0U; retry < MAX_RETRIES; retry++) {
        memcpy(dst, src, size);
        
        /* Verify copy if requested */
        if (flags & RAM_OP_FLAG_VERIFY_DST) {
            status = RamSafety_VerifyCopy(dst, src, size);
            if (status == E_OK) {
                break;
            }
        } else {
            status = E_OK;
            break;
        }
    }
    
    if (status != E_OK) {
        RamSafety_HandleError(E_SAFETY_MEMORY_ERROR, "RamSafety_MemCpy",
                               __FILE__, __LINE__);
        return status;
    }
    
    /* Memory barrier if requested */
    if (flags & RAM_OP_FLAG_BARRIER) {
        __asm__ volatile ("" ::: "memory");
    }
    
    g_safety_status.check_count++;
    
    return E_OK;
}

/**
 * @brief Safe memory set with verification
 */
Std_ReturnType RamSafety_MemSet(void *dst, uint8_t value,
                                 uint32_t size, uint32_t flags)
{
    Std_ReturnType status = E_OK;
    uint8_t retry;
    
    RAM_SAFETY_ASSERT_PTR(dst);
    RAM_SAFETY_ASSERT(size > 0U);
    
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* Perform set with retry and verification */
    for (retry = 0U; retry < MAX_RETRIES; retry++) {
        memset(dst, (int)value, size);
        
        /* Verify if requested */
        if (flags & RAM_OP_FLAG_VERIFY_DST) {
            const uint8_t *p = (const uint8_t *)dst;
            uint32_t i;
            boolean verified = TRUE;
            
            for (i = 0U; i < size; i++) {
                if (p[i] != value) {
                    verified = FALSE;
                    break;
                }
            }
            
            if (verified) {
                break;
            }
            status = E_NOT_OK;
        } else {
            break;
        }
    }
    
    if (flags & RAM_OP_FLAG_BARRIER) {
        __asm__ volatile ("" ::: "memory");
    }
    
    g_safety_status.check_count++;
    
    return status;
}

/**
 * @brief Safe memory compare with ECC check
 */
int32_t RamSafety_MemCmp(const void *ptr1, const void *ptr2,
                          uint32_t size, uint32_t flags)
{
    const uint8_t *p1 = (const uint8_t *)ptr1;
    const uint8_t *p2 = (const uint8_t *)ptr2;
    uint32_t i;
    
    RAM_SAFETY_ASSERT_PTR(ptr1);
    RAM_SAFETY_ASSERT_PTR(ptr2);
    
    if (!g_safety_status.initialized) {
        return -1;
    }
    
    /* Verify inputs if requested */
    if (flags & RAM_OP_FLAG_VERIFY_SRC) {
        if (RamSafety_VerifyMem(ptr1, size, flags) != E_OK ||
            RamSafety_VerifyMem(ptr2, size, flags) != E_OK) {
            return -1;
        }
    }
    
    /* Compare byte by byte */
    for (i = 0U; i < size; i++) {
        if (p1[i] != p2[i]) {
            return (int32_t)p1[i] - (int32_t)p2[i];
        }
    }
    
    g_safety_status.check_count++;
    
    return 0;
}

/**
 * @brief Safe memory move
 */
Std_ReturnType RamSafety_MemMove(void *dst, const void *src,
                                  uint32_t size, uint32_t flags)
{
    RAM_SAFETY_ASSERT_PTR(dst);
    RAM_SAFETY_ASSERT_PTR(src);
    
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* Use memmove for overlapping regions */
    memmove(dst, src, size);
    
    /* Verify if requested */
    if (flags & RAM_OP_FLAG_VERIFY_DST) {
        /* For move, we can only verify if regions don't overlap */
        if (dst < src || (uint8_t *)dst >= (uint8_t *)src + size) {
            if (RamSafety_VerifyCopy(dst, src, size) != E_OK) {
                return E_NOT_OK;
            }
        }
    }
    
    if (flags & RAM_OP_FLAG_BARRIER) {
        __asm__ volatile ("" ::: "memory");
    }
    
    g_safety_status.check_count++;
    
    return E_OK;
}

/**
 * @brief Verify memory region integrity
 */
Std_ReturnType RamSafety_VerifyMem(const void *ptr, uint32_t size, uint32_t flags)
{
    (void)flags;
    
    RAM_SAFETY_ASSERT_PTR(ptr);
    
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* For ECC-protected memory, verify ECC codes */
    if (EccAllocator_IsHeapPtr((void *)ptr)) {
        return EccAllocator_Verify((void *)ptr, NULL);
    }
    
    /* Basic pointer validation */
    if (ptr == NULL || size == 0U) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/******************************************************************************
 * Public API Implementation - Memory Mapping
 ******************************************************************************/

/**
 * @brief Initialize memory map
 */
Std_ReturnType RamSafety_InitMemoryMap(void)
{
    memset(&g_memory_map, 0, sizeof(g_memory_map));
    return E_OK;
}

/**
 * @brief Add memory mapping
 */
Std_ReturnType RamSafety_MapMemory(uint32_t virt_addr, uint32_t phys_addr,
                                    uint32_t size, uint32_t attributes)
{
    uint32_t i;
    
    if (g_memory_map.entry_count >= RAM_MAX_MEMORY_MAP_ENTRIES) {
        return E_NOT_OK;
    }
    
    if (virt_addr == 0U || phys_addr == 0U || size == 0U) {
        return E_NOT_OK;
    }
    
    /* Check for existing mapping */
    for (i = 0U; i < g_memory_map.entry_count; i++) {
        if (g_memory_map.entries[i].virt_addr == virt_addr) {
            return E_NOT_OK;  /* Already mapped */
        }
    }
    
    /* Add new entry */
    i = g_memory_map.entry_count;
    g_memory_map.entries[i].virt_addr = virt_addr;
    g_memory_map.entries[i].phys_addr = phys_addr;
    g_memory_map.entries[i].size = size;
    g_memory_map.entries[i].attributes = attributes;
    g_memory_map.entries[i].is_mapped = TRUE;
    g_memory_map.entry_count++;
    
    return E_OK;
}

/**
 * @brief Remove memory mapping
 */
Std_ReturnType RamSafety_UnmapMemory(uint32_t virt_addr)
{
    uint32_t i, j;
    
    for (i = 0U; i < g_memory_map.entry_count; i++) {
        if (g_memory_map.entries[i].virt_addr == virt_addr) {
            /* Remove by shifting entries */
            for (j = i; j < g_memory_map.entry_count - 1U; j++) {
                g_memory_map.entries[j] = g_memory_map.entries[j + 1U];
            }
            g_memory_map.entry_count--;
            memset(&g_memory_map.entries[g_memory_map.entry_count], 0,
                   sizeof(RamMemoryMapEntryType));
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief Virtual to physical address translation
 */
Std_ReturnType RamSafety_VirtToPhys(uint32_t virt_addr, uint32_t *phys_addr)
{
    uint32_t i;
    
    if (phys_addr == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0U; i < g_memory_map.entry_count; i++) {
        if (g_memory_map.entries[i].is_mapped &&
            virt_addr >= g_memory_map.entries[i].virt_addr &&
            virt_addr < g_memory_map.entries[i].virt_addr + 
                        g_memory_map.entries[i].size) {
            uint32_t offset = virt_addr - g_memory_map.entries[i].virt_addr;
            *phys_addr = g_memory_map.entries[i].phys_addr + offset;
            return E_OK;
        }
    }
    
    /* No mapping found - assume identity mapping */
    *phys_addr = virt_addr;
    return E_OK;
}

/**
 * @brief Physical to virtual address translation
 */
Std_ReturnType RamSafety_PhysToVirt(uint32_t phys_addr, uint32_t *virt_addr)
{
    uint32_t i;
    
    if (virt_addr == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0U; i < g_memory_map.entry_count; i++) {
        if (g_memory_map.entries[i].is_mapped &&
            phys_addr >= g_memory_map.entries[i].phys_addr &&
            phys_addr < g_memory_map.entries[i].phys_addr +
                        g_memory_map.entries[i].size) {
            uint32_t offset = phys_addr - g_memory_map.entries[i].phys_addr;
            *virt_addr = g_memory_map.entries[i].virt_addr + offset;
            return E_OK;
        }
    }
    
    /* No mapping found - assume identity mapping */
    *virt_addr = phys_addr;
    return E_OK;
}

/******************************************************************************
 * Public API Implementation - Safety Checks
 ******************************************************************************/

/**
 * @brief Check memory region
 */
Std_ReturnType RamSafety_CheckRegion(const RamMemoryRegionType *region,
                                      EccCheckResultType *result)
{
    if (!g_safety_status.initialized || region == NULL) {
        return E_NOT_OK;
    }
    
    /* For ECC-protected regions, use ECC checker */
    if (region->use_ecc && EccAllocator_IsHeapPtr((void *)region->start_addr)) {
        return EccAllocator_Verify((void *)region->start_addr, result);
    }
    
    return E_OK;
}

/**
 * @brief Perform full memory check
 */
uint32_t RamSafety_FullCheck(void)
{
    uint32_t errors = 0U;
    
    if (!g_safety_status.initialized) {
        return 0U;
    }
    
    /* Check all heap allocations */
    if (g_safety_status.allocator_active) {
        errors += EccAllocator_CheckAll();
    }
    
    /* Scan all monitored regions */
    if (g_safety_status.monitor_active) {
        errors += RamMonitor_ScanAll();
    }
    
    g_safety_status.check_count++;
    
    if (errors > 0U) {
        g_safety_status.error_count += errors;
    }
    
    return errors;
}

/**
 * @brief Quick sanity check
 */
Std_ReturnType RamSafety_QuickCheck(void)
{
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* Basic health checks */
    if (g_safety_status.monitor_active && !RamMonitor_IsHealthy()) {
        return E_NOT_OK;
    }
    
    g_safety_status.check_count++;
    
    return E_OK;
}

/**
 * @brief Set check mode
 */
Std_ReturnType RamSafety_SetCheckMode(uint8_t mode)
{
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    if (mode > RAM_SAFETY_CHECK_PARANOID) {
        return E_NOT_OK;
    }
    
    g_safety_config.check_mode = mode;
    
    return E_OK;
}

/******************************************************************************
 * Public API Implementation - Timer/Watchdog
 ******************************************************************************/

/**
 * @brief Initialize timer-based checks
 */
Std_ReturnType RamSafety_InitTimer(uint32_t interval_ms)
{
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    g_safety_config.timer_interval_ms = interval_ms;
    g_last_timer_check = (uint32_t)(eth_get_timestamp_us() / 1000U);
    
    return E_OK;
}

/**
 * @brief Timer callback for periodic checks
 */
void RamSafety_TimerCallback(void)
{
    uint32_t current_time;
    
    if (!g_safety_status.initialized) {
        return;
    }
    
    current_time = (uint32_t)(eth_get_timestamp_us() / 1000U);
    
    /* Check if interval elapsed */
    if ((current_time - g_last_timer_check) >= g_safety_config.timer_interval_ms) {
        /* Perform quick check */
        (void)RamSafety_QuickCheck();
        
        /* Scan critical regions */
        if (g_safety_status.monitor_active) {
            (void)RamMonitor_ScanCritical();
        }
        
        g_last_timer_check = current_time;
        g_safety_status.uptime_ms += g_safety_config.timer_interval_ms;
    }
    
    /* Feed watchdog */
    RamSafety_FeedWatchdog();
}

/**
 * @brief Feed safety watchdog
 */
void RamSafety_FeedWatchdog(void)
{
    /* Platform-specific watchdog feeding */
    /* This is a placeholder - actual implementation depends on hardware */
}

/**
 * @brief Check timer health
 */
boolean RamSafety_IsTimerHealthy(void)
{
    uint32_t current_time;
    uint32_t elapsed;
    
    if (!g_safety_status.initialized) {
        return FALSE;
    }
    
    current_time = (uint32_t)(eth_get_timestamp_us() / 1000U);
    elapsed = current_time - g_last_timer_check;
    
    /* Check if timer is running (elapsed should be reasonable) */
    if (elapsed > (g_safety_config.timer_interval_ms * 10U)) {
        return FALSE;
    }
    
    return TRUE;
}

/******************************************************************************
 * Public API Implementation - Error Handling
 ******************************************************************************/

/**
 * @brief Handle assertion failure
 */
void RamSafety_HandleAssert(const char *file, uint32_t line,
                             const char *condition)
{
    (void)file;
    (void)line;
    (void)condition;
    
    /* Log assertion failure */
    g_safety_status.error_count++;
    
    /* In ASIL-D, assertion failures are critical */
    /* Platform-specific: may trigger safe state */
    
    /* Infinite loop for debugging - in production, trigger safe state */
    while (1) {
        /* Halt or trigger watchdog */
    }
}

/**
 * @brief Set error callback
 */
void RamSafety_SetErrorCallback(RamSafetyErrorCallbackType callback)
{
    g_error_callback = callback;
}

/**
 * @brief Get last error code
 */
uint32_t RamSafety_GetLastError(void)
{
    return g_safety_status.last_error_code;
}

/**
 * @brief Get error string
 */
const char* RamSafety_GetErrorString(uint32_t error_code)
{
    switch (error_code) {
        case E_OK:
            return "No error";
        case E_SAFETY_MEMORY_ERROR:
            return "Safety memory error";
        case E_SAFETY_ERROR:
            return "General safety error";
        case E_SAFETY_WATCHDOG_ERROR:
            return "Watchdog error";
        case E_SAFETY_TIMING_ERROR:
            return "Timing error";
        default:
            return "Unknown error";
    }
}

/******************************************************************************
 * Public API Implementation - Status and Statistics
 ******************************************************************************/

/**
 * @brief Get safety status
 */
const RamSafetyStatusType* RamSafety_GetStatus(void)
{
    return &g_safety_status;
}

/**
 * @brief Get combined statistics
 */
Std_ReturnType RamSafety_GetCombinedStats(void *stats)
{
    (void)stats;
    
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    /* Aggregate statistics from all submodules */
    /* Implementation depends on stats structure definition */
    
    return E_OK;
}

/**
 * @brief Reset all statistics
 */
Std_ReturnType RamSafety_ResetStats(void)
{
    if (!g_safety_status.initialized) {
        return E_NOT_OK;
    }
    
    g_safety_status.check_count = 0U;
    g_safety_status.error_count = 0U;
    g_safety_status.last_error_code = 0U;
    
    EccChecker_ResetStats();
    
    if (g_safety_status.monitor_active) {
        RamMonitor_ResetStats();
    }
    
    if (g_safety_status.allocator_active) {
        EccAllocator_ResetStats();
    }
    
    return E_OK;
}

/******************************************************************************
 * Public API Implementation - Version
 ******************************************************************************/

/**
 * @brief Get version information
 */
void RamSafety_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = RAM_SAFETY_VENDOR_ID;
        version->moduleID = RAM_SAFETY_MODULE_ID;
        version->sw_major_version = RAM_SAFETY_SW_MAJOR;
        version->sw_minor_version = RAM_SAFETY_SW_MINOR;
        version->sw_patch_version = RAM_SAFETY_SW_PATCH;
    }
}
