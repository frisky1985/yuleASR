/**
 * @file temp_sensor_subscriber.c
 * @brief 温度传感器数据订阅者
 * @version 1.0
 * @date 2026-04-24
 * 
 * @description
 * 订阅温度数据主题，接收并处理温度传感器数据
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#include "eth_types.h"
#include "eth_utils.h"

/* 温度数据类型定义 - 必须与发布者保持一致 */
typedef struct {
    uint32_t sensor_id;
    float temperature;
    float humidity;
    uint64_t timestamp;
    uint32_t sequence_num;
    char location[32];
} temperature_data_t;

/* 配置参数 */
#define DDS_DOMAIN_ID       0
#define TOPIC_NAME          "TemperatureData"
#define ALARM_HIGH_TEMP     28.0f   /* 高温报警阈值 */
#define ALARM_LOW_TEMP      15.0f   /* 低温报警阈值 */
#define STATS_WINDOW_SIZE   100     /* 统计窗口大小 */

/* 全局状态 */
static volatile bool g_running = true;
static uint32_t g_sample_count = 0;
static float g_temp_sum = 0.0f;
static float g_temp_min = 1000.0f;
static float g_temp_max = -1000.0f;
static uint32_t g_alarm_count = 0;

/* 统计数据 */
typedef struct {
    float temp_readings[STATS_WINDOW_SIZE];
    uint32_t index;
    uint32_t count;
} temp_statistics_t;

static temp_statistics_t g_stats = {0};

/* 更新统计信息 */
static void update_statistics(float temp)
{
    g_temp_sum += temp;
    
    if (temp < g_temp_min) {
        g_temp_min = temp;
    }
    if (temp > g_temp_max) {
        g_temp_max = temp;
    }
    
    /* 更新滑动窗口 */
    g_stats.temp_readings[g_stats.index] = temp;
    g_stats.index = (g_stats.index + 1) % STATS_WINDOW_SIZE;
    if (g_stats.count < STATS_WINDOW_SIZE) {
        g_stats.count++;
    }
}

/* 计算移动平均温度 */
static float calculate_moving_average(void)
{
    if (g_stats.count == 0) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (uint32_t i = 0; i < g_stats.count; i++) {
        sum += g_stats.temp_readings[i];
    }
    return sum / g_stats.count;
}

/* 检查报警条件 */
static void check_alarms(const temperature_data_t *data)
{
    bool alarm = false;
    
    if (data->temperature > ALARM_HIGH_TEMP) {
        printf("  [ALARM] High temperature! %.2f°C exceeds threshold %.2f°C\n",
               data->temperature, ALARM_HIGH_TEMP);
        alarm = true;
    }
    if (data->temperature < ALARM_LOW_TEMP) {
        printf("  [ALARM] Low temperature! %.2f°C below threshold %.2f°C\n",
               data->temperature, ALARM_LOW_TEMP);
        alarm = true;
    }
    if (data->humidity > 80.0f) {
        printf("  [ALARM] High humidity! %.1f%%\n", data->humidity);
        alarm = true;
    }
    
    if (alarm) {
        g_alarm_count++;
    }
}

/* 打印接收到的数据 */
static void print_received_data(const temperature_data_t *data)
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

/* 显示统计信息 */
static void print_statistics(void)
{
    if (g_sample_count == 0) {
        printf("No data received yet.\n");
        return;
    }
    
    float avg_temp = g_temp_sum / g_sample_count;
    float moving_avg = calculate_moving_average();
    
    printf("\n");
    printf("----------------------------------------\n");
    printf("  Statistics Report\n");
    printf("----------------------------------------\n");
    printf("  Total samples:    %u\n", g_sample_count);
    printf("  Min temperature:  %.2f°C\n", g_temp_min);
    printf("  Max temperature:  %.2f°C\n", g_temp_max);
    printf("  Avg temperature:  %.2f°C\n", avg_temp);
    printf("  Moving average:   %.2f°C (%u samples)\n", moving_avg, g_stats.count);
    printf("  Alarms triggered: %u\n", g_alarm_count);
    printf("----------------------------------------\n");
}

/* 处理接收到的数据 */
static void process_data(const temperature_data_t *data)
{
    g_sample_count++;
    
    print_received_data(data);
    update_statistics(data->temperature);
    check_alarms(data);
}

/* 信号处理 */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\nReceived shutdown signal, stopping subscriber...\n");
}

/* 从DDS接收数据（模拟） */
static eth_status_t receive_from_dds(temperature_data_t *data)
{
    /* 实际项目中调用DDS接收函数 */
    /* dds_read(reader, data, &size, 100); */
    
    (void)data;
    /* 模拟没有数据 */
    return ETH_TIMEOUT;
}

/* 显示帮助信息 */
static void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("Options:\n");
    printf("  -h, --help           Show this help message\n");
    printf("  --stats              Show statistics on exit\n");
    printf("  --high-temp <temp>   Set high temperature alarm threshold (default: %.1f)\n",
           ALARM_HIGH_TEMP);
    printf("  --low-temp <temp>    Set low temperature alarm threshold (default: %.1f)\n",
           ALARM_LOW_TEMP);
}

int main(int argc, char *argv[])
{
    bool show_stats = false;
    
    /* 解析命行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--stats") == 0) {
            show_stats = true;
        }
    }
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("\n");
    printf("========================================\n");
    printf("  Temperature Sensor Subscriber\n");
    printf("========================================\n");
    printf("DDS Topic:       %s\n", TOPIC_NAME);
    printf("Domain ID:       %d\n", DDS_DOMAIN_ID);
    printf("High threshold:  %.1f°C\n", ALARM_HIGH_TEMP);
    printf("Low threshold:   %.1f°C\n", ALARM_LOW_TEMP);
    printf("----------------------------------------\n");
    printf("Press Ctrl+C to stop\n\n");
    
    /* 模拟接收数据循环 */
    while (g_running) {
        temperature_data_t data;
        
        eth_status_t status = receive_from_dds(&data);
        
        if (status == ETH_OK) {
            process_data(&data);
        } else if (status == ETH_TIMEOUT) {
            /* 模拟活动 - 每秒打印一次状态 */
            static uint32_t last_print = 0;
            uint64_t now = eth_get_timestamp_ms();
            
            if (now - last_print >= 5000) {  /* 每5秒显示统计 */
                if (g_sample_count > 0) {
                    print_statistics();
                }
                last_print = (uint32_t)now;
            }
            
            eth_delay_ms(100);
        } else {
            fprintf(stderr, "Error receiving data: %d\n", status);
            break;
        }
    }
    
    printf("\nSubscriber stopped.\n");
    
    if (show_stats || g_sample_count > 0) {
        print_statistics();
    }
    
    return 0;
}
