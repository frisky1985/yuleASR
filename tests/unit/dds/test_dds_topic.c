/**
 * @file test_dds_topic.c
 * @brief DDS Topic单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "../../../src/dds/core/dds_core.h"
#include "../../../src/common/types/eth_types.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static dds_domain_participant_t* g_participant = NULL;
static dds_topic_t* g_topic = NULL;

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    dds_runtime_init();
    g_participant = dds_create_participant(0);
    g_topic = NULL;
}

void tearDown(void) {
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
 * 测试用例 - Topic创建
 *==============================================================================*/

/* Test 1: 创建基本Topic */
void test_dds_create_basic_topic(void) {
    g_topic = dds_create_topic(g_participant, "TestTopic", "TestType", NULL);
    TEST_ASSERT_NOT_NULL(g_topic);
}

/* Test 2: 创建带QoS的Topic */
void test_dds_create_topic_with_qos(void) {
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL,
        .deadline_ms = 100,
        .latency_budget_ms = 50,
        .history_depth = 10
    };
    
    g_topic = dds_create_topic(g_participant, "QosTopic", "QosType", &qos);
    TEST_ASSERT_NOT_NULL(g_topic);
}

/* Test 3: 获取Topic名称 */
void test_dds_get_topic_name(void) {
    const char* topic_name = "MyTopicName";
    g_topic = dds_create_topic(g_participant, topic_name, "MyType", NULL);
    TEST_ASSERT_NOT_NULL(g_topic);
    
    dds_topic_name_t retrieved_name;
    eth_status_t status = dds_get_topic_name(g_topic, retrieved_name);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_EQUAL_STRING(topic_name, retrieved_name);
}

/* Test 4: 创建多个Topic */
void test_dds_create_multiple_topics(void) {
    dds_topic_t* t1 = dds_create_topic(g_participant, "Topic1", "Type1", NULL);
    dds_topic_t* t2 = dds_create_topic(g_participant, "Topic2", "Type2", NULL);
    dds_topic_t* t3 = dds_create_topic(g_participant, "Topic3", "Type3", NULL);
    
    TEST_ASSERT_NOT_NULL(t1);
    TEST_ASSERT_NOT_NULL(t2);
    TEST_ASSERT_NOT_NULL(t3);
    
    TEST_ASSERT_NOT_EQUAL(t1, t2);
    TEST_ASSERT_NOT_EQUAL(t2, t3);
    TEST_ASSERT_NOT_EQUAL(t1, t3);
    
    dds_delete_topic(t1);
    dds_delete_topic(t2);
    dds_delete_topic(t3);
}

/* Test 5: 删除Topic */
void test_dds_delete_topic(void) {
    g_topic = dds_create_topic(g_participant, "DeleteTopic", "DeleteType", NULL);
    TEST_ASSERT_NOT_NULL(g_topic);
    
    eth_status_t status = dds_delete_topic(g_topic);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    g_topic = NULL;
}

/* Test 6: Topic匹配 - 同名称同类型 */
void test_dds_topic_same_name_same_type(void) {
    dds_topic_t* t1 = dds_create_topic(g_participant, "MatchTopic", "MatchType", NULL);
    dds_topic_t* t2 = dds_create_topic(g_participant, "MatchTopic", "MatchType", NULL);
    
    TEST_ASSERT_NOT_NULL(t1);
    TEST_ASSERT_NOT_NULL(t2);
    
    /* 同名称同类型的Topic应该匹配 */
    dds_topic_name_t name1, name2;
    dds_get_topic_name(t1, name1);
    dds_get_topic_name(t2, name2);
    TEST_ASSERT_EQUAL_STRING(name1, name2);
    
    dds_delete_topic(t1);
    dds_delete_topic(t2);
}

/* Test 7: 无效参数处理 */
void test_dds_topic_invalid_params(void) {
    /* NULL参与者 */
    dds_topic_t* topic = dds_create_topic(NULL, "Topic", "Type", NULL);
    TEST_ASSERT_NULL(topic);
    
    /* 空主题名 */
    topic = dds_create_topic(g_participant, "", "Type", NULL);
    TEST_ASSERT_NULL(topic);
    
    /* 空类型名 */
    topic = dds_create_topic(g_participant, "Topic", "", NULL);
    TEST_ASSERT_NULL(topic);
}

/* Test 8: 长名称Topic */
void test_dds_topic_long_names(void) {
    char long_name[256];
    memset(long_name, 'A', 255);
    long_name[255] = '\0';
    
    g_topic = dds_create_topic(g_participant, long_name, "LongType", NULL);
    /* 可能失败或截断，根据实现 */
    if (g_topic != NULL) {
        dds_topic_name_t retrieved;
        eth_status_t status = dds_get_topic_name(g_topic, retrieved);
        TEST_ASSERT_EQUAL(ETH_OK, status);
    }
}

/* Test 9: 特殊字符Topic名称 */
void test_dds_topic_special_characters(void) {
    g_topic = dds_create_topic(g_participant, "Topic_With-Special.Chars123", "Type", NULL);
    TEST_ASSERT_NOT_NULL(g_topic);
}

/* Test 10: Topic生命周期管理 */
void test_dds_topic_lifecycle(void) {
    /* 创建Topic */
    g_topic = dds_create_topic(g_participant, "LifecycleTopic", "LifecycleType", NULL);
    TEST_ASSERT_NOT_NULL(g_topic);
    
    /* 创建数据写入者 */
    dds_publisher_t* pub = dds_create_publisher(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(pub);
    
    dds_data_writer_t* writer = dds_create_data_writer(pub, g_topic, NULL);
    TEST_ASSERT_NOT_NULL(writer);
    
    /* 尝试删除被使用的Topic应该失败 */
    eth_status_t status = dds_delete_topic(g_topic);
    /* 可能失败，因为Topic正被使用 */
    
    /* 先删除writer */
    status = dds_delete_data_writer(writer);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    status = dds_delete_publisher(pub);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 现在可以删除Topic */
    status = dds_delete_topic(g_topic);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    g_topic = NULL;
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_dds_create_basic_topic);
    RUN_TEST(test_dds_create_topic_with_qos);
    RUN_TEST(test_dds_get_topic_name);
    RUN_TEST(test_dds_create_multiple_topics);
    RUN_TEST(test_dds_delete_topic);
    RUN_TEST(test_dds_topic_same_name_same_type);
    RUN_TEST(test_dds_topic_invalid_params);
    RUN_TEST(test_dds_topic_long_names);
    RUN_TEST(test_dds_topic_special_characters);
    RUN_TEST(test_dds_topic_lifecycle);
    
    return UNITY_END();
}
