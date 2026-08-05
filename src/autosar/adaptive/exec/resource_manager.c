/******************************************************************************
 * @file    resource_manager.c
 * @brief   Resource Manager Implementation
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "ara/exec/resource_manager.h"
#include <string.h>
#include <stdlib.h>

/******************************************************************************
 * Private Data
 ******************************************************************************/

static bool g_initialized = false;
static ResourceEntryType g_resourceTable[EXEC_MAX_PROCESSES];
static uint32_t g_lastMonitorTime = 0U;

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static ResourceEntryType* FindResourceEntry(ProcessIdType pid) {
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (g_resourceTable[i].pid == pid && g_resourceTable[i].monitoringEnabled) {
            return &g_resourceTable[i];
        }
    }
    return NULL;
}

static ResourceEntryType* FindOrCreateEntry(ProcessIdType pid) {
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry != NULL) {
        return entry;
    }
    
    /* Find free entry */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (!g_resourceTable[i].monitoringEnabled) {
            g_resourceTable[i].pid = pid;
            g_resourceTable[i].monitoringEnabled = false;
            memset(&g_resourceTable[i].limits, 0, sizeof(g_resourceTable[i].limits));
            memset(&g_resourceTable[i].currentUsage, 0, sizeof(ResourceUsageType));
            memset(&g_resourceTable[i].peakUsage, 0, sizeof(ResourceUsageType));
            return &g_resourceTable[i];
        }
    }
    return NULL;
}

static Std_ReturnType PlatformGetResourceUsage(ProcessIdType pid, ResourceUsageType* usage) {
    /* Platform-specific: Get resource usage from OS */
    /* This is a stub - actual implementation depends on target platform */
    (void)pid;
    memset(usage, 0, sizeof(ResourceUsageType));
    return E_OK;
}

static bool PlatformSetResourceLimit(ProcessIdType pid, const ResourceLimitType* limit) {
    /* Platform-specific: Set resource limit in OS */
    /* This is a stub - actual implementation depends on target platform */
    (void)pid;
    (void)limit;
    return true;
}

static bool PlatformGetResourceLimit(ProcessIdType pid, ResourceTypeType type, 
                                      ResourceLimitType* limit) {
    /* Platform-specific: Get resource limit from OS */
    (void)pid;
    (void)type;
    memset(limit, 0, sizeof(ResourceLimitType));
    limit->type = type;
    return true;
}

static void UpdatePeakUsage(ResourceEntryType* entry) {
    if (entry == NULL) {
        return;
    }
    
    if (entry->currentUsage.cpuTimeUs > entry->peakUsage.cpuTimeUs) {
        entry->peakUsage.cpuTimeUs = entry->currentUsage.cpuTimeUs;
    }
    if (entry->currentUsage.memoryBytes > entry->peakUsage.memoryBytes) {
        entry->peakUsage.memoryBytes = entry->currentUsage.memoryBytes;
    }
    if (entry->currentUsage.fileDescriptors > entry->peakUsage.fileDescriptors) {
        entry->peakUsage.fileDescriptors = entry->currentUsage.fileDescriptors;
    }
    if (entry->currentUsage.diskReadBytes > entry->peakUsage.diskReadBytes) {
        entry->peakUsage.diskReadBytes = entry->currentUsage.diskReadBytes;
    }
    if (entry->currentUsage.diskWriteBytes > entry->peakUsage.diskWriteBytes) {
        entry->peakUsage.diskWriteBytes = entry->currentUsage.diskWriteBytes;
    }
    if (entry->currentUsage.networkRxBytes > entry->peakUsage.networkRxBytes) {
        entry->peakUsage.networkRxBytes = entry->currentUsage.networkRxBytes;
    }
    if (entry->currentUsage.networkTxBytes > entry->peakUsage.networkTxBytes) {
        entry->peakUsage.networkTxBytes = entry->currentUsage.networkTxBytes;
    }
}

/******************************************************************************
 * Public Functions
 ******************************************************************************/

Std_ReturnType ResourceManager_Init(void) {
    if (g_initialized) {
        return E_OK;
    }
    
    memset(g_resourceTable, 0, sizeof(g_resourceTable));
    g_lastMonitorTime = 0U;
    
    g_initialized = true;
    return E_OK;
}

Std_ReturnType ResourceManager_Deinit(void) {
    if (!g_initialized) {
        return E_OK;
    }
    
    /* Disable monitoring for all processes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        g_resourceTable[i].monitoringEnabled = false;
    }
    
    g_initialized = false;
    return E_OK;
}

bool ResourceManager_IsInitialized(void) {
    return g_initialized;
}

Std_ReturnType ResourceManager_SetLimit(ProcessIdType pid, const ResourceLimitType* limit) {
    if (!g_initialized || limit == NULL) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindOrCreateEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    /* Validate limits */
    if (limit->softLimit > limit->hardLimit) {
        return E_NOT_OK;
    }
    
    /* Set limit in table */
    memcpy(&entry->limits[limit->type], limit, sizeof(ResourceLimitType));
    
    /* Apply limit to platform */
    if (!PlatformSetResourceLimit(pid, limit)) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

Std_ReturnType ResourceManager_GetLimit(ProcessIdType pid, ResourceTypeType type,
                                         ResourceLimitType* limit) {
    if (!g_initialized || limit == NULL) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry != NULL && entry->limits[type].enabled) {
        memcpy(limit, &entry->limits[type], sizeof(ResourceLimitType));
        return E_OK;
    }
    
    /* Get from platform */
    if (PlatformGetResourceLimit(pid, type, limit)) {
        return E_OK;
    }
    
    return E_NOT_OK;
}

Std_ReturnType ResourceManager_GetUsage(ProcessIdType pid, ResourceUsageType* usage) {
    if (!g_initialized || usage == NULL) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry != NULL) {
        memcpy(usage, &entry->currentUsage, sizeof(ResourceUsageType));
        return E_OK;
    }
    
    /* Get directly from platform */
    return PlatformGetResourceUsage(pid, usage);
}

Std_ReturnType ResourceManager_GetPeakUsage(ProcessIdType pid, ResourceUsageType* usage) {
    if (!g_initialized || usage == NULL) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(usage, &entry->peakUsage, sizeof(ResourceUsageType));
    return E_OK;
}

Std_ReturnType ResourceManager_ResetPeakUsage(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    memset(&entry->peakUsage, 0, sizeof(ResourceUsageType));
    return E_OK;
}

Std_ReturnType ResourceManager_EnableMonitoring(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindOrCreateEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    entry->monitoringEnabled = true;
    
    /* Initialize default limits if not set */
    for (uint32_t i = 0U; i < 5U; i++) {
        if (!entry->limits[i].enabled) {
            entry->limits[i].type = (ResourceTypeType)i;
            entry->limits[i].enabled = true;
            
            switch (i) {
                case RESOURCE_TYPE_CPU:
                    entry->limits[i].softLimit = RESOURCE_DEFAULT_CPU_LIMIT;
                    entry->limits[i].hardLimit = 100U;
                    break;
                case RESOURCE_TYPE_MEMORY:
                    entry->limits[i].softLimit = RESOURCE_DEFAULT_MEMORY_LIMIT;
                    entry->limits[i].hardLimit = RESOURCE_DEFAULT_MEMORY_LIMIT * 2U;
                    break;
                case RESOURCE_TYPE_FILE_DESCRIPTOR:
                    entry->limits[i].softLimit = RESOURCE_DEFAULT_FD_LIMIT;
                    entry->limits[i].hardLimit = RESOURCE_DEFAULT_FD_LIMIT * 2U;
                    break;
                default:
                    entry->limits[i].enabled = false;
                    break;
            }
        }
    }
    
    return E_OK;
}

Std_ReturnType ResourceManager_DisableMonitoring(ProcessIdType pid) {
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry == NULL) {
        return E_NOT_OK;
    }
    
    entry->monitoringEnabled = false;
    return E_OK;
}

bool ResourceManager_IsWithinLimits(ProcessIdType pid) {
    if (!g_initialized) {
        return false;
    }
    
    ResourceEntryType* entry = FindResourceEntry(pid);
    if (entry == NULL) {
        return true;  /* No monitoring = within limits */
    }
    
    /* Check all enabled limits */
    for (uint32_t i = 0U; i < 5U; i++) {
        if (!entry->limits[i].enabled) {
            continue;
        }
        
        uint64_t current = 0U;
        switch (i) {
            case RESOURCE_TYPE_CPU:
                current = entry->currentUsage.cpuPercentage;
                break;
            case RESOURCE_TYPE_MEMORY:
                current = entry->currentUsage.memoryBytes;
                break;
            case RESOURCE_TYPE_FILE_DESCRIPTOR:
                current = entry->currentUsage.fileDescriptors;
                break;
            default:
                continue;
        }
        
        if (current > entry->limits[i].hardLimit) {
            return false;
        }
    }
    
    return true;
}

Std_ReturnType ResourceManager_GetSystemUsage(ResourceUsageType* usage) {
    if (!g_initialized || usage == NULL) {
        return E_NOT_OK;
    }
    
    memset(usage, 0, sizeof(ResourceUsageType));
    
    /* Sum up usage from all monitored processes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (g_resourceTable[i].monitoringEnabled) {
            usage->cpuTimeUs += g_resourceTable[i].currentUsage.cpuTimeUs;
            usage->memoryBytes += g_resourceTable[i].currentUsage.memoryBytes;
            usage->fileDescriptors += g_resourceTable[i].currentUsage.fileDescriptors;
            usage->diskReadBytes += g_resourceTable[i].currentUsage.diskReadBytes;
            usage->diskWriteBytes += g_resourceTable[i].currentUsage.diskWriteBytes;
            usage->networkRxBytes += g_resourceTable[i].currentUsage.networkRxBytes;
            usage->networkTxBytes += g_resourceTable[i].currentUsage.networkTxBytes;
        }
    }
    
    return E_OK;
}

void ResourceManager_MainFunction(void) {
    if (!g_initialized) {
        return;
    }
    
    /* Would check time here: if (currentTime - g_lastMonitorTime < RESOURCE_MONITOR_INTERVAL_MS) return; */
    
    /* Update resource usage for all monitored processes */
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; i++) {
        if (!g_resourceTable[i].monitoringEnabled) {
            continue;
        }
        
        ResourceEntryType* entry = &g_resourceTable[i];
        
        /* Get current usage from platform */
        ResourceUsageType newUsage;
        if (PlatformGetResourceUsage(entry->pid, &newUsage) == E_OK) {
            entry->currentUsage = newUsage;
            UpdatePeakUsage(entry);
        }
        
        /* Check limits and take action if exceeded */
        if (!ResourceManager_IsWithinLimits(entry->pid)) {
            /* Log warning, could trigger process termination */
        }
    }
    
    /* Update last monitor time */
    /* g_lastMonitorTime = currentTime; */
}
