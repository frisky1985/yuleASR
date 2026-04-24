/**
 * @file nvm_redundancy.h
 * @brief NvM Redundancy Management Header
 * @version 1.0.0
 * @date 2025
 * 
 * Redundancy management including:
 * - Dual block redundancy
 * - Checksum-based redundancy
 * - Automatic data recovery
 * - Configuration data partitioning
 */

#ifndef NVM_REDUNDANCY_H
#define NVM_REDUNDANCY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nvm_types.h"

/*============================================================================*
 * Redundancy Configuration
 *============================================================================*/
#define NVM_REDUNDANCY_MAX_PAIRS    32u
#define NVM_REDUNDANCY_MIRROR_SIZE  512u  /* Max size for mirrored blocks */

/*============================================================================*
 * Redundancy Pair Structure
 *============================================================================*/
typedef struct {
    uint16_t primaryBlockId;        /* Primary block ID */
    uint16_t backupBlockId;         /* Backup (redundant) block ID */
    bool enabled;                   /* Redundancy enabled */
    bool autoRecover;               /* Automatic recovery enabled */
    uint32_t checksumPrimary;       /* Last checksum of primary */
    uint32_t checksumBackup;        /* Last checksum of backup */
    uint32_t syncCount;             /* Number of synchronizations */
} Nvm_RedundancyPair_t;

/*============================================================================*
 * Partition Configuration
 *============================================================================*/
typedef enum {
    NVM_PART_CONFIG_A,              /* Configuration partition A */
    NVM_PART_CONFIG_B,              /* Configuration partition B */
    NVM_PART_RUNTIME,               /* Runtime data partition */
    NVM_PART_DIAGNOSTIC             /* Diagnostic data partition */
} Nvm_PartitionId_t;

typedef struct {
    Nvm_PartitionId_t id;
    uint32_t startAddress;
    uint32_t size;
    bool redundant;
    uint32_t checksum;
    bool valid;
} Nvm_PartitionInfo_t;

/*============================================================================*
 * Redundancy Manager Context
 *============================================================================*/
typedef struct {
    bool initialized;
    Nvm_RedundancyPair_t pairs[NVM_REDUNDANCY_MAX_PAIRS];
    uint32_t activePairs;
    uint32_t recoveryCount;
    uint32_t failedRecoveries;
} Nvm_RedundancyContext_t;

/*============================================================================*
 * Redundancy API
 *============================================================================*/

/**
 * @brief Initialize redundancy manager
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Init(void);

/**
 * @brief Deinitialize redundancy manager
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Deinit(void);

/**
 * @brief Configure redundancy pair
 * @param primaryBlockId Primary block ID
 * @param backupBlockId Backup block ID
 * @param autoRecover Enable automatic recovery
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_ConfigurePair(
    uint16_t primaryBlockId,
    uint16_t backupBlockId,
    bool autoRecover
);

/**
 * @brief Remove redundancy pair
 * @param primaryBlockId Primary block ID of the pair
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_RemovePair(uint16_t primaryBlockId);

/**
 * @brief Enable redundancy for block
 * @param blockId Block ID
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Enable(uint16_t blockId);

/**
 * @brief Disable redundancy for block
 * @param blockId Block ID
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Disable(uint16_t blockId);

/**
 * @brief Write data with redundancy
 * @param blockId Block ID
 * @param data Data to write
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Write(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Read data with redundancy check
 * @param blockId Block ID
 * @param data Buffer to read into
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Read(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length
);

/**
 * @brief Verify redundancy pair consistency
 * @param blockId Primary block ID
 * @return NVM_OK if consistent, NVM_E_REDUNDANCY_FAILED otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Verify(uint16_t blockId);

/**
 * @brief Synchronize redundancy pair
 * @param blockId Primary block ID
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Sync(uint16_t blockId);

/**
 * @brief Attempt to recover data from backup
 * @param blockId Primary block ID
 * @return NVM_OK on success, NVM_E_RECOVERY_FAILED otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_Recover(uint16_t blockId);

/**
 * @brief Get redundancy pair status
 * @param blockId Primary block ID
 * @param pair Pointer to store pair info
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_GetPairStatus(
    uint16_t blockId,
    Nvm_RedundancyPair_t* pair
);

/**
 * @brief Calculate checksum for data
 * @param data Data buffer
 * @param length Data length
 * @return Checksum value
 */
uint32_t Nvm_Redundancy_CalculateChecksum(
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Validate checksum
 * @param data Data buffer
 * @param length Data length
 * @param expectedChecksum Expected checksum
 * @return NVM_OK if valid, NVM_E_CRC_FAILED otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_ValidateChecksum(
    const uint8_t* data,
    uint32_t length,
    uint32_t expectedChecksum
);

/**
 * @brief Initialize configuration partition
 * @param partition Partition ID
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_InitPartition(Nvm_PartitionId_t partition);

/**
 * @brief Get partition information
 * @param partition Partition ID
 * @param info Pointer to store info
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_GetPartitionInfo(
    Nvm_PartitionId_t partition,
    Nvm_PartitionInfo_t* info
);

/**
 * @brief Validate configuration partition
 * @param partition Partition ID
 * @return NVM_OK if valid, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_ValidatePartition(Nvm_PartitionId_t partition);

/**
 * @brief Copy configuration partition
 * @param source Source partition
 * @param destination Destination partition
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_CopyPartition(
    Nvm_PartitionId_t source,
    Nvm_PartitionId_t destination
);

/**
 * @brief Swap active configuration partition
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_SwapPartition(void);

/**
 * @brief Get active configuration partition
 * @return Active partition ID
 */
Nvm_PartitionId_t Nvm_Redundancy_GetActivePartition(void);

/**
 * @brief Get redundancy statistics
 * @param recoveryCount Pointer to store recovery count
 * @param failedCount Pointer to store failed recovery count
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Redundancy_GetStats(
    uint32_t* recoveryCount,
    uint32_t* failedCount
);

/**
 * @brief Get redundancy context (internal use)
 * @return Pointer to context
 */
Nvm_RedundancyContext_t* Nvm_Redundancy_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* NVM_REDUNDANCY_H */
