/******************************************************************************
 * @file    resource_manager.h
 * @brief   Resource Manager Header
 *
 * AUTOSAR Adaptive Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef EXEC_RESOURCE_MANAGER_H
#define EXEC_RESOURCE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "autosar_types.h"
#include "autosar_errors.h"
#include "process_manager.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

#define RESOURCE_MONITOR_INTERVAL_MS    1000U
#define RESOURCE_DEFAULT_CPU_LIMIT      80U     /* Percentage */
#define RESOURCE_DEFAULT_MEMORY_LIMIT   (64U * 1024U * 1024U)  /* 64MB */
#define RESOURCE_DEFAULT_FD_LIMIT       1024U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Resource types */
typedef enum {
    RESOURCE_TYPE_CPU = 0,
    RESOURCE_TYPE_MEMORY,
    RESOURCE_TYPE_FILE_DESCRIPTOR,
    RESOURCE_TYPE_DISK_IO,
    RESOURCE_TYPE_NETWORK_IO
} ResourceTypeType;

/* Resource limits */
typedef struct {
    ResourceTypeType type;
    uint64_t softLimit;
    uint64_t hardLimit;
    bool enabled;
} ResourceLimitType;

/* Resource usage statistics */
typedef struct {
    uint64_t cpuTimeUs;
    uint64_t cpuPercentage;     /* 0-100 */
    uint64_t memoryBytes;
    uint64_t memoryPercentage;  /* 0-100 */
    uint32_t fileDescriptors;
    uint64_t diskReadBytes;
    uint64_t diskWriteBytes;
    uint64_t networkRxBytes;
    uint64_t networkTxBytes;
} ResourceUsageType;

/* Resource monitoring entry */
typedef struct {
    ProcessIdType pid;
    ResourceLimitType limits[5];  /* One for each resource type */
    ResourceUsageType currentUsage;
    ResourceUsageType peakUsage;
    bool monitoringEnabled;
    void* platformHandle;
} ResourceEntryType;

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/* Initialize resource manager */
Std_ReturnType ResourceManager_Init(void);

/* Deinitialize resource manager */
Std_ReturnType ResourceManager_Deinit(void);

/* Check if initialized */
bool ResourceManager_IsInitialized(void);

/* Set resource limit for a process */
Std_ReturnType ResourceManager_SetLimit(ProcessIdType pid, const ResourceLimitType* limit);

/* Get resource limit for a process */
Std_ReturnType ResourceManager_GetLimit(ProcessIdType pid, ResourceTypeType type, 
                                         ResourceLimitType* limit);

/* Get current resource usage for a process */
Std_ReturnType ResourceManager_GetUsage(ProcessIdType pid, ResourceUsageType* usage);

/* Get peak resource usage for a process */
Std_ReturnType ResourceManager_GetPeakUsage(ProcessIdType pid, ResourceUsageType* usage);

/* Reset peak usage statistics */
Std_ReturnType ResourceManager_ResetPeakUsage(ProcessIdType pid);

/* Enable resource monitoring for a process */
Std_ReturnType ResourceManager_EnableMonitoring(ProcessIdType pid);

/* Disable resource monitoring for a process */
Std_ReturnType ResourceManager_DisableMonitoring(ProcessIdType pid);

/* Check if process is within resource limits */
bool ResourceManager_IsWithinLimits(ProcessIdType pid);

/* Get resource usage for all processes */
Std_ReturnType ResourceManager_GetSystemUsage(ResourceUsageType* usage);

/* Resource manager main function (call periodically) */
void ResourceManager_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* EXEC_RESOURCE_MANAGER_H */
