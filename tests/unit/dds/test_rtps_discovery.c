/**
 * @file test_rtps_discovery.c
 * @brief RTPS发现机制单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "../../../src/dds/rtps/rtps_discovery.h"
#include "../../../src/common/types/eth_types.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static rtps_discovery_context_t g_context;
static rtps_discovery_config_t g_config;
static int g_participant_discovered = 0;
static int g_endpoint_matched = 0;
static int g_discovery_complete = 0;

/*==============================================================================
 * 回调函数
 *==============================================================================*/

static void on_participant_discovered(rtps_participant_proxy_t* participant) {
    (void)participant;
    g_participant_discovered++;
}

static void on_endpoint_matched(rtps_endpoint_proxy_t* endpoint, bool matched) {
    (void)endpoint;
    if (matched) {
        g_endpoint_matched++;
    }
}

static void on_discovery_complete(void) {
    g_discovery_complete = 1;
}

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    memset(&g_context, 0, sizeof(g_context));
    memset(&g_config, 0, sizeof(g_config));
    
    g_config.type = RTPS_DISCOVERY_TYPE_SIMPLE;
    g_config.domain_id = 0;
    g_config.initial_announce_period_ms = 10;
    g_config.announce_period_ms = 20;
    g_config.lease_duration_ms = 1000;
    g_config.match_timeout_ms = 100;
    g_config.max_participants = 10;
    g_config.max_endpoints_per_participant = 20;
    
    g_participant_discovered = 0;
    g_endpoint_matched = 0;
    g_discovery_complete = 0;
}

void tearDown(void) {
    if (g_context.initialized) {
        rtps_discovery_deinit(&g_context);
    }
}

/*==============================================================================
 * 测试用例 - 发现协议初始化
 *==============================================================================*/

/* Test 1: 初始化发现协议 */
void test_rtps_discovery_init(void) {
    eth_status_t status = rtps_discovery_init(&g_context, &g_config);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_TRUE(g_context.initialized);
}

/* Test 2: 反初始化发现协议 */
void test_rtps_discovery_deinit(void) {
    eth_status_t status = rtps_discovery_init(&g_context, &g_config);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    rtps_discovery_deinit(&g_context);
    TEST_ASSERT_FALSE(g_context.initialized);
}

/* Test 3: 启动和停止发现 */
void test_rtps_discovery_start_stop(void) {
    rtps_discovery_init(&g_context, &g_config);
    
    eth_status_t status = rtps_discovery_start(&g_context);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_TRUE(g_context.active);
    
    rtps_discovery_stop(&g_context);
    TEST_ASSERT_FALSE(g_context.active);
}

/* Test 4: 设置回调函数 */
void test_rtps_discovery_callbacks(void) {
    rtps_discovery_init(&g_context, &g_config);
    
    rtps_discovery_set_callbacks(&g_context,
                                  on_participant_discovered,
                                  on_endpoint_matched,
                                  on_discovery_complete);
    
    TEST_ASSERT_NOT_NULL(g_context.on_participant_discovered);
    TEST_ASSERT_NOT_NULL(g_context.on_endpoint_matched);
    TEST_ASSERT_NOT_NULL(g_context.on_discovery_complete);
}

/* Test 5: 添加本地端点 */
void test_rtps_discovery_add_endpoint(void) {
    rtps_discovery_init(&g_context, &g_config);
    rtps_discovery_start(&g_context);
    
    rtps_endpoint_proxy_t endpoint = {0};
    memcpy(endpoint.topic_name, "TestTopic", 10);
    memcpy(endpoint.type_name, "TestType", 9);
    endpoint.type = RTPS_ENDPOINT_TYPE_WRITER;
    
    eth_status_t status = rtps_discovery_add_local_endpoint(&g_context, &endpoint);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 6: 移除本地端点 */
void test_rtps_discovery_remove_endpoint(void) {
    rtps_discovery_init(&g_context, &g_config);
    rtps_discovery_start(&g_context);
    
    rtps_endpoint_proxy_t endpoint = {0};
    endpoint.guid.entity_id[3] = 1;
    memcpy(endpoint.topic_name, "TestTopic", 10);
    endpoint.type = RTPS_ENDPOINT_TYPE_WRITER;
    
    rtps_discovery_add_local_endpoint(&g_context, &endpoint);
    
    eth_status_t status = rtps_discovery_remove_local_endpoint(&g_context, &endpoint.guid);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 7: 发现端口计算 */
void test_rtps_discovery_port_calculation(void) {
    uint16_t port = rtps_discovery_get_port(0, 0, true);
    TEST_ASSERT_EQUAL(7400, port);
    
    port = rtps_discovery_get_port(0, 0, false);
    TEST_ASSERT_EQUAL(7410, port);
    
    port = rtps_discovery_get_port(1, 0, true);
    TEST_ASSERT_EQUAL(7650, port);
}

/* Test 8: GUID生成 */
void test_rtps_guid_generation(void) {
    eth_mac_addr_t mac = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    rtps_guid_prefix_t prefix;
    
    eth_status_t status = rtps_guid_prefix_generate(prefix, mac, 0);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 检查MAC地址是否在GUID中 */
    TEST_ASSERT_EQUAL(mac[0], prefix[0]);
    TEST_ASSERT_EQUAL(mac[1], prefix[1]);
    TEST_ASSERT_EQUAL(mac[2], prefix[2]);
}

/* Test 9: GUID比较 */
void test_rtps_guid_compare(void) {
    rtps_guid_t guid1 = {0};
    rtps_guid_t guid2 = {0};
    
    guid1.prefix[0] = 1;
    guid2.prefix[0] = 1;
    guid1.entity_id[0] = 2;
    guid2.entity_id[0] = 2;
    
    TEST_ASSERT_TRUE(rtps_guid_equal(&guid1, &guid2));
    
    guid2.entity_id[0] = 3;
    TEST_ASSERT_FALSE(rtps_guid_equal(&guid1, &guid2));
}

/* Test 10: 序列号操作 */
void test_rtps_sequence_number_ops(void) {
    rtps_sequence_number_t seq1 = {0, 1};
    rtps_sequence_number_t seq2 = {0, 2};
    
    int cmp = rtps_seqnum_compare(&seq1, &seq2);
    TEST_ASSERT_LESS_THAN(0, cmp);
    
    cmp = rtps_seqnum_compare(&seq2, &seq1);
    TEST_ASSERT_GREATER_THAN(0, cmp);
    
    rtps_sequence_number_t seq3 = {0, 1};
    cmp = rtps_seqnum_compare(&seq1, &seq3);
    TEST_ASSERT_EQUAL(0, cmp);
    
    /* 递增测试 */
    rtps_seqnum_increment(&seq1);
    cmp = rtps_seqnum_compare(&seq1, &seq3);
    TEST_ASSERT_GREATER_THAN(0, cmp);
}

/* Test 11: 发现完成检查 */
void test_rtps_discovery_is_complete(void) {
    rtps_discovery_init(&g_context, &g_config);
    
    /* 初始状态应该完成 */
    bool complete = rtps_discovery_is_complete(&g_context);
    TEST_ASSERT_TRUE(complete);
}

/* Test 12: 统计信息获取 */
void test_rtps_discovery_stats(void) {
    rtps_discovery_init(&g_context, &g_config);
    rtps_discovery_start(&g_context);
    
    uint32_t participants, endpoints, elapsed;
    eth_status_t status = rtps_discovery_get_stats(&g_context, &participants, &endpoints, &elapsed);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_rtps_discovery_init);
    RUN_TEST(test_rtps_discovery_deinit);
    RUN_TEST(test_rtps_discovery_start_stop);
    RUN_TEST(test_rtps_discovery_callbacks);
    RUN_TEST(test_rtps_discovery_add_endpoint);
    RUN_TEST(test_rtps_discovery_remove_endpoint);
    RUN_TEST(test_rtps_discovery_port_calculation);
    RUN_TEST(test_rtps_guid_generation);
    RUN_TEST(test_rtps_guid_compare);
    RUN_TEST(test_rtps_sequence_number_ops);
    RUN_TEST(test_rtps_discovery_is_complete);
    RUN_TEST(test_rtps_discovery_stats);
    
    return UNITY_END();
}
