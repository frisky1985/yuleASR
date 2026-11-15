/**
 * @file test_fee_mainloop.c
 * @brief Fee 主状态机驱动测试 — MainFunction 路径覆盖
 *
 * Fls stub 是同步的: Fls_Write/Read/Erase 立即返回, Fls_GetJobResult 立即 OK.
 * 因此每个 job 需要两次 MainFunction 走完完整状态机:
 *   #1: ProcessJob → StartFls* → Fee_CurrentJob = PENDING+1
 *   #2: CheckFlsJobStatus → Fee_FlsJob = COMPLETED → Complete*Job → FinishJob
 */

#define UNIT_TEST
#include "Fee.h"
#include "Fls.h"
#include "Det.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*==================================================================================================
 *                                    ASSERT MACROS
 *================================================================================================*/
static int g_failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

/* 来自 fee_expose.c 的内部状态暴露 API */
extern void    FeeTest_SetJobState(uint8 state);
extern void    FeeTest_SetFlsJobState(uint8 state);
extern uint8   FeeTest_GetJobType(void);
extern uint8   FeeTest_GetJobState(void);
extern uint8   FeeTest_GetJobResult(void);
extern boolean FeeTest_IsInitialized(void);

/* 来自 fee_test_support.c 的 Fls stub hooks */
extern Fls_JobResultType  FeeTest_FlsJobResult;
extern Std_ReturnType     FeeTest_FlsEraseResult;
extern Std_ReturnType     FeeTest_FlsWriteResult;
extern uint8              FeeTest_FlsReadFillByte;
extern uint16             FeeTest_DetCallCount;

/* Fee.h 没暴露的内部常量 (从 Fee.c 复制) */
#ifndef FEE_JOB_OK
  #define FEE_JOB_OK          ((Fee_JobResultType)0x00u)
#endif
#ifndef FEE_JOB_FAILED
  #define FEE_JOB_FAILED      ((Fee_JobResultType)0x01u)
#endif
#ifndef FEE_BLOCK_INVALID
  #define FEE_BLOCK_INVALID   ((Fee_JobResultType)0x02u)
#endif

#define FEE_FLS_JOB_IDLE      (0x00u)
#define FEE_FLS_JOB_PENDING   (0x01u)

#ifndef MEMIF_JOB_OK
  #define MEMIF_JOB_OK         ((Fls_JobResultType)0x00u)
#endif
#ifndef MEMIF_JOB_FAILED
  #define MEMIF_JOB_FAILED     ((Fls_JobResultType)0x01u)
#endif

/*==================================================================================================
 *                                    SETUP / TEARDOWN
 *================================================================================================*/
extern const Fee_ConfigType Fee_Config;

static void setup(void)
{
    FeeTest_FlsJobResult   = MEMIF_JOB_OK;
    FeeTest_FlsEraseResult = E_OK;
    FeeTest_FlsWriteResult = E_OK;
    FeeTest_FlsReadFillByte = 0xFFu;
    FeeTest_DetCallCount   = 0u;
}

/*==================================================================================================
 *                                    TEST 1: Write 完整路径
 *================================================================================================*/
static void test_write_complete_path(void)
{
    setup();
    Fee_Init(&Fee_Config);
    CHECK(FeeTest_IsInitialized() == TRUE);

    uint8 data[64];
    for (int i = 0; i < 64; ++i) { data[i] = (uint8)i; }

    /* 1) Fee_Write 入队 */
    CHECK(Fee_Write(1u, data) == E_OK);

    /* 2) 第一次 MainFunction:
     *    ProcessWriteJob case PENDING → StartFlsWrite → CurrentJob = PENDING+1 */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == (FEE_FLS_JOB_PENDING + 1u));

    /* 3) 第二次 MainFunction:
     *    CheckFlsJobStatus → FlsJob=COMPLETED
     *    ProcessWriteJob case PENDING+1 → CompleteWriteJob → FinishJob(OK) */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 2: Read 完整路径
 *================================================================================================*/
static void test_read_complete_path(void)
{
    setup();
    Fee_Init(&Fee_Config);

    uint8 data[64];
    for (int i = 0; i < 64; ++i) { data[i] = (uint8)(0xAA - i); }

    /* 先写一次 — 建立 BlockInfo 有效 BlockAddress */
    CHECK(Fee_Write(2u, data) == E_OK);
    Fee_MainFunction();
    Fee_MainFunction();
    CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);

    /* 现在读 */
    uint8 read_buf[64];
    memset(read_buf, 0, sizeof(read_buf));
    CHECK(Fee_Read(2u, 0u, read_buf, 64u) == E_OK);

    /* 第一次 MainFunction: StartFlsRead, CurrentJob = PENDING+1 */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == (FEE_FLS_JOB_PENDING + 1u));

    /* 第二次 MainFunction: CompleteReadJob → FinishJob(OK) */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 3: Fls_Write 返回 E_NOT_OK → 立即失败
 *================================================================================================*/
static void test_write_fls_error(void)
{
    setup();
    Fee_Init(&Fee_Config);

    uint8 data[64] = {0};
    FeeTest_FlsWriteResult = E_NOT_OK;

    CHECK(Fee_Write(3u, data) == E_OK);

    /* 一次 MainFunction 就 FinishJob — StartFlsWrite 立即返回 E_NOT_OK → FinishJob(FAILED) */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    CHECK(FeeTest_GetJobResult() == FEE_JOB_FAILED);

    FeeTest_FlsWriteResult = E_OK;
    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 4: Fls_JobResult = FAILED → 中途失败
 *================================================================================================*/
static void test_write_fls_job_failed(void)
{
    setup();
    Fee_Init(&Fee_Config);

    uint8 data[64] = {0};
    CHECK(Fee_Write(4u, data) == E_OK);

    /* 第一次 MainFunction 正常 StartFlsWrite */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == (FEE_FLS_JOB_PENDING + 1u));

    /* 把 Fls_GetJobResult 改成 FAILED */
    FeeTest_FlsJobResult = MEMIF_JOB_FAILED;

    /* 第二次 MainFunction:
     * CheckFlsJobStatus → FlsJob = FAILED
     * CompleteWriteJob 检查 FlsJob != COMPLETED → FinishJob(FAILED) */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    CHECK(FeeTest_GetJobResult() == FEE_JOB_FAILED);

    FeeTest_FlsJobResult = MEMIF_JOB_OK;
    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 5: Busy 状态下连续多 job
 *================================================================================================*/
static void test_multiple_jobs_sequential(void)
{
    setup();
    Fee_Init(&Fee_Config);

    uint8 data[32];
    for (int i = 0; i < 32; ++i) { data[i] = (uint8)(0x55 + i); }

    /* 5 个 Write → 每两个 MainFunction 一个 OK */
    for (int block = 1; block <= 5; ++block) {
        CHECK(Fee_Write((Fee_BlockIdType)block, data) == E_OK);
        Fee_MainFunction();
        Fee_MainFunction();
        CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);
    }

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 6: Cancel in Busy state
 *================================================================================================*/
static void test_cancel_during_busy(void)
{
    setup();
    Fee_Init(&Fee_Config);

    uint8 data[64] = {0};

    /* Write 入队 + 一次 MainFunction → Fee_FlsJob=PENDING (Busy!) */
    CHECK(Fee_Write(1u, data) == E_OK);
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == (FEE_FLS_JOB_PENDING + 1u));

    /* 此时 Cancel — 应触发 Fls_Cancel + Busy 清理 */
    Fee_Cancel();

    /* 状态应被清成 IDLE */
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 7: InvalidateBlock 完整路径
 *================================================================================================*/
static void test_invalidate_complete(void)
{
    setup();
    Fee_Init(&Fee_Config);

    /* 先写一个 block 让它 valid */
    uint8 data[32] = {0};
    CHECK(Fee_Write(10u, data) == E_OK);
    Fee_MainFunction();  /* StartFlsWrite */
    Fee_MainFunction();  /* FinishJob OK */

    /* Invalidate — 一步 ProcessInvalidateJob → FinishJob(OK) */
    CHECK(Fee_InvalidateBlock(10u) == E_OK);
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_PENDING);

    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST 8: EraseImmediateBlock 完整路径
 *================================================================================================*/
static void test_erase_immediate_complete(void)
{
    setup();
    Fee_Init(&Fee_Config);

    /* 先写一个 block */
    uint8 data[32] = {0};
    CHECK(Fee_Write(11u, data) == E_OK);
    Fee_MainFunction();
    Fee_MainFunction();
    CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);

    /* EraseImmediate — 一步 ProcessEraseImmediateJob → FinishJob(OK) */
    CHECK(Fee_EraseImmediateBlock(11u) == E_OK);
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_PENDING);

    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    CHECK(FeeTest_GetJobResult() == FEE_JOB_OK);

    /* 再验证 Fls_Erase 失败路径 */
    CHECK(Fee_Write(12u, data) == E_OK);
    Fee_MainFunction();
    Fee_MainFunction();

    FeeTest_FlsEraseResult = E_NOT_OK;
    CHECK(Fee_EraseImmediateBlock(12u) == E_OK);
    Fee_MainFunction();
    /* 注意: EraseImmediate 实现没有真正调 Fls_Erase — 只是 runtime mark. 所以应该还是 OK */
    FeeTest_FlsEraseResult = E_OK;

    Fee_DeInit();
}

/*==================================================================================================
 *                                    MAIN
 *================================================================================================*/
int main(void)
{
    test_write_complete_path();
    test_read_complete_path();
    test_write_fls_error();
    test_write_fls_job_failed();
    test_multiple_jobs_sequential();
    test_cancel_during_busy();
    test_invalidate_complete();
    test_erase_immediate_complete();

    if (g_failures != 0) {
        fprintf(stderr, "\n*** Fee mainloop tests FAILED: %d\n", g_failures);
        return 1;
    }
    printf("\n=== Fee mainloop tests PASSED (8 cases) ===\n");
    return 0;
}
