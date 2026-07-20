/*==================================================================================================
 * NvM 写操作实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_SrcPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    /* Check write protection */
    if (blockDesc != NULL_PTR)
    {
        if ((blockDesc->BlockWriteProt == TRUE) ||
            (blockDesc->BlockWriteOnce == TRUE) ||
            (NvM_InternalState.BlockStates[BlockId].BlockLocked == TRUE))
        {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
            NVM_DET_REPORT_ERROR(0x05U, NVM_E_WRITE_PROTECTED);
#endif
            NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_NOT_OK;
            return E_NOT_OK;
        }
    }

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_WRITE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = (void*)NvM_SrcPtr;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;
        jobEntry.CopyIndex = 0U;

        /* Add to standard queue */
        if (NvM_QueuePush(NvM_InternalState.StandardQueue,
                          &NvM_InternalState.StandardQueueHead,
                          &NvM_InternalState.StandardQueueTail,
                          &NvM_InternalState.StandardQueueCount,
                          NVM_SIZE_STANDARD_JOB_QUEUE,
                          &jobEntry) == E_OK)
        {
            NvM_InternalState.BlockStates[BlockId].JobPending = 1U;
            NvM_InternalState.BlockStates[BlockId].DataChanged = TRUE;
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x05U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Restore block defaults from ROM
 * @param   BlockId     - Block identifier
 * @param   NvM_DestPtr - Destination pointer
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex)
{
    Std_ReturnType result = E_NOT_OK;
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x01U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    if (blockDesc != NULL_PTR)
    {
        if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
        {
            if (DataIndex < blockDesc->NumberOfDataSets)
            {
                NvM_InternalState.BlockStates[BlockId].DataIndex = DataIndex;
                result = E_OK;
            }
            else
            {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
                NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_DATA_IDX);
#endif
            }
        }
        else
        {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
            NVM_DET_REPORT_ERROR(0x01U, NVM_E_PARAM_BLOCK_TYPE);
#endif
        }
    }

    return result;
}

/**
 * @brief   Write block once (write-once protection)
 * @param   BlockId     - Block identifier
 * @param   NvM_SrcPtr  - Source pointer for write data
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WriteBlockOnce(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)
{
    Std_ReturnType result = E_NOT_OK;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_SrcPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if already written once */
    if (NvM_InternalState.BlockStates[BlockId].WriteOnceDone == TRUE)
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x0FU, NVM_E_WRITE_PROTECTED);
#endif
        NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_NOT_OK;
        return E_NOT_OK;
    }

    /* Delegate to regular write block */
    result = NvM_WriteBlock(BlockId, NvM_SrcPtr);

    return result;
}

/**
 * @brief   Set block lock status
 * @param   BlockId     - Block identifier
 * @param   BlockLocked - TRUE to lock, FALSE to unlock
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked)
{
    Std_ReturnType result = E_NOT_OK;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x13U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x13U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    if ((BlockId > 0U) && (BlockId < NVM_NUM_OF_NVRAM_BLOCKS))
    {
        NvM_InternalState.BlockStates[BlockId].BlockLocked = BlockLocked;
        result = E_OK;
    }

    return result;
}

/**
 * @brief   Set block protection
 * @param   BlockId            - Block identifier
 * @param   ProtectionEnabled  - TRUE to enable protection, FALSE to disable
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x14U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x14U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);
    if (blockDesc != NULL_PTR)
    {
        /* This is a runtime protection toggle; in a full implementation
           it would modify configuration. Here we treat it as block lock. */
        NvM_InternalState.BlockStates[BlockId].BlockLocked = ProtectionEnabled;
        return E_OK;
    }

    return E_NOT_OK;
}

/**
 * @brief   Set write once status
 * @param   BlockId   - Block identifier
 * @param   WriteOnce - TRUE to enable write-once, FALSE to disable
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_SetWriteOnceStatus(NvM_BlockIdType BlockId, boolean WriteOnce)
{
    (void)WriteOnce;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x15U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x15U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Write-once status is typically configuration-time only.
       Runtime modification is not supported in this implementation. */
    return E_NOT_OK;
}

/**
 * @brief   Read permanent RAM block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WritePRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x17U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x17U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);
    if ((blockDesc != NULL_PTR) && (blockDesc->RamBlockData != NULL_PTR))
    {
        return NvM_WriteBlock(BlockId, blockDesc->RamBlockData);
    }

    return E_NOT_OK;
}

/**
 * @brief   Cancel jobs for a block
 * @param   BlockId - Block identifier
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WriteAll(void)
{
    Std_ReturnType result = E_OK;
    const NvM_BlockDescriptorType* blockDesc;
    uint16 i;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEALL, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    if (NvM_InternalState.WriteAllInProgress == TRUE)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
    {
        blockDesc = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];

        if ((blockDesc->RamBlockData != NULL_PTR) &&
            (NvM_InternalState.BlockStates[blockDesc->BlockId].DataChanged == TRUE))
        {
            if (NvM_WriteBlock(blockDesc->BlockId, blockDesc->RamBlockData) == E_OK)
            {
                NvM_InternalState.WriteAllPendingCount++;
            }
            else
            {
                result = E_NOT_OK;
            }
        }
    }

    if (NvM_InternalState.WriteAllPendingCount > 0U)
    {
        NvM_InternalState.WriteAllInProgress = TRUE;
    }

    return result;
}

/**
 * @brief   Read a permanent RAM block (uses configured RamBlockData)
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_WritePRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEPRAMBLOCK, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEPRAMBLOCK, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    if ((blockDesc == NULL_PTR) || (blockDesc->RamBlockData == NULL_PTR))
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(NVM_SID_WRITEPRAMBLOCK, NVM_E_BLOCK_CONFIG);
#endif
        return E_NOT_OK;
    }

    return NvM_WriteBlock(BlockId, blockDesc->RamBlockData);
}

/**
 * @brief   Cancel all pending jobs for a block
 * @param   BlockId - Block identifier
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
void NvM_KillWriteAll(void)
{
    NvM_InternalState.KillWriteAllRequested = TRUE;
}

/**
 * @brief   Kill ReadAll operation
 */
void NvM_KillReadAll(void)
{
    NvM_InternalState.KillReadAllRequested = TRUE;
}

#define NVM_STOP_SEC_CODE
#include "MemMap.h"
