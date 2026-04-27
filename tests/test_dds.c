/**
 * @file test_dds.c
 * @brief DDS接口测试
 */

#include <unity.h>
#include "telemetry_dds.h"
#include "dds/dds_client.h"
#include <string.h>

void test_dds_topic_creation(void);
void test_dds_event_serialization(void);
void test_dds_qos_config(void);

/* 模拟DDS客户端 */
static dds_entity_t g_mock_participant = 1;
static dds_entity_t g_mock_publisher = 2;
static dds_entity_t g_mock_topic = 3;
static dds_entity_t g_mock_writer = 4;

dds_entity_t dds_create_participant(const dds_domainid_t domain, 
                                     const dds_qos_t *qos,
                                     const dds_listener_t *listener) {
    (void)domain; (void)qos; (void)listener;
    return g_mock_participant;
}

dds_entity_t dds_create_publisher(dds_entity_t participant,
                                   const dds_qos_t *qos,
                                   const dds_listener_t *listener) {
    (void)participant; (void)qos; (void)listener;
    return g_mock_publisher;
}

dds_entity_t dds_create_topic(dds_entity_t participant,
                               const dds_topic_descriptor_t *desc,
                               const char *name,
                               const dds_qos_t *qos,
                               const dds_listener_t *listener) {
    (void)participant; (void)desc; (void)qos; (void)listener;
    TEST_ASSERT_EQUAL_STRING("TelemetryTopic", name);
    return g_mock_topic;
}

dds_entity_t dds_create_writer(dds_entity_t publisher,
                                dds_entity_t topic,
                                const dds_qos_t *qos,
                                const dds_listener_t *listener) {
    (void)publisher; (void)topic; (void)qos; (void)listener;
    return g_mock_writer;
}

dds_return_t dds_write(dds_entity_t writer, const void *sample) {
    (void)writer; (void)sample;
    return DDS_RETCODE_OK;
}

dds_return_t dds_delete(dds_entity_t entity) {
    (void)entity;
    return DDS_RETCODE_OK;
}

void test_dds_topic_creation(void) {
    TelDDS_Init();
    
    /* 验证DDS实体已创建 */
    const TelDDS_Ctx_t *ctx = TelDDS_GetContext();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_NOT_EQUAL(0, ctx->participant);
    TEST_ASSERT_NOT_EQUAL(0, ctx->publisher);
    TEST_ASSERT_NOT_EQUAL(0, ctx->topic);
    TEST_ASSERT_NOT_EQUAL(0, ctx->writer);
}

void test_dds_event_serialization(void) {
    TelDDS_Init();
    
    /* 创建测试事件 */
    TelEntry_t entry = {
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
    
    /* 序列化 */
    TelDDS_Sample_t sample;
    TelStatus_t status = TelDDS_Serialize(&entry, &sample);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* 验证序列化结果 */
    TEST_ASSERT_EQUAL(12345, sample.header.timestamp);
    TEST_ASSERT_EQUAL(TEL_MOD_SYS, sample.header.module_id);
    TEST_ASSERT_EQUAL(0x01, sample.header.event_id);
}

void test_dds_qos_config(void) {
    TelDDS_QoSConfig_t config = {
        .reliability = DDS_RELIABILITY_RELIABLE,
        .durability = DDS_DURABILITY_TRANSIENT_LOCAL,
        .history_depth = 100,
        .deadline_ms = 1000
    };
    
    TelStatus_t status = TelDDS_ConfigureQoS(&config);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* 重新初始化应应用新的QoS */
    TelDDS_Init();
}

void test_dds_suite(void) {
    RUN_TEST(test_dds_topic_creation);
    RUN_TEST(test_dds_event_serialization);
    RUN_TEST(test_dds_qos_config);
}
