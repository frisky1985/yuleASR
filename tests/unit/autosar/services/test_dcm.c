/* 
 * @file test_dcm.c
 * @brief DCM 模块单元测试
 */

#include <unity.h>
#include "dcm.h"

void setUp(void) {}
void tearDown(void) {}

void test_dcm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

void test_dcm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dcm_Init_should_initialize);
    RUN_TEST(test_dcm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
