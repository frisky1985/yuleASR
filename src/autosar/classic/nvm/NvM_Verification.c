/**
 * @file NvM_Verification.c
 * @brief AUTOSAR NvM Write Verification Implementation
 * @version 4.4.0
 * @date 2025
 *
 * AUTOSAR Classic Platform - NvM Write Verification (Module ID: 0x0E)
 *
 * Implements write verification mechanisms:
 * - Read-after-write verification
 * - Data comparison
 * - CRC verification after write
 * - Block ID validation
 *
 * Features:
 * - Read-after-write verification
 * - CRC verification
 * - Static block ID checking
 * - Configurable verification per block
 *
 * Copyright (c) 2025
 */

#include "NvM_Private.h"
#include "MemIf.h"
#include <string.h>

/*============================================================================*
 * Local Variables
 *============================================================================*/
static uint8_t NvM_VerificationBuffer[NVM_MAX_BLOCK_LENGTH];

/*============================================================================*
 * Local Function Prototypes
 *============================================================================*/
static Std_ReturnType NvM_Verify_DataComparison(
    const void* WrittenData,
    const void* ReadData,
    uint32_t Length
);

static Std_ReturnType NvM_Verify_BlockId(NvM_BlockIdType BlockId, uint16_t ReadBlockId);

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

/**
 * @brief Verify a write operation by reading back and comparing
 * @param BlockId Block identifier
 * @param DataPtr Original data that was written
 * @param Length Data length
 * @return E_OK if verification passed, E_NOT_OK otherwise
 */
Std_ReturnType NvM_Verify_Write(
    NvM_BlockIdType BlockId,
    const void* DataPtr,
    uint32_t Length)
{
    const NvM_BlockDescriptorType* config;
    Std_ReturnType result;
    uint16_t blockNumber;
    
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return E_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    config = NvM_Global.Blocks[BlockId].Config;
    
    if (config == NULL_PTR) {
        return E_NOT_OK;
    }
    
    #if (NVM_WRITE_VERIFICATION == STD_OFF)
    (void)Length;
    return E_OK; /* Verification disabled */
    #endif
    
    /* Check if verification is enabled for this block */
    if (config->WriteVerification == FALSE) {
        return E_OK;
    }
    
    /* Limit length to configured block length */
    if (Length > config->NvBlockLength) {
        Length = config->NvBlockLength;
    }
    
    /* Calculate block number with dataset offset */
    blockNumber = config->NvBlockBaseNumber + NvM_Global.Blocks[BlockId].Status.DataIndex;
    
    /* Read back the data */
    result = MemIf_Read(
        config->NvramDeviceId,
        blockNumber,
        0u, /* Block offset */
        NvM_VerificationBuffer,
        (uint16_t)Length
    );
    
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    /* Wait for read to complete (should be handled by state machine in async mode) */
    /* For now, assume synchronous completion */
    
    /* Verify data by comparison */
    result = NvM_Verify_DataComparison(DataPtr, NvM_VerificationBuffer, Length);
    
    if (result != E_OK) {
        return E_NOT_OK;
    }
    
    /* Verify CRC if enabled */
    if (config->CalcRamBlockCrc == TRUE) {
        uint32_t expectedCrc;
        
        expectedCrc = NvM_Crc_Calculate(config->CrcType, DataPtr, Length);
        
        if (NvM_Crc_Verify(config->CrcType, NvM_VerificationBuffer, Length, expectedCrc) == FALSE) {
            /* CRC verification failed */
            NvM_Block_SetResult(BlockId, NVM_REQ_INTEGRITY_FAILED);
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/**
 * @brief Validate a block after read (check CRC, block ID)
 * @param BlockId Block identifier
 * @param DataPtr Data buffer to validate
 * @param Length Data length
 * @return E_OK if valid, E_NOT_OK otherwise
 */
Std_ReturnType NvM_Verify_ValidateBlock(
    NvM_BlockIdType BlockId,
    const void* DataPtr,
    uint32_t Length)
{
    const NvM_BlockDescriptorType* config;
    
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return E_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    config = NvM_Global.Blocks[BlockId].Config;
    
    if (config == NULL_PTR) {
        return E_NOT_OK;
    }
    
    #if (NVM_STATIC_BLOCK_ID_CHECK == STD_ON)
    /* Verify static block ID if enabled */
    if (config->StaticBlockIDCheck == TRUE) {
        uint16_t storedBlockId;
        /* Assume block ID is stored at the beginning of data */
        storedBlockId = ((const uint16_t*)DataPtr)[0];
        
        if (NvM_Verify_BlockId(BlockId, storedBlockId) != E_OK) {
            NvM_Block_SetResult(BlockId, NVM_REQ_INTEGRITY_FAILED);
            return E_NOT_OK;
        }
    }
    #endif
    
    /* Verify CRC if enabled */
    if (config->CalcRamBlockCrc == TRUE) {
        uint32_t storedCrc;
        uint32_t calculatedCrc;
        
        /* Assume CRC is stored after data or at specific offset */
        /* For this implementation, we calculate CRC on data excluding CRC field */
        calculatedCrc = NvM_Crc_Calculate(config->CrcType, DataPtr, Length - sizeof(uint32_t));
        
        /* Read stored CRC from end of data */
        storedCrc = ((const uint32_t*)DataPtr)[(Length / sizeof(uint32_t)) - 1];
        
        if (storedCrc != calculatedCrc) {
            NvM_Block_SetResult(BlockId, NVM_REQ_INTEGRITY_FAILED);
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/**
 * @brief Verify block header integrity
 * @param BlockId Block identifier
 * @param HeaderPtr Pointer to block header
 * @return E_OK if valid, E_NOT_OK otherwise
 */
Std_ReturnType NvM_Verify_BlockHeader(
    NvM_BlockIdType BlockId,
    const NvM_BlockHeaderType* HeaderPtr)
{
    uint32_t calculatedCrc;
    uint32_t headerSize;
    
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return E_NOT_OK;
    }
    
    if (HeaderPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Verify magic number */
    if (HeaderPtr->Magic != NVM_BLOCK_ID_MAGIC) {
        return E_NOT_OK;
    }
    
    /* Verify block ID */
    if (HeaderPtr->BlockId != BlockId) {
        return E_NOT_OK;
    }
    
    /* Verify header CRC (excluding CRC field itself) */
    headerSize = sizeof(NvM_BlockHeaderType) - sizeof(uint32_t);
    calculatedCrc = NvM_Crc32_Calculate((const uint8_t*)HeaderPtr, headerSize);
    
    if (calculatedCrc != HeaderPtr->HeaderCrc) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Calculate and append CRC to data buffer
 * @param BlockId Block identifier
 * @param DataPtr Data buffer
 * @param DataLength Length of actual data (excluding CRC)
 * @return E_OK on success, E_NOT_OK on failure
 */
Std_ReturnType NvM_Verify_AppendCrc(
    NvM_BlockIdType BlockId,
    uint8_t* DataPtr,
    uint32_t DataLength)
{
    const NvM_BlockDescriptorType* config;
    uint32_t crc;
    uint32_t crcOffset;
    
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return E_NOT_OK;
    }
    
    if (DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    config = NvM_Global.Blocks[BlockId].Config;
    
    if (config == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (config->CalcRamBlockCrc == FALSE) {
        return E_OK; /* CRC not required */
    }
    
    /* Calculate CRC */
    crc = NvM_Crc_Calculate(config->CrcType, DataPtr, DataLength);
    
    /* Append CRC after data */
    crcOffset = DataLength;
    
    switch (config->CrcType) {
        case NVM_CRC_8:
            DataPtr[crcOffset] = (uint8_t)crc;
            break;
            
        case NVM_CRC_16:
            DataPtr[crcOffset] = (uint8_t)(crc >> 8u);
            DataPtr[crcOffset + 1u] = (uint8_t)(crc & 0xFFu);
            break;
            
        case NVM_CRC_32:
            DataPtr[crcOffset] = (uint8_t)(crc >> 24u);
            DataPtr[crcOffset + 1u] = (uint8_t)(crc >> 16u);
            DataPtr[crcOffset + 2u] = (uint8_t)(crc >> 8u);
            DataPtr[crcOffset + 3u] = (uint8_t)(crc & 0xFFu);
            break;
            
        default:
            break;
    }
    
    /* Store CRC in block status */
    NvM_Global.Blocks[BlockId].CurrentCrc = crc;
    
    return E_OK;
}

/*============================================================================*
 * Local Functions
 *============================================================================*/

/**
 * @brief Compare written data with read-back data
 */
static Std_ReturnType NvM_Verify_DataComparison(
    const void* WrittenData,
    const void* ReadData,
    uint32_t Length)
{
    const uint8_t* written = (const uint8_t*)WrittenData;
    const uint8_t* read = (const uint8_t*)ReadData;
    uint32_t i;
    
    for (i = 0u; i < Length; i++) {
        if (written[i] != read[i]) {
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/**
 * @brief Verify block ID matches expected value
 */
static Std_ReturnType NvM_Verify_BlockId(NvM_BlockIdType BlockId, uint16_t ReadBlockId)
{
    /* Block ID in storage should match the requested block ID */
    if (ReadBlockId != BlockId) {
        return E_NOT_OK;
    }
    
    return E_OK;
}
