/* 
 * @file test_swc.c
 * @brief SWC 模块单元测试
 */

// @tests src/bsw/services/swc/src/Swc.c  @tests src/bsw/services/swc/include/Swc.h

#include <unity.h>
#include "swc.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Swc_00001 */
void test_swc_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_swc_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_swc_Init_should_initialize);
    RUN_TEST(test_swc_GetVersionInfo_should_return_version);
    return UNITY_END();
}
