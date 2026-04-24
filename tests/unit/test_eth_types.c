/**
 * @file test_eth_types.c
 * @brief 以太网类型系统单元测试
 * @version 1.0
 * @date 2026-04-24
 */

#include "unity.h"
#include "eth_types.h"
#include <string.h>

/* 测试前置准备 */
void setUp(void)
{
    /* 每个测试用例执行前调用 */
}

void tearDown(void)
{
    /* 每个测试用例执行后调用 */
}

/* ============================================================================
 * MAC地址测试
 * ============================================================================ */
void test_eth_types_MAC_ADDR_macro(void)
{
    eth_mac_addr_t mac = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
    
    TEST_ASSERT_EQUAL_HEX(0x00, mac[0]);
    TEST_ASSERT_EQUAL_HEX(0x11, mac[1]);
    TEST_ASSERT_EQUAL_HEX(0x22, mac[2]);
    TEST_ASSERT_EQUAL_HEX(0x33, mac[3]);
    TEST_ASSERT_EQUAL_HEX(0x44, mac[4]);
    TEST_ASSERT_EQUAL_HEX(0x55, mac[5]);
}

void test_eth_types_ETH_MAC_IS_EQUAL_macro(void)
{
    eth_mac_addr_t mac1 = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
    eth_mac_addr_t mac2 = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
    eth_mac_addr_t mac3 = ETH_MAC_ADDR(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    
    TEST_ASSERT_TRUE(ETH_MAC_IS_EQUAL(mac1, mac2));
    TEST_ASSERT_FALSE(ETH_MAC_IS_EQUAL(mac1, mac3));
}

void test_eth_types_ETH_MAC_IS_BROADCAST_macro(void)
{
    eth_mac_addr_t broadcast = ETH_MAC_ADDR(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    eth_mac_addr_t unicast = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
    
    TEST_ASSERT_TRUE(ETH_MAC_IS_BROADCAST(broadcast));
    TEST_ASSERT_FALSE(ETH_MAC_IS_BROADCAST(unicast));
}

/* ============================================================================
 * IP地址测试
 * ============================================================================ */
void test_eth_types_ETH_IP_ADDR_macro(void)
{
    eth_ip_addr_t ip1 = ETH_IP_ADDR(192, 168, 1, 1);
    eth_ip_addr_t ip2 = ETH_IP_ADDR(10, 0, 0, 1);
    
    TEST_ASSERT_EQUAL_HEX(0xC0A80101, ip1);  /* 192.168.1.1 */
    TEST_ASSERT_EQUAL_HEX(0x0A000001, ip2);  /* 10.0.0.1 */
}

/* ============================================================================
 * 状态码测试
 * ============================================================================ */
void test_eth_types_status_values(void)
{
    TEST_ASSERT_EQUAL_INT(0, ETH_OK);
    TEST_ASSERT_EQUAL_INT(-1, ETH_ERROR);
    TEST_ASSERT_EQUAL_INT(-2, ETH_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(-3, ETH_INVALID_PARAM);
    TEST_ASSERT_EQUAL_INT(-4, ETH_NO_MEMORY);
    TEST_ASSERT_EQUAL_INT(-5, ETH_BUSY);
    TEST_ASSERT_EQUAL_INT(-6, ETH_NOT_INIT);
}

/* ============================================================================
 * 以太网配置结构测试
 * ============================================================================ */
void test_eth_types_eth_config_size(void)
{
    /* 验证eth_config_t的大小不为0 */
    eth_config_t config;
    TEST_ASSERT_NOT_NULL(&config);
    
    /* 验证结构体大小在合理范围内 */
    size_t config_size = sizeof(eth_config_t);
    TEST_ASSERT_TRUE(config_size > 0);
    TEST_ASSERT_TRUE(config_size < 1024);  /* 应该小于1KB */
}

void test_eth_types_eth_config_defaults(void)
{
    eth_config_t config;
    memset(&config, 0, sizeof(config));
    
    /* 检查默认值 */
    TEST_ASSERT_EQUAL_UINT(0, config.enable_dma);
    TEST_ASSERT_EQUAL_UINT(0, config.enable_checksum_offload);
    TEST_ASSERT_EQUAL_UINT(0, config.rx_buffer_size);
    TEST_ASSERT_EQUAL_UINT(0, config.tx_buffer_size);
}

/* ============================================================================
 * 缓冲区结构测试
 * ============================================================================ */
void test_eth_types_eth_buffer_init(void)
{
    uint8_t data[100];
    eth_buffer_t buffer;
    buffer.data = data;
    buffer.len = 0;
    buffer.max_len = 100;
    
    TEST_ASSERT_NOT_NULL(buffer.data);
    TEST_ASSERT_EQUAL_UINT(0, buffer.len);
    TEST_ASSERT_EQUAL_UINT(100, buffer.max_len);
    TEST_ASSERT_EQUAL_MEMORY(data, buffer.data, 100);
}

/* ============================================================================
 * DDS QoS结构测试
 * ============================================================================ */
void test_eth_types_dds_qos_defaults(void)
{
    dds_qos_t qos;
    memset(&qos, 0, sizeof(qos));
    
    qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos.durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL;
    qos.deadline_ms = 1000;
    qos.latency_budget_ms = 100;
    qos.history_depth = 10;
    
    TEST_ASSERT_EQUAL_INT(DDS_QOS_RELIABILITY_RELIABLE, qos.reliability);
    TEST_ASSERT_EQUAL_INT(DDS_QOS_DURABILITY_TRANSIENT_LOCAL, qos.durability);
    TEST_ASSERT_EQUAL_UINT(1000, qos.deadline_ms);
    TEST_ASSERT_EQUAL_UINT(100, qos.latency_budget_ms);
    TEST_ASSERT_EQUAL_UINT(10, qos.history_depth);
}

void test_eth_types_dds_qos_reliability_values(void)
{
    TEST_ASSERT_EQUAL_INT(0, DDS_QOS_RELIABILITY_BEST_EFFORT);
    TEST_ASSERT_EQUAL_INT(1, DDS_QOS_RELIABILITY_RELIABLE);
}

void test_eth_types_dds_qos_durability_values(void)
{
    TEST_ASSERT_EQUAL_INT(0, DDS_QOS_DURABILITY_VOLATILE);
    TEST_ASSERT_EQUAL_INT(1, DDS_QOS_DURABILITY_TRANSIENT_LOCAL);
    TEST_ASSERT_EQUAL_INT(2, DDS_QOS_DURABILITY_TRANSIENT);
    TEST_ASSERT_EQUAL_INT(3, DDS_QOS_DURABILITY_PERSISTENT);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    
    /* MAC地址测试 */
    RUN_TEST(test_eth_types_MAC_ADDR_macro);
    RUN_TEST(test_eth_types_ETH_MAC_IS_EQUAL_macro);
    RUN_TEST(test_eth_types_ETH_MAC_IS_BROADCAST_macro);
    
    /* IP地址测试 */
    RUN_TEST(test_eth_types_ETH_IP_ADDR_macro);
    
    /* 状态码测试 */
    RUN_TEST(test_eth_types_status_values);
    
    /* 配置结构测试 */
    RUN_TEST(test_eth_types_eth_config_size);
    RUN_TEST(test_eth_types_eth_config_defaults);
    
    /* 缓冲区测试 */
    RUN_TEST(test_eth_types_eth_buffer_init);
    
    /* DDS QoS测试 */
    RUN_TEST(test_eth_types_dds_qos_defaults);
    RUN_TEST(test_eth_types_dds_qos_reliability_values);
    RUN_TEST(test_eth_types_dds_qos_durability_values);
    
    return UNITY_END();
}
