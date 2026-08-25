/* 
 * @file test_linm.c
 * @brief LINM 模块单元测试
 */

// @tests src/bsw/services/linm/src/LinM.c  @tests src/bsw/services/linm/include/LinM.h

#include <unity.h>
#include "linm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_LinM_00001 */
void test_linm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_linm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_linm_Init_should_initialize);
    RUN_TEST(test_linm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
