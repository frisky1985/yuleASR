/**
 * @file test_ramtst_init.c
 * @brief RamTst Initialization Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "RamTst.h"

void setUp(void) {
}

void tearDown(void) {
    RamTst_DeInit();
}

void test_RamTst_Init_ValidConfig(void) {
    extern const RamTst_ConfigType RamTst_Config;
    RamTst_Init(&RamTst_Config);
    TEST_ASSERT_EQUAL(RAMTST_STATUS_IDLE, RamTst_GetTestStatus());
    TEST_ASSERT_EQUAL(RAMTST_RESULT_NOT_TESTED, RamTst_GetTestResult());
}

void test_RamTst_Init_NullConfig(void) {
    RamTst_Init(NULL_PTR);
    /* Should trigger DET error */
}

void test_RamTst_DeInit_AfterInit(void) {
    extern const RamTst_ConfigType RamTst_Config;
    RamTst_Init(&RamTst_Config);
    RamTst_DeInit();
    TEST_ASSERT_EQUAL(RAMTST_STATUS_UNINIT, RamTst_GetTestStatus());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_RamTst_Init_ValidConfig);
    RUN_TEST(test_RamTst_Init_NullConfig);
    RUN_TEST(test_RamTst_DeInit_AfterInit);
    return UNITY_END();
}
