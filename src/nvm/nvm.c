/**
 * @file nvm.c
 * @brief NvM Module Main Implementation
 * @version 1.0.0
 * @date 2025
 * 
 * ASIL-D Compliant Implementation
 */

#include "nvm.h"
#include <string.h>

/*============================================================================*
 * Module Variables
 *============================================================================*/
static bool g_nvmInitialized = false;

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_Init(void)
{
    Nvm_ErrorCode_t result;
    Nvm_CoreConfig_t coreConfig = {0};
    
    if (g_nvmInitialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* Initialize storage interface */
    result = Nvm_StorageIf_Init();
    if (result != NVM_OK) {
        return result;
    }
    
    /* Initialize block manager */
    result = Nvm_BlockMgr_Init();
    if (result != NVM_OK) {
        Nvm_StorageIf_Deinit();
        return result;
    }
    
    /* Initialize redundancy manager */
    result = Nvm_Redundancy_Init();
    if (result != NVM_OK) {
        Nvm_BlockMgr_Deinit();
        Nvm_StorageIf_Deinit();
        return result;
    }
    
    /* Initialize job queue */
    result = Nvm_JobQueue_Init();
    if (result != NVM_OK) {
        Nvm_Redundancy_Deinit();
        Nvm_BlockMgr_Deinit();
        Nvm_StorageIf_Deinit();
        return result;
    }
    
    /* Initialize core with default config */
    coreConfig.writeVerification = true;
    coreConfig.readVerify = true;
    coreConfig.autoRestore = true;
    coreConfig.powerLossDetect = true;
    coreConfig.maxRetries = NVM_MAX_WRITE_RETRIES;
    coreConfig.writeTimeoutMs = 1000;
    coreConfig.readTimeoutMs = 500;
    coreConfig.maxWearLevel = 100000;
    
    result = Nvm_Core_Init(&coreConfig);
    if (result != NVM_OK) {
        Nvm_JobQueue_Deinit();
        Nvm_Redundancy_Deinit();
        Nvm_BlockMgr_Deinit();
        Nvm_StorageIf_Deinit();
        return result;
    }
    
    /* Perform cold boot validation */
    result = Nvm_Core_ColdBootValidation();
    if (result != NVM_OK) {
        /* Continue anyway - blocks may need initialization */
    }
    
    g_nvmInitialized = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Deinit(void)
{
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Process any remaining jobs */
    Nvm_JobQueue_ProcessAll();
    
    /* Deinitialize submodules */
    Nvm_Core_Deinit();
    Nvm_JobQueue_Deinit();
    Nvm_Redundancy_Deinit();
    Nvm_BlockMgr_Deinit();
    Nvm_StorageIf_Deinit();
    
    g_nvmInitialized = false;
    
    return NVM_OK;
}

bool Nvm_IsInitialized(void)
{
    return g_nvmInitialized;
}

void Nvm_MainFunction(void)
{
    if (!g_nvmInitialized) {
        return;
    }
    
    /* Process pending jobs */
    Nvm_JobQueue_ProcessOne();
    
    /* Other periodic tasks could go here */
}

/*============================================================================*
 * High-Level Data API
 *============================================================================*/

Nvm_ErrorCode_t Nvm_ReadBlock(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result;
    Nvm_Block_t* block;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    /* Get block info */
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Validate block first */
    result = Nvm_BlockMgr_ValidateHeader(blockId);
    if (result != NVM_OK) {
        /* Try to recover if block is corrupted */
        if (block->redundant) {
            result = Nvm_Redundancy_Recover(blockId);
            if (result == NVM_OK) {
                /* Retry validation */
                result = Nvm_BlockMgr_ValidateHeader(blockId);
            }
        }
        
        if (result != NVM_OK) {
            return result;
        }
    }
    
    /* Read with verification if configured */
    if (Nvm_Core_GetContext()->config.readVerify) {
        result = Nvm_Core_ReadWithVerify(blockId, data, length);
    } else {
        /* Direct read from storage */
        result = Nvm_StorageIf_ReadBlock(blockId, data, length);
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_WriteBlock(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result;
    Nvm_Block_t* block;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    /* Get block info */
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Check write protection */
    if (block->protection == NVM_PROT_WRITE || 
        block->protection == NVM_PROT_ALL) {
        return NVM_E_BLOCK_LOCKED;
    }
    
    /* Write with redundancy if enabled */
    if (block->redundant) {
        result = Nvm_Redundancy_Write(blockId, data, length);
    } else {
        /* Write with retry mechanism */
        result = Nvm_Core_WriteWithRetry(blockId, data, length);
        
        /* Verify write if configured */
        if (result == NVM_OK && Nvm_Core_GetContext()->config.writeVerification) {
            result = Nvm_Core_VerifyWrite(blockId, data, length);
        }
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_EraseBlock(uint16_t blockId)
{
    Nvm_Block_t* block;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Get block info */
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Check erase protection */
    if (block->protection == NVM_PROT_ERASE || 
        block->protection == NVM_PROT_ALL) {
        return NVM_E_BLOCK_LOCKED;
    }
    
    return Nvm_StorageIf_EraseBlock(blockId);
}

Nvm_ErrorCode_t Nvm_ValidateBlock(uint16_t blockId)
{
    Nvm_ErrorCode_t result;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    result = Nvm_BlockMgr_ValidateHeader(blockId);
    
    return result;
}

Nvm_ErrorCode_t Nvm_RestoreBlockDefaults(uint16_t blockId)
{
    Nvm_ErrorCode_t result;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Erase block to reset to defaults */
    result = Nvm_BlockMgr_EraseBlock(blockId);
    
    return result;
}

/*============================================================================*
 * Configuration API
 *============================================================================*/

Nvm_ErrorCode_t Nvm_ConfigureBlock(
    uint16_t blockId,
    const char* name,
    Nvm_BlockType_t type,
    uint32_t address,
    uint32_t length,
    bool redundant)
{
    Nvm_Block_t config = {0};
    Nvm_ErrorCode_t result;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (name == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    /* Fill configuration */
    config.blockId = blockId;
    strncpy(config.name, name, NVM_BLOCK_NAME_LEN - 1);
    config.name[NVM_BLOCK_NAME_LEN - 1] = '\0';
    config.type = type;
    config.storageAddress = address;
    config.dataLength = length;
    config.redundant = redundant;
    config.configured = true;
    config.persistent = true;
    config.protection = NVM_PROT_NONE;
    
    result = Nvm_BlockMgr_ConfigureBlock(blockId, &config);
    
    /* Configure redundancy if enabled */
    if (result == NVM_OK && redundant) {
        /* Use next block as redundant pair */
        result = Nvm_Redundancy_ConfigurePair(blockId, blockId + 1, true);
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_RegisterStorage(
    uint8_t deviceId,
    Nvm_StorageDeviceType_t type,
    uint32_t baseAddress,
    uint32_t totalSize)
{
    Nvm_StorageDevice_t device = {0};
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Fill device configuration */
    device.deviceId = deviceId;
    device.type = type;
    device.baseAddress = baseAddress;
    device.totalSize = totalSize;
    
    /* Set default page/sector sizes based on type */
    switch (type) {
        case NVM_DEV_FLASH_INTERNAL:
        case NVM_DEV_FLASH_EXTERNAL:
        case NVM_DEV_EMULATED_EEPROM:
            device.pageSize = NVM_FLASH_PAGE_SIZE;
            device.sectorSize = NVM_STORAGE_SECTOR_SIZE;
            break;
            
        case NVM_DEV_EEPROM_INTERNAL:
        case NVM_DEV_EEPROM_EXTERNAL:
            device.pageSize = NVM_EEPROM_PAGE_SIZE;
            device.sectorSize = NVM_STORAGE_SECTOR_SIZE;
            break;
            
        default:
            device.pageSize = NVM_STORAGE_PAGE_SIZE;
            device.sectorSize = NVM_STORAGE_SECTOR_SIZE;
            break;
    }
    
    return Nvm_StorageIf_RegisterDevice(&device);
}

/*============================================================================*
 * Asynchronous API
 *============================================================================*/

Nvm_ErrorCode_t Nvm_ReadBlockAsync(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length,
    Nvm_JobCallback_t callback,
    void* userData)
{
    Nvm_Job_t* job;
    Nvm_ErrorCode_t result;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Create job */
    job = Nvm_JobQueue_CreateJob(NVM_JOB_READ, NVM_PRIO_NORMAL, blockId);
    if (job == NULL) {
        return NVM_E_OUT_OF_MEMORY;
    }
    
    job->dataBuffer = data;
    job->dataLength = length;
    
    /* Submit with callback */
    result = Nvm_JobQueue_SubmitWithCallback(job, callback, userData);
    
    if (result != NVM_OK) {
        Nvm_JobQueue_FreeJob(job);
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_WriteBlockAsync(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length,
    Nvm_JobCallback_t callback,
    void* userData)
{
    Nvm_Job_t* job;
    Nvm_ErrorCode_t result;
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Create job */
    job = Nvm_JobQueue_CreateJob(NVM_JOB_WRITE, NVM_PRIO_NORMAL, blockId);
    if (job == NULL) {
        return NVM_E_OUT_OF_MEMORY;
    }
    
    /* Note: Data buffer must remain valid until callback! */
    job->dataBuffer = (uint8_t*)data;
    job->dataLength = length;
    
    /* Submit with callback */
    result = Nvm_JobQueue_SubmitWithCallback(job, callback, userData);
    
    if (result != NVM_OK) {
        Nvm_JobQueue_FreeJob(job);
    }
    
    return result;
}

/*============================================================================*
 * Statistics API
 *============================================================================*/

Nvm_ErrorCode_t Nvm_GetVersion(
    uint8_t* major,
    uint8_t* minor,
    uint8_t* patch)
{
    if (major == NULL || minor == NULL || patch == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    *major = NVM_MAJOR_VERSION;
    *minor = NVM_MINOR_VERSION;
    *patch = NVM_PATCH_VERSION;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_GetStatistics(
    uint32_t* blocksConfigured,
    uint32_t* jobsPending,
    uint32_t* wearLevel)
{
    Nvm_ErrorCode_t result;
    uint32_t minCount, maxCount, avgCount;
    
    if (blocksConfigured == NULL || jobsPending == NULL || wearLevel == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (!g_nvmInitialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    *blocksConfigured = Nvm_BlockMgr_GetConfiguredCount();
    *jobsPending = Nvm_JobQueue_GetPendingCount();
    
    /* Get wear level from first device */
    result = Nvm_WearLevel_GetStats(0, &minCount, &maxCount, &avgCount);
    if (result == NVM_OK) {
        /* Calculate wear level percentage (0-100%) */
        uint32_t threshold = Nvm_Core_GetContext()->config.maxWearLevel;
        *wearLevel = (avgCount * 100) / threshold;
    } else {
        *wearLevel = 0;
    }
    
    return NVM_OK;
}
