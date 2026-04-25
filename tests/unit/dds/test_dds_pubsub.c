/**
 * @file test_dds_pubsub.c
 * @brief DDS发布订阅单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "../../../src/dds/core/dds_core.h"
#include "../../../src/common/types/eth_types.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static dds_domain_participant_t* g_participant = NULL;
static dds_topic_t* g_topic = NULL;
static dds_publisher_t* g_publisher = NULL;
static dds_subscriber_t* g_subscriber = NULL;
static dds_data_writer_t* g_writer = NULL;
static dds_data_reader_t* g_reader = NULL;

static uint8_t g_received_data[1024];
static uint32_t g_received_size = 0;
static volatile int g_data_received = 0;

/*==============================================================================
 * 回调函数
 *==============================================================================*/

static void data_callback(const void* data, uint32_t size, void* user_data) {
    (void)user_data;
    if (size <= sizeof(g_received_data)) {
        memcpy(g_received_data, data, size);
        g_received_size = size;
        g_data_received = 1;
    }
}

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    memset(g_received_data, 0, sizeof(g_received_data));
    g_received_size = 0;
    g_data_received = 0;
    
    dds_runtime_init();
    g_participant = dds_create_participant(0);
    g_topic = dds_create_topic(g_participant, "PubSubTopic", "TestData", NULL);
}

void tearDown(void) {
    if (g_reader != NULL) {
        dds_delete_data_reader(g_reader);
        g_reader = NULL;
    }
    if (g_writer != NULL) {
        dds_delete_data_writer(g_writer);
        g_writer = NULL;
    }
    if (g_subscriber != NULL) {
        dds_delete_subscriber(g_subscriber);
        g_subscriber = NULL;
    }
    if (g_publisher != NULL) {
        dds_delete_publisher(g_publisher);
        g_publisher = NULL;
    }
    if (g_topic != NULL) {
        dds_delete_topic(g_topic);
        g_topic = NULL;
    }
    if (g_participant != NULL) {
        dds_delete_participant(g_participant);
        g_participant = NULL;
    }
    dds_runtime_deinit();
}

/*==============================================================================
 * 测试用例 - 发布者/订阅者创建
 *==============================================================================*/

/* Test 1: 创建发布者 */
void test_dds_create_publisher(void) {
    g_publisher = dds_create_publisher(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(g_publisher);
}

/* Test 2: 创建订阅者 */
void test_dds_create_subscriber(void) {
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(g_subscriber);
}

/* Test 3: 创建数据写入者 */
void test_dds_create_data_writer(void) {
    g_publisher = dds_create_publisher(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(g_publisher);
    
    g_writer = dds_create_data_writer(g_publisher, g_topic, NULL);
    TEST_ASSERT_NOT_NULL(g_writer);
}

/* Test 4: 创建数据读取者 */
void test_dds_create_data_reader(void) {
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(g_subscriber);
    
    g_reader = dds_create_data_reader(g_subscriber, g_topic, NULL);
    TEST_ASSERT_NOT_NULL(g_reader);
}

/* Test 5: 写入和读取数据 */
void test_dds_write_read_data(void) {
    /* 创建发布组件 */
    g_publisher = dds_create_publisher(g_participant, NULL);
    g_writer = dds_create_data_writer(g_publisher, g_topic, NULL);
    
    /* 创建订阅组件 */
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    g_reader = dds_create_data_reader(g_subscriber, g_topic, NULL);
    
    /* 注册回调 */
    eth_status_t status = dds_register_data_callback(g_reader, data_callback, NULL);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 写入数据 */
    const char* test_data = "Hello DDS!";
    status = dds_write(g_writer, test_data, strlen(test_data) + 1, 100);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 等待接收 */
    for (int i = 0; i < 100 && !g_data_received; i++) {
        dds_spin_once(10);
        usleep(1000);
    }
    
    TEST_ASSERT_TRUE(g_data_received);
    TEST_ASSERT_EQUAL(strlen(test_data) + 1, g_received_size);
    TEST_ASSERT_EQUAL_STRING(test_data, (char*)g_received_data);
}

/* Test 6: 多次写入读取 */
void test_dds_multiple_write_read(void) {
    g_publisher = dds_create_publisher(g_participant, NULL);
    g_writer = dds_create_data_writer(g_publisher, g_topic, NULL);
    
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    g_reader = dds_create_data_reader(g_subscriber, g_topic, NULL);
    
    dds_register_data_callback(g_reader, data_callback, NULL);
    
    /* 发送多个消息 */
    for (int i = 0; i < 10; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Message %d", i);
        
        g_data_received = 0;
        eth_status_t status = dds_write(g_writer, msg, strlen(msg) + 1, 100);
        TEST_ASSERT_EQUAL(ETH_OK, status);
        
        /* 等待接收 */
        for (int j = 0; j < 50 && !g_data_received; j++) {
            dds_spin_once(10);
            usleep(1000);
        }
        
        TEST_ASSERT_TRUE(g_data_received);
        TEST_ASSERT_EQUAL_STRING(msg, (char*)g_received_data);
    }
}

/* Test 7: 超时读取 */
void test_dds_read_timeout(void) {
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    g_reader = dds_create_data_reader(g_subscriber, g_topic, NULL);
    
    uint8_t buffer[256];
    uint32_t received;
    
    /* 尝试读取，应该超时 */
    eth_status_t status = dds_read(g_reader, buffer, sizeof(buffer), &received, 50);
    TEST_ASSERT_EQUAL(ETH_TIMEOUT, status);
}

/* Test 8: 设置发布者QoS */
void test_dds_publisher_qos(void) {
    g_publisher = dds_create_publisher(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(g_publisher);
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL,
        .deadline_ms = 100,
        .latency_budget_ms = 50,
        .history_depth = 10
    };
    
    eth_status_t status = dds_set_publisher_qos(g_publisher, &qos);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 9: 设置订阅者QoS */
void test_dds_subscriber_qos(void) {
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(g_subscriber);
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 100,
        .latency_budget_ms = 50,
        .history_depth = 10
    };
    
    eth_status_t status = dds_set_subscriber_qos(g_subscriber, &qos);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 10: 无效参数处理 */
void test_dds_pubsub_invalid_params(void) {
    /* NULL参与者创建发布者 */
    g_publisher = dds_create_publisher(NULL, NULL);
    TEST_ASSERT_NULL(g_publisher);
    
    /* NULL参与者创建订阅者 */
    g_subscriber = dds_create_subscriber(NULL, NULL);
    TEST_ASSERT_NULL(g_subscriber);
    
    /* 正常创建后测试无效参数 */
    g_publisher = dds_create_publisher(g_participant, NULL);
    g_writer = dds_create_data_writer(g_publisher, NULL, NULL);
    TEST_ASSERT_NULL(g_writer);
    
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    g_reader = dds_create_data_reader(g_subscriber, NULL, NULL);
    TEST_ASSERT_NULL(g_reader);
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_dds_create_publisher);
    RUN_TEST(test_dds_create_subscriber);
    RUN_TEST(test_dds_create_data_writer);
    RUN_TEST(test_dds_create_data_reader);
    RUN_TEST(test_dds_write_read_data);
    RUN_TEST(test_dds_multiple_write_read);
    RUN_TEST(test_dds_read_timeout);
    RUN_TEST(test_dds_publisher_qos);
    RUN_TEST(test_dds_subscriber_qos);
    RUN_TEST(test_dds_pubsub_invalid_params);
    
    return UNITY_END();
}
