/******************************************************************************
 * @file    navigation_publisher.c
 * @brief   Navigation System Publisher
 *
 * Publishes navigation and route guidance data
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include "infotainment_types.h"
#include "../../src/dds/core/dds_core.h"

#define NAV_TOPIC_NAME      "Infotainment/Navigation/Status"
#define NAV_TYPE_NAME       "NavigationStatusType"
#define NAV_DOMAIN_ID       4
#define NAV_PUBLISH_HZ      5

static volatile bool g_running = true;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Navigation] Shutting down...\n");
}

static void generate_navigation_data(NavigationStatusType* nav)
{
    static uint32_t simTime = 0;
    simTime++;
    
    nav->guidanceId = 0xD002;
    nav->status = NAV_STATUS_NAVIGATING;
    
    /* Current position - simulate movement */
    nav->currentPosition.latitude = 37.7749f + sinf(simTime * 0.01f) * 0.001f;
    nav->currentPosition.longitude = -122.4194f + cosf(simTime * 0.01f) * 0.001f;
    nav->currentPosition.altitude = 50.0f;
    nav->currentPosition.heading = 45.0f + simTime * 0.5f;
    nav->currentPosition.speedKph = 60.0f + 10.0f * sinf(simTime * 0.1f);
    nav->currentPosition.accuracyM = 5;
    
    /* Destination */
    nav->destination.latitude = 37.8044f;
    nav->destination.longitude = -122.2712f;
    strcpy(nav->destinationName, "Oakland Downtown");
    
    /* Route info */
    nav->totalDistanceM = 15000;
    nav->totalTimeSec = 1200;
    nav->remainingDistanceM = (nav->totalDistanceM > simTime * 10) ? 
                               nav->totalDistanceM - simTime * 10 : 0;
    nav->remainingTimeSec = nav->remainingDistanceM / 15;
    nav->arrivalTimeEpoch = 1640000000 + nav->remainingTimeSec;
    
    /* Current instruction */
    nav->nextManeuver = (simTime % 100 < 50) ? NAV_MANEUVER_TURN_RIGHT : NAV_MANEUVER_STRAIGHT;
    nav->distanceToManeuverM = 500 - (simTime % 50) * 10;
    strcpy(nav->nextStreetName, "Broadway Street");
    
    /* Lane guidance */
    nav->currentLane = 2;
    nav->recommendedLane = (nav->distanceToManeuverM < 200) ? 1 : 2;
    nav->laneGuidanceActive = (nav->distanceToManeuverM < 300);
    
    /* Traffic */
    nav->trafficCondition = TRAFFIC_CONDITION_MODERATE;
    nav->delaySec = 180;
    nav->trafficOnRoute = true;
    nav->alternateRouteAvailable = true;
    
    /* Waypoints */
    nav->waypointCount = 3;
    for (int i = 0; i < 3; i++) {
        nav->waypoints[i].waypointId = i;
        nav->waypoints[i].position.latitude = nav->currentPosition.latitude + i * 0.001f;
        nav->waypoints[i].position.longitude = nav->currentPosition.longitude + i * 0.001f;
        sprintf(nav->waypoints[i].streetName, "Waypoint %d", i);
        nav->waypoints[i].distanceToNextM = 5000 - i * 1000;
        nav->waypoints[i].timeToNextSec = 400 - i * 100;
        nav->waypoints[i].maneuver = (i % 2 == 0) ? NAV_MANEUVER_TURN_RIGHT : NAV_MANEUVER_TURN_LEFT;
    }
    
    /* Road info */
    nav->currentRoadSpeedLimit = 60;
    nav->speedLimitWarning = (nav->currentPosition.speedKph > nav->currentRoadSpeedLimit);
    nav->approachingSchoolZone = false;
    
    nav->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  Navigation Publisher\n");
    printf("  QM (Non-Safety Critical)\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (dds_runtime_init() != ETH_OK) {
        printf("[ERROR] DDS init failed\n");
        return 1;
    }
    
    dds_domain_participant_t* participant = dds_create_participant(NAV_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_BEST_EFFORT, DDS_QOS_DURABILITY_VOLATILE, 250, 20, 2 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, NAV_TOPIC_NAME, NAV_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[Navigation] Running...\n");
    
    NavigationStatusType nav;
    uint32_t count = 0;
    
    while (g_running) {
        generate_navigation_data(&nav);
        
        if (dds_write(writer, &nav, sizeof(nav), 100) == ETH_OK) {
            if (++count % 5 == 0) {
                printf("[Nav] To: %s, Remaining: %dm, ETA: %ds\n",
                       nav.destinationName, nav.remainingDistanceM, nav.remainingTimeSec);
            }
        }
        usleep(1000000 / NAV_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    dds_runtime_deinit();
    
    return 0;
}
