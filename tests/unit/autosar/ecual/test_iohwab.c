/* 
 * @file test_iohwab.c
 * @brief IOHWAB 模块单元测试
 */

// @tests src/bsw/ecual/iohwab/src/IoHwAb.c  @tests src/bsw/ecual/iohwab/include/IoHwAb.h

#include <unity.h>
#include "iohwab.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_IoHwAb_00001 */
void test_iohwab_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_iohwab_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_iohwab_Init_should_initialize);
    RUN_TEST(test_iohwab_GetVersionInfo_should_return_version);
    return UNITY_END();
}
