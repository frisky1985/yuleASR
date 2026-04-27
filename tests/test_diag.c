/**
 * @file test_diag.c
 * @brief 诊断集成测试
 */

#include <unity.h>
#include "telemetry_diag.h"
#include "telemetry.h"
#include <string.h>

void test_diag_read_status(void);
void test_diag_read_stats(void);
void test_diag_write_control(void);
void test_diag_did_handling(void);

static uint8_t g_read_buffer[256];

void test_diag_read_status(void) {
    Tel_Init();
    Tel_Diag_Init();
    
    uint16_t actual_len = 0;
    memset(g_read_buffer, 0, sizeof(g_read_buffer));
    
    Std_ReturnType result = Tel_Diag_ReadData(DID_TEL_STATUS, g_read_buffer,
                                              sizeof(g_read_buffer), &actual_len);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(sizeof(TelDiagStatus_t), actual_len);
    
    TelDiagStatus_t *status = (TelDiagStatus_t*)g_read_buffer;
    TEST_ASSERT_EQUAL(1, status->enabled);
    TEST_ASSERT_EQUAL(TEL_BUFFER_SIZE, status->buffer_size);
}

void test_diag_read_stats(void) {
    Tel_Init();
    Tel_Diag_Init();
    
    /* 添加一些事件 */
    Tel_LogInstant(TEL_MOD_SYS, 0x01, TEL_LEVEL_INFO);
    Tel_LogInstant(TEL_MOD_SYS, 0x02, TEL_LEVEL_INFO);
    
    uint16_t actual_len = 0;
    memset(g_read_buffer, 0, sizeof(g_read_buffer));
    
    Std_ReturnType result = Tel_Diag_ReadData(DID_TEL_STATS, g_read_buffer,
                                              sizeof(g_read_buffer), &actual_len);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(sizeof(TelDiagStats_t), actual_len);
    
    TelDiagStats_t *stats = (TelDiagStats_t*)g_read_buffer;
    TEST_ASSERT_EQUAL(2, stats->total_events);
}

void test_diag_write_control(void) {
    Tel_Init();
    Tel_Diag_Init();
    
    /* 禁用埋点 */
    uint8_t control_data = 0;
    Std_ReturnType result = Tel_Diag_WriteData(DID_TEL_CONTROL, &control_data, 1);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* 设置日志级别 */
    uint8_t level_data = TEL_LEVEL_WARNING;
    result = Tel_Diag_WriteData(DID_TEL_SET_LEVEL, &level_data, 1);
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_diag_did_handling(void) {
    Tel_Init();
    Tel_Diag_Init();
    
    /* 测试无效DID */
    uint16_t actual_len = 0;
    Std_ReturnType result = Tel_Diag_ReadData(0xFFFF, g_read_buffer,
                                              sizeof(g_read_buffer), &actual_len);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    
    /* 测试缓冲区不足 */
    result = Tel_Diag_ReadData(DID_TEL_STATUS, g_read_buffer, 1, &actual_len);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_diag_suite(void) {
    RUN_TEST(test_diag_read_status);
    RUN_TEST(test_diag_read_stats);
    RUN_TEST(test_diag_write_control);
    RUN_TEST(test_diag_did_handling);
}
