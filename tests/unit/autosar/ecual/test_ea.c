/* 
 * @file test_ea.c
 * @brief EA 模块单元测试
 */

#include <unity.h>
#include "ea.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Ea_00001 */
void test_ea_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_ea_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ea_Init_should_initialize);
    RUN_TEST(test_ea_GetVersionInfo_should_return_version);
    return UNITY_END();
}
