/**
 * @file test_dds_domain.c
 * @brief DDS DomainParticipant单元测试
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

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    g_participant = NULL;
    dds_runtime_init();
}

void tearDown(void) {
    if (g_participant != NULL) {
        dds_delete_participant(g_participant);
        g_participant = NULL;
    }
    dds_runtime_deinit();
}

/*==============================================================================
 * 测试用例 - DomainParticipant创建
 *==============================================================================*/

/* Test 1: 创建默认域参与者 */
void test_dds_create_default_participant(void) {
    g_participant = dds_create_participant(0);
    TEST_ASSERT_NOT_NULL(g_participant);
}

/* Test 2: 创建多个域参与者 */
void test_dds_create_multiple_participants(void) {
    dds_domain_participant_t* p1 = dds_create_participant(0);
    dds_domain_participant_t* p2 = dds_create_participant(0);
    dds_domain_participant_t* p3 = dds_create_participant(1);
    
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);
    
    /* 同一域中的参与者应该不同 */
    TEST_ASSERT_NOT_EQUAL(p1, p2);
    TEST_ASSERT_NOT_EQUAL(p1, p3);
    TEST_ASSERT_NOT_EQUAL(p2, p3);
    
    dds_delete_participant(p1);
    dds_delete_participant(p2);
    dds_delete_participant(p3);
}

/* Test 3: 删除域参与者 */
void test_dds_delete_participant(void) {
    g_participant = dds_create_participant(0);
    TEST_ASSERT_NOT_NULL(g_participant);
    
    eth_status_t status = dds_delete_participant(g_participant);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    g_participant = NULL;
}

/* Test 4: 重复删除域参与者 */
void test_dds_delete_participant_twice(void) {
    g_participant = dds_create_participant(0);
    TEST_ASSERT_NOT_NULL(g_participant);
    
    eth_status_t status1 = dds_delete_participant(g_participant);
    TEST_ASSERT_EQUAL(ETH_OK, status1);
    
    /* 第二次删除应该失败 */
    eth_status_t status2 = dds_delete_participant(g_participant);
    TEST_ASSERT_NOT_EQUAL(ETH_OK, status2);
    g_participant = NULL;
}

/* Test 5: 设置和获取QoS */
void test_dds_participant_qos(void) {
    g_participant = dds_create_participant(0);
    TEST_ASSERT_NOT_NULL(g_participant);
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL,
        .deadline_ms = 100,
        .latency_budget_ms = 50,
        .history_depth = 10
    };
    
    eth_status_t status = dds_set_participant_qos(g_participant, &qos);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    dds_qos_t retrieved_qos;
    status = dds_get_participant_qos(g_participant, &retrieved_qos);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    TEST_ASSERT_EQUAL(qos.reliability, retrieved_qos.reliability);
    TEST_ASSERT_EQUAL(qos.durability, retrieved_qos.durability);
    TEST_ASSERT_EQUAL(qos.deadline_ms, retrieved_qos.deadline_ms);
}

/* Test 6: 无效参数处理 */
void test_dds_participant_invalid_params(void) {
    /* NULL参与者设置QoS */
    dds_qos_t qos = {0};
    eth_status_t status = dds_set_participant_qos(NULL, &qos);
    TEST_ASSERT_NOT_EQUAL(ETH_OK, status);
    
    /* NULL QoS指针 */
    g_participant = dds_create_participant(0);
    status = dds_set_participant_qos(g_participant, NULL);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, status);
}

/* Test 7: 多个不同域ID */
void test_dds_different_domain_ids(void) {
    dds_domain_participant_t* p0 = dds_create_participant(0);
    dds_domain_participant_t* p1 = dds_create_participant(1);
    dds_domain_participant_t* p100 = dds_create_participant(100);
    
    TEST_ASSERT_NOT_NULL(p0);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p100);
    
    /* 所有参与者都应该唯一 */
    TEST_ASSERT_NOT_EQUAL(p0, p1);
    TEST_ASSERT_NOT_EQUAL(p1, p100);
    TEST_ASSERT_NOT_EQUAL(p0, p100);
    
    dds_delete_participant(p0);
    dds_delete_participant(p1);
    dds_delete_participant(p100);
}

/* Test 8: 大量参与者创建 */
void test_dds_massive_participants(void) {
    #define MAX_PARTICIPANTS 50
    dds_domain_participant_t* participants[MAX_PARTICIPANTS];
    
    for (int i = 0; i < MAX_PARTICIPANTS; i++) {
        participants[i] = dds_create_participant(i % 10);
        TEST_ASSERT_NOT_NULL(participants[i]);
    }
    
    for (int i = 0; i < MAX_PARTICIPANTS; i++) {
        eth_status_t status = dds_delete_participant(participants[i]);
        TEST_ASSERT_EQUAL(ETH_OK, status);
    }
}

/* Test 9: 参与者状态保持 */
void test_dds_participant_state_persistence(void) {
    g_participant = dds_create_participant(0);
    TEST_ASSERT_NOT_NULL(g_participant);
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_PERSISTENT,
        .deadline_ms = 200,
        .latency_budget_ms = 100,
        .history_depth = 20
    };
    
    eth_status_t status = dds_set_participant_qos(g_participant, &qos);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 运行几次spin_once */
    for (int i = 0; i < 10; i++) {
        dds_spin_once(10);
    }
    
    /* 检查QoS仍然有效 */
    dds_qos_t retrieved_qos;
    status = dds_get_participant_qos(g_participant, &retrieved_qos);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_EQUAL(qos.durability, retrieved_qos.durability);
}

/* Test 10: 参与者应用程序生命周期 */
void test_dds_participant_lifecycle(void) {
    /* 创建参与者 */
    g_participant = dds_create_participant(0);
    TEST_ASSERT_NOT_NULL(g_participant);
    
    /* 创建发布者 */
    dds_publisher_t* publisher = dds_create_publisher(g_participant, NULL);
    TEST_ASSERT_NOT_NULL(publisher);
    
    /* 在删除参与者之前删除发布者 */
    eth_status_t status = dds_delete_publisher(publisher);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 删除参与者 */
    status = dds_delete_participant(g_participant);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    g_participant = NULL;
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_dds_create_default_participant);
    RUN_TEST(test_dds_create_multiple_participants);
    RUN_TEST(test_dds_delete_participant);
    RUN_TEST(test_dds_delete_participant_twice);
    RUN_TEST(test_dds_participant_qos);
    RUN_TEST(test_dds_participant_invalid_params);
    RUN_TEST(test_dds_different_domain_ids);
    RUN_TEST(test_dds_massive_participants);
    RUN_TEST(test_dds_participant_state_persistence);
    RUN_TEST(test_dds_participant_lifecycle);
    
    return UNITY_END();
}
