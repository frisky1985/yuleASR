/******************************************************************************
 * @file    ram_monitor.c
 * @brief   RAM Memory Monitor Implementation
 *
 * Background scanning implementation with:
 * - Priority-based region scheduling
 * - ECC verification on each scan
 * - Error reporting and statistics
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ram_monitor.h"
#include "../../common/utils/eth_utils.h"
#include <string.h>

/******************************************************************************
 * Local Constants
 ******************************************************************************/

#define RAM_MONITOR_VENDOR_ID       0x0001U
#define RAM_MONITOR_MODULE_ID       0x0102U
#define RAM_MONITOR_SW_MAJOR        1U
#define RAM_MONITOR_SW_MINOR        0U
#define RAM_MONITOR_SW_PATCH        0U

/* Scan chunk size (words per scan iteration) */
#define SCAN_CHUNK_SIZE             64U

/* Maximum scan time per iteration (microseconds) */
#define MAX_SCAN_TIME_US            1000U

/******************************************************************************
 * Local Variables
 ******************************************************************************/

static RamMonitorType g_monitor;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

static int32_t RamMonitor_FindRegionIndex(uint32_t region_id);
static Std_ReturnType RamMonitor_ScanRegionInternal(RamRegionType *region, 
                                                     EccCheckResultType *worst_result);
static void RamMonitor_ReportError(const RamRegionType *region, 
                                    const EccCheckResultType *result);
static uint32_t RamMonitor_GetPriorityRegion(void);
static void RamMonitor_UpdateStats(const RamRegionType *region, 
                                    const EccCheckResultType *result);

/******************************************************************************
 * Local Function Implementations
 ******************************************************************************/

/**
 * @brief Find region index by ID
 */
static int32_t RamMonitor_FindRegionIndex(uint32_t region_id)
{
    uint32_t i;
    
    for (i = 0U; i < RAM_MONITOR_MAX_REGIONS; i++) {
        if (g_monitor.regions[i].region_id == region_id) {
            return (int32_t)i;
        }
    }
    return -1;  /* Not found */
}

/**
 * @brief Get highest priority region due for scanning
 */
static uint32_t RamMonitor_GetPriorityRegion(void)
{
    uint32_t i;
    uint32_t best_idx = RAM_MONITOR_MAX_REGIONS;
    uint32_t best_priority = 0U;
    uint32_t current_time = (uint32_t)(eth_get_timestamp_us() / 1000U);

    for (i = 0U; i < RAM_MONITOR_MAX_REGIONS; i++) {
        RamRegionType *region = &g_monitor.regions[i];
        
        if (region->region_id == 0U || !region->enabled) {
            continue;
        }

        /* Check if region is due for scan */
        if ((current_time - region->last_scan_time) >= region->scan_interval_ms) {
            /* Higher priority wins, or same priority: longer waiting wins */
            if (region->priority > best_priority) {
                best_priority = region->priority;
                best_idx = i;
            }
        }
    }

    return best_idx;
}

/**
 * @brief Internal region scan with ECC verification
 */
static Std_ReturnType RamMonitor_ScanRegionInternal(RamRegionType *region,
                                                     EccCheckResultType *worst_result)
{
    Std_ReturnType status = E_OK;
    uint32_t word_count;
    uint32_t words_per_ecc;
    uint32_t i;
    uint32_t *mem_ptr;
    uint8_t *ecc_ptr;
    EccCheckResultType result;
    uint64_t scan_start;
    boolean has_errors = FALSE;

    if (region == NULL || region->start_address == 0U) {
        return E_NOT_OK;
    }

    scan_start = eth_get_timestamp_us();

    /* Calculate word count */
    word_count = region->size_bytes / region->word_size;
    words_per_ecc = (region->word_size == 8U) ? 1U : 1U;  /* One ECC per word */

    /* Get memory and ECC pointers */
    mem_ptr = (uint32_t*)region->start_address;
    ecc_ptr = region->ecc_storage;

    /* Scan memory words */
    for (i = 0U; i < word_count; i++) {
        uint8_t ecc_code;
        uint32_t data;

        /* Read data word */
        data = mem_ptr[i];

        /* Get ECC code */
        if (ecc_ptr != NULL && RamMonitor_RegionHasEcc(region)) {
            ecc_code = ecc_ptr[i];
        } else {
            /* No ECC - just compute and verify */
            EccEncoder_Encode32(data, &ecc_code);
        }

        /* Check and correct */
        if (EccChecker_CheckAndCorrect32(data, ecc_code, &result) != E_OK) {
            has_errors = TRUE;
            
            /* Save worst result */
            if (worst_result != NULL && 
                result.error_type > worst_result->error_type) {
                memcpy(worst_result, &result, sizeof(EccCheckResultType));
            }

            /* Report error */
            RamMonitor_ReportError(region, &result);
            RamMonitor_UpdateStats(region, &result);

            /* Uncorrectable error - fail immediately for critical regions */
            if (RamMonitor_RegionIsCritical(region) && 
                EccChecker_IsUncorrectable(result.error_type)) {
                status = E_NOT_OK;
                break;
            }
        }

        /* Check for timeout */
        if ((eth_get_timestamp_us() - scan_start) > MAX_SCAN_TIME_US) {
            /* Resume from here on next scan */
            g_monitor.scan_start_time = i + 1U;
            g_monitor.status |= RAM_MONITOR_STATUS_SCANNING;
            break;
        }
    }

    /* Update region statistics */
    region->scan_count++;
    region->last_scan_time = (uint32_t)(eth_get_timestamp_us() / 1000U);
    region->scan_duration_us = (uint32_t)(eth_get_timestamp_us() - scan_start);

    return status;
}

/**
 * @brief Report error through callback
 */
static void RamMonitor_ReportError(const RamRegionType *region,
                                    const EccCheckResultType *result)
{
    if (g_monitor.config.error_callback != NULL && region != NULL && result != NULL) {
        g_monitor.config.error_callback(region, result, 
                                        g_monitor.config.callback_user_data);
    }
}

/**
 * @brief Update global and region statistics
 */
static void RamMonitor_UpdateStats(const RamRegionType *region,
                                    const EccCheckResultType *result)
{
    if (region == NULL || result == NULL) {
        return;
    }

    g_monitor.stats.total_errors++;
    g_monitor.stats.last_error_region = region->region_id;
    g_monitor.stats.last_error_time = (uint32_t)(eth_get_timestamp_us() / 1000U);

    if (result->error_type == ECC_ERROR_SINGLE_BIT) {
        g_monitor.stats.single_bit_errors++;
    } else {
        g_monitor.stats.uncorrectable_errors++;
    }

    /* Update region-specific stats */
    ((RamRegionType*)region)->error_count++;
}

/******************************************************************************
 * Public API Implementation
 ******************************************************************************/

/**
 * @brief Initialize RAM monitor
 */
Std_ReturnType RamMonitor_Init(const RamMonitorConfigType *config)
{
    if (config == NULL) {
        return E_NOT_OK;
    }

    /* Clear monitor structure */
    memset(&g_monitor, 0, sizeof(g_monitor));

    /* Copy configuration */
    g_monitor.config = *config;
    
    /* Set defaults if not specified */
    if (g_monitor.config.max_regions == 0U) {
        g_monitor.config.max_regions = RAM_MONITOR_MAX_REGIONS;
    }
    if (g_monitor.config.default_scan_interval == 0U) {
        g_monitor.config.default_scan_interval = RAM_MONITOR_INTERVAL_NORMAL;
    }

    /* Initialize ECC encoder and checker */
    {
        EccEncoderConfigType enc_config;
        EccCheckerConfigType chk_config;

        memset(&enc_config, 0, sizeof(enc_config));
        enc_config.mode = ECC_MODE_32BIT;
        enc_config.asil_level = ASIL_D;
        EccEncoder_Init(&enc_config);

        memset(&chk_config, 0, sizeof(chk_config));
        chk_config.auto_correct = TRUE;
        chk_config.log_errors = TRUE;
        EccChecker_Init(&chk_config);
    }

    g_monitor.initialized = TRUE;
    g_initialized = TRUE;

    /* Auto-start if configured */
    if (config->auto_start) {
        RamMonitor_Start();
    }

    return E_OK;
}

/**
 * @brief Deinitialize RAM monitor
 */
Std_ReturnType RamMonitor_DeInit(void)
{
    g_monitor.running = FALSE;
    g_monitor.initialized = FALSE;
    memset(&g_monitor, 0, sizeof(g_monitor));
    g_initialized = FALSE;
    
    EccEncoder_DeInit();
    EccChecker_DeInit();
    
    return E_OK;
}

/**
 * @brief Start RAM monitoring
 */
Std_ReturnType RamMonitor_Start(void)
{
    if (!g_initialized || !g_monitor.initialized) {
        return E_NOT_OK;
    }

    g_monitor.running = TRUE;
    g_monitor.status &= ~RAM_MONITOR_STATUS_PAUSED;
    
    return E_OK;
}

/**
 * @brief Stop RAM monitoring
 */
Std_ReturnType RamMonitor_Stop(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }

    g_monitor.running = FALSE;
    g_monitor.status |= RAM_MONITOR_STATUS_PAUSED;
    
    return E_OK;
}

/**
 * @brief Register a memory region
 */
Std_ReturnType RamMonitor_RegisterRegion(const RamRegionType *region)
{
    int32_t free_idx = -1;
    uint32_t i;

    if (!g_initialized || region == NULL) {
        return E_NOT_OK;
    }

    /* Check if region already exists */
    if (RamMonitor_FindRegionIndex(region->region_id) >= 0) {
        return E_NOT_OK;  /* Already registered */
    }

    /* Find free slot */
    for (i = 0U; i < RAM_MONITOR_MAX_REGIONS; i++) {
        if (g_monitor.regions[i].region_id == 0U) {
            free_idx = (int32_t)i;
            break;
        }
    }

    if (free_idx < 0) {
        g_monitor.status |= RAM_MONITOR_STATUS_OVERFLOW;
        return E_NOT_OK;  /* No free slots */
    }

    /* Copy region configuration */
    memcpy(&g_monitor.regions[free_idx], region, sizeof(RamRegionType));
    
    /* Set defaults */
    if (g_monitor.regions[free_idx].scan_interval_ms == 0U) {
        g_monitor.regions[free_idx].scan_interval_ms = 
            g_monitor.config.default_scan_interval;
    }
    if (g_monitor.regions[free_idx].word_size == 0U) {
        g_monitor.regions[free_idx].word_size = 4U;
    }

    g_monitor.regions[free_idx].last_scan_time = 0U;
    g_monitor.regions[free_idx].scan_count = 0U;
    g_monitor.regions[free_idx].error_count = 0U;

    g_monitor.stats.total_regions++;
    if (g_monitor.regions[free_idx].enabled) {
        g_monitor.stats.active_regions++;
    }

    return E_OK;
}

/**
 * @brief Unregister a memory region
 */
Std_ReturnType RamMonitor_UnregisterRegion(uint32_t region_id)
{
    int32_t idx;

    if (!g_initialized) {
        return E_NOT_OK;
    }

    idx = RamMonitor_FindRegionIndex(region_id);
    if (idx < 0) {
        return E_NOT_OK;
    }

    if (g_monitor.regions[idx].enabled) {
        g_monitor.stats.active_regions--;
    }
    g_monitor.stats.total_regions--;

    memset(&g_monitor.regions[idx], 0, sizeof(RamRegionType));

    return E_OK;
}

/**
 * @brief Enable/disable region
 */
Std_ReturnType RamMonitor_EnableRegion(uint32_t region_id, boolean enable)
{
    int32_t idx;

    if (!g_initialized) {
        return E_NOT_OK;
    }

    idx = RamMonitor_FindRegionIndex(region_id);
    if (idx < 0) {
        return E_NOT_OK;
    }

    if (g_monitor.regions[idx].enabled != enable) {
        g_monitor.regions[idx].enabled = enable;
        if (enable) {
            g_monitor.stats.active_regions++;
        } else {
            g_monitor.stats.active_regions--;
        }
    }

    return E_OK;
}

/**
 * @brief Set region scan interval
 */
Std_ReturnType RamMonitor_SetRegionInterval(uint32_t region_id, uint32_t interval_ms)
{
    int32_t idx;

    if (!g_initialized) {
        return E_NOT_OK;
    }

    idx = RamMonitor_FindRegionIndex(region_id);
    if (idx < 0) {
        return E_NOT_OK;
    }

    g_monitor.regions[idx].scan_interval_ms = interval_ms;

    return E_OK;
}

/**
 * @brief Scan specific region
 */
Std_ReturnType RamMonitor_ScanRegion(uint32_t region_id, EccCheckResultType *result)
{
    int32_t idx;
    Std_ReturnType status;

    if (!g_initialized) {
        return E_NOT_OK;
    }

    idx = RamMonitor_FindRegionIndex(region_id);
    if (idx < 0) {
        return E_NOT_OK;
    }

    if (result != NULL) {
        memset(result, 0, sizeof(EccCheckResultType));
    }

    status = RamMonitor_ScanRegionInternal(&g_monitor.regions[idx], result);
    
    if (status == E_OK) {
        g_monitor.stats.total_scans++;
    }

    return status;
}

/**
 * @brief Scan all enabled regions
 */
uint32_t RamMonitor_ScanAll(void)
{
    uint32_t i;
    uint32_t error_count = 0U;
    EccCheckResultType worst_result;

    if (!g_initialized || !g_monitor.running) {
        return 0U;
    }

    memset(&worst_result, 0, sizeof(worst_result));

    for (i = 0U; i < RAM_MONITOR_MAX_REGIONS; i++) {
        if (g_monitor.regions[i].region_id != 0U && g_monitor.regions[i].enabled) {
            if (RamMonitor_ScanRegionInternal(&g_monitor.regions[i], 
                                               &worst_result) != E_OK) {
                error_count++;
            }
        }
    }

    g_monitor.stats.total_scans++;

    /* Report progress if callback set */
    if (g_monitor.config.progress_callback != NULL) {
        g_monitor.config.progress_callback(0U, 100U, 
                                           g_monitor.config.callback_user_data);
    }

    return error_count;
}

/**
 * @brief Get monitor state
 */
const RamMonitorType* RamMonitor_GetState(void)
{
    return &g_monitor;
}

/**
 * @brief Get monitor statistics
 */
const RamMonitorStatsType* RamMonitor_GetStats(void)
{
    return &g_monitor.stats;
}

/**
 * @brief Reset statistics
 */
Std_ReturnType RamMonitor_ResetStats(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }

    memset(&g_monitor.stats, 0, sizeof(g_monitor.stats));
    
    /* Restore region count */
    {
        uint32_t i;
        for (i = 0U; i < RAM_MONITOR_MAX_REGIONS; i++) {
            if (g_monitor.regions[i].region_id != 0U) {
                g_monitor.stats.total_regions++;
                if (g_monitor.regions[i].enabled) {
                    g_monitor.stats.active_regions++;
                }
                g_monitor.regions[i].scan_count = 0U;
                g_monitor.regions[i].error_count = 0U;
            }
        }
    }

    return E_OK;
}

/**
 * @brief Get region information
 */
const RamRegionType* RamMonitor_GetRegion(uint32_t region_id)
{
    int32_t idx;

    if (!g_initialized) {
        return NULL;
    }

    idx = RamMonitor_FindRegionIndex(region_id);
    if (idx < 0) {
        return NULL;
    }

    return &g_monitor.regions[idx];
}

/**
 * @brief Get region count
 */
uint32_t RamMonitor_GetRegionCount(void)
{
    if (!g_initialized) {
        return 0U;
    }

    return g_monitor.stats.total_regions;
}

/**
 * @brief Main monitor task (RTOS integration)
 */
void RamMonitor_Task(void *arg)
{
    (void)arg;

    while (g_initialized && g_monitor.running) {
        uint32_t region_idx;

        /* Get highest priority region due for scan */
        region_idx = RamMonitor_GetPriorityRegion();

        if (region_idx < RAM_MONITOR_MAX_REGIONS) {
            /* Scan this region */
            RamMonitor_ScanRegionInternal(&g_monitor.regions[region_idx], NULL);
            
            /* Report progress */
            if (g_monitor.config.progress_callback != NULL) {
                uint32_t percent = (region_idx * 100U) / RAM_MONITOR_MAX_REGIONS;
                g_monitor.config.progress_callback(
                    g_monitor.regions[region_idx].region_id,
                    percent,
                    g_monitor.config.callback_user_data);
            }
        }

        /* Small delay to prevent CPU starvation */
        eth_delay_ms(1);
    }
}

/**
 * @brief Timer callback for periodic scanning
 */
void RamMonitor_TimerCallback(void)
{
    if (!g_initialized || !g_monitor.running) {
        return;
    }

    /* Trigger scan of one priority region */
    {
        uint32_t region_idx = RamMonitor_GetPriorityRegion();
        if (region_idx < RAM_MONITOR_MAX_REGIONS) {
            RamMonitor_ScanRegionInternal(&g_monitor.regions[region_idx], NULL);
        }
    }
}

/**
 * @brief Scan critical regions only
 */
uint32_t RamMonitor_ScanCritical(void)
{
    uint32_t i;
    uint32_t error_count = 0U;

    if (!g_initialized) {
        return 0U;
    }

    for (i = 0U; i < RAM_MONITOR_MAX_REGIONS; i++) {
        if (g_monitor.regions[i].region_id != 0U &&
            g_monitor.regions[i].enabled &&
            RamMonitor_RegionIsCritical(&g_monitor.regions[i])) {
            if (RamMonitor_ScanRegionInternal(&g_monitor.regions[i], NULL) != E_OK) {
                error_count++;
            }
        }
    }

    return error_count;
}

/**
 * @brief Check if monitor is healthy
 */
boolean RamMonitor_IsHealthy(void)
{
    if (!g_initialized) {
        return FALSE;
    }

    /* Check for error rate threshold */
    if (g_monitor.stats.uncorrectable_errors > 10U) {
        return FALSE;
    }

    /* Check for scan overruns */
    if (g_monitor.stats.scan_overruns > 100U) {
        return FALSE;
    }

    return (g_monitor.status & RAM_MONITOR_STATUS_ERROR) ? FALSE : TRUE;
}

/**
 * @brief Get version information
 */
void RamMonitor_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = RAM_MONITOR_VENDOR_ID;
        version->moduleID = RAM_MONITOR_MODULE_ID;
        version->sw_major_version = RAM_MONITOR_SW_MAJOR;
        version->sw_minor_version = RAM_MONITOR_SW_MINOR;
        version->sw_patch_version = RAM_MONITOR_SW_PATCH;
    }
}
