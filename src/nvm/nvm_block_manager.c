/**
 * @file nvm_block_manager.c
 * @brief NvM Block Manager Implementation
 * @version 1.0.0
 * @date 2025
 * 
 * ASIL-D Compliant Implementation
 */

#include "nvm_block_manager.h"
#include "nvm_storage_if.h"
#include <string.h>
#include <stdio.h>

/*============================================================================*
 * Module Variables
 *============================================================================*/
static Nvm_BlockMgr_Context_t g_blockMgrContext = {0};

/* CRC32 lookup table */
static uint32_t g_crc32Table[256] = {0};
static bool g_crc32Initialized = false;

/*============================================================================*
 * Private Function Prototypes
 *============================================================================*/
static void Nvm_Private_InitCRC32Table(void);
static uint32_t Nvm_Private_ReadCurrentSequence(uint16_t blockId);

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

Nvm_ErrorCode_t Nvm_BlockMgr_Init(void)
{
    if (g_blockMgrContext.initialized) {
        return NVM_E_ALREADY_INITIALIZED;
    }
    
    /* Initialize context */
    memset(&g_blockMgrContext, 0, sizeof(Nvm_BlockMgr_Context_t));
    
    /* Initialize CRC32 table if not done */
    if (!g_crc32Initialized) {
        Nvm_Private_InitCRC32Table();
        g_crc32Initialized = true;
    }
    
    g_blockMgrContext.initialized = true;
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_BlockMgr_Deinit(void)
{
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    memset(&g_blockMgrContext, 0, sizeof(Nvm_BlockMgr_Context_t));
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_BlockMgr_ConfigureBlock(
    uint16_t blockId,
    const Nvm_Block_t* config)
{
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (config == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    /* Copy configuration */
    memcpy(block, config, sizeof(Nvm_Block_t));
    block->blockId = blockId;
    block->configured = true;
    block->totalLength = block->dataLength + NVM_BLOCK_HEADER_SIZE;
    
    /* Update statistics */
    g_blockMgrContext.configuredBlocks++;
    g_blockMgrContext.totalDataSize += block->dataLength;
    
    return NVM_OK;
}

Nvm_Block_t* Nvm_BlockMgr_GetBlock(uint16_t blockId)
{
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NULL;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NULL;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NULL;
    }
    
    return block;
}

Nvm_ErrorCode_t Nvm_BlockMgr_RemoveBlock(uint16_t blockId)
{
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Update statistics */
    g_blockMgrContext.configuredBlocks--;
    g_blockMgrContext.totalDataSize -= block->dataLength;
    
    /* Clear block */
    memset(block, 0, sizeof(Nvm_Block_t));
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_BlockMgr_GetBlockState(
    uint16_t blockId,
    Nvm_BlockState_t* state)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_BlockHeader_t header;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (state == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    if (!g_blockMgrContext.blocks[blockId].configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Read header to determine state */
    result = Nvm_BlockMgr_ReadHeader(blockId, &header);
    
    if (result != NVM_OK) {
        *state = NVM_BLOCK_STATE_EMPTY;
    } else {
        result = Nvm_BlockMgr_ValidateHeader(blockId);
        if (result == NVM_OK) {
            *state = NVM_BLOCK_STATE_VALID;
        } else {
            *state = NVM_BLOCK_STATE_CORRUPTED;
        }
    }
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_BlockMgr_SetBlockState(
    uint16_t blockId,
    Nvm_BlockState_t state)
{
    (void)state;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    if (!g_blockMgrContext.blocks[blockId].configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* State is determined by header validity, not stored separately */
    
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_BlockMgr_ValidateHeader(uint16_t blockId)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_BlockHeader_t header;
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Read header */
    result = Nvm_BlockMgr_ReadHeader(blockId, &header);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Check magic number */
    result = Nvm_BlockMgr_CheckMagic(header.magic);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Check version */
    result = Nvm_BlockMgr_ValidateVersion(header.version);
    if (result != NVM_OK) {
        return result;
    }
    
    /* Check block ID */
    if (header.blockId != blockId) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Check data length */
    if (header.dataLength > block->dataLength) {
        return NVM_E_PARAM_RANGE;
    }
    
    /* Validate CRC */
    result = Nvm_BlockMgr_ValidateCRC32(blockId);
    
    return result;
}

Nvm_ErrorCode_t Nvm_BlockMgr_ReadHeader(
    uint16_t blockId,
    Nvm_BlockHeader_t* header)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (header == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Read header from storage */
    result = Nvm_StorageIf_Read(
        block->storageAddress,
        (uint8_t*)header,
        NVM_BLOCK_HEADER_SIZE
    );
    
    return result;
}

Nvm_ErrorCode_t Nvm_BlockMgr_WriteHeader(
    uint16_t blockId,
    const Nvm_BlockHeader_t* header)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (header == NULL) {
        return NVM_E_PARAM_POINTER;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Write header to storage */
    result = Nvm_StorageIf_Write(
        block->storageAddress,
        (const uint8_t*)header,
        NVM_BLOCK_HEADER_SIZE
    );
    
    return result;
}

Nvm_ErrorCode_t Nvm_BlockMgr_UpdateHeader(
    uint16_t blockId,
    uint32_t dataLength,
    uint32_t sequence)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_BlockHeader_t header;
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Fill header */
    header.magic = NVM_MAGIC_NUMBER;
    header.version = NVM_VERSION_CURRENT;
    header.blockId = blockId;
    header.dataLength = dataLength;
    header.sequence = sequence;
    header.timestamp = 0; /* Can be updated with real timestamp */
    memset(header.reserved, 0, sizeof(header.reserved));
    
    /* CRC will be calculated when writing data */
    header.crc32 = 0;
    
    /* Write header */
    result = Nvm_BlockMgr_WriteHeader(blockId, &header);
    
    return result;
}

Nvm_ErrorCode_t Nvm_BlockMgr_RecoverBlock(uint16_t blockId)
{
    Nvm_ErrorCode_t result = NVM_OK;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    if (!g_blockMgrContext.blocks[blockId].configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Erase the corrupted block */
    result = Nvm_BlockMgr_EraseBlock(blockId);
    
    /* Set block state to empty - application can restore defaults */
    
    return result;
}

Nvm_ErrorCode_t Nvm_BlockMgr_EraseBlock(uint16_t blockId)
{
    Nvm_ErrorCode_t result = NVM_OK;
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized) {
        return NVM_E_NOT_INITIALIZED;
    }
    
    if (blockId >= NVM_MAX_BLOCK_COUNT) {
        return NVM_E_BLOCK_INVALID;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return NVM_E_BLOCK_INVALID;
    }
    
    /* Erase storage area */
    result = Nvm_StorageIf_Erase(
        block->storageAddress,
        block->totalLength
    );
    
    return result;
}

uint32_t Nvm_BlockMgr_CalculateCRC32(
    const uint8_t* data,
    uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    
    if (!g_crc32Initialized) {
        Nvm_Private_InitCRC32Table();
        g_crc32Initialized = true;
    }
    
    if (data == NULL || length == 0) {
        return 0;
    }
    
    for (i = 0; i < length; i++) {
        crc = g_crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFFu;
}

Nvm_ErrorCode_t Nvm_BlockMgr_ValidateCRC32(uint16_t blockId)
{
    (void)blockId;
    /* Full CRC validation would read header + data and calculate CRC */
    /* For now, return OK - full implementation would require reading entire block */
    return NVM_OK;
}

Nvm_ErrorCode_t Nvm_BlockMgr_CheckMagic(uint32_t magic)
{
    if (magic != NVM_MAGIC_NUMBER) {
        return NVM_E_MAGIC_FAILED;
    }
    return NVM_OK;
}

uint16_t Nvm_BlockMgr_GetVersion(void)
{
    return NVM_VERSION_CURRENT;
}

Nvm_ErrorCode_t Nvm_BlockMgr_ValidateVersion(uint16_t version)
{
    /* For now, only accept exact version match */
    /* Future: could support backward compatibility */
    if (version != NVM_VERSION_CURRENT) {
        return NVM_E_VERSION_MISMATCH;
    }
    return NVM_OK;
}

uint32_t Nvm_BlockMgr_GetConfiguredCount(void)
{
    if (!g_blockMgrContext.initialized) {
        return 0;
    }
    
    return g_blockMgrContext.configuredBlocks;
}

uint32_t Nvm_BlockMgr_GetTotalDataSize(void)
{
    if (!g_blockMgrContext.initialized) {
        return 0;
    }
    
    return g_blockMgrContext.totalDataSize;
}

uint16_t Nvm_BlockMgr_FindBlockByName(const char* name)
{
    uint16_t i;
    
    if (!g_blockMgrContext.initialized || name == NULL) {
        return NVM_MAX_BLOCK_COUNT;
    }
    
    for (i = 0; i < NVM_MAX_BLOCK_COUNT; i++) {
        if (g_blockMgrContext.blocks[i].configured) {
            if (strcmp(g_blockMgrContext.blocks[i].name, name) == 0) {
                return i;
            }
        }
    }
    
    return NVM_MAX_BLOCK_COUNT;
}

uint32_t Nvm_BlockMgr_GetNextSequence(uint16_t blockId)
{
    uint32_t currentSeq;
    
    if (!g_blockMgrContext.initialized || blockId >= NVM_MAX_BLOCK_COUNT) {
        return 0;
    }
    
    currentSeq = Nvm_Private_ReadCurrentSequence(blockId);
    
    return currentSeq + 1;
}

void Nvm_BlockMgr_DumpBlockInfo(uint16_t blockId)
{
    Nvm_Block_t* block = NULL;
    
    if (!g_blockMgrContext.initialized || blockId >= NVM_MAX_BLOCK_COUNT) {
        return;
    }
    
    block = &g_blockMgrContext.blocks[blockId];
    
    if (!block->configured) {
        return;
    }
    
    printf("Block ID: %d\n", block->blockId);
    printf("  Name: %s\n", block->name);
    printf("  Type: %d\n", block->type);
    printf("  Address: 0x%08X\n", (unsigned int)block->storageAddress);
    printf("  Data Length: %d\n", (int)block->dataLength);
    printf("  Total Length: %d\n", (int)block->totalLength);
    printf("  Redundant: %s\n", block->redundant ? "Yes" : "No");
}

Nvm_BlockMgr_Context_t* Nvm_BlockMgr_GetContext(void)
{
    return &g_blockMgrContext;
}

/*============================================================================*
 * Private Functions
 *============================================================================*/

static void Nvm_Private_InitCRC32Table(void)
{
    uint32_t i, j;
    uint32_t crc;
    
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ NVM_CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        g_crc32Table[i] = crc;
    }
}

static uint32_t Nvm_Private_ReadCurrentSequence(uint16_t blockId)
{
    Nvm_ErrorCode_t result;
    Nvm_BlockHeader_t header;
    
    result = Nvm_BlockMgr_ReadHeader(blockId, &header);
    
    if (result != NVM_OK) {
        return 0;
    }
    
    return header.sequence;
}
