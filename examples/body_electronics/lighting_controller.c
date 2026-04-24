/******************************************************************************
 * @file    lighting_controller.c
 * @brief   Lighting System Publisher
 *
 * Publishes lighting status with E2E protection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include "body_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"

#define LIGHT_TOPIC_NAME    "Body/Lighting/Status"
#define LIGHT_TYPE_NAME     "LightingSystemStatusType"
#define LIGHT_DOMAIN_ID     3
#define LIGHT_PUBLISH_HZ    20

static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Lighting Controller] Shutting down...\n");
}

static void generate_lighting_status(LightingSystemStatusType* status)
{
    static float simTime = 0;
    simTime += 0.05f;
    
    status->systemId = 0xA004;
    status->lightCount = 8;
    status->systemStatus = BODY_STATUS_OK;
    
    /* Headlights */
    status->lights[0].lightId = 0;
    status->lights[0].type = LIGHT_TYPE_HEADLIGHT_LOW;
    status->lights[0].state = LIGHT_STATE_ON;
    status->lights[0].brightnessPercent = 80;
    status->lights[0].autoBrightness = true;
    
    /* Daytime running lights */
    status->lights[1].lightId = 1;
    status->lights[1].type = LIGHT_TYPE_DAYTIME_RUNNING;
    status->lights[1].state = LIGHT_STATE_ON;
    status->lights[1].brightnessPercent = 100;
    
    /* Tail lights */
    status->lights[2].lightId = 2;
    status->lights[2].type = LIGHT_TYPE_TAIL;
    status->lights[2].state = LIGHT_STATE_ON;
    status->lights[2].brightnessPercent = 50;
    
    /* Brake lights */
    status->lights[3].lightId = 3;
    status->lights[3].type = LIGHT_TYPE_BRAKE;
    status->lights[3].state = LIGHT_STATE_OFF;
    status->lights[3].brightnessPercent = 0;
    
    /* Turn signals */
    status->lights[4].lightId = 4;
    status->lights[4].type = LIGHT_TYPE_TURN_LEFT;
    status->lights[4].state = (simTime > 5.0f && simTime < 6.0f) ? LIGHT_STATE_BLINKING : LIGHT_STATE_OFF;
    status->lights[4].brightnessPercent = 100;
    
    status->lights[5].lightId = 5;
    status->lights[5].type = LIGHT_TYPE_TURN_RIGHT;
    status->lights[5].state = LIGHT_STATE_OFF;
    
    /* Interior lights */
    status->lights[6].lightId = 6;
    status->lights[6].type = LIGHT_TYPE_INTERIOR_FRONT;
    status->lights[6].state = LIGHT_STATE_OFF;
    status->lights[6].brightnessPercent = 0;
    
    /* Ambient lighting */
    status->lights[7].lightId = 7;
    status->lights[7].type = LIGHT_TYPE_AMBIENT;
    status->lights[7].state = LIGHT_STATE_ON;
    status->lights[7].brightnessPercent = 30;
    status->lights[7].red = 100;
    status->lights[7].green = 100;
    status->lights[7].blue = 200;
    
    /* System settings */
    status->autoHeadlights = true;
    status->adaptiveHeadlights = true;
    status->headlightLeveling = 0.5f;
    status->highBeamAssist = true;
    status->highBeamActive = false;
    
    status->ambientLightingEnabled = true;
    status->ambientBrightness = 30;
    status->ambientTheme = 1;
    
    status->ambientLightLevel = 500.0f;  /* Lux - daylight */
    status->darknessDetected = false;
    
    status->turnSignalState = (simTime > 5.0f && simTime < 6.0f) ? 1 : 0;
    status->hazardLightsActive = false;
    
    status->brakeLightsActive = false;
    status->emergencyBrakingSignal = false;
    
    status->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  Lighting Controller Module\n");
    printf("  ASIL-B Safety Level\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (dds_runtime_init() != ETH_OK) {
        printf("[ERROR] DDS init failed\n");
        return 1;
    }
    
    E2E_Init();
    E2E_InitContext(&g_e2eContext, BODY_E2E_PROFILE);
    g_e2eContext.config.p05.dataId = BODY_E2E_DATAID_LIGHTING;
    g_e2eContext.config.p05.dataLength = sizeof(LightingSystemStatusType);
    
    dds_domain_participant_t* participant = dds_create_participant(LIGHT_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_RELIABLE, DDS_QOS_DURABILITY_VOLATILE, 75, 5, 1 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, LIGHT_TOPIC_NAME, LIGHT_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[Lighting Controller] Running...\n");
    
    LightingSystemStatusType status;
    uint32_t count = 0;
    
    while (g_running) {
        generate_lighting_status(&status);
        uint32_t len = sizeof(status);
        E2E_P05_Protect(&g_e2eContext, &status, &len);
        
        if (dds_write(writer, &status, sizeof(status), 30) == ETH_OK) {
            if (++count % 20 == 0) {
                const char* turnState = "Off";
                if (status.turnSignalState == 1) turnState = "Left";
                else if (status.turnSignalState == 2) turnState = "Right";
                else if (status.turnSignalState == 3) turnState = "Hazard";
                
                printf("[Lighting] Low beam: %s, Turn: %s, Ambient: %d%%\n",
                       status.lights[0].state == LIGHT_STATE_ON ? "ON" : "OFF",
                       turnState,
                       status.ambientBrightness);
            }
        }
        usleep(1000000 / LIGHT_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    return 0;
}
