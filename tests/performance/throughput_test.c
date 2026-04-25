/**
 * @file throughput_test.c
 * @brief DDS吞吐量测试
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

#include "../../src/dds/core/dds_core.h"
#include "../../src/common/types/eth_types.h"

/*==============================================================================
 * 测试配置
 *==============================================================================*/

#define TEST_DURATION_SEC       10
#define MAX_MESSAGE_SIZE        65536

/*==============================================================================
 * 统计数据结构
 *==============================================================================*/

typedef struct {
    uint64_t messages_sent;
    uint64_t bytes_sent;
    uint64_t messages_received;
    uint64_t bytes_received;
    double elapsed_sec;
    double throughput_mbps;
    double message_rate_hz;
} throughput_result_t;

static volatile int g_running = 1;
static volatile uint64_t g_messages_received = 0;
static volatile uint64_t g_bytes_received = 0;

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
    g_messages_received++;
    g_bytes_received += size;
}

/*==============================================================================
 * 测试场景
 *==============================================================================*/

static int run_throughput_test(uint32_t message_size) {
    printf("\n");
    printf("========================================\n");
    printf("Throughput Test: message_size=%u bytes\n", message_size);
    printf("========================================\n");
    
    g_running = 1;
    g_messages_received = 0;
    g_bytes_received = 0;
    
    /* 初始化DDS */
    dds_runtime_init();
    
    dds_domain_participant_t* participant = dds_create_participant(0);
    if (participant == NULL) {
        printf("Failed to create participant\n");
        return -1;
    }
    
    dds_topic_t* topic = dds_create_topic(participant, "ThroughputTopic", "ThroughputType", NULL);
    if (topic == NULL) {
        printf("Failed to create topic\n");
        return -1;
    }
    
    dds_publisher_t* publisher = dds_create_publisher(participant, NULL);
    dds_subscriber_t* subscriber = dds_create_subscriber(participant, NULL);
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_BEST_EFFORT, /* 吞吐量测试使用最佳努力 */
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 0,
        .latency_budget_ms = 0,
        .history_depth = 1
    };
    
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    dds_data_reader_t* reader = dds_create_data_reader(subscriber, topic, &qos);
    
    /* 注册接收回调 */
    dds_register_data_callback(reader, data_callback, NULL);
    
    /* 准备测试数据 */
    uint8_t* test_data = malloc(message_size);
    for (uint32_t i = 0; i < message_size; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }
    
    /* 温身 */
    printf("Warming up...\n");
    for (int i = 0; i < 100; i++) {
        dds_write(writer, test_data, message_size, 0);
        dds_spin_once(0);
    }
    
    /* 正式测试 */
    printf("Running test for %d seconds...\n", TEST_DURATION_SEC);
    
    uint64_t start_time = get_timestamp_ns();
    uint64_t messages_sent = 0;
    uint64_t bytes_sent = 0;
    
    while (1) {
        uint64_t elapsed = get_timestamp_ns() - start_time;
        if (elapsed >= TEST_DURATION_SEC * 1000000000ULL) {
            break;
        }
        
        /* 尽可能快地发送 */
        eth_status_t status = dds_write(writer, test_data, message_size, 0);
        if (status == ETH_OK) {
            messages_sent++;
            bytes_sent += message_size;
        }
        
        /* 处理接收 */
        dds_spin_once(0);
    }
    
    uint64_t end_time = get_timestamp_ns();
    double elapsed_sec = (end_time - start_time) / 1000000000.0;
    
    /* 等待最后的消息被处理 */
    usleep(100000);
    dds_spin_once(0);
    
    /* 计算结果 */
    throughput_result_t result = {
        .messages_sent = messages_sent,
        .bytes_sent = bytes_sent,
        .messages_received = g_messages_received,
        .bytes_received = g_bytes_received,
        .elapsed_sec = elapsed_sec,
        .throughput_mbps = (bytes_sent * 8.0) / (elapsed_sec * 1000000.0),
        .message_rate_hz = messages_sent / elapsed_sec
    };
    
    /* 输出结果 */
    printf("\nThroughput Results:\n");
    printf("  Duration:           %.2f seconds\n", result.elapsed_sec);
    printf("  Messages sent:      %lu\n", (unsigned long)result.messages_sent);
    printf("  Messages received:  %lu\n", (unsigned long)result.messages_received);
    printf("  Bytes sent:         %lu\n", (unsigned long)result.bytes_sent);
    printf("  Bytes received:     %lu\n", (unsigned long)result.bytes_received);
    printf("  Throughput:         %.2f Mbps\n", result.throughput_mbps);
    printf("  Message rate:       %.2f msg/s\n", result.message_rate_hz);
    printf("  Loss rate:          %.2f%%\n", 
           (1.0 - (double)result.messages_received / result.messages_sent) * 100.0);
    
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
    printf("     ETH-DDS Throughput Performance Test        \n");
    printf("================================================\n");
    
    /* 测试不同消息大小 */
    run_throughput_test(64);       /* 小包 */
    run_throughput_test(256);      /* 小中包 */
    run_throughput_test(1024);     /* 中包 */
    run_throughput_test(4096);     /* 大包 */
    run_throughput_test(8192);     /* 超大包 */
    
    printf("\nThroughput test completed.\n");
    return 0;
}
