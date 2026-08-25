/* 
 * @file test_linnm.c
 * @brief LINNM 模块单元测试
 */

// @tests src/bsw/ecual/linnm/src/LinNm.c  @tests src/bsw/ecual/linnm/include/LinNm.h

#include <unity.h>
#include "linnm.h"

void setUp(void) {}
void tearDown(void) {}

void test_linnm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_linnm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_linnm_Init_should_initialize);
    RUN_TEST(test_linnm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
