/**
 * @file temp_sensor_publisher.c
 * @brief 温度传感器数据发布者
 * @version 1.0
 * @date 2026-04-24
 * 
 * @description
 * 模拟温度传感器，定期采集温度数据并通过DDS发布
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "eth_types.h"
#include "eth_utils.h"

/* 模拟DDS API（实际项目中由DDS库提供） */
typedef void* dds_participant_t;
typedef void* dds_topic_t;
typedef void* dds_writer_t;

/* 温度数据类型定义 */
typedef struct {
    uint32_t sensor_id;           /* 传感器ID */
    float temperature;            /* 温度值(摄氏度) */
    float humidity;               /* 湿度百分比 */
    uint64_t timestamp;           /* 时间戳(ms) */
    uint32_t sequence_num;        /* 序列号 */
    char location[32];            /* 位置信息 */
} temperature_data_t;

/* 配置参数 */
#define SENSOR_ID           0x0001
#define PUBLISH_INTERVAL_MS 1000    /* 1秒发布一次 */
#define DDS_DOMAIN_ID       0
#define TOPIC_NAME          "TemperatureData"
#define SENSOR_LOCATION     "Room_101"

/* 全局状态 */
static volatile bool g_running = true;
static uint32_t g_sequence = 0;

/* 模拟温度读取 - 实际项目中替换为真实传感器读取 */
static float read_temperature(void)
{
    /* 模拟正弦波温度变化，范围20-30度 */
    static float base_temp = 22.5f;
    static time_t start_time = 0;
    
    if (start_time == 0) {
        start_time = time(NULL);
    }
    
    time_t elapsed = time(NULL) - start_time;
    float variation = 5.0f * sinf((float)elapsed * 0.1f);
    float noise = ((float)(rand() % 100) - 50.0f) / 100.0f;
    
    return base_temp + variation + noise;
}

/* 模拟湿度读取 */
static float read_humidity(void)
{
    /* 模拟湿度，范围40-70%，正弦变化 */
    static time_t start_time = 0;
    
    if (start_time == 0) {
        start_time = time(NULL);
    }
    
    time_t elapsed = time(NULL) - start_time;
    float variation = 10.0f * cosf((float)elapsed * 0.05f);
    float noise = ((float)(rand() % 50) - 25.0f) / 100.0f;
    
    return 55.0f + variation + noise;
}

/* 采集传感器数据 */
static void collect_sensor_data(temperature_data_t *data)
{
    data->sensor_id = SENSOR_ID;
    data->temperature = read_temperature();
    data->humidity = read_humidity();
    data->timestamp = eth_get_timestamp_ms();
    data->sequence_num = g_sequence++;
    strncpy(data->location, SENSOR_LOCATION, sizeof(data->location) - 1);
    data->location[sizeof(data->location) - 1] = '\0';
}

/* 打印温度数据 */
static void print_temperature_data(const temperature_data_t *data)
{
    printf("[Seq:%04u] Sensor %04X | Location: %-10s | "
           "Temp: %6.2f°C | Humidity: %5.1f%% | Time: %llu\n",
           data->sequence_num,
           data->sensor_id,
           data->location,
           data->temperature,
           data->humidity,
           (unsigned long long)data->timestamp);
}

/* 信号处理 */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\nReceived shutdown signal, stopping publisher...\n");
}

/* 发布数据到DDS（模拟） */
static eth_status_t publish_to_dds(const temperature_data_t *data)
{
    /* 实际项目中调用DDS发布函数 */
    /* dds_write(writer, data, sizeof(temperature_data_t), 100); */
    
    (void)data;
    return ETH_OK;
}

/* 显示帮助信息 */
static void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("Options:\n");
    printf("  -i, --interval <ms>   Publishing interval in milliseconds (default: %d)\n",
           PUBLISH_INTERVAL_MS);
    printf("  -s, --sensor-id <id>  Sensor ID in hex (default: %04X)\n", SENSOR_ID);
    printf("  -l, --location <loc>  Sensor location (default: %s)\n", SENSOR_LOCATION);
    printf("  -h, --help            Show this help message\n");
}

int main(int argc, char *argv[])
{
    int interval_ms = PUBLISH_INTERVAL_MS;
    uint32_t sensor_id = SENSOR_ID;
    char location[32] = SENSOR_LOCATION;
    
    /* 解析命行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) 
                   && i + 1 < argc) {
            interval_ms = atoi(argv[++i]);
            if (interval_ms < 100) {
                fprintf(stderr, "Warning: Interval too small, using 100ms\n");
                interval_ms = 100;
            }
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sensor-id") == 0)
                   && i + 1 < argc) {
            sensor_id = (uint32_t)strtoul(argv[++i], NULL, 16);
        } else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--location") == 0)
                   && i + 1 < argc) {
            strncpy(location, argv[++i], sizeof(location) - 1);
            location[sizeof(location) - 1] = '\0';
        }
    }
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 初始化随机数生成器 */
    srand((unsigned int)time(NULL));
    
    printf("\n");
    printf("========================================\n");
    printf("  Temperature Sensor Publisher\n");
    printf("========================================\n");
    printf("Sensor ID:  0x%04X\n", sensor_id);
    printf("Location:   %s\n", location);
    printf("Interval:   %d ms\n", interval_ms);
    printf("DDS Topic:  %s\n", TOPIC_NAME);
    printf("Domain ID:  %d\n", DDS_DOMAIN_ID);
    printf("----------------------------------------\n");
    printf("Press Ctrl+C to stop\n\n");
    
    temperature_data_t data;
    
    while (g_running) {
        /* 采集数据 */
        collect_sensor_data(&data);
        
        /* 显示数据 */
        print_temperature_data(&data);
        
        /* 发布到DDS */
        eth_status_t status = publish_to_dds(&data);
        if (status != ETH_OK) {
            fprintf(stderr, "Failed to publish data: %d\n", status);
        }
        
        /* 等待下一个发布周期 */
        eth_delay_ms((uint32_t)interval_ms);
    }
    
    printf("\nPublisher stopped.\n");
    printf("Total samples published: %u\n", g_sequence);
    
    return 0;
}
