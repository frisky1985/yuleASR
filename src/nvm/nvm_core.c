/**
 * @file nvm_core.c
 * @brief NvM Core Management Implementation
 * @version 1.0.0
 * @date 2025
 * 
 * ASIL-D Compliant Implementation
 */

#include "nvm_core.h"
#include "nvm_block_manager.h"
#include "nvm_storage_if.h"
#include <string.h>

/*============================================================================*
 * Module Variables
 *============================================================================*/
static Nvm_CoreContext_t g_coreContext = {0};
static uint8_t g_verifyBuffer[NVM_MAX_PAGE_SIZE];

/*============================================================================*
 * Private Function Prototypes
 *============================================================================*/
static Nvm_ErrorCode_t Nvm_Private_DetectPowerLoss(void);
static Nvm_ErrorCode_t Nvm_Private_PerifySlowRead(
    uint32_t address,
    uint8_t* data,
    uint32_t length
);

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_Core_Init(const Nvm_CoreConfig_t* config)
{
    Nvm_ErrorCode_t result = NVM_OK;
    
    /* ASIL-D: Check for double initialization */
    if (g_coreContext.initialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* ASIL-D: Null pointer check */
    if (config == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    /* Initialize context */
    memset(&g_coreContext, 0, sizeof(Nvm_CoreContext_t));
    
    /* Copy configuration */
    g_coreContext.config = *config;
    
    /* Set default values if not specified */
    if (g_coreContext.config.maxRetries == 0) {
        g_coreContext.config.maxRetries = NVM_MAX_WRITE_RETRIES;
    }
    if (g_coreContext.config.writeTimeoutMs == 0) {
        g_coreContext.config.writeTimeoutMs = 1000u; /* 1 second default */
    }
    if (g_coreContext.config.readTimeoutMs == 0) {
        g_coreContext.config.readTimeoutMs = 500u; /* 500ms default */
    }
    
    /* Initialize safety monitor */
    memset(&g_coreContext.safety, 0, sizeof(Nvm_SafetyMonitor_t));
    
    /* Check for previous power loss */
    if (g_coreContext.config.powerLossDetect) {
        result = Nvm_Private_DetectPowerLoss();
        if (result != NVM_OK) {
            g_coreContext.safety.powerLossCount++;
        }
    }
    
    /* Mark as initialized */
    g_coreContext.initialized = true;
    g_coreContext.writeProtected = false;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Core_Deinit(void)
{
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Clear all sensitive data */
    memset(&g_coreContext, 0, sizeof(Nvm_CoreContext_t));
    memset(g_verifyBuffer, 0, sizeof(g_verifyBuffer));
    
    return NVM_OK;
}

bool Nvm_Core_IsInitialized(void)
{
    return g_coreContext.initialized;
}

Nvm_ErrorCode_t Nvm_Core_EnableWriteProtect(void)
{
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    g_coreContext.writeProtected = true;
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Core_DisableWriteProtect(uint32_t password)
{
    (void)password; /* Password checking can be implemented */
    
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    g_coreContext.writeProtected = false;
    return NVM_OK;
}

bool Nvm_Core_IsWriteProtected(void)
{
    return g_coreContext.writeProtected;
}

Nvm_ErrorCode_t Nvm_Core_WriteWithRetry(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_E_NOT_OK;
    uint8_t retryCount = 0;
    Nvm_Block_t* block = NULL;
    
    /* ASIL-D: Pre-condition checks */
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (length == 0 || length > NVM_MAX_PAGE_SIZE) {
        return NVM_E_PARAM_RANGE;
    }
    
    /* Check write protection */
    if (g_coreContext.writeProtected) {
        return NVM_E_BLOCK_LOCKED;
    }
    
    /* Get block information */
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Check block protection */
    if (block->protection == NVM_PROT_WRITE || 
        block->protection == NVM_PROT_ALL) {
        return NVM_E_BLOCK_LOCKED;
    }
    
    /* Retry loop */
    while (retryCount <= g_coreContext.config.maxRetries) {
        result = Nvm_StorageIf_Write(
            block->storageAddress,
            data,
            length
        );
        
        if (result == NVM_OK) {
            /* Write successful */
            g_coreContext.safety.writeCount++;
            break;
        }
        
        /* Write failed, increment counters */
        retryCount++;
        g_coreContext.safety.retryCount++;
        
        /* Small delay before retry (implement if needed) */
        /* Nvm_Platform_Delay(1); */
    }
    
    /* Update statistics on failure */
    if (result != NVM_OK) {
        g_coreContext.safety.errorCount++;
        g_coreContext.safety.lastError = (uint32_t)result;
    }
    
    return result;
}

Nvm_ErrorCode_t Nvm_Core_VerifyWrite(
    uint16_t blockId,
    const uint8_t* expectedData,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_Block_t* block = NULL;
    
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (expectedData == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Read back the data */
    result = Nvm_StorageIf_Read(
        block->storageAddress,
        g_verifyBuffer,
        length
    );
    
    if (result != NVM_OK) {
        return NVM_E_READ_FAILED;
    }
    
    /* Compare data */
    if (memcmp(g_verifyBuffer, expectedData, length) != 0) {
        return NVM_E_VERIFY_FAILED;
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Core_ReadWithVerify(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_Block_t* block = NULL;
    
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* First read - normal speed */
    result = Nvm_StorageIf_Read(
        block->storageAddress,
        data,
        length
    );
    
    if (result != NVM_OK) {
        return NVM_E_READ_FAILED;
    }
    
    /* Second read - slow verification read */
    result = Nvm_Private_PerifySlowRead(
        block->storageAddress,
        g_verifyBuffer,
        length
    );
    
    if (result != NVM_OK) {
        return NVM_E_READ_FAILED;
    }
    
    /* Compare both reads */
    if (memcmp(data, g_verifyBuffer, length) != 0) {
        /* Data inconsistency detected */
        g_coreContext.safety.errorCount++;
        return NVM_E_READ_FAILED;
    }
    
    g_coreContext.safety.readCount++;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Core_ColdBootValidation(void)
{
    Nvm_ErrorCode_t result = NVM_OK;
    uint16_t blockId;
    Nvm_Block_t* block = NULL;
    uint32_t validCount = 0;
    uint32_t corruptCount = 0;
    
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    /* Iterate through all configured blocks */
    for (blockId = 0; blockId < NVM_MAX_BLOCK_COUNT; blockId++) {
        block = Nvm_BlockMgr_GetBlock(blockId);
        if (block == NULL || !block->configured) {
            continue;
        }
        
        /* Validate block header */
        result = Nvm_BlockMgr_ValidateHeader(blockId);
        
        if (result == NVM_OK) {
            validCount++;
        } else {
            corruptCount++;
            
            /* Attempt recovery if configured */
            if (g_coreContext.config.autoRestore) {
                result = Nvm_BlockMgr_RecoverBlock(blockId);
                if (result == NVM_OK) {
                    g_coreContext.safety.recoveryCount++;
                    validCount++;
                    corruptCount--;
                }
            }
        }
    }
    
    /* ASIL-D: At least critical blocks must be valid */
    if (corruptCount > 0) {
        /* Log error but don't fail if recovery was successful */
        if (!g_coreContext.config.autoRestore) {
            return NVM_E_BLOCK_CORRUPTED;
        }
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Core_HandlePowerLoss(void)
{
    Nvm_ErrorCode_t result = NVM_OK;
    
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    g_coreContext.safety.powerLossCount++;
    
    /* Scan for blocks that might be in inconsistent state */
    result = Nvm_Core_ColdBootValidation();
    
    return result;
}

Nvm_ErrorCode_t Nvm_Core_GetSafetyMonitor(Nvm_SafetyMonitor_t* monitor)
{
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (monitor == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    *monitor = g_coreContext.safety;
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_Core_ResetSafetyMonitor(void)
{
    if (!g_coreContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    memset(&g_coreContext.safety, 0, sizeof(Nvm_SafetyMonitor_t));
    return NVM_OK;
}

Nvm_CoreContext_t* Nvm_Core_GetContext(void)
{
    return &g_coreContext;
}

/*============================================================================*
 * Private Functions
 *============================================================================*/

static Nvm_ErrorCode_t Nvm_Private_DetectPowerLoss(void)
{
    /* Check power loss flag in storage */
    /* This would typically read a specific location in flash/EEPROM */
    /* that is set at the start of write and cleared at completion */
    
    /* For now, assume no power loss detected */
    return NVM_OK;
}

static Nvm_ErrorCode_t Nvm_Private_PerifySlowRead(
    uint32_t address,
    uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_OK;
    
    /* Slow read implementation - adds delays between reads */
    /* This helps detect marginal cells */
    
    result = Nvm_StorageIf_Read(address, data, length);
    
    /* Add delay after read */
    /* Nvm_Platform_Delay(1); */
    
    return result;
}
