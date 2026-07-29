/* 
 * @file test_stbm.c
 * @brief STBM 模块单元测试
 */

#include <unity.h>
#include "stbm.h"

void setUp(void) {}
void tearDown(void) {}

void test_stbm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_stbm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_stbm_Init_should_initialize);
    RUN_TEST(test_stbm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
