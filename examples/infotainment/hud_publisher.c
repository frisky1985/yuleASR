/******************************************************************************
 * @file    hud_publisher.c
 * @brief   Head-Up Display (HUD) Publisher
 *
 * Publishes HUD overlay data for driver display
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "infotainment_types.h"
#include "../../src/dds/core/dds_core.h"

#define HUD_TOPIC_NAME      "Infotainment/HUD/Display"
#define HUD_TYPE_NAME       "HudDisplayType"
#define HUD_DOMAIN_ID       4
#define HUD_PUBLISH_HZ      30

static volatile bool g_running = true;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[HUD Publisher] Shutting down...\n");
}

static void generate_hud_data(HudDisplayType* hud)
{
    static uint32_t speed = 0;
    static uint32_t count = 0;
    count++;
    
    /* Simulate accelerating */
    if (speed < 120 && count % 10 == 0) speed++;
    
    hud->hudId = 0xD001;
    hud->enabled = true;
    hud->brightnessLevel = 80;
    
    /* Current speed */
    hud->currentSpeedKph = speed;
    hud->speedLimit = 100;
    hud->speedLimitExceeded = (speed > hud->speedLimit);
    
    /* Navigation overlay */
    hud->navActive = true;
    strcpy(hud->navNextTurn, "Main Street");
    hud->navDistanceToTurnM = 500 - (count * 5) % 500;
    hud->navManeuver = NAV_MANEUVER_TURN_RIGHT;
    
    /* ADAS overlay */
    hud->adasActive = true;
    hud->laneDepartureWarning = false;
    hud->collisionWarning = false;
    hud->followingDistanceS = 2.5f;
    
    /* Elements */
    hud->elementCount = 4;
    
    /* Speed element */
    hud->elements[0].elementId = 0;
    hud->elements[0].type = HUD_ELEMENT_SPEED;
    hud->elements[0].priority = HUD_PRIORITY_HIGH;
    hud->elements[0].posX = 0.5f;
    hud->elements[0].posY = 0.3f;
    sprintf(hud->elements[0].text, "%d", speed);
    hud->elements[0].numericValue = speed;
    hud->elements[0].color = 0x00FF00;
    hud->elements[0].visible = true;
    
    /* Navigation element */
    hud->elements[1].elementId = 1;
    hud->elements[1].type = HUD_ELEMENT_NAVIGATION;
    hud->elements[1].priority = HUD_PRIORITY_MEDIUM;
    hud->elements[1].posX = 0.5f;
    hud->elements[1].posY = 0.5f;
    sprintf(hud->elements[1].text, "Turn right in %dm", hud->navDistanceToTurnM);
    hud->elements[1].color = 0xFFFFFF;
    hud->elements[1].visible = true;
    
    /* Lane guidance */
    hud->elements[2].elementId = 2;
    hud->elements[2].type = HUD_ELEMENT_LANE_GUIDANCE;
    hud->elements[2].priority = HUD_PRIORITY_MEDIUM;
    hud->elements[2].posX = 0.5f;
    hud->elements[2].posY = 0.7f;
    strcpy(hud->elements[2].text, "Keep in lane");
    hud->elements[2].color = 0x00FFFF;
    hud->elements[2].visible = true;
    
    /* Warning if speeding */
    hud->elements[3].elementId = 3;
    hud->elements[3].type = HUD_ELEMENT_WARNING;
    hud->elements[3].priority = HUD_PRIORITY_CRITICAL;
    hud->elements[3].posX = 0.5f;
    hud->elements[3].posY = 0.2f;
    strcpy(hud->elements[3].text, "SPEED LIMIT");
    hud->elements[3].color = 0xFF0000;
    hud->elements[3].blink = hud->speedLimitExceeded;
    hud->elements[3].blinkRateMs = 500;
    hud->elements[3].visible = hud->speedLimitExceeded;
    
    hud->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  HUD Display Publisher\n");
    printf("  QM (Non-Safety Critical)\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (dds_runtime_init() != ETH_OK) {
        printf("[ERROR] DDS init failed\n");
        return 1;
    }
    
    dds_domain_participant_t* participant = dds_create_participant(HUD_DOMAIN_ID);
    /* Use BEST_EFFORT for non-critical data */
    dds_qos_t qos = { DDS_QOS_RELIABILITY_BEST_EFFORT, DDS_QOS_DURABILITY_VOLATILE, 50, 5, 1 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, HUD_TOPIC_NAME, HUD_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[HUD Publisher] Running...\n");
    
    HudDisplayType hud;
    uint32_t count = 0;
    
    while (g_running) {
        generate_hud_data(&hud);
        
        if (dds_write(writer, &hud, sizeof(hud), 20) == ETH_OK) {
            if (++count % 30 == 0) {
                printf("[HUD] Speed: %d km/h, Nav: %s in %dm\n",
                       hud.currentSpeedKph, hud.navNextTurn, hud.navDistanceToTurnM);
            }
        }
        usleep(1000000 / HUD_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    dds_runtime_deinit();
    
    return 0;
}
