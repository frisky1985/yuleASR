/**
 * @file test_ramtst.c
 * @brief RamTst Unit Tests (substantiated)
 * @version 1.0.0
 * @date 2026-10-26
 *
 * Links the real production source src/bsw/mcal/ramtst/src/RamTst.c against
 * the mock register model and the mock_det error hook.
 *
 * Notes on driver behaviour covered here:
 *  - RamTst is a pure-software module: REG_READ32/REG_WRITE32 are redirected
 *    by mock_registers.h to a keyed address/value table (256 entries), so the
 *    test region must stay small (<= 16 words).
 *  - Each RamTst_MainFunction() call performs exactly one word operation for
 *    March-C/Checkerboard/GALPAT (plus step transitions), so call counts are
 *    deterministic and used to place faults at the precise algorithm step.
 *  - A 1-word March-C region completes in 12 calls
 *    (M0,M1,M2,M3,M4 = write/verify phases with transitions; M5 = final read).
 *  - Tests that must observe the genuine uninitialized state are named
 *    *BeforeInit* so the generated runner executes them first.
 */

// @tests src/bsw/mcal/ramtst/src/RamTst.c  @tests src/bsw/mcal/ramtst/include/RamTst.h

#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "RamTst.h"
#include "RamTst_Cfg.h"

/*===========================================================================
 * Test region
 *==========================================================================*/
#define TEST_REGION_BASE                (0x20000000UL)
#define TEST_STEP                       (4U)

/* March-C 1-word completion sequence: per MainFunction call one word op +
 * any step transitions execute inline (the while loop does not stop at
 * transitions), so 7 calls complete a 1-word region. */
#define MARCH_C_1WORD_CALLS             (7U)
/* M5 (final read) happens on call 6 -> corrupt at index 5 */
#define MARCH_C_M5_INDEX                (5U)

/* GALPAT per-bit cycle: 2 bg writes + trans + 2 mask writes + trans +
 * 2 verifies + trans(bit++) = 9 calls */
#define GALPAT_CYCLE_CALLS              (9U)

/*===========================================================================
 * Helpers
 *==========================================================================*/
static RamTst_ConfigType test_Config;

static void test_SetupConfig(RamTst_AlgType alg, uint32 sizeBytes,
                             uint32 timeoutMs, boolean stopOnError)
{
    test_Config.StartAddress = TEST_REGION_BASE;
    test_Config.Size         = sizeBytes;
    test_Config.Algorithm    = alg;
    test_Config.CallCycle    = 10U;
    test_Config.TimeoutMs    = timeoutMs;
    test_Config.StopOnError  = stopOnError;
    test_Config.PatternSeed  = 0xA5A5A5A5UL;
}

/* Run MainFunction up to maxCalls times; stop early once the test completes.
 * Returns the number of calls actually made. */
static uint32 test_RunMain(uint32 maxCalls)
{
    uint32 i;

    for (i = 0U; i < maxCalls; i++) {
        if (RamTst_GetTestStatus() == RAMTST_STATUS_COMPLETED) {
            break;
        }
        RamTst_MainFunction();
    }
    return i;
}

void setUp(void)
{
    Det_Mock_Reset();
    MockRegisters_Reset();
}

void tearDown(void)
{
    RamTst_DeInit();
}

/*===========================================================================
 * BeforeInit tests (executed first by the generated runner)
 *==========================================================================*/
void test_GetStatusBeforeInit_ReturnsUninit(void)
{
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_UNINIT, RamTst_GetTestStatus());
}

void test_GetResultBeforeInit_ReturnsNotTested(void)
{
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}

void test_RunBeforeInit_ReportsDetAndRejects(void)
{
    Std_ReturnType ret = RamTst_Run();

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(1U, Det_Mock_GetCallCount());
}

/*===========================================================================
 * Init / DeInit / version
 *==========================================================================*/
void test_Init_NullConfig_ReportsDet(void)
{
    RamTst_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT32(1U, Det_Mock_GetCallCount());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_UNINIT, RamTst_GetTestStatus());
}

void test_Init_ValidConfig_EntersIdle(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 5000U, FALSE);
    RamTst_Init(&test_Config);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}

void test_DeInit_ResetsToUninit(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 5000U, FALSE);
    RamTst_Init(&test_Config);
    RamTst_DeInit();

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_UNINIT, RamTst_GetTestStatus());
}

void test_GetVersionInfo_PopulatesFields(void)
{
    Std_VersionInfoType info;

    RamTst_GetVersionInfo(&info);

    TEST_ASSERT_EQUAL_UINT16(0x0055U, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(0x64U, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(2U, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(0U, info.sw_patch_version);
}

void test_GetVersionInfo_NullPtr_ReportsDet(void)
{
    RamTst_GetVersionInfo(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT32(1U, Det_Mock_GetCallCount());
}

/*===========================================================================
 * Run parameter validation
 *==========================================================================*/
void test_Run_ZeroSize_ReturnsNotOk(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 0U, 5000U, FALSE);
    RamTst_Init(&test_Config);

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_Run());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
}

void test_Run_InvalidAlgorithm_ReturnsNotOk(void)
{
    test_SetupConfig((RamTst_AlgType)0x99U, 4U, 5000U, FALSE);
    RamTst_Init(&test_Config);

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_Run());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
}

void test_Run_WhileRunning_ReportsBusy(void)
{
    /* GALPAT on a 2-word region needs many cycles - stays RUNNING */
    test_SetupConfig(RAMTST_ALGORITHM_GALPAT, 8U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_Run());
    TEST_ASSERT_EQUAL_UINT32(1U, Det_Mock_GetCallCount());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_RUNNING, RamTst_GetTestStatus());
}

/*===========================================================================
 * March-C
 *==========================================================================*/
void test_MarchC_CompletesOk(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    (void)test_RunMain(MARCH_C_1WORD_CALLS + 4U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_OK, RamTst_GetTestResult());
    /* no errors recorded */
    {
        RamTst_ErrorRecordType rec;
        TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_GetErrorRecord(&rec));
    }
}

void test_MarchC_13N_SharesMarchCExecution(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_13N, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    (void)test_RunMain(MARCH_C_1WORD_CALLS + 4U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_OK, RamTst_GetTestResult());
}

void test_MarchC_CorruptedCell_RecordsError(void)
{
    RamTst_ErrorRecordType rec;
    uint32 i;

    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    /* Corrupt the single cell right before the final M5 verify phase
     * (call 6 performs the M5 read). */
    for (i = 0U; i < MARCH_C_1WORD_CALLS; i++) {
        if (i == MARCH_C_M5_INDEX) {
            MockRegisters_Write32(TEST_REGION_BASE, 0xDEADBEEFUL);
        }
        if (RamTst_GetTestStatus() != RAMTST_STATUS_RUNNING) {
            break;
        }
        RamTst_MainFunction();
    }

    /* Final transition call completes the test */
    (void)test_RunMain(4U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_FAILED, RamTst_GetTestResult());
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_GetErrorRecord(&rec));
    TEST_ASSERT_EQUAL_UINT32(TEST_REGION_BASE, rec.FailedAddress);
    TEST_ASSERT_EQUAL_UINT32(0x00000000UL, rec.ExpectedValue);
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEFUL, rec.ActualValue);
    TEST_ASSERT_EQUAL_UINT8(5U, rec.AlgorithmStep);
}

void test_MarchC_StopOnError_AbortsImmediately(void)
{
    uint32 i;

    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, TRUE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    for (i = 0U; i < MARCH_C_1WORD_CALLS; i++) {
        if (i == MARCH_C_M5_INDEX) {
            MockRegisters_Write32(TEST_REGION_BASE, 0x12345678UL);
        }
        if (RamTst_GetTestStatus() != RAMTST_STATUS_RUNNING) {
            break;
        }
        RamTst_MainFunction();
    }

    /* With StopOnError the M5 read returns immediately without advancing,
     * so the verify re-reads the corrupted cell on every further call and
     * the test can never reach the completed step. */
    (void)test_RunMain(8U);
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_FAILED, RamTst_GetTestResult());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_RUNNING, RamTst_GetTestStatus());

    /* Stop recovers the module to IDLE with an aborted result */
    RamTst_Stop();
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_ABORTED, RamTst_GetTestResult());
}

void test_MarchC_Timeout_ResultsTimeout(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 2U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    /* TickCount increments per MainFunction; TickCount > 2 triggers timeout */
    (void)test_RunMain(20U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_TIMEOUT, RamTst_GetTestResult());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
}

/*===========================================================================
 * Stop / idle behaviour
 *==========================================================================*/
void test_Stop_NotRunning_IsNoOp(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);

    RamTst_Stop();

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}

void test_Stop_AbortsRunningTest(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());
    RamTst_MainFunction();

    RamTst_Stop();

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_ABORTED, RamTst_GetTestResult());
}

void test_MainFunction_NotRunning_IsNoOp(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);

    RamTst_MainFunction();
    RamTst_MainFunction();

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}

/*===========================================================================
 * Checkerboard
 *==========================================================================*/
void test_Checkerboard_CompletesOk(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_CHECKERBOARD, 16U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    (void)test_RunMain(40U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_OK, RamTst_GetTestResult());
}

void test_Checkerboard_CorruptedOddCell_RecordsError(void)
{
    RamTst_ErrorRecordType rec;
    uint32 i;

    test_SetupConfig(RAMTST_ALGORITHM_CHECKERBOARD, 16U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    /* 4-word region: calls 1-3 write even cells, 4-6 write odd cells,
     * 7-9 verify even, 10-12 verify odd. Corrupt an odd cell after the
     * write phase (call 6) so the odd verify phase detects it. */
    for (i = 0U; i < 40U; i++) {
        if (i == 6U) {
            MockRegisters_Write32(TEST_REGION_BASE + 4U, 0x55555554UL);
        }
        if (RamTst_GetTestStatus() == RAMTST_STATUS_COMPLETED) {
            break;
        }
        RamTst_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_FAILED, RamTst_GetTestResult());
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_GetErrorRecord(&rec));
    TEST_ASSERT_EQUAL_UINT32(TEST_REGION_BASE + 4U, rec.FailedAddress);
    TEST_ASSERT_EQUAL_UINT32(0x55555555UL, rec.ExpectedValue);
    TEST_ASSERT_EQUAL_UINT32(0x55555554UL, rec.ActualValue);
}

/*===========================================================================
 * GALPAT
 *==========================================================================*/
void test_Galpat_CompletesOk(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_GALPAT, 8U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    (void)test_RunMain(400U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_OK, RamTst_GetTestResult());
}

void test_Galpat_CorruptedCell_RecordsError(void)
{
    RamTst_ErrorRecordType rec;
    uint32 i;

    test_SetupConfig(RAMTST_ALGORITHM_GALPAT, 8U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    /* Per-bit cycle is 9 calls (see GALPAT_CYCLE_CALLS). Call 7 (index 6)
     * is the transition into the verify step - corrupt the first cell just
     * before it so the walking-1 verify reads a mismatching value. */
    for (i = 0U; i < 400U; i++) {
        if ((i % GALPAT_CYCLE_CALLS) == 6U) {
            uint32 bit = i / GALPAT_CYCLE_CALLS;
            MockRegisters_Write32(TEST_REGION_BASE, (1UL << bit) ^ 1UL);
        }
        if (RamTst_GetTestStatus() == RAMTST_STATUS_COMPLETED) {
            break;
        }
        RamTst_MainFunction();
    }

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_FAILED, RamTst_GetTestResult());
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_GetErrorRecord(&rec));
    TEST_ASSERT_EQUAL_UINT32(TEST_REGION_BASE, rec.FailedAddress);
    TEST_ASSERT_EQUAL_UINT8(2U, rec.AlgorithmStep);
}

/*===========================================================================
 * Walkpath
 *==========================================================================*/
void test_Walkpath_CompletesOk(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_WALKPATH, 8U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());

    (void)test_RunMain(20U);

    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_COMPLETED, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_OK, RamTst_GetTestResult());
}

/*===========================================================================
 * Error record API
 *==========================================================================*/
void test_GetErrorRecord_NullPtr_ReportsDet(void)
{
    RamTst_ErrorRecordType rec;

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_GetErrorRecord(&rec));
    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_GetErrorRecord(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(1U, Det_Mock_GetCallCount());
}

void test_GetErrorRecord_NoErrors_ReturnsNotOk(void)
{
    RamTst_ErrorRecordType rec;

    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());
    (void)test_RunMain(MARCH_C_1WORD_CALLS + 4U);

    TEST_ASSERT_EQUAL_UINT32(E_NOT_OK, RamTst_GetErrorRecord(&rec));
}

void test_ReInit_RestartsCleanly(void)
{
    test_SetupConfig(RAMTST_ALGORITHM_MARCH_C, 4U, 0U, FALSE);
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(E_OK, RamTst_Run());
    (void)test_RunMain(MARCH_C_1WORD_CALLS + 4U);
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_OK, RamTst_GetTestResult());

    /* Re-init must clear the previous result */
    RamTst_Init(&test_Config);
    TEST_ASSERT_EQUAL_UINT32(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL_UINT32(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}
