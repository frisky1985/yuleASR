/******************************************************************************
 * @file    ram_monitor.h
 * @brief   RAM Memory Monitor - Background Scanning and Error Detection
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * Features:
 * - Background RAM scanning task
 * - Configurable monitoring regions
 * - Periodic integrity checks
 * - Error counting and reporting
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef RAM_MONITOR_H
#define RAM_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"
#include "ecc_checker.h"
#include <stddef.h>

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Maximum number of monitored regions */
#define RAM_MONITOR_MAX_REGIONS     16U

/* Monitoring modes */
#define RAM_MONITOR_MODE_OFF        0x00U
#define RAM_MONITOR_MODE_PASSIVE    0x01U   /* Read-only monitoring */
#define RAM_MONITOR_MODE_ACTIVE     0x02U   /* Read with ECC check */
#define RAM_MONITOR_MODE_STRESS     0x03U   /* Write-read-verify test */

/* Default scan intervals (milliseconds) */
#define RAM_MONITOR_INTERVAL_FAST   10U     /* 10ms - critical regions */
#define RAM_MONITOR_INTERVAL_NORMAL 100U    /* 100ms - normal regions */
#define RAM_MONITOR_INTERVAL_SLOW   1000U   /* 1s - non-critical regions */

/* Monitor status flags */
#define RAM_MONITOR_STATUS_OK           0x00U
#define RAM_MONITOR_STATUS_SCANNING     0x01U
#define RAM_MONITOR_STATUS_ERROR        0x02U
#define RAM_MONITOR_STATUS_PAUSED       0x04U
#define RAM_MONITOR_STATUS_OVERFLOW     0x08U

/* Region attributes */
#define RAM_REGION_ATTR_CRITICAL        0x01U   /* ASIL-D critical */
#define RAM_REGION_ATTR_SAFEGUARD       0x02U   /* Protected by ECC */
#define RAM_REGION_ATTR_READ_ONLY       0x04U   /* ROM-like region */
#define RAM_REGION_ATTR_STACK           0x08U   /* Stack area */
#define RAM_REGION_ATTR_HEAP            0x10U   /* Heap area */

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Forward declarations */
struct RamRegionType;
struct RamMonitorType;

/* Monitor callback function type */
typedef void (*RamMonitorCallbackType)(
    const struct RamRegionType *region,
    const EccCheckResultType *error_result,
    void *user_data
);

/* Region scan progress callback */
typedef void (*RamScanProgressType)(
    uint32_t region_id,
    uint32_t progress_percent,
    void *user_data
);

/* Memory region definition */
typedef struct RamRegionType {
    uint32_t region_id;                 /* Unique region identifier */
    uint32_t attributes;                /* Region attributes (RAM_REGION_ATTR_*) */
    uint32_t start_address;             /* Start address (physical or virtual) */
    uint32_t size_bytes;                /* Region size in bytes */
    uint32_t word_size;                 /* Word size (4 or 8 bytes) */
    uint8_t *ecc_storage;               /* Pointer to ECC storage (NULL if none) */
    uint32_t scan_interval_ms;          /* Scan interval in milliseconds */
    uint32_t priority;                  /* Scan priority (higher = more urgent) */
    boolean enabled;                    /* Region enabled flag */
    
    /* Statistics */
    uint32_t scan_count;                /* Number of scans performed */
    uint32_t error_count;               /* Errors detected in this region */
    uint32_t last_scan_time;            /* Timestamp of last scan */
    uint32_t scan_duration_us;          /* Last scan duration in microseconds */
} RamRegionType;

/* Monitor configuration */
typedef struct {
    uint32_t max_regions;               /* Maximum regions to monitor */
    uint32_t default_scan_interval;     /* Default scan interval */
    boolean auto_start;                 /* Auto-start on init */
    boolean enable_interrupts;          /* Use interrupt-driven scanning */
    uint32_t task_stack_size;           /* Monitor task stack size */
    uint32_t task_priority;             /* Monitor task priority */
    RamMonitorCallbackType error_callback;  /* Error callback function */
    RamScanProgressType progress_callback;  /* Progress callback */
    void *callback_user_data;           /* User data for callbacks */
} RamMonitorConfigType;

/* Monitor statistics */
typedef struct {
    uint32_t total_regions;             /* Total registered regions */
    uint32_t active_regions;            /* Currently enabled regions */
    uint32_t total_scans;               /* Total scan cycles completed */
    uint32_t total_errors;              /* Total errors detected */
    uint32_t single_bit_errors;         /* Single-bit errors (corrected) */
    uint32_t uncorrectable_errors;      /* Uncorrectable errors */
    uint32_t scan_overruns;             /* Scan timing overruns */
    uint32_t uptime_seconds;            /* Monitor uptime in seconds */
    uint32_t last_error_region;         /* Last region with error */
    uint32_t last_error_time;           /* Timestamp of last error */
} RamMonitorStatsType;

/* Monitor state */
typedef struct RamMonitorType {
    boolean initialized;
    boolean running;
    uint32_t status;
    RamMonitorConfigType config;
    RamMonitorStatsType stats;
    RamRegionType regions[RAM_MONITOR_MAX_REGIONS];
    uint32_t current_region;            /* Currently scanning region */
    uint32_t scan_start_time;           /* Current scan start time */
} RamMonitorType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize RAM monitor
 * @param config Pointer to monitor configuration
 * @return E_OK on success, E_NOT_OK on error
 */
Std_ReturnType RamMonitor_Init(const RamMonitorConfigType *config);

/**
 * @brief Deinitialize RAM monitor
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_DeInit(void);

/**
 * @brief Start RAM monitoring
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_Start(void);

/**
 * @brief Stop RAM monitoring
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_Stop(void);

/**
 * @brief Register a memory region for monitoring
 * @param region Pointer to region definition
 * @return E_OK on success, E_NOT_OK on error
 */
Std_ReturnType RamMonitor_RegisterRegion(const RamRegionType *region);

/**
 * @brief Unregister a memory region
 * @param region_id Region identifier
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_UnregisterRegion(uint32_t region_id);

/**
 * @brief Enable/disable a monitored region
 * @param region_id Region identifier
 * @param enable TRUE to enable, FALSE to disable
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_EnableRegion(uint32_t region_id, boolean enable);

/**
 * @brief Set region scan interval
 * @param region_id Region identifier
 * @param interval_ms New interval in milliseconds
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_SetRegionInterval(uint32_t region_id, uint32_t interval_ms);

/**
 * @brief Perform single scan of a specific region
 * @param region_id Region identifier
 * @param result Pointer to store check result (can be NULL)
 * @return E_OK on success, E_NOT_OK if errors found
 */
Std_ReturnType RamMonitor_ScanRegion(uint32_t region_id, EccCheckResultType *result);

/**
 * @brief Perform scan of all enabled regions
 * @return Number of regions with errors, 0 if all OK
 */
uint32_t RamMonitor_ScanAll(void);

/**
 * @brief Get monitor state
 * @return Pointer to monitor state
 */
const RamMonitorType* RamMonitor_GetState(void);

/**
 * @brief Get monitor statistics
 * @return Pointer to statistics
 */
const RamMonitorStatsType* RamMonitor_GetStats(void);

/**
 * @brief Reset monitor statistics
 * @return E_OK on success
 */
Std_ReturnType RamMonitor_ResetStats(void);

/**
 * @brief Get region information
 * @param region_id Region identifier
 * @return Pointer to region info, NULL if not found
 */
const RamRegionType* RamMonitor_GetRegion(uint32_t region_id);

/**
 * @brief Get number of registered regions
 * @return Number of regions
 */
uint32_t RamMonitor_GetRegionCount(void);

/**
 * @brief Main monitor task (for RTOS integration)
 * @param arg Task argument (unused)
 */
void RamMonitor_Task(void *arg);

/**
 * @brief Timer callback for periodic scanning
 */
void RamMonitor_TimerCallback(void);

/**
 * @brief Force immediate scan of critical regions
 * @return Number of errors found
 */
uint32_t RamMonitor_ScanCritical(void);

/**
 * @brief Check if monitor is healthy
 * @return TRUE if monitor is operating normally
 */
boolean RamMonitor_IsHealthy(void);

/**
 * @brief Get version information
 * @param version Pointer to version info structure
 */
void RamMonitor_GetVersionInfo(Std_VersionInfoType *version);

/******************************************************************************
 * Inline Helper Functions
 ******************************************************************************/

/**
 * @brief Check if region has ECC protection
 * @param region Pointer to region
 * @return TRUE if region has ECC
 */
static inline boolean RamMonitor_RegionHasEcc(const RamRegionType *region)
{
    return ((region->attributes & RAM_REGION_ATTR_SAFEGUARD) != 0U &&
            region->ecc_storage != NULL) ? TRUE : FALSE;
}

/**
 * @brief Check if region is critical
 * @param region Pointer to region
 * @return TRUE if region is critical
 */
static inline boolean RamMonitor_RegionIsCritical(const RamRegionType *region)
{
    return ((region->attributes & RAM_REGION_ATTR_CRITICAL) != 0U) ? TRUE : FALSE;
}

#ifdef __cplusplus
}
#endif

#endif /* RAM_MONITOR_H */
