/**
 * @file basic_pubsub.c
 * @brief DDS基础发布订阅示例
 * @version 1.0
 * @date 2026-04-24
 */

#include <stdio.h>
#include <string.h>
#include "../src/dds/core/dds_core.h"
#include "../src/ethernet/driver/eth_driver.h"

/* ============================================================================
 * 示例数据类型定义
 * ============================================================================ */

typedef struct {
    uint32_t timestamp;
    uint32_t sensor_id;
    float temperature;
    float humidity;
    uint8_t status;
} sensor_data_t;

typedef struct {
    uint32_t timestamp;
    uint32_t target_id;
    float setpoint;
    uint8_t command;
} control_command_t;

/* ============================================================================
 * 全局变量
 * ============================================================================ */

static dds_domain_participant_t *g_participant = NULL;
static dds_publisher_t *g_publisher = NULL;
static dds_subscriber_t *g_subscriber = NULL;
static dds_topic_t *g_sensor_topic = NULL;
static dds_topic_t *g_control_topic = NULL;
static dds_data_writer_t *g_sensor_writer = NULL;
static dds_data_reader_t *g_control_reader = NULL;

/* ============================================================================
 * 回调函数
 * ============================================================================ */

/**
 * @brief 控制命令接收回调
 */
void control_command_callback(const void *data, uint32_t size, void *user_data)
{
    if (size != sizeof(control_command_t)) {
        printf("Error: Invalid control command size\n");
        return;
    }
    
    const control_command_t *cmd = (const control_command_t *)data;
    printf("Received Control Command:\n");
    printf("  Timestamp: %u\n", cmd->timestamp);
    printf("  Target ID: %u\n", cmd->target_id);
    printf("  Setpoint: %.2f\n", cmd->setpoint);
    printf("  Command: %u\n", cmd->command);
    
    (void)user_data;
}

/**
 * @brief 连接状态变化回调
 */
void link_status_callback(eth_link_status_t status, void *user_data)
{
    if (status == ETH_LINK_UP) {
        printf("Ethernet Link: CONNECTED\n");
    } else {
        printf("Ethernet Link: DISCONNECTED\n");
    }
    (void)user_data;
}

/* ============================================================================
 * 初始化函数
 * ============================================================================ */

eth_status_t init_ethernet(void)
{
    eth_config_t config = {
        .mac_addr = ETH_MAC_ADDR(0x00, 0x80, 0xE1, 0x00, 0x00, 0x01),
        .mode = ETH_MODE_AUTO_NEGOTIATION,
        .enable_dma = true,
        .enable_checksum_offload = true,
        .rx_buffer_size = 1536,
        .tx_buffer_size = 1536,
        .rx_desc_count = 8,
        .tx_desc_count = 8,
    };
    
    eth_status_t status = eth_driver_init(&config);
    if (status != ETH_OK) {
        printf("Failed to initialize Ethernet driver: %d\n", status);
        return status;
    }
    
    /* 注册连接状态回调 */
    eth_driver_register_link_callback(link_status_callback, NULL);
    
    /* 启用以太网 */
    status = eth_driver_start();
    if (status != ETH_OK) {
        printf("Failed to start Ethernet: %d\n", status);
        return status;
    }
    
    printf("Ethernet initialized successfully\n");
    return ETH_OK;
}

eth_status_t init_dds(void)
{
    dds_qos_t qos;
    
    /* 初始化DDS运行时 */
    eth_status_t status = dds_runtime_init();
    if (status != ETH_OK) {
        printf("Failed to initialize DDS runtime\n");
        return status;
    }
    
    /* 创建DDS域参与者 */
    g_participant = dds_create_participant(0);  /* Domain ID = 0 */
    if (g_participant == NULL) {
        printf("Failed to create DDS participant\n");
        return ETH_ERROR;
    }
    
    /* 配置实时性QoS */
    qos.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos.deadline_ms = 50;
    qos.latency_budget_ms = 5;
    qos.history_depth = 1;
    
    dds_set_participant_qos(g_participant, &qos);
    
    /* 创建发布者和订阅者 */
    g_publisher = dds_create_publisher(g_participant, NULL);
    g_subscriber = dds_create_subscriber(g_participant, NULL);
    
    if (g_publisher == NULL || g_subscriber == NULL) {
        printf("Failed to create publisher/subscriber\n");
        return ETH_ERROR;
    }
    
    /* 创建主题 */
    g_sensor_topic = dds_create_topic(g_participant, "SensorData", "sensor_data_t", NULL);
    g_control_topic = dds_create_topic(g_participant, "ControlCommand", "control_command_t", NULL);
    
    if (g_sensor_topic == NULL || g_control_topic == NULL) {
        printf("Failed to create topics\n");
        return ETH_ERROR;
    }
    
    /* 配置传感器数据QoS (实时性) */
    qos.reliability = DDS_QOS_RELIABILITY_BEST_EFFORT;
    qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos.deadline_ms = 50;
    
    /* 创建数据写入者 */
    g_sensor_writer = dds_create_data_writer(g_publisher, g_sensor_topic, &qos);
    if (g_sensor_writer == NULL) {
        printf("Failed to create sensor data writer\n");
        return ETH_ERROR;
    }
    
    /* 配置控制命令QoS (可靠性) */
    qos.reliability = DDS_QOS_RELIABILITY_RELIABLE;
    qos.durability = DDS_QOS_DURABILITY_VOLATILE;
    qos.deadline_ms = 100;
    
    /* 创建数据读取者 */
    g_control_reader = dds_create_data_reader(g_subscriber, g_control_topic, &qos);
    if (g_control_reader == NULL) {
        printf("Failed to create control command reader\n");
        return ETH_ERROR;
    }
    
    /* 注册接收回调 */
    dds_register_data_callback(g_control_reader, control_command_callback, NULL);
    
    printf("DDS initialized successfully\n");
    return ETH_OK;
}

void cleanup(void)
{
    /* 清理DDS资源 */
    if (g_sensor_writer) dds_delete_data_writer(g_sensor_writer);
    if (g_control_reader) dds_delete_data_reader(g_control_reader);
    if (g_sensor_topic) dds_delete_topic(g_sensor_topic);
    if (g_control_topic) dds_delete_topic(g_control_topic);
    if (g_publisher) dds_delete_publisher(g_publisher);
    if (g_subscriber) dds_delete_subscriber(g_subscriber);
    if (g_participant) dds_delete_participant(g_participant);
    
    dds_runtime_deinit();
    
    /* 清理以太网资源 */
    eth_driver_stop();
    eth_driver_deinit();
    
    printf("Cleanup completed\n");
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(int argc, char *argv[])
{
    eth_status_t status;
    sensor_data_t sensor_data = {
        .sensor_id = 1,
        .temperature = 25.5f,
        .humidity = 60.0f,
        .status = 0,
    };
    
    uint32_t counter = 0;
    
    (void)argc;
    (void)argv;
    
    printf("=== ETH-DDS Integration Demo ===\n");
    
    /* 初始化以太网 */
    status = init_ethernet();
    if (status != ETH_OK) {
        return -1;
    }
    
    /* 初始化DDS */
    status = init_dds();
    if (status != ETH_OK) {
        cleanup();
        return -1;
    }
    
    printf("\nDemo started. Publishing sensor data every 1 second...\n");
    printf("Press Ctrl+C to exit\n\n");
    
    /* 主循环 */
    while (1) {
        /* 更新传感器数据 */
        sensor_data.timestamp = counter;
        sensor_data.temperature = 25.0f + (counter % 10);
        sensor_data.humidity = 55.0f + (counter % 20);
        
        /* 发布传感器数据 */
        status = dds_write(g_sensor_writer, &sensor_data, sizeof(sensor_data), 100);
        if (status == ETH_OK) {
            printf("Published Sensor[%u]: Temp=%.1fC, Humidity=%.1f%%\n",
                   counter, sensor_data.temperature, sensor_data.humidity);
        } else {
            printf("Failed to publish sensor data: %d\n", status);
        }
        
        /* 处理DDS事务 (接收数据) */
        dds_spin_once(100);
        
        counter++;
        
        /* 模拟1秒周期 */
        /* 在真实环境中使用OS延时或定时器 */
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    cleanup();
    return 0;
}
