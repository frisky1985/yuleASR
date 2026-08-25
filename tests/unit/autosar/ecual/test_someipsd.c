/* 
 * @file test_someipsd.c
 * @brief SOMEIPSD 模块单元测试
 */

#include <unity.h>
#include "someipsd.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_SomeIp_00001 */
void test_someipsd_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_someipsd_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_someipsd_Init_should_initialize);
    RUN_TEST(test_someipsd_GetVersionInfo_should_return_version);
    return UNITY_END();
}
