/* 
 * @file test_xcp.c
 * @brief XCP 模块单元测试
 */

// @tests src/bsw/services/xcp/src/Xcp.c  @tests src/bsw/services/xcp/include/Xcp.h

#include <unity.h>
#include "xcp.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Xcp_00001 */
void test_xcp_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_xcp_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_xcp_Init_should_initialize);
    RUN_TEST(test_xcp_GetVersionInfo_should_return_version);
    return UNITY_END();
}
