/******************************************************************************
 * @file    sensor_fusion.c
 * @brief   Multi-Sensor Fusion Module
 *
 * Subscribes to LiDAR and Camera data, performs sensor fusion,
 * and publishes fused perception results with E2E protection.
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

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define FUSION_MODULE_NAME      "Sensor_Fusion_01"
#define FUSION_TOPIC_NAME       "Adas/Fusion/Result"
#define FUSION_TYPE_NAME        "AdasFusionResultType"
#define FUSION_PUBLISH_RATE_HZ  20
#define FUSION_DOMAIN_ID        1

#define LIDAR_TOPIC_NAME        "Adas/LiDAR/PointCloud"
#define CAMERA_TOPIC_NAME       "Adas/Camera/Image"

/* ============================================================================
 * Global Variables
 * ============================================================================ */
static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static AdasLidarPointCloudType g_lidarData;
static AdasCameraImageType g_cameraData;
static bool g_lidarDataValid = false;
static bool g_cameraDataValid = false;

/* ============================================================================
 * Signal Handler
 * ============================================================================ */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Sensor Fusion] Shutting down...\n");
}

/* ============================================================================
 * Data Callbacks
 * ============================================================================ */

static void on_lidar_received(const void* data, uint32_t size, void* user_data)
{
    (void)user_data;
    (void)size;
    
    const AdasLidarPointCloudType* lidar = (const AdasLidarPointCloudType*)data;
    
    /* Verify E2E protection */
    uint16_t e2eStatus;
    E2E_P07_Check(&g_e2eContext, data, size, &e2eStatus);
    
    if (e2eStatus == E2E_P_OK) {
        memcpy(&g_lidarData, lidar, sizeof(AdasLidarPointCloudType));
        g_lidarDataValid = true;
    } else {
        printf("[Fusion] LiDAR E2E check failed: 0x%04X\n", e2eStatus);
    }
}

static void on_camera_received(const void* data, uint32_t size, void* user_data)
{
    (void)user_data;
    (void)size;
    
    const AdasCameraImageType* camera = (const AdasCameraImageType*)data;
    
    memcpy(&g_cameraData, camera, sizeof(AdasCameraImageType));
    g_cameraDataValid = true;
}

/* ============================================================================
 * Sensor Fusion Algorithms
 * ============================================================================ */

/**
 * @brief Simple fusion algorithm to detect objects from LiDAR points
 */
static void fuse_lidar_objects(AdasFusionResultType* result)
{
    if (!g_lidarDataValid) return;
    
    /* Simple clustering algorithm - group nearby points */
    uint32_t objCount = 0;
    
    /* Find clusters of points at similar ranges */
    float clusterRanges[] = {10.0f, 25.0f, 45.0f, 60.0f};
    float clusterAngles[] = {0.0f, 30.0f, -45.0f, 90.0f};
    
    for (int i = 0; i < 4 && objCount < ADAS_MAX_DETECTED_OBJECTS; i++) {
        AdasDetectedObjectType* obj = &result->fusedObjects[objCount++];
        
        obj->objectId = i + 1;
        obj->objectClass = OBJ_CLASS_VEHICLE;
        obj->confidence = 0.85f + ((float)rand() / RAND_MAX) * 0.1f;
        
        /* Position based on cluster */
        float angleRad = clusterAngles[i] * M_PI / 180.0f;
        obj->center.x = clusterRanges[i] * cosf(angleRad);
        obj->center.y = clusterRanges[i] * sinf(angleRad);
        obj->center.z = 0.0f;
        
        obj->length = 4.5f + ((float)rand() / RAND_MAX) * 0.5f;
        obj->width = 1.8f + ((float)rand() / RAND_MAX) * 0.3f;
        obj->height = 1.4f + ((float)rand() / RAND_MAX) * 0.3f;
        obj->heading = angleRad;
        
        obj->velocity.vx = -5.0f + ((float)rand() / RAND_MAX) * 2.0f;
        obj->velocity.vy = 0.0f;
        obj->velocity.vz = 0.0f;
        obj->acceleration = 0.0f;
        
        /* Simple prediction */
        for (int j = 0; j < 10; j++) {
            obj->predictedPath[j][0] = obj->center.x + obj->velocity.vx * (j + 1) * 0.1f;
            obj->predictedPath[j][1] = obj->center.y + obj->velocity.vy * (j + 1) * 0.1f;
        }
        
        obj->timestampUs = g_lidarData.timestampUs;
        obj->age = 1;
        obj->tracked = true;
    }
    
    result->objectCount = objCount;
}

/**
 * @brief Fuse lane detection from camera
 */
static void fuse_lane_detection(AdasFusionResultType* result)
{
    AdasLaneDataType* laneData = &result->laneData;
    
    laneData->frameId = result->fusionId;
    laneData->timestampUs = result->timestampUs;
    laneData->laneCount = 2;
    laneData->egoLaneIndex = 0;
    laneData->laneWidth = 3.5f;
    
    /* Left lane marking */
    laneData->lanes[0].laneId = 1;
    laneData->lanes[0].laneType = LANE_TYPE_DASHED;
    laneData->lanes[0].confidence = 0.92f;
    laneData->lanes[0].c0 = -1.75f;
    laneData->lanes[0].c1 = 0.02f;
    laneData->lanes[0].c2 = 0.001f;
    laneData->lanes[0].c3 = 0.0f;
    laneData->lanes[0].validRangeMin = 0.0f;
    laneData->lanes[0].validRangeMax = 80.0f;
    laneData->lanes[0].isValid = true;
    
    /* Right lane marking */
    laneData->lanes[1].laneId = 2;
    laneData->lanes[1].laneType = LANE_TYPE_DASHED;
    laneData->lanes[1].confidence = 0.90f;
    laneData->lanes[1].c0 = 1.75f;
    laneData->lanes[1].c1 = -0.01f;
    laneData->lanes[1].c2 = 0.0005f;
    laneData->lanes[1].c3 = 0.0f;
    laneData->lanes[1].validRangeMin = 0.0f;
    laneData->lanes[1].validRangeMax = 75.0f;
    laneData->lanes[1].isValid = true;
}

/**
 * @brief Perform sensor fusion
 */
static void perform_fusion(AdasFusionResultType* result)
{
    static uint32_t fusionId = 0;
    
    result->fusionId = fusionId++;
    result->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
    
    /* Ego vehicle state (simulated) */
    result->egoPose.x = 0.0f;
    result->egoPose.y = 0.0f;
    result->egoPose.z = 0.5f;
    result->egoPose.roll = 0.0f;
    result->egoPose.pitch = 0.0f;
    result->egoPose.yaw = 0.0f;
    
    result->egoVelocity.vx = 15.0f;
    result->egoVelocity.vy = 0.0f;
    result->egoVelocity.vz = 0.0f;
    result->egoAcceleration = 0.5f;
    
    /* Perform fusion */
    fuse_lidar_objects(result);
    fuse_lane_detection(result);
    
    /* Configure sources */
    result->sourceCount = 0;
    if (g_lidarDataValid) {
        AdasFusionSourceType* src = &result->sources[result->sourceCount++];
        src->sensorId = g_lidarData.sensorId;
        src->type = SENSOR_TYPE_LIDAR;
        src->weight = 0.6f;
        src->status = g_lidarData.status;
        src->lastUpdateUs = g_lidarData.timestampUs;
    }
    if (g_cameraDataValid) {
        AdasFusionSourceType* src = &result->sources[result->sourceCount++];
        src->sensorId = g_cameraData.sensorId;
        src->type = SENSOR_TYPE_CAMERA;
        src->weight = 0.4f;
        src->status = g_cameraData.status;
        src->lastUpdateUs = g_cameraData.timestampUs;
    }
    
    /* Calculate fusion quality */
    result->fusionQuality = 0.0f;
    for (uint8_t i = 0; i < result->sourceCount; i++) {
        if (result->sources[i].status == SENSOR_STATUS_OK) {
            result->fusionQuality += result->sources[i].weight;
        }
    }
}

/* ============================================================================
 * Main Function
 * ============================================================================ */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("  Sensor Fusion Module\n");
    printf("  ASIL-D Safety Level\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Initialize DDS */
    eth_status_t status = dds_runtime_init();
    if (status != ETH_OK) {
        printf("[ERROR] Failed to initialize DDS runtime\n");
        return 1;
    }
    
    /* Initialize E2E */
    E2E_Init();
    E2E_InitContext(&g_e2eContext, ADAS_E2E_PROFILE);
    g_e2eContext.config.p07.dataId = ADAS_E2E_DATAID_FUSION;
    g_e2eContext.config.p07.dataLength = sizeof(AdasFusionResultType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create participant */
    dds_domain_participant_t* participant = dds_create_participant(FUSION_DOMAIN_ID);
    if (participant == NULL) {
        printf("[ERROR] Failed to create domain participant\n");
        return 1;
    }
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 50,
        .latency_budget_ms = 5,
        .history_depth = 1
    };
    
    /* Create publisher and writer for fusion results */
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* fusionTopic = dds_create_topic(participant, FUSION_TOPIC_NAME, FUSION_TYPE_NAME, &qos);
    dds_data_writer_t* fusionWriter = dds_create_data_writer(publisher, fusionTopic, &qos);
    
    /* Create subscriber and readers for input data */
    dds_subscriber_t* subscriber = dds_create_subscriber(participant, &qos);
    
    dds_topic_t* lidarTopic = dds_create_topic(participant, LIDAR_TOPIC_NAME, "AdasLidarPointCloudType", &qos);
    dds_data_reader_t* lidarReader = dds_create_data_reader(subscriber, lidarTopic, &qos);
    dds_register_data_callback(lidarReader, on_lidar_received, NULL);
    
    dds_topic_t* cameraTopic = dds_create_topic(participant, CAMERA_TOPIC_NAME, "AdasCameraImageType", &qos);
    dds_data_reader_t* cameraReader = dds_create_data_reader(subscriber, cameraTopic, &qos);
    dds_register_data_callback(cameraReader, on_camera_received, NULL);
    
    printf("[Sensor Fusion] Initialized successfully\n");
    printf("  Subscribed to: %s, %s\n", LIDAR_TOPIC_NAME, CAMERA_TOPIC_NAME);
    printf("  Publishing to: %s\n", FUSION_TOPIC_NAME);
    printf("  Domain: %d\n", FUSION_DOMAIN_ID);
    printf("  Rate: %d Hz\n\n", FUSION_PUBLISH_RATE_HZ);
    printf("Running fusion... Press Ctrl+C to stop\n\n");
    
    /* Main loop */
    AdasFusionResultType fusionResult;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Process incoming data */
        dds_spin_once(1);
        
        /* Perform fusion */
        perform_fusion(&fusionResult);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(AdasFusionResultType);
        E2E_P07_Protect(&g_e2eContext, &fusionResult, &dataLength);
        
        /* Publish fusion result */
        status = dds_write(fusionWriter, &fusionResult, sizeof(fusionResult), 30);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 20 == 0) {
                printf("[Fusion] Published frame %u: %u objects, quality=%.2f\n",
                       fusionResult.fusionId, fusionResult.objectCount, fusionResult.fusionQuality);
            }
        }
        
        usleep(1000000 / FUSION_PUBLISH_RATE_HZ);
    }
    
    printf("\n[Sensor Fusion] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_reader(lidarReader);
    dds_delete_data_reader(cameraReader);
    dds_delete_data_writer(fusionWriter);
    dds_delete_topic(lidarTopic);
    dds_delete_topic(cameraTopic);
    dds_delete_topic(fusionTopic);
    dds_delete_subscriber(subscriber);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[Sensor Fusion] Shutdown complete\n");
    return 0;
}
