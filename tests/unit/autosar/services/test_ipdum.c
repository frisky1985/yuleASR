/* 
 * @file test_ipdum.c
 * @brief IPDUM 模块单元测试
 */

// @tests src/bsw/services/ipdum/src/IpduM.c  @tests src/bsw/services/ipdum/include/IpduM.h

#include <unity.h>
#include "ipdum.h"

void setUp(void) {}
void tearDown(void) {}

void test_ipdum_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

void test_ipdum_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ipdum_Init_should_initialize);
    RUN_TEST(test_ipdum_GetVersionInfo_should_return_version);
    return UNITY_END();
}
