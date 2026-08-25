/* 
 * @file test_ramsafety.c
 * @brief RAMSAFETY 模块单元测试
 */

// @tests src/bsw/services/ramsafety/src/RamSafety.c  @tests src/bsw/services/ramsafety/include/RamSafety.h

#include <unity.h>
#include "ramsafety.h"

void setUp(void) {}
void tearDown(void) {}

void test_ramsafety_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_ramsafety_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ramsafety_Init_should_initialize);
    RUN_TEST(test_ramsafety_GetVersionInfo_should_return_version);
    return UNITY_END();
}
