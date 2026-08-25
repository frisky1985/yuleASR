/* 
 * @file test_wdgm.c
 * @brief WDGM 模块单元测试
 */

// @tests src/bsw/services/wdgm/src/WdgM.c  @tests src/bsw/services/wdgm/include/WdgM.h

#include <unity.h>
#include "wdgm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_WdgM_00001 */
void test_wdgm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

/** @req SWS_WdgM_00020 */
void test_wdgm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_wdgm_Init_should_initialize);
    RUN_TEST(test_wdgm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
