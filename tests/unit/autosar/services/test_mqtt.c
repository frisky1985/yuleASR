/* 
 * @file test_mqtt.c
 * @brief MQTT 模块单元测试
 */

#include <unity.h>
#include "mqtt.h"

void setUp(void) {}
void tearDown(void) {}

void test_mqtt_Init_should_initialize(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

void test_mqtt_GetVersionInfo_should_return_version(void) {
    TEST_IGNORE_MESSAGE("API stub");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_Init_should_initialize);
    RUN_TEST(test_mqtt_GetVersionInfo_should_return_version);
    return UNITY_END();
}
