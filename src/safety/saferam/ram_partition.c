/******************************************************************************
 * @file    ram_partition.c
 * @brief   RAM Memory Partition Protection Implementation
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 ******************************************************************************/

#include "ram_partition.h"
#include <string.h>

/******************************************************************************
 * Private Constants
 ******************************************************************************/

#define RAM_PARTITION_MAGIC         0x52544E50U     /* "RTNP" */
#define RAM_PARTITION_GUARD_FILL    0xDEADBEEFU

/******************************************************************************
 * Private Variables
 ******************************************************************************/

static RamPartitionTableType g_partition_table;
static RamPartitionViolationType g_last_violation;
static RamPartitionViolationCallbackType g_violation_callback = NULL;
static boolean g_initialized = FALSE;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/

static Std_ReturnType ValidateAddressRange(uint32_t start, uint32_t size);
static void InitGuardZonePattern(uint32_t addr, uint32_t size);
static boolean CheckGuardZonePattern(uint32_t addr, uint32_t size);
static void ReportViolation(const RamPartitionViolationType *violation);

/******************************************************************************
 * Function Implementations - Initialization
 ******************************************************************************/

Std_ReturnType RamPartition_Init(void)
{
    uint8_t i;
    
    if (g_initialized) {
        return E_OK;
    }
    
    /* Clear partition table */
    memset(&g_partition_table, 0, sizeof(RamPartitionTableType));
    memset(&g_last_violation, 0, sizeof(RamPartitionViolationType));
    
    /* Initialize all partition entries */
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        g_partition_table.partitions[i].id = i;
        g_partition_table.partitions[i].state = PARTITION_STATE_UNINIT;
        g_partition_table.partitions[i].is_initialized = FALSE;
    }
    
    g_partition_table.count = 0;
    g_partition_table.initialized = TRUE;
    g_initialized = TRUE;
    
    return E_OK;
}

Std_ReturnType RamPartition_DeInit(void)
{
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    /* Unregister all partitions */
    while (g_partition_table.count > 0) {
        RamPartition_Unregister(g_partition_table.partitions[0].id);
    }
    
    g_partition_table.initialized = FALSE;
    g_initialized = FALSE;
    g_violation_callback = NULL;
    
    return E_OK;
}

Std_ReturnType RamPartition_Register(
    const RamPartitionConfigType *config,
    uint8_t *partition_id)
{
    uint8_t i;
    RamPartitionInfoType *partition;
    
    if (!g_initialized || config == NULL || partition_id == NULL) {
        return E_NOT_OK;
    }
    
    /* Validate configuration */
    if (RamPartition_ValidateConfig(config) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Check for overlap with existing partitions */
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        if (g_partition_table.partitions[i].is_initialized) {
            if (RamPartition_CheckOverlap(
                    config->start_addr, config->size,
                    g_partition_table.partitions[i].start_addr,
                    g_partition_table.partitions[i].size)) {
                return E_NOT_OK;  /* Overlap detected */
            }
        }
    }
    
    /* Find free partition slot */
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        if (!g_partition_table.partitions[i].is_initialized) {
            break;
        }
    }
    
    if (i >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;  /* No free slots */
    }
    
    /* Initialize partition */
    partition = &g_partition_table.partitions[i];
    partition->id = i;
    strncpy(partition->name, config->name, RAM_PARTITION_NAME_MAX_LEN - 1);
    partition->name[RAM_PARTITION_NAME_MAX_LEN - 1] = '\0';
    partition->type = config->type;
    partition->attributes = config->attributes;
    partition->safety_level = config->safety_level;
    partition->start_addr = config->start_addr;
    partition->end_addr = config->start_addr + config->size - 1;
    partition->size = config->size;
    partition->used_size = 0;
    partition->permissions = config->permissions;
    partition->owner_task_id = config->owner_task_id;
    partition->state = PARTITION_STATE_INIT;
    partition->is_initialized = TRUE;
    partition->access_count = 0;
    partition->violation_count = 0;
    partition->peak_usage = 0;
    
    /* Setup guard zones if enabled */
    if (config->enable_guard_zones) {
        partition->guard_bottom = config->start_addr - RAM_GUARD_ZONE_SIZE;
        partition->guard_top = partition->end_addr + 1;
        RamPartition_InitGuardZones(i);
    } else {
        partition->guard_bottom = 0;
        partition->guard_top = 0;
    }
    
    partition->state = PARTITION_STATE_ACTIVE;
    *partition_id = i;
    g_partition_table.count++;
    
    return E_OK;
}

Std_ReturnType RamPartition_Unregister(uint8_t partition_id)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    /* Clear partition data */
    memset(partition, 0, sizeof(RamPartitionInfoType));
    partition->id = partition_id;
    partition->state = PARTITION_STATE_UNINIT;
    partition->is_initialized = FALSE;
    
    g_partition_table.count--;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Partition Management
 ******************************************************************************/

Std_ReturnType RamPartition_GetInfo(
    uint8_t partition_id,
    RamPartitionInfoType *info)
{
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT || info == NULL) {
        return E_NOT_OK;
    }
    
    if (!g_partition_table.partitions[partition_id].is_initialized) {
        return E_NOT_OK;
    }
    
    memcpy(info, &g_partition_table.partitions[partition_id], 
           sizeof(RamPartitionInfoType));
    
    return E_OK;
}

Std_ReturnType RamPartition_SetPermissions(
    uint8_t partition_id,
    uint8_t permissions)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized || partition->state == PARTITION_STATE_LOCKED) {
        return E_NOT_OK;
    }
    
    partition->permissions = permissions;
    
    return E_OK;
}

Std_ReturnType RamPartition_Lock(uint8_t partition_id)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    partition->state = PARTITION_STATE_LOCKED;
    
    return E_OK;
}

Std_ReturnType RamPartition_Unlock(uint8_t partition_id)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    if (partition->state == PARTITION_STATE_VIOLATED) {
        return E_NOT_OK;  /* Violated partitions stay locked */
    }
    
    partition->state = PARTITION_STATE_ACTIVE;
    
    return E_OK;
}

Std_ReturnType RamPartition_UpdateUsage(uint8_t partition_id, uint32_t used_size)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    partition->used_size = used_size;
    
    if (used_size > partition->peak_usage) {
        partition->peak_usage = used_size;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Guard Zone Management
 ******************************************************************************/

Std_ReturnType RamPartition_InitGuardZones(uint8_t partition_id)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    /* Initialize bottom guard zone */
    if (partition->guard_bottom != 0) {
        InitGuardZonePattern(partition->guard_bottom, RAM_GUARD_ZONE_SIZE);
    }
    
    /* Initialize top guard zone */
    if (partition->guard_top != 0) {
        InitGuardZonePattern(partition->guard_top, RAM_GUARD_ZONE_SIZE);
    }
    
    return E_OK;
}

Std_ReturnType RamPartition_CheckGuardZones(uint8_t partition_id, boolean *result)
{
    RamPartitionInfoType *partition;
    boolean bottom_ok = TRUE;
    boolean top_ok = TRUE;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT || result == NULL) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    /* Check bottom guard zone */
    if (partition->guard_bottom != 0) {
        bottom_ok = CheckGuardZonePattern(partition->guard_bottom, RAM_GUARD_ZONE_SIZE);
    }
    
    /* Check top guard zone */
    if (partition->guard_top != 0) {
        top_ok = CheckGuardZonePattern(partition->guard_top, RAM_GUARD_ZONE_SIZE);
    }
    
    *result = bottom_ok && top_ok;
    
    if (!(*result)) {
        /* Report violation */
        RamPartitionViolationType violation;
        violation.partition_id = partition_id;
        violation.violation_addr = bottom_ok ? partition->guard_top : partition->guard_bottom;
        violation.violation_type = RAM_PARTITION_ERR_GUARD_VIOL;
        violation.access_type = 0;
        violation.timestamp = 0;  /* Should get from OS */
        violation.task_id = 0;
        
        partition->state = PARTITION_STATE_VIOLATED;
        partition->violation_count++;
        
        ReportViolation(&violation);
    }
    
    return E_OK;
}

uint32_t RamPartition_CheckAllGuardZones(void)
{
    uint8_t i;
    uint32_t violation_count = 0;
    boolean result;
    
    if (!g_initialized) {
        return 0;
    }
    
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        if (g_partition_table.partitions[i].is_initialized) {
            if (RamPartition_CheckGuardZones(i, &result) == E_OK) {
                if (!result) {
                    violation_count++;
                }
            }
        }
    }
    
    return violation_count;
}

/******************************************************************************
 * Function Implementations - Access Control
 ******************************************************************************/

Std_ReturnType RamPartition_CheckAccess(
    uint32_t addr,
    uint32_t size,
    uint8_t access_type,
    RamAccessCheckResultType *result)
{
    uint8_t i;
    RamPartitionInfoType *partition;
    boolean found = FALSE;
    
    if (!g_initialized || result == NULL || size == 0) {
        return E_NOT_OK;
    }
    
    result->allowed = FALSE;
    result->required_perm = access_type;
    result->actual_perm = 0;
    result->partition_id = 0xFF;
    
    /* Find partition containing the address range */
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        partition = &g_partition_table.partitions[i];
        
        if (partition->is_initialized) {
            if (addr >= partition->start_addr && 
                (addr + size - 1) <= partition->end_addr) {
                found = TRUE;
                result->partition_id = i;
                result->actual_perm = partition->permissions;
                break;
            }
        }
    }
    
    if (!found) {
        return E_OK;  /* Address not in any partition - allowed by default */
    }
    
    /* Check permissions */
    if ((access_type & RAM_PERM_READ) && !(partition->permissions & RAM_PERM_READ)) {
        return E_OK;  /* Read not allowed */
    }
    
    if ((access_type & RAM_PERM_WRITE) && !(partition->permissions & RAM_PERM_WRITE)) {
        return E_OK;  /* Write not allowed */
    }
    
    if ((access_type & RAM_PERM_EXEC) && !(partition->permissions & RAM_PERM_EXEC)) {
        return E_OK;  /* Execute not allowed */
    }
    
    result->allowed = TRUE;
    partition->access_count++;
    
    return E_OK;
}

Std_ReturnType RamPartition_FindByAddress(uint32_t addr, uint8_t *partition_id)
{
    uint8_t i;
    
    if (!g_initialized || partition_id == NULL) {
        return E_NOT_OK;
    }
    
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        if (g_partition_table.partitions[i].is_initialized) {
            if (addr >= g_partition_table.partitions[i].start_addr &&
                addr <= g_partition_table.partitions[i].end_addr) {
                *partition_id = i;
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;  /* Not found */
}

Std_ReturnType RamPartition_GetSafetyLevel(uint8_t partition_id, uint8_t *level)
{
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT || level == NULL) {
        return E_NOT_OK;
    }
    
    if (!g_partition_table.partitions[partition_id].is_initialized) {
        return E_NOT_OK;
    }
    
    *level = g_partition_table.partitions[partition_id].safety_level;
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - MPU/MMU Integration
 ******************************************************************************/

Std_ReturnType RamPartition_ConfigureMpu(
    uint8_t partition_id,
    const RamMpuRegionConfigType *mpu_config)
{
    /* MPU configuration is platform-specific */
    /* This is a stub for integration with platform-specific MPU driver */
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT || mpu_config == NULL) {
        return E_NOT_OK;
    }
    
    if (!g_partition_table.partitions[partition_id].is_initialized) {
        return E_NOT_OK;
    }
    
    /* Mark partition as MPU protected */
    g_partition_table.partitions[partition_id].attributes |= RAM_PARTITION_ATTR_MPU;
    
    /* Platform-specific MPU setup would go here */
    
    return E_OK;
}

Std_ReturnType RamPartition_EnableMpuProtection(uint8_t partition_id)
{
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    if (!g_partition_table.partitions[partition_id].is_initialized) {
        return E_NOT_OK;
    }
    
    if (!(g_partition_table.partitions[partition_id].attributes & RAM_PARTITION_ATTR_MPU)) {
        return E_NOT_OK;  /* Not configured for MPU */
    }
    
    /* Platform-specific enable would go here */
    
    return E_OK;
}

Std_ReturnType RamPartition_DisableMpuProtection(uint8_t partition_id)
{
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT) {
        return E_NOT_OK;
    }
    
    if (!g_partition_table.partitions[partition_id].is_initialized) {
        return E_NOT_OK;
    }
    
    /* Platform-specific disable would go here */
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Safety Checks
 ******************************************************************************/

Std_ReturnType RamPartition_CheckBounds(uint8_t partition_id, boolean *result)
{
    RamPartitionInfoType *partition;
    
    if (!g_initialized || partition_id >= RAM_PARTITION_MAX_COUNT || result == NULL) {
        return E_NOT_OK;
    }
    
    partition = &g_partition_table.partitions[partition_id];
    
    if (!partition->is_initialized) {
        return E_NOT_OK;
    }
    
    *result = TRUE;
    
    /* Check for overflow */
    if (partition->used_size > partition->size) {
        *result = FALSE;
        
        RamPartitionViolationType violation;
        violation.partition_id = partition_id;
        violation.violation_addr = partition->end_addr;
        violation.violation_type = RAM_PARTITION_ERR_OVERFLOW;
        violation.access_type = 0;
        violation.timestamp = 0;
        violation.task_id = 0;
        
        partition->state = PARTITION_STATE_VIOLATED;
        partition->violation_count++;
        
        ReportViolation(&violation);
    }
    
    return E_OK;
}

Std_ReturnType RamPartition_VerifyIntegrity(uint8_t partition_id)
{
    boolean guard_ok;
    boolean bounds_ok;
    
    if (RamPartition_CheckGuardZones(partition_id, &guard_ok) != E_OK) {
        return E_NOT_OK;
    }
    
    if (RamPartition_CheckBounds(partition_id, &bounds_ok) != E_OK) {
        return E_NOT_OK;
    }
    
    if (!guard_ok || !bounds_ok) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

uint32_t RamPartition_FullCheck(void)
{
    uint8_t i;
    uint32_t error_count = 0;
    
    if (!g_initialized) {
        return 0;
    }
    
    /* Check all guard zones */
    error_count += RamPartition_CheckAllGuardZones();
    
    /* Check bounds for all partitions */
    for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
        if (g_partition_table.partitions[i].is_initialized) {
            boolean result;
            if (RamPartition_CheckBounds(i, &result) == E_OK) {
                if (!result) {
                    error_count++;
                }
            }
        }
    }
    
    return error_count;
}

/******************************************************************************
 * Function Implementations - Violation Handling
 ******************************************************************************/

void RamPartition_SetViolationCallback(RamPartitionViolationCallbackType callback)
{
    g_violation_callback = callback;
}

void RamPartition_HandleViolation(const RamPartitionViolationType *violation)
{
    if (violation == NULL) {
        return;
    }
    
    /* Store last violation */
    memcpy(&g_last_violation, violation, sizeof(RamPartitionViolationType));
    
    /* Set partition state to violated */
    if (violation->partition_id < RAM_PARTITION_MAX_COUNT) {
        g_partition_table.partitions[violation->partition_id].state = 
            PARTITION_STATE_VIOLATED;
    }
    
    /* Call registered callback */
    if (g_violation_callback != NULL) {
        g_violation_callback(violation);
    }
}

Std_ReturnType RamPartition_GetLastViolation(RamPartitionViolationType *violation)
{
    if (violation == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(violation, &g_last_violation, sizeof(RamPartitionViolationType));
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Statistics and Info
 ******************************************************************************/

Std_ReturnType RamPartition_GetTable(RamPartitionTableType *table)
{
    if (!g_initialized || table == NULL) {
        return E_NOT_OK;
    }
    
    memcpy(table, &g_partition_table, sizeof(RamPartitionTableType));
    
    return E_OK;
}

uint8_t RamPartition_GetCount(void)
{
    if (!g_initialized) {
        return 0;
    }
    
    return g_partition_table.count;
}

Std_ReturnType RamPartition_ResetStats(uint8_t partition_id)
{
    uint8_t i;
    
    if (!g_initialized) {
        return E_NOT_OK;
    }
    
    if (partition_id == 0xFF) {
        /* Reset all partitions */
        for (i = 0; i < RAM_PARTITION_MAX_COUNT; i++) {
            if (g_partition_table.partitions[i].is_initialized) {
                g_partition_table.partitions[i].access_count = 0;
                g_partition_table.partitions[i].violation_count = 0;
                g_partition_table.partitions[i].peak_usage = 
                    g_partition_table.partitions[i].used_size;
            }
        }
    } else {
        if (partition_id >= RAM_PARTITION_MAX_COUNT) {
            return E_NOT_OK;
        }
        
        if (!g_partition_table.partitions[partition_id].is_initialized) {
            return E_NOT_OK;
        }
        
        g_partition_table.partitions[partition_id].access_count = 0;
        g_partition_table.partitions[partition_id].violation_count = 0;
        g_partition_table.partitions[partition_id].peak_usage = 
            g_partition_table.partitions[partition_id].used_size;
    }
    
    return E_OK;
}

/******************************************************************************
 * Function Implementations - Utility Functions
 ******************************************************************************/

Std_ReturnType RamPartition_ValidateConfig(const RamPartitionConfigType *config)
{
    if (config == NULL) {
        return E_NOT_OK;
    }
    
    /* Validate name */
    if (config->name[0] == '\0') {
        return E_NOT_OK;
    }
    
    /* Validate size */
    if (config->size == 0) {
        return E_NOT_OK;
    }
    
    /* Validate address range */
    if (ValidateAddressRange(config->start_addr, config->size) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Validate safety level */
    if (config->safety_level > RAM_PARTITION_LEVEL_RED) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

boolean RamPartition_CheckOverlap(
    uint32_t addr1,
    uint32_t size1,
    uint32_t addr2,
    uint32_t size2)
{
    uint32_t end1 = addr1 + size1 - 1;
    uint32_t end2 = addr2 + size2 - 1;
    
    /* Check if ranges overlap */
    return (addr1 <= end2) && (addr2 <= end1);
}

void RamPartition_GetVersionInfo(Std_VersionInfoType *version)
{
    if (version != NULL) {
        version->vendorID = 0;
        version->moduleID = 0;
        version->sw_major_version = RAM_PARTITION_VERSION_MAJOR;
        version->sw_minor_version = RAM_PARTITION_VERSION_MINOR;
        version->sw_patch_version = RAM_PARTITION_VERSION_PATCH;
    }
}

/******************************************************************************
 * Private Functions
 ******************************************************************************/

static Std_ReturnType ValidateAddressRange(uint32_t start, uint32_t size)
{
    uint32_t end;
    
    /* Check for overflow */
    if (start == 0 && size > 0) {
        return E_NOT_OK;  /* Null pointer not allowed */
    }
    
    end = start + size;
    if (end < start) {
        return E_NOT_OK;  /* Overflow */
    }
    
    return E_OK;
}

static void InitGuardZonePattern(uint32_t addr, uint32_t size)
{
    uint32_t i;
    uint32_t *ptr = (uint32_t *)addr;
    uint32_t word_count = size / sizeof(uint32_t);
    
    for (i = 0; i < word_count; i++) {
        ptr[i] = RAM_PARTITION_GUARD_FILL;
    }
}

static boolean CheckGuardZonePattern(uint32_t addr, uint32_t size)
{
    uint32_t i;
    uint32_t *ptr = (uint32_t *)addr;
    uint32_t word_count = size / sizeof(uint32_t);
    
    for (i = 0; i < word_count; i++) {
        if (ptr[i] != RAM_PARTITION_GUARD_FILL) {
            return FALSE;
        }
    }
    
    return TRUE;
}

static void ReportViolation(const RamPartitionViolationType *violation)
{
    if (violation == NULL) {
        return;
    }
    
    /* Store as last violation */
    memcpy(&g_last_violation, violation, sizeof(RamPartitionViolationType));
    
    /* Call callback if registered */
    if (g_violation_callback != NULL) {
        g_violation_callback(violation);
    }
}
