/* 
 * @file test_someiptp.c
 * @brief SOMEIPTP 模块单元测试
 */

// @tests src/bsw/services/someiptp/src/SomeIpTp.c  @tests src/bsw/services/someiptp/include/SomeIpTp.h

#include <unity.h>
#include "someiptp.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_SomeIp_00001 */
void test_someiptp_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_someiptp_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_someiptp_Init_should_initialize);
    RUN_TEST(test_someiptp_GetVersionInfo_should_return_version);
    return UNITY_END();
}
