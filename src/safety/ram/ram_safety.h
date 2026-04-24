/******************************************************************************
 * @file    ram_safety.h
 * @brief   RAM Safety Interface - Unified API for RAM Protection
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides a unified interface to:
 * - ECC encoding/decoding
 * - RAM monitoring
 * - Safe memory operations
 * - Memory mapping management
 * - Safety checks and assertions
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef RAM_SAFETY_H
#define RAM_SAFETY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"
#include "ecc_encoder.h"
#include "ecc_checker.h"
#include "ram_monitor.h"
#include "ecc_allocator.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* RAM Safety version */
#define RAM_SAFETY_VERSION_MAJOR    1U
#define RAM_SAFETY_VERSION_MINOR    0U
#define RAM_SAFETY_VERSION_PATCH    0U

/* Safety check modes */
#define RAM_SAFETY_CHECK_NONE       0x00U
#define RAM_SAFETY_CHECK_FAST       0x01U   /* Basic checks only */
#define RAM_SAFETY_CHECK_FULL       0x02U   /* Full ECC verification */
#define RAM_SAFETY_CHECK_PARANOID   0x03U   /* All checks + redundant ops */

/* Memory region types */
#define RAM_REGION_TYPE_GENERIC     0x00U
#define RAM_REGION_TYPE_CODE        0x01U   /* Code/ROM */
#define RAM_REGION_TYPE_DATA        0x02U   /* Data RAM */
#define RAM_REGION_TYPE_STACK       0x03U   /* Stack area */
#define RAM_REGION_TYPE_HEAP        0x04U   /* Heap area */
#define RAM_REGION_TYPE_DMA         0x05U   /* DMA buffer */
#define RAM_REGION_TYPE_CACHE       0x06U   /* Cacheable memory */
#define RAM_REGION_TYPE_DEVICE      0x07U   /* Device memory */

/* Memory operation flags */
#define RAM_OP_FLAG_NONE            0x00U
#define RAM_OP_FLAG_VERIFY_SRC      0x01U   /* Verify source before copy */
#define RAM_OP_FLAG_VERIFY_DST      0x02U   /* Verify destination after copy */
#define RAM_OP_FLAG_USE_ECC         0x04U   /* Use ECC-protected operations */
#define RAM_OP_FLAG_ATOMIC          0x08U   /* Atomic operation */
#define RAM_OP_FLAG_BARRIER         0x10U   /* Memory barrier after op */

/* Safety assertion macros (ASIL-D) */
#ifdef RAM_SAFETY_ENABLE_ASSERTS
    #define RAM_SAFETY_ASSERT(cond) \
        do { \
            if (!(cond)) { \
                RamSafety_HandleAssert(__FILE__, __LINE__, #cond); \
            } \
        } while(0)
#else
    #define RAM_SAFETY_ASSERT(cond) ((void)0)
#endif

#define RAM_SAFETY_ASSERT_PTR(ptr) \
    RAM_SAFETY_ASSERT((ptr) != NULL)

#define RAM_SAFETY_ASSERT_ALIGN(ptr, align) \
    RAM_SAFETY_ASSERT((((uintptr_t)(ptr)) & ((align)-1)) == 0)

#define RAM_SAFETY_ASSERT_RANGE(val, min, max) \
    RAM_SAFETY_ASSERT(((val) >= (min)) && ((val) <= (max)))

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* RAM safety configuration */
typedef struct {
    uint8_t check_mode;             /* Safety check mode */
    boolean enable_monitor;         /* Enable RAM monitoring */
    boolean enable_allocator;       /* Enable ECC allocator */
    boolean enable_timer_checks;    /* Enable timer-based checks */
    uint32_t timer_interval_ms;     /* Timer check interval */
    uint8_t asil_level;             /* Target ASIL level */
    
    /* Sub-module configs */
    EccEncoderConfigType encoder_config;
    EccCheckerConfigType checker_config;
    RamMonitorConfigType monitor_config;
    EccHeapConfigType heap_config;
} RamSafetyConfigType;

/* Memory region descriptor */
typedef struct {
    uint32_t type;                  /* Region type */
    uint32_t start_addr;            /* Start address */
    uint32_t size;                  /* Size in bytes */
    uint32_t attributes;            /* Region attributes */
    const char *name;               /* Region name */
    boolean use_ecc;                /* ECC enabled for region */
    boolean is_monitored;           /* Monitoring enabled */
} RamMemoryRegionType;

/* Safe memory operation context */
typedef struct {
    uint32_t op_id;                 /* Operation identifier */
    uint32_t flags;                 /* Operation flags */
    uint32_t timestamp_start;       /* Start timestamp */
    uint32_t timeout_ms;            /* Timeout in milliseconds */
    uint8_t retry_count;            /* Retry counter */
    boolean completed;              /* Completion flag */
} RamSafeOpContextType;

/* RAM safety status */
typedef struct {
    boolean initialized;
    boolean monitor_active;
    boolean allocator_active;
    uint32_t check_count;
    uint32_t error_count;
    uint32_t last_error_code;
    uint32_t uptime_ms;
} RamSafetyStatusType;

/* Memory map entry */
typedef struct {
    uint32_t virt_addr;             /* Virtual address */
    uint32_t phys_addr;             /* Physical address */
    uint32_t size;                  /* Size */
    uint32_t attributes;            /* Memory attributes */
    boolean is_mapped;              /* Mapping valid */
} RamMemoryMapEntryType;

/* Memory map */
#define RAM_MAX_MEMORY_MAP_ENTRIES  32U
typedef struct {
    uint32_t entry_count;
    RamMemoryMapEntryType entries[RAM_MAX_MEMORY_MAP_ENTRIES];
} RamMemoryMapType;

/* Error callback type */
typedef void (*RamSafetyErrorCallbackType)(
    uint32_t error_code,
    const char *func,
    const char *file,
    uint32_t line
);

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize RAM safety module
 * @param config Pointer to safety configuration
 * @return E_OK on success, error code on failure
 */
Std_ReturnType RamSafety_Init(const RamSafetyConfigType *config);

/**
 * @brief Deinitialize RAM safety module
 * @return E_OK on success
 */
Std_ReturnType RamSafety_DeInit(void);

/**
 * @brief Get initialization status
 * @return TRUE if initialized
 */
boolean RamSafety_IsInitialized(void);

/******************************************************************************
 * Function Prototypes - Memory Operations
 ******************************************************************************/

/**
 * @brief Safe memory copy with ECC verification
 * @param dst Destination pointer
 * @param src Source pointer
 * @param size Number of bytes to copy
 * @param flags Operation flags
 * @return E_OK on success, error code on failure
 */
Std_ReturnType RamSafety_MemCpy(void *dst, const void *src, 
                                 uint32_t size, uint32_t flags);

/**
 * @brief Safe memory set with verification
 * @param dst Destination pointer
 * @param value Value to set
 * @param size Number of bytes
 * @param flags Operation flags
 * @return E_OK on success
 */
Std_ReturnType RamSafety_MemSet(void *dst, uint8_t value, 
                                 uint32_t size, uint32_t flags);

/**
 * @brief Safe memory compare with ECC check
 * @param ptr1 First memory block
 * @param ptr2 Second memory block
 * @param size Number of bytes to compare
 * @param flags Operation flags
 * @return 0 if equal, non-zero if different or error
 */
int32_t RamSafety_MemCmp(const void *ptr1, const void *ptr2, 
                          uint32_t size, uint32_t flags);

/**
 * @brief Safe memory move (handles overlapping regions)
 * @param dst Destination pointer
 * @param src Source pointer
 * @param size Number of bytes
 * @param flags Operation flags
 * @return E_OK on success
 */
Std_ReturnType RamSafety_MemMove(void *dst, const void *src, 
                                  uint32_t size, uint32_t flags);

/**
 * @brief Verify memory region integrity
 * @param ptr Pointer to memory
 * @param size Size in bytes
 * @param flags Verification flags
 * @return E_OK if valid
 */
Std_ReturnType RamSafety_VerifyMem(const void *ptr, uint32_t size, 
                                    uint32_t flags);

/******************************************************************************
 * Function Prototypes - Memory Mapping
 ******************************************************************************/

/**
 * @brief Initialize memory map
 * @return E_OK on success
 */
Std_ReturnType RamSafety_InitMemoryMap(void);

/**
 * @brief Add memory mapping
 * @param virt_addr Virtual address
 * @param phys_addr Physical address
 * @param size Size in bytes
 * @param attributes Mapping attributes
 * @return E_OK on success
 */
Std_ReturnType RamSafety_MapMemory(uint32_t virt_addr, uint32_t phys_addr,
                                    uint32_t size, uint32_t attributes);

/**
 * @brief Remove memory mapping
 * @param virt_addr Virtual address
 * @return E_OK on success
 */
Std_ReturnType RamSafety_UnmapMemory(uint32_t virt_addr);

/**
 * @brief Translate virtual to physical address
 * @param virt_addr Virtual address
 * @param phys_addr Pointer to store physical address
 * @return E_OK if translation successful
 */
Std_ReturnType RamSafety_VirtToPhys(uint32_t virt_addr, uint32_t *phys_addr);

/**
 * @brief Translate physical to virtual address
 * @param phys_addr Physical address
 * @param virt_addr Pointer to store virtual address
 * @return E_OK if translation successful
 */
Std_ReturnType RamSafety_PhysToVirt(uint32_t phys_addr, uint32_t *virt_addr);

/******************************************************************************
 * Function Prototypes - Safety Checks
 ******************************************************************************/

/**
 * @brief Perform safety check on memory region
 * @param region Pointer to region descriptor
 * @param result Pointer to store check result
 * @return E_OK if region is safe
 */
Std_ReturnType RamSafety_CheckRegion(const RamMemoryRegionType *region,
                                      EccCheckResultType *result);

/**
 * @brief Perform full memory check
 * @return Number of errors found
 */
uint32_t RamSafety_FullCheck(void);

/**
 * @brief Quick sanity check
 * @return E_OK if basic checks pass
 */
Std_ReturnType RamSafety_QuickCheck(void);

/**
 * @brief Set safety check mode
 * @param mode Check mode (RAM_SAFETY_CHECK_*)
 * @return E_OK on success
 */
Std_ReturnType RamSafety_SetCheckMode(uint8_t mode);

/******************************************************************************
 * Function Prototypes - Timer/Watchdog
 ******************************************************************************/

/**
 * @brief Initialize timer-based checks
 * @param interval_ms Check interval in milliseconds
 * @return E_OK on success
 */
Std_ReturnType RamSafety_InitTimer(uint32_t interval_ms);

/**
 * @brief Timer callback for periodic checks
 */
void RamSafety_TimerCallback(void);

/**
 * @brief Feed safety watchdog
 */
void RamSafety_FeedWatchdog(void);

/**
 * @brief Check if timer/watchdog is healthy
 * @return TRUE if healthy
 */
boolean RamSafety_IsTimerHealthy(void);

/******************************************************************************
 * Function Prototypes - Error Handling
 ******************************************************************************/

/**
 * @brief Handle assertion failure
 * @param file Source file
 * @param line Line number
 * @param condition Failed condition string
 */
void RamSafety_HandleAssert(const char *file, uint32_t line, 
                             const char *condition);

/**
 * @brief Set error callback
 * @param callback Error callback function
 */
void RamSafety_SetErrorCallback(RamSafetyErrorCallbackType callback);

/**
 * @brief Get last error code
 * @return Last error code
 */
uint32_t RamSafety_GetLastError(void);

/**
 * @brief Get error string
 * @param error_code Error code
 * @return Human-readable error description
 */
const char* RamSafety_GetErrorString(uint32_t error_code);

/******************************************************************************
 * Function Prototypes - Status and Statistics
 ******************************************************************************/

/**
 * @brief Get RAM safety status
 * @return Pointer to status structure
 */
const RamSafetyStatusType* RamSafety_GetStatus(void);

/**
 * @brief Get combined statistics from all submodules
 * @param stats Pointer to store combined statistics
 * @return E_OK on success
 */
Std_ReturnType RamSafety_GetCombinedStats(void *stats);

/**
 * @brief Reset all statistics
 * @return E_OK on success
 */
Std_ReturnType RamSafety_ResetStats(void);

/******************************************************************************
 * Function Prototypes - Convenience Wrappers
 ******************************************************************************/

/**
 * @brief Allocate ECC-protected memory (wrapper)
 * @param size Size in bytes
 * @return Pointer to allocated memory, NULL on failure
 */
static inline void* RamSafety_Malloc(uint32_t size)
{
    return EccAllocator_Malloc(size, ECC_ALLOC_FLAG_ECC | ECC_ALLOC_FLAG_GUARD);
}

/**
 * @brief Free ECC-protected memory (wrapper)
 * @param ptr Pointer to memory
 */
static inline void RamSafety_Free(void *ptr)
{
    (void)EccAllocator_Free(ptr);
}

/**
 * @brief Encode data with ECC (wrapper)
 * @param data 32-bit data
 * @param ecc_code Pointer to store ECC code
 * @return E_OK on success
 */
static inline Std_ReturnType RamSafety_Encode32(uint32_t data, uint8_t *ecc_code)
{
    return EccEncoder_Encode32(data, ecc_code);
}

/**
 * @brief Check and correct data (wrapper)
 * @param data Input data
 * @param ecc_code ECC code
 * @param result Check result
 * @return E_OK if no error or corrected
 */
static inline Std_ReturnType RamSafety_CheckAndCorrect32(
    uint32_t data, uint8_t ecc_code, EccCheckResultType *result)
{
    return EccChecker_CheckAndCorrect32(data, ecc_code, result);
}

/**
 * @brief Register memory region for monitoring (wrapper)
 * @param region Region descriptor
 * @return E_OK on success
 */
static inline Std_ReturnType RamSafety_RegisterRegion(const RamRegionType *region)
{
    return RamMonitor_RegisterRegion(region);
}

/**
 * @brief Get version information
 * @param version Pointer to version info structure
 */
void RamSafety_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* RAM_SAFETY_H */
