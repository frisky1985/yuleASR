/* 
 * @file test_docan.c
 * @brief DOCAN 模块单元测试
 */

// @tests src/bsw/services/docan/src/DoCan.c  @tests src/bsw/services/docan/include/DoCan.h

#include <unity.h>
#include "docan.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_DoCan_00001 */
void test_docan_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_docan_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_docan_Init_should_initialize);
    RUN_TEST(test_docan_GetVersionInfo_should_return_version);
    return UNITY_END();
}
