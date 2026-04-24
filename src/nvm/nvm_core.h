/**
 * @file nvm_core.h
 * @brief NvM Core Management Header
 * @version 1.0.0
 * @date 2025
 * 
 * Core management functionality for NVM module including:
 * - Block management
 * - Write protection
 * - Write retry mechanism
 * - Write verification
 */

#ifndef NVM_CORE_H
#define NVM_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nvm_types.h"

/*============================================================================*
 * Configuration Structure
 *============================================================================*/
typedef struct {
    bool writeVerification;                 /* Enable write verification */
    bool readVerify;                        /* Read-after-write verify */
    bool autoRestore;                       /* Automatic data restore on error */
    bool powerLossDetect;                   /* Enable power loss detection */
    uint8_t maxRetries;                     /* Maximum write retries */
    uint32_t writeTimeoutMs;                /* Write timeout in milliseconds */
    uint32_t readTimeoutMs;                 /* Read timeout in milliseconds */
    uint32_t maxWearLevel;                  /* Maximum acceptable wear level */
} Nvm_CoreConfig_t;

/*============================================================================*
 * Core Context
 *============================================================================*/
typedef struct {
    bool initialized;                       /* Module initialized flag */
    bool writeProtected;                    /* Global write protection */
    uint32_t totalBlocks;                   /* Total configured blocks */
    uint32_t activeJobs;                    /* Number of active jobs */
    Nvm_CoreConfig_t config;                /* Runtime configuration */
    Nvm_SafetyMonitor_t safety;             /* Safety monitor counters */
    void* storageHandle;                    /* Storage interface handle */
} Nvm_CoreContext_t;

/*============================================================================*
 * Core API Functions
 *============================================================================*/

/**
 * @brief Initialize NvM core module
 * @param config Pointer to core configuration
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_Init(const Nvm_CoreConfig_t* config);

/**
 * @brief Deinitialize NvM core module
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_Deinit(void);

/**
 * @brief Get core module initialization status
 * @return true if initialized, false otherwise
 */
bool Nvm_Core_IsInitialized(void);

/**
 * @brief Enable global write protection
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_EnableWriteProtect(void);

/**
 * @brief Disable global write protection
 * @param password Security password (if configured)
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_DisableWriteProtect(uint32_t password);

/**
 * @brief Check if write protection is active
 * @return true if write protected, false otherwise
 */
bool Nvm_Core_IsWriteProtected(void);

/**
 * @brief Write data with retry mechanism
 * @param blockId Block identifier
 * @param data Data to write
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_WriteWithRetry(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Verify written data (read-after-write)
 * @param blockId Block identifier
 * @param expectedData Expected data
 * @param length Data length
 * @return NVM_OK if verified, NVM_E_VERIFY_FAILED otherwise
 */
Nvm_ErrorCode_t Nvm_Core_VerifyWrite(
    uint16_t blockId,
    const uint8_t* expectedData,
    uint32_t length
);

/**
 * @brief Read data with slow-read verification
 * @param blockId Block identifier
 * @param data Buffer to read into
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_ReadWithVerify(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length
);

/**
 * @brief Perform cold boot validation of all blocks
 * @return NVM_OK if all blocks valid, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_ColdBootValidation(void);

/**
 * @brief Handle power loss detection
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_HandlePowerLoss(void);

/**
 * @brief Get safety monitor statistics
 * @param monitor Pointer to store monitor data
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_GetSafetyMonitor(Nvm_SafetyMonitor_t* monitor);

/**
 * @brief Reset safety monitor counters
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_Core_ResetSafetyMonitor(void);

/**
 * @brief Get core context (for internal use)
 * @return Pointer to core context
 */
Nvm_CoreContext_t* Nvm_Core_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* NVM_CORE_H */
