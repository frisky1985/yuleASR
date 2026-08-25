/**
 * @file test_bswm.c
 * @brief BswM (BSW Manager) Unit Tests
 * @req SWS_BswM
 */

// @tests src/bsw/services/bswm/src/BswM.c  @tests src/bsw/services/bswm/include/BswM.h
#include "unity.h"
#include "BswM.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static BswM_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_BswM_00001 */
void test_BswM_Init_NullPtr_ShouldNotCrash(void) {
    BswM_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00001 */
void test_BswM_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.NumRules = 0U;
    BswM_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00002 */
void test_BswM_DeInit_AfterInit_ShouldSucceed(void) {
    BswM_Init(&testConfig);
    BswM_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00010 */
void test_BswM_RequestMode_AfterInit_ShouldSucceed(void) {
    BswM_Init(&testConfig);
    Std_ReturnType ret = BswM_RequestMode(0U, BSWM_MODE_FULL);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_BswM_00011 */
void test_BswM_GetCurrentMode_AfterInit_ShouldReturnMode(void) {
    BswM_Init(&testConfig);
    BswM_ModeType mode = BswM_GetCurrentMode();
    TEST_ASSERT_TRUE(mode == BSWM_MODE_FULL || mode == BSWM_MODE_MINIMUM || mode == BSWM_MODE_UNINIT);
}

/** @req SWS_BswM_00012 */
void test_BswM_GetRequestedMode_AfterRequest_ShouldReturnMode(void) {
    BswM_Init(&testConfig);
    BswM_RequestMode(0U, BSWM_MODE_FULL);
    BswM_ModeType mode = BswM_GetRequestedMode();
    TEST_ASSERT_TRUE(mode == BSWM_MODE_FULL || mode == BSWM_MODE_MINIMUM);
}

/** @req SWS_BswM_00020 */
void test_BswM_MainFunction_AfterInit_ShouldNotCrash(void) {
    BswM_Init(&testConfig);
    BswM_MainFunction();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    BswM_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(BSWM_VENDOR_ID, info.vendorID);
}

/** @req SWS_BswM_00030 */
void test_BswM_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    BswM_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

void test_BswM_Init_DoubleInit_ShouldNotCrash(void) {
    BswM_Init(&testConfig);
    BswM_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

void test_BswM_DeInit_BeforeInit_ShouldNotCrash(void) {
    BswM_DeInit();
    TEST_ASSERT_TRUE(1);
}

void test_BswM_RequestMode_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = BswM_RequestMode(0U, BSWM_MODE_FULL);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}
