/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Dlt Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-30
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "Dlt.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Dlt_ConfigType g_test_config;

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_config.outputMode = DLT_OUTPUT_MODE_BUFFER;
    g_test_config.defaultLogLevel = DLT_LOG_DEBUG;
    g_test_config.timestampEnabled = TRUE;
    g_test_config.ecuIdEnabled = TRUE;
    g_test_config.sessionIdEnabled = TRUE;
    g_test_config.devErrorDetect = TRUE;
    g_test_config.versionInfoApi = TRUE;
    g_test_config.networkPort = 3490;
    
    /* Set ECU ID */
    g_test_config.ecuId[0] = 'E';
    g_test_config.ecuId[1] = 'C';
    g_test_config.ecuId[2] = 'U';
    g_test_config.ecuId[3] = '1';
    g_test_config.ecuId[4] = '\0';
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Dlt_Init with valid config */
TEST_CASE(dlt_init_valid_config)
{
    setup_test_config();
    
    Dlt_Init(&g_test_config);
    
    TEST_PASS();
}

/* Test: Dlt_Init with NULL config */
TEST_CASE(dlt_init_null_config)
{
    Dlt_Init(NULL_PTR);
    
    TEST_PASS();
}

/* Test: Dlt_DeInit */
TEST_CASE(dlt_deinit)
{
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    Dlt_DeInit();
    
    TEST_PASS();
}

/* Test: Dlt_GetVersionInfo */
TEST_CASE(dlt_get_version_info)
{
    Std_VersionInfoType version_info;
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    Dlt_GetVersionInfo(&version_info);
    
    ASSERT_EQ(DLT_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(DLT_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(DLT_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_PASS();
}

/* Test: Dlt_RegisterContext */
TEST_CASE(dlt_register_context)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    result = Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_UnregisterContext */
TEST_CASE(dlt_unregister_context)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    result = Dlt_UnregisterContext(app_id, context_id);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_LogMessage */
TEST_CASE(dlt_log_message)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    result = Dlt_LogMessage(app_id, context_id, DLT_LOG_INFO, "Test log message");
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_SetLogLevel */
TEST_CASE(dlt_set_log_level)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    result = Dlt_SetLogLevel(app_id, context_id, DLT_LOG_WARN);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_GetLogLevel */
TEST_CASE(dlt_get_log_level)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    Dlt_LogLevelType log_level;
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    result = Dlt_GetLogLevel(app_id, context_id, &log_level);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_SetOutputMode */
TEST_CASE(dlt_set_output_mode)
{
    Dlt_ReturnType result;
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    result = Dlt_SetOutputMode(DLT_OUTPUT_MODE_SERIAL);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_GetOutputMode */
TEST_CASE(dlt_get_output_mode)
{
    Dlt_ReturnType result;
    Dlt_OutputModeType output_mode;
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    result = Dlt_GetOutputMode(&output_mode);
    
    ASSERT_EQ(E_OK, result);
    ASSERT_EQ(DLT_OUTPUT_MODE_BUFFER, output_mode);
    TEST_PASS();
}

/* Test: Dlt_TracePoint */
TEST_CASE(dlt_trace_point)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    result = Dlt_TracePoint(app_id, context_id, DLT_TRACE_TYPE_FUNCTION_IN, 0);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_TraceVariable */
TEST_CASE(dlt_trace_variable)
{
    Dlt_ReturnType result;
    Dlt_ApplicationIdType app_id = {'T', 'E', 'S', 'T', '\0'};
    Dlt_ContextIdType context_id = {'C', 'T', 'X', '1', '\0'};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    Dlt_RegisterContext(app_id, context_id, "Test Context");
    
    result = Dlt_TraceVariable(app_id, context_id, "test_var", 42);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_FlushBuffer */
TEST_CASE(dlt_flush_buffer)
{
    Dlt_ReturnType result;
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    result = Dlt_FlushBuffer();
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_GetBufferStatus */
TEST_CASE(dlt_get_buffer_status)
{
    Dlt_ReturnType result;
    uint16 used_entries;
    uint16 free_entries;
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    result = Dlt_GetBufferStatus(&used_entries, &free_entries);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: Dlt_MainFunction */
TEST_CASE(dlt_main_function)
{
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    Dlt_MainFunction();
    
    TEST_PASS();
}

/* Test: Dlt_TxConfirmation */
TEST_CASE(dlt_tx_confirmation)
{
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    Dlt_TxConfirmation(0, E_OK);
    
    TEST_PASS();
}

/* Test: Dlt_RxIndication */
TEST_CASE(dlt_rx_indication)
{
    PduInfoType pdu_info;
    uint8 pdu_data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    setup_test_config();
    Dlt_Init(&g_test_config);
    
    pdu_info.SduDataPtr = pdu_data;
    pdu_info.SduLength = 8;
    pdu_info.MetaDataPtr = NULL_PTR;
    
    Dlt_RxIndication(0, &pdu_info);
    
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(dlt)
{
}

TEST_SUITE_TEARDOWN(dlt)
{
}

TEST_SUITE(dlt)
{
    RUN_TEST(dlt_init_valid_config);
    RUN_TEST(dlt_init_null_config);
    RUN_TEST(dlt_deinit);
    RUN_TEST(dlt_get_version_info);
    RUN_TEST(dlt_register_context);
    RUN_TEST(dlt_unregister_context);
    RUN_TEST(dlt_log_message);
    RUN_TEST(dlt_set_log_level);
    RUN_TEST(dlt_get_log_level);
    RUN_TEST(dlt_set_output_mode);
    RUN_TEST(dlt_get_output_mode);
    RUN_TEST(dlt_trace_point);
    RUN_TEST(dlt_trace_variable);
    RUN_TEST(dlt_flush_buffer);
    RUN_TEST(dlt_get_buffer_status);
    RUN_TEST(dlt_main_function);
    RUN_TEST(dlt_tx_confirmation);
    RUN_TEST(dlt_rx_indication);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(dlt);
TEST_MAIN_END()
