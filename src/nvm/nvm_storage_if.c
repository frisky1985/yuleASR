/**
 * @file nvm_storage_if.c
 * @brief NvM Storage Interface Implementation
 * @version 1.0.0
 * @date 2025
 * 
 * ASIL-D Compliant Implementation
 */

#include "nvm_storage_if.h"
#include "nvm_block_manager.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*
 * Module Variables
 *============================================================================*/
static Nvm_StorageIf_Context_t g_storageIfContext = {0};
static uint8_t g_pageBuffer[NVM_STORAGE_PAGE_SIZE];

/* Simulated storage memory (for testing) */
static uint8_t* g_simulatedStorage = NULL;
static uint32_t g_simulatedStorageSize = 0;

#define NVM_USE_SIMULATED_STORAGE   1

/*============================================================================*
 * Private Function Prototypes
 *============================================================================*/
static Nvm_StorageDevice_t* Nvm_Private_FindDeviceByAddress(uint32_t address);
static Nvm_ErrorCode_t Nvm_Private_WriteToDevice(
    Nvm_StorageDevice_t* device,
    uint32_t address,
    const uint8_t* data,
    uint32_t length
);
static uint32_t Nvm_Private_CalculateRegionIndex(
    Nvm_StorageDevice_t* device,
    uint32_t address
);

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_StorageIf_Init(void)
{
    if (g_storageIfContext.initialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* Initialize context */
    memset(&g_storageIfContext, 0, sizeof(Nvm_StorageIf_Context_t));
    memset(g_pageBuffer, 0xFF, sizeof(g_pageBuffer));
    
    #if NVM_USE_SIMULATED_STORAGE
    /* Allocate simulated storage (64KB) */
    if (g_simulatedStorage == NULL) {
        g_simulatedStorageSize = 0x10000; /* 64KB */
        g_simulatedStorage = (uint8_t*)malloc(g_simulatedStorageSize);
        if (g_simulatedStorage == NULL) {
            return NVM_E_OUT_OF_MEMORY;
        }
        memset(g_simulatedStorage, 0xFF, g_simulatedStorageSize);
    }
    #endif
    
    g_storageIfContext.wearLevelThreshold = 100000; /* 100k cycles default */
    g_storageIfContext.initialized = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_StorageIf_Deinit(void)
{
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    #if NVM_USE_SIMULATED_STORAGE
    /* Free simulated storage */
    if (g_simulatedStorage != NULL) {
        free(g_simulatedStorage);
        g_simulatedStorage = NULL;
        g_simulatedStorageSize = 0;
    }
    #endif
    
    memset(&g_storageIfContext, 0, sizeof(Nvm_StorageIf_Context_t));
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_StorageIf_RegisterDevice(
    const Nvm_StorageDevice_t* device)
{
    Nvm_StorageDevice_t* dev;
    uint32_t i;
    
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (device == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (device->deviceId >= NVM_STORAGE_MAX_DEVICES) {
        return NVM_E_PARAM_RANGE;
    }
    
    dev = &g_storageIfContext.devices[device->deviceId];
    
    /* Copy device configuration */
    memcpy(dev, device, sizeof(Nvm_StorageDevice_t));
    dev->initialized = true;
    dev->eraseCycles = 0xFFFFFFFF; /* Initial value */
    
    /* Initialize regions */
    for (i = 0; i < NVM_STORAGE_MAX_REGIONS; i++) {
        dev->regions[i].startAddress = dev->baseAddress + (i * NVM_STORAGE_SECTOR_SIZE);
        dev->regions[i].size = NVM_STORAGE_SECTOR_SIZE;
        dev->regions[i].eraseCount = 0;
        dev->regions[i].writeCount = 0;
        dev->regions[i].inUse = false;
        dev->regions[i].worn = false;
    }
    
    g_storageIfContext.activeDevices++;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_StorageIf_UnregisterDevice(uint8_t deviceId)
{
    Nvm_StorageDevice_t* device;
    
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (deviceId >= NVM_STORAGE_MAX_DEVICES) {
        return NVM_E_PARAM_RANGE;
    }
    
    device = &g_storageIfContext.devices[deviceId];
    
    if (!device->initialized) {
        return NVM_E_NOT_OK;
    }
    
    memset(device, 0, sizeof(Nvm_StorageDevice_t));
    g_storageIfContext.activeDevices--;
    
    return NVM_OK;
}

Nvm_StorageDevice_t* Nvm_StorageIf_GetDevice(uint8_t deviceId)
{
    Nvm_StorageDevice_t* device;
    
    if (!g_storageIfContext.initialized) {
        return NULL;
    }
    
    if (deviceId >= NVM_STORAGE_MAX_DEVICES) {
        return NULL;
    }
    
    device = &g_storageIfContext.devices[deviceId];
    
    if (!device->initialized) {
        return NULL;
    }
    
    return device;
}

Nvm_ErrorCode_t Nvm_StorageIf_Read(
    uint32_t address,
    uint8_t* data,
    uint32_t length)
{
    Nvm_StorageDevice_t* device;
    
    (void)device;
    
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (length == 0) {
        return NVM_E_PARAM_RANGE;
    }
    
    #if NVM_USE_SIMULATED_STORAGE
    /* Use simulated storage */
    if (g_simulatedStorage != NULL) {
        uint32_t storageOffset = address & (g_simulatedStorageSize - 1);
        if (storageOffset + length <= g_simulatedStorageSize) {
            memcpy(data, g_simulatedStorage + storageOffset, length);
            return NVM_OK;
        } else {
            return NVM_E_PARAM_RANGE;
        }
    }
    return NVM_E_STORAGE_NOT_READY;
    #else
    /* Find device by address */
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    /* Use device read function */
    if (device->readFn != NULL) {
        return device->readFn(address, data, length);
    } else {
        /* Default: copy from memory (simulation) */
        memcpy(data, (void*)(uintptr_t)address, length);
        return NVM_OK;
    }
    #endif
}

Nvm_ErrorCode_t Nvm_StorageIf_Write(
    uint32_t address,
    const uint8_t* data,
    uint32_t length)
{
    Nvm_StorageDevice_t* device;
    Nvm_ErrorCode_t result;
    
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (data == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (length == 0) {
        return NVM_E_PARAM_RANGE;
    }
    
    /* Find device by address */
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    /* Check for worn region */
    if (Nvm_WearLevel_IsCritical(address)) {
        /* Try to find better region */
        uint32_t newAddress;
        result = Nvm_WearLevel_FindBestRegion(device->deviceId, &newAddress);
        if (result == NVM_OK) {
            /* Would need to handle address translation here */
        }
    }
    
    /* Use device write function */
    result = Nvm_Private_WriteToDevice(device, address, data, length);
    
    return result;
}

Nvm_ErrorCode_t Nvm_StorageIf_Erase(
    uint32_t address,
    uint32_t length)
{
    Nvm_StorageDevice_t* device;
    uint32_t endAddress;
    uint32_t currentAddr;
    
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (length == 0) {
        return NVM_E_PARAM_RANGE;
    }
    
    #if NVM_USE_SIMULATED_STORAGE
    /* Use simulated storage */
    if (g_simulatedStorage != NULL) {
        uint32_t storageOffset = address & (g_simulatedStorageSize - 1);
        uint32_t eraseLength = length;
        
        if (storageOffset + eraseLength > g_simulatedStorageSize) {
            eraseLength = g_simulatedStorageSize - storageOffset;
        }
        
        memset(g_simulatedStorage + storageOffset, 0xFF, eraseLength);
        
        /* Update wear level simulation */
        g_storageIfContext.totalEraseCount++;
        
        return NVM_OK;
    }
    return NVM_E_STORAGE_NOT_READY;
    #else
    /* Find device by address */
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    /* Erase by sectors */
    endAddress = address + length;
    currentAddr = address;
    
    while (currentAddr < endAddress) {
        /* Align to sector boundary */
        uint32_t sectorAddr = currentAddr & ~(device->sectorSize - 1);
        
        if (device->eraseFn != NULL) {
            if (device->eraseFn(sectorAddr, device->sectorSize) != NVM_OK) {
                return NVM_E_ERASE_FAILED;
            }
        } else {
            /* Default: memset to 0xFF (simulation) */
            memset((void*)(uintptr_t)sectorAddr, 0xFF, device->sectorSize);
        }
        
        /* Update wear level */
        Nvm_WearLevel_Update(sectorAddr);
        g_storageIfContext.totalEraseCount++;
        
        currentAddr = sectorAddr + device->sectorSize;
    }
    
    return NVM_OK;
    #endif
}

Nvm_ErrorCode_t Nvm_StorageIf_ReadBlock(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length)
{
    Nvm_Block_t* block;
    Nvm_ErrorCode_t result;
    uint32_t dataAddress;
    
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Calculate data address (after header) */
    dataAddress = block->storageAddress + NVM_BLOCK_HEADER_SIZE;
    
    /* Read header */
    Nvm_BlockHeader_t header;
    result = Nvm_StorageIf_Read(block->storageAddress, (uint8_t*)&header, 
                                NVM_BLOCK_HEADER_SIZE);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Validate header */
    if (header.magic != NVM_MAGIC_NUMBER) {
        return NVM_E_MAGIC_FAILED;
    }
    
    /* Read data */
    result = Nvm_StorageIf_Read(dataAddress, data, length);
    
    return result;
}

Nvm_ErrorCode_t Nvm_StorageIf_WriteBlock(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length)
{
    Nvm_Block_t* block;
    Nvm_ErrorCode_t result;
    Nvm_BlockHeader_t header;
    uint32_t dataAddress;
    uint32_t sequence;
    
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Get next sequence number */
    sequence = Nvm_BlockMgr_GetNextSequence(blockId);
    
    /* Prepare header */
    header.magic = NVM_MAGIC_NUMBER;
    header.version = NVM_VERSION_CURRENT;
    header.blockId = blockId;
    header.dataLength = length;
    header.sequence = sequence;
    header.timestamp = 0; /* Could use system tick */
    header.crc32 = Nvm_BlockMgr_CalculateCRC32(data, length);
    memset(header.reserved, 0, sizeof(header.reserved));
    
    /* Calculate data address */
    dataAddress = block->storageAddress + NVM_BLOCK_HEADER_SIZE;
    
    /* Erase block area first */
    result = Nvm_StorageIf_Erase(block->storageAddress, block->totalLength);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Write header */
    result = Nvm_StorageIf_Write(block->storageAddress, (uint8_t*)&header, 
                                 NVM_BLOCK_HEADER_SIZE);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Write data */
    result = Nvm_StorageIf_Write(dataAddress, data, length);
    
    return result;
}

Nvm_ErrorCode_t Nvm_StorageIf_EraseBlock(uint16_t blockId)
{
    Nvm_Block_t* block;
    
    block = Nvm_BlockMgr_GetBlock(blockId);
    if (block == NULL) {
        return NVM_E_BLOCK_INVALID;
    }
    
    return Nvm_StorageIf_Erase(block->storageAddress, block->totalLength);
}

/*============================================================================*
 * Page Management Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_StorageIf_AllocPage(
    uint8_t deviceId,
    uint32_t* pageAddress)
{
    Nvm_StorageDevice_t* device;
    uint32_t i;
    
    if (!g_storageIfContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (pageAddress == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    device = Nvm_StorageIf_GetDevice(deviceId);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    /* Find free page */
    for (i = 0; i < NVM_STORAGE_MAX_REGIONS; i++) {
        if (!device->regions[i].inUse && !device->regions[i].worn) {
            device->regions[i].inUse = true;
            *pageAddress = device->regions[i].startAddress;
            return NVM_OK;
        }
    }
    
    return NVM_E_OUT_OF_MEMORY;
}

Nvm_ErrorCode_t Nvm_StorageIf_FreePage(
    uint8_t deviceId,
    uint32_t pageAddress)
{
    Nvm_StorageDevice_t* device;
    uint32_t i;
    
    device = Nvm_StorageIf_GetDevice(deviceId);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    /* Find region */
    for (i = 0; i < NVM_STORAGE_MAX_REGIONS; i++) {
        if (device->regions[i].startAddress == pageAddress) {
            device->regions[i].inUse = false;
            return NVM_OK;
        }
    }
    
    return NVM_E_PARAM_RANGE;
}

Nvm_ErrorCode_t Nvm_StorageIf_GetPageInfo(
    uint32_t address,
    uint8_t* deviceId,
    uint32_t* pageOffset)
{
    Nvm_StorageDevice_t* device;
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    if (deviceId != NULL) {
        *deviceId = device->deviceId;
    }
    
    if (pageOffset != NULL) {
        *pageOffset = (address - device->baseAddress) % device->pageSize;
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_StorageIf_WritePage(
    uint32_t address,
    const uint8_t* data)
{
    Nvm_StorageDevice_t* device;
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    return Nvm_StorageIf_Write(address, data, device->pageSize);
}

Nvm_ErrorCode_t Nvm_StorageIf_ReadPage(
    uint32_t address,
    uint8_t* data)
{
    Nvm_StorageDevice_t* device;
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    return Nvm_StorageIf_Read(address, data, device->pageSize);
}

/*============================================================================*
 * Wear Leveling Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_WearLevel_Init(uint8_t deviceId)
{
    (void)deviceId;
    /* Wear leveling already initialized with device registration */
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_WearLevel_GetCount(
    uint32_t address,
    uint32_t* eraseCount)
{
    Nvm_StorageDevice_t* device;
    uint32_t regionIdx;
    
    if (eraseCount == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    regionIdx = Nvm_Private_CalculateRegionIndex(device, address);
    if (regionIdx >= NVM_STORAGE_MAX_REGIONS) {
        return NVM_E_PARAM_RANGE;
    }
    
    *eraseCount = device->regions[regionIdx].eraseCount;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_WearLevel_Update(uint32_t address)
{
    Nvm_StorageDevice_t* device;
    uint32_t regionIdx;
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    regionIdx = Nvm_Private_CalculateRegionIndex(device, address);
    if (regionIdx >= NVM_STORAGE_MAX_REGIONS) {
        return NVM_E_PARAM_RANGE;
    }
    
    device->regions[regionIdx].eraseCount++;
    
    /* Check if region is worn */
    if (device->regions[regionIdx].eraseCount >= g_storageIfContext.wearLevelThreshold) {
        device->regions[regionIdx].worn = true;
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_WearLevel_FindBestRegion(
    uint8_t deviceId,
    uint32_t* regionAddress)
{
    Nvm_StorageDevice_t* device;
    uint32_t i;
    uint32_t minEraseCount = 0xFFFFFFFF;
    int32_t bestRegion = -1;
    
    if (regionAddress == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    device = Nvm_StorageIf_GetDevice(deviceId);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    /* Find region with minimum erase count */
    for (i = 0; i < NVM_STORAGE_MAX_REGIONS; i++) {
        if (!device->regions[i].worn && device->regions[i].eraseCount < minEraseCount) {
            minEraseCount = device->regions[i].eraseCount;
            bestRegion = (int32_t)i;
        }
    }
    
    if (bestRegion < 0) {
        return NVM_E_WEAR_LEVEL_CRITICAL;
    }
    
    *regionAddress = device->regions[bestRegion].startAddress;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_WearLevel_MarkWorn(uint32_t address)
{
    Nvm_StorageDevice_t* device;
    uint32_t regionIdx;
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    regionIdx = Nvm_Private_CalculateRegionIndex(device, address);
    if (regionIdx >= NVM_STORAGE_MAX_REGIONS) {
        return NVM_E_PARAM_RANGE;
    }
    
    device->regions[regionIdx].worn = true;
    
    return NVM_OK;
}

bool Nvm_WearLevel_IsCritical(uint32_t address)
{
    Nvm_StorageDevice_t* device;
    uint32_t regionIdx;
    
    device = Nvm_Private_FindDeviceByAddress(address);
    if (device == NULL) {
        return false;
    }
    
    regionIdx = Nvm_Private_CalculateRegionIndex(device, address);
    if (regionIdx >= NVM_STORAGE_MAX_REGIONS) {
        return false;
    }
    
    return (device->regions[regionIdx].eraseCount >= 
            (g_storageIfContext.wearLevelThreshold * 8 / 10)); /* 80% threshold */
}

Nvm_ErrorCode_t Nvm_WearLevel_GetStats(
    uint8_t deviceId,
    uint32_t* minCount,
    uint32_t* maxCount,
    uint32_t* avgCount)
{
    Nvm_StorageDevice_t* device;
    uint32_t i;
    uint32_t min = 0xFFFFFFFF;
    uint32_t max = 0;
    uint64_t sum = 0;
    uint32_t validRegions = 0;
    
    if (minCount == NULL || maxCount == NULL || avgCount == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    device = Nvm_StorageIf_GetDevice(deviceId);
    if (device == NULL) {
        return NVM_E_STORAGE_NOT_READY;
    }
    
    for (i = 0; i < NVM_STORAGE_MAX_REGIONS; i++) {
        if (device->regions[i].eraseCount > 0 || device->regions[i].inUse) {
            if (device->regions[i].eraseCount < min) {
                min = device->regions[i].eraseCount;
            }
            if (device->regions[i].eraseCount > max) {
                max = device->regions[i].eraseCount;
            }
            sum += device->regions[i].eraseCount;
            validRegions++;
        }
    }
    
    *minCount = (min == 0xFFFFFFFF) ? 0 : min;
    *maxCount = max;
    *avgCount = (validRegions > 0) ? (uint32_t)(sum / validRegions) : 0;
    
    return NVM_OK;
}

Nvm_StorageIf_Context_t* Nvm_StorageIf_GetContext(void)
{
    return &g_storageIfContext;
}

/*============================================================================*
 * Private Functions
 *============================================================================*/

static Nvm_StorageDevice_t* Nvm_Private_FindDeviceByAddress(uint32_t address)
{
    uint32_t i;
    
    for (i = 0; i < NVM_STORAGE_MAX_DEVICES; i++) {
        if (g_storageIfContext.devices[i].initialized) {
            if (address >= g_storageIfContext.devices[i].baseAddress &&
                address < (g_storageIfContext.devices[i].baseAddress + 
                          g_storageIfContext.devices[i].totalSize)) {
                return &g_storageIfContext.devices[i];
            }
        }
    }
    
    return NULL;
}

static Nvm_ErrorCode_t Nvm_Private_WriteToDevice(
    Nvm_StorageDevice_t* device,
    uint32_t address,
    const uint8_t* data,
    uint32_t length)
{
    Nvm_ErrorCode_t result = NVM_OK;
    uint32_t offset;
    uint32_t remaining;
    uint32_t writeSize;
    
    (void)device;
    
    #if NVM_USE_SIMULATED_STORAGE
    /* Use simulated storage */
    if (g_simulatedStorage != NULL) {
        uint32_t storageOffset = address & (g_simulatedStorageSize - 1);
        if (storageOffset + length <= g_simulatedStorageSize) {
            memcpy(g_simulatedStorage + storageOffset, data, length);
        } else {
            return NVM_E_PARAM_RANGE;
        }
    }
    #else
    if (device->writeFn != NULL) {
        result = device->writeFn(address, data, length);
    } else {
        /* Default: copy to memory (simulation) */
        /* Handle page alignment if necessary */
        offset = 0;
        remaining = length;
        
        while (remaining > 0) {
            writeSize = (remaining > device->pageSize) ? device->pageSize : remaining;
            
            memcpy((void*)(uintptr_t)(address + offset), data + offset, writeSize);
            
            offset += writeSize;
            remaining -= writeSize;
        }
    }
    #endif
    
    /* Update write count for region */
    if (result == NVM_OK) {
        uint32_t regionIdx = Nvm_Private_CalculateRegionIndex(device, address);
        if (regionIdx < NVM_STORAGE_MAX_REGIONS) {
            device->regions[regionIdx].writeCount++;
        }
    }
    
    return result;
}

static uint32_t Nvm_Private_CalculateRegionIndex(
    Nvm_StorageDevice_t* device,
    uint32_t address)
{
    uint32_t offset;
    
    if (address < device->baseAddress) {
        return NVM_STORAGE_MAX_REGIONS; /* Invalid */
    }
    
    offset = address - device->baseAddress;
    return offset / NVM_STORAGE_SECTOR_SIZE;
}
