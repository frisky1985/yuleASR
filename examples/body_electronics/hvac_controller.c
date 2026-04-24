/******************************************************************************
 * @file    hvac_controller.c
 * @brief   HVAC (Climate Control) Publisher
 *
 * Publishes climate control status with E2E protection
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

#define HVAC_TOPIC_NAME     "Body/HVAC/Status"
#define HVAC_TYPE_NAME      "HvacStatusType"
#define HVAC_DOMAIN_ID      3
#define HVAC_PUBLISH_HZ     5

static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[HVAC Controller] Shutting down...\n");
}

static void generate_hvac_status(HvacStatusType* status)
{
    static float simTime = 0;
    simTime += 0.2f;
    
    status->systemId = 0xA002;
    status->mode = HVAC_MODE_AUTO;
    status->autoMode = true;
    status->acEnabled = true;
    status->recirculationEnabled = false;
    status->zoneCount = 2;
    
    /* Driver zone */
    status->zones[0].zone = BODY_ZONE_FRONT;
    status->zones[0].targetTempC = 22.0f;
    status->zones[0].actualTempC = 22.5f + sinf(simTime) * 0.5f;
    status->zones[0].fanSpeed = FAN_SPEED_MEDIUM;
    status->zones[0].fanSpeedPercent = 50;
    status->zones[0].ventFace = true;
    status->zones[0].ventFoot = true;
    
    /* Passenger zone */
    status->zones[1].zone = BODY_ZONE_FRONT;
    status->zones[1].targetTempC = 21.0f;
    status->zones[1].actualTempC = 21.5f + cosf(simTime) * 0.5f;
    status->zones[1].fanSpeed = FAN_SPEED_LOW;
    
    status->cabinTempC = 22.0f;
    status->outsideTempC = 28.0f;
    status->cabinHumidity = 0.45f;
    status->co2LevelPpm = 650;
    status->airQualityIndex = 45;
    
    status->compressorRunning = true;
    status->compressorSpeedRpm = 2500;
    status->refrigerantPressure = 15.5f;
    
    status->cabinFilterLifePercent = 75;
    status->rearDefrostEnabled = false;
    status->heatedSteeringWheel = false;
    
    status->powerConsumptionKw = 2.5f;
    status->systemStatus = BODY_STATUS_OK;
    status->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  HVAC Controller Module\n");
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
    g_e2eContext.config.p05.dataId = BODY_E2E_DATAID_HVAC;
    g_e2eContext.config.p05.dataLength = sizeof(HvacStatusType);
    
    dds_domain_participant_t* participant = dds_create_participant(HVAC_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_RELIABLE, DDS_QOS_DURABILITY_VOLATILE, 250, 20, 2 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, HVAC_TOPIC_NAME, HVAC_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[HVAC Controller] Running...\n");
    
    HvacStatusType status;
    uint32_t count = 0;
    
    while (g_running) {
        generate_hvac_status(&status);
        uint32_t len = sizeof(status);
        E2E_P05_Protect(&g_e2eContext, &status, &len);
        
        if (dds_write(writer, &status, sizeof(status), 100) == ETH_OK) {
            if (++count % 5 == 0) {
                printf("[HVAC] Cabin: %.1fC, Outside: %.1fC, AC: %s, Power: %.1fkW\n",
                       status.cabinTempC, status.outsideTempC,
                       status.acEnabled ? "ON" : "OFF", status.powerConsumptionKw);
            }
        }
        usleep(1000000 / HVAC_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    return 0;
}
