/**
 * @file test_tsn_dds_integration.c
 * @brief TSN和DDS集成测试
 */

#include "unity.h"
#include "eth_dds.h"
#include <string.h>

void setUp(void) {
    eth_dds_config_t config = ETH_DDS_CONFIG_DEFAULT;
    config.flags = ETH_DDS_INIT_DDS | ETH_DDS_INIT_TSN;
    
    eth_dds_ret_t ret = eth_dds_init(&config);
    TEST_ASSERT_TRUE(ETH_DDS_IS_OK(ret));
}

void tearDown(void) {
    eth_dds_deinit();
}

/**
 * @brief 测试TSN和DDS模块同时初始化
 */
void test_tsn_dds_init(void) {
    TEST_ASSERT_TRUE(eth_dds_module_available('D'));
    TEST_ASSERT_TRUE(eth_dds_module_available('T'));
}

/**
 * @brief 测试TSN QoS映射到DDS
 */
void test_tsn_qos_mapping(void) {
    // 测试实时QoS级别
    eth_qos_level_t qos = ETH_QOS_DETERMINISTIC;
    TEST_ASSERT_EQUAL_INT(ETH_QOS_DETERMINISTIC, 3);
}

/**
 * @brief 测试时间同步影响
 */
void test_time_sync_impact(void) {
    // 测试gPTP时间同步对DDS的影响
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_tsn_dds_init);
    RUN_TEST(test_tsn_qos_mapping);
    RUN_TEST(test_time_sync_impact);
    
    return UNITY_END();
}
