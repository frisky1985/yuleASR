/**
 * @file test_eth_driver.c
 * @brief 车载以太网驱动层测试
 * @version 1.0
 * @date 2026-04-24
 */

#include "unity.h"
#include "ethernet/eth_manager.h"
#include "ethernet/driver/eth_mac_driver.h"
#include "ethernet/driver/eth_dma.h"
#include "ethernet/driver/eth_automotive.h"
#include "common/types/eth_types.h"
#include <string.h>

/* 测试配置 */
static eth_manager_config_t g_test_config;

void setUp(void)
{
    memset(&g_test_config, 0, sizeof(g_test_config));

    /* 配置MAC */
    g_test_config.mac_config.mac_type = ETH_MAC_TYPE_GENERIC;
    g_test_config.mac_config.mac_addr[0] = 0x00;
    g_test_config.mac_config.mac_addr[1] = 0x11;
    g_test_config.mac_config.mac_addr[2] = 0x22;
    g_test_config.mac_config.mac_addr[3] = 0x33;
    g_test_config.mac_config.mac_addr[4] = 0x44;
    g_test_config.mac_config.mac_addr[5] = 0x55;
    g_test_config.mac_config.speed_mode = ETH_MODE_100M_FULL;
    g_test_config.mac_config.mac_mode = ETH_MAC_MODE_NORMAL;
    g_test_config.mac_config.asil_level = ETH_ASIL_B;
    g_test_config.mac_config.base_addr = 0x40000000;
    g_test_config.mac_config.clock_freq_hz = 50000000;

    /* 配置DMA */
    g_test_config.dma_config.rx_desc_count = 16;
    g_test_config.dma_config.tx_desc_count = 16;
    g_test_config.dma_config.buffer_size = 1536;
    g_test_config.dma_config.mode = ETH_DMA_MODE_POLLING;
    g_test_config.dma_config.rx_interrupt = true;
    g_test_config.dma_config.tx_interrupt = true;

    /* 配置PHY */
    g_test_config.phy_config.phy_type = AUTOMOTIVE_PHY_GENERIC;
    g_test_config.phy_config.standard = AUTOMOTIVE_ETH_STANDARD_100BASE_T1;
    g_test_config.phy_config.phy_addr = 1;
    g_test_config.phy_config.enable_master_mode = true;

    /* 配置管理器 */
    g_test_config.link_monitor.enable_link_monitoring = true;
    g_test_config.link_monitor.link_check_interval_ms = 100;
    g_test_config.negotiation.enable_auto_negotiation = false;
}

void tearDown(void)
{
    eth_manager_deinit();
}

/* ============================================================================
 * MAC驱动测试
 * ============================================================================ */

void test_eth_mac_init_success(void)
{
    eth_status_t status = eth_mac_init(&g_test_config.mac_config);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    eth_mac_deinit();
}

void test_eth_mac_init_null_config(void)
{
    eth_status_t status = eth_mac_init(NULL);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, status);
}

void test_eth_mac_address_operations(void)
{
    eth_mac_init(&g_test_config.mac_config);

    eth_mac_addr_t read_addr;
    eth_status_t status = eth_mac_get_address(read_addr);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_TRUE(ETH_MAC_IS_EQUAL(read_addr, g_test_config.mac_config.mac_addr));

    eth_mac_addr_t new_addr = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    status = eth_mac_set_address(new_addr);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    eth_mac_get_address(read_addr);
    TEST_ASSERT_TRUE(ETH_MAC_IS_EQUAL(read_addr, new_addr));

    eth_mac_deinit();
}

void test_eth_mac_mode_operations(void)
{
    eth_mac_init(&g_test_config.mac_config);

    eth_status_t status = eth_mac_set_mode(ETH_MAC_MODE_LOOPBACK);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    status = eth_mac_set_mode(ETH_MAC_MODE_PROMISCUOUS);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    eth_mac_deinit();
}

/* ============================================================================
 * DMA测试
 * ============================================================================ */

void test_eth_dma_init_success(void)
{
    eth_status_t status = eth_dma_init(&g_test_config.dma_config);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    eth_dma_deinit();
}

void test_eth_dma_init_invalid_desc_count(void)
{
    eth_dma_config_t config = g_test_config.dma_config;
    config.rx_desc_count = 0;

    eth_status_t status = eth_dma_init(&config);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, status);
}

void test_eth_dma_state_transitions(void)
{
    eth_dma_init(&g_test_config.dma_config);

    eth_dma_state_t state;
    eth_status_t status = eth_dma_get_state(&state);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_EQUAL(ETH_DMA_STATE_INIT, state);

    eth_dma_start();
    eth_dma_get_state(&state);
    TEST_ASSERT_EQUAL(ETH_DMA_STATE_RUNNING, state);

    eth_dma_stop();
    eth_dma_get_state(&state);
    TEST_ASSERT_EQUAL(ETH_DMA_STATE_STOPPED, state);

    eth_dma_deinit();
}

/* ============================================================================
 * 汽车以太网PHY测试
 * ============================================================================ */

void test_automotive_phy_init_success(void)
{
    eth_status_t status = automotive_phy_init(&g_test_config.phy_config);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    automotive_phy_deinit();
}

void test_automotive_phy_link_status(void)
{
    automotive_phy_init(&g_test_config.phy_config);

    automotive_link_status_t status;
    eth_status_t ret = automotive_phy_get_link_status(&status);
    TEST_ASSERT_EQUAL(ETH_OK, ret);

    /* 初始状态应该是link_down */
    TEST_ASSERT_FALSE(status.link_up);

    automotive_phy_deinit();
}

void test_automotive_phy_stats(void)
{
    automotive_phy_init(&g_test_config.phy_config);

    automotive_phy_stats_t stats;
    eth_status_t status = automotive_phy_get_stats(&stats);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    /* 初始化后所有统计应该为0 */
    TEST_ASSERT_EQUAL(0, stats.link_up_count);
    TEST_ASSERT_EQUAL(0, stats.link_down_count);

    automotive_phy_deinit();
}

void test_automotive_phy_master_mode(void)
{
    automotive_phy_init(&g_test_config.phy_config);

    eth_status_t status = automotive_phy_set_master_mode(true);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    automotive_link_status_t link_status;
    automotive_phy_get_link_status(&link_status);
    TEST_ASSERT_TRUE(link_status.is_master);
    TEST_ASSERT_TRUE(link_status.master_slave_resolved);

    automotive_phy_deinit();
}

/* ============================================================================
 * 以太网管理器测试
 * ============================================================================ */

void test_eth_manager_init_success(void)
{
    eth_status_t status = eth_manager_init(&g_test_config);
    TEST_ASSERT_EQUAL(ETH_OK, status);
}

void test_eth_manager_init_null_config(void)
{
    eth_status_t status = eth_manager_init(NULL);
    TEST_ASSERT_EQUAL(ETH_INVALID_PARAM, status);
}

void test_eth_manager_state_transitions(void)
{
    eth_manager_init(&g_test_config);

    eth_manager_state_t state;
    eth_status_t status = eth_manager_get_state(&state);
    TEST_ASSERT_EQUAL(ETH_OK, status);
    TEST_ASSERT_EQUAL(ETH_MANAGER_STATE_INIT, state);

    /* 启动后状态会变化 */
    eth_manager_start();
    eth_manager_get_state(&state);
    /* 可能是LINK_DOWN或LINK_UP */
    TEST_ASSERT_TRUE(state == ETH_MANAGER_STATE_LINK_DOWN ||
                     state == ETH_MANAGER_STATE_LINK_UP);

    eth_manager_stop();
    eth_manager_get_state(&state);
    TEST_ASSERT_EQUAL(ETH_MANAGER_STATE_INIT, state);
}

void test_eth_manager_stats_operations(void)
{
    eth_manager_init(&g_test_config);

    eth_manager_stats_t stats;
    eth_status_t status = eth_manager_get_stats(&stats);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    status = eth_manager_clear_stats();
    TEST_ASSERT_EQUAL(ETH_OK, status);

    eth_manager_deinit();
}

void test_eth_manager_error_handling(void)
{
    eth_manager_init(&g_test_config);

    eth_error_info_t error;
    eth_status_t status = eth_manager_get_last_error(&error);
    TEST_ASSERT_EQUAL(ETH_OK, status);

    /* 初始状态下无错误 */
    TEST_ASSERT_EQUAL(0, error.error_code);

    status = eth_manager_clear_error();
    TEST_ASSERT_EQUAL(ETH_OK, status);

    eth_manager_deinit();
}

/* ============================================================================
 * 辅助函数测试
 * ============================================================================ */

void test_eth_manager_error_to_string(void)
{
    const char *str = eth_manager_error_to_string(0x0000);
    TEST_ASSERT_NOT_NULL(str);

    str = eth_manager_error_to_string(0x1001);
    TEST_ASSERT_NOT_NULL(str);

    str = eth_manager_error_to_string(0xFFFF);
    TEST_ASSERT_NOT_NULL(str);
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void)
{
    UNITY_BEGIN();

    /* MAC驱动测试 */
    RUN_TEST(test_eth_mac_init_success);
    RUN_TEST(test_eth_mac_init_null_config);
    RUN_TEST(test_eth_mac_address_operations);
    RUN_TEST(test_eth_mac_mode_operations);

    /* DMA测试 */
    RUN_TEST(test_eth_dma_init_success);
    RUN_TEST(test_eth_dma_init_invalid_desc_count);
    RUN_TEST(test_eth_dma_state_transitions);

    /* 汽车以太网PHY测试 */
    RUN_TEST(test_automotive_phy_init_success);
    RUN_TEST(test_automotive_phy_link_status);
    RUN_TEST(test_automotive_phy_stats);
    RUN_TEST(test_automotive_phy_master_mode);

    /* 以太网管理器测试 */
    RUN_TEST(test_eth_manager_init_success);
    RUN_TEST(test_eth_manager_init_null_config);
    RUN_TEST(test_eth_manager_state_transitions);
    RUN_TEST(test_eth_manager_stats_operations);
    RUN_TEST(test_eth_manager_error_handling);
    RUN_TEST(test_eth_manager_error_to_string);

    return UNITY_END();
}
