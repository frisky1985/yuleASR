/**
 * @file nvm_fee_adapter.c
 * @brief NvM to Fee Adapter Layer
 * @version 1.0.0
 * @date 2026
 *
 * Adapter layer between NvM (Non-Volatile Memory) and Fee (Flash EEPROM Emulation).
 * Provides the bridge for:
 * - Block mapping between NvM and Fee
 * - Native and Redundant block support
 * - Error code translation
 * - Write/Read operations
 */

#include "nvm.h"
#include "mcal/fee/fee.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*
 * Configuration
 *============================================================================*/
#define NVM_FEE_ADAPTER_VERSION_MAJOR   1u
#define NVM_FEE_ADAPTER_VERSION_MINOR   0u
#define NVM_FEE_ADAPTER_VERSION_PATCH   0u

#define NVM_FEE_MAX_BLOCKS              256u    /* Maximum NvM blocks mapped to Fee */
#define NVM_FEE_INVALID_BLOCK_ID        0xFFFFu

/*============================================================================*
 * Error Code Mapping
 *============================================================================*/
static Nvm_ErrorCode_t Nvm_Fee_MapErrorCode(Fee_ErrorCode_t feeError)
{
    switch (feeError) {
        case FEE_OK:
            return NVM_OK;
        case FEE_E_NOT_OK:
            return NVM_E_NOT_OK;
        case FEE_E_BUSY:
        case FEE_E_BUSY_INTERNAL:
            return NVM_E_STORAGE_NOT_READY;
        case FEE_E_INVALID_BLOCK_NO:
            return NVM_E_BLOCK_INVALID;
        case FEE_E_INVALID_BLOCK_LEN:
            return NVM_E_OUT_OF_MEMORY;
        case FEE_E_PARAM_POINTER:
            return NVM_E_PARAM_POINTER;
        case FEE_E_UNINIT:
            return NVM_E_NOT_INITIALIZED;
        case FEE_E_WRITE_PROTECTED:
            return NVM_E_BLOCK_LOCKED;
        case FEE_E_READ_FAILED:
            return NVM_E_READ_FAILED;
        case FEE_E_WRITE_FAILED:
            return NVM_E_WRITE_FAILED;
        case FEE_E_ERASE_FAILED:
            return NVM_E_ERASE_FAILED;
        case FEE_E_VERIFY_FAILED:
            return NVM_E_VERIFY_FAILED;
        case FEE_E_GC_BUSY:
            return NVM_E_STORAGE_NOT_READY;
        case FEE_E_OUT_OF_MEMORY:
            return NVM_E_OUT_OF_MEMORY;
        case FEE_E_TIMEOUT:
            return NVM_E_TIMEOUT;
        case FEE_E_INCONSISTENT:
            return NVM_E_BLOCK_CORRUPTED;
        default:
            return NVM_E_NOT_OK;
    }
}

/*============================================================================*
 * Block Mapping Structure
 *============================================================================*/
typedef struct {
    uint16_t nvmBlockId;                /* NvM block ID */
    uint16_t feeBlockNumber;            /* Fee logical block number */
    bool redundant;                     /* Redundant storage enabled */
    uint16_t feeBlockNumberRedundant;   /* Redundant Fee block number */
    uint16_t blockSize;                 /* Block size */
    bool writeProtected;                /* Write protection */
    bool valid;                         /* Mapping valid */
} Nvm_Fee_BlockMapping_t;

/*============================================================================*
 * Adapter State
 *============================================================================*/
typedef struct {
    bool initialized;
    Nvm_Fee_BlockMapping_t blockMap[NVM_FEE_MAX_BLOCKS];
    uint32_t mappedBlockCount;
    Fee_NotificationCallback_t originalFeeCallback;
} Nvm_Fee_AdapterState_t;

static Nvm_Fee_AdapterState_t gNvm_Fee_State = {
    .initialized = false,
    .mappedBlockCount = 0u
};

/*============================================================================*
 * Internal Helper Functions
 *============================================================================*/

/**
 * @brief Find mapping by NvM block ID
 */
static Nvm_Fee_BlockMapping_t* Nvm_Fee_FindMapping(uint16_t nvmBlockId)
{
    uint32_t i;

    for (i = 0u; i < NVM_FEE_MAX_BLOCKS; i++) {
        if ((gNvm_Fee_State.blockMap[i].valid) &&
            (gNvm_Fee_State.blockMap[i].nvmBlockId == nvmBlockId)) {
            return &gNvm_Fee_State.blockMap[i];
        }
    }

    return NULL;
}

/**
 * @brief Find mapping by Fee block number
 */
static Nvm_Fee_BlockMapping_t* Nvm_Fee_FindMappingByFee(uint16_t feeBlockNumber)
{
    uint32_t i;

    for (i = 0u; i < NVM_FEE_MAX_BLOCKS; i++) {
        if ((gNvm_Fee_State.blockMap[i].valid) &&
            ((gNvm_Fee_State.blockMap[i].feeBlockNumber == feeBlockNumber) ||
             (gNvm_Fee_State.blockMap[i].feeBlockNumberRedundant == feeBlockNumber))) {
            return &gNvm_Fee_State.blockMap[i];
        }
    }

    return NULL;
}

/**
 * @brief Allocate new mapping slot
 */
static Nvm_Fee_BlockMapping_t* Nvm_Fee_AllocateMapping(void)
{
    uint32_t i;

    for (i = 0u; i < NVM_FEE_MAX_BLOCKS; i++) {
        if (!gNvm_Fee_State.blockMap[i].valid) {
            (void)memset(&gNvm_Fee_State.blockMap[i], 0, sizeof(Nvm_Fee_BlockMapping_t));
            gNvm_Fee_State.blockMap[i].valid = true;
            return &gNvm_Fee_State.blockMap[i];
        }
    }

    return NULL;
}

/**
 * @brief Fee notification callback wrapper
 */
static void Nvm_Fee_NotificationCallback(
    Fee_JobType_t jobType,
    Fee_ErrorCode_t result,
    uint16_t blockNumber
)
{
    (void)jobType;
    (void)result;
    (void)blockNumber;

    /* Could notify NvM or trigger further actions */
    /* For now, just store the callback result */
}

/**
 * @brief Read with redundancy check
 */
static Nvm_ErrorCode_t Nvm_Fee_ReadRedundant(
    Nvm_Fee_BlockMapping_t* mapping,
    uint16_t offset,
    uint8_t* data,
    uint16_t length
)
{
    Fee_ErrorCode_t result1;
    Fee_ErrorCode_t result2;
    uint8_t* data1;
    uint8_t* data2;
    bool primaryValid = false;
    bool redundantValid = false;

    /* Allocate buffers */
    data1 = (uint8_t*)malloc(length);
    data2 = (uint8_t*)malloc(length);

    if ((data1 == NULL) || (data2 == NULL)) {
        if (data1 != NULL) free(data1);
        if (data2 != NULL) free(data2);
        return NVM_E_OUT_OF_MEMORY;
    }

    /* Read primary copy */
    result1 = Fee_Read(mapping->feeBlockNumber, offset, data1, length);
    if (result1 == FEE_OK) {
        primaryValid = true;
    }

    /* Read redundant copy */
    result2 = Fee_Read(mapping->feeBlockNumberRedundant, offset, data2, length);
    if (result2 == FEE_OK) {
        redundantValid = true;
    }

    /* Determine which data to use */
    if (primaryValid && redundantValid) {
        /* Both valid - compare */
        if (memcmp(data1, data2, length) == 0) {
            (void)memcpy(data, data1, length);
        } else {
            /* Mismatch - use primary, could trigger recovery */
            (void)memcpy(data, data1, length);
        }
    } else if (primaryValid) {
        /* Only primary valid */
        (void)memcpy(data, data1, length);
        /* Could rewrite redundant copy */
    } else if (redundantValid) {
        /* Only redundant valid - recover */
        (void)memcpy(data, data2, length);
        /* Rewrite primary copy */
        (void)Fee_Write(mapping->feeBlockNumber, data2);
    } else {
        /* Both failed */
        free(data1);
        free(data2);
        return NVM_E_READ_FAILED;
    }

    free(data1);
    free(data2);

    return NVM_OK;
}

/**
 * @brief Write with redundancy
 */
static Nvm_ErrorCode_t Nvm_Fee_WriteRedundant(
    Nvm_Fee_BlockMapping_t* mapping,
    const uint8_t* data
)
{
    Fee_ErrorCode_t result;

    /* Write primary copy */
    result = Fee_Write(mapping->feeBlockNumber, data);
    if (result != FEE_OK) {
        return Nvm_Fee_MapErrorCode(result);
    }

    /* Write redundant copy */
    result = Fee_Write(mapping->feeBlockNumberRedundant, data);
    if (result != FEE_OK) {
        /* Primary succeeded but redundant failed */
        /* Could retry or mark for recovery */
        return NVM_E_REDUNDANCY_FAILED;
    }

    return NVM_OK;
}

/*============================================================================*
 * Public API
 *============================================================================*/

/**
 * @brief Initialize NvM-Fee adapter
 */
Nvm_ErrorCode_t Nvm_Fee_Adapter_Init(void)
{
    Fee_ErrorCode_t feeResult;

    if (gNvm_Fee_State.initialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }

    /* Initialize Fee if not already done */
    if (!Fee_IsInitialized()) {
        feeResult = Fee_Init(NULL);
        if (feeResult != FEE_OK) {
            return Nvm_Fee_MapErrorCode(feeResult);
        }
    }

    /* Initialize state */
    (void)memset(&gNvm_Fee_State.blockMap, 0, sizeof(gNvm_Fee_State.blockMap));
    gNvm_Fee_State.mappedBlockCount = 0u;

    /* Register Fee callback */
    Fee_SetNotificationCallback(Nvm_Fee_NotificationCallback);

    gNvm_Fee_State.initialized = true;

    return NVM_OK;
}

/**
 * @brief Deinitialize NvM-Fee adapter
 */
Nvm_ErrorCode_t Nvm_Fee_Adapter_Deinit(void)
{
    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    gNvm_Fee_State.initialized = false;

    return NVM_OK;
}

/**
 * @brief Register NvM block to Fee block mapping
 */
Nvm_ErrorCode_t Nvm_Fee_RegisterBlockMapping(
    uint16_t nvmBlockId,
    uint16_t feeBlockNumber,
    uint16_t blockSize,
    bool redundant,
    uint16_t feeBlockNumberRedundant
)
{
    Nvm_Fee_BlockMapping_t* mapping;
    Fee_ErrorCode_t feeResult;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    /* Check if already mapped */
    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping != NULL) {
        /* Update existing mapping */
        mapping->feeBlockNumber = feeBlockNumber;
        mapping->blockSize = blockSize;
        mapping->redundant = redundant;
        mapping->feeBlockNumberRedundant = feeBlockNumberRedundant;
        return NVM_OK;
    }

    /* Allocate new mapping */
    mapping = Nvm_Fee_AllocateMapping();
    if (mapping == NULL) {
        return NVM_E_OUT_OF_MEMORY;
    }

    /* Initialize Fee block if not exists */
    Fee_BlockStatus_t status;
    feeResult = Fee_GetBlockStatus(feeBlockNumber, &status);
    if ((feeResult != FEE_OK) || (status == FEE_BLOCK_EMPTY)) {
        /* Block doesn't exist in Fee yet - configure it */
        /* Fee will create it on first write */
    }

    mapping->nvmBlockId = nvmBlockId;
    mapping->feeBlockNumber = feeBlockNumber;
    mapping->blockSize = blockSize;
    mapping->redundant = redundant;
    mapping->feeBlockNumberRedundant = feeBlockNumberRedundant;
    mapping->writeProtected = false;

    gNvm_Fee_State.mappedBlockCount++;

    return NVM_OK;
}

/**
 * @brief Unregister NvM block mapping
 */
Nvm_ErrorCode_t Nvm_Fee_UnregisterBlockMapping(uint16_t nvmBlockId)
{
    Nvm_Fee_BlockMapping_t* mapping;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    mapping->valid = false;
    gNvm_Fee_State.mappedBlockCount--;

    return NVM_OK;
}

/**
 * @brief Read NvM block via Fee
 */
Nvm_ErrorCode_t Nvm_Fee_ReadBlock(
    uint16_t nvmBlockId,
    uint16_t offset,
    uint8_t* data,
    uint16_t length
)
{
    Nvm_Fee_BlockMapping_t* mapping;
    Fee_ErrorCode_t feeResult;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    if (mapping->redundant) {
        return Nvm_Fee_ReadRedundant(mapping, offset, data, length);
    } else {
        feeResult = Fee_Read(mapping->feeBlockNumber, offset, data, length);
        return Nvm_Fee_MapErrorCode(feeResult);
    }
}

/**
 * @brief Write NvM block via Fee
 */
Nvm_ErrorCode_t Nvm_Fee_WriteBlock(
    uint16_t nvmBlockId,
    const uint8_t* data
)
{
    Nvm_Fee_BlockMapping_t* mapping;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    if (mapping->writeProtected) {
        return NVM_E_BLOCK_LOCKED;
    }

    if (mapping->redundant) {
        return Nvm_Fee_WriteRedundant(mapping, data);
    } else {
        Fee_ErrorCode_t feeResult = Fee_Write(mapping->feeBlockNumber, data);
        return Nvm_Fee_MapErrorCode(feeResult);
    }
}

/**
 * @brief Erase NvM block via Fee
 */
Nvm_ErrorCode_t Nvm_Fee_EraseBlock(uint16_t nvmBlockId)
{
    Nvm_Fee_BlockMapping_t* mapping;
    Fee_ErrorCode_t feeResult;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    if (mapping->writeProtected) {
        return NVM_E_BLOCK_LOCKED;
    }

    /* Erase primary */
    feeResult = Fee_EraseImmediateBlock(mapping->feeBlockNumber);
    if (feeResult != FEE_OK) {
        return Nvm_Fee_MapErrorCode(feeResult);
    }

    /* Erase redundant if enabled */
    if (mapping->redundant) {
        feeResult = Fee_EraseImmediateBlock(mapping->feeBlockNumberRedundant);
        if (feeResult != FEE_OK) {
            return Nvm_Fee_MapErrorCode(feeResult);
        }
    }

    return NVM_OK;
}

/**
 * @brief Invalidate NvM block via Fee
 */
Nvm_ErrorCode_t Nvm_Fee_InvalidateBlock(uint16_t nvmBlockId)
{
    Nvm_Fee_BlockMapping_t* mapping;
    Fee_ErrorCode_t feeResult;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    /* Invalidate primary */
    feeResult = Fee_InvalidateBlock(mapping->feeBlockNumber);
    if (feeResult != FEE_OK) {
        return Nvm_Fee_MapErrorCode(feeResult);
    }

    /* Invalidate redundant if enabled */
    if (mapping->redundant) {
        feeResult = Fee_InvalidateBlock(mapping->feeBlockNumberRedundant);
        if (feeResult != FEE_OK) {
            return Nvm_Fee_MapErrorCode(feeResult);
        }
    }

    return NVM_OK;
}

/**
 * @brief Set block write protection
 */
Nvm_ErrorCode_t Nvm_Fee_SetWriteProtection(uint16_t nvmBlockId, bool protect)
{
    Nvm_Fee_BlockMapping_t* mapping;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    mapping->writeProtected = protect;

    /* Also set in Fee */
    Fee_SetBlockProtection(mapping->feeBlockNumber, protect);
    if (mapping->redundant) {
        Fee_SetBlockProtection(mapping->feeBlockNumberRedundant, protect);
    }

    return NVM_OK;
}

/**
 * @brief Get Fee block status for NvM block
 */
Nvm_ErrorCode_t Nvm_Fee_GetBlockStatus(
    uint16_t nvmBlockId,
    Fee_BlockStatus_t* status
)
{
    Nvm_Fee_BlockMapping_t* mapping;
    Fee_ErrorCode_t feeResult;

    if (!gNvm_Fee_State.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }

    if (status == NULL) {
        return NVM_E_PARAM_POINTER;
    }

    mapping = Nvm_Fee_FindMapping(nvmBlockId);
    if (mapping == NULL) {
        return NVM_E_BLOCK_INVALID;
    }

    feeResult = Fee_GetBlockStatus(mapping->feeBlockNumber, status);
    return Nvm_Fee_MapErrorCode(feeResult);
}

/**
 * @brief Get mapped block count
 */
uint32_t Nvm_Fee_GetMappedBlockCount(void)
{
    return gNvm_Fee_State.mappedBlockCount;
}

/**
 * @brief Check if adapter is initialized
 */
bool Nvm_Fee_IsInitialized(void)
{
    return gNvm_Fee_State.initialized;
}

/**
 * @brief Process adapter (call Fee MainFunction)
 */
void Nvm_Fee_MainFunction(void)
{
    if (gNvm_Fee_State.initialized) {
        Fee_MainFunction();
    }
}

/**
 * @brief Get version info
 */
void Nvm_Fee_GetVersionInfo(uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    if (major != NULL) {
        *major = NVM_FEE_ADAPTER_VERSION_MAJOR;
    }
    if (minor != NULL) {
        *minor = NVM_FEE_ADAPTER_VERSION_MINOR;
    }
    if (patch != NULL) {
        *patch = NVM_FEE_ADAPTER_VERSION_PATCH;
    }
}
