/**
 * @file fee.h
 * @brief Fee (Flash EEPROM Emulation) Main Header
 * @version 1.0.0
 * @date 2026
 *
 * AUTOSAR MCAL Fee Module
 * Compliant with AUTOSAR R22-11 MCAL Specification
 *
 * Features:
 * - Virtual EEPROM block management
 * - Wear leveling support
 * - Garbage collection
 * - Power-loss safe operations
 * - MISRA C:2012 compliant
 */

#ifndef FEE_H
#define FEE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fee_types.h"

/*============================================================================*
 * External Configuration
 *============================================================================*/
extern const Fee_ConfigType Fee_Config;

/*============================================================================*
 * Initialization API
 *============================================================================*/

/**
 * @brief Initialize the Fee module
 *
 * Initializes the EEPROM emulation layer, scans existing blocks,
 * and prepares the module for operation.
 *
 * @param config Pointer to configuration structure (NULL for default)
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_Init(const Fee_ConfigType* config);

/**
 * @brief Deinitialize the Fee module
 *
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_Deinit(void);

/**
 * @brief Get Fee module status
 *
 * @return Current module status
 */
Fee_ModuleStatus_t Fee_GetStatus(void);

/**
 * @brief Check if Fee module is initialized
 *
 * @return true if initialized, false otherwise
 */
bool Fee_IsInitialized(void);

/**
 * @brief Read the block status
 *
 * @param blockNumber Block number
 * @param status Pointer to store block status
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetBlockStatus(
    uint16_t blockNumber,
    Fee_BlockStatus_t* status
);

/*============================================================================*
 * Synchronous API
 *============================================================================*/

/**
 * @brief Read data from logical block (synchronous)
 *
 * @param blockNumber Block number
 * @param blockOffset Offset within block
 * @param dataBufferPtr Buffer to read into
 * @param length Number of bytes to read
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_Read(
    uint16_t blockNumber,
    uint16_t blockOffset,
    uint8_t* dataBufferPtr,
    uint16_t length
);

/**
 * @brief Write data to logical block (synchronous)
 *
 * Writes data to the specified logical block.
 * The actual write to flash may be deferred.
 *
 * @param blockNumber Block number
 * @param dataBufferPtr Data to write
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_Write(
    uint16_t blockNumber,
    const uint8_t* dataBufferPtr
);

/**
 * @brief Erase logical block immediately (synchronous)
 *
 * @param blockNumber Block number
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_EraseImmediateBlock(uint16_t blockNumber);

/**
 * @brief Invalidate logical block (mark as deleted)
 *
 * @param blockNumber Block number
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_InvalidateBlock(uint16_t blockNumber);

/*============================================================================*
 * Asynchronous API
 *============================================================================*/

/**
 * @brief Read data from logical block (asynchronous)
 *
 * @param blockNumber Block number
 * @param blockOffset Offset within block
 * @param dataBufferPtr Buffer to read into
 * @param length Number of bytes to read
 * @return FEE_OK if job started, FEE_E_BUSY if another job running
 */
Fee_ErrorCode_t Fee_Read_Async(
    uint16_t blockNumber,
    uint16_t blockOffset,
    uint8_t* dataBufferPtr,
    uint16_t length
);

/**
 * @brief Write data to logical block (asynchronous)
 *
 * @param blockNumber Block number
 * @param dataBufferPtr Data to write
 * @return FEE_OK if job started, FEE_E_BUSY if another job running
 */
Fee_ErrorCode_t Fee_Write_Async(
    uint16_t blockNumber,
    const uint8_t* dataBufferPtr
);

/**
 * @brief Cancel pending asynchronous operation
 *
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_Cancel(void);

/**
 * @brief Get current job status
 *
 * @return Current job status
 */
Fee_JobStatus_t Fee_GetJobStatus(void);

/**
 * @brief Get result of last operation
 *
 * @return Result code of last operation
 */
Fee_ErrorCode_t Fee_GetJobResult(void);

/*============================================================================*
 * Job Processing
 *============================================================================*/

/**
 * @brief Main function for job processing
 *
 * Must be called periodically to process pending jobs,
 * garbage collection, and write queue.
 */
void Fee_MainFunction(void);

/*============================================================================*
 * Block Management API
 *============================================================================*/

/**
 * @brief Get block size
 *
 * @param blockNumber Block number
 * @param size Pointer to store block size
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetBlockSize(uint16_t blockNumber, uint16_t* size);

/**
 * @brief Get block write count
 *
 * @param blockNumber Block number
 * @param count Pointer to store write count
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetBlockWriteCount(uint16_t blockNumber, uint32_t* count);

/**
 * @brief Get block flash address
 *
 * @param blockNumber Block number
 * @param address Pointer to store flash address
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetBlockAddress(uint16_t blockNumber, uint32_t* address);

/**
 * @brief Check if block is available (can be written)
 *
 * @param blockNumber Block number
 * @return true if available, false otherwise
 */
bool Fee_BlockIsAvailable(uint16_t blockNumber);

/*============================================================================*
 * Cluster/Space Management API
 *============================================================================*/

/**
 * @brief Get number of free bytes in active cluster
 *
 * @return Number of free bytes
 */
uint32_t Fee_GetFreeSpace(void);

/**
 * @brief Get used space percentage
 *
 * @return Used space percentage (0-100)
 */
uint32_t Fee_GetUsedSpacePercent(void);

/**
 * @brief Force garbage collection
 *
 * Triggers GC regardless of threshold.
 *
 * @return FEE_OK if GC started, FEE_E_GC_BUSY if already running
 */
Fee_ErrorCode_t Fee_ForceGarbageCollection(void);

/**
 * @brief Get cluster status
 *
 * @param clusterIndex Cluster index
 * @param status Pointer to store cluster status
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetClusterStatus(
    uint32_t clusterIndex,
    Fee_ClusterStatus_t* status
);

/**
 * @brief Swap clusters (trigger cluster exchange)
 *
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_SwapClusters(void);

/*============================================================================*
 * Write Queue Management
 *============================================================================*/

/**
 * @brief Check if write queue is empty
 *
 * @return true if empty, false otherwise
 */
bool Fee_WriteQueueIsEmpty(void);

/**
 * @brief Check if write queue is full
 *
 * @return true if full, false otherwise
 */
bool Fee_WriteQueueIsFull(void);

/**
 * @brief Get write queue count
 *
 * @return Number of pending writes in queue
 */
uint32_t Fee_GetWriteQueueCount(void);

/**
 * @brief Flush write queue (write all pending)
 *
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_FlushWriteQueue(void);

/**
 * @brief Clear write queue (discard pending)
 *
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_ClearWriteQueue(void);

/*============================================================================*
 * Wear Leveling API
 *============================================================================*/

/**
 * @brief Get cluster erase count
 *
 * @param clusterIndex Cluster index
 * @param count Pointer to store erase count
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetClusterEraseCount(uint32_t clusterIndex, uint32_t* count);

/**
 * @brief Get most worn cluster
 *
 * @return Index of most worn cluster, 0xFFFFFFFF if error
 */
uint32_t Fee_GetMostWornCluster(void);

/**
 * @brief Get least worn cluster
 *
 * @return Index of least worn cluster, 0xFFFFFFFF if error
 */
uint32_t Fee_GetLeastWornCluster(void);

/**
 * @brief Get average erase count
 *
 * @return Average erase count
 */
uint32_t Fee_GetAverageEraseCount(void);

/*============================================================================*
 * Notification Callbacks
 *============================================================================*/

/**
 * @brief Set notification callback
 *
 * @param callback Notification callback function
 */
void Fee_SetNotificationCallback(Fee_NotificationCallback_t callback);

/**
 * @brief Job end notification (callback from driver)
 *
 * Called by driver when job completes successfully.
 * Can be overridden by application.
 */
void Fee_JobEndNotification(void);

/**
 * @brief Job error notification (callback from driver)
 *
 * Called by driver when job fails.
 * Can be overridden by application.
 */
void Fee_JobErrorNotification(void);

/*============================================================================*
 * Version/Info API
 *============================================================================*/

/**
 * @brief Get Fee module version
 *
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetVersionInfo(
    uint8_t* major,
    uint8_t* minor,
    uint8_t* patch
);

/**
 * @brief Get runtime state
 *
 * @return Pointer to runtime state structure (internal use)
 */
const Fee_RuntimeStateType* Fee_GetRuntimeState(void);

/**
 * @brief Get module statistics
 *
 * @param totalWrites Pointer to store total writes
 * @param totalReads Pointer to store total reads
 * @param totalErases Pointer to store total erases
 * @param gcCount Pointer to store GC count
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_GetStatistics(
    uint32_t* totalWrites,
    uint32_t* totalReads,
    uint32_t* totalErases,
    uint32_t* gcCount
);

/*============================================================================*
 * NvM Integration Functions
 *============================================================================*/

/**
 * @brief Get the NVM job result mapping
 *
 * Maps Fee result to NVM result code.
 *
 * @param feeResult Fee result code
 * @return Equivalent NVM result code
 */
uint8_t Fee_MapResultToNvm(Fee_ErrorCode_t feeResult);

/**
 * @brief Set block protection
 *
 * @param blockNumber Block number
 * @param protect true to protect, false to unprotect
 * @return FEE_OK on success, error code otherwise
 */
Fee_ErrorCode_t Fee_SetBlockProtection(uint16_t blockNumber, bool protect);

/**
 * @brief Find block runtime info (internal use)
 *
 * @param blockNumber Block number
 * @return Pointer to block info, NULL if not found
 */
Fee_BlockInfoType* Fee_FindBlockInfo(uint16_t blockNumber);

#ifdef __cplusplus
}
#endif

#endif /* FEE_H */
