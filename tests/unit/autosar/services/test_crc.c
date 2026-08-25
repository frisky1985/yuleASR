/* 
 * @file test_crc.c
 * @brief CRC 模块单元测试
 */

#include <unity.h>
#include "crc.h"

void setUp(void) {}
void tearDown(void) {}

/** @req SWS_Crc_00001 */
void test_crc_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_crc_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_crc_Init_should_initialize);
    RUN_TEST(test_crc_GetVersionInfo_should_return_version);
    return UNITY_END();
}
