/**
 * @file test_nvm_internal.c
 * @brief NvM.c 内部 STATIC 分支白盒测试（配合 nvm_expose.c）
 *
 * 通过 nvm_expose 导出的内部函数直接驱动那些公开 API 难以触达的
 * "无效块描述符 / 队列满 / CRC_NONE / 批处理归零 / 忙状态异常" 等分支。
 * 不链接 service_nvm，避免与真实实现重复定义。
 */

#include "NvM.h"
#include "MemIf.h"

#include <stdio.h>
#include <string.h>

/*==================================================================================================
 *                                    ASSERT
 *================================================================================================*/
static int g_failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

/* NvM.c 内部任务类型（未在 NvM.h 暴露） */
#define NVM_JOB_TYPE_READ    (1u)
#define NVM_JOB_TYPE_WRITE   (2u)

/* Det stub (NvM.c 需要) */
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/* expose 桥接 */
extern uint8  NvMTest_QueueIsFull(uint8 count, uint8 maxSize);
extern uint32 NvMTest_CalculateCrc(const void* dataPtr, uint16 length, NvM_BlockCrcType crcType);
extern uint8  NvMTest_GetCrcSize(NvM_BlockCrcType crcType);
extern void   NvMTest_ProcessRead(NvM_BlockIdType blockId);
extern void   NvMTest_ProcessWrite(NvM_BlockIdType blockId);
extern void   NvMTest_ProcessErase(NvM_BlockIdType blockId);
extern void   NvMTest_ProcessInvalidate(NvM_BlockIdType blockId);
extern void   NvMTest_UpdateBatch(uint8 jobType);
extern void   NvMTest_SetBatch(uint8 jobType, uint16 pending);
extern uint16 NvMTest_GetBatchPending(uint8 jobType);
extern uint8  NvMTest_GetBatchInProgress(uint8 jobType);
extern uint8  NvMTest_Resubmit(uint8 jobType);
extern void   NvMTest_HandleBusyNull(void);
extern void   NvMTest_HandleBusyUnknownBlock(void);
extern void   NvMTest_ResetConfig(void);

/* NvM.c 中的 STATIC 已导出为全局符号 */
extern const uint8* NvM_StageWriteFrame(const NvM_BlockDescriptorType* blockDesc, const void* srcPtr);
extern uint8* NvM_StageReadFrame(const NvM_BlockDescriptorType* blockDesc, void* callerPtr, uint16* lengthPtr);

/*==================================================================================================
 *                                    TESTS
 *================================================================================================*/
static void test_queue_is_full(void)
{
    NvMTest_ResetConfig();
    CHECK(NvMTest_QueueIsFull(4u, 4u) == 1u);    /* 384 TRUE 分支 */
    CHECK(NvMTest_QueueIsFull(0u, 4u) == 0u);    /* 384 FALSE 分支 */
}

static void test_crc_none_paths(void)
{
    NvMTest_ResetConfig();
    CHECK(NvMTest_CalculateCrc(NULL_PTR, 0u, NVM_CRC_NONE) == 0u);  /* 523/525/526 */
    CHECK(NvMTest_GetCrcSize(NVM_CRC_NONE) == 0u);                  /* 553/555/556 */
}

static void test_stage_frame_overflow(void)
{
    NvM_BlockDescriptorType desc;
    uint8 buf[8] = {0};
    uint16 len = 0u;

    NvMTest_ResetConfig();
    (void)memset(&desc, 0, sizeof(desc));
    desc.BlockUseCrc = TRUE;
    desc.CrcType = NVM_CRC_32;
    desc.NvBlockLength = 0xFFFFu;   /* 远超 NvM_CrcScratch */

    CHECK(NvM_StageWriteFrame(&desc, buf) == NULL_PTR);   /* 617 */
    CHECK(NvM_StageReadFrame(&desc, buf, &len) == NULL_PTR); /* 647 */
}

static void test_process_jobs_unknown_block(void)
{
    NvMTest_ResetConfig();   /* ConfigPtr = NULL → 所有 BlockId 查不到描述符 */
    NvMTest_ProcessRead(0x7FFFu);        /* 842-843 */
    NvMTest_ProcessWrite(0x7FFFu);       /* 962-963 */
    NvMTest_ProcessErase(0x7FFFu);       /* 1028-1029 */
    NvMTest_ProcessInvalidate(0x7FFFu);  /* 1076-1077 */
}

static void test_batch_operation_status(void)
{
    NvMTest_ResetConfig();

    /* WriteAll: pending 1 → 0, in-progress 清 0 */
    NvMTest_SetBatch(NVM_JOB_TYPE_WRITE, 1u);
    NvMTest_UpdateBatch(NVM_JOB_TYPE_WRITE);
    CHECK(NvMTest_GetBatchPending(NVM_JOB_TYPE_WRITE) == 0u);
    CHECK(NvMTest_GetBatchInProgress(NVM_JOB_TYPE_WRITE) == 0u);   /* 1101-1107 */

    /* ReadAll: pending 1 → 0 */
    NvMTest_SetBatch(NVM_JOB_TYPE_READ, 1u);
    NvMTest_UpdateBatch(NVM_JOB_TYPE_READ);
    CHECK(NvMTest_GetBatchPending(NVM_JOB_TYPE_READ) == 0u);
    CHECK(NvMTest_GetBatchInProgress(NVM_JOB_TYPE_READ) == 0u);
}

static void test_resubmit_unknown_type(void)
{
    NvMTest_ResetConfig();
    CHECK(NvMTest_Resubmit(99u) == 0u);   /* 2207-2208 default */
}

static void test_handle_busy_edge_cases(void)
{
    NvMTest_ResetConfig();
    NvMTest_HandleBusyNull();            /* CurrentJob == NULL → 2300-2301 */
    NvMTest_HandleBusyUnknownBlock();    /* blockDesc == NULL → 2308-2310 */
}

/*==================================================================================================
 *                                    MAIN
 *================================================================================================*/
int main(void)
{
    test_queue_is_full();
    test_crc_none_paths();
    test_stage_frame_overflow();
    test_process_jobs_unknown_block();
    test_batch_operation_status();
    test_resubmit_unknown_type();
    test_handle_busy_edge_cases();

    if (g_failures != 0)
    {
        fprintf(stderr, "\n*** NvM internal tests FAILED: %d\n", g_failures);
        return 1;
    }
    printf("\n=== NvM internal tests PASSED (7 cases) ===\n");
    return 0;
}
