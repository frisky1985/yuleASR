/*==================================================================================================
 * NvM 作业处理实现
 * 自动拆分自 NvM.c
 *================================================================================================*/
#define NVM_START_SEC_CODE
#include "MemMap.h"
#include "MemIf.h"

Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry);
Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry);
boolean NvM_QueueIsEmpty(uint8 Count);
boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize);

const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId);
void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr);
void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr);
void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr);
void NvM_ProcessEraseJob(NvM_JobQueueEntryType* JobPtr);
void NvM_ProcessInvalidateJob(NvM_JobQueueEntryType* JobPtr);

void NvM_ReadRedundantBlock(NvM_JobQueueEntryType* JobPtr);
void NvM_WriteRedundantBlock(NvM_JobQueueEntryType* JobPtr);
Std_ReturnType NvM_QueuePush(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, const NvM_JobQueueEntryType* Entry)
{
    Std_ReturnType result = E_NOT_OK;

    if ((Queue != NULL_PTR) && (Head != NULL_PTR) && (Tail != NULL_PTR) && (Count != NULL_PTR) && (Entry != NULL_PTR))
    {
        if (*Count < MaxSize)
        {
            Queue[*Tail] = *Entry;
            *Tail = (*Tail + 1U) % MaxSize;
            (*Count)++;
            result = E_OK;
        }
    }

    return result;
}
/**
 * @brief   Pop job from queue
 */
Std_ReturnType NvM_QueuePop(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_JobQueueEntryType* Entry)
{
    Std_ReturnType result = E_NOT_OK;

    if ((Queue != NULL_PTR) && (Head != NULL_PTR) && (Tail != NULL_PTR) && (Count != NULL_PTR) && (Entry != NULL_PTR))
    {
        if (*Count > 0U)
        {
            *Entry = Queue[*Head];
            *Head = (*Head + 1U) % MaxSize;
            (*Count)--;
            result = E_OK;
        }
    }

    return result;
}
/**
 * @brief   Check if queue is empty
 */
boolean NvM_QueueIsEmpty(uint8 Count)
{
    return (Count == 0U) ? TRUE : FALSE;
}
/**
 * @brief   Check if queue is full
 */
boolean NvM_QueueIsFull(uint8 Count, uint8 MaxSize)
{
    return (Count >= MaxSize) ? TRUE : FALSE;
}
/**
 * @brief   Get block descriptor for given block ID
 */
const NvM_BlockDescriptorType* NvM_GetBlockDescriptor(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* result = NULL_PTR;
    uint8 i;

    if (NvM_InternalState.ConfigPtr != NULL_PTR)
    {
        for (i = 0U; i < NvM_InternalState.ConfigPtr->NumBlockDescriptors; i++)
        {
            if (NvM_InternalState.ConfigPtr->BlockDescriptors[i].BlockId == BlockId)
            {
                result = &NvM_InternalState.ConfigPtr->BlockDescriptors[i];
                break;
            }
        }
    }

    return result;
}
/**
 * @brief   Validate block ID
 */
void NvM_ReadRedundantBlock(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;
    uint16 readLength;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            blockNumber = blockDesc->BlockBaseNumber + JobPtr->CopyIndex;
            readLength = blockDesc->NvBlockLength;

            if (blockDesc->BlockUseCrc == TRUE)
            {
                readLength += NvM_GetCrcSize(blockDesc->CrcType);
            }

            /* Call MemIf to read from NV memory */
            memIfResult = MemIf_Read(blockDesc->DeviceId, blockNumber, 0U,
                                     (uint8*)JobPtr->DataPtr, readLength);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                /* Read failed immediately, try next copy or ROM fallback */
                if (JobPtr->CopyIndex == 0U)
                {
                    JobPtr->CopyIndex = 1U;
                    NvM_ReadRedundantBlock(JobPtr);
                }
                else
                {
                    NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
                    JobPtr->Result = NVM_REQ_RESTORED_FROM_ROM;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
            }
        }
    }
}
/**
 * @brief   Process read job
 */
void NvM_ProcessReadJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;
    uint16 readLength;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_REDUNDANT)
            {
                JobPtr->CopyIndex = 0U;
                NvM_ReadRedundantBlock(JobPtr);
            }
            else
            {
                if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
                {
                    blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
                }
                else
                {
                    blockNumber = blockDesc->BlockBaseNumber;
                }

                readLength = blockDesc->NvBlockLength;
                if (blockDesc->BlockUseCrc == TRUE)
                {
                    readLength += NvM_GetCrcSize(blockDesc->CrcType);
                }

                /* Call MemIf to read from NV memory */
                memIfResult = MemIf_Read(blockDesc->DeviceId, blockNumber, 0U,
                                         (uint8*)JobPtr->DataPtr, readLength);

                if (memIfResult == E_OK)
                {
                    JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
                }
                else
                {
                    /* Read failed immediately, try to restore from ROM */
                    NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);
                    JobPtr->Result = NVM_REQ_RESTORED_FROM_ROM;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_RESTORED_FROM_ROM;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
            }
        }
        else
        {
            JobPtr->Result = NVM_REQ_NOT_OK;
            JobPtr->JobState = NVM_JOB_STATE_IDLE;
        }
    }
}
/**
 * @brief   Process redundant block write (first or second copy)
 */
void NvM_WriteRedundantBlock(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;
    uint8 crcSize;
    uint32 calcCrc;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            blockNumber = blockDesc->BlockBaseNumber + JobPtr->CopyIndex;

            /* Append CRC if configured */
            if ((blockDesc->BlockUseCrc == TRUE) && (blockDesc->CrcType != NVM_CRC_NONE))
            {
                crcSize = NvM_GetCrcSize(blockDesc->CrcType);
                calcCrc = NvM_CalculateCrc(JobPtr->DataPtr, blockDesc->NvBlockLength, blockDesc->CrcType);

                if (crcSize == 1U)
                {
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)calcCrc;
                }
                else if (crcSize == 2U)
                {
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 8U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)calcCrc;
                }
                else if (crcSize == 4U)
                {
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 24U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)(calcCrc >> 16U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 2U] = (uint8)(calcCrc >> 8U);
                    ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 3U] = (uint8)calcCrc;
                }
                else
                {
                    /* No action needed */
                }
            }

            /* Call MemIf to write to NV memory */
            memIfResult = MemIf_Write(blockDesc->DeviceId, blockNumber,
                                      (uint8*)JobPtr->DataPtr);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                /* Write failed immediately */
                if (JobPtr->CopyIndex == 0U)
                {
                    JobPtr->CopyIndex = 1U;
                    NvM_WriteRedundantBlock(JobPtr);
                }
                else
                {
                    JobPtr->Result = NVM_REQ_NOT_OK;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
            }
        }
    }
}
/**
 * @brief   Process write job
 */
void NvM_ProcessWriteJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;
    uint8 crcSize;
    uint32 calcCrc;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_REDUNDANT)
            {
                JobPtr->CopyIndex = 0U;
                NvM_WriteRedundantBlock(JobPtr);
            }
            else
            {
                if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
                {
                    blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
                }
                else
                {
                    blockNumber = blockDesc->BlockBaseNumber;
                }

                /* Append CRC if configured */
                if ((blockDesc->BlockUseCrc == TRUE) && (blockDesc->CrcType != NVM_CRC_NONE))
                {
                    crcSize = NvM_GetCrcSize(blockDesc->CrcType);
                    calcCrc = NvM_CalculateCrc(JobPtr->DataPtr, blockDesc->NvBlockLength, blockDesc->CrcType);

                    if (crcSize == 1U)
                    {
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)calcCrc;
                    }
                    else if (crcSize == 2U)
                    {
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 8U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)calcCrc;
                    }
                    else if (crcSize == 4U)
                    {
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength] = (uint8)(calcCrc >> 24U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 1U] = (uint8)(calcCrc >> 16U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 2U] = (uint8)(calcCrc >> 8U);
                        ((uint8*)JobPtr->DataPtr)[blockDesc->NvBlockLength + 3U] = (uint8)calcCrc;
                    }
                    else
                    {
                        /* No action needed */
                    }
                }

                /* Call MemIf to write to NV memory */
                memIfResult = MemIf_Write(blockDesc->DeviceId, blockNumber,
                                          (uint8*)JobPtr->DataPtr);

                if (memIfResult == E_OK)
                {
                    JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
                }
                else
                {
                    JobPtr->Result = NVM_REQ_NOT_OK;
                    JobPtr->JobState = NVM_JOB_STATE_IDLE;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
                    NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
                }
            }
        }
        else
        {
            JobPtr->Result = NVM_REQ_NOT_OK;
            JobPtr->JobState = NVM_JOB_STATE_IDLE;
        }
    }
}
/**
 * @brief   Process restore job
 */
void NvM_ProcessRestoreJob(NvM_JobQueueEntryType* JobPtr)
{
    if (JobPtr != NULL_PTR)
    {
        /* Copy ROM data to RAM */
        NvM_CopyRomDataToRam(JobPtr->BlockId, JobPtr->DataPtr);

        JobPtr->Result = NVM_REQ_OK;
        JobPtr->JobState = NVM_JOB_STATE_IDLE;
        NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_OK;
        NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
    }
}

/**
 * @brief   Process erase job
 */
void NvM_ProcessEraseJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
            {
                blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
            }
            else
            {
                blockNumber = blockDesc->BlockBaseNumber;
            }

            memIfResult = MemIf_EraseImmediateBlock(blockDesc->DeviceId, blockNumber);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                JobPtr->Result = NVM_REQ_NOT_OK;
                JobPtr->JobState = NVM_JOB_STATE_IDLE;
                NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
            }
        }
        else
        {
            JobPtr->Result = NVM_REQ_NOT_OK;
            JobPtr->JobState = NVM_JOB_STATE_IDLE;
        }
    }
}

/**
 * @brief   Process invalidate job
 */
void NvM_ProcessInvalidateJob(NvM_JobQueueEntryType* JobPtr)
{
    const NvM_BlockDescriptorType* blockDesc;
    Std_ReturnType memIfResult;
    uint16 blockNumber;

    if (JobPtr != NULL_PTR)
    {
        blockDesc = NvM_GetBlockDescriptor(JobPtr->BlockId);

        if (blockDesc != NULL_PTR)
        {
            if (blockDesc->ManagementType == NVM_BLOCK_DATASET)
            {
                blockNumber = blockDesc->BlockBaseNumber + NvM_InternalState.BlockStates[JobPtr->BlockId].DataIndex;
            }
            else
            {
                blockNumber = blockDesc->BlockBaseNumber;
            }

            memIfResult = MemIf_InvalidateBlock(blockDesc->DeviceId, blockNumber);

            if (memIfResult == E_OK)
            {
                JobPtr->JobState = NVM_JOB_STATE_PROCESSING;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 1U;
            }
            else
            {
                JobPtr->Result = NVM_REQ_NOT_OK;
                JobPtr->JobState = NVM_JOB_STATE_IDLE;
                NvM_InternalState.BlockStates[JobPtr->BlockId].LastResult = NVM_REQ_NOT_OK;
                NvM_InternalState.BlockStates[JobPtr->BlockId].JobPending = 0U;
            }
        }
        else
        {
            JobPtr->Result = NVM_REQ_NOT_OK;
            JobPtr->JobState = NVM_JOB_STATE_IDLE;
        }
    }
}

/**
 * @brief   Cancel all jobs for a given BlockId in a queue
 */
void NvM_QueueCancelJobs(NvM_JobQueueEntryType* Queue, uint8* Head, uint8* Tail, uint8* Count, uint8 MaxSize, NvM_BlockIdType BlockId)
{
    uint8 tempCount;
    uint8 i;
    NvM_JobQueueEntryType tempEntries[NVM_SIZE_STANDARD_JOB_QUEUE];
    boolean canceled = FALSE;

    tempCount = *Count;

    for (i = 0U; i < tempCount; i++)
    {
        (void)NvM_QueuePop(Queue, Head, Tail, Count, MaxSize, &tempEntries[i]);
    }

    for (i = 0U; i < tempCount; i++)
    {
        if (tempEntries[i].BlockId == BlockId)
        {
            canceled = TRUE;
        }
        else
        {
            (void)NvM_QueuePush(Queue, Head, Tail, Count, MaxSize, &tempEntries[i]);
        }
    }

    if (canceled == TRUE)
    {
        NvM_InternalState.BlockStates[BlockId].JobPending = 0U;
    }
}

/**
 * @brief   Update batch operation status after a job completes
 */
void NvM_UpdateBatchOperationStatus(uint8 JobType)
{
    if ((NvM_InternalState.ReadAllInProgress == TRUE) && (JobType == NVM_JOB_TYPE_READ))
    {
        if (NvM_InternalState.ReadAllPendingCount > 0U)
        {
            NvM_InternalState.ReadAllPendingCount--;
        }
        if (NvM_InternalState.ReadAllPendingCount == 0U)
        {
            NvM_InternalState.ReadAllInProgress = FALSE;
        }
    }

    if ((NvM_InternalState.WriteAllInProgress == TRUE) && (JobType == NVM_JOB_TYPE_WRITE))
    {
        if (NvM_InternalState.WriteAllPendingCount > 0U)
        {
            NvM_InternalState.WriteAllPendingCount--;
        }
        if (NvM_InternalState.WriteAllPendingCount == 0U)
        {
            NvM_InternalState.WriteAllInProgress = FALSE;
        }
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the NvM module
 * @param   ConfigPtr - Pointer to configuration structure
 * @return  None
 */
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
/* [MISRA Advisory] Redundant:                                         jobComplete = FALSE; */
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
#define NVM_STOP_SEC_CODE
#include "MemMap.h"
