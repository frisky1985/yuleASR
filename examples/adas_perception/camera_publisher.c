/******************************************************************************
 * @file    camera_publisher.c
 * @brief   Camera Image Publisher
 *
 * Simulates a camera sensor and publishes image data via DDS
 * with E2E protection for ASIL-D safety requirements.
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
#include "../../src/common/log/dds_log.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define CAMERA_PUBLISHER_NAME   "Camera_Publisher_01"
#define CAMERA_TOPIC_NAME       "Adas/Camera/Image"
#define CAMERA_TYPE_NAME        "AdasCameraImageType"
#define CAMERA_PUBLISH_RATE_HZ  30
#define CAMERA_DOMAIN_ID        1
#define CAMERA_WIDTH            640
#define CAMERA_HEIGHT           480

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
    printf("\n[Camera Publisher] Shutting down...\n");
}

/* ============================================================================
 * Camera Simulation Functions
 * ============================================================================ */

/**
 * @brief Generate simulated camera image
 * @param image Output image data
 */
static void generate_camera_image(AdasCameraImageType* image)
{
    static uint32_t frameId = 0;
    
    image->sensorId = 0x2001;
    image->frameId = frameId++;
    image->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
    image->width = CAMERA_WIDTH;
    image->height = CAMERA_HEIGHT;
    image->format = CAM_FORMAT_RGB888;
    image->status = SENSOR_STATUS_OK;
    image->exposureTimeMs = 10.0f;
    image->gainDb = 0.0f;
    
    /* Generate synthetic image pattern */
    uint32_t pixelIndex = 0;
    for (uint16_t y = 0; y < CAMERA_HEIGHT; y++) {
        for (uint16_t x = 0; x < CAMERA_WIDTH; x++) {
            /* Create a pattern with simulated road and lane markings */
            uint8_t r, g, b;
            
            /* Horizon line at 40% of image */
            if (y < CAMERA_HEIGHT * 0.4f) {
                /* Sky - gradient blue */
                r = 100 + (y * 155 / (CAMERA_HEIGHT * 0.4f));
                g = 150 + (y * 105 / (CAMERA_HEIGHT * 0.4f));
                b = 255;
            } else {
                /* Road - gray with perspective */
                float roadY = (float)(y - CAMERA_HEIGHT * 0.4f) / (CAMERA_HEIGHT * 0.6f);
                uint8_t gray = 60 + (uint8_t)(roadY * 40);
                
                r = gray;
                g = gray;
                b = gray;
                
                /* Lane markings */
                int centerX = CAMERA_WIDTH / 2;
                int laneOffset = (int)((roadY * roadY) * (CAMERA_WIDTH * 0.3f));
                
                /* Left lane marking */
                if (abs(x - (centerX - laneOffset)) < (int)(2 + roadY * 3)) {
                    r = 255; g = 255; b = 255;
                }
                /* Right lane marking */
                if (abs(x - (centerX + laneOffset)) < (int)(2 + roadY * 3)) {
                    r = 255; g = 255; b = 255;
                }
                
                /* Center dashed line */
                if (abs(x - centerX) < 2 && ((y / 20) % 2 == 0)) {
                    r = 255; g = 255; b = 255;
                }
                
                /* Simulated vehicle ahead */
                int vehicleY = (int)(CAMERA_HEIGHT * 0.7f);
                int vehicleWidth = 40;
                int vehicleHeight = 35;
                int vehicleX = centerX;
                
                if (abs((int)x - vehicleX) < vehicleWidth/2 && 
                    abs((int)y - vehicleY) < vehicleHeight/2) {
                    /* Vehicle body - red */
                    r = 200; g = 50; b = 50;
                    
                    /* Taillights */
                    if (y > vehicleY + vehicleHeight/4) {
                        if (x < vehicleX - vehicleWidth/3 || x > vehicleX + vehicleWidth/3) {
                            r = 255; g = 0; b = 0;
                        }
                    }
                }
            }
            
            /* Add some noise */
            uint8_t noise = (uint8_t)(rand() % 20);
            r = (r > noise) ? r - noise : 0;
            g = (g > noise) ? g - noise : 0;
            b = (b > noise) ? b - noise : 0;
            
            image->imageData[pixelIndex++] = r;
            image->imageData[pixelIndex++] = g;
            image->imageData[pixelIndex++] = b;
        }
    }
    
    image->imageDataSize = pixelIndex;
}

/* ============================================================================
 * Main Function
 * ============================================================================ */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("  Camera Image Publisher\n");
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
    g_e2eContext.config.p07.dataId = ADAS_E2E_DATAID_CAMERA;
    g_e2eContext.config.p07.dataLength = sizeof(AdasCameraImageType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create domain participant */
    dds_domain_participant_t* participant = dds_create_participant(CAMERA_DOMAIN_ID);
    if (participant == NULL) {
        printf("[ERROR] Failed to create domain participant\n");
        return 1;
    }
    
    /* Configure QoS */
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 50,
        .latency_budget_ms = 5,
        .history_depth = 2
    };
    
    /* Create publisher */
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    if (publisher == NULL) {
        printf("[ERROR] Failed to create publisher\n");
        dds_delete_participant(participant);
        return 1;
    }
    
    /* Create topic */
    dds_topic_t* topic = dds_create_topic(participant, CAMERA_TOPIC_NAME, CAMERA_TYPE_NAME, &qos);
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
    
    printf("[Camera Publisher] Initialized successfully\n");
    printf("  Topic: %s\n", CAMERA_TOPIC_NAME);
    printf("  Domain: %d\n", CAMERA_DOMAIN_ID);
    printf("  Resolution: %dx%d\n", CAMERA_WIDTH, CAMERA_HEIGHT);
    printf("  Rate: %d Hz\n", CAMERA_PUBLISH_RATE_HZ);
    printf("  E2E Profile: P07 (CRC32 + Counter)\n\n");
    printf("Publishing data... Press Ctrl+C to stop\n\n");
    
    /* Main publishing loop */
    AdasCameraImageType image;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Generate simulated camera data */
        generate_camera_image(&image);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(AdasCameraImageType);
        E2E_P07_Protect(&g_e2eContext, &image, &dataLength);
        
        /* Publish data */
        status = dds_write(writer, &image, sizeof(image), 30);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 30 == 0) {
                printf("[Camera Publisher] Published frame %u (%.2f MB)\n", 
                       image.frameId, image.imageDataSize / (1024.0f * 1024.0f));
            }
        } else {
            printf("[Camera Publisher] Failed to publish: %d\n", status);
        }
        
        /* Sleep until next frame (30Hz = 33.33ms) */
        usleep(1000000 / CAMERA_PUBLISH_RATE_HZ);
    }
    
    printf("\n[Camera Publisher] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[Camera Publisher] Shutdown complete\n");
    return 0;
}
