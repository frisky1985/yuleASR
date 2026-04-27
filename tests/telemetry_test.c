/*
 * @file telemetry_test.c
 * @brief Telemetry模块单元测试
 */

#include <unity.h>
#include "telemetry.h"
#include "telemetry_events.h"
#include <string.h>
#include <stdio.h>

static uint8_t test_buffer[4096];
static uint16_t test_buffer_len = 0;

/* 测试setup/teardown */
void setUp(void) {
    Tel_Init();
    test_buffer_len = 0;
    memset(test_buffer, 0, sizeof(test_buffer));
}

void tearDown(void) {
    /* 清理 */
}

/* 测试基本初始化 */
void test_telemetry_init(void) {
    TelStatus_t status = Tel_Init();
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_NOT_NULL(stats);
    TEST_ASSERT_EQUAL(0, stats->total_events);
}

/* 测试instant事件 */
void test_telemetry_log_instant(void) {
    TelStatus_t status = Tel_LogInstant(TEL_MOD_SYS, TEL_EVT_SYS_BOOT, TEL_LEVEL_INFO);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(1, stats->total_events);
}

/* 测试counter事件 */
void test_telemetry_log_counter(void) {
    uint16_t test_values[] = {0, 1, 255, 256, 65535};
    
    for (int i = 0; i < 5; i++) {
        TelStatus_t status = Tel_LogCounter(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, test_values[i]);
        TEST_ASSERT_EQUAL(TEL_OK, status);
    }
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(5, stats->total_events);
}

/* 测试state事件 */
void test_telemetry_log_state(void) {
    TelStatus_t status = Tel_LogState(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, 0, 1);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* 测试边界值 */
    status = Tel_LogState(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, 0xFF, 0x00);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(2, stats->total_events);
}

/* 测试metric事件 */
void test_telemetry_log_metric(void) {
    TelStatus_t status = Tel_LogMetric(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, 1000);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* 测试负值 */
    status = Tel_LogMetric(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO, -1000);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(2, stats->total_events);
}

/* 测试日志级别过滤 */
void test_telemetry_level_filtering(void) {
    Tel_SetGlobalLevel(TEL_LEVEL_WARNING);
    
    /* INFO级别应被过滤 */
    TelStatus_t status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    TEST_ASSERT_EQUAL(TEL_FILTERED, status);
    
    /* WARNING级别应被接受 */
    status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_WARNING);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* ERROR级别应被接受 */
    status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_ERROR);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(2, stats->total_events);
}

/* 测试模块使能控制 */
void test_telemetry_module_enable(void) {
    /* 禁用某个模块 */
    Tel_EnableModule(TEL_MOD_SYS, false);
    
    TelStatus_t status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    TEST_ASSERT_EQUAL(TEL_FILTERED, status);
    
    /* 启用模块 */
    Tel_EnableModule(TEL_MOD_SYS, true);
    status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    TEST_ASSERT_EQUAL(TEL_OK, status);
}

/* 测试缓冲区溢出 */
void test_telemetry_buffer_overflow(void) {
    /* 填满缓冲区 */
    for (int i = 0; i < 10000; i++) {
        Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    }
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT(stats->overflow_cnt > 0);
}

/* 测试读取事件 */
void test_telemetry_read_events(void) {
    /* 添加几个事件 */
    Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    Tel_LogCounter(TEL_MOD_SYS, 0x02, TEL_LEVEL_INFO, 42);
    Tel_LogState(TEL_MOD_SYS, 0x03, TEL_LEVEL_INFO, 0, 1);
    
    uint16_t read_len = 0;
    TelStatus_t status = Tel_ReadEvents(test_buffer, sizeof(test_buffer), &read_len);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    TEST_ASSERT(read_len > 0);
}

/* 测试清空缓冲区 */
void test_telemetry_clear_buffer(void) {
    Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    Tel_ClearBuffer();
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(0, stats->current_usage);
}

/* 测试便捷宏 */
void test_telemetry_macros(void) {
    /* 这些宏在TEL_SYS_ENABLED为1时应该能正常工作 */
    #if TEL_SYS_ENABLED
    TEL_SYS_INSTANT(0x01);
    TEL_SYS_COUNTER(0x02, 100);
    TEL_SYS_STATE(0x03, 0, 1);
    TEL_SYS_METRIC(0x04, 1000);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(4, stats->total_events);
    #endif
}

/* 测试NULL参数 */
void test_telemetry_null_params(void) {
    /* 测试ReadEvents的NULL参数 */
    TelStatus_t status = Tel_ReadEvents(NULL, 100, NULL);
    TEST_ASSERT_EQUAL(TEL_ERR_PARAM, status);
    
    status = Tel_ReadEvents(test_buffer, 100, NULL);
    TEST_ASSERT_EQUAL(TEL_ERR_PARAM, status);
}

/* 测试最大事件数 */
void test_telemetry_max_events(void) {
    uint32_t max_events = 65535;
    Tel_SetMaxEvents(max_events);
    
    /* 添加一个事件应该正常 */
    TelStatus_t status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* 设置最大事件数为0应该禁用埋点 */
    Tel_SetMaxEvents(0);
    status = Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    TEST_ASSERT_EQUAL(TEL_ERR_PARAM, status);
}

/* 测试时间戳 */
void test_telemetry_timestamps(void) {
    TelConfig_t config = {
        .timestamp_src = TEL_TS_OS,
        .output_mode = TEL_MODE_BUFFER
    };
    Tel_Configure(&config);
    
    Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    
    /* 验证时间戳是否在合理范围 */
    /* 注: 这需要实际读取事件数据来验证 */
}

/* 主函数 */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_telemetry_init);
    RUN_TEST(test_telemetry_log_instant);
    RUN_TEST(test_telemetry_log_counter);
    RUN_TEST(test_telemetry_log_state);
    RUN_TEST(test_telemetry_log_metric);
    RUN_TEST(test_telemetry_level_filtering);
    RUN_TEST(test_telemetry_module_enable);
    RUN_TEST(test_telemetry_buffer_overflow);
    RUN_TEST(test_telemetry_read_events);
    RUN_TEST(test_telemetry_clear_buffer);
    RUN_TEST(test_telemetry_macros);
    RUN_TEST(test_telemetry_null_params);
    RUN_TEST(test_telemetry_max_events);
    RUN_TEST(test_telemetry_timestamps);
    
    return UNITY_END();
}
