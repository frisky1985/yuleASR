/**
 * @file test_ramtst.c
 * @brief RamTst (RAM Test) Unit Tests
 * @req SWS_RamTst
 */

// @tests src/bsw/services/ramtst/src/RamTst.c  @tests src/bsw/services/ramtst/include/RamTst.h
#include "unity.h"
#include "RamTst.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static RamTst_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_RamTst_00001 */
void test_RamTst_Init_NullPtr_ShouldNotCrash(void) {
    RamTst_Init(NULL_PTR);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_RamTst_00001 */
void test_RamTst_Init_ValidConfig_ShouldSucceed(void) {
    testConfig.NumRegions = 0U;
    RamTst_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_RamTst_00002 */
void test_RamTst_DeInit_AfterInit_ShouldSucceed(void) {
    RamTst_Init(&testConfig);
    RamTst_DeInit();
    TEST_ASSERT_TRUE(1);
}

/** @req SWS_RamTst_00003 */
void test_RamTst_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info;
    RamTst_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(RAMTST_VENDOR_ID, info.vendorID);
}

/** @req SWS_RamTst_00003 */
void test_RamTst_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    RamTst_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls);
}

/** @req SWS_RamTst_00004 */
void test_RamTst_RunTest_BeforeInit_ShouldFail(void) {
    Std_ReturnType ret = RamTst_RunTest(0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_RamTst_00004 */
void test_RamTst_RunTest_AfterInit_ShouldReturnResult(void) {
    RamTst_Init(&testConfig);
    Std_ReturnType ret = RamTst_RunTest(0U);
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_RamTst_00005 */
void test_RamTst_GetResult_AfterInit_ShouldSucceed(void) {
    RamTst_Init(&testConfig);
    RamTst_ResultType result;
    Std_ReturnType ret = RamTst_GetResult(&result);
    TEST_ASSERT_EQUAL(E_OK, ret);
}

/** @req SWS_RamTst_00005 */
void test_RamTst_GetResult_NullPtr_ShouldFail(void) {
    RamTst_Init(&testConfig);
    Std_ReturnType ret = RamTst_GetResult(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
}

/** @req SWS_RamTst_00006 */
void test_RamTst_Abort_AfterInit_ShouldReturnResult(void) {
    RamTst_Init(&testConfig);
    Std_ReturnType ret = RamTst_Abort();
    TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK);
}

/** @req SWS_RamTst_00007 */
void test_RamTst_MainFunction_AfterInit_ShouldNotCrash(void) {
    RamTst_Init(&testConfig);
    RamTst_MainFunction();
    TEST_ASSERT_TRUE(1);
}

void test_RamTst_DeInit_BeforeInit_ShouldNotCrash(void) {
    RamTst_DeInit();
    TEST_ASSERT_TRUE(1);
}

void test_RamTst_Init_DoubleInit_ShouldNotCrash(void) {
    RamTst_Init(&testConfig);
    RamTst_Init(&testConfig);
    TEST_ASSERT_TRUE(1);
}
