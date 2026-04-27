/**
 * @file test_telemetry.c
 * @brief Telemetry模块测试
 */

#include <unity.h>
#include "telemetry.h"
#include "telemetry_events.h"
#include <string.h>

void test_telemetry_init(void);
void test_telemetry_basic_logging(void);
void test_telemetry_level_filtering(void);
void test_telemetry_module_control(void);
void test_telemetry_buffer_management(void);

static uint8_t g_test_buffer[4096];

void test_telemetry_init(void) {
    TelStatus_t status = Tel_Init();
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_NOT_NULL(stats);
    TEST_ASSERT_EQUAL(0, stats->total_events);
}

void test_telemetry_basic_logging(void) {
    Tel_Init();
    
    /* 测试各种事件类型 */
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO));
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogCounter(TEL_MOD_SYS, 0x02, TEL_LEVEL_INFO, 100));
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogState(TEL_MOD_SYS, 0x03, TEL_LEVEL_INFO, 0, 1));
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogMetric(TEL_MOD_SYS, 0x04, TEL_LEVEL_INFO, 5000));
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(4, stats->total_events);
}

void test_telemetry_level_filtering(void) {
    Tel_Init();
    Tel_SetGlobalLevel(TEL_LEVEL_WARNING);
    
    /* INFO 级别应被过滤 */
    TEST_ASSERT_EQUAL(TEL_FILTERED, Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO));
    
    /* WARNING 和 ERROR 应通过 */
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_WARNING));
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_ERROR));
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(2, stats->total_events);
}

void test_telemetry_module_control(void) {
    Tel_Init();
    
    /* 禁用 DDS 模块 */
    Tel_EnableModule(TEL_MOD_DDS, false);
    TEST_ASSERT_EQUAL(TEL_FILTERED, Tel_LogInstant(TEL_MOD_DDS, 0x01, TEL_LEVEL_INFO));
    
    /* SYS 模块仍然可用 */
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO));
    
    /* 重新启用 */
    Tel_EnableModule(TEL_MOD_DDS, true);
    TEST_ASSERT_EQUAL(TEL_OK, Tel_LogInstant(TEL_MOD_DDS, 0x01, TEL_LEVEL_INFO));
}

void test_telemetry_buffer_management(void) {
    Tel_Init();
    
    /* 添加事件并读取 */
    Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    
    uint16_t read_len = 0;
    memset(g_test_buffer, 0, sizeof(g_test_buffer));
    
    TelStatus_t status = Tel_ReadEvents(g_test_buffer, sizeof(g_test_buffer), &read_len);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    TEST_ASSERT_GREATER_THAN(0, read_len);
    
    /* 清空后验证 */
    Tel_ClearBuffer();
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(0, stats->current_usage);
}

void test_telemetry_suite(void) {
    RUN_TEST(test_telemetry_init);
    RUN_TEST(test_telemetry_basic_logging);
    RUN_TEST(test_telemetry_level_filtering);
    RUN_TEST(test_telemetry_module_control);
    RUN_TEST(test_telemetry_buffer_management);
}
