/* 
 * @file test_lintp.c
 * @brief LINTP 模块单元测试
 */

// @tests src/bsw/ecual/lintp/src/LinTp.c  @tests src/bsw/ecual/lintp/include/LinTp.h

#include <unity.h>
#include "lintp.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_LinTp_00001 */
void test_lintp_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_lintp_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_lintp_Init_should_initialize);
    RUN_TEST(test_lintp_GetVersionInfo_should_return_version);
    return UNITY_END();
}
