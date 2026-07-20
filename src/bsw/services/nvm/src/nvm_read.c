/*==================================================================================================
 * NvM 读操作实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_DstPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_READ;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NvM_DstPtr;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;

        /* Add to standard queue */
        if (NvM_QueuePush(NvM_InternalState.StandardQueue,
                          &NvM_InternalState.StandardQueueHead,
                          &NvM_InternalState.StandardQueueTail,
                          &NvM_InternalState.StandardQueueCount,
                          NVM_SIZE_STANDARD_JOB_QUEUE,
                          &jobEntry) == E_OK)
        {
            NvM_InternalState.BlockStates[BlockId].JobPending = 1U;
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x04U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Write block to NV memory
 * @param   BlockId     - Block identifier
 * @param   NvM_SrcPtr  - Source pointer for write data
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_DestPtr == NULL_PTR)
    {
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_RESTORE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NvM_DestPtr;
        jobEntry.Result = NVM_REQ_PENDING;
        jobEntry.RetryCount = 0U;

        /* Add to immediate queue (high priority) */
        if (NvM_QueuePush(NvM_InternalState.ImmediateQueue,
                          &NvM_InternalState.ImmediateQueueHead,
                          &NvM_InternalState.ImmediateQueueTail,
                          &NvM_InternalState.ImmediateQueueCount,
                          NVM_SIZE_IMMEDIATE_JOB_QUEUE,
                          &jobEntry) == E_OK)
        {
            NvM_InternalState.BlockStates[BlockId].JobPending = 1U;
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x06U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Set data index for a dataset block
 * @param   BlockId   - Block identifier
 * @param   DataIndex - Data index to set
 * @return  E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x16U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x16U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);
    if ((blockDesc != NULL_PTR) && (blockDesc->RamBlockData != NULL_PTR))
    {
        return NvM_ReadBlock(BlockId, blockDesc->RamBlockData);
    }

    return E_NOT_OK;
}

/**
 * @brief   Write permanent RAM block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_CancelJobs(NvM_BlockIdType BlockId)
{
#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x10U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x10U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Cancel is not fully implemented in this version */
    return E_NOT_OK;
}

/**
 * @brief   Get version information
 * @param   versioninfo - Pointer to version info structure
 * @return  None
 */
Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x09U, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x09U, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_ERASE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NULL_PTR;
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
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x09U, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Invalidate NV block
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)
{
    Std_ReturnType result = E_NOT_OK;
    NvM_JobQueueEntryType jobEntry;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(0x0AU, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(0x0AU, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Check if block already has pending job */
    if (NvM_InternalState.BlockStates[BlockId].JobPending == 0U)
    {
        /* Prepare job entry */
        jobEntry.BlockId = BlockId;
        jobEntry.JobType = NVM_JOB_TYPE_INVALIDATE;
        jobEntry.JobState = NVM_JOB_STATE_PENDING;
        jobEntry.DataPtr = NULL_PTR;
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
            result = E_OK;
        }
    }
    else
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(0x0AU, NVM_E_BLOCK_PENDING);
#endif
    }

    return result;
}

/**
 * @brief   Main function for NvM processing
 * @param   None
 * @return  None
 */
Std_ReturnType NvM_ReadAll(void)
{
    Std_ReturnType result = E_OK;
    const NvM_BlockDescriptorType* blockDesc;
    uint16 i;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_READALL, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif

    if (NvM_InternalState.ReadAllInProgress == TRUE)
    {
        return E_NOT_OK;
    }

    for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
    {
        blockDesc = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];

        if (blockDesc->RamBlockData != NULL_PTR)
        {
            if (NvM_ReadBlock(blockDesc->BlockId, blockDesc->RamBlockData) == E_OK)
            {
                NvM_InternalState.ReadAllPendingCount++;
            }
            else
            {
                result = E_NOT_OK;
            }
        }
    }

    if (NvM_InternalState.ReadAllPendingCount > 0U)
    {
        NvM_InternalState.ReadAllInProgress = TRUE;
    }

    return result;
}

/**
 * @brief   Write all dirty permanent RAM blocks to NV memory (shutdown flush)
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* blockDesc;

#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_READPRAMBLOCK, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_READPRAMBLOCK, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    blockDesc = NvM_GetBlockDescriptor(BlockId);

    if ((blockDesc == NULL_PTR) || (blockDesc->RamBlockData == NULL_PTR))
    {
#if (NVM_DEV_ERROR_DETECT == STD_ON)
        NVM_DET_REPORT_ERROR(NVM_SID_READPRAMBLOCK, NVM_E_BLOCK_CONFIG);
#endif
        return E_NOT_OK;
    }

    return NvM_ReadBlock(BlockId, blockDesc->RamBlockData);
}

/**
 * @brief   Write a permanent RAM block (uses configured RamBlockData)
 * @param   BlockId - Block identifier
 * @return  E_OK if request accepted, E_NOT_OK otherwise
 */
Std_ReturnType NvM_CancelJobs(NvM_BlockIdType BlockId)
{
#if (NVM_DEV_ERROR_DETECT == STD_ON)
    if (NvM_InternalState.State == NVM_STATE_UNINIT)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_CANCELJOBS, NVM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }

    if (NvM_ValidateBlockId(BlockId) != E_OK)
    {
        NVM_DET_REPORT_ERROR(NVM_SID_CANCELJOBS, NVM_E_PARAM_BLOCK_ID);
        return E_NOT_OK;
    }
#endif

    /* Cancel pending jobs in standard queue */
    NvM_QueueCancelJobs(NvM_InternalState.StandardQueue,
                        &NvM_InternalState.StandardQueueHead,
                        &NvM_InternalState.StandardQueueTail,
                        &NvM_InternalState.StandardQueueCount,
                        NVM_SIZE_STANDARD_JOB_QUEUE,
                        BlockId);

    /* Cancel pending jobs in immediate queue */
    NvM_QueueCancelJobs(NvM_InternalState.ImmediateQueue,
                        &NvM_InternalState.ImmediateQueueHead,
                        &NvM_InternalState.ImmediateQueueTail,
                        &NvM_InternalState.ImmediateQueueCount,
                        NVM_SIZE_IMMEDIATE_JOB_QUEUE,
                        BlockId);

    /* If current job matches, mark result as canceled */
    if ((NvM_InternalState.CurrentJob != NULL_PTR) &&
        (NvM_InternalState.CurrentJob->BlockId == BlockId))
    {
        NvM_InternalState.CurrentJob->Result = NVM_REQ_CANCELED;
        NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_CANCELED;
    }

/* [MISRA Advisory] Redundant:     NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_CANCELED; */

    return E_OK;
}

/**
 * @brief   Kill WriteAll operation
 */
#define NVM_STOP_SEC_CODE
#include "MemMap.h"
