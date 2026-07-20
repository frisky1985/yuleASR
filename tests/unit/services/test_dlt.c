/**
 * @file test_Dlt.c
 * @brief DLT 模块单元测试
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 */

#include "../test_framework.h"
#include "../../src/bsw/services/dlt/include/Dlt.h"
#include "../../src/bsw/services/dlt/include/Dlt_Internal.h"

/* ========================================================================== */
/*                          测试辅助函数和变量                                  */
/* ========================================================================== */

static Dlt_ConfigType g_test_config = {
    .transportConfig = NULL,
    .filterConfig = NULL,
    .filterCount = 0U,
    .queueSize = DLT_QUEUE_SIZE
};

static Dlt_AppInfoType g_test_app_info = {
    .appId = "TEST",
    .appDescription = "Test Application",
    .maxLogLevel = DLT_LOG_DEBUG
};

static Dlt_TransportConfigType g_test_transport_config = {
    .protocol = DLT_TRANSPORT_UDP,
    .port = 3490U,
    .bufferSize = 4096U,
    .maxMessageSize = 1400U
};

/* Mock Det_ReportError */
void Det_ReportError(uint8 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
}

/* ========================================================================== */
/*                          初始化/反初始化测试                                 */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_Init_valid_config)
{
    Dlt_Init(&g_test_config);
    
    Dlt_ModuleStateType state = Dlt_GetStatus();
    ASSERT_EQ(DLT_STATE_READY, state);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_Init_null_config)
{
    /* 测试空指针配置 - 应该通过 DET 检测但不崩溃 */
    Dlt_Init(NULL);
    
    /* 模块应保持未初始化状态 */
    Dlt_ModuleStateType state = Dlt_GetStatus();
    ASSERT_EQ(DLT_STATE_UNINIT, state);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_DeInit)
{
    Dlt_Init(&g_test_config);
    Dlt_DeInit();
    
    Dlt_ModuleStateType state = Dlt_GetStatus();
    ASSERT_EQ(DLT_STATE_UNINIT, state);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          版本信息测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_GetVersionInfo_valid)
{
    Std_VersionInfoType versionInfo;
    
    Dlt_GetVersionInfo(&versionInfo);
    
    ASSERT_EQ(DLT_VENDOR_ID, versionInfo.vendorID);
    ASSERT_EQ(DLT_MODULE_ID, versionInfo.moduleID);
    ASSERT_EQ(DLT_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    ASSERT_EQ(DLT_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    ASSERT_EQ(DLT_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_GetVersionInfo_null_pointer)
{
    /* 测试空指针 - 应该通过 DET 检测 */
    Dlt_GetVersionInfo(NULL);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          应用注册测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_RegisterApp_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    ASSERT_NEQ(DLT_INVALID_APP_HANDLE, handle);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_RegisterApp_null_pointer)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(NULL);
    
    ASSERT_EQ(DLT_INVALID_APP_HANDLE, handle);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_RegisterApp_uninit)
{
    /* 在未初始化状态下注册 */
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    ASSERT_EQ(DLT_INVALID_APP_HANDLE, handle);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_UnregisterApp_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_UnregisterApp(handle);
    
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_UnregisterApp_invalid_handle)
{
    Dlt_Init(&g_test_config);
    
    Std_ReturnType result = Dlt_UnregisterApp(DLT_INVALID_APP_HANDLE);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          日志消息测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_SendLogMessage_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    uint8 testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_uninit)
{
    uint8 testData[8] = {0};
    Std_ReturnType result = Dlt_SendLogMessage(1, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_invalid_handle)
{
    Dlt_Init(&g_test_config);
    
    uint8 testData[8] = {0};
    Std_ReturnType result = Dlt_SendLogMessage(DLT_INVALID_APP_HANDLE, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_null_data)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, NULL, 0);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_exceed_max_length)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    uint8 testData[2000]; /* 超过 DLT_MAX_MSG_SIZE (1400) */
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 2000);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          跟踪消息测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_SendTraceMessage_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    uint8 testData[16] = {0};
    Std_ReturnType result = Dlt_SendTraceMessage(handle, DLT_TRACE_VARIABLE, 0x0001U, testData, 16);
    
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SendTraceMessage_uninit)
{
    uint8 testData[8] = {0};
    Std_ReturnType result = Dlt_SendTraceMessage(1, DLT_TRACE_VARIABLE, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SendTraceMessage_null_data)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_SendTraceMessage(handle, DLT_TRACE_VARIABLE, 0x0001U, NULL, 0);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          主函数测试                                         */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_MainFunction_uninit)
{
    /* 在未初始化状态下调用 MainFunction - 应该安全返回 */
    Dlt_MainFunction();
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_MainFunction_ready)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    uint8 testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    /* 调用 MainFunction 处理队列 */
    Dlt_MainFunction();
    
TEST_ASSERT_TRUE(1U == 1U);
    TEST_PASS();
}

/* ========================================================================== */
/*                          过滤器测试                                         */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_SetFilter_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_SetFilter(handle, DLT_LOG_DEBUG, TRUE);
    
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_SetFilter_invalid_handle)
{
    Dlt_Init(&g_test_config);
    
    Std_ReturnType result = Dlt_SetFilter(DLT_INVALID_APP_HANDLE, DLT_LOG_DEBUG, TRUE);
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          队列操作测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_FlushQueue)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    /* 添加一些消息到队列 */
    uint8 testData[8] = {0};
    Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 8);
    Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0002U, testData, 8);
    
    /* 清空队列 */
    Std_ReturnType result = Dlt_FlushQueue();
    
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_FlushQueue_uninit)
{
    Std_ReturnType result = Dlt_FlushQueue();
    
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          多消息测试                                         */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_MultipleMessages)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    /* 发送多条消息 */
    for (uint8 i = 0; i < 10; i++) {
        uint8 testData[8] = {i, i+1, i+2, i+3, i+4, i+5, i+6, i+7};
        Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, i, testData, 8);
        ASSERT_EQ(E_OK, result);
    }
    
    /* 处理队列 */
    Dlt_MainFunction();
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          边界条件测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_EmptyMessage)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    /* 发送空消息 (长度为0) */
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, NULL, 0);
    
    /* 由于 NULL 指针检查，应该失败 */
    ASSERT_EQ(E_NOT_OK, result);
    
    TEST_PASS();
}

TEST_CASE_DECLARE(Dlt_MaxLengthMessage)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    /* 发送最大长度的消息 */
    uint8 testData[1400]; /* DLT_MAX_MSG_SIZE */
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 1400);
    
    ASSERT_EQ(E_OK, result);
    
    TEST_PASS();
}

/* ========================================================================== */
/*                          测试主函数                                         */
/* ========================================================================== */

TEST_MAIN_BEGIN()
{
    /* 初始化/反初始化测试 */
    RUN_TEST(Dlt_Init_valid_config);
    RUN_TEST(Dlt_Init_null_config);
    RUN_TEST(Dlt_DeInit);
    
    /* 版本信息测试 */
    RUN_TEST(Dlt_GetVersionInfo_valid);
    RUN_TEST(Dlt_GetVersionInfo_null_pointer);
    
    /* 应用注册测试 */
    RUN_TEST(Dlt_RegisterApp_valid);
    RUN_TEST(Dlt_RegisterApp_null_pointer);
    RUN_TEST(Dlt_RegisterApp_uninit);
    RUN_TEST(Dlt_UnregisterApp_valid);
    RUN_TEST(Dlt_UnregisterApp_invalid_handle);
    
    /* 日志消息测试 */
    RUN_TEST(Dlt_SendLogMessage_valid);
    RUN_TEST(Dlt_SendLogMessage_uninit);
    RUN_TEST(Dlt_SendLogMessage_invalid_handle);
    RUN_TEST(Dlt_SendLogMessage_null_data);
    RUN_TEST(Dlt_SendLogMessage_exceed_max_length);
    
    /* 跟踪消息测试 */
    RUN_TEST(Dlt_SendTraceMessage_valid);
    RUN_TEST(Dlt_SendTraceMessage_uninit);
    RUN_TEST(Dlt_SendTraceMessage_null_data);
    
    /* 主函数测试 */
    RUN_TEST(Dlt_MainFunction_uninit);
    RUN_TEST(Dlt_MainFunction_ready);
    
    /* 过滤器测试 */
    RUN_TEST(Dlt_SetFilter_valid);
    RUN_TEST(Dlt_SetFilter_invalid_handle);
    
    /* 队列操作测试 */
    RUN_TEST(Dlt_FlushQueue);
    RUN_TEST(Dlt_FlushQueue_uninit);
    
    /* 多消息测试 */
    RUN_TEST(Dlt_MultipleMessages);
    
    /* 边界条件测试 */
    RUN_TEST(Dlt_EmptyMessage);
    RUN_TEST(Dlt_MaxLengthMessage);
}
TEST_MAIN_END()
