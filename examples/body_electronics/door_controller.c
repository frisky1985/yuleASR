/******************************************************************************
 * @file    door_controller.c
 * @brief   Door Management Publisher
 *
 * Publishes door status and central locking with E2E protection
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "body_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"

#define DOOR_TOPIC_NAME     "Body/Door/Status"
#define DOOR_TYPE_NAME      "DoorSystemStatusType"
#define DOOR_DOMAIN_ID      3
#define DOOR_PUBLISH_HZ     10

static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Door Controller] Shutting down...\n");
}

static void generate_door_status(DoorSystemStatusType* status)
{
    status->systemId = 0xA003;
    status->doorCount = 6;
    status->systemStatus = BODY_STATUS_OK;
    
    /* Front left door */
    status->doors[0].doorId = 0;
    status->doors[0].position = DOOR_POSITION_FRONT_LEFT;
    status->doors[0].state = DOOR_STATE_CLOSED_LOCKED;
    status->doors[0].openPercent = 0.0f;
    status->doors[0].locked = true;
    status->doors[0].childLockEnabled = false;
    status->doors[0].windowPosition = 0.0f;
    status->doors[0].windowObstruction = false;
    
    /* Front right door */
    status->doors[1].doorId = 1;
    status->doors[1].position = DOOR_POSITION_FRONT_RIGHT;
    status->doors[1].state = DOOR_STATE_CLOSED_LOCKED;
    status->doors[1].openPercent = 0.0f;
    status->doors[1].locked = true;
    status->doors[1].windowPosition = 0.0f;
    
    /* Rear doors */
    status->doors[2].doorId = 2;
    status->doors[2].position = DOOR_POSITION_REAR_LEFT;
    status->doors[2].state = DOOR_STATE_CLOSED_LOCKED;
    status->doors[2].locked = true;
    status->doors[2].windowPosition = 0.0f;
    
    status->doors[3].doorId = 3;
    status->doors[3].position = DOOR_POSITION_REAR_RIGHT;
    status->doors[3].state = DOOR_STATE_CLOSED_LOCKED;
    status->doors[3].locked = true;
    status->doors[3].windowPosition = 0.0f;
    
    /* Tailgate */
    status->doors[4].doorId = 4;
    status->doors[4].position = DOOR_POSITION_TAILGATE;
    status->doors[4].state = DOOR_STATE_CLOSED_LOCKED;
    status->doors[4].openPercent = 0.0f;
    status->doors[4].locked = true;
    
    /* Hood */
    status->doors[5].doorId = 5;
    status->doors[5].position = DOOR_POSITION_HOOD;
    status->doors[5].state = DOOR_STATE_CLOSED_UNLOCKED;
    status->doors[5].openPercent = 0.0f;
    status->doors[5].locked = false;
    
    /* Central locking */
    status->centralLockLocked = true;
    status->centralLockDeadlocked = false;
    status->walkAwayLockEnabled = true;
    status->walkAwayLockActive = true;
    status->speedAutoLockEnabled = true;
    status->speedAutoLockTriggered = false;
    status->keylessEntryEnabled = true;
    status->keyFobCount = 2;
    
    status->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  Door Controller Module\n");
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
    g_e2eContext.config.p05.dataId = BODY_E2E_DATAID_DOOR;
    g_e2eContext.config.p05.dataLength = sizeof(DoorSystemStatusType);
    
    dds_domain_participant_t* participant = dds_create_participant(DOOR_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_RELIABLE, DDS_QOS_DURABILITY_VOLATILE, 150, 10, 2 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, DOOR_TOPIC_NAME, DOOR_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[Door Controller] Running...\n");
    
    DoorSystemStatusType status;
    uint32_t count = 0;
    
    while (g_running) {
        generate_door_status(&status);
        uint32_t len = sizeof(status);
        E2E_P05_Protect(&g_e2eContext, &status, &len);
        
        if (dds_write(writer, &status, sizeof(status), 50) == ETH_OK) {
            if (++count % 10 == 0) {
                printf("[Door] All locked: %s, Keyless: %s\n",
                       status.centralLockLocked ? "Yes" : "No",
                       status.keylessEntryEnabled ? "Active" : "Inactive");
            }
        }
        usleep(1000000 / DOOR_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    return 0;
}
