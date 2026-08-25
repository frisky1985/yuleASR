/* 
 * @file test_lntm.c
 * @brief LNTM 模块单元测试
 */

// @tests src/bsw/ecual/linTp/src/LinTp.c  @tests src/bsw/ecual/linTp/include/LinTp.h

#include <unity.h>
#include "lntm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Tm_00001 */
void test_lntm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_lntm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lntm_Init_should_initialize);
    RUN_TEST(test_lntm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
