/**
 * @file test_fee_write.c
 * @brief Fee Write Operation Tests
 */

#include "unity.h"
#include "test_framework.h"
#include "Fee.h"

static const uint8 test_data[32] = {0x01, 0x02, 0x03, 0x04};

void setUp(void) {
    extern const Fee_ConfigType Fee_Config;
    Fee_Init(&Fee_Config);
}

void tearDown(void) {
    Fee_DeInit();
}

void test_Fee_Write_ValidParams(void) {
    Std_ReturnType result = Fee_Write(0, test_data);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_Fee_Write_InvalidBlock(void) {
    Std_ReturnType result = Fee_Write(999, test_data);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_Fee_Write_NullData(void) {
    Std_ReturnType result = Fee_Write(0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Fee_Write_ValidParams);
    RUN_TEST(test_Fee_Write_InvalidBlock);
    RUN_TEST(test_Fee_Write_NullData);
    return UNITY_END();
}
