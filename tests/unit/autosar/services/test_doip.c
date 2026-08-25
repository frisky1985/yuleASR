/* 
 * @file test_doip.c
 * @brief DOIP 模块单元测试
 */

// @tests src/bsw/services/doip/src/DoIP.c  @tests src/bsw/services/doip/include/DoIP.h

#include <unity.h>
#include "doip.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_DoIP_00001 */
void test_doip_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_doip_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_doip_Init_should_initialize);
    RUN_TEST(test_doip_GetVersionInfo_should_return_version);
    return UNITY_END();
}
