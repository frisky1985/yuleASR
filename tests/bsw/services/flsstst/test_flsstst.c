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

    return UnityEnd();
}
