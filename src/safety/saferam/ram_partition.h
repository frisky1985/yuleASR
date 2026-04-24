/******************************************************************************
 * @file    ram_partition.h
 * @brief   RAM Memory Partition Protection Module
 *
 * AUTOSAR R22-11 compliant
 * ISO 26262 ASIL-D Safety Level
 *
 * This module provides RAM memory partition management with:
 * - Stack/Heap/Static/BSS partition isolation
 * - Inter-partition guard zones
 * - Red/Yellow/Green partition classification
 * - MPU/MMU integration support
 * - Partition permission control
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef RAM_PARTITION_H
#define RAM_PARTITION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/autosar_types.h"
#include "../../common/autosar_errors.h"

/******************************************************************************
 * Constants and Macros
 ******************************************************************************/

/* Module version */
#define RAM_PARTITION_VERSION_MAJOR     1U
#define RAM_PARTITION_VERSION_MINOR     0U
#define RAM_PARTITION_VERSION_PATCH     0U

/* Maximum partitions */
#define RAM_PARTITION_MAX_COUNT         16U
#define RAM_PARTITION_NAME_MAX_LEN      32U

/* Guard zone configuration */
#define RAM_GUARD_ZONE_SIZE             256U    /* Minimum 256 bytes */
#define RAM_GUARD_ZONE_PATTERN          0xDEADBEEFU

/* Partition safety levels (Red/Yellow/Green) */
#define RAM_PARTITION_LEVEL_GREEN       0U      /* Safe, unrestricted */
#define RAM_PARTITION_LEVEL_YELLOW      1U      /* Caution, monitored */
#define RAM_PARTITION_LEVEL_RED         2U      /* Critical, restricted */

/* Partition types */
#define RAM_PARTITION_TYPE_STACK        0x01U   /* Task stack */
#define RAM_PARTITION_TYPE_HEAP         0x02U   /* Dynamic heap */
#define RAM_PARTITION_TYPE_STATIC       0x04U   /* Static data */
#define RAM_PARTITION_TYPE_BSS          0x08U   /* BSS/uninitialized */
#define RAM_PARTITION_TYPE_CODE         0x10U   /* Code/ROM */
#define RAM_PARTITION_TYPE_DMA          0x20U   /* DMA buffer */
#define RAM_PARTITION_TYPE_DEVICE       0x40U   /* Device memory */

/* Partition attributes */
#define RAM_PARTITION_ATTR_READ         0x0001U /* Read access */
#define RAM_PARTITION_ATTR_WRITE        0x0002U /* Write access */
#define RAM_PARTITION_ATTR_EXEC         0x0004U /* Execute access */
#define RAM_PARTITION_ATTR_CACHEABLE    0x0008U /* Cacheable */
#define RAM_PARTITION_ATTR_BUFFERABLE   0x0010U /* Bufferable */
#define RAM_PARTITION_ATTR_SHAREABLE    0x0020U /* Shareable */
#define RAM_PARTITION_ATTR_ECC          0x0040U /* ECC protected */
#define RAM_PARTITION_ATTR_MPU          0x0080U /* MPU protected */

/* Permission flags */
#define RAM_PERM_NONE                   0x00U
#define RAM_PERM_READ                   0x01U
#define RAM_PERM_WRITE                  0x02U
#define RAM_PERM_EXEC                   0x04U
#define RAM_PERM_USER                   0x10U
#define RAM_PERM_PRIVILEGED             0x20U

/* Error codes */
#define RAM_PARTITION_ERR_NONE          0x00U
#define RAM_PARTITION_ERR_INVALID       0x01U
#define RAM_PARTITION_ERR_OVERLAP       0x02U
#define RAM_PARTITION_ERR_GUARD_VIOL    0x03U
#define RAM_PARTITION_ERR_ACCESS_DENY   0x04U
#define RAM_PARTITION_ERR_OVERFLOW      0x05U
#define RAM_PARTITION_ERR_UNDERFLOW     0x06U

/******************************************************************************
 * Type Definitions
 ******************************************************************************/

/* Partition state */
typedef enum {
    PARTITION_STATE_UNINIT = 0,
    PARTITION_STATE_INIT,
    PARTITION_STATE_ACTIVE,
    PARTITION_STATE_VIOLATED,
    PARTITION_STATE_LOCKED
} RamPartitionStateType;

/* Partition information */
typedef struct {
    uint8_t id;                                 /* Partition ID */
    char name[RAM_PARTITION_NAME_MAX_LEN];      /* Partition name */
    uint32_t type;                              /* Partition type */
    uint32_t attributes;                        /* Attributes */
    uint8_t safety_level;                       /* Red/Yellow/Green */
    
    /* Memory boundaries */
    uint32_t start_addr;                        /* Start address */
    uint32_t end_addr;                          /* End address */
    uint32_t size;                              /* Total size */
    uint32_t used_size;                         /* Used size */
    
    /* Guard zones */
    uint32_t guard_bottom;                      /* Bottom guard address */
    uint32_t guard_top;                         /* Top guard address */
    
    /* Permission */
    uint8_t permissions;                        /* Access permissions */
    uint8_t owner_task_id;                      /* Owning task ID */
    
    /* State */
    RamPartitionStateType state;
    boolean is_initialized;
    
    /* Statistics */
    uint32_t access_count;
    uint32_t violation_count;
    uint32_t peak_usage;
} RamPartitionInfoType;

/* Partition configuration */
typedef struct {
    char name[RAM_PARTITION_NAME_MAX_LEN];
    uint32_t type;
    uint32_t attributes;
    uint8_t safety_level;
    uint32_t start_addr;
    uint32_t size;
    uint8_t permissions;
    uint8_t owner_task_id;
    boolean enable_guard_zones;
} RamPartitionConfigType;

/* Partition violation info */
typedef struct {
    uint8_t partition_id;
    uint32_t violation_addr;
    uint8_t violation_type;
    uint8_t access_type;
    uint32_t timestamp;
    uint32_t task_id;
} RamPartitionViolationType;

/* Partition table */
typedef struct {
    uint8_t count;
    RamPartitionInfoType partitions[RAM_PARTITION_MAX_COUNT];
    boolean initialized;
} RamPartitionTableType;

/* MPU region configuration (for integration) */
typedef struct {
    uint32_t base_addr;
    uint32_t size;
    uint32_t attributes;
    uint8_t permissions;
    uint8_t region_num;
    boolean enable;
} RamMpuRegionConfigType;

/* Access check result */
typedef struct {
    boolean allowed;
    uint8_t required_perm;
    uint8_t actual_perm;
    uint8_t partition_id;
} RamAccessCheckResultType;

/* Violation callback type */
typedef void (*RamPartitionViolationCallbackType)(
    const RamPartitionViolationType *violation
);

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief Initialize RAM partition module
 * @return E_OK on success
 */
Std_ReturnType RamPartition_Init(void);

/**
 * @brief Deinitialize RAM partition module
 * @return E_OK on success
 */
Std_ReturnType RamPartition_DeInit(void);

/**
 * @brief Register a new memory partition
 * @param config Partition configuration
 * @param partition_id Pointer to store assigned partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_Register(
    const RamPartitionConfigType *config,
    uint8_t *partition_id
);

/**
 * @brief Unregister a partition
 * @param partition_id Partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_Unregister(uint8_t partition_id);

/******************************************************************************
 * Function Prototypes - Partition Management
 ******************************************************************************/

/**
 * @brief Get partition information
 * @param partition_id Partition ID
 * @param info Pointer to store information
 * @return E_OK on success
 */
Std_ReturnType RamPartition_GetInfo(
    uint8_t partition_id,
    RamPartitionInfoType *info
);

/**
 * @brief Update partition permissions
 * @param partition_id Partition ID
 * @param permissions New permissions
 * @return E_OK on success
 */
Std_ReturnType RamPartition_SetPermissions(
    uint8_t partition_id,
    uint8_t permissions
);

/**
 * @brief Lock partition (prevent modifications)
 * @param partition_id Partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_Lock(uint8_t partition_id);

/**
 * @brief Unlock partition
 * @param partition_id Partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_Unlock(uint8_t partition_id);

/**
 * @brief Update partition usage statistics
 * @param partition_id Partition ID
 * @param used_size Current used size
 * @return E_OK on success
 */
Std_ReturnType RamPartition_UpdateUsage(uint8_t partition_id, uint32_t used_size);

/******************************************************************************
 * Function Prototypes - Guard Zone Management
 ******************************************************************************/

/**
 * @brief Initialize guard zones for a partition
 * @param partition_id Partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_InitGuardZones(uint8_t partition_id);

/**
 * @brief Verify guard zone integrity
 * @param partition_id Partition ID
 * @param result Pointer to store result (TRUE if intact)
 * @return E_OK on success
 */
Std_ReturnType RamPartition_CheckGuardZones(
    uint8_t partition_id,
    boolean *result
);

/**
 * @brief Check all partition guard zones
 * @return Number of violations found
 */
uint32_t RamPartition_CheckAllGuardZones(void);

/******************************************************************************
 * Function Prototypes - Access Control
 ******************************************************************************/

/**
 * @brief Check if memory access is allowed
 * @param addr Memory address
 * @param size Access size
 * @param access_type Access type (read/write/exec)
 * @param result Pointer to store check result
 * @return E_OK on success
 */
Std_ReturnType RamPartition_CheckAccess(
    uint32_t addr,
    uint32_t size,
    uint8_t access_type,
    RamAccessCheckResultType *result
);

/**
 * @brief Find partition containing address
 * @param addr Memory address
 * @param partition_id Pointer to store partition ID
 * @return E_OK if found
 */
Std_ReturnType RamPartition_FindByAddress(
    uint32_t addr,
    uint8_t *partition_id
);

/**
 * @brief Get partition safety level
 * @param partition_id Partition ID
 * @param level Pointer to store level
 * @return E_OK on success
 */
Std_ReturnType RamPartition_GetSafetyLevel(
    uint8_t partition_id,
    uint8_t *level
);

/******************************************************************************
 * Function Prototypes - MPU/MMU Integration
 ******************************************************************************/

/**
 * @brief Configure MPU region for partition
 * @param partition_id Partition ID
 * @param mpu_config MPU region configuration
 * @return E_OK on success
 */
Std_ReturnType RamPartition_ConfigureMpu(
    uint8_t partition_id,
    const RamMpuRegionConfigType *mpu_config
);

/**
 * @brief Enable MPU protection for partition
 * @param partition_id Partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_EnableMpuProtection(uint8_t partition_id);

/**
 * @brief Disable MPU protection for partition
 * @param partition_id Partition ID
 * @return E_OK on success
 */
Std_ReturnType RamPartition_DisableMpuProtection(uint8_t partition_id);

/******************************************************************************
 * Function Prototypes - Safety Checks
 ******************************************************************************/

/**
 * @brief Check partition for overflow/underflow
 * @param partition_id Partition ID
 * @param result Pointer to store result
 * @return E_OK on success
 */
Std_ReturnType RamPartition_CheckBounds(
    uint8_t partition_id,
    boolean *result
);

/**
 * @brief Verify partition integrity
 * @param partition_id Partition ID
 * @return E_OK if intact
 */
Std_ReturnType RamPartition_VerifyIntegrity(uint8_t partition_id);

/**
 * @brief Perform full partition check
 * @return Number of errors found
 */
uint32_t RamPartition_FullCheck(void);

/******************************************************************************
 * Function Prototypes - Violation Handling
 ******************************************************************************/

/**
 * @brief Register violation callback
 * @param callback Callback function
 */
void RamPartition_SetViolationCallback(RamPartitionViolationCallbackType callback);

/**
 * @brief Handle partition violation
 * @param violation Violation information
 */
void RamPartition_HandleViolation(const RamPartitionViolationType *violation);

/**
 * @brief Get last violation info
 * @param violation Pointer to store violation
 * @return E_OK if violation occurred
 */
Std_ReturnType RamPartition_GetLastViolation(RamPartitionViolationType *violation);

/******************************************************************************
 * Function Prototypes - Statistics and Info
 ******************************************************************************/

/**
 * @brief Get partition table information
 * @param table Pointer to store table
 * @return E_OK on success
 */
Std_ReturnType RamPartition_GetTable(RamPartitionTableType *table);

/**
 * @brief Get partition count
 * @return Number of registered partitions
 */
uint8_t RamPartition_GetCount(void);

/**
 * @brief Reset partition statistics
 * @param partition_id Partition ID (0xFF for all)
 * @return E_OK on success
 */
Std_ReturnType RamPartition_ResetStats(uint8_t partition_id);

/******************************************************************************
 * Function Prototypes - Utility Functions
 ******************************************************************************/

/**
 * @brief Validate partition configuration
 * @param config Configuration to validate
 * @return E_OK if valid
 */
Std_ReturnType RamPartition_ValidateConfig(const RamPartitionConfigType *config);

/**
 * @brief Check if partitions overlap
 * @param addr1 First address range start
 * @param size1 First address range size
 * @param addr2 Second address range start
 * @param size2 Second address range size
 * @return TRUE if overlap
 */
boolean RamPartition_CheckOverlap(
    uint32_t addr1,
    uint32_t size1,
    uint32_t addr2,
    uint32_t size2
);

/**
 * @brief Get version information
 * @param version Pointer to version info
 */
void RamPartition_GetVersionInfo(Std_VersionInfoType *version);

#ifdef __cplusplus
}
#endif

#endif /* RAM_PARTITION_H */
