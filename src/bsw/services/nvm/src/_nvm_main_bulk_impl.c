/*==================================================================================================
 * NVM 主函数/批量操作实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"

void NvM_MainFunction(void)
{
    NvM_JobQueueEntryType jobEntry;
    MemIf_StatusType memIfStatus;
    const NvM_BlockDescriptorType* blockDesc;
    boolean jobComplete;
    uint8 i;
    uint8 tempCount;
    NvM_JobQueueEntryType tempEntries[NVM_SIZE_STANDARD_JOB_QUEUE];

    /* Handle KillReadAll request */
    if (NvM_InternalState.KillReadAllRequested == TRUE)
    {
        NvM_InternalState.KillReadAllRequested = FALSE;
        if (NvM_InternalState.ReadAllInProgress == TRUE)
        {
            /* Remove all READ jobs from standard queue */
            tempCount = NvM_InternalState.StandardQueueCount;
            for (i = 0U; i < tempCount; i++)
            {
                (void)NvM_QueuePop(NvM_InternalState.StandardQueue,
                                   &NvM_InternalState.StandardQueueHead,
                                   &NvM_InternalState.StandardQueueTail,
                                   &NvM_InternalState.StandardQueueCount,
                                   NVM_SIZE_STANDARD_JOB_QUEUE,
                                   &tempEntries[i]);
            }
            for (i = 0U; i < tempCount; i++)
            {
                if (tempEntries[i].JobType != NVM_JOB_TYPE_READ)
                {
                    (void)NvM_QueuePush(NvM_InternalState.StandardQueue,
                                        &NvM_InternalState.StandardQueueHead,
                                        &NvM_InternalState.StandardQueueTail,
                                        &NvM_InternalState.StandardQueueCount,
                                        NVM_SIZE_STANDARD_JOB_QUEUE,
                                        &tempEntries[i]);
                }
                else
                {
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].JobPending = 0U;
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].LastResult = NVM_REQ_CANCELED;
                }
            }
            NvM_InternalState.ReadAllInProgress = FALSE;
            NvM_InternalState.ReadAllPendingCount = 0U;
        }
    }

    /* Handle KillWriteAll request */
    if (NvM_InternalState.KillWriteAllRequested == TRUE)
    {
        NvM_InternalState.KillWriteAllRequested = FALSE;
        if (NvM_InternalState.WriteAllInProgress == TRUE)
        {
            /* Remove all WRITE jobs from standard queue */
            tempCount = NvM_InternalState.StandardQueueCount;
            for (i = 0U; i < tempCount; i++)
            {
                (void)NvM_QueuePop(NvM_InternalState.StandardQueue,
                                   &NvM_InternalState.StandardQueueHead,
                                   &NvM_InternalState.StandardQueueTail,
                                   &NvM_InternalState.StandardQueueCount,
                                   NVM_SIZE_STANDARD_JOB_QUEUE,
                                   &tempEntries[i]);
            }
            for (i = 0U; i < tempCount; i++)
            {
                if (tempEntries[i].JobType != NVM_JOB_TYPE_WRITE)
                {
                    (void)NvM_QueuePush(NvM_InternalState.StandardQueue,
                                        &NvM_InternalState.StandardQueueHead,
                                        &NvM_InternalState.StandardQueueTail,
                                        &NvM_InternalState.StandardQueueCount,
                                        NVM_SIZE_STANDARD_JOB_QUEUE,
                                        &tempEntries[i]);
                }
                else
                {
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].JobPending = 0U;
                    NvM_InternalState.BlockStates[tempEntries[i].BlockId].LastResult = NVM_REQ_CANCELED;
                }
            }
            NvM_InternalState.WriteAllInProgress = FALSE;
            NvM_InternalState.WriteAllPendingCount = 0U;
        }
    }

    if (NvM_InternalState.State == NVM_STATE_IDLE)
    {
        /* Process immediate queue first (high priority) */
        if (!NvM_QueueIsEmpty(NvM_InternalState.ImmediateQueueCount))
        {
            if (NvM_QueuePop(NvM_InternalState.ImmediateQueue,
                             &NvM_InternalState.ImmediateQueueHead,
                             &NvM_InternalState.ImmediateQueueTail,
                             &NvM_InternalState.ImmediateQueueCount,
                             NVM_SIZE_IMMEDIATE_JOB_QUEUE,
                             &jobEntry) == E_OK)
            {
                NvM_InternalState.ActiveJob = jobEntry;
                NvM_InternalState.CurrentJob = &NvM_InternalState.ActiveJob;

                switch (NvM_InternalState.ActiveJob.JobType)
                {
                    case NVM_JOB_TYPE_RESTORE:
                        NvM_ProcessRestoreJob(&NvM_InternalState.ActiveJob);
                        break;

                    default:
                        break;
                }

                NvM_InternalState.CurrentJob = NULL_PTR;
            }
        }
        /* Process standard queue */
        else if (!NvM_QueueIsEmpty(NvM_InternalState.StandardQueueCount))
        {
            if (NvM_QueuePop(NvM_InternalState.StandardQueue,
                             &NvM_InternalState.StandardQueueHead,
                             &NvM_InternalState.StandardQueueTail,
                             &NvM_InternalState.StandardQueueCount,
                             NVM_SIZE_STANDARD_JOB_QUEUE,
                             &jobEntry) == E_OK)
            {
                NvM_InternalState.ActiveJob = jobEntry;
                NvM_InternalState.CurrentJob = &NvM_InternalState.ActiveJob;
                NvM_InternalState.State = NVM_STATE_BUSY;

                /* Process the job based on type */
                switch (NvM_InternalState.ActiveJob.JobType)
                {
                    case NVM_JOB_TYPE_READ:
                        NvM_ProcessReadJob(&NvM_InternalState.ActiveJob);
                        break;

                    case NVM_JOB_TYPE_WRITE:
                        NvM_ProcessWriteJob(&NvM_InternalState.ActiveJob);
                        break;

                    case NVM_JOB_TYPE_ERASE:
                        NvM_ProcessEraseJob(&NvM_InternalState.ActiveJob);
                        break;

                    case NVM_JOB_TYPE_INVALIDATE:
                        NvM_ProcessInvalidateJob(&NvM_InternalState.ActiveJob);
                        break;

                    default:
                        NvM_InternalState.CurrentJob = NULL_PTR;
                        NvM_InternalState.State = NVM_STATE_IDLE;
                        break;
                }
            }
        }
    }
    else if (NvM_InternalState.State == NVM_STATE_BUSY)
    {
        /* Check if current job is complete */
        if (NvM_InternalState.CurrentJob != NULL_PTR)
        {
            blockDesc = NvM_GetBlockDescriptor(NvM_InternalState.CurrentJob->BlockId);

            if (blockDesc != NULL_PTR)
            {
                memIfStatus = MemIf_GetStatus(blockDesc->DeviceId);

                if (memIfStatus == MEMIF_IDLE)
                {
                    MemIf_JobResultType jobResult = MemIf_GetJobResult(blockDesc->DeviceId);
                    jobComplete = TRUE;

                    if (jobResult == MEMIF_JOB_OK)
                    {
                        /* Job completed successfully */
                        if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                        {
#if (NVM_CALC_RAM_BLOCK_CRC == STD_ON)
                            /* Validate CRC if configured */
                            if (blockDesc->BlockUseCrc == TRUE)
                            {
                                uint8 crcSize = NvM_GetCrcSize(blockDesc->CrcType);
                                uint32 storedCrc = 0U;
                                uint32 calcCrc;
                                boolean crcMatch = FALSE;
                                uint8 idx;

                                calcCrc = NvM_CalculateCrc(NvM_InternalState.CurrentJob->DataPtr,
                                                           blockDesc->NvBlockLength,
                                                           blockDesc->CrcType);

                                /* Extract stored CRC from tail of data buffer */
                                for (idx = 0U; idx < crcSize; idx++)
                                {
                                    storedCrc = (storedCrc << 8U) |
                                                ((const uint8*)NvM_InternalState.CurrentJob->DataPtr)[blockDesc->NvBlockLength + idx];
                                }

                                if (crcSize == 1U)
                                {
                                    crcMatch = ((uint8)calcCrc == (uint8)storedCrc);
                                }
                                else if (crcSize == 2U)
                                {
                                    crcMatch = ((uint16)calcCrc == (uint16)storedCrc);
                                }
                                else if (crcSize == 4U)
                                {
                                    crcMatch = (calcCrc == storedCrc);
                                }
                                else
                                {
                                    crcMatch = TRUE;
                                }

                                if (crcMatch == FALSE)
                                {
                                    /* CRC mismatch - try redundant copy or ROM fallback */
                                    if ((blockDesc->ManagementType == NVM_BLOCK_REDUNDANT) &&
                                        (NvM_InternalState.CurrentJob->CopyIndex == 0U))
                                    {
                                        NvM_InternalState.CurrentJob->CopyIndex = 1U;
                                        NvM_ReadRedundantBlock(NvM_InternalState.CurrentJob);
                                        /* REDUNDANT: jobComplete = FALSE; */
                                    }
                                    else
                                    {
                                        NvM_CopyRomDataToRam(NvM_InternalState.CurrentJob->BlockId,
                                                             NvM_InternalState.CurrentJob->DataPtr);
                                        NvM_InternalState.CurrentJob->Result = NVM_REQ_INTEGRITY_FAILED;
                                        NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_INTEGRITY_FAILED;
                                        jobComplete = TRUE;
                                    }
                                }
                                else
                                {
                                    /* CRC OK */
                                    NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                                    jobComplete = TRUE;
                                }
                            }
                            else
#endif
                            {
                                NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                                jobComplete = TRUE;
                            }
                        }
                        else if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_WRITE)
                        {
                            if ((blockDesc->ManagementType == NVM_BLOCK_REDUNDANT) &&
                                (NvM_InternalState.CurrentJob->CopyIndex == 0U))
                            {
                                /* Write second redundant copy */
                                NvM_InternalState.CurrentJob->CopyIndex = 1U;
                                NvM_WriteRedundantBlock(NvM_InternalState.CurrentJob);
                                jobComplete = FALSE;
                            }
                            else
                            {
                                /* Increment write counter on successful write */
                                if (NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].WriteCounter < 0xFFU)
                                {
                                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].WriteCounter++;
                                }
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].DataChanged = FALSE;

                                if (blockDesc->BlockWriteOnce == TRUE)
                                {
                                    NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].WriteOnceDone = TRUE;
                                }

                                NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                                jobComplete = TRUE;
                            }
                        }
                        else
                        {
                            NvM_InternalState.CurrentJob->Result = NVM_REQ_OK;
                            NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_OK;
                            jobComplete = TRUE;
                        }
                    }
                    else
                    {
                        /* Job failed - check retry count */
                        uint16 maxRetries;

                        if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                        {
                            maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfReadRetries;
                        }
                        else
                        {
                            maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfWriteRetries;
                        }

                        if (NvM_InternalState.CurrentJob->RetryCount < maxRetries)
                        {
                            NvM_InternalState.CurrentJob->RetryCount++;
                            jobComplete = FALSE;

                            /* Re-submit the job */
                            switch (NvM_InternalState.CurrentJob->JobType)
                            {
                                case NVM_JOB_TYPE_READ:
                                    NvM_ProcessReadJob(NvM_InternalState.CurrentJob);
                                    break;

                                case NVM_JOB_TYPE_WRITE:
                                    NvM_ProcessWriteJob(NvM_InternalState.CurrentJob);
                                    break;

                                case NVM_JOB_TYPE_ERASE:
                                    NvM_ProcessEraseJob(NvM_InternalState.CurrentJob);
                                    break;

                                case NVM_JOB_TYPE_INVALIDATE:
                                    NvM_ProcessInvalidateJob(NvM_InternalState.CurrentJob);
                                    break;

                                default:
                                    jobComplete = TRUE;
                                    break;
                            }
                        }
                        else
                        {
                            /* Max retries exceeded */
                            if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                            {
                                NvM_CopyRomDataToRam(NvM_InternalState.CurrentJob->BlockId,
                                                     NvM_InternalState.CurrentJob->DataPtr);
                                NvM_InternalState.CurrentJob->Result = NVM_REQ_RESTORED_FROM_ROM;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                            }
                            else
                            {
                                NvM_InternalState.CurrentJob->Result = NVM_REQ_NOT_OK;
                                NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_NOT_OK;
                            }
                            jobComplete = TRUE;
                        }
                    }

                    if (jobComplete == TRUE)
                    {
                        NvM_UpdateBatchOperationStatus(NvM_InternalState.CurrentJob->JobType);

                        NvM_InternalState.CurrentJob->JobState = NVM_JOB_STATE_IDLE;
                        NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].JobPending = 0U;
                        NvM_InvokeJobEndCallback(NvM_InternalState.CurrentJob->BlockId,
                                                 NvM_InternalState.CurrentJob->Result);
                        NvM_InternalState.CurrentJob = NULL_PTR;
                        NvM_InternalState.State = NVM_STATE_IDLE;
                    }
                }
                else if ((memIfStatus != MEMIF_BUSY) && (memIfStatus != MEMIF_BUSY_INTERNAL))
                {
                    /* Unexpected error state - treat as failure with retry */
                    uint16 maxRetries;

                    if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                    {
                        maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfReadRetries;
                    }
                    else
                    {
                        maxRetries = NvM_InternalState.ConfigPtr->MaxNumberOfWriteRetries;
                    }

                    if (NvM_InternalState.CurrentJob->RetryCount < maxRetries)
                    {
                        NvM_InternalState.CurrentJob->RetryCount++;
                        /* Re-submit */
                        switch (NvM_InternalState.CurrentJob->JobType)
                        {
                            case NVM_JOB_TYPE_READ:
                                NvM_ProcessReadJob(NvM_InternalState.CurrentJob);
                                break;
                            case NVM_JOB_TYPE_WRITE:
                                NvM_ProcessWriteJob(NvM_InternalState.CurrentJob);
                                break;
                            case NVM_JOB_TYPE_ERASE:
                                NvM_ProcessEraseJob(NvM_InternalState.CurrentJob);
                                break;
                            case NVM_JOB_TYPE_INVALIDATE:
                                NvM_ProcessInvalidateJob(NvM_InternalState.CurrentJob);
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        NvM_UpdateBatchOperationStatus(NvM_InternalState.CurrentJob->JobType);

                        if (NvM_InternalState.CurrentJob->JobType == NVM_JOB_TYPE_READ)
                        {
                            NvM_CopyRomDataToRam(NvM_InternalState.CurrentJob->BlockId,
                                                 NvM_InternalState.CurrentJob->DataPtr);
                            NvM_InternalState.CurrentJob->Result = NVM_REQ_RESTORED_FROM_ROM;
                            NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                        }
                        else
                        {
                            NvM_InternalState.CurrentJob->Result = NVM_REQ_NOT_OK;
                            NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].LastResult = NVM_REQ_NOT_OK;
                        }
                        NvM_InternalState.CurrentJob->JobState = NVM_JOB_STATE_IDLE;
                        NvM_InternalState.BlockStates[NvM_InternalState.CurrentJob->BlockId].JobPending = 0U;
                        NvM_InvokeJobEndCallback(NvM_InternalState.CurrentJob->BlockId,
                                                 NvM_InternalState.CurrentJob->Result);
                        NvM_InternalState.CurrentJob = NULL_PTR;
                        NvM_InternalState.State = NVM_STATE_IDLE;
                    }
                }
                /* else: still busy, wait for next cycle */
            }
            else
            {
                NvM_InternalState.CurrentJob = NULL_PTR;
                NvM_InternalState.State = NVM_STATE_IDLE;
            }
        }
        else
        {
            NvM_InternalState.State = NVM_STATE_IDLE;
        }
    }
    else
    {
        /* NVM_STATE_UNINIT - do nothing */
    }
}

/**
 * @brief   Read all permanent RAM blocks from NV memory (startup recovery)
 * @return  E_OK if request accepted, E_NOT_OK otherwise
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

    /* REDUNDANT: NvM_InternalState.BlockStates[BlockId].LastResult = NVM_REQ_CANCELED; */

    return E_OK;
}

/**
 * @brief   Kill WriteAll operation
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
