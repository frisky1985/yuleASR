/**
 * @file test_fee_init.c
 * @brief Fee Initialization Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "Fee.h"

void setUp(void) {
    /* Reset before each test */
}

void tearDown(void) {
    Fee_DeInit();
}

void test_Fee_Init_ValidConfig(void) {
    extern const Fee_ConfigType Fee_Config;
    Fee_Init(&Fee_Config);
    TEST_ASSERT_EQUAL(FEE_IDLE, Fee_GetStatus());
}

void test_Fee_Init_NullConfig(void) {
    /* This should trigger DET error when DEV_ERROR_DETECT is ON */
    Fee_Init(NULL_PTR);
    /* Status should remain uninitialized or trigger error */
}

void test_Fee_DeInit_AfterInit(void) {
    extern const Fee_ConfigType Fee_Config;
    Fee_Init(&Fee_Config);
    Fee_DeInit();
    TEST_ASSERT_EQUAL(FEE_UNINIT, Fee_GetStatus());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Fee_Init_ValidConfig);
    RUN_TEST(test_Fee_Init_NullConfig);
    RUN_TEST(test_Fee_DeInit_AfterInit);
    return UNITY_END();
}
