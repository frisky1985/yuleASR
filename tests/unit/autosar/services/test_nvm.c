/* 
 * @file test_nvm.c
 * @brief NVM 模块单元测试
 */

// @tests src/bsw/services/nvm/src/NvM.c  @tests src/bsw/services/nvm/include/NvM.h

#include <unity.h>
#include "nvm.h"

void setUp(void) {}
void tearDown(void) {}

void test_nvm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

void test_nvm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub - needs implementation");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nvm_Init_should_initialize);
    RUN_TEST(test_nvm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
