/**
 * @file test_throughput.c
 * @brief 吞吐量性能测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "eth_dds.h"

#define TEST_DURATION_SEC 10
#define MESSAGE_SIZE_MIN 64
#define MESSAGE_SIZE_MAX 8192
#define MESSAGE_SIZE_STEP 512

/**
 * @brief 测试配置
 */
typedef struct {
    int duration_sec;
    int message_size;
    int message_count;
} throughput_config_t;

/**
 * @brief 测试结果
 */
typedef struct {
    double messages_per_sec;
    double bytes_per_sec;
    double mbps;
    int total_messages;
    size_t total_bytes;
} throughput_result_t;

/**
 * @brief 运行吞吐量测试
 */
void run_throughput_test(const throughput_config_t* config, throughput_result_t* result) {
    time_t start_time = time(NULL);
    int message_count = 0;
    size_t total_bytes = 0;
    
    uint8_t* message = malloc(config->message_size);
    memset(message, 0xAA, config->message_size);
    
    printf("Testing with message size: %d bytes\n", config->message_size);
    
    while (difftime(time(NULL), start_time) < config->duration_sec) {
        // 模拟发送
        message_count++;
        total_bytes += config->message_size;
        
        // 模拟处理时间
        if (message_count % 1000 == 0) {
            usleep(1);
        }
    }
    
    double duration = difftime(time(NULL), start_time);
    
    result->total_messages = message_count;
    result->total_bytes = total_bytes;
    result->messages_per_sec = message_count / duration;
    result->bytes_per_sec = total_bytes / duration;
    result->mbps = (result->bytes_per_sec * 8) / (1024 * 1024);
    
    free(message);
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    printf("ETH-DDS Throughput Performance Test\n");
    printf("===================================\n\n");
    
    // 初始化
    eth_dds_config_t dds_config = ETH_DDS_CONFIG_DEFAULT;
    dds_config.flags = ETH_DDS_INIT_DDS | ETH_DDS_INIT_ETHERNET;
    
    if (ETH_DDS_IS_ERROR(eth_dds_init(&dds_config))) {
        fprintf(stderr, "Failed to initialize ETH-DDS\n");
        return 1;
    }
    
    throughput_config_t config = {
        .duration_sec = TEST_DURATION_SEC,
        .message_size = MESSAGE_SIZE_MIN,
        .message_count = 0
    };
    
    throughput_result_t result;
    
    printf("Running throughput tests...\n\n");
    printf("%-15s %-15s %-15s %-15s\n", "Message Size", "Messages/sec", "Bytes/sec", "Mbps");
    printf("%-15s %-15s %-15s %-15s\n", "------------", "------------", "---------", "----");
    
    for (int size = MESSAGE_SIZE_MIN; size <= MESSAGE_SIZE_MAX; size += MESSAGE_SIZE_STEP) {
        config.message_size = size;
        run_throughput_test(&config, &result);
        
        printf("%-15d %-15.0f %-15.0f %-15.2f\n",
               size,
               result.messages_per_sec,
               result.bytes_per_sec,
               result.mbps);
    }
    
    printf("\nThroughput test completed.\n");
    
    eth_dds_deinit();
    return 0;
}
