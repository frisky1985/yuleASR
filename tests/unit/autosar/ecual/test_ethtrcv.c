/* 
 * @file test_ethtrcv.c
 * @brief ETHTRCV 模块单元测试
 */

// @tests src/bsw/ecual/ethtrcv/src/EthTrcv.c  @tests src/bsw/ecual/ethtrcv/include/EthTrcv.h

#include <unity.h>
#include "ethtrcv.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_EthTrcv_00001 */
void test_ethtrcv_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_ethtrcv_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ethtrcv_Init_should_initialize);
    RUN_TEST(test_ethtrcv_GetVersionInfo_should_return_version);
    return UNITY_END();
}
