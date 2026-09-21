/**
 * @file test_stbm.c
 * @brief StbM Unit Tests — Substantiated
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Substantiation: rewritten against the real StbM API (StbM.h) implemented
 * in src/bsw/services/stbm/src/StbM.c. The Ethernet hardware clock is
 * replaced by a deterministic Eth_GetCurrentTime stub. Assertions cover
 * return values, global/virtual time values, master/slave distinction and
 * exact DET mock arguments.
 *
 * Note: StbM_GetTimeBaseStatus / StbM_GetMasterConfig / StbM_SetRateCorrection /
 * StbM_GetTimeBaseUpdateCounter / StbM_GetCurrentTimeDiff / StbM_SetUserData /
 * StbM_MainFunction / StbM_TimeStampChanged are declared in StbM.h but NOT
 * implemented in StbM.c — they are intentionally not exercised here.
 */

// @tests src/bsw/services/stbm/src/StbM.c  @tests src/bsw/services/stbm/include/StbM.h

#include "unity.h"
#include "StbM.h"
#include "Eth.h"

/* Deterministic Ethernet hardware clock stub:
 * constant 100 s + 500 ns -> virtual local time 100000000500 ns. */
#define TEST_ETH_SECONDS        100U
#define TEST_ETH_NANOSECONDS    500U
#define TEST_ETH_LOCAL_TIME_NS  (100000000500ULL)

static uint32 mock_EthCallCount = 0U;

Std_ReturnType Eth_GetCurrentTime(uint8 ControllerId, Eth_TimeStampType* TimeStampPtr, Eth_RxStatusType* RxStatusPtr)
{
    (void)ControllerId;
    (void)RxStatusPtr;
    mock_EthCallCount++;
    if (TimeStampPtr != NULL_PTR)
    {
        TimeStampPtr->seconds = TEST_ETH_SECONDS;
        TimeStampPtr->nanoseconds = TEST_ETH_NANOSECONDS;
    }
    return E_OK;
}

/* Mock Det_ReportError — records full argument set for verification */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint16 mock_DetLastModuleId = 0U;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetLastModuleId = 0U;
    mock_DetCallCount = 0U;
    mock_EthCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test configuration: time base 0 = master, time base 1 = slave. */
static const StbM_TimeBaseConfigType testTimeBaseConfigs[] = {
    {
        0U,                         /* timeBaseId */
        STBM_TIMEBASE_GLOBAL,       /* timeBaseType */
        STBM_MASTER_CONFIG_MASTER,  /* masterConfig */
        FALSE,                      /* enableTimeRecording */
        FALSE,                      /* enableRateCorrection */
        1000U,                      /* syncTimeout */
        10U,                        /* updateFreq */
        100000U,                    /* allowedRateDeviation */
        0U                          /* ethControllerId */
    },
    {
        1U,                         /* timeBaseId */
        STBM_TIMEBASE_GLOBAL,       /* timeBaseType */
        STBM_MASTER_CONFIG_SLAVE,   /* masterConfig */
        FALSE,                      /* enableTimeRecording */
        FALSE,                      /* enableRateCorrection */
        1000U,                      /* syncTimeout */
        10U,                        /* updateFreq */
        100000U,                    /* allowedRateDeviation */
        0U                          /* ethControllerId */
    },
};

static const StbM_ConfigType testConfig = {
    testTimeBaseConfigs,
    2U,          /* numTimeBases */
    TRUE,        /* devErrorDetect */
    TRUE,        /* versionInfoApi */
    TRUE         /* enableGptp */
};

/* SWS_StbM error codes (StbM.h) */
#define STBM_E_ALREADY_INITIALIZED_TEST  0x04U
#define STBM_E_INVALID_TIMEBASE_ID_TEST  0x05U

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
}

/* StbM keeps a file-static init flag; there is no silent re-init path, so
 * tests that need a fresh UNINIT state simply never initialize first. */

/** @req SWS_StbM_00001 */
void test_StbM_Init_NullPtr_ShouldReportDet(void) {
    StbM_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(STBM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_StbM_00001 */
void test_StbM_Init_ValidConfig_ShouldSucceed(void) {
    StbM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Module initialized: GetCurrentVirtualTime must reach the Eth stub. */
    StbM_VirtualLocalTimeType localTime = 0ULL;
    TEST_ASSERT_EQUAL(E_OK, StbM_GetCurrentVirtualTime(0U, &localTime));
    TEST_ASSERT_TRUE(localTime == TEST_ETH_LOCAL_TIME_NS);
    TEST_ASSERT_TRUE(mock_EthCallCount > 0U);
    StbM_DeInit();
}

/** @req SWS_StbM_00001 */
void test_StbM_Init_DoubleInit_ShouldReportAlreadyInitialized(void) {
    StbM_Init(&testConfig);
    mock_Det_Reset();
    StbM_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_ALREADY_INITIALIZED_TEST, mock_DetLastErrorId);
    StbM_DeInit();
}

/** @req SWS_StbM_00002 */
void test_StbM_DeInit_Uninit_ShouldReportDet(void) {
    StbM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_StbM_00002 */
void test_StbM_DeInit_ValidCall_ShouldReturnToUninit(void) {
    StbM_Init(&testConfig);
    StbM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    StbM_TimeStampType timeStamp = {0U, 0U, 0U, 0U};
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_GetCurrentTime(0U, &timeStamp, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_StbM_00003 */
void test_StbM_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    StbM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_StbM_00003 */
void test_StbM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    StbM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(STBM_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(STBM_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(STBM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(STBM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(STBM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_StbM_00005 */
void test_StbM_GetCurrentTime_Uninit_ShouldReportDet(void) {
    StbM_TimeStampType timeStamp = {0U, 0U, 0U, 0U};
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_GetCurrentTime(0U, &timeStamp, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_StbM_00005 */
void test_StbM_GetCurrentTime_InvalidTimeBaseId_ShouldReportDet(void) {
    StbM_Init(&testConfig);
    StbM_TimeStampType timeStamp = {0U, 0U, 0U, 0U};
    /* STBM_NUMBER_OF_TIMEBASES = 4; id 4 is out of range. */
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_GetCurrentTime(STBM_NUMBER_OF_TIMEBASES, &timeStamp, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_INVALID_TIMEBASE_ID_TEST, mock_DetLastErrorId);
    StbM_DeInit();
}

/** @req SWS_StbM_00005 */
void test_StbM_GetCurrentTime_NullPtr_ShouldReportDet(void) {
    StbM_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_GetCurrentTime(0U, NULL_PTR, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetLastErrorId);
    StbM_DeInit();
}

/** @req SWS_StbM_00005 */
void test_StbM_GetCurrentTime_BeforeTimeValid_ShouldReturnNotOk(void) {
    StbM_Init(&testConfig);
    /* Time base never synchronized: E_NOT_OK from the timeValid check, no DET. */
    StbM_TimeStampType timeStamp = {0U, 0U, 0U, 0U};
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_GetCurrentTime(0U, &timeStamp, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    StbM_DeInit();
}

/** @req SWS_StbM_00007 */
void test_StbM_SetGlobalTime_SlaveTimeBase_ShouldReturnNotOk(void) {
    StbM_Init(&testConfig);
    const StbM_TimeStampType newTime = {1000U, 0U, 0U, 0U};
    /* Only a master time base may set global time directly. */
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_SetGlobalTime(1U, &newTime, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    StbM_DeInit();
}

/** @req SWS_StbM_00007 */
void test_StbM_SetGlobalTime_MasterTimeBase_ShouldStoreAndReturnTime(void) {
    StbM_Init(&testConfig);
    const StbM_TimeStampType newTime = {0U, 1000U, 0U, 0U}; /* nanoseconds=0, seconds=1000 */
    TEST_ASSERT_EQUAL(E_OK, StbM_SetGlobalTime(0U, &newTime, NULL_PTR));

    /* Eth clock is constant, so elapsed local time is 0 and the stored
     * global time must come back unchanged. */
    StbM_TimeStampType readBack = {0U, 0U, 0U, 0U};
    TEST_ASSERT_EQUAL(E_OK, StbM_GetCurrentTime(0U, &readBack, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(1000U, readBack.seconds);
    TEST_ASSERT_EQUAL_UINT32(0U, readBack.nanoseconds);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    StbM_DeInit();
}

/** @req SWS_StbM_00008 */
void test_StbM_BusSetGlobalTime_Slave_ShouldSyncAndReturnTime(void) {
    StbM_Init(&testConfig);
    const StbM_TimeStampType busTime = {500U, 2000U, 0U, 0U}; /* ns=500, s=2000 */
    /* Receiving local time = current Eth clock -> zero elapsed time afterwards. */
    const StbM_VirtualLocalTimeType rxLocalTime = TEST_ETH_LOCAL_TIME_NS;
    TEST_ASSERT_EQUAL(E_OK, StbM_BusSetGlobalTime(1U, &busTime, &rxLocalTime, NULL_PTR));

    /* Constant Eth clock -> zero elapsed time -> stored time unchanged. */
    StbM_TimeStampType readBack = {0U, 0U, 0U, 0U};
    TEST_ASSERT_EQUAL(E_OK, StbM_GetCurrentTime(1U, &readBack, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT32(2000U, readBack.seconds);
    TEST_ASSERT_EQUAL_UINT32(500U, readBack.nanoseconds);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    StbM_DeInit();
}

/** @req SWS_StbM_00006 */
void test_StbM_GetCurrentVirtualTime_Uninit_ShouldReportDet(void) {
    StbM_VirtualLocalTimeType localTime = 0ULL;
    TEST_ASSERT_EQUAL(E_NOT_OK, StbM_GetCurrentVirtualTime(0U, &localTime));
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTVIRTUALTIME, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetLastErrorId);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_StbM_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_StbM_DeInit_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_StbM_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_StbM_GetCurrentTime_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentVirtualTime_Uninit_ShouldReportDet);
    /* From here on the module state is INIT until DeInit is called. */
    RUN_TEST(test_StbM_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_StbM_Init_DoubleInit_ShouldReportAlreadyInitialized);
    RUN_TEST(test_StbM_DeInit_ValidCall_ShouldReturnToUninit);
    RUN_TEST(test_StbM_GetCurrentTime_InvalidTimeBaseId_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentTime_NullPtr_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentTime_BeforeTimeValid_ShouldReturnNotOk);
    RUN_TEST(test_StbM_SetGlobalTime_SlaveTimeBase_ShouldReturnNotOk);
    RUN_TEST(test_StbM_SetGlobalTime_MasterTimeBase_ShouldStoreAndReturnTime);
    RUN_TEST(test_StbM_BusSetGlobalTime_Slave_ShouldSyncAndReturnTime);

    return UnityEnd();
}
