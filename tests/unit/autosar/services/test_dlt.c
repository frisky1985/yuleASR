/* 
 * @file test_dlt.c
 * @brief DLT 模块单元测试
 */

// @tests src/bsw/services/dlt/src/Dlt.c  @tests src/bsw/services/dlt/include/Dlt.h

#include <unity.h>
#include "dlt.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Dlt_00001 */
void test_dlt_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

void test_dlt_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dlt_Init_should_initialize);
    RUN_TEST(test_dlt_GetVersionInfo_should_return_version);
    return UNITY_END();
}
