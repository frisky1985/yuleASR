/**
 * @file nvm_block_manager.h
 * @brief NvM Block Manager Header
 * @version 1.0.0
 * @date 2025
 * 
 * Block management including:
 * - Block configuration table
 * - Block state management
 * - CRC32 calculation
 * - Magic number validation
 * - Version management
 */

#ifndef NVM_BLOCK_MANAGER_H
#define NVM_BLOCK_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nvm_types.h"

/*============================================================================*
 * Block Configuration Structure
 *============================================================================*/
typedef struct {
    uint16_t blockId;                       /* Block identifier */
    char name[NVM_BLOCK_NAME_LEN];          /* Block name */
    Nvm_BlockType_t type;                   /* Block type */
    Nvm_Protection_t protection;            /* Protection level */
    Nvm_StorageType_t storageType;          /* Storage type */
    uint32_t storageAddress;                /* Storage address */
    uint32_t dataLength;                    /* Data length */
    uint32_t totalLength;                   /* Total length (data + header) */
    bool redundant;                         /* Redundancy enabled */
    uint16_t redundantBlockId;              /* Redundant block ID */
    uint8_t priority;                       /* Block priority (0-255) */
    bool configured;                        /* Block is configured */
    bool persistent;                        /* Persistent across resets */
} Nvm_Block_t;

/*============================================================================*
 * Block Manager Context
 *============================================================================*/
typedef struct {
    bool initialized;
    Nvm_Block_t blocks[NVM_MAX_BLOCK_COUNT];
    uint32_t configuredBlocks;
    uint32_t totalDataSize;
} Nvm_BlockMgr_Context_t;

/*============================================================================*
 * Block Manager API
 *============================================================================*/

/**
 * @brief Initialize block manager
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_Init(void);

/**
 * @brief Deinitialize block manager
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_Deinit(void);

/**
 * @brief Configure a block
 * @param blockId Block identifier
 * @param config Block configuration
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_ConfigureBlock(
    uint16_t blockId,
    const Nvm_Block_t* config
);

/**
 * @brief Get block configuration
 * @param blockId Block identifier
 * @return Pointer to block configuration, NULL if not found
 */
Nvm_Block_t* Nvm_BlockMgr_GetBlock(uint16_t blockId);

/**
 * @brief Remove block configuration
 * @param blockId Block identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_RemoveBlock(uint16_t blockId);

/**
 * @brief Get block state
 * @param blockId Block identifier
 * @param state Pointer to store state
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_GetBlockState(
    uint16_t blockId,
    Nvm_BlockState_t* state
);

/**
 * @brief Set block state
 * @param blockId Block identifier
 * @param state New state
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_SetBlockState(
    uint16_t blockId,
    Nvm_BlockState_t state
);

/**
 * @brief Validate block header
 * @param blockId Block identifier
 * @return NVM_OK if valid, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_ValidateHeader(uint16_t blockId);

/**
 * @brief Read block header from storage
 * @param blockId Block identifier
 * @param header Pointer to store header
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_ReadHeader(
    uint16_t blockId,
    Nvm_BlockHeader_t* header
);

/**
 * @brief Write block header to storage
 * @param blockId Block identifier
 * @param header Header to write
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_WriteHeader(
    uint16_t blockId,
    const Nvm_BlockHeader_t* header
);

/**
 * @brief Update block header with new data info
 * @param blockId Block identifier
 * @param dataLength Data length
 * @param sequence Sequence number
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_UpdateHeader(
    uint16_t blockId,
    uint32_t dataLength,
    uint32_t sequence
);

/**
 * @brief Recover corrupted block
 * @param blockId Block identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_RecoverBlock(uint16_t blockId);

/**
 * @brief Erase block data
 * @param blockId Block identifier
 * @return NVM_OK on success, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_EraseBlock(uint16_t blockId);

/**
 * @brief Calculate CRC32 for data
 * @param data Data buffer
 * @param length Data length
 * @return CRC32 value
 */
uint32_t Nvm_BlockMgr_CalculateCRC32(
    const uint8_t* data,
    uint32_t length
);

/**
 * @brief Validate CRC32 for block
 * @param blockId Block identifier
 * @return NVM_OK if CRC valid, error code otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_ValidateCRC32(uint16_t blockId);

/**
 * @brief Check magic number
 * @param magic Magic number to check
 * @return NVM_OK if valid, NVM_E_MAGIC_FAILED otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_CheckMagic(uint32_t magic);

/**
 * @brief Get current version
 * @return Current version number
 */
uint16_t Nvm_BlockMgr_GetVersion(void);

/**
 * @brief Validate version compatibility
 * @param version Version to check
 * @return NVM_OK if compatible, NVM_E_VERSION_MISMATCH otherwise
 */
Nvm_ErrorCode_t Nvm_BlockMgr_ValidateVersion(uint16_t version);

/**
 * @brief Get configured block count
 * @return Number of configured blocks
 */
uint32_t Nvm_BlockMgr_GetConfiguredCount(void);

/**
 * @brief Get total data size of all blocks
 * @return Total data size in bytes
 */
uint32_t Nvm_BlockMgr_GetTotalDataSize(void);

/**
 * @brief Find block by name
 * @param name Block name
 * @return Block ID or NVM_MAX_BLOCK_COUNT if not found
 */
uint16_t Nvm_BlockMgr_FindBlockByName(const char* name);

/**
 * @brief Get next sequence number for block
 * @param blockId Block identifier
 * @return Next sequence number
 */
uint32_t Nvm_BlockMgr_GetNextSequence(uint16_t blockId);

/**
 * @brief Dump block information (for debugging)
 * @param blockId Block identifier
 */
void Nvm_BlockMgr_DumpBlockInfo(uint16_t blockId);

/**
 * @brief Get block manager context (internal use)
 * @return Pointer to context
 */
Nvm_BlockMgr_Context_t* Nvm_BlockMgr_GetContext(void);

#ifdef __cplusplus
}
#endif

#endif /* NVM_BLOCK_MANAGER_H */
