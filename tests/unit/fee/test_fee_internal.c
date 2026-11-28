/**
 * @file test_fee_internal.c
 * @brief Fee 内部状态机 / 静态助手 白盒测试（fee_expose 编译单元）
 *
 * 背景: 本仓库在使用系统 Unity 包时，test_framework.h 的 RUN_TEST 退化为
 *       直接调用宏，不执行 setUp/tearDown，因此 "direct" 组测试实际在未初始化
 *       状态下运行，无法覆盖真实的初始化后分支。
 *
 * 本文件不依赖 setUp：每个用例显式 Fee_Init / 注入内部状态，通过 fee_expose
 * 暴露的 setter 与 static 函数（static 已被移除为全局符号）精确驱动状态机，
 * 覆盖错误分支、default 分支与通知回调。
 */

#define UNIT_TEST
#include "Fee.h"
#include "Fls.h"
#include "Det.h"

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

/*==================================================================================================
 *                                    locals (mirror Fee.c defines)
 *================================================================================================*/
#define FEE_JOB_READ            (1u)
#define FEE_JOB_WRITE           (2u)
#define FEE_FLS_JOB_IDLE        (0u)
#define FEE_FLS_JOB_PENDING     (1u)
#define FEE_GC_STATE_IDLE       (0u)
#define FEE_BLOCK_STATE_EMPTY   (0x00u)
#define FEE_BLOCK_STATE_VALID   (0x01u)

/*==================================================================================================
 *                                    expose helpers
 *================================================================================================*/
extern void    FeeTest_SetJobType(uint8 type);
extern void    FeeTest_SetJobState(uint8 state);
extern void    FeeTest_SetFlsJobState(uint8 state);
extern void    FeeTest_SetModuleStatus(uint8 status);
extern void    FeeTest_SetConfigPtr(const Fee_ConfigType* cfg);
extern uint8   FeeTest_GetJobResult(void);
extern uint8   FeeTest_GetJobState(void);
extern boolean FeeTest_IsInitialized(void);

extern void    FeeTest_ResetAllSectors(void);
extern void    FeeTest_SetSectorInfo(uint8 idx, boolean isValid, boolean isActive,
                                     uint32 freeSpace, uint32 sectorSize, uint8 sectorState);
extern void    FeeTest_SetGcState(uint8 s);
extern boolean FeeTest_CallIsGcNeeded(void);

/* static helpers, exported by fee_expose.c (static removed) */
extern void          Fee_ScanSectors(void);
extern void          Fee_BuildBlockTable(void);
extern Std_ReturnType Fee_FindBlockAddress(Fee_BlockIdType BlockId, Fee_AddressType* AddressPtr);
extern Std_ReturnType Fee_StartFlsRead(Fls_AddressType Address, uint8* DataPtr, uint32 Length);
extern Std_ReturnType Fee_StartFlsWrite(Fls_AddressType Address, const uint8* DataPtr, uint32 Length);
extern Std_ReturnType Fee_StartFlsErase(Fls_AddressType Address, uint32 Length);
extern void          Fee_ProcessFlsJob(void);
extern void          Fee_UpdateWearLeveling(void);
extern boolean       Fee_IsAddressInSector(Fee_AddressType Address, uint8 SectorIndex);

/* stub hooks */
extern Fls_JobResultType  FeeTest_FlsJobResult;
extern Std_ReturnType     FeeTest_FlsEraseResult;
extern Std_ReturnType     FeeTest_FlsWriteResult;
extern uint8              FeeTest_FlsReadFillByte;

extern const Fee_ConfigType Fee_Config;

/*==================================================================================================
 *                                    SETUP
 *================================================================================================*/
static void reset_fee(void)
{
    if (FeeTest_IsInitialized() == TRUE)
    {
        Fee_DeInit();
    }
    FeeTest_FlsJobResult    = MEMIF_JOB_OK;
    FeeTest_FlsEraseResult  = E_OK;
    FeeTest_FlsWriteResult  = E_OK;
    FeeTest_FlsReadFillByte = 0xFFu;
    Fee_Init(&Fee_Config);
}

/* 写一个 block 并跑完两拍 MainFunction, 使其进入 valid 状态 */
static void make_block_valid(Fee_BlockIdType id)
{
    uint8 data[1024] = {0};
    (void)Fee_Write(id, data);
    Fee_MainFunction();   /* StartFlsWrite */
    Fee_MainFunction();   /* CompleteWriteJob */
}

/*==================================================================================================
 * 1. DeInit while BUSY → 走 Fee_Cancel 分支
 *================================================================================================*/
static void test_deinit_while_busy(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    CHECK(Fee_Write(1u, data) == E_OK);
    CHECK(Fee_GetStatus() == FEE_BUSY);
    Fee_DeInit();                       /* ModuleStatus==BUSY → Fee_Cancel */
    CHECK(FeeTest_IsInitialized() == FALSE);
}

/*==================================================================================================
 * 2. SetMode 合法/非法
 *================================================================================================*/
static void test_setmode_paths(void)
{
    reset_fee();
    Fee_SetMode(FEE_MODE_FAST);         /* 408/411/413 */
    Fee_SetMode(FEE_MODE_SLOW);         /* 417 */
    Fee_SetMode((Fee_ModeType)99);      /* 398-401 invalid mode */
    Fee_DeInit();
}

/*==================================================================================================
 * 3. Read DET guards
 *================================================================================================*/
static void test_read_det_guards(void)
{
    uint8 buf[16] = {0};
    reset_fee();
    CHECK(Fee_Read(9999u, 0u, buf, 16u) == E_NOT_OK);        /* 451-452 */
    CHECK(Fee_Read(0u, 0xFFFFu, buf, 16u) == E_NOT_OK);      /* 457-458 */
    CHECK(Fee_Read(0u, 0u, NULL_PTR, 16u) == E_NOT_OK);      /* 463-464 */
    CHECK(Fee_Read(0u, 0u, buf, 0u) == E_NOT_OK);            /* 469-470 */
    Fee_DeInit();
}

/*==================================================================================================
 * 4. Read 无效 / 已失效 block
 *================================================================================================*/
static void test_read_invalid_blocks(void)
{
    uint8 buf[16] = {0};
    reset_fee();
    CHECK(Fee_Read(5u, 0u, buf, 16u) == E_NOT_OK);           /* 477-481 not valid */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_BLOCK_INVALID);

    make_block_valid(1u);
    CHECK(Fee_InvalidateBlock(1u) == E_OK);
    Fee_MainFunction();                                      /* ProcessInvalidateJob */
    CHECK(Fee_Read(1u, 0u, buf, 16u) == E_NOT_OK);           /* 484-488 invalidated */
    Fee_DeInit();
}

/*==================================================================================================
 * 5. Read while BUSY
 *================================================================================================*/
static void test_read_while_busy(void)
{
    uint8 data[1024] = {0};
    uint8 buf[16] = {0};
    reset_fee();
    make_block_valid(1u);
    CHECK(Fee_Write(3u, data) == E_OK);                      /* 置 BUSY */
    CHECK(Fee_Read(1u, 0u, buf, 16u) == E_NOT_OK);           /* 492-495 */
    Fee_DeInit();
}

/*==================================================================================================
 * 6. Write DET guards
 *================================================================================================*/
static void test_write_det_guards(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    CHECK(Fee_Write(9999u, data) == E_NOT_OK);               /* 537-538 */
    CHECK(Fee_Write(0u, NULL_PTR) == E_NOT_OK);              /* 543-544 */
    Fee_DeInit();
}

/*==================================================================================================
 * 7. Write while BUSY
 *================================================================================================*/
static void test_write_while_busy(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    CHECK(Fee_Write(1u, data) == E_OK);
    CHECK(Fee_Write(2u, data) == E_NOT_OK);                  /* 551-554 */
    Fee_DeInit();
}

/*==================================================================================================
 * 8. Write with NULL config pointer
 *================================================================================================*/
static void test_write_null_config(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    FeeTest_SetConfigPtr(NULL_PTR);
    CHECK(Fee_Write(1u, data) == E_NOT_OK);                  /* 558 false → 564-565 */
    FeeTest_SetConfigPtr(&Fee_Config);
    Fee_DeInit();
}

/*==================================================================================================
 * 9. 初始化后 getter
 *================================================================================================*/
static void test_getters_after_init(void)
{
    reset_fee();
    CHECK(Fee_GetJobResult() == FEE_JOB_OK);                 /* 661 */
    (void)Fee_GetCycleCount();                               /* 824 */
    (void)Fee_GetEraseCycleCount();                          /* 841 */
    (void)Fee_GetWriteCycleCount();                          /* 858 */
    Fee_DeInit();
}

/*==================================================================================================
 * 10. Invalidate / EraseImmediate DET guards + BUSY
 *================================================================================================*/
static void test_invalidate_guards(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    CHECK(Fee_InvalidateBlock(9999u) == E_NOT_OK);           /* 681-684 */
    CHECK(Fee_Write(1u, data) == E_OK);
    CHECK(Fee_InvalidateBlock(1u) == E_NOT_OK);              /* 691-694 */
    Fee_DeInit();
}

static void test_erase_immediate_guards(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    CHECK(Fee_EraseImmediateBlock(9999u) == E_NOT_OK);       /* 729-732 */
    CHECK(Fee_Write(1u, data) == E_OK);
    CHECK(Fee_EraseImmediateBlock(1u) == E_NOT_OK);          /* 739-742 */
    Fee_DeInit();
}

/*==================================================================================================
 * 11. MainFunction on uninitialized module
 *================================================================================================*/
static void test_mainfunction_uninit(void)
{
    reset_fee();
    Fee_DeInit();
    Fee_MainFunction();                                      /* 867-869 early return */
    CHECK(FeeTest_IsInitialized() == FALSE);
}

/*==================================================================================================
 * 12. FLS 作业回调 + 包装
 *================================================================================================*/
static void test_fls_callbacks(void)
{
    reset_fee();
    Fee_FlsJobEndNotification();                             /* 913 false */
    FeeTest_SetFlsJobState(FEE_FLS_JOB_PENDING);
    Fee_FlsJobEndNotification();                             /* 915 COMPLETED */
    FeeTest_SetFlsJobState(FEE_FLS_JOB_PENDING);
    Fee_FlsJobErrorNotification();                           /* 931 FAILED */

    FeeTest_SetFlsJobState(FEE_FLS_JOB_PENDING);
    Fls_JobEndNotification();                                /* 1766-1768 */
    FeeTest_SetFlsJobState(FEE_FLS_JOB_PENDING);
    Fls_JobErrorNotification();                              /* 1771-1774 */
    Fee_DeInit();
}

/*==================================================================================================
 * 13. Scan/BuildBlockTable 边界
 *================================================================================================*/
static void test_scan_zero_size_sector(void)
{
    reset_fee();
    FeeTest_ResetAllSectors();
    /* valid sector with size 0 → while 不执行, 直接走 994 的 end-of-sector 分支 */
    FeeTest_SetSectorInfo(0u, TRUE, FALSE, 0u, 0u, FEE_BLOCK_STATE_EMPTY);
    Fee_ScanSectors();                                       /* 996-998 */
    Fee_DeInit();
}

static void test_build_block_table_fallback(void)
{
    reset_fee();
    FeeTest_ResetAllSectors();
    /* FreeSpace==0 → 第一轮不选; 第二轮 valid 兜底选中并重置 */
    FeeTest_SetSectorInfo(0u, TRUE, FALSE, 0u, 1000u, FEE_BLOCK_STATE_VALID);
    Fee_BuildBlockTable();                                   /* 1027-1035 */
    Fee_DeInit();
}

/*==================================================================================================
 * 14. FindBlockAddress 错误分支
 *================================================================================================*/
static void test_find_block_address_errors(void)
{
    Fee_AddressType addr = 0u;
    reset_fee();
    CHECK(Fee_FindBlockAddress(9999u, &addr) == E_NOT_OK);   /* 1099 */
    CHECK(Fee_FindBlockAddress(15u, &addr) == E_NOT_OK);     /* 1104 unwritten */
    Fee_DeInit();
}

/*==================================================================================================
 * 15. IsGcNeeded 无有效 sector
 *================================================================================================*/
static void test_is_gc_needed_no_valid(void)
{
    reset_fee();
    FeeTest_ResetAllSectors();
    CHECK(FeeTest_CallIsGcNeeded() == FALSE);                /* 1208 */
    Fee_DeInit();
}

/*==================================================================================================
 * 16. ProcessGc default 分支
 *================================================================================================*/
static void test_process_gc_default(void)
{
    reset_fee();
    FeeTest_SetGcState(99u);
    Fee_MainFunction();                                      /* 1270-1272 → IDLE */
    CHECK(FeeTest_GetJobState() == FEE_FLS_JOB_IDLE);
    Fee_DeInit();
}

/*==================================================================================================
 * 17. StartFls* 已有 pending 时拒绝
 *================================================================================================*/
static void test_start_fls_pending(void)
{
    uint8 buf[16] = {0};
    reset_fee();
    FeeTest_SetFlsJobState(FEE_FLS_JOB_PENDING);
    CHECK(Fee_StartFlsRead(0x1000u, buf, 16u) == E_NOT_OK);      /* 1374 */
    CHECK(Fee_StartFlsWrite(0x1000u, buf, 16u) == E_NOT_OK);     /* 1395 */
    CHECK(Fee_StartFlsErase(0x1000u, 16u) == E_NOT_OK);          /* 1414 */
    Fee_DeInit();
}

/*==================================================================================================
 * 18. ProcessJob / ProcessReadJob / ProcessWriteJob default 分支
 *================================================================================================*/
static void test_process_job_default(void)
{
    reset_fee();
    FeeTest_SetGcState(FEE_GC_STATE_IDLE);
    FeeTest_SetJobType(99u);
    FeeTest_SetJobState(0u);
    FeeTest_SetModuleStatus((uint8)FEE_BUSY);
    Fee_MainFunction();                                      /* 1472-1474 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_JOB_FAILED);
    Fee_DeInit();
}

static void test_read_job_find_block_fail(void)
{
    reset_fee();
    FeeTest_SetGcState(FEE_GC_STATE_IDLE);
    FeeTest_SetJobType(FEE_JOB_READ);
    FeeTest_SetJobState(FEE_FLS_JOB_PENDING);
    FeeTest_SetModuleStatus((uint8)FEE_BUSY);   /* BlockId=0 未写 → FindBlockAddress 失败 */
    Fee_MainFunction();                                      /* 1494-1495 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_BLOCK_INVALID);
    Fee_DeInit();
}

static void test_read_default_state(void)
{
    reset_fee();
    FeeTest_SetGcState(FEE_GC_STATE_IDLE);
    FeeTest_SetJobType(FEE_JOB_READ);
    FeeTest_SetJobState(99u);
    FeeTest_SetModuleStatus((uint8)FEE_BUSY);
    Fee_MainFunction();                                      /* 1525-1527 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_JOB_FAILED);
    Fee_DeInit();
}

static void test_write_default_state(void)
{
    reset_fee();
    FeeTest_SetGcState(FEE_GC_STATE_IDLE);
    FeeTest_SetJobType(FEE_JOB_WRITE);
    FeeTest_SetJobState(99u);
    FeeTest_SetModuleStatus((uint8)FEE_BUSY);
    Fee_MainFunction();                                      /* 1662-1664 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_JOB_FAILED);
    Fee_DeInit();
}

/*==================================================================================================
 * 19. Read: StartFlsRead 失败 / FLS 失败 / 正常等待
 *================================================================================================*/
static void test_read_start_fails(void)
{
    uint8 buf[16] = {0};
    reset_fee();
    make_block_valid(1u);
    CHECK(Fee_Read(1u, 0u, buf, 16u) == E_OK);
    /* 让 FLS 保持 pending, 使 StartFlsRead 拒绝启动 */
    FeeTest_SetFlsJobState(FEE_FLS_JOB_PENDING);
    FeeTest_FlsJobResult = MEMIF_JOB_PENDING;
    Fee_MainFunction();                                      /* 1504-1505 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_JOB_FAILED);
    FeeTest_FlsJobResult = MEMIF_JOB_OK;
    Fee_DeInit();
}

static void test_read_wait_failed(void)
{
    uint8 buf[16] = {0};
    reset_fee();
    make_block_valid(1u);
    CHECK(Fee_Read(1u, 0u, buf, 16u) == E_OK);
    Fee_MainFunction();                                      /* StartFlsRead → PENDING+1 */
    FeeTest_FlsJobResult = MEMIF_JOB_FAILED;
    Fee_MainFunction();                                      /* 1519-1521 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_JOB_FAILED);
    FeeTest_FlsJobResult = MEMIF_JOB_OK;
    Fee_DeInit();
}

/*==================================================================================================
 * 20. Write: 无可用 sector
 *================================================================================================*/
static void test_write_no_sector(void)
{
    uint8 data[1024] = {0};
    reset_fee();
    FeeTest_ResetAllSectors();               /* 全 invalid → SelectSector 返回 0xFF */
    CHECK(Fee_Write(1u, data) == E_OK);
    Fee_MainFunction();                                      /* 1634 */
    CHECK(FeeTest_GetJobResult() == (uint8)FEE_JOB_FAILED);
    Fee_DeInit();
}

/*==================================================================================================
 * 21. 杂项静态助手
 *================================================================================================*/
static void test_misc_helpers(void)
{
    reset_fee();
    Fee_ProcessFlsJob();                                     /* 899-903 no-op */
    Fee_UpdateWearLeveling();                                /* 1724-1728 no-op */

    FeeTest_ResetAllSectors();
    FeeTest_SetSectorInfo(0u, TRUE, FALSE, 1000u, 1000u, FEE_BLOCK_STATE_VALID);
    /* sector0 start = 0x10000 */
    CHECK(Fee_IsAddressInSector(0x10005u, 0u) == TRUE);      /* 1752-1754 */
    CHECK(Fee_IsAddressInSector(0x20000u, 0u) == FALSE);
    CHECK(Fee_IsAddressInSector(0x10000u, 99u) == FALSE);    /* 1747-1750 */
    Fee_DeInit();
}

/*==================================================================================================
 *                                    MAIN
 *================================================================================================*/
int main(void)
{
    test_deinit_while_busy();
    test_setmode_paths();
    test_read_det_guards();
    test_read_invalid_blocks();
    test_read_while_busy();
    test_write_det_guards();
    test_write_while_busy();
    test_write_null_config();
    test_getters_after_init();
    test_invalidate_guards();
    test_erase_immediate_guards();
    test_mainfunction_uninit();
    test_fls_callbacks();
    test_scan_zero_size_sector();
    test_build_block_table_fallback();
    test_find_block_address_errors();
    test_is_gc_needed_no_valid();
    test_process_gc_default();
    test_start_fls_pending();
    test_process_job_default();
    test_read_job_find_block_fail();
    test_read_default_state();
    test_write_default_state();
    test_read_start_fails();
    test_read_wait_failed();
    test_write_no_sector();
    test_misc_helpers();

    if (g_failures != 0)
    {
        fprintf(stderr, "\n*** Fee internal tests FAILED: %d\n", g_failures);
        return 1;
    }
    printf("\n=== Fee internal tests PASSED (27 cases) ===\n");
    return 0;
}
