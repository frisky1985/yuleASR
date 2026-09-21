/**
 * @file test_tm.c
 * @brief Tm Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/tm/src/Tm.c  @tests src/bsw/services/tm/include/Tm.h

#include "unity.h"
#include "Tm.h"
#include "Det.h"

/* Mock Det_ReportError — records last ApiId/ErrorId and call count.
 * test_tm.c defines its own Det_ReportError, so tests/mocks/mock_det.c
 * must NOT be linked into this target. */
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

/* Test config — real Tm_ConfigType fields (numTimeBases/defaultResolution/enableSync) */
static Tm_ConfigType testConfig = { 1U, 1000U, TRUE };

void setUp(void) {
    /* Force known UNINIT state; Tm_DeInit() is unguarded in the SUT. */
    Tm_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_Tm_00001 */
void test_Tm_Init_NullPtr_ShouldNotCrash(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_Init(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetLastApiId);              /* SID Tm_Init = 0 */
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Tm_00001 */
void test_Tm_Init_ValidConfig_ShouldSucceed(void) {
    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00001 */
void test_Tm_Init_DoubleInit_ShouldFail(void) {
    /* Actual SUT behaviour: second Tm_Init() returns E_NOT_OK without DET. */
    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00002 */
void test_Tm_DeInit_Uninit_ShouldNotReportDet(void) {
    /* Actual SUT behaviour: Tm_DeInit() is unguarded and reports no DET. */
    Tm_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00002 */
void test_Tm_DeInit_ValidCall_ShouldResetState(void) {
    Tm_TimeBaseType value = 0U;

    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    Tm_DeInit();
    /* After DeInit the module is UNINIT again: GetTimeBaseValue must fail with DET. */
    mock_Det_Reset();
    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_GetTimeBaseValue(0U, &value));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);              /* SID Tm_GetTimeBaseValue = 1 */
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Tm_00007 */
void test_Tm_GetGlobalTime_NullPtr_ShouldReturnNotOk(void) {
    /* Actual SUT behaviour: no DET is reported for NULL pointer. */
    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_GetGlobalTime(NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00007 */
void test_Tm_GetGlobalTime_ValidPtr_ShouldConvert(void) {
    Tm_GlobalTimeType time = { 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU };

    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    /* local time 2500 ticks * 1 ms resolution = 2 s + 500 ms */
    TEST_ASSERT_EQUAL(E_OK, Tm_SetTimeBaseValue(0U, 2500U));
    TEST_ASSERT_EQUAL(E_OK, Tm_GetGlobalTime(&time));
    TEST_ASSERT_EQUAL_UINT32(0U, time.secondsHigh);
    TEST_ASSERT_EQUAL_UINT32(2U, time.secondsLow);
    TEST_ASSERT_EQUAL_UINT32(500000000U, time.nanoseconds);
}

/** @req SWS_Tm_00003 */
void test_Tm_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Actual SUT behaviour: Tm_MainFunction() silently no-ops when UNINIT, no DET. */
    Tm_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00003 */
void test_Tm_MainFunction_ValidCall_ShouldTick(void) {
    Tm_TimeBaseType value = 0U;

    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    Tm_MainFunction();
    Tm_MainFunction();
    Tm_MainFunction();
    TEST_ASSERT_EQUAL(E_OK, Tm_GetTimeBaseValue(0U, &value));
    TEST_ASSERT_EQUAL_UINT32(3U, value);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00004 */
void test_Tm_GetTimeBaseValue_Uninit_ShouldReportError(void) {
    Tm_TimeBaseType value = 0U;

    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_GetTimeBaseValue(0U, &value));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetLastApiId);              /* SID Tm_GetTimeBaseValue = 1 */
    TEST_ASSERT_EQUAL_UINT8(DET_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Tm_00004 */
void test_Tm_GetTimeBaseValue_NullPtr_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    mock_Det_Reset();
    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_GetTimeBaseValue(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(DET_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Tm_00004 */
void test_Tm_GetTimeBaseValue_ValidCall_ShouldSucceed(void) {
    Tm_TimeBaseType value = 0U;

    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL(E_OK, Tm_SetTimeBaseValue(0U, 0x1234U));
    TEST_ASSERT_EQUAL(E_OK, Tm_GetTimeBaseValue(0U, &value));
    TEST_ASSERT_EQUAL_UINT32(0x1234U, value);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00005 */
void test_Tm_SetTimeBaseValue_Uninit_ShouldNotReportDet(void) {
    /* Actual SUT behaviour: Tm_SetTimeBaseValue() is unguarded (no init check, no DET). */
    TEST_ASSERT_EQUAL(E_OK, Tm_SetTimeBaseValue(0U, 2500U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00005 */
void test_Tm_SetTimeBaseValue_ValidCall_ShouldSucceed(void) {
    Tm_TimeBaseType value = 0U;

    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL(E_OK, Tm_SetTimeBaseValue(0U, 0xABCDEFU));
    TEST_ASSERT_EQUAL(E_OK, Tm_GetTimeBaseValue(0U, &value));
    TEST_ASSERT_EQUAL_UINT32(0xABCDEFU, value);
}

/** @req SWS_Tm_00006 */
void test_Tm_GetTimeBaseInfo_NullPtr_ShouldReturnNotOk(void) {
    /* Actual SUT behaviour: no DET is reported for NULL pointer. */
    TEST_ASSERT_EQUAL(E_NOT_OK, Tm_GetTimeBaseInfo(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00006 */
void test_Tm_GetTimeBaseInfo_ValidCall_ShouldFillInfo(void) {
    Tm_TimeBaseInfoType info;

    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL(E_OK, Tm_SetTimeBaseValue(0U, 42U));
    TEST_ASSERT_EQUAL(E_OK, Tm_GetTimeBaseInfo(0U, &info));
    TEST_ASSERT_EQUAL_UINT32(42U, info.currentValue);
    TEST_ASSERT_EQUAL_UINT32(1000U, info.resolution);
    TEST_ASSERT_EQUAL(FALSE, info.isSynchronized);
    TEST_ASSERT_EQUAL(TM_STATUS_RUNNING, info.status);
}

/** @req SWS_Tm_00008 */
void test_Tm_SetGlobalTime_ValidCall_ShouldSucceed(void) {
    const Tm_GlobalTimeType time = { 0U, 2U, 500000000U };

    TEST_ASSERT_EQUAL(E_OK, Tm_SetGlobalTime(&time));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Tm_00009 */
void test_Tm_SyncTimeBase_ValidCall_ShouldSucceed(void) {
    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL(E_OK, Tm_SyncTimeBase(0U, 1U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_Tm_GetElapsedDuration_ShouldComputeDelta(void) {
    TEST_ASSERT_EQUAL(E_OK, Tm_Init(&testConfig));
    TEST_ASSERT_EQUAL(E_OK, Tm_SetTimeBaseValue(0U, 1000U));
    Tm_MainFunction();
    Tm_MainFunction();
    Tm_MainFunction();
    Tm_MainFunction();
    Tm_MainFunction();
    /* 5 ticks since the stored value of 1000 */
    TEST_ASSERT_EQUAL_UINT32(5U, Tm_GetElapsedDuration(0U, 1000U));
    /* 'since' in the future yields 0 */
    TEST_ASSERT_EQUAL_UINT32(0U, Tm_GetElapsedDuration(0U, 2000U));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Tm_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Tm_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Tm_Init_DoubleInit_ShouldFail);
    RUN_TEST(test_Tm_DeInit_Uninit_ShouldNotReportDet);
    RUN_TEST(test_Tm_DeInit_ValidCall_ShouldResetState);
    RUN_TEST(test_Tm_GetGlobalTime_NullPtr_ShouldReturnNotOk);
    RUN_TEST(test_Tm_GetGlobalTime_ValidPtr_ShouldConvert);
    RUN_TEST(test_Tm_MainFunction_Uninit_ShouldNotCrash);
    RUN_TEST(test_Tm_MainFunction_ValidCall_ShouldTick);
    RUN_TEST(test_Tm_GetTimeBaseValue_Uninit_ShouldReportError);
    RUN_TEST(test_Tm_GetTimeBaseValue_NullPtr_ShouldReportError);
    RUN_TEST(test_Tm_GetTimeBaseValue_ValidCall_ShouldSucceed);
    RUN_TEST(test_Tm_SetTimeBaseValue_Uninit_ShouldNotReportDet);
    RUN_TEST(test_Tm_SetTimeBaseValue_ValidCall_ShouldSucceed);
    RUN_TEST(test_Tm_GetTimeBaseInfo_NullPtr_ShouldReturnNotOk);
    RUN_TEST(test_Tm_GetTimeBaseInfo_ValidCall_ShouldFillInfo);
    RUN_TEST(test_Tm_SetGlobalTime_ValidCall_ShouldSucceed);
    RUN_TEST(test_Tm_SyncTimeBase_ValidCall_ShouldSucceed);
    RUN_TEST(test_Tm_GetElapsedDuration_ShouldComputeDelta);
    return UnityEnd();
}
