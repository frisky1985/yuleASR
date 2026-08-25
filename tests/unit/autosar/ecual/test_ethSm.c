/* 
 * @file test_ethsm.c
 * @brief ETHSM 模块单元测试
 */

#include <unity.h>
#include "ethsm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_EthSM_00001 */
void test_ethsm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

/** @req SWS_EthSM_00003 */
void test_ethsm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ethsm_Init_should_initialize);
    RUN_TEST(test_ethsm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
