/**
 * @file nvm_expose.c
 * @brief 白盒暴露 NvM.c 内部 STATIC 函数（编译进独立测试目标）
 *
 * 做法: 先包含与 NvM.c 相同的头文件，使 Compiler.h 的 STATIC(=static) 生效；
 *       随后 #undef STATIC 并重新定义为空，再 include NvM.c。
 *       这样 NvM.c 内部函数/变量变为外部可见，供测试直接驱动，
 *       且不影响任何产品文件语义。
 *
 * 注意: 本 TU 不链接 service_nvm 库，避免与真实实现的全局符号冲突。
 */

#include "NvM.h"
#include "NvM_Cfg.h"
#include "Det.h"
#include "Compiler.h"
#include "MemIf.h"
#include "Mcal.h"
#include <string.h>

/* 让 NvM.c 中的 STATIC 失效，从而导出内部符号 */
#undef STATIC
#define STATIC

#include "NvM.c"

/*==================================================================================================
 *                                  测试访问桥接
 *================================================================================================*/
#include <string.h>

uint8 NvMTest_QueueIsFull(uint8 count, uint8 maxSize)
{
    return (NvM_QueueIsFull(count, maxSize) == TRUE) ? 1u : 0u;
}

uint32 NvMTest_CalculateCrc(const void* dataPtr, uint16 length, NvM_BlockCrcType crcType)
{
    return NvM_CalculateCrc(dataPtr, length, crcType);
}

uint8 NvMTest_GetCrcSize(NvM_BlockCrcType crcType)
{
    return NvM_GetCrcSize(crcType);
}

static void NvMTest_MakeJob(NvM_JobQueueEntryType* job, NvM_BlockIdType blockId)
{
    (void)memset(job, 0, sizeof(*job));
    job->BlockId = blockId;
}

void NvMTest_ProcessRead(NvM_BlockIdType blockId)
{
    NvM_JobQueueEntryType job;
    NvMTest_MakeJob(&job, blockId);
    NvM_ProcessReadJob(&job);
}

void NvMTest_ProcessWrite(NvM_BlockIdType blockId)
{
    NvM_JobQueueEntryType job;
    NvMTest_MakeJob(&job, blockId);
    NvM_ProcessWriteJob(&job);
}

void NvMTest_ProcessErase(NvM_BlockIdType blockId)
{
    NvM_JobQueueEntryType job;
    NvMTest_MakeJob(&job, blockId);
    NvM_ProcessEraseJob(&job);
}

void NvMTest_ProcessInvalidate(NvM_BlockIdType blockId)
{
    NvM_JobQueueEntryType job;
    NvMTest_MakeJob(&job, blockId);
    NvM_ProcessInvalidateJob(&job);
}

void NvMTest_UpdateBatch(uint8 jobType)
{
    NvM_UpdateBatchOperationStatus(jobType);
}

void NvMTest_SetBatch(uint8 jobType, uint16 pending)
{
    if (jobType == NVM_JOB_TYPE_READ)
    {
        NvM_InternalState.ReadAllInProgress = TRUE;
        NvM_InternalState.ReadAllPendingCount = pending;
    }
    else
    {
        NvM_InternalState.WriteAllInProgress = TRUE;
        NvM_InternalState.WriteAllPendingCount = pending;
    }
}

uint16 NvMTest_GetBatchPending(uint8 jobType)
{
    return (jobType == NVM_JOB_TYPE_READ)
               ? NvM_InternalState.ReadAllPendingCount
               : NvM_InternalState.WriteAllPendingCount;
}

uint8 NvMTest_GetBatchInProgress(uint8 jobType)
{
    boolean flag = (jobType == NVM_JOB_TYPE_READ)
                       ? NvM_InternalState.ReadAllInProgress
                       : NvM_InternalState.WriteAllInProgress;
    return (flag == TRUE) ? 1u : 0u;
}

uint8 NvMTest_Resubmit(uint8 jobType)
{
    NvM_InternalState.CurrentJob = &NvM_InternalState.ActiveJob;
    NvM_InternalState.ActiveJob.JobType = jobType;
    return (Nvm_ResubmitJob() == TRUE) ? 1u : 0u;
}

void NvMTest_HandleBusyNull(void)
{
    NvM_InternalState.State = NVM_STATE_BUSY;
    NvM_InternalState.CurrentJob = NULL_PTR;
    Nvm_HandleBusyState();
}

void NvMTest_HandleBusyUnknownBlock(void)
{
    NvM_InternalState.State = NVM_STATE_BUSY;
    NvM_InternalState.CurrentJob = &NvM_InternalState.ActiveJob;
    NvM_InternalState.ActiveJob.BlockId = 0x7FFFu;
    NvM_InternalState.ConfigPtr = NULL_PTR;
    Nvm_HandleBusyState();
}

void NvMTest_ResetConfig(void)
{
    NvM_InternalState.ConfigPtr = NULL_PTR;
    NvM_InternalState.State = NVM_STATE_UNINIT;
    NvM_InternalState.CurrentJob = NULL_PTR;
    NvM_InternalState.ReadAllInProgress = FALSE;
    NvM_InternalState.WriteAllInProgress = FALSE;
    NvM_InternalState.ReadAllPendingCount = 0U;
    NvM_InternalState.WriteAllPendingCount = 0U;
}
