/* 
 * @file test_someipif.c
 * @brief SOMEIPIF 模块单元测试
 */

#include <unity.h>
#include "someipif.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_SomeIp_00001 */
void test_someipif_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_someipif_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_someipif_Init_should_initialize);
    RUN_TEST(test_someipif_GetVersionInfo_should_return_version);
    return UNITY_END();
}
