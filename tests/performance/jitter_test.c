/**
 * @file jitter_test.c
 * @brief DDS抖动测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "../../src/dds/core/dds_core.h"
#include "../../src/common/types/eth_types.h"

/*==============================================================================
 * 测试配置
 *==============================================================================*/

#define TEST_DURATION_SEC       60
#define PUBLISH_INTERVAL_US     10000  /* 10ms = 100Hz */
#define MAX_SAMPLES             10000

/*==============================================================================
 * 统计数据结构
 *==============================================================================*/

typedef struct {
    uint64_t intervals[MAX_SAMPLES];
    uint32_t count;
    double mean_interval_us;
    double jitter_us;  /* 标准差 */
    uint64_t min_interval_us;
    uint64_t max_interval_us;
} jitter_stats_t;

static volatile uint64_t g_last_recv_time = 0;
static volatile uint32_t g_sample_count = 0;
static volatile uint64_t g_intervals[MAX_SAMPLES];

/*==============================================================================
 * 辅助函数
 *==============================================================================*/

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void data_callback(const void* data, uint32_t size, void* user_data) {
    (void)data;
    (void)user_data;
    
    uint64_t now = get_timestamp_ns();
    
    if (g_last_recv_time != 0 && g_sample_count < MAX_SAMPLES) {
        uint64_t interval = now - g_last_recv_time;
        g_intervals[g_sample_count++] = interval;
    }
    
    g_last_recv_time = now;
}

static void print_jitter_stats(const char* name, jitter_stats_t* stats) {
    if (stats->count == 0) {
        printf("%s: No samples collected\n", name);
        return;
    }
    
    /* 计算百分位 */
    uint64_t* sorted = malloc(stats->count * sizeof(uint64_t));
    memcpy(sorted, stats->intervals, stats->count * sizeof(uint64_t));
    
    /* 排序 */
    for (uint32_t i = 0; i < stats->count; i++) {
        for (uint32_t j = i + 1; j < stats->count; j++) {
            if (sorted[j] < sorted[i]) {
                uint64_t tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }
    
    uint64_t p50 = sorted[stats->count / 2];
    uint64_t p90 = sorted[(stats->count * 90) / 100];
    uint64_t p99 = sorted[(stats->count * 99) / 100];
    
    printf("\n%s Jitter Statistics:\n", name);
    printf("  Expected interval:  %d us\n", PUBLISH_INTERVAL_US);
    printf("  Mean interval:      %.2f us\n", stats->mean_interval_us);
    printf("  Jitter (stddev):    %.2f us\n", stats->jitter_us);
    printf("  Min interval:       %.2f us\n", (double)stats->min_interval_us / 1000.0);
    printf("  Max interval:       %.2f us\n", (double)stats->max_interval_us / 1000.0);
    printf("  P50 interval:       %.2f us\n", (double)p50 / 1000.0);
    printf("  P90 interval:       %.2f us\n", (double)p90 / 1000.0);
    printf("  P99 interval:       %.2f us\n", (double)p99 / 1000.0);
    
    /* 计算和标准化抖动指数 */
    double nfi = stats->jitter_us / PUBLISH_INTERVAL_US * 100.0;  /* Normalized Flutter Index */
    printf("  Normalized Flutter: %.2f%%\n", nfi);
    
    if (nfi < 1.0) {
        printf("  Rating: EXCELLENT\n");
    } else if (nfi < 5.0) {
        printf("  Rating: GOOD\n");
    } else if (nfi < 10.0) {
        printf("  Rating: ACCEPTABLE\n");
    } else {
        printf("  Rating: POOR\n");
    }
    
    free(sorted);
}

/*==============================================================================
 * 测试场景
 *==============================================================================*/

static int run_jitter_test(dds_qos_reliability_t reliability) {
    printf("\n");
    printf("========================================\n");
    printf("Jitter Test: reliability=%s\n", 
           reliability == DDS_QOS_RELIABILITY_RELIABLE ? "RELIABLE" : "BEST_EFFORT");
    printf("========================================\n");
    
    g_last_recv_time = 0;
    g_sample_count = 0;
    memset(g_intervals, 0, sizeof(g_intervals));
    
    /* 初始化DDS */
    dds_runtime_init();
    
    dds_domain_participant_t* participant = dds_create_participant(0);
    dds_topic_t* topic = dds_create_topic(participant, "JitterTopic", "JitterType", NULL);
    
    dds_publisher_t* publisher = dds_create_publisher(participant, NULL);
    dds_subscriber_t* subscriber = dds_create_subscriber(participant, NULL);
    
    dds_qos_t qos = {
        .reliability = reliability,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 0,
        .latency_budget_ms = 0,
        .history_depth = 10
    };
    
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    dds_data_reader_t* reader = dds_create_data_reader(subscriber, topic, &qos);
    
    dds_register_data_callback(reader, data_callback, NULL);
    
    /* 准备测试数据 */
    uint8_t test_data[64];
    memset(test_data, 0xAB, sizeof(test_data));
    
    /* 温身 */
    printf("Warming up...\n");
    for (int i = 0; i < 50; i++) {
        dds_write(writer, test_data, sizeof(test_data), 10);
        usleep(PUBLISH_INTERVAL_US);
        dds_spin_once(0);
    }
    
    /* 重置统计 */
    g_last_recv_time = 0;
    g_sample_count = 0;
    
    /* 正式测试 */
    printf("Running test for %d seconds...\n", TEST_DURATION_SEC);
    
    uint64_t start_time = get_timestamp_ns();
    uint64_t next_send_time = start_time;
    uint32_t messages_sent = 0;
    
    while (1) {
        uint64_t now = get_timestamp_ns();
        if (now - start_time >= TEST_DURATION_SEC * 1000000000ULL) {
            break;
        }
        
        /* 准时发送 */
        if (now >= next_send_time) {
            dds_write(writer, test_data, sizeof(test_data), 0);
            messages_sent++;
            next_send_time += PUBLISH_INTERVAL_US * 1000ULL;
        }
        
        dds_spin_once(0);
    }
    
    /* 等待最后的消息 */
    usleep(100000);
    dds_spin_once(0);
    
    /* 计算统计 */
    jitter_stats_t stats = {0};
    stats.count = g_sample_count > 0 ? g_sample_count - 1 : 0;
    stats.min_interval_us = UINT64_MAX;
    
    if (stats.count > 0) {
        double sum = 0;
        double sum_sq = 0;
        
        for (uint32_t i = 1; i < g_sample_count; i++) {
            uint64_t interval = g_intervals[i] / 1000;  /* 转换为us */
            stats.intervals[i-1] = interval;
            sum += interval;
            sum_sq += interval * interval;
            
            if (interval < stats.min_interval_us) stats.min_interval_us = interval;
            if (interval > stats.max_interval_us) stats.max_interval_us = interval;
        }
        
        stats.mean_interval_us = sum / stats.count;
        double variance = (sum_sq / stats.count) - (stats.mean_interval_us * stats.mean_interval_us);
        stats.jitter_us = sqrt(variance);
    }
    
    /* 输出结果 */
    print_jitter_stats(reliability == DDS_QOS_RELIABILITY_RELIABLE ? "Reliable" : "BestEffort", &stats);
    
    printf("\nTest Summary:\n");
    printf("  Messages sent:     %u\n", messages_sent);
    printf("  Messages received: %u\n", g_sample_count);
    printf("  Success rate:      %.2f%%\n", (double)g_sample_count / messages_sent * 100.0);
    
    /* 清理 */
    dds_delete_data_reader(reader);
    dds_delete_data_writer(writer);
    dds_delete_subscriber(subscriber);
    dds_delete_publisher(publisher);
    dds_delete_topic(topic);
    dds_delete_participant(participant);
    dds_runtime_deinit();
    
    return 0;
}

/*==============================================================================
 * 主函数
 *==============================================================================*/

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("================================================\n");
    printf("      ETH-DDS Jitter Performance Test           \n");
    printf("================================================\n");
    
    /* 测试最佳努力模式 */
    run_jitter_test(DDS_QOS_RELIABILITY_BEST_EFFORT);
    
    /* 测试可靠模式 */
    run_jitter_test(DDS_QOS_RELIABILITY_RELIABLE);
    
    printf("\nJitter test completed.\n");
    return 0;
}
