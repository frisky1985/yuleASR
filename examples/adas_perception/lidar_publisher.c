/******************************************************************************
 * @file    lidar_publisher.c
 * @brief   LiDAR Point Cloud Publisher
 *
 * Simulates a LiDAR sensor and publishes point cloud data via DDS
 * with E2E protection for ASIL-D safety requirements.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include "adas_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"
#include "../../src/common/log/dds_log.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define LIDAR_PUBLISHER_NAME    "LiDAR_Publisher_01"
#define LIDAR_TOPIC_NAME        "Adas/LiDAR/PointCloud"
#define LIDAR_TYPE_NAME         "AdasLidarPointCloudType"
#define LIDAR_PUBLISH_RATE_HZ   10
#define LIDAR_DOMAIN_ID         1

#define LIDAR_HORIZONTAL_RES    0.2f    /* degrees */
#define LIDAR_VERTICAL_RES      0.4f    /* degrees */
#define LIDAR_HORIZONTAL_FOV    360.0f  /* degrees */
#define LIDAR_VERTICAL_FOV      30.0f   /* degrees */
#define LIDAR_MAX_RANGE         200.0f  /* meters */
#define LIDAR_MIN_RANGE         0.5f    /* meters */

/* ============================================================================
 * Global Variables
 * ============================================================================ */
static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

/* ============================================================================
 * Signal Handler
 * ============================================================================ */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[LiDAR Publisher] Shutting down...\n");
}

/* ============================================================================
 * LiDAR Simulation Functions
 * ============================================================================ */

/**
 * @brief Generate simulated LiDAR point cloud
 * @param pointCloud Output point cloud data
 */
static void generate_lidar_points(AdasLidarPointCloudType* pointCloud)
{
    static uint32_t frameId = 0;
    
    pointCloud->sensorId = 0x1001;
    pointCloud->frameId = frameId++;
    pointCloud->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
    pointCloud->status = SENSOR_STATUS_OK;
    
    /* Sensor pose - mounted on vehicle roof */
    pointCloud->sensorPose.x = 1.5f;
    pointCloud->sensorPose.y = 0.0f;
    pointCloud->sensorPose.z = 1.8f;
    pointCloud->sensorPose.roll = 0.0f;
    pointCloud->sensorPose.pitch = 0.0f;
    pointCloud->sensorPose.yaw = 0.0f;
    
    /* Generate points in a simulated environment */
    uint32_t pointCount = 0;
    
    /* Generate points for simulated obstacles */
    const float obstacleRanges[] = {10.0f, 25.0f, 45.0f, 60.0f};
    const float obstacleAngles[] = {0.0f, 30.0f, -45.0f, 90.0f};
    
    for (int i = 0; i < 4 && pointCount < ADAS_MAX_LIDAR_POINTS; i++) {
        for (float hAngle = -10.0f; hAngle <= 10.0f; hAngle += LIDAR_HORIZONTAL_RES) {
            for (float vAngle = -5.0f; vAngle <= 5.0f; vAngle += LIDAR_VERTICAL_RES) {
                if (pointCount >= ADAS_MAX_LIDAR_POINTS) break;
                
                float angleRad = (obstacleAngles[i] + hAngle) * M_PI / 180.0f;
                float range = obstacleRanges[i] + ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
                
                AdasLidarPointType* pt = &pointCloud->points[pointCount++];
                pt->x = range * cosf(angleRad);
                pt->y = range * sinf(angleRad);
                pt->z = (vAngle / 180.0f) * M_PI * range * 0.1f + pointCloud->sensorPose.z;
                pt->intensity = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
                pt->ring = (uint8_t)((vAngle + LIDAR_VERTICAL_FOV/2) / LIDAR_VERTICAL_RES);
                pt->timestamp = pointCloud->timestampUs;
            }
        }
    }
    
    /* Add ground plane points */
    for (float hAngle = 0.0f; hAngle < 360.0f; hAngle += 5.0f) {
        if (pointCount >= ADAS_MAX_LIDAR_POINTS) break;
        
        for (int ring = 0; ring < 16; ring++) {
            if (pointCount >= ADAS_MAX_LIDAR_POINTS) break;
            
            float angleRad = hAngle * M_PI / 180.0f;
            float range = 5.0f + ((float)ring * 10.0f);
            float vAngle = -15.0f + ring * LIDAR_VERTICAL_RES;
            
            AdasLidarPointType* pt = &pointCloud->points[pointCount++];
            pt->x = range * cosf(angleRad);
            pt->y = range * sinf(angleRad);
            pt->z = pointCloud->sensorPose.z + range * tanf(vAngle * M_PI / 180.0f);
            pt->intensity = 0.3f + ((float)rand() / RAND_MAX) * 0.2f;
            pt->ring = (uint8_t)ring;
            pt->timestamp = pointCloud->timestampUs;
        }
    }
    
    pointCloud->pointCount = pointCount;
}

/* ============================================================================
 * DDS Callbacks
 * ============================================================================ */

static void on_data_written(void* user_data)
{
    (void)user_data;
    /* Log occasional success */
}

/* ============================================================================
 * Main Function
 * ============================================================================ */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("  LiDAR Point Cloud Publisher\n");
    printf("  ASIL-D Safety Level\n");
    printf("========================================\n\n");
    
    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Initialize DDS runtime */
    eth_status_t status = dds_runtime_init();
    if (status != ETH_OK) {
        printf("[ERROR] Failed to initialize DDS runtime\n");
        return 1;
    }
    
    /* Initialize E2E protection */
    E2E_Init();
    E2E_InitContext(&g_e2eContext, ADAS_E2E_PROFILE);
    g_e2eContext.config.p07.dataId = ADAS_E2E_DATAID_LIDAR;
    g_e2eContext.config.p07.dataLength = sizeof(AdasLidarPointCloudType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create domain participant */
    dds_domain_participant_t* participant = dds_create_participant(LIDAR_DOMAIN_ID);
    if (participant == NULL) {
        printf("[ERROR] Failed to create domain participant\n");
        return 1;
    }
    
    /* Configure QoS for reliable, real-time transmission */
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 100,
        .latency_budget_ms = 10,
        .history_depth = 1
    };
    
    /* Create publisher */
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    if (publisher == NULL) {
        printf("[ERROR] Failed to create publisher\n");
        dds_delete_participant(participant);
        return 1;
    }
    
    /* Create topic */
    dds_topic_t* topic = dds_create_topic(participant, LIDAR_TOPIC_NAME, LIDAR_TYPE_NAME, &qos);
    if (topic == NULL) {
        printf("[ERROR] Failed to create topic\n");
        dds_delete_publisher(publisher);
        dds_delete_participant(participant);
        return 1;
    }
    
    /* Create data writer */
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    if (writer == NULL) {
        printf("[ERROR] Failed to create data writer\n");
        dds_delete_topic(topic);
        dds_delete_publisher(publisher);
        dds_delete_participant(participant);
        return 1;
    }
    
    dds_register_write_callback(writer, on_data_written, NULL);
    
    printf("[LiDAR Publisher] Initialized successfully\n");
    printf("  Topic: %s\n", LIDAR_TOPIC_NAME);
    printf("  Domain: %d\n", LIDAR_DOMAIN_ID);
    printf("  Rate: %d Hz\n", LIDAR_PUBLISH_RATE_HZ);
    printf("  E2E Profile: P07 (CRC32 + Counter)\n\n");
    printf("Publishing data... Press Ctrl+C to stop\n\n");
    
    /* Main publishing loop */
    AdasLidarPointCloudType pointCloud;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Generate simulated LiDAR data */
        generate_lidar_points(&pointCloud);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(AdasLidarPointCloudType);
        E2E_P07_Protect(&g_e2eContext, &pointCloud, &dataLength);
        
        /* Publish data */
        status = dds_write(writer, &pointCloud, sizeof(pointCloud), 50);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 10 == 0) {
                printf("[LiDAR Publisher] Published frame %u (%u points)\n", 
                       pointCloud.frameId, pointCloud.pointCount);
            }
        } else {
            printf("[LiDAR Publisher] Failed to publish: %d\n", status);
        }
        
        /* Sleep until next cycle */
        usleep(1000000 / LIDAR_PUBLISH_RATE_HZ);
    }
    
    printf("\n[LiDAR Publisher] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[LiDAR Publisher] Shutdown complete\n");
    return 0;
}
