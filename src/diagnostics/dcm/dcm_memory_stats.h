/******************************************************************************
 * @file    dcm_memory_stats.h
 * @brief   DCM Memory Statistics and Monitoring
 *
 * Comprehensive memory usage tracking for DCM module including:
 * - Real-time memory usage tracking
 * - Peak usage monitoring
 * - Allocation failure tracking
 * - Memory leak detection
 * - Per-module statistics
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef DCM_MEMORY_STATS_H
#define DCM_MEMORY_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcm_types.h"
#include "dcm_memory_pool.h"

/******************************************************************************
 * Memory Statistics Configuration
 ******************************************************************************/
#define DCM_MEM_STATS_MAX_TRACKED_PTRS  64U     /* Max tracked allocations */
#define DCM_MEM_STATS_HISTORY_SIZE      16U     /* Usage history depth */
#define DCM_MEM_STATS_MODULE_COUNT      8U      /* Number of trackable modules */

/* Memory warning thresholds */
#define DCM_MEM_WARN_THRESHOLD_LOW      70U     /* 70% - Low warning */
#define DCM_MEM_WARN_THRESHOLD_MEDIUM   85U     /* 85% - Medium warning */
#define DCM_MEM_WARN_THRESHOLD_HIGH     95U     /* 95% - High warning */

/******************************************************************************
 * Memory Statistics Types
 ******************************************************************************/

/* Memory module identifiers */
typedef enum {
    DCM_MEM_MODULE_CORE = 0,            /* DCM core module */
    DCM_MEM_MODULE_SESSION,             /* Session control */
    DCM_MEM_MODULE_SECURITY,            /* Security access */
    DCM_MEM_MODULE_COMMUNICATION,       /* Communication control */
    DCM_MEM_MODULE_DYNAMIC_DID,         /* Dynamic DID */
    DCM_MEM_MODULE_MEMORY,              /* Memory write */
    DCM_MEM_MODULE_ROUTINE,             /* Routine control */
    DCM_MEM_MODULE_CACHE,               /* Response cache */
    DCM_MEM_MODULE_PRIORITY_QUEUE,      /* Priority queue */
    DCM_MEM_MODULE_COUNT
} Dcm_MemModuleId;

/* Memory usage snapshot */
typedef struct {
    uint32_t timestamp;                 /* Snapshot timestamp */
    uint32_t totalAllocated;            /* Total bytes allocated */
    uint32_t totalFreed;                /* Total bytes freed */
    uint32_t currentUsed;               /* Current bytes in use */
    uint32_t peakUsed;                  /* Peak usage */
    uint32_t allocationCount;           /* Number of allocations */
    uint32_t freeCount;                 /* Number of frees */
    uint32_t failCount;                 /* Failed allocations */
} Dcm_MemUsageSnapshot;

/* Per-module statistics */
typedef struct {
    uint32_t currentAllocated;          /* Currently allocated bytes */
    uint32_t peakAllocated;             /* Peak allocation for module */
    uint32_t totalAllocations;          /* Total allocation count */
    uint32_t totalFrees;                /* Total free count */
    uint32_t leakSuspects;              /* Potential leak count */
    const char *moduleName;             /* Module name */
} Dcm_MemModuleStats;

/* Tracked allocation entry */
typedef struct {
    void *ptr;                          /* Allocated pointer */
    uint32_t size;                      /* Allocation size */
    uint32_t timestamp;                 /* Allocation time */
    Dcm_MemModuleId module;             /* Allocating module */
    const char *file;                   /* Source file */
    uint32_t line;                      /* Source line */
    bool inUse;                         /* Entry active flag */
    uint16_t sequence;                  /* Sequence number */
} Dcm_MemTrackedAlloc;

/* Memory leak report entry */
typedef struct {
    void *ptr;
    uint32_t size;
    uint32_t age;                       /* Time since allocation */
    Dcm_MemModuleId module;
    const char *file;
    uint32_t line;
} Dcm_MemLeakEntry;

/* Memory statistics state */
typedef struct {
    bool initialized;
    bool trackingEnabled;
    
    /* Global statistics */
    Dcm_MemUsageSnapshot current;
    Dcm_MemUsageSnapshot history[DCM_MEM_STATS_HISTORY_SIZE];
    uint8_t historyIndex;
    uint8_t historyCount;
    
    /* Per-module statistics */
    Dcm_MemModuleStats moduleStats[DCM_MEM_MODULE_COUNT];
    
    /* Detailed tracking */
    Dcm_MemTrackedAlloc tracked[DCM_MEM_STATS_MAX_TRACKED_PTRS];
    uint16_t trackedCount;
    uint16_t sequenceCounter;
    
    /* Warnings */
    uint32_t warningCount;
    uint32_t lastWarningTime;
    uint8_t lastWarningLevel;
    
    /* Pool integration */
    Dcm_PoolStats poolStats;
} Dcm_MemStatsState;

/* Memory report structure */
typedef struct {
    Dcm_MemUsageSnapshot snapshot;
    Dcm_MemModuleStats modules[DCM_MEM_MODULE_COUNT];
    uint32_t leakCount;
    uint32_t fragmentationPercent;
    uint8_t overallHealth;              /* 0-100 health score */
} Dcm_MemReport;

/* Memory warning callback */
typedef void (*Dcm_MemWarningCallback)(uint8_t level, uint32_t usagePercent,
                                       Dcm_MemModuleId module);

/******************************************************************************
 * Memory Statistics API
 ******************************************************************************/

/**
 * @brief Initialize memory statistics
 *
 * @param enableTracking Enable detailed allocation tracking
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsInit(bool enableTracking);

/**
 * @brief Deinitialize memory statistics
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsDeInit(void);

/**
 * @brief Record memory allocation
 *
 * @param ptr Allocated pointer
 * @param size Allocation size
 * @param module Allocating module
 * @param file Source file (NULL if not tracking)
 * @param line Source line
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsRecordAlloc(void *ptr, uint32_t size,
                                       Dcm_MemModuleId module,
                                       const char *file, uint32_t line);

/**
 * @brief Record memory free
 *
 * @param ptr Freed pointer
 * @param module Module freeing memory
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsRecordFree(void *ptr, Dcm_MemModuleId module);

/**
 * @brief Record allocation failure
 *
 * @param size Requested size
 * @param module Requesting module
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsRecordFail(uint32_t size, Dcm_MemModuleId module);

/**
 * @brief Get current memory usage snapshot
 *
 * @param snapshot Pointer to snapshot structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsGetSnapshot(Dcm_MemUsageSnapshot *snapshot);

/**
 * @brief Get module statistics
 *
 * @param module Module ID
 * @param stats Pointer to statistics structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_MemModuleStats* Dcm_MemStatsGetModuleStats(Dcm_MemModuleId module);

/**
 * @brief Get memory report
 *
 * @param report Pointer to report structure
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsGetReport(Dcm_MemReport *report);

/**
 * @brief Detect memory leaks
 *
 * @param leaks Array to store leak entries
 * @param maxEntries Maximum entries to store
 * @param leakCount Output: number of leaks found
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsDetectLeaks(Dcm_MemLeakEntry *leaks,
                                       uint32_t maxEntries,
                                       uint32_t *leakCount);

/**
 * @brief Set memory warning callback
 *
 * @param callback Callback function
 * @param thresholdLow Low warning threshold (percent)
 * @param thresholdMed Medium warning threshold (percent)
 * @param thresholdHigh High warning threshold (percent)
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsSetWarningCallback(Dcm_MemWarningCallback callback,
                                               uint8_t thresholdLow,
                                               uint8_t thresholdMed,
                                               uint8_t thresholdHigh);

/**
 * @brief Check memory health
 *
 * @return uint8_t Health score (0-100)
 */
uint8_t Dcm_MemStatsCheckHealth(void);

/**
 * @brief Reset statistics
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsReset(void);

/**
 * @brief Update statistics (call periodically)
 *
 * @param elapsedMs Time since last update
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsUpdate(uint32_t elapsedMs);

/**
 * @brief Get peak memory usage
 *
 * @return uint32_t Peak bytes used
 */
uint32_t Dcm_MemStatsGetPeakUsage(void);

/**
 * @brief Get current memory usage
 *
 * @return uint32_t Current bytes used
 */
uint32_t Dcm_MemStatsGetCurrentUsage(void);

/**
 * @brief Check if tracking is enabled
 *
 * @return bool True if tracking enabled
 */
bool Dcm_MemStatsIsTrackingEnabled(void);

/**
 * @brief Dump statistics to debug output
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsDump(void);

/**
 * @brief Print memory report
 *
 * @return Dcm_ReturnType DCM_E_OK on success
 */
Dcm_ReturnType Dcm_MemStatsPrintReport(void);

/******************************************************************************
 * Convenience Macros
 ******************************************************************************/

/* Track allocation with automatic module detection */
#if defined(DCM_MEM_TRACKING_ENABLED)
    #define DCM_MEM_TRACK_ALLOC(ptr, size, module) \
        Dcm_MemStatsRecordAlloc((ptr), (size), (module), __FILE__, __LINE__)
    
    #define DCM_MEM_TRACK_FREE(ptr, module) \
        Dcm_MemStatsRecordFree((ptr), (module))
    
    #define DCM_MEM_TRACK_FAIL(size, module) \
        Dcm_MemStatsRecordFail((size), (module))
#else
    #define DCM_MEM_TRACK_ALLOC(ptr, size, module) ((void)0)
    #define DCM_MEM_TRACK_FREE(ptr, module) ((void)0)
    #define DCM_MEM_TRACK_FAIL(size, module) ((void)0)
#endif

/* Module-specific tracking */
#define DCM_MEM_TRACK_CORE_ALLOC(ptr, size)     DCM_MEM_TRACK_ALLOC(ptr, size, DCM_MEM_MODULE_CORE)
#define DCM_MEM_TRACK_CORE_FREE(ptr)            DCM_MEM_TRACK_FREE(ptr, DCM_MEM_MODULE_CORE)
#define DCM_MEM_TRACK_SESSION_ALLOC(ptr, size)  DCM_MEM_TRACK_ALLOC(ptr, size, DCM_MEM_MODULE_SESSION)
#define DCM_MEM_TRACK_SESSION_FREE(ptr)         DCM_MEM_TRACK_FREE(ptr, DCM_MEM_MODULE_SESSION)
#define DCM_MEM_TRACK_SECURITY_ALLOC(ptr, size) DCM_MEM_TRACK_ALLOC(ptr, size, DCM_MEM_MODULE_SECURITY)
#define DCM_MEM_TRACK_SECURITY_FREE(ptr)        DCM_MEM_TRACK_FREE(ptr, DCM_MEM_MODULE_SECURITY)

/* Helper to get module name */
static inline const char* Dcm_MemStatsGetModuleName(Dcm_MemModuleId module) {
    switch (module) {
        case DCM_MEM_MODULE_CORE:          return "CORE";
        case DCM_MEM_MODULE_SESSION:       return "SESSION";
        case DCM_MEM_MODULE_SECURITY:      return "SECURITY";
        case DCM_MEM_MODULE_COMMUNICATION: return "COMM";
        case DCM_MEM_MODULE_DYNAMIC_DID:   return "DID";
        case DCM_MEM_MODULE_MEMORY:        return "MEMORY";
        case DCM_MEM_MODULE_ROUTINE:       return "ROUTINE";
        case DCM_MEM_MODULE_CACHE:         return "CACHE";
        case DCM_MEM_MODULE_PRIORITY_QUEUE:return "PQUEUE";
        default:                           return "UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* DCM_MEMORY_STATS_H */
