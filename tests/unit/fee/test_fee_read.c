/**
 * @file test_fee_read.c
 * @brief Fee Read Operation Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "Fee.h"

static uint8 test_buffer[256];

void setUp(void) {
    extern const Fee_ConfigType Fee_Config;
    Fee_Init(&Fee_Config);
}

void tearDown(void) {
    Fee_DeInit();
}

void test_Fee_Read_ValidParams(void) {
    memset(test_buffer, 0, sizeof(test_buffer));
    Std_ReturnType result = Fee_Read(0, 0, test_buffer, 32);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Fee_Read_InvalidBlock(void) {
    memset(test_buffer, 0, sizeof(test_buffer));
    Std_ReturnType result = Fee_Read(999, 0, test_buffer, 32);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_Fee_Read_NullBuffer(void) {
    Std_ReturnType result = Fee_Read(0, 0, NULL_PTR, 32);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Fee_Read_ValidParams);
    RUN_TEST(test_Fee_Read_InvalidBlock);
    RUN_TEST(test_Fee_Read_NullBuffer);
    return UNITY_END();
}
