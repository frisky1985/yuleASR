/* 
 * @file test_ecuc.c
 * @brief ECUC 模块单元测试
 */

// @tests src/bsw/services/ecuc/src/EcuC.c  @tests src/bsw/services/ecuc/include/EcuC.h

#include <unity.h>
#include "ecuc.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_EcuC_00001 */
void test_ecuc_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_ecuc_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ecuc_Init_should_initialize);
    RUN_TEST(test_ecuc_GetVersionInfo_should_return_version);
    return UNITY_END();
}
