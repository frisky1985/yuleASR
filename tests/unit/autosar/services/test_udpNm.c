/* 
 * @file test_udpnm.c
 * @brief UDPNM 模块单元测试
 */

#include <unity.h>
#include "udpnm.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_UdpNm_00001 */
void test_udpnm_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

/** @req SWS_UdpNm_00012 */
void test_udpnm_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_udpnm_Init_should_initialize);
    RUN_TEST(test_udpnm_GetVersionInfo_should_return_version);
    return UNITY_END();
}
