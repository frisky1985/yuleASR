/* 
 * @file test_j1939tp.c
 * @brief J1939TP 模块单元测试
 */

#include <unity.h>
#include "j1939tp.h"

void setUp(void) {}
void tearDown(void) {}

void test_j1939tp_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_j1939tp_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_j1939tp_Init_should_initialize);
    RUN_TEST(test_j1939tp_GetVersionInfo_should_return_version);
    return UNITY_END();
}
