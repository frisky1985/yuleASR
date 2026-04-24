/******************************************************************************
 * @file    seat_controller.c
 * @brief   Seat Control Module Publisher
 *
 * Publishes seat position and comfort settings with E2E protection
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

#define SEAT_TOPIC_NAME     "Body/Seat/Status"
#define SEAT_TYPE_NAME      "SeatSystemStatusType"
#define SEAT_DOMAIN_ID      3
#define SEAT_PUBLISH_HZ     10

static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Seat Controller] Shutting down...\n");
}

static void generate_seat_status(SeatSystemStatusType* status)
{
    static float simTime = 0;
    simTime += 0.1f;
    
    status->systemId = 0xA001;
    status->seatCount = 4;
    status->systemStatus = BODY_STATUS_OK;
    
    /* Driver seat */
    SeatStatusType* driver = &status->seats[0];
    driver->seatId = 0;
    driver->position = SEAT_POSITION_DRIVER;
    driver->occupied = true;
    driver->beltBuckled = true;
    driver->slidePosition = 0.5f + 0.1f * sinf(simTime * 0.1f);
    driver->reclineAngle = 0.3f;
    driver->heightPosition = 0.4f;
    driver->heatingLevel = 2;
    driver->surfaceTemp = 35.0f;
    driver->airbagEnabled = true;
    driver->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
    
    /* Passenger seat */
    SeatStatusType* passenger = &status->seats[1];
    passenger->seatId = 1;
    passenger->position = SEAT_POSITION_FRONT_PASSENGER;
    passenger->occupied = false;
    passenger->beltBuckled = false;
    passenger->heatingLevel = 0;
    passenger->airbagEnabled = true;
    passenger->timestampUs = driver->timestampUs;
    
    status->timestampUs = driver->timestampUs;
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  Seat Controller Module\n");
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
    g_e2eContext.config.p05.dataId = BODY_E2E_DATAID_SEAT;
    g_e2eContext.config.p05.dataLength = sizeof(SeatSystemStatusType);
    
    dds_domain_participant_t* participant = dds_create_participant(SEAT_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_RELIABLE, DDS_QOS_DURABILITY_VOLATILE, 150, 10, 2 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, SEAT_TOPIC_NAME, SEAT_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[Seat Controller] Running...\n");
    
    SeatSystemStatusType status;
    uint32_t count = 0;
    
    while (g_running) {
        generate_seat_status(&status);
        uint32_t len = sizeof(status);
        E2E_P05_Protect(&g_e2eContext, &status, &len);
        
        if (dds_write(writer, &status, sizeof(status), 50) == ETH_OK) {
            if (++count % 10 == 0) {
                printf("[Seat] Driver occupied: %s, Heating: %d\n",
                       status.seats[0].occupied ? "Yes" : "No",
                       status.seats[0].heatingLevel);
            }
        }
        usleep(1000000 / SEAT_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    return 0;
}
