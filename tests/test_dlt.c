/**
 * @file test_dlt.c
 * @brief DLT模块测试
 */

#include <unity.h>
#include "dlt.h"
#include <string.h>

void test_dlt_init(void);
void test_dlt_context_management(void);
void test_dlt_logging(void);
void test_dlt_log_levels(void);
void test_dlt_trace(void);
void test_dlt_statistics(void);

static Dlt_ContextType g_test_ctx;

void test_dlt_init(void) {
    /* 测试未初始化时调用 */
    TEST_ASSERT_FALSE(Dlt_IsInitialized());
    
    /* 初始化 */
    Dlt_ReturnType ret = Dlt_Init(NULL);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    TEST_ASSERT_TRUE(Dlt_IsInitialized());
    
    /* 重复初始化应失败 */
    ret = Dlt_Init(NULL);
    TEST_ASSERT_EQUAL(DLT_RETURN_ERROR, ret);
    
    /* 反初始化 */
    Dlt_DeInit();
    TEST_ASSERT_FALSE(Dlt_IsInitialized());
}

void test_dlt_context_management(void) {
    Dlt_Init(NULL);
    
    /* 注册上下文 */
    Dlt_ReturnType ret = Dlt_RegisterContext(&g_test_ctx, "TEST", "UNIT", "Test Context");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    TEST_ASSERT_TRUE(Dlt_IsContextRegistered(&g_test_ctx));
    
    /* 验证AppID和ContextID已设置 */
    TEST_ASSERT_NOT_EQUAL(0, g_test_ctx.app_id);
    TEST_ASSERT_NOT_EQUAL(0, g_test_ctx.context_id);
    
    /* 设置日志级别 */
    ret = Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_DEBUG);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    TEST_ASSERT_EQUAL(DLT_LOG_DEBUG, g_test_ctx.log_level);
    
    /* 注销上下文 */
    ret = Dlt_UnregisterContext(&g_test_ctx);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    Dlt_DeInit();
}

void test_dlt_logging(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "LOG", "Logging Test");
    
    /* 测试基础日志 */
    Dlt_ReturnType ret = Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Test message");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试格式化日志 */
    ret = Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, "Value: %d", 42);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试数据日志 */
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    ret = Dlt_LogData(&g_test_ctx, DLT_LOG_INFO, test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    Dlt_DeInit();
}

void test_dlt_log_levels(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "LVL", "Level Test");
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_WARNING);
    
    /* DEBUG级别应被过滤 */
    Dlt_ReturnType ret = Dlt_LogString(&g_test_ctx, DLT_LOG_DEBUG, "Debug message");
    TEST_ASSERT_EQUAL(DLT_RETURN_LOGGING_DISABLED, ret);
    
    /* WARNING级别应通过 */
    ret = Dlt_LogString(&g_test_ctx, DLT_LOG_WARNING, "Warning message");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* ERROR级别应通过 */
    ret = Dlt_LogString(&g_test_ctx, DLT_LOG_ERROR, "Error message");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    Dlt_DeInit();
}

void test_dlt_trace(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "TRC", "Trace Test");
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_DEBUG);
    
    /* 测试函数追踪 */
    Dlt_ReturnType ret = Dlt_TraceFunction(&g_test_ctx, DLT_TRACE_FUNCTION_IN, "test_func");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    ret = Dlt_TraceFunction(&g_test_ctx, DLT_TRACE_FUNCTION_OUT, "test_func");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试变量追踪 */
    uint32_t test_var = 0x12345678;
    ret = Dlt_TraceVariable(&g_test_ctx, "test_var", &test_var, sizeof(test_var));
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试网络追踪 */
    uint8_t eth_frame[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ret = Dlt_TraceNetwork(&g_test_ctx, DLT_NW_TRACE_ETHERNET, eth_frame, sizeof(eth_frame));
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    Dlt_DeInit();
}

void test_dlt_statistics(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "STAT", "Stats Test");
    
    /* 初始统计应为0 */
    const Dlt_StatisticsType *stats = Dlt_GetStatistics();
    TEST_ASSERT_NOT_NULL(stats);
    TEST_ASSERT_EQUAL(0, stats->messages_sent);
    TEST_ASSERT_EQUAL(0, stats->messages_dropped);
    
    /* 发送一些消息 */
    Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Message 1");
    Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Message 2");
    
    /* 验证统计更新 */
    stats = Dlt_GetStatistics();
    TEST_ASSERT(stats->messages_sent >= 2);
    
    /* 重置统计 */
    Dlt_ResetStatistics();
    stats = Dlt_GetStatistics();
    TEST_ASSERT_EQUAL(0, stats->messages_sent);
    
    Dlt_DeInit();
}

void test_dlt_suite(void) {
    RUN_TEST(test_dlt_init);
    RUN_TEST(test_dlt_context_management);
    RUN_TEST(test_dlt_logging);
    RUN_TEST(test_dlt_log_levels);
    RUN_TEST(test_dlt_trace);
    RUN_TEST(test_dlt_statistics);
}
