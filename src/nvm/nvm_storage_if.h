/**
 * @file nvm_storage_if.h
 * @brief NvM Storage Interface Header
 * @version 1.0.0
 * @date 2025
 * 
 * Storage interface abstraction including:
 * - Flash/EEPROM abstraction
 * - Page management
 * - Wear leveling
 */

#ifndef NVM_STORAGE_IF_H
#define NVM_STORAGE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nvm_types.h"

/*============================================================================*
 * Storage Configuration
 *============================================================================*/
#define NVM_STORAGE_MAX_DEVICES     4u
#define NVM_STORAGE_MAX_REGIONS     16u
#define NVM_STORAGE_PAGE_SIZE       256u
#define NVM_STORAGE_SECTOR_SIZE     4096u

/*============================================================================*
 * Device Types
 *============================================================================*/
typedef enum {
    NVM_DEV_FLASH_INTERNAL,         /* Internal Flash */
    NVM_DEV_FLASH_EXTERNAL,         /* External Flash */
    NVM_DEV_EEPROM_INTERNAL,        /* Internal EEPROM */
    NVM_DEV_EEPROM_EXTERNAL,        /* External EEPROM */
    NVM_DEV_EMULATED_EEPROM         /* Emulated EEPROM on Flash */
} Nvm_StorageDeviceType_t;

/*============================================================================*
 * Storage Region (for wear leveling)
 *============================================================================*/
typedef struct {
    uint32_t startAddress;          /* Region start address */
    uint32_t size;                  /* Region size */
    uint32_t eraseCount;            /* Erase cycle count */
    uint32_t writeCount;            /* Write operation count */
    bool inUse;                     /* Region in use */
    bool worn;                      /* Region marked as worn */
} Nvm_StorageRegion_t;

/*============================================================================*
 * Storage Device
 *============================================================================*/
typedef struct {
    uint8_t deviceId;               /* Device identifier */
    Nvm_StorageDeviceType_t type;   /* Device type */
    uint32_t baseAddress;           /* Base address */
    uint32_t totalSize;             /* Total size */
    uint32_t pageSize;              /* Page size */
    uint32_t sectorSize;            /* Sector size */
    uint32_t eraseCycles;           /* Estimated erase cycles remaining */
    bool initialized;               /* Device initialized */
    Nvm_StorageRegion_t regions[NVM_STORAGE_MAX_REGIONS];
    Nvm_StorageRead_t readFn;       /* Read function */
    Nvm_StorageWrite_t writeFn;     /* Write function */
    Nvm_StorageErase_t eraseFn;     /* Erase function */
} Nvm_StorageDevice_t;

/*============================================================================*
 * Storage Interface Context
 *============================================================================*/
typedef struct {
    bool initialized;
    Nvm_StorageDevice_t devices[NVM_STORAGE_MAX_DEVICES];
    uint32_t activeDevices;
    uint32_t totalEraseCount;
    uint32_t wearLevelThreshold;    /* Threshold for wear warning */
} Nvm_StorageIf_Context_t;

/*============================================================================*
 * Storage Interface API
 *============================================================================*/

/**
 * @brief Initialize storage interface
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_Init(void);

/**
 * @brief Deinitialize storage interface
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_Deinit(void);

/**
 * @brief Register storage device
 * @param device Device configuration
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_RegisterDevice(
    const Nvm_StorageDevice_t* device
);

/**
 * @brief Unregister storage device
 * @param deviceId Device identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_UnregisterDevice(uint8_t deviceId);

/**
 * @brief Get storage device
 * @param deviceId Device identifier
 * @return Pointer to device, NULL if not found
 */
Nvm_StorageDevice_t* Nvm_StorageIf_GetDevice(uint8_t deviceId);

/**
 * @brief Read from storage
 * @param address Storage address
 * @param data Buffer to read into
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_Read(
    uint32_t address,
    uint8_t* data,
    uint32_t length
);

/**
 * @brief Write to storage
 * @param address Storage address
 * @param data Data to write
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_Write(
    uint32_t address,
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Erase storage region
 * @param address Start address
 * @param length Length to erase
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_Erase(
    uint32_t address,
    uint32_t length
);

/**
 * @brief Read block (header + data)
 * @param blockId Block ID
 * @param data Buffer to read into
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_ReadBlock(
    uint16_t blockId,
    uint8_t* data,
    uint32_t length
);

/**
 * @brief Write block (header + data)
 * @param blockId Block ID
 * @param data Data to write
 * @param length Data length
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_WriteBlock(
    uint16_t blockId,
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Erase block
 * @param blockId Block ID
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_EraseBlock(uint16_t blockId);

/*============================================================================*
 * Page Management API
 *============================================================================*/

/**
 * @brief Allocate page
 * @param deviceId Device identifier
 * @param pageAddress Pointer to store page address
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_AllocPage(
    uint8_t deviceId,
    uint32_t* pageAddress
);

/**
 * @brief Free page
 * @param deviceId Device identifier
 * @param pageAddress Page address
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_FreePage(
    uint8_t deviceId,
    uint32_t pageAddress
);

/**
 * @brief Get page info
 * @param address Page address
 * @param deviceId Pointer to store device ID
 * @param pageOffset Pointer to store page offset
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_GetPageInfo(
    uint32_t address,
    uint8_t* deviceId,
    uint32_t* pageOffset
);

/**
 * @brief Write page (aligned to page size)
 * @param address Page address
 * @param data Data to write
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_WritePage(
    uint32_t address,
    const uint8_t* data
);

/**
 * @brief Read page (aligned to page size)
 * @param address Page address
 * @param data Buffer to read into
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_StorageIf_ReadPage(
    uint32_t address,
    uint8_t* data
);

/*============================================================================*
 * Wear Leveling API
 *============================================================================*/

/**
 * @brief Initialize wear leveling for device
 * @param deviceId Device identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WearLevel_Init(uint8_t deviceId);

/**
 * @brief Get wear level for region
 * @param address Region address
 * @param eraseCount Pointer to store erase count
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WearLevel_GetCount(
    uint32_t address,
    uint32_t* eraseCount
);

/**
 * @brief Update wear level after erase
 * @param address Region address
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WearLevel_Update(uint32_t address);

/**
 * @brief Find least worn region
 * @param deviceId Device identifier
 * @param regionAddress Pointer to store address
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WearLevel_FindBestRegion(
    uint8_t deviceId,
    uint32_t* regionAddress
);

/**
 * @brief Mark region as worn
 * @param address Region address
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WearLevel_MarkWorn(uint32_t address);

/**
 * @brief Check if wear level is critical
 * @param address Region address
 * @return true if critical, false otherwise
 */
bool Nvm_WearLevel_IsCritical(uint32_t address);

/**
 * @brief Get wear level statistics
 * @param deviceId Device identifier
 * @param minCount Pointer to store minimum erase count
 * @param maxCount Pointer to store maximum erase count
 * @param avgCount Pointer to store average erase count
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_WearLevel_GetStats(
    uint8_t deviceId,
    uint32_t* minCount,
    uint32_t* maxCount,
    uint32_t* avgCount
);

/**
 * @brief Get storage interface context (internal use)
 * @return Pointer to context
 */
Nvm_StorageIf_Context_t* Nvm_StorageIf_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* NVM_STORAGE_IF_H */
