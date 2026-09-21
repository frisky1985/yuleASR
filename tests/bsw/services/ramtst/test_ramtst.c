/**
 * @file test_ramtst.c
 * @brief Unit tests for the RamTst module (RAM Test, March C algorithm).
 *
 * The SUT executes a real March C test over real RAM addressed via raw uint32
 * addresses. On this 64-bit host every pointer lives above 4 GB, so the
 * truncated uint32 address would segfault the moment RamTst_MainFunction
 * walks a region - and RamTst_RunTest rejects regions smaller than 4 bytes,
 * so no accepted region can ever be walked safely here. The tests therefore
 * cover all validation, state-machine, callback and Det behavior up to (but
 * not including) the raw-memory walk, plus the too-small-region validation.
 */
// @tests src/bsw/services/ramtst/src/RamTst.c  @tests src/bsw/services/ramtst/include/RamTst.h
#include "unity.h"
#include "RamTst.h"

/* ------------------------------------------------------------------------ */
/* Det mock                                                                 */
/* ------------------------------------------------------------------------ */

static uint8 mock_DetErrorCount;
static uint8 mock_DetLastApiId;
static uint8 mock_DetLastErrorId;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    mock_DetErrorCount++;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    return E_OK;
}

/* ------------------------------------------------------------------------ */
/* Completion callback mock                                                 */
/* ------------------------------------------------------------------------ */

static uint8 mock_CompletionCallCount;
static RamTst_ResultType mock_CompletionLastResult;

static void mock_CompletionCallback(RamTst_ResultType Result)
{
    mock_CompletionCallCount++;
    mock_CompletionLastResult = Result;
}

/* ------------------------------------------------------------------------ */
/* Test fixture                                                             */
/* ------------------------------------------------------------------------ */

/* Real RAM region: RAMTST_WORDS_PER_CYCLE (8) 32-bit words, so each March
 * step finishes in exactly one RamTst_MainFunction call. Non-const: the
 * StartAddr field is populated at runtime in setUp() (a pointer cast is not
 * a C99 compile-time constant). */
static uint32 testRamBuffer[RAMTST_WORDS_PER_CYCLE];

static RamTst_RegionType testRegion;

static const RamTst_ConfigType testConfig = {
    1U,                      /* NumRegions */
    &testRegion,             /* Regions */
    RAMTST_ALGO_MARCH_C,     /* Algorithm */
    FALSE,                   /* RunOnStartup */
    &mock_CompletionCallback,/* CompletionCb */
    TRUE,                    /* DevErrorDetect */
    TRUE                     /* VersionInfoApi */
};

/* Tracks module state because RamTst has no public GetState API. */
static boolean testModuleInitialized;

void setUp(void)
{
    mock_DetErrorCount = 0U;
    mock_DetLastApiId = 0U;
    mock_DetLastErrorId = 0U;
    mock_CompletionCallCount = 0U;
    mock_CompletionLastResult = RAMTST_RESULT_NOT_RUN;

    testRegion.StartAddr = (uint32)testRamBuffer;
    testRegion.Size = (uint32)sizeof(testRamBuffer);
    testRegion.RegionId = 0U;

    if (testModuleInitialized)
    {
        RamTst_DeInit();
        testModuleInitialized = FALSE;
    }
}

void tearDown(void)
{
}

/* ------------------------------------------------------------------------ */
/* Initialization tests                                                     */
/* ------------------------------------------------------------------------ */

/**
 * @req SWS_RamTst_00001
 * RamTst_Init has a void return; the failure is observable through the Det
 * report and through the module remaining uninitialized (GetResult still
 * reports the reset value).
 */
void test_RamTst_Init_NullConfig_ReportsDet(void)
{
    RamTst_ResultType result = RAMTST_RESULT_FAILED;
    RamTst_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_PARAM_POINTER, mock_DetLastErrorId);
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_GetResult(&result));
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_NOT_RUN, result);
}

/** @req SWS_RamTst_00001 */
void test_RamTst_Init_ValidConfig_InitializesModule(void)
{
    RamTst_ResultType result = RAMTST_RESULT_FAILED;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
    /* Freshly initialized module reports NOT_RUN */
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_GetResult(&result));
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_NOT_RUN, result);
}

/** @req SWS_RamTst_00001 */
void test_RamTst_Init_DoubleInit_ReportsDet(void)
{
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    RamTst_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

/** @req SWS_RamTst_00002 */
void test_RamTst_DeInit_BeforeInit_ReportsDet(void)
{
    RamTst_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_RamTst_00002 */
void test_RamTst_DeInit_AfterInit_SucceedsSilently(void)
{
    RamTst_Init(&testConfig);
    RamTst_DeInit();
    testModuleInitialized = FALSE;
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
    /* A second DeInit now has to hit the UNINIT guard again */
    RamTst_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_UNINIT, mock_DetLastErrorId);
}

/* ------------------------------------------------------------------------ */
/* Version info tests                                                       */
/* ------------------------------------------------------------------------ */

#if (RAMTST_VERSION_INFO_API == STD_ON)
/** @req SWS_RamTst_00003 */
void test_RamTst_GetVersionInfo_NullPtr_ReportsDet(void)
{
    RamTst_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_RamTst_00003 */
void test_RamTst_GetVersionInfo_ValidPtr_ReturnsExpectedValues(void)
{
    Std_VersionInfoType versionInfo;
    RamTst_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL_UINT16(RAMTST_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(RAMTST_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}
#endif

/* ------------------------------------------------------------------------ */
/* RunTest / March C execution tests                                        */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamTst_00004 */
void test_RamTst_RunTest_BeforeInit_ReturnsNotOk(void)
{
    Std_ReturnType result = RamTst_RunTest(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_RUNTEST, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_RamTst_00004 */
void test_RamTst_RunTest_UnknownRegion_ReturnsNotOkWithoutDet(void)
{
    Std_ReturnType result;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    result = RamTst_RunTest(99U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
}

/** @req SWS_RamTst_00004 */
void test_RamTst_RunTest_ValidRegion_ReturnsOk(void)
{
    Std_ReturnType result;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    result = RamTst_RunTest(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
}

/** @req SWS_RamTst_00004 */
void test_RamTst_RunTest_WhileBusy_ReturnsNotOk(void)
{
    Std_ReturnType result;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    (void)RamTst_RunTest(0U);
    result = RamTst_RunTest(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
}

/**
 * @req SWS_RamTst_00004
 * Regions smaller than one 32-bit word are rejected by RunTest with
 * E_NOT_OK (and without a Det report). On this 64-bit host this validation
 * is also what keeps every later raw-memory walk unreachable in tests -
 * see the file header.
 */
void test_RamTst_RunTest_RegionTooSmall_ReturnsNotOk(void)
{
    Std_ReturnType result;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    testRegion.Size = 2U;   /* below the 4-byte minimum */
    result = RamTst_RunTest(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
    /* A second RunTest must hit the busy guard, proving the first call
     * did not start a test either */
    testRegion.Size = (uint32)sizeof(testRamBuffer);
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_RunTest(0U));
}

/**
 * @req SWS_RamTst_00005
 * @req SWS_RamTst_00007
 * While a test is running (BUSY), GetResult must still report NOT_RUN and
 * the completion callback must not fire - completion is deferred to the
 * MainFunction calls that drive the walk (not executable on this host, see
 * header; the test is cleaned up via Abort which is covered separately).
 */
void test_RamTst_GetResult_WhileBusy_ReturnsNotRun(void)
{
    RamTst_ResultType result = RAMTST_RESULT_FAILED;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_RunTest(0U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_GetResult(&result));
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_NOT_RUN, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_CompletionCallCount);
}

/* ------------------------------------------------------------------------ */
/* Abort tests                                                              */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamTst_00006 */
void test_RamTst_Abort_BeforeInit_ReturnsNotOk(void)
{
    Std_ReturnType result = RamTst_Abort();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_ABORT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_RamTst_00006 */
void test_RamTst_Abort_WhileBusy_CompletesAsAborted(void)
{
    RamTst_ResultType result = RAMTST_RESULT_NOT_RUN;
    Std_ReturnType ret;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    (void)RamTst_RunTest(0U);
    ret = RamTst_Abort();
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_GetResult(&result));
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_ABORTED, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_CompletionCallCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_ABORTED, mock_CompletionLastResult);
}

/** @req SWS_RamTst_00006 */
void test_RamTst_Abort_WithoutActiveTest_SucceedsSilently(void)
{
    Std_ReturnType result;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    result = RamTst_Abort();
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_CompletionCallCount);
}

/* ------------------------------------------------------------------------ */
/* GetResult / MainFunction tests                                           */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamTst_00005 */
void test_RamTst_GetResult_NullPtr_ReturnsNotOk(void)
{
    Std_ReturnType result;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    result = RamTst_GetResult(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_SID_GETRESULT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(RAMTST_E_PARAM_POINTER, mock_DetLastErrorId);
}

/**
 * @req SWS_RamTst_00005
 * RamTst_GetResult performs no init-state check; before init it returns
 * E_OK with the reset result value.
 */
void test_RamTst_GetResult_BeforeInit_ReturnsResetValue(void)
{
    RamTst_ResultType result = RAMTST_RESULT_FAILED;
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_GetResult(&result));
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_NOT_RUN, result);
}

/**
 * @req SWS_RamTst_00007
 * With no test running (state INIT), MainFunction must be a silent no-op:
 * no completion callback and no result change.
 */
void test_RamTst_MainFunction_WithoutActiveTest_IsSilentNoOp(void)
{
    RamTst_ResultType result = RAMTST_RESULT_NOT_RUN;
    RamTst_Init(&testConfig);
    testModuleInitialized = TRUE;
    RamTst_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_CompletionCallCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamTst_GetResult(&result));
    TEST_ASSERT_EQUAL_UINT8(RAMTST_RESULT_NOT_RUN, result);
}

/* ------------------------------------------------------------------------ */
/* Test runner                                                              */
/* ------------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_RamTst_Init_NullConfig_ReportsDet);
    RUN_TEST(test_RamTst_Init_ValidConfig_InitializesModule);
    RUN_TEST(test_RamTst_Init_DoubleInit_ReportsDet);
    RUN_TEST(test_RamTst_DeInit_BeforeInit_ReportsDet);
    RUN_TEST(test_RamTst_DeInit_AfterInit_SucceedsSilently);
#if (RAMTST_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_RamTst_GetVersionInfo_NullPtr_ReportsDet);
    RUN_TEST(test_RamTst_GetVersionInfo_ValidPtr_ReturnsExpectedValues);
#endif
    RUN_TEST(test_RamTst_RunTest_BeforeInit_ReturnsNotOk);
    RUN_TEST(test_RamTst_RunTest_UnknownRegion_ReturnsNotOkWithoutDet);
    RUN_TEST(test_RamTst_RunTest_ValidRegion_ReturnsOk);
    RUN_TEST(test_RamTst_RunTest_WhileBusy_ReturnsNotOk);
    RUN_TEST(test_RamTst_RunTest_RegionTooSmall_ReturnsNotOk);
    RUN_TEST(test_RamTst_GetResult_WhileBusy_ReturnsNotRun);
    RUN_TEST(test_RamTst_Abort_BeforeInit_ReturnsNotOk);
    RUN_TEST(test_RamTst_Abort_WhileBusy_CompletesAsAborted);
    RUN_TEST(test_RamTst_Abort_WithoutActiveTest_SucceedsSilently);
    RUN_TEST(test_RamTst_GetResult_NullPtr_ReturnsNotOk);
    RUN_TEST(test_RamTst_GetResult_BeforeInit_ReturnsResetValue);
    RUN_TEST(test_RamTst_MainFunction_WithoutActiveTest_IsSilentNoOp);
    return UnityEnd();
}
