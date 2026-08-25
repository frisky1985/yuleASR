/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file NvM_Redundant.c
 * @brief NVM Redundant Storage implementation
 * @details Implements data redundancy and recovery mechanisms
 */

#include "NvM.h"
#include "NvM_Redundant.h"
#include "NvM_Cfg.h"
#include "Crc.h"
#include <string.h>

#if (NVM_REDUNDANT_STORAGE_ENABLED == STD_ON)

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NVM_REDUNDANT_BLOCK_STATE_VALID     (0x01U)
#define NVM_REDUNDANT_BLOCK_STATE_INVALID   (0x02U)
#define NVM_REDUNDANT_BLOCK_STATE_INCONSISTENT (0x03U)

/*******************************************************************************
 * Types
 ******************************************************************************/
typedef enum
{
    NVM_RED_INSTANCE_PRIMARY = 0,
    NVM_RED_INSTANCE_MIRROR,
    NVM_RED_INSTANCE_COUNT
} NvM_RedundantInstanceType;

typedef struct
{
    uint8 State;
    uint8 InstanceIdx;
    uint16 Crc;
    uint32 SequenceNumber;
} NvM_RedundantBlockInfoType;

/*******************************************************************************
 * Local Variables
 ******************************************************************************/
static NvM_RedundantBlockInfoType NvM_RedundantBlockInfo[NVM_NUM_REDUNDANT_BLOCKS];
static uint32 NvM_RedundantSequenceCounter = 0U;

/*******************************************************************************
 * Local Functions
 ******************************************************************************/
static Std_ReturnType NvM_RedundantReadBlock(
    NvM_BlockIdType BlockId,
    NvM_RedundantInstanceType Instance,
    uint8* DestPtr);

static Std_ReturnType NvM_RedundantWriteBlock(
    NvM_BlockIdType BlockId,
    NvM_RedundantInstanceType Instance,
    const uint8* SrcPtr);

static uint16 NvM_CalculateCRC16(const uint8* DataPtr, uint32 Length);

static const NvM_BlockDescriptorType* NvM_Redundant_GetBlockDescriptor(NvM_BlockIdType BlockId);

/*******************************************************************************
 * API Functions
 ******************************************************************************/

/**
 * @brief Write block with redundancy
 */
Std_ReturnType NvM_RedundantWrite(NvM_BlockIdType BlockId, const uint8* SrcPtr)
{
    Std_ReturnType RetVal;
    uint16 Crc;
    uint16 BlockLength;
    const NvM_BlockDescriptorType* BlockDesc;
    
    if (BlockId >= NVM_NUM_REDUNDANT_BLOCKS)
    {
        return E_NOT_OK;
    }
    
    BlockDesc = NvM_Redundant_GetBlockDescriptor(BlockId);
    if (BlockDesc == NULL_PTR)
    {
        return E_NOT_OK;
    }
    BlockLength = BlockDesc->NvBlockLength;
    
    /* Calculate CRC */
    Crc = NvM_CalculateCRC16(SrcPtr, BlockLength);
    
    /* Write to primary instance */
    RetVal = NvM_RedundantWriteBlock(BlockId, NVM_RED_INSTANCE_PRIMARY, SrcPtr);
    
    if (RetVal == E_OK)
    {
        /* Write to mirror instance */
        RetVal = NvM_RedundantWriteBlock(BlockId, NVM_RED_INSTANCE_MIRROR, SrcPtr);
        
        if (RetVal == E_OK)
        {
            /* Update block info */
            NvM_RedundantBlockInfo[BlockId].State = NVM_REDUNDANT_BLOCK_STATE_VALID;
            NvM_RedundantBlockInfo[BlockId].Crc = Crc;
            NvM_RedundantBlockInfo[BlockId].SequenceNumber = NvM_RedundantSequenceCounter++;
        }
        else
        {
            /* Mirror write failed, mark as inconsistent */
            NvM_RedundantBlockInfo[BlockId].State = NVM_REDUNDANT_BLOCK_STATE_INCONSISTENT;
        }
    }
    
    return RetVal;
}

/**
 * @brief Read block with automatic recovery
 */
Std_ReturnType NvM_RedundantRead(NvM_BlockIdType BlockId, uint8* DestPtr)
{
    Std_ReturnType PrimaryRetVal;
    Std_ReturnType MirrorRetVal;
    uint8 PrimaryBuffer[NVM_MAX_BLOCK_SIZE];
    uint8 MirrorBuffer[NVM_MAX_BLOCK_SIZE];
    uint16 PrimaryCrc;
    uint16 MirrorCrc;
    boolean PrimaryValid;
    boolean MirrorValid;
    uint16 BlockLength;
    const NvM_BlockDescriptorType* BlockDesc;
    
    if (BlockId >= NVM_NUM_REDUNDANT_BLOCKS)
    {
        return E_NOT_OK;
    }
    
    BlockDesc = NvM_Redundant_GetBlockDescriptor(BlockId);
    if (BlockDesc == NULL_PTR)
    {
        return E_NOT_OK;
    }
    BlockLength = BlockDesc->NvBlockLength;
    
    /* Read both instances */
    PrimaryRetVal = NvM_RedundantReadBlock(BlockId, NVM_RED_INSTANCE_PRIMARY, PrimaryBuffer);
    MirrorRetVal = NvM_RedundantReadBlock(BlockId, NVM_RED_INSTANCE_MIRROR, MirrorBuffer);
    
    /* Calculate CRCs */
    PrimaryCrc = NvM_CalculateCRC16(PrimaryBuffer, BlockLength);
    MirrorCrc = NvM_CalculateCRC16(MirrorBuffer, BlockLength);
    
    /* Check validity */
    PrimaryValid = (PrimaryRetVal == E_OK) && (PrimaryCrc == NvM_RedundantBlockInfo[BlockId].Crc);
    MirrorValid = (MirrorRetVal == E_OK) && (MirrorCrc == NvM_RedundantBlockInfo[BlockId].Crc);
    
    /* Determine which data to use */
    if (PrimaryValid && MirrorValid)
    {
        /* Both valid - use primary, check sequence numbers */
        memcpy(DestPtr, PrimaryBuffer, BlockLength);
        return E_OK;
    }
    else if (PrimaryValid)
    {
        /* Primary valid, mirror corrupt - recover mirror */
        memcpy(DestPtr, PrimaryBuffer, BlockLength);
        NvM_RedundantWriteBlock(BlockId, NVM_RED_INSTANCE_MIRROR, PrimaryBuffer);
        NvM_RedundantBlockInfo[BlockId].State = NVM_REDUNDANT_BLOCK_STATE_INCONSISTENT;
        return E_OK;
    }
    else if (MirrorValid)
    {
        /* Mirror valid, primary corrupt - recover primary */
        memcpy(DestPtr, MirrorBuffer, BlockLength);
        NvM_RedundantWriteBlock(BlockId, NVM_RED_INSTANCE_PRIMARY, MirrorBuffer);
        NvM_RedundantBlockInfo[BlockId].State = NVM_REDUNDANT_BLOCK_STATE_INCONSISTENT;
        return E_OK;
    }
    else
    {
        /* Both corrupt - implicit recovery failed */
        NvM_RedundantBlockInfo[BlockId].State = NVM_REDUNDANT_BLOCK_STATE_INVALID;
        return E_NOT_OK;
    }
}

/**
 * @brief Check block consistency
 */
Std_ReturnType NvM_RedundantCheckConsistency(NvM_BlockIdType BlockId)
{
    Std_ReturnType RetVal;
    uint8 PrimaryBuffer[NVM_MAX_BLOCK_SIZE];
    uint8 MirrorBuffer[NVM_MAX_BLOCK_SIZE];
    uint16 BlockLength;
    const NvM_BlockDescriptorType* BlockDesc;
    
    if (BlockId >= NVM_NUM_REDUNDANT_BLOCKS)
    {
        return E_NOT_OK;
    }
    
    BlockDesc = NvM_Redundant_GetBlockDescriptor(BlockId);
    if (BlockDesc == NULL_PTR)
    {
        return E_NOT_OK;
    }
    BlockLength = BlockDesc->NvBlockLength;
    
    /* Read both instances */
    RetVal = NvM_RedundantReadBlock(BlockId, NVM_RED_INSTANCE_PRIMARY, PrimaryBuffer);
    if (RetVal != E_OK)
    {
        return E_NOT_OK;
    }
    
    RetVal = NvM_RedundantReadBlock(BlockId, NVM_RED_INSTANCE_MIRROR, MirrorBuffer);
    if (RetVal != E_OK)
    {
        return E_NOT_OK;
    }
    
    /* Compare */
    if (memcmp(PrimaryBuffer, MirrorBuffer, BlockLength) == 0U )
    {
        return E_OK;
    }
    else
    {
        return E_NOT_OK;
    }
}

/**
 * @brief Repair inconsistent block
 */
Std_ReturnType NvM_RedundantRepair(NvM_BlockIdType BlockId)
{
    Std_ReturnType RetVal;
    uint8 PrimaryBuffer[NVM_MAX_BLOCK_SIZE];
    uint8 MirrorBuffer[NVM_MAX_BLOCK_SIZE];
    uint16 PrimaryCrc;
    uint16 MirrorCrc;
    uint16 BlockLength;
    const NvM_BlockDescriptorType* BlockDesc;
    
    if (BlockId >= NVM_NUM_REDUNDANT_BLOCKS)
    {
        return E_NOT_OK;
    }
    
    BlockDesc = NvM_Redundant_GetBlockDescriptor(BlockId);
    if (BlockDesc == NULL_PTR)
    {
        return E_NOT_OK;
    }
    BlockLength = BlockDesc->NvBlockLength;
    
    /* Read both instances */
    NvM_RedundantReadBlock(BlockId, NVM_RED_INSTANCE_PRIMARY, PrimaryBuffer);
    NvM_RedundantReadBlock(BlockId, NVM_RED_INSTANCE_MIRROR, MirrorBuffer);
    
    /* Calculate CRCs */
    PrimaryCrc = NvM_CalculateCRC16(PrimaryBuffer, BlockLength);
    MirrorCrc = NvM_CalculateCRC16(MirrorBuffer, BlockLength);
    
    /* Try to determine valid data */
    if (PrimaryCrc == NvM_RedundantBlockInfo[BlockId].Crc)
    {
        /* Primary seems valid - copy to mirror */
        RetVal = NvM_RedundantWriteBlock(BlockId, NVM_RED_INSTANCE_MIRROR, PrimaryBuffer);
    }
    else if (MirrorCrc == NvM_RedundantBlockInfo[BlockId].Crc)
    {
        /* Mirror seems valid - copy to primary */
        RetVal = NvM_RedundantWriteBlock(BlockId, NVM_RED_INSTANCE_PRIMARY, MirrorBuffer);
    }
    else
    {
        /* Neither valid - use default/ROM data */
        RetVal = E_NOT_OK;
    }
    
    if (RetVal == E_OK)
    {
        NvM_RedundantBlockInfo[BlockId].State = NVM_REDUNDANT_BLOCK_STATE_VALID;
    }
    
    return RetVal;
}

/*******************************************************************************
 * Local Functions
 ******************************************************************************/

static const NvM_BlockDescriptorType* NvM_Redundant_GetBlockDescriptor(NvM_BlockIdType BlockId)
{
    uint16 i;
    NvM_BlockIdType PrimaryBlockId;
    const NvM_BlockDescriptorType* result = NULL_PTR;

    /* Redundant group uses two consecutive NV blocks: primary = BlockId*2, mirror = BlockId*2+1 */
    PrimaryBlockId = (NvM_BlockIdType)((uint16)BlockId * NVM_RED_INSTANCE_COUNT);

    for (i = 0U; i < NvM_Config.NumBlockDescriptors; i++)
    {
        if (NvM_Config.BlockDescriptors[i].BlockId == PrimaryBlockId)
        {
            result = &NvM_Config.BlockDescriptors[i];
            break;
        }
    }

    return result;
}

static Std_ReturnType NvM_RedundantReadBlock(
    NvM_BlockIdType BlockId,
    NvM_RedundantInstanceType Instance,
    uint8* DestPtr)
{
    NvM_BlockIdType ActualBlockId;
    
    /* Calculate actual block ID based on instance */
    ActualBlockId = (NvM_BlockIdType)((BlockId * NVM_RED_INSTANCE_COUNT) + Instance);
    
    /* Read via standard NVM read */
    return NvM_ReadBlock(ActualBlockId, DestPtr);
}

static Std_ReturnType NvM_RedundantWriteBlock(
    NvM_BlockIdType BlockId,
    NvM_RedundantInstanceType Instance,
    const uint8* SrcPtr)
{
    NvM_BlockIdType ActualBlockId;
    
    /* Calculate actual block ID based on instance */
    ActualBlockId = (NvM_BlockIdType)((BlockId * NVM_RED_INSTANCE_COUNT) + Instance);
    
    /* Write via standard NVM write */
    return NvM_WriteBlock(ActualBlockId, SrcPtr);
}

static uint16 NvM_CalculateCRC16(const uint8* DataPtr, uint32 Length)
{
    /* Use CRC library */
    return Crc_CalculateCRC16(DataPtr, Length, 0xFFFFU, TRUE);
}

#endif /* NVM_REDUNDANT_STORAGE_ENABLED */
