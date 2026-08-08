/******************************************************************************
 * @file    object_detector.c
 * @brief   Object Detection Publisher
 *
 * Subscribes to fusion results and publishes detected objects
 * with additional classification and tracking information.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "adas_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define DETECTOR_NAME           "Object_Detector_01"
#define OBJECTS_TOPIC_NAME      "Adas/Objects/Detected"
#define OBJECTS_TYPE_NAME       "AdasObjectListType"
#define OBJECTS_PUBLISH_RATE_HZ 20
#define OBJECTS_DOMAIN_ID       1

#define FUSION_TOPIC_NAME       "Adas/Fusion/Result"

/* ============================================================================
 * Global Variables
 * ============================================================================ */
static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;
static AdasFusionResultType g_fusionData;
static bool g_fusionDataValid = false;

/* ============================================================================
 * Signal Handler
 * ============================================================================ */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Object Detector] Shutting down...\n");
}

/* ============================================================================
 * Data Callback
 * ============================================================================ */

static void on_fusion_received(const void* data, uint32_t size, void* user_data)
{
    (void)user_data;
    (void)size;
    
    const AdasFusionResultType* fusion = (const AdasFusionResultType*)data;
    
    /* Verify E2E protection */
    uint16_t e2eStatus;
    E2E_P07_Check(&g_e2eContext, data, size, &e2eStatus);
    
    if (e2eStatus == E2E_P_OK) {
        memcpy(&g_fusionData, fusion, sizeof(AdasFusionResultType));
        g_fusionDataValid = true;
    }
}

/* ============================================================================
 * Object Detection Functions
 * ============================================================================ */

/**
 * @brief Create object list from fusion data
 */
static void create_object_list(AdasObjectListType* objectList)
{
    static uint32_t frameId = 0;
    
    objectList->frameId = frameId++;
    objectList->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
    objectList->sensorId = 0x4001;
    
    if (!g_fusionDataValid) {
        objectList->objectCount = 0;
        return;
    }
    
    /* Copy objects from fusion result */
    objectList->objectCount = g_fusionData.objectCount;
    if (objectList->objectCount > ADAS_MAX_DETECTED_OBJECTS) {
        objectList->objectCount = ADAS_MAX_DETECTED_OBJECTS;
    }
    
    for (uint32_t i = 0; i < objectList->objectCount; i++) {
        memcpy(&objectList->objects[i], &g_fusionData.fusedObjects[i], 
               sizeof(AdasDetectedObjectType));
        
        /* Add tracking metadata */
        objectList->objects[i].age++;
        objectList->objects[i].tracked = (objectList->objects[i].age > 3);
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
    printf("  Object Detection Publisher\n");
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
    g_e2eContext.config.p07.dataId = ADAS_E2E_DATAID_OBJECTS;
    g_e2eContext.config.p07.dataLength = sizeof(AdasObjectListType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create participant */
    dds_domain_participant_t* participant = dds_create_participant(OBJECTS_DOMAIN_ID);
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
    
    /* Create publisher and writer for objects */
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* objectsTopic = dds_create_topic(participant, OBJECTS_TOPIC_NAME, OBJECTS_TYPE_NAME, &qos);
    dds_data_writer_t* objectsWriter = dds_create_data_writer(publisher, objectsTopic, &qos);
    
    /* Create subscriber and reader for fusion data */
    dds_subscriber_t* subscriber = dds_create_subscriber(participant, &qos);
    dds_topic_t* fusionTopic = dds_create_topic(participant, FUSION_TOPIC_NAME, "AdasFusionResultType", &qos);
    dds_data_reader_t* fusionReader = dds_create_data_reader(subscriber, fusionTopic, &qos);
    dds_register_data_callback(fusionReader, on_fusion_received, NULL);
    
    printf("[Object Detector] Initialized successfully\n");
    printf("  Subscribed to: %s\n", FUSION_TOPIC_NAME);
    printf("  Publishing to: %s\n", OBJECTS_TOPIC_NAME);
    printf("  Domain: %d\n", OBJECTS_DOMAIN_ID);
    printf("  Rate: %d Hz\n\n", OBJECTS_PUBLISH_RATE_HZ);
    printf("Detecting objects... Press Ctrl+C to stop\n\n");
    
    /* Main loop */
    AdasObjectListType objectList;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Process incoming data */
        dds_spin_once(1);
        
        /* Create object list */
        create_object_list(&objectList);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(AdasObjectListType);
        E2E_P07_Protect(&g_e2eContext, &objectList, &dataLength);
        
        /* Publish object list */
        status = dds_write(objectsWriter, &objectList, sizeof(objectList), 30);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 20 == 0) {
                printf("[Object Detector] Published frame %u: %u objects\n",
                       objectList.frameId, objectList.objectCount);
            }
        }
        
        usleep(1000000 / OBJECTS_PUBLISH_RATE_HZ);
    }
    
    printf("\n[Object Detector] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_reader(fusionReader);
    dds_delete_data_writer(objectsWriter);
    dds_delete_topic(fusionTopic);
    dds_delete_topic(objectsTopic);
    dds_delete_subscriber(subscriber);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[Object Detector] Shutdown complete\n");
    return 0;
}
