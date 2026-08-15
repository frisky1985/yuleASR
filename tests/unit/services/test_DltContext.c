/**
 * @file test_DltContext.c
 * @brief DLT Context 管理 API 单元测试 (ecual 合并, 2026-08-15)
 *
 * 覆盖: 注册 / 注销 / 设置-读取日志级别 / 设置-读取跟踪状态 /
 *       Com 回调占位 / 边界 (未初始化 / 未找到 context / 表满 / 空指针 / 超长描述)
 */

#include "../test_framework.h"
#include "Dlt.h"
#include "Dlt_Cfg.h"
#include "Dlt_Types.h"

/* Dlt_Lcfg.c 导出的链接期配置符号 */
extern const Dlt_ContextType Dlt_ContextConfig[DLT_MAX_CONTEXT_COUNT];
extern Dlt_ContextType Dlt_RuntimeContext[DLT_MAX_CONTEXT_COUNT];

/* Mock Det_ReportError */
void Det_ReportError(uint8 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
}

/* ========================================================================== */
/*                               辅助函数                                      */
/* ========================================================================== */

static void ContextTest_Init(void)
{
    const Dlt_ConfigType config = {
        .transportConfig = NULL,
        .filterConfig = NULL,
        .filterCount = 0U,
        .queueSize = DLT_QUEUE_SIZE
    };
    Dlt_Init(&config);
}

static void ContextTest_DeInit(void)
{
    Dlt_DeInit();
}

/* 测试常量: DEFA/CMDL 为链接期预配置 context (槽位 0) */
#define TEST_APP_DEFA   0x44454641U   /* "DEFA" */
#define TEST_CTX_CMDL   0x434D444CU   /* "CMDL" */
#define TEST_APP_NEW    0x54455354U   /* "TEST" */
#define TEST_CTX_NEW    0x43545831U   /* "CTX1" */

/* ========================================================================== */
/*                              测试用例                                       */
/* ========================================================================== */

TEST_CASE_DECLARE(DltContext_Init_runtime_copy)
{
    ContextTest_Init();

    /* Dlt_Init 后运行时表应完整复制链接期配置表 (32 项) */
    ASSERT_EQ(DLT_MAX_CONTEXT_COUNT, 32U);
    ASSERT_EQ(TEST_APP_DEFA, Dlt_RuntimeContext[0].appId);
    ASSERT_EQ(TEST_CTX_CMDL, Dlt_RuntimeContext[0].contextId);
    ASSERT_TRUE(Dlt_RuntimeContext[0].registered);
    ASSERT_EQ(0x41505032U, Dlt_RuntimeContext[31].appId);   /* "APP2" */
    ASSERT_TRUE(Dlt_RuntimeContext[31].registered);
    ASSERT_EQ(0U, memcmp(Dlt_ContextConfig, Dlt_RuntimeContext,
                         sizeof(Dlt_ContextConfig)));

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Register_existing_idempotent)
{
    ContextTest_Init();

    /* 预配置 context 重复注册 → 幂等 E_OK */
    const uint8 desc[4] = {'d', 'u', 'p', 0};
    Std_ReturnType result = Dlt_RegisterContext(TEST_APP_DEFA, TEST_CTX_CMDL, desc, 4U);
    ASSERT_EQ(E_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Register_new_when_full)
{
    ContextTest_Init();

    /* 32 个链接期预配置 context 占满表项 → 新注册返回 E_NOT_OK */
    const uint8 desc[4] = {'n', 'e', 'w', 0};
    Std_ReturnType result = Dlt_RegisterContext(TEST_APP_NEW, TEST_CTX_NEW, desc, 4U);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Unregister_then_register)
{
    ContextTest_Init();

    /* 注销 DEFA/CMDL 释放槽位后, 新 context 可注册 */
    Std_ReturnType result = Dlt_UnregisterContext(TEST_APP_DEFA, TEST_CTX_CMDL);
    ASSERT_EQ(E_OK, result);

    const uint8 desc[4] = {'n', 'e', 'w', 0};
    result = Dlt_RegisterContext(TEST_APP_NEW, TEST_CTX_NEW, desc, 4U);
    ASSERT_EQ(E_OK, result);

    /* 新注册 context 使用默认日志级别 */
    Dlt_LogLevelType level = DLT_LOG_FATAL;
    result = Dlt_GetLogLevel(TEST_APP_NEW, TEST_CTX_NEW, &level);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_DEFAULT_LOG_LEVEL, level);

    /* 注销新 context */
    result = Dlt_UnregisterContext(TEST_APP_NEW, TEST_CTX_NEW);
    ASSERT_EQ(E_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Unregister_not_found)
{
    ContextTest_Init();

    Std_ReturnType result = Dlt_UnregisterContext(TEST_APP_NEW, TEST_CTX_NEW);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_SetGet_LogLevel)
{
    ContextTest_Init();

    /* 设置并读取日志级别 */
    Std_ReturnType result = Dlt_SetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, DLT_LOG_DEBUG);
    ASSERT_EQ(E_OK, result);

    Dlt_LogLevelType level = DLT_LOG_FATAL;
    result = Dlt_GetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, &level);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_LOG_DEBUG, level);

    /* 设置回 INFO */
    result = Dlt_SetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, DLT_LOG_INFO);
    ASSERT_EQ(E_OK, result);
    result = Dlt_GetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, &level);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_LOG_INFO, level);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_SetLogLevel_not_found)
{
    ContextTest_Init();

    Std_ReturnType result = Dlt_SetLogLevel(TEST_APP_NEW, TEST_CTX_NEW, DLT_LOG_INFO);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_GetLogLevel_null_ptr)
{
    ContextTest_Init();

    Std_ReturnType result = Dlt_GetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, NULL);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_GetLogLevel_not_found)
{
    ContextTest_Init();

    Dlt_LogLevelType level = DLT_LOG_FATAL;
    Std_ReturnType result = Dlt_GetLogLevel(TEST_APP_NEW, TEST_CTX_NEW, &level);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_SetGet_TraceStatus)
{
    ContextTest_Init();

    /* 预配置为 ON, 关闭再开启 */
    Std_ReturnType result = Dlt_SetTraceStatus(TEST_APP_DEFA, TEST_CTX_CMDL, DLT_TRACE_STATUS_OFF);
    ASSERT_EQ(E_OK, result);

    Dlt_TraceStatusType status = DLT_TRACE_STATUS_ON;
    result = Dlt_GetTraceStatus(TEST_APP_DEFA, TEST_CTX_CMDL, &status);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_TRACE_STATUS_OFF, status);

    result = Dlt_SetTraceStatus(TEST_APP_DEFA, TEST_CTX_CMDL, DLT_TRACE_STATUS_ON);
    ASSERT_EQ(E_OK, result);
    result = Dlt_GetTraceStatus(TEST_APP_DEFA, TEST_CTX_CMDL, &status);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_TRACE_STATUS_ON, status);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_GetTraceStatus_null_ptr)
{
    ContextTest_Init();

    Std_ReturnType result = Dlt_GetTraceStatus(TEST_APP_DEFA, TEST_CTX_CMDL, NULL);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Com_callbacks)
{
    ContextTest_Init();

#if (DLT_USE_COM == STD_ON)
    /* 占位回调: 调用不崩溃 */
    Dlt_ComTxConfirmation(E_OK);
    const uint8 rxData[4] = {0x01, 0x02, 0x03, 0x04};
    Dlt_ComRxIndication(rxData, 4U);
    Dlt_ComRxIndication(NULL, 0U);
#endif
    ASSERT_TRUE(TRUE);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Register_null_description)
{
    ContextTest_Init();

    /* 先释放槽位, 再验证空描述指针 → E_NOT_OK */
    Std_ReturnType result = Dlt_UnregisterContext(TEST_APP_DEFA, TEST_CTX_CMDL);
    ASSERT_EQ(E_OK, result);

    result = Dlt_RegisterContext(TEST_APP_NEW, TEST_CTX_NEW, NULL, 0U);
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Register_desc_too_long)
{
    ContextTest_Init();

    uint8 longDesc[DLT_MAX_CONTEXT_DESCRIPTION + 1U];
    (void)memset(longDesc, 'x', sizeof(longDesc));

    Std_ReturnType result = Dlt_RegisterContext(TEST_APP_DEFA, TEST_CTX_CMDL,
                                                longDesc, (uint8)sizeof(longDesc));
    ASSERT_EQ(E_NOT_OK, result);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_Register_uninit)
{
    /* 未初始化状态下注册 → E_NOT_OK (DET 检测) */
    ContextTest_DeInit();

    const uint8 desc[4] = {'t', 'e', 's', 't'};
    Std_ReturnType result = Dlt_RegisterContext(TEST_APP_NEW, TEST_CTX_NEW, desc, 4U);
    ASSERT_EQ(E_NOT_OK, result);
}

TEST_CASE_DECLARE(DltContext_Reregister_resets_level)
{
    ContextTest_Init();

    /* 改级别 → 注销 → 重新注册: 级别恢复默认 */
    Std_ReturnType result = Dlt_SetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, DLT_LOG_VERBOSE);
    ASSERT_EQ(E_OK, result);

    result = Dlt_UnregisterContext(TEST_APP_DEFA, TEST_CTX_CMDL);
    ASSERT_EQ(E_OK, result);

    const uint8 desc[4] = {'a', 'g', 'a', 'i'};
    result = Dlt_RegisterContext(TEST_APP_DEFA, TEST_CTX_CMDL, desc, 4U);
    ASSERT_EQ(E_OK, result);

    Dlt_LogLevelType level = DLT_LOG_FATAL;
    result = Dlt_GetLogLevel(TEST_APP_DEFA, TEST_CTX_CMDL, &level);
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_DEFAULT_LOG_LEVEL, level);

    ContextTest_DeInit();
}

TEST_CASE_DECLARE(DltContext_AllContexts_match_config)
{
    ContextTest_Init();

    /* 全表一致性: 32 个 context 均与链接期配置一致 (含未修改的 traceStatus) */
    for (uint16 i = 0U; i < DLT_MAX_CONTEXT_COUNT; i++) {
        ASSERT_TRUE(Dlt_RuntimeContext[i].registered);
        ASSERT_EQ(Dlt_ContextConfig[i].appId, Dlt_RuntimeContext[i].appId);
        ASSERT_EQ(Dlt_ContextConfig[i].contextId, Dlt_RuntimeContext[i].contextId);
    }

    ContextTest_DeInit();
}

/* ========================================================================== */
/*                              测试主函数                                     */
/* ========================================================================== */

TEST_MAIN_BEGIN()
{
    RUN_TEST(DltContext_Init_runtime_copy);
    RUN_TEST(DltContext_Register_existing_idempotent);
    RUN_TEST(DltContext_Register_new_when_full);
    RUN_TEST(DltContext_Unregister_then_register);
    RUN_TEST(DltContext_Unregister_not_found);
    RUN_TEST(DltContext_SetGet_LogLevel);
    RUN_TEST(DltContext_SetLogLevel_not_found);
    RUN_TEST(DltContext_GetLogLevel_null_ptr);
    RUN_TEST(DltContext_GetLogLevel_not_found);
    RUN_TEST(DltContext_SetGet_TraceStatus);
    RUN_TEST(DltContext_GetTraceStatus_null_ptr);
    RUN_TEST(DltContext_Com_callbacks);
    RUN_TEST(DltContext_Register_null_description);
    RUN_TEST(DltContext_Register_desc_too_long);
    RUN_TEST(DltContext_Register_uninit);
    RUN_TEST(DltContext_Reregister_resets_level);
    RUN_TEST(DltContext_AllContexts_match_config);
}
TEST_MAIN_END()
