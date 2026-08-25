/* 
 * @file test_schm.c
 * @brief SCHM 模块单元测试
 */

// @tests src/bsw/services/schm/src/SchM.c  @tests src/bsw/services/schm/include/SchM.h

#include <unity.h>
#include "schm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_SchM_00001 */
void test_schm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_schm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_schm_Init_should_initialize);
    RUN_TEST(test_schm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
