/**
 * @file test_ramsafety.c
 * @brief Unit tests for the RamSafety module (RAM safety runtime driver).
 *
 * These tests exercise the production RamSafety service against mocked
 * platform/Mcal dependencies and a Det mock. Assertions target real return
 * values, module state, Det mock call counts/error codes, and stub call
 * counters.
 *
 * HOST LIMITATION: the SUT addresses RAM through raw uint32 addresses. On this
 * 64-bit host all pointers live above 4 GB, so any SUT code path that
 * dereferences a region address (March C walk, walk-pattern test, VerifyRange
 * with size > 0, startup tests on a region with startupTest == TRUE) would
 * dereference a truncated pointer and segfault. The tests therefore exercise
 * the full state machine and the CRC/ECC/stub paths, and use zero-size ranges
 * or startupTest == FALSE wherever the SUT would otherwise touch memory.
 */
// @tests src/bsw/services/ramsafety/src/RamSafety.c  @tests src/bsw/services/ramsafety/include/RamSafety.h
#include <string.h>

#include "unity.h"
#include "RamSafety.h"
#include "Platform_RamSafety.h"
#include "Mcal.h"

/* ------------------------------------------------------------------------ */
/* Mock framework state                                                     */
/* ------------------------------------------------------------------------ */

static uint8 mock_DetErrorCount;
static uint8 mock_DetLastApiId;
static uint8 mock_DetLastErrorId;

static uint8 mock_EccCheckCount;
static uint32 mock_EccLastAddress;
static uint32 mock_CrcCalcCount;
static uint32 mock_CrcGetCount;
static uint32 mock_EnterSafeStateCount;
static uint32 mock_PlatformDeInitCount;

static uint8 mock_ProgressCallCount;
static uint8 mock_ProgressLastPercent;
static const RamSafety_RegionType* mock_ProgressLastRegion;

#define MOCK_CRC_VALUE 0x12345678U

/* ------------------------------------------------------------------------ */
/* Det mock                                                                 */
/* ------------------------------------------------------------------------ */
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
/* Mcal mocks                                                               */
/* ------------------------------------------------------------------------ */
void Mcal_DisableAllInterrupts(void)
{
}

void Mcal_EnableAllInterrupts(void)
{
}

/* ------------------------------------------------------------------------ */
/* Platform_RamSafety stubs                                                 */
/* ------------------------------------------------------------------------ */
Std_ReturnType Platform_RamSafety_Init(const RamSafety_ConfigType* config)
{
    (void)config;
    return E_OK;
}

Std_ReturnType Platform_RamSafety_DeInit(void)
{
    mock_PlatformDeInitCount++;
    return E_OK;
}

Std_ReturnType Platform_RamSafety_CheckEccStatus(uint32 startAddr, boolean* hasError, uint32* errorCount)
{
    mock_EccCheckCount++;
    mock_EccLastAddress = startAddr;
    if (hasError != NULL_PTR)
    {
        *hasError = FALSE;
    }
    if (errorCount != NULL_PTR)
    {
        *errorCount = 0U;
    }
    return E_OK;
}

uint32 Platform_RamSafety_CalculateCrc(const uint8* data, uint32 length, uint32 seed)
{
    (void)data;
    (void)length;
    (void)seed;
    mock_CrcCalcCount++;
    return MOCK_CRC_VALUE;
}

uint32 Platform_RamSafety_GetStoredCrc(uint8 regionId)
{
    (void)regionId;
    mock_CrcGetCount++;
    return MOCK_CRC_VALUE;
}

void Platform_RamSafety_EnterSafeState(void)
{
    mock_EnterSafeStateCount++;
}

/* ------------------------------------------------------------------------ */
/* Progress callback mock                                                   */
/* ------------------------------------------------------------------------ */
static void mock_ProgressCallback(uint8 percent, const RamSafety_RegionType* region)
{
    mock_ProgressCallCount++;
    mock_ProgressLastPercent = percent;
    mock_ProgressLastRegion = region;
}

/* ------------------------------------------------------------------------ */
/* Test fixture                                                             */
/* ------------------------------------------------------------------------ */

#define RAMSAFETY_TEST_REGION_SIZE (64U)

/* Real writable RAM buffer; uint32 guarantees the 4-byte alignment required
 * by RamSafety_ValidateConfig. */
static uint32 testRegionBufferWords[RAMSAFETY_TEST_REGION_SIZE / 4U];

/* Non-const: the startAddress field is populated at runtime in setUp()
 * (a pointer cast is not a C99 compile-time constant). */
static RamSafety_RegionType testRegions[1U];

static const RamSafety_ConfigType testConfig = {
    testRegions,        /* regions */
    1U,                 /* numRegions */
    1U,                 /* runtimePeriodMs (1 so MainFunction checks immediately) */
    TRUE,               /* useHardwareEcc */
    1U                  /* maxRuntimeRegionsPerCycle */
};

static const RamSafety_ConfigType testConfigInvalid = {
    NULL_PTR,           /* regions */
    0U,                 /* numRegions */
    100U,               /* runtimePeriodMs */
    FALSE,              /* useHardwareEcc */
    1U                  /* maxRuntimeRegionsPerCycle */
};

void setUp(void)
{
    mock_DetErrorCount = 0U;
    mock_DetLastApiId = 0U;
    mock_DetLastErrorId = 0U;

    mock_EccCheckCount = 0U;
    mock_EccLastAddress = 0U;
    mock_CrcCalcCount = 0U;
    mock_CrcGetCount = 0U;
    mock_EnterSafeStateCount = 0U;
    mock_PlatformDeInitCount = 0U;
    mock_ProgressCallCount = 0U;
    mock_ProgressLastPercent = 0U;
    mock_ProgressLastRegion = NULL_PTR;

    testRegions[0U].startAddress = (uint32)testRegionBufferWords;
    testRegions[0U].size = RAMSAFETY_TEST_REGION_SIZE;
    testRegions[0U].priority = 1U;
    /* startupTest stays FALSE: with TRUE the SUT would March-C walk the raw
     * uint32 region address, which is a truncated pointer on this 64-bit host
     * (see file header). The state machine is still fully exercised. */
    testRegions[0U].startupTest = FALSE;
    testRegions[0U].runtimeTest = TRUE;
    testRegions[0U].eccEnabled = TRUE;
    testRegions[0U].crcSeed = 0U;

    /* Reset module state so every test starts from UNINIT regardless of
     * what a previous test left behind. */
    if (RamSafety_GetState() != RAMSAFETY_STATE_UNINIT)
    {
        (void)RamSafety_DeInit();
    }
}

void tearDown(void)
{
}

/* ------------------------------------------------------------------------ */
/* Initialization tests                                                     */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamSafety_00001 */
void test_RamSafety_Init_NullConfig_ReturnsNotOk(void)
{
    Std_ReturnType result = RamSafety_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(0x01U, mock_DetLastApiId);            /* RAMSAFETY_API_INIT */
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_E_INIT_FAILED, mock_DetLastErrorId);
}

/** @req SWS_RamSafety_00001 @req SWS_RamSafety_00101 */
void test_RamSafety_Init_InvalidConfig_ReturnsNotOk(void)
{
    Std_ReturnType result = RamSafety_Init(&testConfigInvalid);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_E_INIT_FAILED, mock_DetLastErrorId);
}

/** @req SWS_RamSafety_00001 */
void test_RamSafety_Init_ValidConfig_ReturnsOk(void)
{
    Std_ReturnType result = RamSafety_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_INIT, RamSafety_GetState());
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
    (void)RamSafety_DeInit();
}

/** @req SWS_RamSafety_00001 */
void test_RamSafety_Init_AlreadyInitialized_ReturnsNotOk(void)
{
    (void)RamSafety_Init(&testConfig);
    Std_ReturnType result = RamSafety_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_E_INVALID_STATE, mock_DetLastErrorId);
    (void)RamSafety_DeInit();
}

/** @req SWS_RamSafety_00002 */
void test_RamSafety_DeInit_NotInitialized_ReturnsNotOk(void)
{
    Std_ReturnType result = RamSafety_DeInit();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(0x02U, mock_DetLastApiId);            /* RAMSAFETY_API_DEINIT */
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_E_INVALID_STATE, mock_DetLastErrorId);
}

/** @req SWS_RamSafety_00002 */
void test_RamSafety_DeInit_AfterInit_ReturnsOk(void)
{
    (void)RamSafety_Init(&testConfig);
    Std_ReturnType result = RamSafety_DeInit();
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_UNINIT, RamSafety_GetState());
    TEST_ASSERT_EQUAL_UINT32(1U, mock_PlatformDeInitCount);
}

/* ------------------------------------------------------------------------ */
/* Startup and triggered test tests                                         */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamSafety_00010 */
void test_RamSafety_RunStartupTest_NotInitialized_ReturnsNotOk(void)
{
    Std_ReturnType result = RamSafety_RunStartupTest(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(0x03U, mock_DetLastApiId);            /* RAMSAFETY_API_RUN_STARTUP */
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_E_INVALID_STATE, mock_DetLastErrorId);
}

/**
 * @req SWS_RamSafety_00010
 * @req SWS_RamSafety_00102
 * @req SWS_RamSafety_00103
 * Region is configured with startupTest == FALSE (64-bit host, see header),
 * so the startup test performs no raw-memory walks but must still finish
 * successfully, reach ACTIVE, and report final progress (100 %, no region).
 */
void test_RamSafety_RunStartupTest_Valid_ReachesActiveState(void)
{
    (void)RamSafety_Init(&testConfig);
    Std_ReturnType result = RamSafety_RunStartupTest(mock_ProgressCallback);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_ACTIVE, RamSafety_GetState());
    /* No startup-test region -> no ECC check, no error handling */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EccCheckCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EnterSafeStateCount);
    /* Final progress notification: exactly once, 100 %, no region */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_ProgressCallCount);
    TEST_ASSERT_EQUAL_UINT8(100U, mock_ProgressLastPercent);
    TEST_ASSERT_NULL(mock_ProgressLastRegion);
    (void)RamSafety_DeInit();
}

/**
 * @req SWS_RamSafety_00012
 * RamSafety_TriggerTest reports no DET; invalid state is signalled via the
 * RamSafety_ResultType return value only.
 */
void test_RamSafety_TriggerTest_NotInitialized_ReturnsError(void)
{
    RamSafety_ResultType result = RamSafety_TriggerTest(RAMSAFETY_TEST_MARCH_C, 0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_RESULT_ERROR, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
}

/** @req SWS_RamSafety_00012 */
void test_RamSafety_TriggerTest_InvalidRegionId_ReturnsError(void)
{
    (void)RamSafety_Init(&testConfig);
    RamSafety_ResultType result = RamSafety_TriggerTest(RAMSAFETY_TEST_MARCH_C, 5U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_RESULT_ERROR, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
    (void)RamSafety_DeInit();
}

/* ------------------------------------------------------------------------ */
/* Region verification tests                                                */
/* ------------------------------------------------------------------------ */

/**
 * @req SWS_RamSafety_00020
 * Stubbed CRCs match (MOCK_CRC_VALUE) so verification succeeds; both platform
 * CRC hooks must have been invoked exactly once.
 */
void test_RamSafety_VerifyRegion_ValidId_ReturnsOk(void)
{
    Std_ReturnType result;
    (void)RamSafety_Init(&testConfig);
    result = RamSafety_VerifyRegion(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_CrcCalcCount);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_CrcGetCount);
    (void)RamSafety_DeInit();
}

/** @req SWS_RamSafety_00020 */
void test_RamSafety_VerifyRegion_InvalidId_ReturnsNotOk(void)
{
    Std_ReturnType result;
    (void)RamSafety_Init(&testConfig);
    result = RamSafety_VerifyRegion(1U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_CrcCalcCount);
    (void)RamSafety_DeInit();
}

/**
 * @req SWS_RamSafety_00021
 * VerifyRange with size 0 performs no walk at all (a nonzero size would make
 * the SUT write the raw uint32 address, which is a truncated pointer on this
 * 64-bit host - see file header). The API is also callable before Init
 * (no state guard), so this doubles as the not-initialized behavior check.
 */
void test_RamSafety_VerifyRange_ZeroSizeRange_ReturnsOk(void)
{
    Std_ReturnType result = RamSafety_VerifyRange((uint32)testRegionBufferWords, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    /* Buffer must remain untouched (no walk performed) */
    TEST_ASSERT_EQUAL_UINT32(0U, testRegionBufferWords[0U]);
}

/* ------------------------------------------------------------------------ */
/* Statistics tests                                                         */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamSafety_00030 */
void test_RamSafety_GetStatistics_NotInitialized_ReturnsNotOk(void)
{
    RamSafety_StatisticsType stats;
    Std_ReturnType result = RamSafety_GetStatistics(&stats);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
}

/** @req SWS_RamSafety_00030 */
void test_RamSafety_GetStatistics_AfterInit_ReturnsOk(void)
{
    RamSafety_StatisticsType stats;
    Std_ReturnType result;
    (void)RamSafety_Init(&testConfig);
    memset(&stats, 0xFF, sizeof(stats));
    result = RamSafety_GetStatistics(&stats);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.testsPassed);
    TEST_ASSERT_EQUAL_UINT32(0U, stats.testsFailed);
}

/**
 * @req SWS_RamSafety_00031
 * In UNINIT state ClearStatistics returns E_NOT_OK. Unlike Init/DeInit/
 * RunStartupTest the SUT guard returns without a Det report (SUT behavior,
 * asserted as-is).
 */
void test_RamSafety_ClearStatistics_NotInitialized_ReturnsNotOk(void)
{
    Std_ReturnType result = RamSafety_ClearStatistics();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
}

/* ------------------------------------------------------------------------ */
/* ECC status tests                                                         */
/* ------------------------------------------------------------------------ */

/** @req SWS_RamSafety_00032 */
void test_RamSafety_CheckEccStatus_NullHasError_ReturnsNotOk(void)
{
    Std_ReturnType result;
    uint32 errorCount = 0U;
    (void)RamSafety_Init(&testConfig);
    result = RamSafety_CheckEccStatus(0U, NULL_PTR, &errorCount);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_EccCheckCount);
    (void)RamSafety_DeInit();
}

/**
 * @req SWS_RamSafety_00032
 * Valid call must be forwarded to the platform layer with the region start
 * address and report the stubbed no-error status.
 */
void test_RamSafety_CheckEccStatus_Valid_CallsPlatformStub(void)
{
    Std_ReturnType result;
    boolean hasError = TRUE;
    uint32 errorCount = 99U;
    (void)RamSafety_Init(&testConfig);
    result = RamSafety_CheckEccStatus(0U, &hasError, &errorCount);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_EccCheckCount);
    TEST_ASSERT_EQUAL_UINT32((uint32)testRegionBufferWords, mock_EccLastAddress);
    TEST_ASSERT_EQUAL(FALSE, hasError);
    TEST_ASSERT_EQUAL_UINT32(0U, errorCount);
    (void)RamSafety_DeInit();
}

/* ------------------------------------------------------------------------ */
/* Version info tests                                                       */
/* ------------------------------------------------------------------------ */

#if (RAMSAFETY_VERSION_INFO_API == STD_ON)
/** @req SWS_RamSafety_00050 */
void test_RamSafety_GetVersionInfo_ReturnsExpectedValues(void)
{
    Std_VersionInfoType versionInfo;
    memset(&versionInfo, 0, sizeof(versionInfo));
    RamSafety_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL_UINT16(43U, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(0x1BU, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(1U, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(0U, versionInfo.sw_patch_version);
}
#endif

/* ------------------------------------------------------------------------ */
/* MainFunction / safe-state tests                                          */
/* ------------------------------------------------------------------------ */

/**
 * @req SWS_RamSafety_00011
 * MainFunction in non-ACTIVE state is a silent no-op (no Det report, no
 * state change).
 */
void test_RamSafety_MainFunction_NotActive_NoDetNoStateChange(void)
{
    RamSafety_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetErrorCount);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_UNINIT, RamSafety_GetState());
}

/**
 * @req SWS_RamSafety_00011
 * @req SWS_RamSafety_00020
 * In ACTIVE state with runtimePeriodMs=1 the first MainFunction call must
 * run the CRC-based runtime verification (platform CRC hooks invoked) and
 * keep the module in ACTIVE state.
 */
void test_RamSafety_MainFunction_Active_RunsRuntimeVerification(void)
{
    (void)RamSafety_Init(&testConfig);
    (void)RamSafety_RunStartupTest(NULL_PTR);
    mock_CrcCalcCount = 0U;
    mock_CrcGetCount = 0U;
    RamSafety_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_ACTIVE, RamSafety_GetState());
    TEST_ASSERT_EQUAL_UINT32(1U, mock_CrcCalcCount);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_CrcGetCount);
    (void)RamSafety_DeInit();
}

/**
 * @req SWS_RamSafety_00040
 * EnterSafeState must move the module to ERROR state and notify the
 * platform safe-state hook exactly once.
 */
void test_RamSafety_EnterSafeState_SetsErrorState(void)
{
    (void)RamSafety_Init(&testConfig);
    RamSafety_EnterSafeState(RAMSAFETY_E_TEST_FAILED);
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_ERROR, RamSafety_GetState());
    TEST_ASSERT_EQUAL_UINT32(1U, mock_EnterSafeStateCount);
    /* DeInit from ERROR state is allowed (only UNINIT is rejected) */
    TEST_ASSERT_EQUAL_UINT8(E_OK, RamSafety_DeInit());
    TEST_ASSERT_EQUAL_UINT8(RAMSAFETY_STATE_UNINIT, RamSafety_GetState());
}

/* ------------------------------------------------------------------------ */
/* Test runner                                                              */
/* ------------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_RamSafety_Init_NullConfig_ReturnsNotOk);
    RUN_TEST(test_RamSafety_Init_InvalidConfig_ReturnsNotOk);
    RUN_TEST(test_RamSafety_Init_ValidConfig_ReturnsOk);
    RUN_TEST(test_RamSafety_Init_AlreadyInitialized_ReturnsNotOk);
    RUN_TEST(test_RamSafety_DeInit_NotInitialized_ReturnsNotOk);
    RUN_TEST(test_RamSafety_DeInit_AfterInit_ReturnsOk);
    RUN_TEST(test_RamSafety_RunStartupTest_NotInitialized_ReturnsNotOk);
    RUN_TEST(test_RamSafety_RunStartupTest_Valid_ReachesActiveState);
    RUN_TEST(test_RamSafety_TriggerTest_NotInitialized_ReturnsError);
    RUN_TEST(test_RamSafety_TriggerTest_InvalidRegionId_ReturnsError);
    RUN_TEST(test_RamSafety_VerifyRegion_ValidId_ReturnsOk);
    RUN_TEST(test_RamSafety_VerifyRegion_InvalidId_ReturnsNotOk);
    RUN_TEST(test_RamSafety_VerifyRange_ZeroSizeRange_ReturnsOk);
    RUN_TEST(test_RamSafety_GetStatistics_NotInitialized_ReturnsNotOk);
    RUN_TEST(test_RamSafety_GetStatistics_AfterInit_ReturnsOk);
    RUN_TEST(test_RamSafety_ClearStatistics_NotInitialized_ReturnsNotOk);
    RUN_TEST(test_RamSafety_CheckEccStatus_NullHasError_ReturnsNotOk);
    RUN_TEST(test_RamSafety_CheckEccStatus_Valid_CallsPlatformStub);
#if (RAMSAFETY_VERSION_INFO_API == STD_ON)
    RUN_TEST(test_RamSafety_GetVersionInfo_ReturnsExpectedValues);
#endif
    RUN_TEST(test_RamSafety_MainFunction_NotActive_NoDetNoStateChange);
    RUN_TEST(test_RamSafety_MainFunction_Active_RunsRuntimeVerification);
    RUN_TEST(test_RamSafety_EnterSafeState_SetsErrorState);
    return UnityEnd();
}
