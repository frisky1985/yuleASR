/* 
 * @file test_lintrcv.c
 * @brief LINTRCV 模块单元测试
 */

// @tests src/bsw/ecual/lintrcv/src/LinTrcv.c  @tests src/bsw/ecual/lintrcv/include/LinTrcv.h

#include <unity.h>
#include "lintrcv.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_LinTrcv_00001 */
void test_lintrcv_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_lintrcv_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lintrcv_Init_should_initialize);
    RUN_TEST(test_lintrcv_GetVersionInfo_should_return_version);
    return UNITY_END();
}
