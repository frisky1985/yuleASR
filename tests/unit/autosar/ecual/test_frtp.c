/* 
 * @file test_frtp.c
 * @brief FRTP 模块单元测试
 */

// @tests src/bsw/ecual/frtp/src/FrTp.c  @tests src/bsw/ecual/frtp/include/FrTp.h

#include <unity.h>
#include "frtp.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_FrTp_00001 */
void test_frtp_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_frtp_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_frtp_Init_should_initialize);
    RUN_TEST(test_frtp_GetVersionInfo_should_return_version);
    return UNITY_END();
}
