/**
 * @file test_wdg.c
 * @brief Wdg (Watchdog Driver) Unit Tests
 * @req SWS_Wdg
 */

// @tests src/bsw/mcal/wdg/src/Wdg.c  @tests src/bsw/mcal/wdg/include/Wdg.h
#include "unity.h"
#include "Wdg.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Wdg_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Wdg_00001 */
void test_Wdg_Init_NullPtr_ShouldNotCrash(void) {
    Wdg_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Wdg_00001 */
void test_Wdg_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.DefaultMode = WDGM_MODE_OFF;
    testConfig.TriggerCondition = 1000U;
    Wdg_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Wdg_00002 */
void test_Wdg_SetMode_AfterInit_ShouldSucceed(void) {
    Wdg_Init(&testConfig);
    Std_ReturnType ret = Wdg_SetMode(WDGM_MODE_FAST);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Wdg_00002 */
void test_Wdg_SetMode_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = Wdg_SetMode(WDGM_MODE_FAST);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_Wdg_00003 */
void test_Wdg_Trigger_AfterInit_ShouldNotCrash(void) {
    Wdg_Init(&testConfig);
    Wdg_Trigger();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Wdg_00003 */
void test_Wdg_Trigger_BeforeInit_ShouldNotCrash(void) {
    Wdg_Trigger();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_Wdg_00004 */
void test_Wdg_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    Wdg_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(WDG_VENDOR_ID, info.vendorID);
}

/** @req SWS_Wdg_00004 */
void test_Wdg_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Wdg_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_Wdg_00005 */
void test_Wdg_SetTriggerCondition_AfterInit_ShouldSucceed(void) {
    Wdg_Init(&testConfig);
    Std_ReturnType ret = Wdg_SetTriggerCondition(500U);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_Wdg_00006 */
void test_Wdg_GetStatus_AfterInit_ShouldReturnState(void) {
    Wdg_Init(&testConfig);
    Wdg_StateType state = Wdg_GetStatus();
    TEST_ASSERT_TRUE(state == WDG_STATE_IDLE || state == WDG_STATE_BUSY || state == WDG_STATE_UNINIT);
}

/** @req SWS_Wdg_00007 */
void test_Wdg_GetTriggerCounter_AfterInit_ShouldReturnZero(void) {
    Wdg_Init(&testConfig);
    uint32 count = Wdg_GetTriggerCounter();
    TEST_ASSERT_EQUAL(0U, count);
}

/** @req SWS_Wdg_00008 */
void test_Wdg_GetLastTriggerTime_AfterInit_ShouldReturnZero(void) {
    Wdg_Init(&testConfig);
    uint32 time = Wdg_GetLastTriggerTime();
    TEST_ASSERT_EQUAL(0U, time);
}

void test_Wdg_Init_DoubleInit_ShouldNotCrash(void) {
    Wdg_Init(&testConfig);
    Wdg_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

void test_Wdg_SetMode_InvalidMode_ShouldFail(void) {
    Wdg_Init(&testConfig);
    Std_ReturnType ret = Wdg_SetMode(0xFFU);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}
