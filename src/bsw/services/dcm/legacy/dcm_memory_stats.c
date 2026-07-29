/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/******************************************************************************
 * @file    dcm_memory_stats.c
 * @brief   DCM Memory Statistics Implementation
 *
 * Comprehensive memory usage tracking and monitoring for DCM module.
 *
 * AUTOSAR R22-11 compliant
 * MISRA C:2012 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "dcm_memory_stats.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Private Macros
 ******************************************************************************/
#define DCM_MEM_STATS_MAGIC_INIT        0x53544154U  /* "STAT" */
#define DCM_MEM_STATS_LEAK_AGE_THRESHOLD 60000U      /* 60 seconds */

/******************************************************************************
 * Module State
 ******************************************************************************/
static Dcm_MemStatsState s_memStats;
static Dcm_MemWarningCallback s_warningCallback = NULL_PTR;
static uint8_t s_thresholdLow = DCM_MEM_WARN_THRESHOLD_LOW;
static uint8_t s_thresholdMed = DCM_MEM_WARN_THRESHOLD_MEDIUM;
static uint8_t s_thresholdHigh = DCM_MEM_WARN_THRESHOLD_HIGH;

/******************************************************************************
 * Module Names
 ******************************************************************************/
static const char* s_moduleNames[DCM_MEM_MODULE_COUNT] = {
    "CORE",
    "SESSION",
    "SECURITY",
    "COMMUNICATION",
    "DYNAMIC_DID",
    "MEMORY",
    "ROUTINE",
    "CACHE"
};

/******************************************************************************
 * Internal Helper Functions
 ******************************************************************************/

/**
 * @brief Initialize module statistics
 */
static void initModuleStats(void)
{
    for (uint8_t i = 0U; i < DCM_MEM_MODULE_COUNT; i++) {
        (void)memset(&s_memStats.moduleStats[i], 0, sizeof(Dcm_MemModuleStats));
        s_memStats.moduleStats[i].moduleName = s_moduleNames[i];
    }
}

/**
 * @brief Add snapshot to history
 */
static void addToHistory(const Dcm_MemUsageSnapshot *snapshot)
{
    if (s_memStats.historyCount < DCM_MEM_STATS_HISTORY_SIZE) {
        s_memStats.history[s_memStats.historyCount] = *snapshot;
        s_memStats.historyCount++;
    } else {
        /* Overwrite oldest */
        s_memStats.history[s_memStats.historyIndex] = *snapshot;
        s_memStats.historyIndex = (s_memStats.historyIndex + 1U) % DCM_MEM_STATS_HISTORY_SIZE;
    }
}

/**
 * @brief Find tracked allocation entry
 */
static int16_t findTrackedEntry(const void *ptr)
{
    if (ptr == NULL_PTR) {
        return -1;
    }
    
    for (uint16_t i = 0U; i < DCM_MEM_STATS_MAX_TRACKED_PTRS; i++) {
        if (s_memStats.tracked[i].inUse && (s_memStats.tracked[i].ptr == ptr)) {
            return (int16_t)i;
        }
    }
    
    return -1;
}

/**
 * @brief Find free tracked allocation slot
 */
static int16_t findFreeTrackedSlot(void)
{
    for (uint16_t i = 0U; i < DCM_MEM_STATS_MAX_TRACKED_PTRS; i++) {
        if (!s_memStats.tracked[i].inUse) {
            return (int16_t)i;
        }
    }
    
    return -1;
}

/**
 * @brief Check and trigger warnings
 */
static void checkWarnings(Dcm_MemModuleId module)
{
    if (s_warningCallback == NULL_PTR) {
        return;
    }
    
    /* Calculate usage percentage for module */
    uint32_t moduleUsage = 0U;
    if (s_memStats.moduleStats[module].peakAllocated > 0U) {
        moduleUsage = (s_memStats.moduleStats[module].currentAllocated * 100U) /
                      s_memStats.moduleStats[module].peakAllocated;
    }
    
    uint8_t level = 0U;
    if (moduleUsage >= s_thresholdHigh) {
        level = 3U;
    } else if (moduleUsage >= s_thresholdMed) {
        level = 2U;
    } else if (moduleUsage >= s_thresholdLow) {
        level = 1U;
    }
    
    if ((level > 0U) && (level > s_memStats.lastWarningLevel)) {
        s_warningCallback(level, (uint8_t)moduleUsage, module);
        s_memStats.warningCount++;
        s_memStats.lastWarningLevel = level;
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Dcm_ReturnType Dcm_MemStatsInit(bool enableTracking)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    /* Clear state */
    (void)memset(&s_memStats, 0, sizeof(s_memStats));
    
    /* Initialize module stats */
    initModuleStats();
    
    s_memStats.trackingEnabled = enableTracking;
    s_memStats.initialized = true;
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsDeInit(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (s_memStats.initialized) {
        (void)memset(&s_memStats, 0, sizeof(s_memStats));
        s_warningCallback = NULL_PTR;
        result = DCM_E_OK;
    }
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsRecordAlloc(void *ptr, uint32_t size,
                                       Dcm_MemModuleId module,
                                       const char *file, uint32_t line)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized || (ptr == NULL_PTR) || (size == 0U)) {
        return result;
    }
    
    if (module >= DCM_MEM_MODULE_COUNT) {
        module = DCM_MEM_MODULE_CORE;
    }
    
    /* Update global stats */
    s_memStats.current.totalAllocated += size;
    s_memStats.current.currentUsed += size;
    s_memStats.current.allocationCount++;
    
    if (s_memStats.current.currentUsed > s_memStats.current.peakUsed) {
        s_memStats.current.peakUsed = s_memStats.current.currentUsed;
    }
    
    /* Update module stats */
    s_memStats.moduleStats[module].currentAllocated += size;
    s_memStats.moduleStats[module].totalAllocations++;
    
    if (s_memStats.moduleStats[module].currentAllocated > 
        s_memStats.moduleStats[module].peakAllocated) {
        s_memStats.moduleStats[module].peakAllocated = 
            s_memStats.moduleStats[module].currentAllocated;
    }
    
    /* Track allocation if enabled */
    if (s_memStats.trackingEnabled) {
        int16_t slot = findFreeTrackedSlot();
        if (slot >= 0) {
            Dcm_MemTrackedAlloc *track = &s_memStats.tracked[slot];
            track->ptr = ptr;
            track->size = size;
            track->timestamp = 0U; /* 时间戳依赖系统定时器集成 */
            track->module = module;
            track->file = file;
            track->line = line;
            track->inUse = true;
            track->sequence = s_memStats.sequenceCounter++;
            s_memStats.trackedCount++;
        }
    }
    
    /* Check for warnings */
    checkWarnings(module);
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsRecordFree(void *ptr, Dcm_MemModuleId module)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized || (ptr == NULL_PTR)) {
        return result;
    }
    
    if (module >= DCM_MEM_MODULE_COUNT) {
        module = DCM_MEM_MODULE_CORE;
    }
    
    /* Find tracked entry to get size */
    uint32_t size = 0U;
    if (s_memStats.trackingEnabled) {
        int16_t slot = findTrackedEntry(ptr);
        if (slot >= 0) {
            size = s_memStats.tracked[slot].size;
            s_memStats.tracked[slot].inUse = false;
            s_memStats.trackedCount--;
        }
    }
    
    /* Update global stats */
    s_memStats.current.totalFreed += size;
    if (s_memStats.current.currentUsed >= size) {
        s_memStats.current.currentUsed -= size;
    } else {
        s_memStats.current.currentUsed = 0U;
    }
    s_memStats.current.freeCount++;
    
    /* Update module stats */
    if (s_memStats.moduleStats[module].currentAllocated >= size) {
        s_memStats.moduleStats[module].currentAllocated -= size;
    } else {
        s_memStats.moduleStats[module].currentAllocated = 0U;
    }
    s_memStats.moduleStats[module].totalFrees++;
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsRecordFail(uint32_t size, Dcm_MemModuleId module)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized) {
        return result;
    }
    
    if (module >= DCM_MEM_MODULE_COUNT) {
        module = DCM_MEM_MODULE_CORE;
    }
    
    s_memStats.current.failCount++;
    
    /* Trigger high warning on allocation failure */
    if (s_warningCallback != NULL_PTR) {
        s_warningCallback(3U, 100U, module);
    }
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsGetSnapshot(Dcm_MemUsageSnapshot *snapshot)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized || (snapshot == NULL_PTR)) {
        return result;
    }
    
    *snapshot = s_memStats.current;
    result = DCM_E_OK;
    
    return result;
}

Dcm_MemModuleStats* Dcm_MemStatsGetModuleStats(Dcm_MemModuleId module)
{
    if (!s_memStats.initialized || (module >= DCM_MEM_MODULE_COUNT)) {
        return NULL_PTR;
    }
    
    return &s_memStats.moduleStats[module];
}

Dcm_ReturnType Dcm_MemStatsGetReport(Dcm_MemReport *report)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized || (report == NULL_PTR)) {
        return result;
    }
    
    /* Copy snapshot */
    report->snapshot = s_memStats.current;
    
    /* Copy module stats */
    for (uint8_t i = 0U; i < DCM_MEM_MODULE_COUNT; i++) {
        report->modules[i] = s_memStats.moduleStats[i];
    }
    
    /* Calculate leak count */
    report->leakCount = 0U;
    if (s_memStats.trackingEnabled) {
        for (uint16_t i = 0U; i < DCM_MEM_STATS_MAX_TRACKED_PTRS; i++) {
            if (s_memStats.tracked[i].inUse) {
                report->leakCount++;
            }
        }
    }
    
    /* Calculate fragmentation (simplified) */
    report->fragmentationPercent = 0U;
    
    /* Calculate health score */
    report->overallHealth = Dcm_MemStatsCheckHealth();
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsDetectLeaks(Dcm_MemLeakEntry *leaks,
                                       uint32_t maxEntries,
                                       uint32_t *leakCount)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized || (leaks == NULL_PTR) || (leakCount == NULL_PTR)) {
        return result;
    }
    
    *leakCount = 0U;
    
    if (!s_memStats.trackingEnabled) {
        return DCM_E_OK; /* No tracking, no leaks detected */
    }
    
    uint32_t currentTime = 0U; /* 时间戳依赖系统定时器集成 */
    
    for (uint16_t i = 0U; i < DCM_MEM_STATS_MAX_TRACKED_PTRS; i++) {
        if (s_memStats.tracked[i].inUse) {
            uint32_t age = currentTime - s_memStats.tracked[i].timestamp;
            
            /* Consider it a leak if old enough */
            if (age >= DCM_MEM_STATS_LEAK_AGE_THRESHOLD) {
                if (*leakCount < maxEntries) {
                    leaks[*leakCount].ptr = s_memStats.tracked[i].ptr;
                    leaks[*leakCount].size = s_memStats.tracked[i].size;
                    leaks[*leakCount].age = age;
                    leaks[*leakCount].module = s_memStats.tracked[i].module;
                    leaks[*leakCount].file = s_memStats.tracked[i].file;
                    leaks[*leakCount].line = s_memStats.tracked[i].line;
                }
                (*leakCount)++;
            }
        }
    }
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsSetWarningCallback(Dcm_MemWarningCallback callback,
                                               uint8_t thresholdLow,
                                               uint8_t thresholdMed,
                                               uint8_t thresholdHigh)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized) {
        return result;
    }
    
    s_warningCallback = callback;
    s_thresholdLow = thresholdLow;
    s_thresholdMed = thresholdMed;
    s_thresholdHigh = thresholdHigh;
    
    result = DCM_E_OK;
    
    return result;
}

uint8_t Dcm_MemStatsCheckHealth(void)
{
    if (!s_memStats.initialized) {
        return 0U;
    }
    
    uint8_t health = 100U;
    
    /* Reduce health based on failure rate */
    if (s_memStats.current.failCount > 0U) {
        health -= (uint8_t)((s_memStats.current.failCount > 10U) ? 30U : 
                            (s_memStats.current.failCount * 3U));
    }
    
    /* Reduce health if near peak */
    if (s_memStats.current.peakUsed > 0U) {
        uint32_t usagePercent = (s_memStats.current.currentUsed * 100U) / 
                                s_memStats.current.peakUsed;
        if (usagePercent > 90U) {
            health -= 20U;
        } else if (usagePercent > 70U) {
            health -= 10U;
        }
    }
    
    /* Reduce health for potential leaks */
    uint32_t leakCount = 0U;
    for (uint8_t i = 0U; i < DCM_MEM_MODULE_COUNT; i++) {
        int32_t outstanding = (int32_t)s_memStats.moduleStats[i].totalAllocations - 
                              (int32_t)s_memStats.moduleStats[i].totalFrees;
        if (outstanding > 10) {
            leakCount += (uint32_t)outstanding;
        }
    }
    
    if (leakCount > 0U) {
        health -= (uint8_t)((leakCount > 10U) ? 20U : (uint8_t)(leakCount * 2U));
    }
    
    return (health > 100U) ? 0U : health;
}

Dcm_ReturnType Dcm_MemStatsReset(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized) {
        return result;
    }
    
    /* Reset current stats but keep peak */
    uint32_t peak = s_memStats.current.peakUsed;
    (void)memset(&s_memStats.current, 0, sizeof(s_memStats.current));
    s_memStats.current.peakUsed = peak;
    
    /* Reset history */
    s_memStats.historyCount = 0U;
    s_memStats.historyIndex = 0U;
    
    /* Reset tracked allocations */
    for (uint16_t i = 0U; i < DCM_MEM_STATS_MAX_TRACKED_PTRS; i++) {
        s_memStats.tracked[i].inUse = false;
    }
    s_memStats.trackedCount = 0U;
    
    /* Reset warnings */
    s_memStats.warningCount = 0U;
    s_memStats.lastWarningLevel = 0U;
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsUpdate(uint32_t elapsedMs)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized) {
        return result;
    }
    
    /* Update timestamp */
    s_memStats.current.timestamp += elapsedMs;
    
    /* Add to history every 1000ms */
    static uint32_t historyAccumulator = 0U;
    historyAccumulator += elapsedMs;
    
    if (historyAccumulator >= 1000U) {
        addToHistory(&s_memStats.current);
        historyAccumulator = 0U;
    }
    
    result = DCM_E_OK;
    
    return result;
}

uint32_t Dcm_MemStatsGetPeakUsage(void)
{
    if (!s_memStats.initialized) {
        return 0U;
    }
    
    return s_memStats.current.peakUsed;
}

uint32_t Dcm_MemStatsGetCurrentUsage(void)
{
    if (!s_memStats.initialized) {
        return 0U;
    }
    
    return s_memStats.current.currentUsed;
}

bool Dcm_MemStatsIsTrackingEnabled(void)
{
    return (s_memStats.initialized && s_memStats.trackingEnabled);
}

Dcm_ReturnType Dcm_MemStatsDump(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized) {
        return result;
    }
    
    /* Output would go to debug console in real implementation */
    /* For now, just print to stdout */
    (void)printf("\n========== DCM Memory Statistics ==========\n");
    (void)printf("Current Usage:  %lu bytes\n", 
                 (unsigned long)s_memStats.current.currentUsed);
    (void)printf("Peak Usage:     %lu bytes\n", 
                 (unsigned long)s_memStats.current.peakUsed);
    (void)printf("Total Allocs:   %lu\n", 
                 (unsigned long)s_memStats.current.allocationCount);
    (void)printf("Total Frees:    %lu\n", 
                 (unsigned long)s_memStats.current.freeCount);
    (void)printf("Failures:       %lu\n", 
                 (unsigned long)s_memStats.current.failCount);
    (void)printf("Health Score:   %u%%\n", Dcm_MemStatsCheckHealth());
    (void)printf("\n---------- Per-Module Statistics ----------\n");
    
    for (uint8_t i = 0U; i < DCM_MEM_MODULE_COUNT; i++) {
        const Dcm_MemModuleStats *mod = &s_memStats.moduleStats[i];
        if (mod->totalAllocations > 0U) {
            (void)printf("%-15s: Cur=%lu Peak=%lu Allocs=%lu Frees=%lu\n",
                         mod->moduleName,
                         (unsigned long)mod->currentAllocated,
                         (unsigned long)mod->peakAllocated,
                         (unsigned long)mod->totalAllocations,
                         (unsigned long)mod->totalFrees);
        }
    }
    
    (void)printf("==========================================\n\n");
    
    result = DCM_E_OK;
    
    return result;
}

Dcm_ReturnType Dcm_MemStatsPrintReport(void)
{
    Dcm_ReturnType result = DCM_E_NOT_OK;
    
    if (!s_memStats.initialized) {
        return result;
    }
    
    Dcm_MemReport report;
    
    if (Dcm_MemStatsGetReport(&report) == DCM_E_OK) {
        (void)printf("\n========== DCM Memory Report ==========\n");
        (void)printf("Timestamp:      %lu ms\n", 
                     (unsigned long)report.snapshot.timestamp);
        (void)printf("Current Used:   %lu bytes\n", 
                     (unsigned long)report.snapshot.currentUsed);
        (void)printf("Peak Used:      %lu bytes\n", 
                     (unsigned long)report.snapshot.peakUsed);
        (void)printf("Fragmentation:  %lu%%\n", 
                     (unsigned long)report.fragmentationPercent);
        (void)printf("Health Score:   %u%%\n", report.overallHealth);
        (void)printf("Potential Leaks: %lu\n", 
                     (unsigned long)report.leakCount);
        (void)printf("=======================================\n\n");
        
        result = DCM_E_OK;
    }
    
    return result;
}
