/* 
 * @file test_dcm.c
 * @brief DCM 模块单元测试
 */

// @tests src/bsw/services/dcm/src/Dcm.c  @tests src/bsw/services/dcm/include/Dcm.h

#include <unity.h>
#include "dcm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Dcm_00001 */
void test_dcm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

/** @req SWS_Dcm_00010 */
void test_dcm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dcm_Init_should_initialize);
    RUN_TEST(test_dcm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
