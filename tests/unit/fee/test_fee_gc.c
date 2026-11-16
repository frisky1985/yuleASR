/**
 * @file test_fee_gc.c
 * @brief Fee GC (Garbage Collection) 路径单元测试
 *
 * 覆盖: IsGcNeeded → StartGc → SCAN → COPY → ERASE → FINALIZE 完整状态机
 *       + 错误分支 (StartFlsErase 失败 / FlsJob FAILED)
 *       + 找不到 source/target sector
 *       + GC 优先于正常 job
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

/* ---- expose API (extern) ---- */
extern void    FeeTest_SetJobState(uint8 state);
extern uint8   FeeTest_GetJobType(void);
extern uint8   FeeTest_GetJobState(void);
extern uint8   FeeTest_GetJobResult(void);
extern boolean FeeTest_IsInitialized(void);

extern void    FeeTest_SetFlsJobState(uint8 state);
extern uint8   FeeTest_GetFlsJobState(void);

extern void    FeeTest_SetSectorInfo(uint8 idx, boolean isValid, boolean isActive,
                                     uint32 freeSpace, uint32 sectorSize, uint8 sectorState);
extern void    FeeTest_ResetAllSectors(void);

extern uint8   FeeTest_GetGcState(void);
extern uint8   FeeTest_GetGcSourceSector(void);
extern uint8   FeeTest_GetGcTargetSector(void);
extern uint32  FeeTest_GetGcCycleCounter(void);
extern uint32  FeeTest_GetEraseCycleCounter(void);
extern uint8   FeeTest_GetModuleStatus(void);
extern boolean FeeTest_CallIsGcNeeded(void);

/* ---- stub hooks ---- */
extern Fls_JobResultType  FeeTest_FlsJobResult;
extern Std_ReturnType     FeeTest_FlsEraseResult;
extern Std_ReturnType     FeeTest_FlsWriteResult;
extern uint8              FeeTest_FlsReadFillByte;

/* ---- GC / FLS 常量 ---- */
#define FEE_GC_STATE_IDLE       (0u)
#define FEE_GC_STATE_SCAN       (1u)
#define FEE_GC_STATE_COPY       (2u)
#define FEE_GC_STATE_ERASE      (3u)
#define FEE_GC_STATE_FINALIZE   (4u)

#define FEE_FLS_JOB_IDLE        (0u)
#define FEE_FLS_JOB_PENDING     (1u)
#define FEE_FLS_JOB_COMPLETED   (2u)
#define FEE_FLS_JOB_FAILED      (3u)

#define FEE_BLOCK_STATE_EMPTY   (0x00u)
#define FEE_BLOCK_STATE_VALID   (0x01u)

#ifndef MEMIF_JOB_OK
  #define MEMIF_JOB_OK         ((Fls_JobResultType)0x00u)
#endif
#ifndef MEMIF_JOB_FAILED
  #define MEMIF_JOB_FAILED     ((Fls_JobResultType)0x01u)
#endif

/*==================================================================================================
 *                                    FEE_BUSY / FEE_IDLE (经验值)
 *================================================================================================*/
#define FEE_IDLE   (0u)
#define FEE_BUSY   (1u)

/*==================================================================================================
 *                                    SETUP
 *================================================================================================*/
extern const Fee_ConfigType Fee_Config;

/** 构造 IsGcNeeded=TRUE + FindActive + FindFree 都能找到的 sector 布局 */
static void setup_gc_scarcity(void)
{
    FeeTest_SetFlsJobState(FEE_FLS_JOB_IDLE);
    FeeTest_FlsJobResult   = MEMIF_JOB_OK;
    FeeTest_FlsEraseResult = E_OK;
    FeeTest_FlsWriteResult = E_OK;

    FeeTest_SetSectorInfo(0, TRUE, TRUE,  FALSE, 1000u, FEE_BLOCK_STATE_VALID);   /* source: 90% used */
    FeeTest_SetSectorInfo(1, TRUE, FALSE, TRUE,  1000u, FEE_BLOCK_STATE_EMPTY);   /* target: empty */
    FeeTest_SetSectorInfo(2, TRUE, FALSE, TRUE,  1000u, FEE_BLOCK_STATE_EMPTY);   /* target 备选 */
    FeeTest_SetSectorInfo(3, FALSE,FALSE, FALSE, 0u,    FEE_BLOCK_STATE_EMPTY);   /* 未使用 */

    /* totalFree=100 (sector 0) + 1000 + 1000 = 2100... 不对, 我要 totalFree < 20% */
    /* 重新: sector 0 freeSpace=100, sector 1 freeSpace=50, sector 2 freeSpace=50, sector 3 invalid */
    FeeTest_SetSectorInfo(0, TRUE, TRUE,  FALSE, 1000u, FEE_BLOCK_STATE_VALID);
    FeeTest_SetSectorInfo(1, TRUE, FALSE, TRUE,  1000u, FEE_BLOCK_STATE_EMPTY);
    FeeTest_SetSectorInfo(2, TRUE, FALSE, TRUE,  1000u, FEE_BLOCK_STATE_EMPTY);
    /* 这时候 totalFree=2100, totalSize=3000, freePct=70% → GC 不需要 */
    /* 我们要 freePct < 20%, 即 totalFree < totalSize * 0.2 */
    /* 简化: 让 sector 0 是唯一 valid, FreeSpace=100, Size=1000 → freePct=10% → GC */
}

/** 最简构造: 单 sector 只有 10% free, 另一个 sector 作为 empty target */
static void setup_gc_minimal(void)
{
    /* 先 reset */
    FeeTest_ResetAllSectors();

    /* sector 0: valid + active, size=1000, free=100 → 90% used */
    FeeTest_SetSectorInfo(0, /*valid=*/TRUE, /*active=*/TRUE,
                           /*free=*/100u, /*size=*/1000u, FEE_BLOCK_STATE_VALID);
    /* sector 1: valid + empty + not active — target candidate */
    FeeTest_SetSectorInfo(1, /*valid=*/TRUE, /*active=*/FALSE,
                           /*free=*/1000u, /*size=*/1000u, FEE_BLOCK_STATE_EMPTY);

    /* totalFree=1100, totalSize=2000 → freePct=55% → GC 不需要!
     * 改成 sector 1 也只剩 100 free */
    FeeTest_SetSectorInfo(1, TRUE, FALSE, 100u, 1000u, FEE_BLOCK_STATE_EMPTY);

    /* totalFree=200, totalSize=2000 → freePct=10% < 20% → GC! */

    FeeTest_FlsJobResult   = MEMIF_JOB_OK;
    FeeTest_FlsEraseResult = E_OK;
}

/*==================================================================================================
 *                                    TEST GC-0: IsGcNeeded 正确判断
 *================================================================================================*/
static void test_gc_needed_propagation(void)
{
    Fee_Init(&Fee_Config);

    /* 默认构造 — 所有 sector valid=TRUE 但 Init 会把 FreeSpace 设满, 所以 IsGcNeeded=FALSE */
    CHECK(FeeTest_CallIsGcNeeded() == FALSE);

    /* 手动构造 scarcity */
    setup_gc_minimal();
    CHECK(FeeTest_CallIsGcNeeded() == TRUE);

    /* 把 source sector 设 IsActive=FALSE → FindActive=0xFF, GC 不会触发 */
    FeeTest_SetSectorInfo(0, TRUE, FALSE, 100u, 1000u, FEE_BLOCK_STATE_VALID);
    CHECK(FeeTest_CallIsGcNeeded() == TRUE);   /* IsGcNeeded 只看 freePct, 不看 IsActive */

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-1: 完整 GC 路径 (6 次 MainFunction)
 *
 * MainFunction 每次顺序: CheckFlsJobStatus → ProcessGc (if active) → ProcessJob → IsGcNeeded→StartGc
 * GC-1 完整状态机:
 *   #1 IDLE → StartGC → SCAN
 *   #2 SCAN → Scan → COPY
 *   #3 COPY → CopyBlocks → ERASE  (COPY→ERASE 后直接 return, ERASE case 没跑到)
 *   #4 ERASE → FlsJob=IDLE → StartFlsErase → PENDING (GcState 仍 ERASE, 等下次 CheckFls)
 *   #5 CheckFls: PENDING → COMPLETED; ERASE case → COMPLETED → FINALIZE
 *   #6 FINALIZE → IDLE + GcCycleCounter++
 *================================================================================================*/
static void test_gc_complete_path(void)
{
    Fee_Init(&Fee_Config);
    setup_gc_minimal();

    CHECK(FeeTest_CallIsGcNeeded() == TRUE);
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);
    uint32 gc_before = FeeTest_GetGcCycleCounter();
    uint32 er_before = FeeTest_GetEraseCycleCounter();

    /* #1: StartGC → SCAN */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_SCAN);
    CHECK(FeeTest_GetGcSourceSector() == 0u);
    CHECK(FeeTest_GetGcTargetSector() == 1u);

    /* #2: SCAN → COPY */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_COPY);

    /* #3: COPY → ERASE (CopyBlocks 直接设, Fls 还没启动) */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_ERASE);

    /* #4: ERASE case → StartFlsErase (FlsJob=IDLE→PENDING) */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_ERASE);
    CHECK(FeeTest_GetFlsJobState() == FEE_FLS_JOB_PENDING);

    /* #5: CheckFlsJobStatus: PENDING → Fls_GetJobResult=OK → FlsJob=COMPLETED
     *     ERASE case: FlsJob==COMPLETED → FINALIZE + EraseCycleCounter++ */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_FINALIZE);
    CHECK(FeeTest_GetEraseCycleCounter() == er_before + 1u);

    /* #6: FINALIZE → IDLE + GcCycleCounter++ */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);
    CHECK(FeeTest_GetGcCycleCounter() == gc_before + 1u);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-2: FindActiveSector 返回 0xFF — GC abort
 *================================================================================================*/
static void test_gc_no_source_sector(void)
{
    Fee_Init(&Fee_Config);
    FeeTest_ResetAllSectors();

    /* 两个 sector 都 valid + freeSpace 少 (IsGcNeeded=TRUE), 但都 IsActive=FALSE */
    FeeTest_SetSectorInfo(0, TRUE, FALSE, 100u, 1000u, FEE_BLOCK_STATE_VALID);
    FeeTest_SetSectorInfo(1, TRUE, FALSE, 100u, 1000u, FEE_BLOCK_STATE_EMPTY);
    /* totalFree=200, totalSize=2000, freePct=10% < 20% → IsGcNeeded=TRUE */

    CHECK(FeeTest_CallIsGcNeeded() == TRUE);

    /* MainFunction → StartGC → FindActive=0xFF → return, GcState 保持 IDLE */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-3: FindFreeSector 返回 0xFF — GC abort
 *================================================================================================*/
static void test_gc_no_target_sector(void)
{
    Fee_Init(&Fee_Config);
    FeeTest_ResetAllSectors();

    /* sector 0: valid + active (source), 但 sector 1/2 都不是 valid 或 EMPTY */
    FeeTest_SetSectorInfo(0, TRUE,  TRUE,  100u, 1000u, FEE_BLOCK_STATE_VALID);
    FeeTest_SetSectorInfo(1, TRUE,  TRUE,  100u, 1000u, FEE_BLOCK_STATE_VALID);
    /* sector 3 invalid */

    CHECK(FeeTest_CallIsGcNeeded() == TRUE);

    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-4: StartFlsErase 返回 E_NOT_OK — abort GC
 *
 * StartFlsErase 实际在第 #4 次 MainFunction 的 ERASE case 里调
 * #1 → SCAN, #2 → COPY, #3 → ERASE (CopyBlocks), #4 ERASE→StartFlsErase 失败 → IDLE
 *================================================================================================*/
static void test_gc_erase_start_fails(void)
{
    Fee_Init(&Fee_Config);
    setup_gc_minimal();
    FeeTest_FlsEraseResult = E_NOT_OK;  /* 强制失败 */

    Fee_MainFunction();  /* #1 → SCAN */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_SCAN);
    Fee_MainFunction();  /* #2 → COPY */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_COPY);
    Fee_MainFunction();  /* #3 → ERASE (CopyBlocks) */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_ERASE);
    Fee_MainFunction();  /* #4: ERASE case, StartFlsErase → E_NOT_OK → GcState=IDLE */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);

    FeeTest_FlsEraseResult = E_OK;
    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-5: Fls_JobResult FAILED — abort GC
 *
 * GC 走到 ERASE + FlsJob=PENDING 后, 改 FlsJobResult 为 FAILED, 下次 MainFunction 触发 CheckFls → ERASE→IDLE
 * #1 SCAN, #2 COPY, #3 ERASE, #4 StartFlsErase→PENDING
 * set JobResult=FAILED
 * #5 CheckFls: PENDING→FAILED, ERASE case → IDLE (abort)
 *================================================================================================*/
static void test_gc_fls_job_failed(void)
{
    Fee_Init(&Fee_Config);
    setup_gc_minimal();

    Fee_MainFunction();  /* #1 → SCAN */
    Fee_MainFunction();  /* #2 → COPY */
    Fee_MainFunction();  /* #3 → ERASE */
    Fee_MainFunction();  /* #4: ERASE case, StartFlsErase → PENDING */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_ERASE);
    CHECK(FeeTest_GetFlsJobState() == FEE_FLS_JOB_PENDING);

    /* 强制 Fls 异步返回 FAILED */
    FeeTest_FlsJobResult = MEMIF_JOB_FAILED;

    /* #5: CheckFls: PENDING→FAILED, ERASE case → FAILED → IDLE (abort) */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);
    CHECK(FeeTest_GetEraseCycleCounter() == 0u);

    FeeTest_FlsJobResult = MEMIF_JOB_OK;
    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-6: GC 优先于正常 job
 *
 * setup 后 IsGcNeeded=TRUE. Fee_Write 会在入队前自动触发 StartGarbageCollection,
 * 所以 Write 入队时 GcState 已经是 SCAN (不是 IDLE).
 * GC 走完 SCAN→COPY→ERASE→StartFlsErase→FINALIZE→IDLE = 5 MF (不是 6).
 * GC 期间 ProcessGc return 让 ProcessJob 跳过, Write 不被处理.
 * GC 完后 ProcessJob 接手 Write (2 MF 走完).
 * 总共: 5 (GC) + 2 (Write) = 7 MF
 *================================================================================================*/
static void test_gc_has_priority(void)
{
    Fee_Init(&Fee_Config);
    setup_gc_minimal();
    uint32 gc_before = FeeTest_GetGcCycleCounter();

    /* Block 7 is configured with BlockSize == 128; Fee_Write reads that many
     * bytes from the caller buffer, so it must be block-sized. */
    uint8 data[128] = {0};

    /* 入队 Write — ModuleStatus=IDLE. Fee_Write 内部检测 IsGcNeeded=TRUE,
     * 触发 StartGarbageCollection() → GcState=SCAN, 然后入队 Write */
    CHECK(Fee_Write(7u, data) == E_OK);
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_SCAN);  /* Write 里已触发 GC */

    /* 完整 GC (5 MF: COPY → ERASE → StartFlsErase → COMPLETED → FINALIZE → IDLE) */
    Fee_MainFunction();  /* MF#1: SCAN → COPY */
    Fee_MainFunction();  /* MF#2: COPY → ERASE */
    Fee_MainFunction();  /* MF#3: ERASE + StartFlsErase → PENDING */
    Fee_MainFunction();  /* MF#4: CheckFls PENDING→COMPLETED → ERASE→COMPLETED→FINALIZE */
    Fee_MainFunction();  /* MF#5: FINALIZE → IDLE, GcCycleCounter++ */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);
    CHECK(FeeTest_GetGcCycleCounter() == gc_before + 1u);
    /* GC 期间 Write 不应被处理 — ProcessGc return 让 ProcessJob 跳过 */

    /* #6: ProcessJob PENDING → StartFlsWrite → PENDING+1 */
    Fee_MainFunction();
    CHECK(FeeTest_GetJobState() == (uint8)(FEE_FLS_JOB_PENDING + 1u));

    /* #7: CheckFls → COMPLETED, ProcessWriteJob → CompleteWriteJob → FinishJob OK */
    Fee_MainFunction();
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);
    CHECK(FeeTest_GetJobResult() == 0u);  /* FEE_JOB_OK */

    Fee_DeInit();
}

/*==================================================================================================
 *                                    TEST GC-7: 连续两次 GC (每次 6 MF, 共 12 MF)
 *================================================================================================*/
static void test_gc_run_twice(void)
{
    Fee_Init(&Fee_Config);
    uint32 gc_before = FeeTest_GetGcCycleCounter();

    /* === 第一次 GC === */
    setup_gc_minimal();

    Fee_MainFunction();  /* #1 → SCAN */
    Fee_MainFunction();  /* #2 → COPY */
    Fee_MainFunction();  /* #3 → ERASE */
    Fee_MainFunction();  /* #4 → ERASE + PENDING */
    Fee_MainFunction();  /* #5 → FINALIZE */
    Fee_MainFunction();  /* #6 → IDLE */
    CHECK(FeeTest_GetGcCycleCounter() == gc_before + 1u);

    /* Finalize 后: source=0 → FreeSpace=1000, SectorState=EMPTY, IsActive=FALSE
     *              target=1 → IsActive=TRUE, SectorState=VALID, FreeSpace=100
     * 但 sector 2 还是 IsValid=TRUE, FreeSpace=1000 (Init 默认值)
     * totalFree = 1000 + 100 + 1000 = 2100, totalSize = 3000 → 70% → 不触发
     * 手动构造 scarcity */
    FeeTest_SetSectorInfo(0, TRUE, FALSE, 50u, 1000u, FEE_BLOCK_STATE_EMPTY);
    FeeTest_SetSectorInfo(1, TRUE, TRUE,  100u, 1000u, FEE_BLOCK_STATE_VALID);
    FeeTest_SetSectorInfo(2, TRUE, FALSE, 50u, 1000u, FEE_BLOCK_STATE_EMPTY);
    /* totalFree=200, totalSize=3000 → 6.7% < 20% → GC! */

    CHECK(FeeTest_CallIsGcNeeded() == TRUE);

    /* === 第二次 GC === */
    Fee_MainFunction();  /* #7 → SCAN (source=1, target=0 或 2) */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_SCAN);
    Fee_MainFunction();  /* #8 → COPY */
    Fee_MainFunction();  /* #9 → ERASE */
    Fee_MainFunction();  /* #10 → ERASE + PENDING */
    Fee_MainFunction();  /* #11 → FINALIZE */
    Fee_MainFunction();  /* #12 → IDLE */
    CHECK(FeeTest_GetGcState() == FEE_GC_STATE_IDLE);
    CHECK(FeeTest_GetGcCycleCounter() == gc_before + 2u);

    Fee_DeInit();
}

/*==================================================================================================
 *                                    MAIN
 *================================================================================================*/
int main(void)
{
    test_gc_needed_propagation();
    test_gc_complete_path();
    test_gc_no_source_sector();
    test_gc_no_target_sector();
    test_gc_erase_start_fails();
    test_gc_fls_job_failed();
    test_gc_has_priority();
    test_gc_run_twice();

    if (g_failures != 0) {
        fprintf(stderr, "\n*** Fee GC tests FAILED: %d\n", g_failures);
        return 1;
    }
    printf("\n=== Fee GC tests PASSED (8 cases) ===\n");
    return 0;
}
