/* 
 * @file test_linif.c
 * @brief LINIF 模块单元测试
 */

#include <unity.h>
#include "linif.h"

void setUp(void) {}
void tearDown(void) {}

void test_linif_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_linif_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_linif_Init_should_initialize);
    RUN_TEST(test_linif_GetVersionInfo_should_return_version);
    return UNITY_END();
}
