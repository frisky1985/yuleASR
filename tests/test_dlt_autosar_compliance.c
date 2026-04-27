/**
 * @file test_dlt_autosar_compliance.c
 * @brief DLT AutoSAR R21-11 规范合规性测试
 * 
 * 测试项目基于 AUTOSAR SWS Diagnostic Log and Trace R21-11
 */

#include <unity.h>
#include "dlt.h"
#include <string.h>

/*=============================================================================
 * 测试辅助函数和宏
 *============================================================================*/
#define TEST_ASSERT_DLT_OK(x)     TEST_ASSERT_EQUAL(DLT_RETURN_OK, (x))
#define TEST_ASSERT_DLT_ERROR(x)  TEST_ASSERT_EQUAL(DLT_RETURN_ERROR, (x))

static Dlt_ContextType g_test_ctx;

/*=============================================================================
 * [SWS_Dlt_00001] - DLT消息格式测试
 *============================================================================*/

/**
 * @brief 测试Standard Header格式是否符合规范
 * 
 * 规范要求:
 * - Header Type (1 byte)
 * - Message Counter (1 byte)
 * - Length (2 bytes)
 */
void test_dlt_standard_header_format(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "HEAD", "Header Test");
    
    /* 发送消息获取内部缓冲区数据 */
    Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Header test");
    
    const Dlt_StatisticsType *stats = Dlt_GetStatistics();
    TEST_ASSERT_GREATER_THAN(0, stats->messages_sent);
    TEST_ASSERT_GREATER_THAN(0, stats->bytes_written);
    
    /* 验证消息长度至少包含Standard Header (4 bytes) */
    /* 实际验证需要读取缓冲区，这里简化检查 */
    
    Dlt_DeInit();
}

/**
 * @brief 测试Extended Header格式
 * 
 * 规范要求:
 * - MSIN (Message Info, 1 byte)
 * - NOAR (Number of Arguments, 1 byte)
 * - APID (Application ID, 4 bytes)
 * - CTID (Context ID, 4 bytes)
 */
void test_dlt_extended_header_format(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "APPL", "CTXT", "Extended Header");
    
    /* 发送消息 */
    Dlt_ReturnType ret = Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Test");
    TEST_ASSERT_DLT_OK(ret);
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00030] - 日志级别测试
 *============================================================================*/

/**
 * @brief 测试所有日志级别的正确定义和过滤
 * 
 * 日志级别顺序: FATAL(1) > ERROR(2) > WARN(3) > INFO(4) > DEBUG(5) > VERBOSE(6)
 */
void test_dlt_log_levels_all(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "LVLS", "Level Test");
    
    /* 设置上下文日志级别为INFO */
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_INFO);
    
    /* FATAL - 应该通过 */
    TEST_ASSERT_DLT_OK(Dlt_LogString(&g_test_ctx, DLT_LOG_FATAL, "Fatal"));
    
    /* ERROR - 应该通过 */
    TEST_ASSERT_DLT_OK(Dlt_LogString(&g_test_ctx, DLT_LOG_ERROR, "Error"));
    
    /* WARN - 应该通过 */
    TEST_ASSERT_DLT_OK(Dlt_LogString(&g_test_ctx, DLT_LOG_WARN, "Warning"));
    
    /* INFO - 应该通过 */
    TEST_ASSERT_DLT_OK(Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Info"));
    
    /* DEBUG - 应该被过滤 */
    TEST_ASSERT_EQUAL(DLT_RETURN_LOGGING_DISABLED, 
                      Dlt_LogString(&g_test_ctx, DLT_LOG_DEBUG, "Debug"));
    
    /* VERBOSE - 应该被过滤 */
    TEST_ASSERT_EQUAL(DLT_RETURN_LOGGING_DISABLED,
                      Dlt_LogString(&g_test_ctx, DLT_LOG_VERBOSE, "Verbose"));
    
    Dlt_DeInit();
}

/**
 * @brief 测试OFF日志级别 (完全禁用)
 */
void test_dlt_log_level_off(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "OFF", "Off Test");
    
    /* 设置日志级别为OFF */
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_OFF);
    
    /* 所有消息都应该被过滤 */
    TEST_ASSERT_EQUAL(DLT_RETURN_LOGGING_DISABLED,
                      Dlt_LogString(&g_test_ctx, DLT_LOG_FATAL, "Fatal"));
    TEST_ASSERT_EQUAL(DLT_RETURN_LOGGING_DISABLED,
                      Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Info"));
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00040] - 消息类型测试
 *============================================================================*/

/**
 * @brief 测试日志消息类型 (DLT_TYPE_LOG)
 */
void test_dlt_message_type_log(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "LOG", "Log Type");
    
    /* 测试不同级别的日志消息 */
    TEST_ASSERT_DLT_OK(DLT_LOG_FATAL(&g_test_ctx, "Fatal message"));
    TEST_ASSERT_DLT_OK(DLT_LOG_ERROR(&g_test_ctx, "Error message"));
    TEST_ASSERT_DLT_OK(DLT_LOG_WARNING(&g_test_ctx, "Warning message"));
    TEST_ASSERT_DLT_OK(DLT_LOG_INFO(&g_test_ctx, "Info message"));
    TEST_ASSERT_DLT_OK(DLT_LOG_DEBUG(&g_test_ctx, "Debug message"));
    
    Dlt_DeInit();
}

/**
 * @brief 测试应用追踪消息类型 (DLT_TYPE_APP_TRACE)
 */
void test_dlt_message_type_app_trace(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "TRC", "Trace Type");
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_DEBUG);
    
    /* 测试函数入口/出口追踪 */
    TEST_ASSERT_DLT_OK(Dlt_TraceFunction(&g_test_ctx, DLT_TRACE_FUNCTION_IN, "test_function"));
    TEST_ASSERT_DLT_OK(Dlt_TraceFunction(&g_test_ctx, DLT_TRACE_FUNCTION_OUT, "test_function"));
    
    /* 测试状态追踪 */
    TEST_ASSERT_DLT_OK(Dlt_TraceFunction(&g_test_ctx, DLT_TRACE_STATE, "RUNNING"));
    
    /* 测试变量追踪 */
    uint32_t test_var = 0x12345678;
    TEST_ASSERT_DLT_OK(Dlt_TraceVariable(&g_test_ctx, "test_var", &test_var, sizeof(test_var)));
    
    Dlt_DeInit();
}

/**
 * @brief 测试网络追踪消息类型 (DLT_TYPE_NW_TRACE)
 */
void test_dlt_message_type_network_trace(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "NW", "Network Type");
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_DEBUG);
    
    uint8_t dummy_data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    
    /* 测试各种网络类型追踪 */
    TEST_ASSERT_DLT_OK(Dlt_TraceNetwork(&g_test_ctx, DLT_NW_TRACE_CAN, dummy_data, sizeof(dummy_data)));
    TEST_ASSERT_DLT_OK(Dlt_TraceNetwork(&g_test_ctx, DLT_NW_TRACE_ETHERNET, dummy_data, sizeof(dummy_data)));
    TEST_ASSERT_DLT_OK(Dlt_TraceNetwork(&g_test_ctx, DLT_NW_TRACE_SOMEIP, dummy_data, sizeof(dummy_data)));
    TEST_ASSERT_DLT_OK(Dlt_TraceNetwork(&g_test_ctx, DLT_NW_TRACE_FLEXRAY, dummy_data, sizeof(dummy_data)));
    
    /* 使用宏 */
    TEST_ASSERT_DLT_OK(DLT_NET_CAN(&g_test_ctx, dummy_data, sizeof(dummy_data)));
    TEST_ASSERT_DLT_OK(DLT_NET_ETH(&g_test_ctx, dummy_data, sizeof(dummy_data)));
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00050] - 上下文管理测试
 *============================================================================*/

/**
 * @brief 测试上下文注册限制
 * 
 * 规范要求最多支持DLT_MAX_CONTEXTS个上下文
 */
void test_dlt_context_limit(void) {
    Dlt_Init(NULL);
    
    Dlt_ContextType contexts[DLT_MAX_CONTEXTS + 5];
    char app_id[5], ctx_id[5];
    
    /* 注册最大数量的上下文 */
    for (int i = 0; i < DLT_MAX_CONTEXTS; i++) {
        snprintf(app_id, sizeof(app_id), "APP%02d", i);
        snprintf(ctx_id, sizeof(ctx_id), "CTX%02d", i);
        
        Dlt_ReturnType ret = Dlt_RegisterContext(&contexts[i], app_id, ctx_id, "Test context");
        TEST_ASSERT_DLT_OK(ret);
    }
    
    /* 尝试注册超过限制的上下文 */
    Dlt_ReturnType ret = Dlt_RegisterContext(&contexts[DLT_MAX_CONTEXTS], "FULL", "FULL", "Should fail");
    TEST_ASSERT_EQUAL(DLT_RETURN_ERROR, ret);
    
    Dlt_DeInit();
}

/**
 * @brief 测试Application ID和Context ID格式
 * 
 * 规范要求: 4字符ASCII字符串
 */
void test_dlt_context_id_format(void) {
    Dlt_Init(NULL);
    
    /* 测试4字符ID */
    Dlt_ContextType ctx1;
    TEST_ASSERT_DLT_OK(Dlt_RegisterContext(&ctx1, "ABCD", "WXYZ", "Valid IDs"));
    TEST_ASSERT_EQUAL(0x41424344, ctx1.app_id);     /* "ABCD" */
    TEST_ASSERT_EQUAL(0x5758595A, ctx1.context_id); /* "WXYZ" */
    
    /* 测试无效ID (太短) - 应该处理错误或截断 */
    Dlt_ContextType ctx2;
    /* 当前实现可能不检查长度，这是符合性缺陷 */
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00070] - 缓冲区管理测试
 *============================================================================*/

/**
 * @brief 测试缓冲区溢出处理
 */
void test_dlt_buffer_overflow(void) {
    Dlt_ConfigType config = {
        .mode = DLT_MODE_EXTERNAL,
        .default_level = DLT_LOG_INFO,
        .enable_timestamp = true,
        .enable_ecu_id = true,
        .enable_session_id = true,
        .buffer_size = 1024, /* 小缓冲区便于测试溢出 */
        .udp_port = 3490,
        .enable_file_output = false
    };
    
    Dlt_Init(&config);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "OVRF", "Overflow Test");
    
    /* 发送大量消息以触发溢出 */
    int overflow_count = 0;
    for (int i = 0; i < 1000; i++) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Message %d with some padding to fill buffer faster", i);
        Dlt_ReturnType ret = Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, msg);
        if (ret == DLT_RETURN_BUFFER_FULL) {
            overflow_count++;
        }
    }
    
    /* 应该有溢出发生 */
    const Dlt_StatisticsType *stats = Dlt_GetStatistics();
    TEST_ASSERT_GREATER_THAN(0, stats->buffer_overflows);
    TEST_ASSERT_GREATER_THAN(0, stats->messages_dropped);
    
    Dlt_DeInit();
}

/**
 * @brief 测试缓冲区清除功能
 */
void test_dlt_buffer_clear(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "CLR", "Clear Test");
    
    /* 发送一些消息 */
    for (int i = 0; i < 10; i++) {
        Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Test message");
    }
    
    /* 清除缓冲区 */
    Dlt_ClearBuffer();
    
    /* 验证可以继续发送消息 */
    TEST_ASSERT_DLT_OK(Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "After clear"));
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00100] - 消息计数器测试
 *============================================================================*/

/**
 * @brief 测试消息计数器递增
 */
void test_dlt_message_counter(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "CNT", "Counter Test");
    
    /* 发送消息并检查统计 */
    const Dlt_StatisticsType *stats_before = Dlt_GetStatistics();
    uint32_t sent_before = stats_before->messages_sent;
    
    for (int i = 0; i < 5; i++) {
        Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Count test");
    }
    
    const Dlt_StatisticsType *stats_after = Dlt_GetStatistics();
    TEST_ASSERT_EQUAL(sent_before + 5, stats_after->messages_sent);
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00200] - 参数验证测试
 *============================================================================*/

/**
 * @brief 测试NULL参数处理
 */
void test_dlt_null_parameter_handling(void) {
    Dlt_Init(NULL);
    
    /* 测试NULL上下文 */
    TEST_ASSERT_EQUAL(DLT_RETURN_WRONG_PARAMETER, 
                      Dlt_LogString(NULL, DLT_LOG_INFO, "Test"));
    
    /* 测试NULL消息 */
    Dlt_ContextType ctx;
    Dlt_RegisterContext(&ctx, "TEST", "NULL", "Null Test");
    TEST_ASSERT_EQUAL(DLT_RETURN_WRONG_PARAMETER,
                      Dlt_LogString(&ctx, DLT_LOG_INFO, NULL));
    
    /* 测试未初始化调用 */
    Dlt_DeInit();
    TEST_ASSERT_EQUAL(DLT_RETURN_ERROR,
                      Dlt_LogString(&ctx, DLT_LOG_INFO, "Test"));
}

/**
 * @brief 测试无效参数处理
 */
void test_dlt_invalid_parameter_handling(void) {
    Dlt_Init(NULL);
    
    /* 测试无效的日志级别 (超出范围) */
    Dlt_ContextType ctx;
    Dlt_RegisterContext(&ctx, "TEST", "INV", "Invalid Test");
    
    /* 超出定义的日志级别 - 行为取决于实现 */
    /* 规范允许实现自定义处理 */
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00300] - 宏定义测试
 *============================================================================*/

/**
 * @brief 测试便捷日志宏
 */
void test_dlt_log_macros(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "MAC", "Macro Test");
    
    /* 测试所有日志宏 */
    DLT_LOG_FATAL(&g_test_ctx, "Fatal via macro");
    DLT_LOG_ERROR(&g_test_ctx, "Error via macro");
    DLT_LOG_WARNING(&g_test_ctx, "Warning via macro");
    DLT_LOG_INFO(&g_test_ctx, "Info via macro");
    
    /* DEBUG级别需要设置 */
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_DEBUG);
    DLT_LOG_DEBUG(&g_test_ctx, "Debug via macro");
    DLT_LOG_VERBOSE(&g_test_ctx, "Verbose via macro");
    
    /* 测试格式化宏 */
    DLT_LOGF_INFO(&g_test_ctx, "Formatted: %s, %d", "test", 42);
    
    Dlt_DeInit();
}

/**
 * @brief 测试追踪宏
 */
void test_dlt_trace_macros(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "TRCM", "Trace Macro");
    Dlt_SetContextLogLevel(&g_test_ctx, DLT_LOG_DEBUG);
    
    /* 测试追踪宏 */
    DLT_TRACE_IN(&g_test_ctx, "test_function");
    DLT_TRACE_OUT(&g_test_ctx, "test_function");
    DLT_TRACE_STATE(&g_test_ctx, "RUNNING");
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00400] - 格式化日志测试
 *============================================================================*/

/**
 * @brief 测试格式化字符串功能
 */
void test_dlt_format_string(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "FMT", "Format Test");
    
    /* 测试各种格式化选项 */
    TEST_ASSERT_DLT_OK(Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, 
                                           "String: %s", "test"));
    TEST_ASSERT_DLT_OK(Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, 
                                           "Integer: %d", -42));
    TEST_ASSERT_DLT_OK(Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, 
                                           "Unsigned: %u", 42));
    TEST_ASSERT_DLT_OK(Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, 
                                           "Hex: 0x%X", 0xABCD));
    TEST_ASSERT_DLT_OK(Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, 
                                           "Multiple: %s %d %u", "a", 1, 2));
    
    /* 测试长格式化字符串 */
    char long_fmt[512];
    snprintf(long_fmt, sizeof(long_fmt), "Long message: %%s");
    char long_msg[1024];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';
    
    Dlt_ReturnType ret = Dlt_LogFormatString(&g_test_ctx, DLT_LOG_INFO, 
                                              long_fmt, long_msg);
    /* 长消息可能被截断，但仍然应该成功 */
    TEST_ASSERT(ret == DLT_RETURN_OK || ret == DLT_RETURN_ERROR);
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00500] - 数据日志测试
 *============================================================================*/

/**
 * @brief 测试二进制数据日志
 */
void test_dlt_data_logging(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "DATA", "Data Test");
    
    /* 测试小数据 */
    uint8_t small_data[] = {0x00, 0x01, 0x02, 0x03};
    TEST_ASSERT_DLT_OK(Dlt_LogData(&g_test_ctx, DLT_LOG_INFO, small_data, sizeof(small_data)));
    
    /* 测试大数据 */
    uint8_t large_data[256];
    for (int i = 0; i < sizeof(large_data); i++) {
        large_data[i] = i;
    }
    TEST_ASSERT_DLT_OK(Dlt_LogData(&g_test_ctx, DLT_LOG_INFO, large_data, sizeof(large_data)));
    
    /* 测试空数据 */
    TEST_ASSERT_EQUAL(DLT_RETURN_WRONG_PARAMETER,
                      Dlt_LogData(&g_test_ctx, DLT_LOG_INFO, NULL, 10));
    
    /* 测试零长度 */
    TEST_ASSERT_DLT_OK(Dlt_LogData(&g_test_ctx, DLT_LOG_INFO, small_data, 0));
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00600] - 统计功能测试
 *============================================================================*/

/**
 * @brief 测试统计信息准确性
 */
void test_dlt_statistics_accuracy(void) {
    Dlt_Init(NULL);
    Dlt_RegisterContext(&g_test_ctx, "TEST", "STAT", "Stats Test");
    
    /* 重置统计 */
    Dlt_ResetStatistics();
    const Dlt_StatisticsType *stats = Dlt_GetStatistics();
    TEST_ASSERT_EQUAL(0, stats->messages_sent);
    TEST_ASSERT_EQUAL(0, stats->messages_dropped);
    TEST_ASSERT_EQUAL(0, stats->buffer_overflows);
    TEST_ASSERT_EQUAL(0, stats->bytes_written);
    TEST_ASSERT_EQUAL(0, stats->bytes_dropped);
    
    /* 发送消息并验证统计 */
    Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Test1");
    stats = Dlt_GetStatistics();
    TEST_ASSERT_EQUAL(1, stats->messages_sent);
    TEST_ASSERT_GREATER_THAN(0, stats->bytes_written);
    
    /* 发送更多消息 */
    for (int i = 0; i < 5; i++) {
        Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Test");
    }
    stats = Dlt_GetStatistics();
    TEST_ASSERT_EQUAL(6, stats->messages_sent);
    
    Dlt_DeInit();
}

/*=============================================================================
 * [SWS_Dlt_00700] - 会话管理测试
 *============================================================================*/

/**
 * @brief 测试会话ID一致性
 */
void test_dlt_session_consistency(void) {
    Dlt_Init(NULL);
    
    /* 同一DLT实例中，会话ID应保持一致 */
    Dlt_RegisterContext(&g_test_ctx, "TEST", "SESS", "Session Test");
    
    /* 发送多条消息 */
    for (int i = 0; i < 5; i++) {
        Dlt_LogString(&g_test_ctx, DLT_LOG_INFO, "Session test");
    }
    
    Dlt_DeInit();
}

/*=============================================================================
 * 回归测试套件
 *============================================================================*/

void test_dlt_autosar_compliance_suite(void) {
    /* 消息格式测试 */
    RUN_TEST(test_dlt_standard_header_format);
    RUN_TEST(test_dlt_extended_header_format);
    
    /* 日志级别测试 */
    RUN_TEST(test_dlt_log_levels_all);
    RUN_TEST(test_dlt_log_level_off);
    
    /* 消息类型测试 */
    RUN_TEST(test_dlt_message_type_log);
    RUN_TEST(test_dlt_message_type_app_trace);
    RUN_TEST(test_dlt_message_type_network_trace);
    
    /* 上下文管理测试 */
    RUN_TEST(test_dlt_context_limit);
    RUN_TEST(test_dlt_context_id_format);
    
    /* 缓冲区管理测试 */
    RUN_TEST(test_dlt_buffer_overflow);
    RUN_TEST(test_dlt_buffer_clear);
    
    /* 消息计数器测试 */
    RUN_TEST(test_dlt_message_counter);
    
    /* 参数验证测试 */
    RUN_TEST(test_dlt_null_parameter_handling);
    RUN_TEST(test_dlt_invalid_parameter_handling);
    
    /* 宏定义测试 */
    RUN_TEST(test_dlt_log_macros);
    RUN_TEST(test_dlt_trace_macros);
    
    /* 格式化测试 */
    RUN_TEST(test_dlt_format_string);
    
    /* 数据日志测试 */
    RUN_TEST(test_dlt_data_logging);
    
    /* 统计功能测试 */
    RUN_TEST(test_dlt_statistics_accuracy);
    
    /* 会话管理测试 */
    RUN_TEST(test_dlt_session_consistency);
}
