/**
 * @file test_ramtst_run.c
 * @brief RamTst Test Execution Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "RamTst.h"

void setUp(void) {
    extern const RamTst_ConfigType RamTst_Config;
    RamTst_Init(&RamTst_Config);
}

void tearDown(void) {
    RamTst_DeInit();
}

/** @req SWS_RamTst_00003 */
void test_RamTst_Run_Start(void) {
    Std_ReturnType result = RamTst_Run();
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(RAMTST_STATUS_RUNNING, RamTst_GetTestStatus());
}

/** @req SWS_RamTst_00004 */
void test_RamTst_Stop_Running(void) {
    RamTst_Run();
    RamTst_Stop();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
}

/** @req SWS_RamTst_00005 */
void test_RamTst_GetTestResult_BeforeRun(void) {
    RamTst_TestResultType result = RamTst_GetTestResult();
    TEST_ASSERT_EQUAL(RAMTST_RESULT_NOT_TESTED, result);
}

/** @req SWS_RamTst_00008 */
void test_RamTst_MainFunction_CompletesTest(void) {
    RamTst_Run();
    RamTst_MainFunction();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL(RAMTST_RESULT_OK, RamTst_GetTestResult());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_RamTst_Run_Start);
    RUN_TEST(test_RamTst_Stop_Running);
    RUN_TEST(test_RamTst_GetTestResult_BeforeRun);
    RUN_TEST(test_RamTst_MainFunction_CompletesTest);
    return UNITY_END();
}
