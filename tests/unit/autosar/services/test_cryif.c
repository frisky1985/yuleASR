/* 
 * @file test_cryif.c
 * @brief CRYIF 模块单元测试
 */

// @tests src/bsw/services/cryif/src/CryIf.c  @tests src/bsw/services/cryif/include/CryIf.h

#include <unity.h>
#include "cryif.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_CryIf_00001 */
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
