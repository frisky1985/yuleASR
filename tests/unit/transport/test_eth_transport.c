/**
 * @file test_eth_transport.c
 * @brief 以太网传输单元测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unity.h"
#include "../../../tests/mocks/mock_eth_driver.h"
#include "../../../src/common/types/eth_types.h"
#include "../../../src/transport/transport_interface.h"

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static mock_eth_handle_t g_eth_handle = NULL;
static eth_config_t g_config;
static uint8_t g_rx_buffer[2048];
static volatile int g_packet_received = 0;
static uint32_t g_rx_length = 0;

/*==============================================================================
 * 回调函数
 *==============================================================================*/

static void rx_callback(eth_buffer_t* buffer, void* user_data) {
    (void)user_data;
    if (buffer->len <= sizeof(g_rx_buffer)) {
        memcpy(g_rx_buffer, buffer->data, buffer->len);
        g_rx_length = buffer->len;
        g_packet_received = 1;
    }
}

/*==============================================================================
 * Unity Setup/Teardown
 *==============================================================================*/

void setUp(void) {
    memset(&g_config, 0, sizeof(g_config));
    g_config.mac_addr[0] = 0x00;
    g_config.mac_addr[1] = 0x11;
    g_config.mac_addr[2] = 0x22;
    g_config.mac_addr[3] = 0x33;
    g_config.mac_addr[4] = 0x44;
    g_config.mac_addr[5] = 0x55;
    g_config.mode = ETH_MODE_100M_FULL;
    g_config.enable_dma = true;
    g_config.rx_buffer_size = 2048;
    g_config.tx_buffer_size = 2048;
    
    g_eth_handle = mock_eth_driver_init(&g_config);
    g_packet_received = 0;
    g_rx_length = 0;
    memset(g_rx_buffer, 0, sizeof(g_rx_buffer));
}

void tearDown(void) {
    if (g_eth_handle != NULL) {
        mock_eth_driver_stop(g_eth_handle);
        mock_eth_driver_deinit(g_eth_handle);
        g_eth_handle = NULL;
    }
    mock_eth_reset_all();
}

/*==============================================================================
 * 测试用例 - 以太网驱动初始化
 *==============================================================================*/

/* Test 1: 驱动初始化 */
void test_eth_driver_init(void) {
    TEST_ASSERT_NOT_NULL(g_eth_handle);
}

/* Test 2: 驱动启动和停止 */
void test_eth_driver_start_stop(void) {
    eth_status_t status = mock_eth_driver_start(g_eth_handle);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    status = mock_eth_driver_stop(g_eth_handle);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 3: 设置链路状态 */
void test_eth_link_status(void) {
    eth_status_t status = mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    eth_link_status_t link_status;
    status = mock_eth_get_link_status(g_eth_handle, &link_status);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_EQUAL(ETH_LINK_UP, link_status);
    
    status = mock_eth_set_link_status(g_eth_handle, ETH_LINK_DOWN);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 4: 注册接收回调 */
void test_eth_register_rx_callback(void) {
    eth_status_t status = mock_eth_register_rx_callback(g_eth_handle, rx_callback, NULL);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 5: 发送数据包 */
void test_eth_send_packet(void) {
    /* 首先启用链路 */
    mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    
    eth_mac_addr_t dst_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data[] = "Hello Ethernet!";
    
    eth_status_t status = mock_eth_send_packet(g_eth_handle, dst_addr, data, sizeof(data));
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 验证统计 */
    mock_eth_stats_t stats;
    status = mock_eth_get_stats(g_eth_handle, &stats);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_EQUAL(1, stats.tx_packets);
    TEST_ASSERT_EQUAL(sizeof(data), stats.tx_bytes);
}

/* Test 6: 接收数据包 */
void test_eth_receive_packet(void) {
    mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    mock_eth_register_rx_callback(g_eth_handle, rx_callback, NULL);
    mock_eth_driver_start(g_eth_handle);
    
    eth_mac_addr_t src_addr = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t data[] = "Test packet data";
    
    eth_status_t status = mock_eth_inject_rx_packet(g_eth_handle, src_addr, data, sizeof(data));
    TEST_ASSERT_EQUAL(ETH_OK, status);
    
    /* 等待接收 */
    for (int i = 0; i < 100 && !g_packet_received; i++) {
        usleep(1000);
    }
    
    TEST_ASSERT_TRUE(g_packet_received);
    TEST_ASSERT_EQUAL(sizeof(data), g_rx_length);
    TEST_ASSERT_EQUAL_MEMORY(data, g_rx_buffer, sizeof(data));
}

/* Test 7: VLAN数据包发送 */
void test_eth_vlan_packet(void) {
    mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    mock_eth_enable_vlan(true);
    
    eth_mac_addr_t dst_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data[] = "VLAN packet";
    
    eth_status_t status = mock_eth_send_vlan_packet(g_eth_handle, dst_addr, 100, 5, data, sizeof(data));
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

/* Test 8: 多次发送 */
void test_eth_multiple_packets(void) {
    mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    
    eth_mac_addr_t dst_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    for (int i = 0; i < 100; i++) {
        uint8_t data[64];
        snprintf((char*)data, sizeof(data), "Packet %d", i);
        
        eth_status_t status = mock_eth_send_packet(g_eth_handle, dst_addr, data, sizeof(data));
        TEST_ASSERT_EQUAL(ETH_OK, status);
    }
    
    mock_eth_stats_t stats;
    mock_eth_get_stats(g_eth_handle, &stats);
    TEST_ASSERT_EQUAL(100, stats.tx_packets);
}

/* Test 9: 统计重置 */
void test_eth_reset_stats(void) {
    mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    
    eth_mac_addr_t dst_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data[] = "test";
    mock_eth_send_packet(g_eth_handle, dst_addr, data, sizeof(data));
    
    mock_eth_reset_stats(g_eth_handle);
    
    mock_eth_stats_t stats;
    mock_eth_get_stats(g_eth_handle, &stats);
    TEST_ASSERT_EQUAL(0, stats.tx_packets);
    TEST_ASSERT_EQUAL(0, stats.tx_bytes);
}

/* Test 10: 模拟延迟和丢包 */
void test_eth_simulation_features(void) {
    mock_eth_set_link_status(g_eth_handle, ETH_LINK_UP);
    
    /* 设置延迟 */
    mock_eth_set_latency(100, 200);
    
    /* 设置丢包率 (5%) */
    mock_eth_set_packet_loss(500);
    
    eth_mac_addr_t dst_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t data[] = "test";
    
    int success_count = 0;
    for (int i = 0; i < 100; i++) {
        eth_status_t status = mock_eth_send_packet(g_eth_handle, dst_addr, data, sizeof(data));
        if (status == ETH_OK) {
            success_count++;
        }
    }
    
    /* 验证丢包模拟工作（大约 95% 成功率） */
    TEST_ASSERT_LESS_THAN(100, success_count);
    TEST_ASSERT_GREATER_THAN(80, success_count);
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_eth_driver_init);
    RUN_TEST(test_eth_driver_start_stop);
    RUN_TEST(test_eth_link_status);
    RUN_TEST(test_eth_register_rx_callback);
    RUN_TEST(test_eth_send_packet);
    RUN_TEST(test_eth_receive_packet);
    RUN_TEST(test_eth_vlan_packet);
    RUN_TEST(test_eth_multiple_packets);
    RUN_TEST(test_eth_reset_stats);
    RUN_TEST(test_eth_simulation_features);
    
    return UNITY_END();
}
