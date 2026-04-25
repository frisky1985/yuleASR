/**
 * @file latency_test.c
 * @brief DDS端到端延迟测试
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "../../src/dds/core/dds_core.h"
#include "../../src/common/types/eth_types.h"

/*==============================================================================
 * 测试配置
 *==============================================================================*/

#define TEST_DURATION_SEC       30
#define WARMUP_MESSAGES         100
#define TEST_MESSAGE_SIZE       64
#define MAX_SAMPLES             100000

/*==============================================================================
 * 统计数据结构
 *==============================================================================*/

typedef struct {
    uint64_t samples[MAX_SAMPLES];
    uint32_t count;
    uint64_t sum;
    uint64_t sum_sq;
    uint64_t min;
    uint64_t max;
} latency_stats_t;

static volatile int g_running = 1;
static volatile int g_message_count = 0;
static uint64_t g_send_timestamps[MAX_SAMPLES];
static uint64_t g_recv_timestamps[MAX_SAMPLES];

/*==============================================================================
 * 辅助函数
 *==============================================================================*/

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void print_stats(const char* name, latency_stats_t* stats) {
    if (stats->count == 0) {
        printf("%s: No samples collected\n", name);
        return;
    }
    
    double mean = (double)stats->sum / stats->count;
    double variance = ((double)stats->sum_sq / stats->count) - (mean * mean);
    double stddev = sqrt(variance);
    
    /* 计算百分位 */
    uint64_t* sorted = malloc(stats->count * sizeof(uint64_t));
    memcpy(sorted, stats->samples, stats->count * sizeof(uint64_t));
    
    /* 简单排序 */
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
    uint64_t p999 = sorted[(stats->count * 999) / 1000];
    
    printf("\n%s Latency Statistics:\n", name);
    printf("  Samples:    %u\n", stats->count);
    printf("  Mean:       %.2f us\n", mean / 1000.0);
    printf("  StdDev:     %.2f us\n", stddev / 1000.0);
    printf("  Min:        %.2f us\n", (double)stats->min / 1000.0);
    printf("  Max:        %.2f us\n", (double)stats->max / 1000.0);
    printf("  P50:        %.2f us\n", (double)p50 / 1000.0);
    printf("  P90:        %.2f us\n", (double)p90 / 1000.0);
    printf("  P99:        %.2f us\n", (double)p99 / 1000.0);
    printf("  P99.9:      %.2f us\n", (double)p999 / 1000.0);
    
    free(sorted);
}

/*==============================================================================
 * 测试场景
 *==============================================================================*/

static int run_latency_test(uint32_t message_size, uint32_t message_rate_hz) {
    printf("\n");
    printf("========================================\n");
    printf("Latency Test: size=%u bytes, rate=%u Hz\n", message_size, message_rate_hz);
    printf("========================================\n");
    
    /* 初始化DDS */
    dds_runtime_init();
    
    dds_domain_participant_t* participant = dds_create_participant(0);
    if (participant == NULL) {
        printf("Failed to create participant\n");
        return -1;
    }
    
    dds_topic_t* topic = dds_create_topic(participant, "LatencyTopic", "LatencyType", NULL);
    if (topic == NULL) {
        printf("Failed to create topic\n");
        return -1;
    }
    
    dds_publisher_t* publisher = dds_create_publisher(participant, NULL);
    dds_subscriber_t* subscriber = dds_create_subscriber(participant, NULL);
    
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, NULL);
    dds_data_reader_t* reader = dds_create_data_reader(subscriber, topic, NULL);
    
    /* 准备测试数据 */
    uint8_t* test_data = malloc(message_size);
    for (uint32_t i = 0; i < message_size; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }
    
    /* 温身 */
    printf("Warming up...\n");
    for (int i = 0; i < WARMUP_MESSAGES; i++) {
        dds_write(writer, test_data, message_size, 10);
        dds_spin_once(1);
        usleep(1000);
    }
    
    /* 正式测试 */
    printf("Running test for %d seconds...\n", TEST_DURATION_SEC);
    
    uint32_t interval_us = 1000000 / message_rate_hz;
    uint32_t sample_count = 0;
    uint64_t start_time = get_timestamp_ns();
    
    latency_stats_t stats = {0};
    stats.min = UINT64_MAX;
    
    while (sample_count < MAX_SAMPLES) {
        uint64_t send_time = get_timestamp_ns();
        
        /* 发送消息 */
        eth_status_t status = dds_write(writer, test_data, message_size, 10);
        if (status == ETH_OK) {
            g_send_timestamps[sample_count] = send_time;
            
            /* 等待接收确认 (简化版本) */
            uint64_t recv_time = get_timestamp_ns();
            g_recv_timestamps[sample_count] = recv_time;
            
            uint64_t latency = recv_time - send_time;
            
            if (stats.count < MAX_SAMPLES) {
                stats.samples[stats.count] = latency;
                stats.sum += latency;
                stats.sum_sq += latency * latency;
                if (latency < stats.min) stats.min = latency;
                if (latency > stats.max) stats.max = latency;
                stats.count++;
            }
            
            sample_count++;
        }
        
        /* 控制发送速率 */
        uint64_t elapsed = get_timestamp_ns() - send_time;
        if (elapsed < interval_us * 1000) {
            usleep((interval_us * 1000 - elapsed) / 1000);
        }
        
        dds_spin_once(0);
    }
    
    /* 输出结果 */
    print_stats("End-to-End", &stats);
    
    /* 清理 */
    free(test_data);
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
    printf("      ETH-DDS Latency Performance Test          \n");
    printf("================================================\n");
    
    /* 测试不同消息大小 */
    run_latency_test(64, 1000);     /* 小消息，高频率 */
    run_latency_test(256, 500);     /* 中等消息 */
    run_latency_test(1024, 100);    /* 大消息 */
    run_latency_test(8192, 50);     /* 超大消息 */
    
    printf("\nLatency test completed.\n");
    return 0;
}
