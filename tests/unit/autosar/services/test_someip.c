/* 
 * @file test_someip.c
 * @brief SOMEIP 模块单元测试
 */

// @tests src/bsw/services/someip/src/SomeIp.c  @tests src/bsw/services/someip/include/SomeIp.h

#include <unity.h>
#include "someip.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_SomeIp_00001 */
void test_someip_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_someip_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_someip_Init_should_initialize);
    RUN_TEST(test_someip_GetVersionInfo_should_return_version);
    return UNITY_END();
}
