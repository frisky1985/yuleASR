/**
 * @file nvm.h
 * @brief NvM (Non-Volatile Memory) Module Main Header
 * @version 1.0.0
 * @date 2025
 * 
 * This is the main header for the AUTOSAR-compliant NVM module.
 * Include this header to use all NVM functionality.
 * 
 * Features:
 * - Block-based storage management
 * - Dual redundancy for critical data
 * - CRC32 data integrity
 * - Write verification
 * - Wear leveling
 * - ASIL-D safety compliance
 */

#ifndef NVM_H
#define NVM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include all NVM submodules */
#include "nvm_types.h"
#include "nvm_core.h"
#include "nvm_block_manager.h"
#include "nvm_redundancy.h"
#include "nvm_job_queue.h"
#include "nvm_storage_if.h"

/*============================================================================*
 * Module Information
 *============================================================================*/
#define NVM_MODULE_NAME         "NvM"
#define NVM_MODULE_VENDOR       "AUTOSAR Compliant"
#define NVM_AR_MAJOR_VERSION    4
#define NVM_AR_MINOR_VERSION    4
#define NVM_AR_PATCH_VERSION    0

/*============================================================================*
 * Initialization API
 *============================================================================*/

/**
 * @brief Initialize entire NVM module
 * 
 * Initializes all submodules:
 * - Core management
 * - Block manager
 * - Redundancy manager
 * - Job queue
 * - Storage interface
 * 
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Init(void);

/**
 * @brief Deinitialize entire NVM module
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Deinit(void);

/**
 * @brief Check if NVM module is initialized
 * @return true if initialized, false otherwise
 */
bool Nvm_IsInitialized(void);

/**
 * @brief NVM main function - should be called periodically
 * 
 * Processes pending jobs and performs maintenance tasks.
 * Call this function in your main loop or cyclic task.
 */
void Nvm_MainFunction(void);

/*============================================================================*
 * High-Level Data API
 *============================================================================*/

/**
 * @brief Read data from NVM block
 * @param blockId Block identifier
 * @param data Buffer to read into
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_ReadBlock(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length
);

/**
 * @brief Write data to NVM block
 * @param blockId Block identifier
 * @param data Data to write
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WriteBlock(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Erase NVM block
 * @param blockId Block identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_EraseBlock(uint16_t blockId);

/**
 * @brief Validate NVM block integrity
 * @param blockId Block identifier
 * @return NVM_OK if valid, error code otherwise
 */
Nvm_ErrorCode_t Nvm_ValidateBlock(uint16_t blockId);

/**
 * @brief Restore NVM block to defaults
 * @param blockId Block identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_RestoreBlockDefaults(uint16_t blockId);

/*============================================================================*
 * Configuration API
 *============================================================================*/

/**
 * @brief Configure NVM block
 * @param blockId Block identifier
 * @param name Block name
 * @param type Block type
 * @param address Storage address
 * @param length Data length
 * @param redundant Enable redundancy
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_ConfigureBlock(
    uint16_t blockId,
    const char* name,
    Nvm_BlockType_t type,
    uint32_t address,
    uint32_t length,
    bool redundant
);

/**
 * @brief Register storage device
 * @param deviceId Device identifier
 * @param type Device type
 * @param baseAddress Base address
 * @param totalSize Total size
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_RegisterStorage(
    uint8_t deviceId,
    Nvm_StorageDeviceType_t type,
    uint32_t baseAddress,
    uint32_t totalSize
);

/*============================================================================*
 * Asynchronous API
 *============================================================================*/

/**
 * @brief Read block asynchronously
 * @param blockId Block identifier
 * @param data Buffer to read into
 * @param length Data length
 * @param callback Completion callback
 * @param userData User data for callback
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_ReadBlockAsync(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length,
    Nvm_JobCallback_t callback,
    void* userData
);

/**
 * @brief Write block asynchronously
 * @param blockId Block identifier
 * @param data Data to write
 * @param length Data length
 * @param callback Completion callback
 * @param userData User data for callback
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WriteBlockAsync(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length,
    Nvm_JobCallback_t callback,
    void* userData
);

/*============================================================================*
 * Statistics API
 *============================================================================*/

/**
 * @brief Get NVM module version
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_GetVersion(
    uint8_t* major,
    uint8_t* minor,
    uint8_t* patch
);

/**
 * @brief Get NVM statistics
 * @param blocksConfigured Pointer to store configured block count
 * @param jobsPending Pointer to store pending job count
 * @param wearLevel Pointer to store wear level percentage
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_GetStatistics(
    uint32_t* blocksConfigured,
    uint32_t* jobsPending,
    uint32_t* wearLevel
);

#ifdef __cplusplus
}
#endif

#endif /* NVM_H */
