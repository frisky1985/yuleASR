/*==================================================================================================
 * NVM 读取操作实现
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


#define NVM_STOP_SEC_CODE
#include "MemMap.h"
