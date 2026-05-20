/* 
 * @file test_cryif.c
 * @brief CRYIF 模块单元测试
 */

#include <unity.h>
#include "cryif.h"

void setUp(void) {}
void tearDown(void) {}

void test_cryif_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_cryif_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cryif_Init_should_initialize);
    RUN_TEST(test_cryif_GetVersionInfo_should_return_version);
    return UNITY_END();
}
