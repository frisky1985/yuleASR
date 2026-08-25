/* 
 * @file test_fee.c
 * @brief FEE 模块单元测试
 */

// @tests src/bsw/ecual/fee/src/Fee.c  @tests src/bsw/ecual/fee/include/Fee.h

#include <unity.h>
#include "fee.h"

void setUp(void) {}
void tearDown(void) {}

void test_fee_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_fee_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fee_Init_should_initialize);
    RUN_TEST(test_fee_GetVersionInfo_should_return_version);
    return UNITY_END();
}
