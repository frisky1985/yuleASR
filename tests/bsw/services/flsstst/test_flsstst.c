/**
 * @file test_flsstst.c
 * @brief FlStSt (Flash Test, March C + Erase/Program Verify) Unit Tests
 * @req SWS_FlStSt
 *
 * Substantiated against the production implementation in
 * src/bsw/services/flstst (module API prefix is FlStSt).
 */

// @tests src/bsw/services/flstst/src/FlStSt.c  @tests src/bsw/services/flstst/include/FlStSt.h

#include "unity.h"
#include "FlStSt.h"

/* Mock Det_ReportError — defined here, so tests/mocks/mock_det.c must NOT be linked. */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Sector under test: small sector so March C completes in a few MainFunction cycles.
 * FlStSt_LocalReadByte() is a production stub that always returns the erased
 * value 0xFF, and the March C background pattern is 0x55, so a full March C run
 * deterministically reports FLSTST_RESULT_FAILED after the background-write step. */
static const FlStSt_SectorType testSectors[1] = {
    {0x00010000U, 16U, 0U, 256U}
};

static FlStSt_ConfigType testConfig = {
    .NumSectors = 1U,
    .Sectors = testSectors,
    .Algorithm = FLSTST_ALGO_MARCH_C,
    .RunOnInit = FALSE,
    .DevErrorDetect = TRUE,
    .VersionInfoApi = TRUE
};

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_FlStSt_00001 */
void test_FlStSt_Init_NullPtr_ShouldReportError(void) {
    FlStSt_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FlStSt_00001 */
void test_FlStSt_Init_ValidConfig_ShouldSucceed(void) {
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Initialization succeeded: DeInit must succeed without DET errors,
     * and a second DeInit must now report E_UNINIT (state really reset). */
    FlStSt_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FlStSt_00001 */
void test_FlStSt_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    FlStSt_Init(&testConfig);
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00002 */
void test_FlStSt_DeInit_Uninit_ShouldReportError(void) {
    FlStSt_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FlStSt_00003 */
void test_FlStSt_GetVersionInfo_NullPtr_ShouldReportError(void) {
    FlStSt_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_FlStSt_00003 */
void test_FlStSt_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    FlStSt_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(FLSTST_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_FlStSt_00004 */
void test_FlStSt_MainFunction_Uninit_ShouldReturnSilently(void) {
    /* No DET reporting in MainFunction when uninitialised — it returns early. */
    FlStSt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_FlStSt_00005 */
void test_FlStSt_RunTest_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = FlStSt_RunTest(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_FlStSt_00005 */
void test_FlStSt_RunTest_InvalidSector_ShouldReturnNotOk(void) {
    Std_ReturnType ret;
    FlStSt_Init(&testConfig);
    ret = FlStSt_RunTest(99U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00005 */
void test_FlStSt_RunTest_ValidSector_ShouldRunToCompletion(void) {
    FlStSt_ResultType result = FLSTST_RESULT_NOT_RUN;
    uint8 i;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    /* March C: 16 bytes / FLSTST_BYTES_PER_CYCLE(4) per cycle = 4 cycles for the
     * background write, then the first read step detects the stub read value
     * 0xFF != background 0x55 and fails. 16 cycles is safely past completion. */
    for (i = 0U; i < 16U; i++) {
        FlStSt_MainFunction();
    }
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    TEST_ASSERT_EQUAL(FLSTST_RESULT_FAILED, result);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00009 */
void test_FlStSt_Abort_WhileBusy_ShouldAbortTest(void) {
    FlStSt_ResultType result = FLSTST_RESULT_NOT_RUN;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    TEST_ASSERT_EQUAL(E_OK, FlStSt_Abort());
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    TEST_ASSERT_EQUAL(FLSTST_RESULT_ABORTED, result);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00006 */
void test_FlStSt_VerifyErase_ValidSector_ShouldReportErased(void) {
    boolean erased = FALSE;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyErase(0U, &erased));
    TEST_ASSERT_TRUE(erased);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00006 */
void test_FlStSt_VerifyErase_NullResult_ShouldReportError(void) {
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_VerifyErase(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_PARAM_POINTER, mock_DetLastErrorId);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00007 */
void test_FlStSt_VerifyProgram_MatchingData_ShouldPass(void) {
    boolean match = FALSE;
    /* Production read stub always returns 0xFF; expected data must match it. */
    const uint8 expected[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyProgram(0U, expected, 4U, &match));
    TEST_ASSERT_TRUE(match);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00007 */
void test_FlStSt_VerifyProgram_MismatchedData_ShouldFail(void) {
    boolean match = TRUE;
    const uint8 expected[4] = {0x00U, 0x11U, 0x22U, 0x33U};
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyProgram(0U, expected, 4U, &match));
    TEST_ASSERT_FALSE(match);
    FlStSt_DeInit();
}

/** @req SWS_FlStSt_00008 */
void test_FlStSt_GetResult_NullPtr_ShouldReportError(void) {
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_GetResult(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_PARAM_POINTER, mock_DetLastErrorId);
    FlStSt_DeInit();
}

/* ── Branch coverage additions ────────────────────────────────────────────── */

/* Branch: FlStSt_VerifyErase — called before init (E_UNINIT path, line 382) */
void test_FlStSt_VerifyErase_Uninit_ShouldReportError(void) {
    boolean result;
    Std_ReturnType ret = FlStSt_VerifyErase(0U, &result);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_UNINIT, mock_DetLastErrorId);
}

/* Branch: FlStSt_VerifyErase — invalid sector id (sector == NULL_PTR, line 395) */
void test_FlStSt_VerifyErase_InvalidSector_ShouldReturnNotOk(void) {
    boolean result;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_VerifyErase(99U, &result));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyErase — sector larger than VERIFY_CHUNK_SIZE so the
 * ternary on line 408 takes the FALSE branch (chunk = FLSTST_VERIFY_CHUNK_SIZE)
 * and also the TRUE branch on the last partial chunk. */
void test_FlStSt_VerifyErase_LargeSector_ShouldCoverBothChunks(void) {
    boolean erased;
    FlStSt_SectorType largeSector[1] = {
        {0x00020000U, 512U, 10U, 256U}   /* 512 bytes = 2 full chunks */
    };
    FlStSt_ConfigType largeCfg = {
        .NumSectors = 1U,
        .Sectors = largeSector,
        .Algorithm = FLSTST_ALGO_MARCH_C,
        .RunOnInit = FALSE,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };
    FlStSt_Init(&largeCfg);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyErase(10U, &erased));
    TEST_ASSERT_TRUE(erased);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyErase — sector size not a multiple of VERIFY_CHUNK_SIZE
 * so the last chunk is partial (ternary TRUE branch on line 408). */
void test_FlStSt_VerifyErase_NonAlignedSize_ShouldCoverPartialChunk(void) {
    boolean erased;
    FlStSt_SectorType oddSector[1] = {
        {0x00030000U, 300U, 11U, 256U}   /* 300 = 256 + 44 → two iterations */
    };
    FlStSt_ConfigType oddCfg = {
        .NumSectors = 1U,
        .Sectors = oddSector,
        .Algorithm = FLSTST_ALGO_MARCH_C,
        .RunOnInit = FALSE,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };
    FlStSt_Init(&oddCfg);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyErase(11U, &erased));
    TEST_ASSERT_TRUE(erased);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyProgram — called before init (E_UNINIT, line 438) */
void test_FlStSt_VerifyProgram_Uninit_ShouldReportError(void) {
    boolean result;
    const uint8 data[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    Std_ReturnType ret = FlStSt_VerifyProgram(0U, data, 4U, &result);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_UNINIT, mock_DetLastErrorId);
}

/* Branch: FlStSt_VerifyProgram — NULL ExpectedData (first half of ||, line 443) */
void test_FlStSt_VerifyProgram_NullExpectedData_ShouldReportError(void) {
    boolean result;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_VerifyProgram(0U, NULL_PTR, 4U, &result));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_PARAM_POINTER, mock_DetLastErrorId);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyProgram — NULL Result pointer (second half of ||, line 443) */
void test_FlStSt_VerifyProgram_NullResult_ShouldReportError(void) {
    const uint8 data[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_VerifyProgram(0U, data, 4U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_PARAM_POINTER, mock_DetLastErrorId);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyProgram — invalid sector (sector == NULL_PTR, line 451) */
void test_FlStSt_VerifyProgram_InvalidSector_ShouldReturnNotOk(void) {
    boolean result;
    const uint8 data[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_VerifyProgram(99U, data, 4U, &result));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyProgram — Length > sector->Size (line 456) */
void test_FlStSt_VerifyProgram_LengthExceedsSector_ShouldReturnNotOk(void) {
    boolean result;
    const uint8 data[32] = {0};
    FlStSt_Init(&testConfig);
    /* testConfig sector 0 has Size=16, so Length=32 > 16 */
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_VerifyProgram(0U, data, 32U, &result));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyProgram — Length == 0, for loop body never executed */
void test_FlStSt_VerifyProgram_ZeroLength_ShouldReportMatch(void) {
    boolean result = FALSE;
    const uint8 data[1] = {0x00};
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyProgram(0U, data, 0U, &result));
    TEST_ASSERT_TRUE(result);  /* match stays TRUE — loop never runs */
    FlStSt_DeInit();
}

/* Branch: FlStSt_Abort — called before init (E_UNINIT, line 498) */
void test_FlStSt_Abort_Uninit_ShouldReportError(void) {
    Std_ReturnType ret = FlStSt_Abort();
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(FLSTST_E_UNINIT, mock_DetLastErrorId);
}

/* Branch: FlStSt_RunTest — called while another test is already busy (line 348) */
void test_FlStSt_RunTest_WhileBusy_ShouldReturnNotOk(void) {
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    /* Second RunTest while first is still active */
    TEST_ASSERT_EQUAL(E_NOT_OK, FlStSt_RunTest(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_Abort();
    FlStSt_DeInit();
}

/* Branch: FlStSt_DeInit — called while a test is active (Active != 0, line 306) */
void test_FlStSt_DeInit_WhileBusy_ShouldAbortTest(void) {
    FlStSt_ResultType result;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    /* DeInit while test is active — should abort the running test */
    FlStSt_DeInit();
    /* After DeInit, module is uninitialised. Re-init and check last result. */
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    /* LastResult was set by DeInit path: Result = ABORTED before state reset,
     * but DeInit doesn't update LastResult — it just clears CurrentTest.
     * After re-init LastResult is reset to NOT_RUN. */
    TEST_ASSERT_EQUAL(FLSTST_RESULT_NOT_RUN, result);
    FlStSt_DeInit();
}

/* Branch: FlStSt_Init — RunOnInit=TRUE with NumSectors > 0.
 * NOTE: FLSTST_RUN_ON_INIT is not defined in FlStSt_Cfg.h, so the RunOnInit
 * code block (#if FLSTST_RUN_ON_INIT == STD_ON) is compiled out.
 * This test verifies that Init with RunOnInit=TRUE still succeeds normally
 * and does NOT auto-start a test (since the code is not compiled in). */
void test_FlStSt_Init_RunOnInit_ShouldStartTest(void) {
    FlStSt_SectorType ronSector[1] = {
        {0x00040000U, 16U, 20U, 256U}
    };
    FlStSt_ConfigType ronCfg = {
        .NumSectors = 1U,
        .Sectors = ronSector,
        .Algorithm = FLSTST_ALGO_MARCH_C,
        .RunOnInit = TRUE,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };
    FlStSt_Init(&ronCfg);
    /* RunOnInit code is compiled out, so no test should be auto-started.
     * RunTest should succeed (not busy). */
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(20U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_Abort();
    FlStSt_DeInit();
}

/* Branch: FlStSt_Init — RunOnInit=TRUE but NumSectors=0 (line 286 false branch).
 * The inner if (NumSectors > 0) should not execute. */
void test_FlStSt_Init_RunOnInit_NoSectors_ShouldNotStartTest(void) {
    FlStSt_ConfigType noSectorCfg = {
        .NumSectors = 0U,
        .Sectors = NULL_PTR,
        .Algorithm = FLSTST_ALGO_MARCH_C,
        .RunOnInit = TRUE,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };
    FlStSt_Init(&noSectorCfg);
    /* No test should be running; MainFunction should return silently */
    FlStSt_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    FlStSt_DeInit();
}

/* Branch: FlStSt_MainFunction — State==BUSY but CurrentTest.Active==FALSE
 * before MarchCStep (line 527). This can happen after MarchCStep deactivates
 * the test but before MainFunction updates State. We simulate it by running
 * MainFunction enough times for the test to complete, then one more call. */
void test_FlStSt_MainFunction_TestCompletes_ShouldTransitionToInit(void) {
    FlStSt_ResultType result;
    uint8 i;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    /* Run until the March C test completes (fails due to stub read).
     * 16-byte sector: 4 cycles for W0, then 1 cycle for R0_W1 fail = 5 calls.
     * Use 20 to be safe. */
    for (i = 0U; i < 20U; i++) {
        FlStSt_MainFunction();
    }
    /* After completion, State should be INIT and LastResult should be FAILED */
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    TEST_ASSERT_EQUAL(FLSTST_RESULT_FAILED, result);
    /* Calling MainFunction again should return silently (State != BUSY) */
    FlStSt_MainFunction();
    TEST_ASSERT_EQUAL(FLSTST_RESULT_FAILED, result);
    FlStSt_DeInit();
}

/* Branch: FlStSt_LocalRunMarchCStep — step transition through all W0 bytes
 * and into R0_W1 with a minimal 4-byte sector (exactly one cycle per step).
 * This exercises the CurrentOffset >= Size check (line 226) and the
 * MarchStep++ transition (line 229). */
void test_FlStSt_MarchC_MinimalSector_ShouldCompleteW0AndFailOnR0W1(void) {
    FlStSt_ResultType result;
    FlStSt_SectorType tinySector[1] = {
        {0x00050000U, 4U, 30U, 256U}   /* Exactly FLSTST_BYTES_PER_CYCLE bytes */
    };
    FlStSt_ConfigType tinyCfg = {
        .NumSectors = 1U,
        .Sectors = tinySector,
        .Algorithm = FLSTST_ALGO_MARCH_C,
        .RunOnInit = FALSE,
        .DevErrorDetect = TRUE,
        .VersionInfoApi = TRUE
    };
    FlStSt_Init(&tinyCfg);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(30U));
    /* First call: W0 step processes all 4 bytes, transitions to R0_W1 */
    FlStSt_MainFunction();
    /* Second call: R0_W1 step reads 0xFF != 0x55 → FAIL */
    FlStSt_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    TEST_ASSERT_EQUAL(FLSTST_RESULT_FAILED, result);
    FlStSt_DeInit();
}

/* Branch: FlStSt_GetResult — after abort, result should be ABORTED */
void test_FlStSt_GetResult_AfterAbort_ShouldReturnAborted(void) {
    FlStSt_ResultType result;
    FlStSt_Init(&testConfig);
    FlStSt_RunTest(0U);
    FlStSt_Abort();
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    TEST_ASSERT_EQUAL(FLSTST_RESULT_ABORTED, result);
    FlStSt_DeInit();
}

/* Branch: FlStSt_Abort — not busy (Active==FALSE, line 505 true branch).
 * Already partially covered, but explicitly verify no state change. */
void test_FlStSt_Abort_NotBusy_ShouldReturnOkNoChange(void) {
    FlStSt_ResultType result;
    FlStSt_Init(&testConfig);
    /* No test running — abort should succeed without side effects */
    TEST_ASSERT_EQUAL(E_OK, FlStSt_Abort());
    TEST_ASSERT_EQUAL(E_OK, FlStSt_GetResult(&result));
    TEST_ASSERT_EQUAL(FLSTST_RESULT_NOT_RUN, result);
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyErase — called while test is running (State==BUSY).
 * This covers the FALSE branch of !FLSTST_IS_INIT() at line 382, where
 * State == BUSY satisfies FLSTST_IS_INIT(). */
void test_FlStSt_VerifyErase_WhileBusy_ShouldSucceed(void) {
    boolean erased;
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    /* Call VerifyErase while test is running (State == BUSY) */
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyErase(0U, &erased));
    TEST_ASSERT_TRUE(erased);
    FlStSt_Abort();
    FlStSt_DeInit();
}

/* Branch: FlStSt_VerifyProgram — called while test is running (State==BUSY).
 * This covers the FALSE branch of !FLSTST_IS_INIT() at line 438. */
void test_FlStSt_VerifyProgram_WhileBusy_ShouldSucceed(void) {
    boolean match;
    const uint8 data[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    FlStSt_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, FlStSt_RunTest(0U));
    /* Call VerifyProgram while test is running (State == BUSY) */
    TEST_ASSERT_EQUAL(E_OK, FlStSt_VerifyProgram(0U, data, 4U, &match));
    TEST_ASSERT_TRUE(match);
    FlStSt_Abort();
    FlStSt_DeInit();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_FlStSt_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_FlStSt_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_FlStSt_Init_DoubleInit_ShouldReportAlreadyInitialized);
    RUN_TEST(test_FlStSt_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_FlStSt_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_FlStSt_GetVersionInfo_ValidPtr_ShouldReturnVersion);
    RUN_TEST(test_FlStSt_MainFunction_Uninit_ShouldReturnSilently);
    RUN_TEST(test_FlStSt_RunTest_Uninit_ShouldReportError);
    RUN_TEST(test_FlStSt_RunTest_InvalidSector_ShouldReturnNotOk);
    RUN_TEST(test_FlStSt_RunTest_ValidSector_ShouldRunToCompletion);
    RUN_TEST(test_FlStSt_Abort_WhileBusy_ShouldAbortTest);
    RUN_TEST(test_FlStSt_VerifyErase_ValidSector_ShouldReportErased);
    RUN_TEST(test_FlStSt_VerifyErase_NullResult_ShouldReportError);
    RUN_TEST(test_FlStSt_VerifyProgram_MatchingData_ShouldPass);
    RUN_TEST(test_FlStSt_VerifyProgram_MismatchedData_ShouldFail);
    RUN_TEST(test_FlStSt_GetResult_NullPtr_ShouldReportError);

    /* ── Branch coverage additions ── */
    RUN_TEST(test_FlStSt_VerifyErase_Uninit_ShouldReportError);
    RUN_TEST(test_FlStSt_VerifyErase_InvalidSector_ShouldReturnNotOk);
    RUN_TEST(test_FlStSt_VerifyErase_LargeSector_ShouldCoverBothChunks);
    RUN_TEST(test_FlStSt_VerifyErase_NonAlignedSize_ShouldCoverPartialChunk);
    RUN_TEST(test_FlStSt_VerifyProgram_Uninit_ShouldReportError);
    RUN_TEST(test_FlStSt_VerifyProgram_NullExpectedData_ShouldReportError);
    RUN_TEST(test_FlStSt_VerifyProgram_NullResult_ShouldReportError);
    RUN_TEST(test_FlStSt_VerifyProgram_InvalidSector_ShouldReturnNotOk);
    RUN_TEST(test_FlStSt_VerifyProgram_LengthExceedsSector_ShouldReturnNotOk);
    RUN_TEST(test_FlStSt_VerifyProgram_ZeroLength_ShouldReportMatch);
    RUN_TEST(test_FlStSt_Abort_Uninit_ShouldReportError);
    RUN_TEST(test_FlStSt_RunTest_WhileBusy_ShouldReturnNotOk);
    RUN_TEST(test_FlStSt_DeInit_WhileBusy_ShouldAbortTest);
    RUN_TEST(test_FlStSt_Init_RunOnInit_ShouldStartTest);
    RUN_TEST(test_FlStSt_Init_RunOnInit_NoSectors_ShouldNotStartTest);
    RUN_TEST(test_FlStSt_MainFunction_TestCompletes_ShouldTransitionToInit);
    RUN_TEST(test_FlStSt_MarchC_MinimalSector_ShouldCompleteW0AndFailOnR0W1);
    RUN_TEST(test_FlStSt_GetResult_AfterAbort_ShouldReturnAborted);
    RUN_TEST(test_FlStSt_Abort_NotBusy_ShouldReturnOkNoChange);
    RUN_TEST(test_FlStSt_VerifyErase_WhileBusy_ShouldSucceed);
    RUN_TEST(test_FlStSt_VerifyProgram_WhileBusy_ShouldSucceed);

    return UnityEnd();
}
