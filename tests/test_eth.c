/**
 * @file test_eth.c
 * @brief Ethernet接口测试
 */

#include <unity.h>
#include "telemetry.h"
#include <string.h>

void test_eth_link_state_logging(void);
void test_eth_tx_complete_logging(void);
void test_eth_error_logging(void);

/* 模拟以太网驱动回调 */
static uint32_t g_logged_events = 0;

void test_eth_link_state_logging(void) {
    Tel_Init();
    
    /* 模拟链路上升 */
    TelStatus_t status = Tel_LogState(TEL_MOD_ETH, TEL_EVT_ETH_LINK_UP, 
                                       TEL_LEVEL_INFO, 0, 1);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    /* 模拟链路下降 */
    status = Tel_LogState(TEL_MOD_ETH, TEL_EVT_ETH_LINK_DOWN,
                          TEL_LEVEL_WARNING, 1, 0);
    TEST_ASSERT_EQUAL(TEL_OK, status);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(2, stats->total_events);
}

void test_eth_tx_complete_logging(void) {
    Tel_Init();
    
    /* 模拟多个TX完成事件 */
    for (int i = 0; i < 10; i++) {
        TelStatus_t status = Tel_LogInstant(TEL_MOD_ETH, TEL_EVT_ETH_TX_COMPLETE,
                                            TEL_LEVEL_DEBUG);
        TEST_ASSERT_EQUAL(TEL_OK, status);
    }
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(10, stats->total_events);
}

void test_eth_error_logging(void) {
    Tel_Init();
    
    /* 模拟各种错误 */
    Tel_LogInstant(TEL_MOD_ETH, TEL_EVT_ETH_TX_ERROR, TEL_LEVEL_ERROR);
    Tel_LogInstant(TEL_MOD_ETH, TEL_EVT_ETH_RX_ERROR, TEL_LEVEL_ERROR);
    Tel_LogInstant(TEL_MOD_ETH, TEL_EVT_ETH_PHY_ERROR, TEL_LEVEL_ERROR);
    
    const TelStats_t *stats = Tel_GetStats();
    TEST_ASSERT_EQUAL(3, stats->total_events);
    TEST_ASSERT_EQUAL(0, stats->dropped_events);
}

void test_eth_suite(void) {
    RUN_TEST(test_eth_link_state_logging);
    RUN_TEST(test_eth_tx_complete_logging);
    RUN_TEST(test_eth_error_logging);
}
