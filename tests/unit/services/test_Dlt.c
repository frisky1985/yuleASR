/**
 * @file test_Dlt.c
 * @brief DLT 模块单元测试
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 */

// @tests src/bsw/services/dlt/src/Dlt.c  @tests src/bsw/services/dlt/include/Dlt.h

#include "../test_framework.h"
#include "Dlt.h"
#include "Dlt_Internal.h"
#include "Dlt_Cfg.h"
#include <string.h>

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
    
}

TEST_CASE_DECLARE(Dlt_Init_null_config)
{
    /* 确保从未初始化状态开始 (模块状态跨测试残留) */
    Dlt_DeInit();

    /* 测试空指针配置 - 应该通过 DET 检测但不崩溃 */
    Dlt_Init(NULL);
    
    /* 模块应保持未初始化状态 */
    Dlt_ModuleStateType state = Dlt_GetStatus();
    ASSERT_EQ(DLT_STATE_UNINIT, state);
    
}

TEST_CASE_DECLARE(Dlt_DeInit_basic)
{
    Dlt_Init(&g_test_config);
    Dlt_DeInit();
    
    Dlt_ModuleStateType state = Dlt_GetStatus();
    ASSERT_EQ(DLT_STATE_UNINIT, state);
    
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
    
}

TEST_CASE_DECLARE(Dlt_GetVersionInfo_null_pointer)
{
    /* 测试空指针 - 应该通过 DET 检测 */
    Dlt_GetVersionInfo(NULL);
    
}

/* ========================================================================== */
/*                          应用注册测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_RegisterApp_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    ASSERT_NEQ(DLT_INVALID_APP_HANDLE, handle);
    
}

TEST_CASE_DECLARE(Dlt_RegisterApp_null_pointer)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(NULL);
    
    ASSERT_EQ(DLT_INVALID_APP_HANDLE, handle);
    
}

TEST_CASE_DECLARE(Dlt_RegisterApp_uninit)
{
    /* 在未初始化状态下注册 */
    Dlt_DeInit();
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    ASSERT_EQ(DLT_INVALID_APP_HANDLE, handle);
    
}

TEST_CASE_DECLARE(Dlt_UnregisterApp_valid)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_UnregisterApp(handle);
    
    ASSERT_EQ(E_OK, result);
    
}

TEST_CASE_DECLARE(Dlt_UnregisterApp_invalid_handle)
{
    Dlt_Init(&g_test_config);
    
    Std_ReturnType result = Dlt_UnregisterApp(DLT_INVALID_APP_HANDLE);
    
    ASSERT_EQ(E_NOT_OK, result);
    
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
    
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_uninit)
{
    Dlt_DeInit();
    uint8 testData[8] = {0};
    Std_ReturnType result = Dlt_SendLogMessage(1, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_NOT_OK, result);
    
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_invalid_handle)
{
    Dlt_Init(&g_test_config);
    
    uint8 testData[8] = {0};
    Std_ReturnType result = Dlt_SendLogMessage(DLT_INVALID_APP_HANDLE, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_NOT_OK, result);
    
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_null_data)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, NULL, 0);
    
    ASSERT_EQ(E_NOT_OK, result);
    
}

TEST_CASE_DECLARE(Dlt_SendLogMessage_exceed_max_length)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    uint8 testData[2000]; /* 超过 DLT_MAX_MSG_SIZE (1400) */
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 2000);
    
    ASSERT_EQ(E_NOT_OK, result);
    
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
    
}

TEST_CASE_DECLARE(Dlt_SendTraceMessage_uninit)
{
    Dlt_DeInit();
    uint8 testData[8] = {0};
    Std_ReturnType result = Dlt_SendTraceMessage(1, DLT_TRACE_VARIABLE, 0x0001U, testData, 8);
    
    ASSERT_EQ(E_NOT_OK, result);
    
}

TEST_CASE_DECLARE(Dlt_SendTraceMessage_null_data)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    Std_ReturnType result = Dlt_SendTraceMessage(handle, DLT_TRACE_VARIABLE, 0x0001U, NULL, 0);
    
    ASSERT_EQ(E_NOT_OK, result);
    
}

/* ========================================================================== */
/*                          主函数测试                                         */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_MainFunction_uninit)
{
    /* 在未初始化状态下调用 MainFunction - 应该安全返回 */
    Dlt_MainFunction();
    
}

TEST_CASE_DECLARE(Dlt_MainFunction_ready)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    uint8 testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 8);
    
    /* 调用 MainFunction 处理队列 */
    Dlt_MainFunction();
    
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
    
}

TEST_CASE_DECLARE(Dlt_SetFilter_invalid_handle)
{
    Dlt_Init(&g_test_config);
    
    Std_ReturnType result = Dlt_SetFilter(DLT_INVALID_APP_HANDLE, DLT_LOG_DEBUG, TRUE);
    
    ASSERT_EQ(E_NOT_OK, result);
    
}

/* ========================================================================== */
/*                          队列操作测试                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(Dlt_FlushQueue_basic)
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
    
}

TEST_CASE_DECLARE(Dlt_FlushQueue_uninit)
{
    Dlt_DeInit();
    Std_ReturnType result = Dlt_FlushQueue();
    
    ASSERT_EQ(E_NOT_OK, result);
    
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
    
}

TEST_CASE_DECLARE(Dlt_MaxLengthMessage)
{
    Dlt_Init(&g_test_config);
    
    Dlt_AppHandleType handle = Dlt_RegisterApp(&g_test_app_info);
    
    /* 发送最大长度的消息 */
    uint8 testData[1400]; /* DLT_MAX_MSG_SIZE */
    Std_ReturnType result = Dlt_SendLogMessage(handle, DLT_LOG_INFO, 0x0001U, testData, 1400);
    
    ASSERT_EQ(E_OK, result);
    
}

/* ========================================================================== */
/*                  Context 配置表测试 (ecual 合并, 2026-08-15)                */
/* ========================================================================== */

/* Dlt_Lcfg.c 导出的链接期配置符号 */
extern const Dlt_ContextType Dlt_ContextConfig[DLT_MAX_CONTEXT_COUNT];
extern Dlt_ContextType Dlt_RuntimeContext[DLT_MAX_CONTEXT_COUNT];
extern const uint16 Dlt_ContextGroupCount;
extern const uint8 Dlt_EcuId[DLT_ECU_ID_LENGTH];
extern const uint32 Dlt_DefaultSessionId;
extern const uint8 Dlt_ProtocolVersionMajor;
extern const uint8 Dlt_ProtocolVersionMinor;
extern const uint32 Dlt_BufferTimeout;
extern const uint32 Dlt_MainFunctionPeriod;

TEST_CASE_DECLARE(Dlt_Lcfg_ContextConfig_table_size)
{
    /* Dlt_ContextConfig 必须为 DLT_MAX_CONTEXT_COUNT (32) 项 */
    ASSERT_EQ(DLT_MAX_CONTEXT_COUNT, 32U);
    ASSERT_EQ(0x44454641U, Dlt_ContextConfig[0].appId);   /* "DEFA" */
    ASSERT_EQ(0x434D444CU, Dlt_ContextConfig[0].contextId); /* "CMDL" */
    ASSERT_EQ(DLT_LOG_INFO, Dlt_ContextConfig[0].logLevel);
    ASSERT_EQ(DLT_TRACE_STATUS_ON, Dlt_ContextConfig[0].traceStatus);
    ASSERT_TRUE(Dlt_ContextConfig[0].registered);
    /* 尾项 APP2 */
    ASSERT_EQ(0x41505032U, Dlt_ContextConfig[31].appId);
    ASSERT_TRUE(Dlt_ContextConfig[31].registered);
}

TEST_CASE_DECLARE(Dlt_Lcfg_ContextGroups)
{
    /* 分组表与 context 表一一对应 (32 组) */
    ASSERT_EQ(32U, Dlt_ContextGroupCount);
}

TEST_CASE_DECLARE(Dlt_Lcfg_GlobalConfig)
{
    ASSERT_EQ(0U, memcmp("ECU1", Dlt_EcuId, DLT_ECU_ID_LENGTH));
    ASSERT_EQ(DLT_DEFAULT_SESSION_ID, Dlt_DefaultSessionId);
    ASSERT_EQ(DLT_PROTOCOL_VERSION_MAJOR, Dlt_ProtocolVersionMajor);
    ASSERT_EQ(DLT_PROTOCOL_VERSION_MINOR, Dlt_ProtocolVersionMinor);
    ASSERT_EQ(DLT_BUFFERING_TIMEOUT, Dlt_BufferTimeout);
    ASSERT_EQ(DLT_MAIN_FUNCTION_PERIOD, Dlt_MainFunctionPeriod);
}

/* ========================================================================== */
/*                          测试主函数                                         */
/* ========================================================================== */

TEST_MAIN_BEGIN()
{
    /* 初始化/反初始化测试 */
    RUN_TEST(Dlt_Init_valid_config);
    RUN_TEST(Dlt_Init_null_config);
    RUN_TEST(Dlt_DeInit_basic);
    
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
    RUN_TEST(Dlt_FlushQueue_basic);
    RUN_TEST(Dlt_FlushQueue_uninit);
    
    /* 多消息测试 */
    RUN_TEST(Dlt_MultipleMessages);
    
    /* 边界条件测试 */
    RUN_TEST(Dlt_EmptyMessage);
    RUN_TEST(Dlt_MaxLengthMessage);

    /* Context 配置表测试 (ecual 合并) */
    RUN_TEST(Dlt_Lcfg_ContextConfig_table_size);
    RUN_TEST(Dlt_Lcfg_ContextGroups);
    RUN_TEST(Dlt_Lcfg_GlobalConfig);
}
TEST_MAIN_END()
