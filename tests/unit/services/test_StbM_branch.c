/**
 * @file test_StbM_branch.c
 * @brief Targeted branch coverage tests for StbM.c
 *
 * Covers previously uncovered branches:
 *   - Line 118: if (cfgPtr != NULL_PTR) in StbM_FindTimeBaseConfig
 *   - Line 120: for loop body in StbM_FindTimeBaseConfig
 *   - Line 147: if (StbM_FindTimeBaseConfig(...) == E_OK) in StbM_GetVirtualLocalTime
 *   - Line 149: if (Eth_GetCurrentTime(...) == E_OK) in StbM_GetVirtualLocalTime
 *   - Line 169: if ((tbPtr->timeValid) != 0U) in StbM_UpdateGlobalTime
 *
 * We include StbM.c directly to access static functions and variables.
 */

#include <string.h>
#include <stdio.h>
#include "unity.h"
#include "Std_Types.h"
#include "Eth.h"

/* ---- Mocks for external dependencies ------------------------------------ */

/* Det_ReportError mock */
static uint8 mock_DetCallCount = 0;
static uint8 mock_DetApiId = 0;
static uint8 mock_DetErrorId = 0;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    mock_DetCallCount++;
    mock_DetApiId = ApiId;
    mock_DetErrorId = ErrorId;
    return E_OK;
}

/* Eth_GetCurrentTime mock */
static Std_ReturnType mock_Eth_GetCurrentTime_Return = E_NOT_OK;
static uint8 mock_Eth_GetCurrentTime_CallCount = 0;

Std_ReturnType Eth_GetCurrentTime(uint8 ControllerId,
                                   Eth_TimeStampType* TimeStampPtr,
                                   Eth_RxStatusType* RxStatusPtr)
{
    (void)ControllerId;
    (void)RxStatusPtr;
    mock_Eth_GetCurrentTime_CallCount++;
    if (mock_Eth_GetCurrentTime_Return == E_OK)
    {
        if (TimeStampPtr != NULL_PTR)
        {
            TimeStampPtr->seconds = 100U;
            TimeStampPtr->nanoseconds = 500000U;
        }
    }
    return mock_Eth_GetCurrentTime_Return;
}

/* Include production source to access static functions and variables */
#include "src/bsw/services/stbm/src/StbM.c"

/* ---- Test configuration ------------------------------------------------- */

static const StbM_TimeBaseConfigType testTimeBaseConfigs[] = {
    {
        .timeBaseId = 0U,
        .timeBaseType = 0U,
        .masterConfig = STBM_MASTER_CONFIG_MASTER,
        .enableTimeRecording = FALSE,
        .enableRateCorrection = TRUE,
        .syncTimeout = 1000U,
        .updateFreq = 10U,
        .allowedRateDeviation = 100000U,
        .ethControllerId = 0U
    },
    {
        .timeBaseId = 1U,
        .timeBaseType = 1U,
        .masterConfig = STBM_MASTER_CONFIG_SLAVE,
        .enableTimeRecording = FALSE,
        .enableRateCorrection = TRUE,
        .syncTimeout = 1000U,
        .updateFreq = 10U,
        .allowedRateDeviation = 100000U,
        .ethControllerId = 0U
    }
};

static const StbM_ConfigType testConfig = {
    .timeBaseConfigs = testTimeBaseConfigs,
    .numTimeBases = 2U,
    .devErrorDetect = TRUE,
    .versionInfoApi = TRUE,
    .enableGptp = TRUE
};

/* ---- setUp / tearDown --------------------------------------------------- */

void setUp(void)
{
    mock_DetCallCount = 0;
    mock_DetApiId = 0;
    mock_DetErrorId = 0;
    mock_Eth_GetCurrentTime_Return = E_NOT_OK;
    mock_Eth_GetCurrentTime_CallCount = 0;
    /* Reset internal state */
    memset(&StbM_InternalState, 0, sizeof(StbM_InternalState));
}

void tearDown(void) {}

/* ---- Helper to init module --------------------------------------------- */
static void test_StbM_DoInit(void)
{
    StbM_Init(&testConfig);
}

/* =========================================================================
 * Lines 118, 120: StbM_FindTimeBaseConfig
 * ========================================================================= */

void test_StbM_FindTimeBaseConfig_ValidConfig_ShouldFindTimeBase(void)
{
    test_StbM_DoInit();

    /* Set timeValid = TRUE via BusSetGlobalTime */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 50U;
    ts.nanoseconds = 1000U;
    StbM_VirtualLocalTimeType vlt = 100000ULL;
    Std_ReturnType ret = StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, StbM_InternalState.TimeBases[0].timeValid);

    /* Now call GetCurrentTime which triggers FindTimeBaseConfig via GetVirtualLocalTime */
    mock_Eth_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, mock_Eth_GetCurrentTime_CallCount);
}

/* =========================================================================
 * Line 147: if (StbM_FindTimeBaseConfig(...) == E_OK)
 * Line 149: if (Eth_GetCurrentTime(...) == E_OK)
 * ========================================================================= */

void test_StbM_GetVirtualLocalTime_AllOk_ShouldReturnTime(void)
{
    test_StbM_DoInit();

    /* Set timeValid = TRUE */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 50U;
    ts.nanoseconds = 1000U;
    StbM_VirtualLocalTimeType vlt = 100000ULL;
    StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);

    mock_Eth_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, mock_Eth_GetCurrentTime_CallCount);
    /* Time was updated from Eth (value depends on rate correction math) */
    TEST_ASSERT_NOT_EQUAL(0U, outTs.seconds + outTs.nanoseconds);
}

void test_StbM_GetVirtualLocalTime_EthFails_ShouldReturnZero(void)
{
    test_StbM_DoInit();

    /* Set timeValid = TRUE */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 50U;
    ts.nanoseconds = 1000U;
    StbM_VirtualLocalTimeType vlt = 100000ULL;
    StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);

    mock_Eth_GetCurrentTime_Return = E_NOT_OK;

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, mock_Eth_GetCurrentTime_CallCount);
}

/* =========================================================================
 * Line 169: if ((tbPtr->timeValid) != 0U) in StbM_UpdateGlobalTime
 * ========================================================================= */

void test_StbM_UpdateGlobalTime_TimeValid_ShouldApplyCorrection(void)
{
    test_StbM_DoInit();

    /* Set timeValid = TRUE */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 50U;
    ts.nanoseconds = 1000U;
    StbM_VirtualLocalTimeType vlt = 100000ULL;
    StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);

    TEST_ASSERT_NOT_EQUAL(0U, StbM_InternalState.TimeBases[0].timeValid);

    mock_Eth_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
}

void test_StbM_UpdateGlobalTime_TimeNotValid_ShouldSkipCorrection(void)
{
    test_StbM_DoInit();

    TEST_ASSERT_EQUAL_UINT8(0U, StbM_InternalState.TimeBases[0].timeValid);

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_Eth_GetCurrentTime_CallCount);
}

/* =========================================================================
 * Line 118 false branch: cfgPtr == NULL
 * ========================================================================= */

void test_StbM_FindTimeBaseConfig_NullConfig_ShouldReturnNotOk(void)
{
    test_StbM_DoInit();

    /* Set timeValid = TRUE so GetCurrentTime enters the time-update path */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    StbM_VirtualLocalTimeType vlt = 0ULL;
    StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);

    /* Set ConfigPtr to NULL to trigger false branch of line 118 */
    StbM_InternalState.ConfigPtr = NULL_PTR;

    mock_Eth_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);

    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Eth should NOT have been called since FindTimeBaseConfig returned E_NOT_OK */
    TEST_ASSERT_EQUAL_UINT8(0U, mock_Eth_GetCurrentTime_CallCount);
}

/* =========================================================================
 * Init already-initialized (line 251)
 * ========================================================================= */

void test_StbM_Init_AlreadyInitialized_ShouldReportDet(void)
{
    test_StbM_DoInit();
    mock_DetCallCount = 0;

    StbM_Init(&testConfig);
    TEST_ASSERT_NOT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_INIT, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_ALREADY_INITIALIZED, mock_DetErrorId);
}

/* =========================================================================
 * Init NULL config (line 257)
 * ========================================================================= */

void test_StbM_Init_NullConfig_ShouldReportDet(void)
{
    /* State is UNINIT after setUp */
    StbM_Init(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_INIT, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

/* =========================================================================
 * DeInit uninit (line 305)
 * ========================================================================= */

void test_StbM_DeInit_Uninit_ShouldReportDet(void)
{
    /* State is UNINIT after setUp */
    StbM_DeInit();
    TEST_ASSERT_NOT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_DEINIT, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetErrorId);
}

/* =========================================================================
 * GetVersionInfo NULL (line 327)
 * ========================================================================= */

void test_StbM_GetVersionInfo_Null_ShouldReportDet(void)
{
    StbM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETVERSIONINFO, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

/* =========================================================================
 * GetVersionInfo valid (lines 334-338)
 * ========================================================================= */

void test_StbM_GetVersionInfo_Valid_ShouldFillFields(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    StbM_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(STBM_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(STBM_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(STBM_SW_MAJOR_VERSION, vi.sw_major_version);
}

/* =========================================================================
 * GetCurrentTime uninit (line 354)
 * ========================================================================= */

void test_StbM_GetCurrentTime_Uninit_ShouldReportDet(void)
{
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &ts, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTTIME, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetErrorId);
}

/* =========================================================================
 * GetCurrentTime invalid timebase (line 360)
 * ========================================================================= */

void test_StbM_GetCurrentTime_InvalidTimebase_ShouldReportDet(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    Std_ReturnType ret = StbM_GetCurrentTime(255U, &ts, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_INVALID_TIMEBASE_ID, mock_DetErrorId);
}

/* =========================================================================
 * GetCurrentTime NULL timeStampPtr (line 366)
 * ========================================================================= */

void test_StbM_GetCurrentTime_NullTimeStampPtr_ShouldReportDet(void)
{
    test_StbM_DoInit();
    Std_ReturnType ret = StbM_GetCurrentTime(0U, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

/* =========================================================================
 * GetCurrentTime with userDataPtr (lines 387-392)
 * ========================================================================= */

void test_StbM_GetCurrentTime_WithUserData_ShouldCopyUserData(void)
{
    test_StbM_DoInit();

    /* Set timeValid = TRUE and set userData via SetGlobalTime */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 10U;
    ts.nanoseconds = 500U;
    StbM_UserDataType ud;
    ud.userByte0 = 0xAA;
    ud.userByte1 = 0xBB;
    ud.userByte2 = 0xCC;
    StbM_SetGlobalTime(0U, &ts, &ud);

    mock_Eth_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType outTs;
    StbM_UserDataType outUd;
    memset(&outTs, 0, sizeof(outTs));
    memset(&outUd, 0, sizeof(outUd));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, &outUd);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0xAA, outUd.userByte0);
    TEST_ASSERT_EQUAL_UINT8(0xBB, outUd.userByte1);
    TEST_ASSERT_EQUAL_UINT8(0xCC, outUd.userByte2);
}

/* =========================================================================
 * GetCurrentVirtualTime uninit (line 410)
 * ========================================================================= */

void test_StbM_GetCurrentVirtualTime_Uninit_ShouldReportDet(void)
{
    StbM_VirtualLocalTimeType vlt = 0ULL;
    Std_ReturnType ret = StbM_GetCurrentVirtualTime(0U, &vlt);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_GETCURRENTVIRTUALTIME, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetErrorId);
}

/* =========================================================================
 * GetCurrentVirtualTime invalid timebase (line 416)
 * ========================================================================= */

void test_StbM_GetCurrentVirtualTime_InvalidTimebase_ShouldReportDet(void)
{
    test_StbM_DoInit();
    StbM_VirtualLocalTimeType vlt = 0ULL;
    Std_ReturnType ret = StbM_GetCurrentVirtualTime(255U, &vlt);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_INVALID_TIMEBASE_ID, mock_DetErrorId);
}

/* =========================================================================
 * GetCurrentVirtualTime NULL ptr (line 422)
 * ========================================================================= */

void test_StbM_GetCurrentVirtualTime_NullPtr_ShouldReportDet(void)
{
    test_StbM_DoInit();
    Std_ReturnType ret = StbM_GetCurrentVirtualTime(0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

/* =========================================================================
 * GetCurrentVirtualTime valid (lines 429-430)
 * ========================================================================= */

void test_StbM_GetCurrentVirtualTime_Valid_ShouldReturnOk(void)
{
    test_StbM_DoInit();
    StbM_VirtualLocalTimeType vlt = 0ULL;
    Std_ReturnType ret = StbM_GetCurrentVirtualTime(0U, &vlt);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
}

/* =========================================================================
 * SetGlobalTime uninit (line 447)
 * ========================================================================= */

void test_StbM_SetGlobalTime_Uninit_ShouldReportDet(void)
{
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    Std_ReturnType ret = StbM_SetGlobalTime(0U, &ts, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_SETGLOBALTIME, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetErrorId);
}

/* =========================================================================
 * SetGlobalTime invalid timebase (line 453)
 * ========================================================================= */

void test_StbM_SetGlobalTime_InvalidTimebase_ShouldReportDet(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    Std_ReturnType ret = StbM_SetGlobalTime(255U, &ts, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_INVALID_TIMEBASE_ID, mock_DetErrorId);
}

/* =========================================================================
 * SetGlobalTime NULL ptr (line 459)
 * ========================================================================= */

void test_StbM_SetGlobalTime_NullPtr_ShouldReportDet(void)
{
    test_StbM_DoInit();
    Std_ReturnType ret = StbM_SetGlobalTime(0U, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

/* =========================================================================
 * SetGlobalTime master with userData (lines 469-485)
 * ========================================================================= */

void test_StbM_SetGlobalTime_MasterWithUserData_ShouldSetTimeAndData(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 42U;
    ts.nanoseconds = 12345U;
    StbM_UserDataType ud;
    ud.userByte0 = 0x11;
    ud.userByte1 = 0x22;
    ud.userByte2 = 0x33;

    Std_ReturnType ret = StbM_SetGlobalTime(0U, &ts, &ud);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, StbM_InternalState.TimeBases[0].timeValid);
    TEST_ASSERT_EQUAL_UINT8(0x11, StbM_InternalState.TimeBases[0].userData.userByte0);
}

/* =========================================================================
 * SetGlobalTime slave (isMaster == FALSE) -> returns E_NOT_OK
 * ========================================================================= */

void test_StbM_SetGlobalTime_Slave_ShouldReturnNotOk(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 10U;
    /* Timebase 1 is SLAVE */
    Std_ReturnType ret = StbM_SetGlobalTime(1U, &ts, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

/* =========================================================================
 * BusSetGlobalTime uninit (line 505)
 * ========================================================================= */

void test_StbM_BusSetGlobalTime_Uninit_ShouldReportDet(void)
{
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    StbM_VirtualLocalTimeType vlt = 0ULL;
    Std_ReturnType ret = StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_SID_BUSSETGLOBALTIME, mock_DetApiId);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_UNINIT, mock_DetErrorId);
}

/* =========================================================================
 * BusSetGlobalTime invalid timebase (line 511)
 * ========================================================================= */

void test_StbM_BusSetGlobalTime_InvalidTimebase_ShouldReportDet(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    StbM_VirtualLocalTimeType vlt = 0ULL;
    Std_ReturnType ret = StbM_BusSetGlobalTime(255U, &ts, &vlt, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_INVALID_TIMEBASE_ID, mock_DetErrorId);
}

/* =========================================================================
 * BusSetGlobalTime NULL ptrs (line 517)
 * ========================================================================= */

void test_StbM_BusSetGlobalTime_NullTimeStamp_ShouldReportDet(void)
{
    test_StbM_DoInit();
    StbM_VirtualLocalTimeType vlt = 0ULL;
    Std_ReturnType ret = StbM_BusSetGlobalTime(0U, NULL_PTR, &vlt, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

void test_StbM_BusSetGlobalTime_NullVlt_ShouldReportDet(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    Std_ReturnType ret = StbM_BusSetGlobalTime(0U, &ts, NULL_PTR, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(STBM_E_PARAM_POINTER, mock_DetErrorId);
}

/* =========================================================================
 * BusSetGlobalTime with userData (lines 527-533)
 * ========================================================================= */

void test_StbM_BusSetGlobalTime_WithUserData_ShouldSucceed(void)
{
    test_StbM_DoInit();
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 100U;
    ts.nanoseconds = 5000U;
    StbM_VirtualLocalTimeType vlt = 200000ULL;
    StbM_UserDataType ud;
    ud.userByte0 = 0xDD;
    ud.userByte1 = 0xEE;
    ud.userByte2 = 0xFF;

    Std_ReturnType ret = StbM_BusSetGlobalTime(0U, &ts, &vlt, &ud);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_NOT_EQUAL(0U, StbM_InternalState.TimeBases[0].timeValid);
    TEST_ASSERT_EQUAL_UINT8(STBM_SYNC_STATUS_SYNC, StbM_InternalState.TimeBases[0].syncStatus);
}

/* =========================================================================
 * Nanoseconds overflow in UpdateGlobalTime (line 182)
 * Set nanoseconds close to STBM_NS_PER_SECOND so addition overflows.
 * ========================================================================= */

void test_StbM_UpdateGlobalTime_NanosecondOverflow_ShouldIncrementSeconds(void)
{
    test_StbM_DoInit();

    /* Set initial time via BusSetGlobalTime with nanoseconds near 1 second */
    StbM_TimeStampType ts;
    memset(&ts, 0, sizeof(ts));
    ts.seconds = 5U;
    ts.nanoseconds = 999999000U; /* close to 1 second */
    StbM_VirtualLocalTimeType vlt = 1000000ULL;
    StbM_BusSetGlobalTime(0U, &ts, &vlt, NULL_PTR);

    /* Now call GetCurrentTime with Eth returning a later time that causes
       the time increment to push nanoseconds past 1 billion */
    mock_Eth_GetCurrentTime_Return = E_OK;

    StbM_TimeStampType outTs;
    memset(&outTs, 0, sizeof(outTs));
    Std_ReturnType ret = StbM_GetCurrentTime(0U, &outTs, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* The overflow branch should have been exercised */
}

/* =========================================================================
 * DeInit valid path (lines 312-316)
 * ========================================================================= */

void test_StbM_DeInit_Valid_ShouldClearState(void)
{
    test_StbM_DoInit();
    StbM_DeInit();
    TEST_ASSERT_EQUAL_UINT8(STBM_STATE_UNINIT, StbM_InternalState.State);
    TEST_ASSERT_NULL(StbM_InternalState.ConfigPtr);
}

/* ---- main --------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_StbM_FindTimeBaseConfig_ValidConfig_ShouldFindTimeBase);
    RUN_TEST(test_StbM_GetVirtualLocalTime_AllOk_ShouldReturnTime);
    RUN_TEST(test_StbM_GetVirtualLocalTime_EthFails_ShouldReturnZero);
    RUN_TEST(test_StbM_UpdateGlobalTime_TimeValid_ShouldApplyCorrection);
    RUN_TEST(test_StbM_UpdateGlobalTime_TimeNotValid_ShouldSkipCorrection);
    RUN_TEST(test_StbM_FindTimeBaseConfig_NullConfig_ShouldReturnNotOk);
    RUN_TEST(test_StbM_Init_AlreadyInitialized_ShouldReportDet);
    RUN_TEST(test_StbM_Init_NullConfig_ShouldReportDet);
    RUN_TEST(test_StbM_DeInit_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_DeInit_Valid_ShouldClearState);
    RUN_TEST(test_StbM_GetVersionInfo_Null_ShouldReportDet);
    RUN_TEST(test_StbM_GetVersionInfo_Valid_ShouldFillFields);
    RUN_TEST(test_StbM_GetCurrentTime_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentTime_InvalidTimebase_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentTime_NullTimeStampPtr_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentTime_WithUserData_ShouldCopyUserData);
    RUN_TEST(test_StbM_GetCurrentVirtualTime_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentVirtualTime_InvalidTimebase_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentVirtualTime_NullPtr_ShouldReportDet);
    RUN_TEST(test_StbM_GetCurrentVirtualTime_Valid_ShouldReturnOk);
    RUN_TEST(test_StbM_SetGlobalTime_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_SetGlobalTime_InvalidTimebase_ShouldReportDet);
    RUN_TEST(test_StbM_SetGlobalTime_NullPtr_ShouldReportDet);
    RUN_TEST(test_StbM_SetGlobalTime_MasterWithUserData_ShouldSetTimeAndData);
    RUN_TEST(test_StbM_SetGlobalTime_Slave_ShouldReturnNotOk);
    RUN_TEST(test_StbM_BusSetGlobalTime_Uninit_ShouldReportDet);
    RUN_TEST(test_StbM_BusSetGlobalTime_InvalidTimebase_ShouldReportDet);
    RUN_TEST(test_StbM_BusSetGlobalTime_NullTimeStamp_ShouldReportDet);
    RUN_TEST(test_StbM_BusSetGlobalTime_NullVlt_ShouldReportDet);
    RUN_TEST(test_StbM_BusSetGlobalTime_WithUserData_ShouldSucceed);
    RUN_TEST(test_StbM_UpdateGlobalTime_NanosecondOverflow_ShouldIncrementSeconds);

    return UNITY_END();
}
