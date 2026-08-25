/* 
 * @file test_j1939nm.c
 * @brief J1939NM 模块单元测试
 */

#include <unity.h>
#include "j1939nm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_J1939Nm_00001 */
void test_j1939nm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_j1939nm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_j1939nm_Init_should_initialize);
    RUN_TEST(test_j1939nm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
