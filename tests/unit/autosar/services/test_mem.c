/* 
 * @file test_mem.c
 * @brief MEM 模块单元测试
 */

#include <unity.h>
#include "mem.h"

void setUp(void) {}
void tearDown(void) {}

void test_mem_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_mem_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mem_Init_should_initialize);
    RUN_TEST(test_mem_GetVersionInfo_should_return_version);
    return UNITY_END();
}
