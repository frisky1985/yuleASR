/**
 * @file test_runner.c
 * @brief 统一测试运行器
 * @version 1.0
 * @date 2026-04-24
 * 
 * @description
 * 运行所有单元测试并生成统一报告
 */

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

/* 外部测试函数声明 - eth_types */
extern void test_eth_types_MAC_ADDR_macro(void);
extern void test_eth_types_ETH_MAC_IS_EQUAL_macro(void);
extern void test_eth_types_ETH_MAC_IS_BROADCAST_macro(void);
extern void test_eth_types_ETH_IP_ADDR_macro(void);
extern void test_eth_types_status_values(void);
extern void test_eth_types_eth_config_size(void);
extern void test_eth_types_eth_config_defaults(void);
extern void test_eth_types_eth_buffer_init(void);
extern void test_eth_types_dds_qos_defaults(void);
extern void test_eth_types_dds_qos_reliability_values(void);
extern void test_eth_types_dds_qos_durability_values(void);

/* 外部测试函数声明 - eth_utils */
extern void test_eth_utils_crc32_basic(void);
extern void test_eth_utils_crc32_empty_data(void);
extern void test_eth_utils_crc32_different_data(void);
extern void test_eth_utils_htonl_basic(void);
extern void test_eth_utils_htons_basic(void);
extern void test_eth_utils_byte_order_zero(void);
extern void test_eth_utils_byte_order_max(void);
extern void test_eth_utils_mac_to_string(void);
extern void test_eth_utils_mac_to_string_buffer_too_small(void);
extern void test_eth_utils_mac_from_string(void);
extern void test_eth_utils_mac_from_string_invalid_format(void);
extern void test_eth_utils_mac_roundtrip(void);
extern void test_eth_utils_ip_to_string(void);
extern void test_eth_utils_ip_from_string(void);
extern void test_eth_utils_ip_roundtrip(void);
extern void test_eth_utils_ip_from_string_invalid(void);
extern void test_eth_utils_safe_memcpy_basic(void);
extern void test_eth_utils_safe_memcpy_src_larger(void);
extern void test_eth_utils_safe_memcpy_null_dst(void);
extern void test_eth_utils_safe_memcpy_null_src(void);
extern void test_eth_utils_get_timestamp(void);
extern void test_eth_utils_delay_ms(void);

/* 外部测试函数声明 - eth_buffer */
extern void test_eth_buffer_init(void);
extern void test_eth_buffer_init_null_data(void);
extern void test_eth_buffer_remaining_space(void);
extern void test_eth_buffer_full(void);
extern void test_eth_buffer_empty(void);
extern void test_eth_buffer_write_data(void);
extern void test_eth_buffer_append_data(void);
extern void test_eth_buffer_overflow_protection(void);
extern void test_eth_buffer_clear(void);
extern void test_eth_buffer_reset(void);
extern void test_eth_buffer_compare_equal(void);
extern void test_eth_buffer_compare_different(void);
extern void test_eth_buffer_truncate(void);

/* 外部测试函数声明 - dds_qos */
extern void test_dds_qos_reliability_best_effort(void);
extern void test_dds_qos_reliability_reliable(void);
extern void test_dds_qos_durability_volatile(void);
extern void test_dds_qos_durability_transient_local(void);
extern void test_dds_qos_durability_transient(void);
extern void test_dds_qos_durability_persistent(void);
extern void test_dds_qos_full_configuration(void);
extern void test_dds_qos_best_effort_volatile(void);
extern void test_dds_qos_persistent_historical(void);
extern void test_dds_qos_compatibility_reader_writer(void);
extern void test_dds_qos_compatibility_compatible(void);

/* 统计信息 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int ignored_tests;
} test_stats_t;

static test_stats_t stats = {0, 0, 0, 0};

void setUp(void) { }
void tearDown(void) { }

/* 运行单个测试并记录结果 */
static int run_single_test(void (*test_func)(void), const char* name)
{
    stats.total_tests++;
    
    printf("  [%2d] %-50s ", stats.total_tests, name);
    fflush(stdout);
    
    UNITY_BEGIN();
    RUN_TEST(test_func);
    int result = UNITY_END();
    
    if (result == 0) {
        stats.passed_tests++;
        printf("[PASS]\n");
    } else {
        stats.failed_tests++;
        printf("[FAIL]\n");
    }
    
    return result;
}

/* 打印测试分组标题 */
static void print_group_header(const char* group_name)
{
    printf("\n----------------------------------------\n");
    printf("  %s\n", group_name);
    printf("----------------------------------------\n");
}

/* 打印测试统计 */
static void print_summary(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("  Total:   %3d\n", stats.total_tests);
    printf("  Passed:  %3d\n", stats.passed_tests);
    printf("  Failed:  %3d\n", stats.failed_tests);
    printf("  Ignored: %3d\n", stats.ignored_tests);
    printf("----------------------------------------\n");
    
    if (stats.failed_tests == 0) {
        printf("  Result:  ALL TESTS PASSED!\n");
    } else {
        printf("  Result:  %d TEST(S) FAILED!\n", stats.failed_tests);
    }
    printf("========================================\n");
}

int main(int argc, char* argv[])
{
    printf("\n");
    printf("========================================\n");
    printf("  ETH-DDS Integration Test Suite\n");
    printf("========================================\n");
    
    /* eth_types 测试 */
    print_group_header("ETH Types Tests");
    run_single_test(test_eth_types_MAC_ADDR_macro, "MAC_ADDR_macro");
    run_single_test(test_eth_types_ETH_MAC_IS_EQUAL_macro, "MAC_IS_EQUAL_macro");
    run_single_test(test_eth_types_ETH_MAC_IS_BROADCAST_macro, "MAC_IS_BROADCAST_macro");
    run_single_test(test_eth_types_ETH_IP_ADDR_macro, "IP_ADDR_macro");
    run_single_test(test_eth_types_status_values, "status_values");
    run_single_test(test_eth_types_eth_config_size, "config_size");
    run_single_test(test_eth_types_eth_config_defaults, "config_defaults");
    run_single_test(test_eth_types_eth_buffer_init, "buffer_init");
    run_single_test(test_eth_types_dds_qos_defaults, "qos_defaults");
    run_single_test(test_eth_types_dds_qos_reliability_values, "qos_reliability_values");
    run_single_test(test_eth_types_dds_qos_durability_values, "qos_durability_values");
    
    /* eth_buffer 测试 */
    print_group_header("ETH Buffer Tests");
    run_single_test(test_eth_buffer_init, "buffer_init");
    run_single_test(test_eth_buffer_init_null_data, "buffer_init_null_data");
    run_single_test(test_eth_buffer_remaining_space, "buffer_remaining_space");
    run_single_test(test_eth_buffer_full, "buffer_full");
    run_single_test(test_eth_buffer_empty, "buffer_empty");
    run_single_test(test_eth_buffer_write_data, "buffer_write_data");
    run_single_test(test_eth_buffer_append_data, "buffer_append_data");
    run_single_test(test_eth_buffer_overflow_protection, "buffer_overflow_protection");
    run_single_test(test_eth_buffer_clear, "buffer_clear");
    run_single_test(test_eth_buffer_reset, "buffer_reset");
    run_single_test(test_eth_buffer_compare_equal, "buffer_compare_equal");
    run_single_test(test_eth_buffer_compare_different, "buffer_compare_different");
    run_single_test(test_eth_buffer_truncate, "buffer_truncate");
    
    /* dds_qos 测试 */
    print_group_header("DDS QoS Tests");
    run_single_test(test_dds_qos_reliability_best_effort, "qos_reliability_best_effort");
    run_single_test(test_dds_qos_reliability_reliable, "qos_reliability_reliable");
    run_single_test(test_dds_qos_durability_volatile, "qos_durability_volatile");
    run_single_test(test_dds_qos_durability_transient_local, "qos_durability_transient_local");
    run_single_test(test_dds_qos_durability_transient, "qos_durability_transient");
    run_single_test(test_dds_qos_durability_persistent, "qos_durability_persistent");
    run_single_test(test_dds_qos_full_configuration, "qos_full_configuration");
    run_single_test(test_dds_qos_best_effort_volatile, "qos_best_effort_volatile");
    run_single_test(test_dds_qos_persistent_historical, "qos_persistent_historical");
    run_single_test(test_dds_qos_compatibility_reader_writer, "qos_compatibility_reader_writer");
    run_single_test(test_dds_qos_compatibility_compatible, "qos_compatibility_compatible");
    
    /* 打印结果汇总 */
    print_summary();
    
    return stats.failed_tests;
}
