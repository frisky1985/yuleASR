/* 
 * @file test_linsm.c
 * @brief LINSM 模块单元测试
 */

// @tests src/bsw/services/linsm/src/LinSM.c  @tests src/bsw/services/linsm/include/LinSM.h

#include <unity.h>
#include "linsm.h"

void setUp(void) {}
void tearDown(void) {}

void test_linsm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_linsm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_linsm_Init_should_initialize);
    RUN_TEST(test_linsm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
