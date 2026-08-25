/* 
 * @file test_det.c
 * @brief DET 模块单元测试
 */

#include <unity.h>
#include "det.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Det_00001 */
void test_det_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

/** @req SWS_Det_00006 */
void test_det_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_det_Init_should_initialize);
    RUN_TEST(test_det_GetVersionInfo_should_return_version);
    return UNITY_END();
}
