/**
 * @file test_dlt_adapter.c
 * @brief DLT-Telemetry适配器测试
 */

#include <unity.h>
#include "dlt_adapter.h"
#include "telemetry.h"
#include <string.h>

void test_adapter_init(void);
void test_adapter_module_registration(void);
void test_adapter_event_conversion(void);
void test_adapter_levels(void);

void test_adapter_init(void) {
    Dlt_AdapterConfigType config = {
        .auto_register_contexts = true,
        .default_level = DLT_LOG_INFO,
        .enable_event_mapping = true,
        .enable_network_trace = true
    };
    
    /* 先初始化DLT */
    Dlt_Init(NULL);
    
    Dlt_ReturnType ret = Dlt_Adapter_Init(&config);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 验证默认模块已注册 */
    Dlt_ContextType *sys_ctx = Dlt_Adapter_GetContext(TEL_MOD_SYS);
    TEST_ASSERT_NOT_NULL(sys_ctx);
    
    Dlt_ContextType *dds_ctx = Dlt_Adapter_GetContext(TEL_MOD_DDS);
    TEST_ASSERT_NOT_NULL(dds_ctx);
    
    Dlt_Adapter_DeInit();
    Dlt_DeInit();
}

void test_adapter_module_registration(void) {
    Dlt_Init(NULL);
    Dlt_Adapter_Init(NULL);
    
    /* 测试注册新模块 */
    Dlt_ReturnType ret = Dlt_Adapter_RegisterModule(TEL_MOD_SYS, "TEST", "APP");
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 验证可以获取上下文 */
    Dlt_ContextType *ctx = Dlt_Adapter_GetContext(TEL_MOD_SYS);
    TEST_ASSERT_NOT_NULL(ctx);
    
    /* 测试注销 */
    ret = Dlt_Adapter_UnregisterModule(TEL_MOD_SYS);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    ctx = Dlt_Adapter_GetContext(TEL_MOD_SYS);
    /* 注销后应该返回NULL或其他值 */
    
    Dlt_Adapter_DeInit();
    Dlt_DeInit();
}

void test_adapter_event_conversion(void) {
    Dlt_Init(NULL);
    Dlt_Adapter_Init(NULL);
    
    /* 创建测试事件 */
    TelEntry_t event = {
        .header = {
            .timestamp = 12345,
            .module_id = TEL_MOD_SYS,
            .event_id = 0x01,
            .level = TEL_LEVEL_INFO,
            .event_type = TEL_TYPE_INSTANT,
            .payload_len = 0,
            .seq_num = 1,
            .crc = 0
        }
    };
    
    /* 测试转换 */
    Dlt_ReturnType ret = Dlt_Adapter_ConvertEvent(&event);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试Counter事件 */
    event.header.event_type = TEL_TYPE_COUNTER;
    event.payload.counter_value = 42;
    ret = Dlt_Adapter_ConvertEvent(&event);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试State事件 */
    event.header.event_type = TEL_TYPE_STATE;
    event.payload.state.old_state = 0;
    event.payload.state.new_state = 1;
    ret = Dlt_Adapter_ConvertEvent(&event);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 测试Metric事件 */
    event.header.event_type = TEL_TYPE_METRIC;
    event.payload.metric_value = 1000;
    ret = Dlt_Adapter_ConvertEvent(&event);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    Dlt_Adapter_DeInit();
    Dlt_DeInit();
}

void test_adapter_levels(void) {
    Dlt_Init(NULL);
    Dlt_Adapter_Init(NULL);
    
    /* 测试设置模块日志级别 */
    Dlt_ReturnType ret = Dlt_Adapter_SetModuleLevel(TEL_MOD_SYS, DLT_LOG_DEBUG);
    TEST_ASSERT_EQUAL(DLT_RETURN_OK, ret);
    
    /* 获取上下文验证 */
    Dlt_ContextType *ctx = Dlt_Adapter_GetContext(TEL_MOD_SYS);
    TEST_ASSERT_NOT_NULL(ctx);
    
    Dlt_Adapter_DeInit();
    Dlt_DeInit();
}

void test_adapter_suite(void) {
    RUN_TEST(test_adapter_init);
    RUN_TEST(test_adapter_module_registration);
    RUN_TEST(test_adapter_event_conversion);
    RUN_TEST(test_adapter_levels);
}
