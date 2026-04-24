/******************************************************************************
 * @file    vcu_publisher.c
 * @brief   Vehicle Control Unit Publisher
 *
 * Publishes vehicle control status including speed, gear, pedal positions
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

#include "powertrain_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define VCU_PUBLISHER_NAME      "VCU_Publisher_01"
#define VCU_TOPIC_NAME          "Powertrain/VCU/Status"
#define VCU_TYPE_NAME           "VcuStatusType"
#define VCU_PUBLISH_RATE_HZ     50
#define VCU_DOMAIN_ID           2

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
    printf("\n[VCU Publisher] Shutting down...\n");
}

/* ============================================================================
 * VCU Simulation Functions
 * ============================================================================ */

/**
 * @brief Generate simulated VCU status
 * @param status Output VCU status
 */
static void generate_vcu_status(VcuStatusType* status)
{
    static float32 simTime = 0.0f;
    static float32 vehicleSpeed = 0.0f;
    static float32 accelPedal = 0.0f;
    
    simTime += 0.02f;  /* 50Hz */
    
    status->vcuId = 0x7001;
    status->state = VCU_STATE_DRIVE;
    status->mode = PT_MODE_DRIVE;
    
    /* Simulate accelerator pedal cycling */
    accelPedal = 0.3f + 0.4f * sinf(simTime * 0.5f);
    if (accelPedal < 0.0f) accelPedal = 0.0f;
    if (accelPedal > 1.0f) accelPedal = 1.0f;
    status->acceleratorPedal = accelPedal;
    
    /* Vehicle speed based on pedal position */
    float32 targetSpeed = accelPedal * 150.0f;  /* Max 150 km/h */
    float32 speedDiff = targetSpeed - vehicleSpeed;
    vehicleSpeed += speedDiff * 0.05f;  /* Smooth acceleration */
    status->vehicleSpeedKph = vehicleSpeed;
    
    /* Braking simulation */
    float32 brakePhase = fmodf(simTime, 20.0f);
    if (brakePhase > 15.0f && brakePhase < 17.0f) {
        status->brakePedal = (brakePhase - 15.0f) / 2.0f;
        vehicleSpeed *= 0.98f;  /* Slow down when braking */
    } else {
        status->brakePedal = 0.0f;
    }
    status->brakeSwitch = (status->brakePedal > 0.01f);
    status->brakeSwitch2 = status->brakeSwitch;
    
    /* Acceleration calculation */
    static float32 prevSpeed = 0.0f;
    status->accelerationMss = (vehicleSpeed - prevSpeed) * 1000.0f / 3600.0f / 0.02f;
    prevSpeed = vehicleSpeed;
    status->decelerationMss = (status->accelerationMss < 0.0f) ? -status->accelerationMss : 0.0f;
    status->lateralAcceleration = 0.5f * sinf(simTime * 0.3f);
    status->yawRateDps = 5.0f * sinf(simTime * 0.2f);
    
    /* Steering */
    status->steeringAngle = 15.0f * sinf(simTime * 0.4f);
    status->steeringRate = 30.0f * cosf(simTime * 0.4f);
    
    /* Gear */
    if (vehicleSpeed < 1.0f) {
        status->gearPosition = GEAR_PARK;
    } else {
        status->gearPosition = GEAR_DRIVE;
    }
    status->requestedGear = status->gearPosition;
    status->gearShifterLocked = false;
    
    /* Driver commands */
    status->requestedTorque = accelPedal * 400.0f;  /* Max 400 Nm */
    status->requestedPower = accelPedal * 150.0f;   /* Max 150 kW */
    status->cruiseControlEnabled = false;
    status->cruiseControlSpeed = 0.0f;
    
    /* Vehicle state */
    static float32 odometer = 50000.0f;
    odometer += vehicleSpeed / 3600.0f * 0.02f;  /* km per 20ms */
    status->odometerKm = odometer;
    status->tripMeterKm = fmodf(odometer, 1000.0f);
    status->ignitionCycles = 1523;
    
    /* Ready state */
    status->readyToDrive = (status->state == VCU_STATE_DRIVE);
    status->highVoltageActive = true;
    status->chargingConnected = false;
    status->chargingActive = false;
    
    /* Faults */
    status->faultCount = 0;
    memset(status->faultCodes, 0, sizeof(status->faultCodes));
    status->health = PT_HEALTH_OK;
    
    status->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

/* ============================================================================
 * Main Function
 * ============================================================================ */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("  VCU Status Publisher\n");
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
    E2E_InitContext(&g_e2eContext, PT_E2E_PROFILE);
    g_e2eContext.config.p07.dataId = PT_E2E_DATAID_VCU;
    g_e2eContext.config.p07.dataLength = sizeof(VcuStatusType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create participant */
    dds_domain_participant_t* participant = dds_create_participant(VCU_DOMAIN_ID);
    if (participant == NULL) {
        printf("[ERROR] Failed to create domain participant\n");
        return 1;
    }
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 30,
        .latency_budget_ms = 5,
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
    dds_topic_t* topic = dds_create_topic(participant, VCU_TOPIC_NAME, VCU_TYPE_NAME, &qos);
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
    
    printf("[VCU Publisher] Initialized successfully\n");
    printf("  Topic: %s\n", VCU_TOPIC_NAME);
    printf("  Domain: %d\n", VCU_DOMAIN_ID);
    printf("  Rate: %d Hz\n", VCU_PUBLISH_RATE_HZ);
    printf("  E2E Profile: P07 (CRC32 + Counter)\n\n");
    printf("Publishing data... Press Ctrl+C to stop\n\n");
    
    /* Main publishing loop */
    VcuStatusType vcuStatus;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Generate VCU status */
        generate_vcu_status(&vcuStatus);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(VcuStatusType);
        E2E_P07_Protect(&g_e2eContext, &vcuStatus, &dataLength);
        
        /* Publish data */
        status = dds_write(writer, &vcuStatus, sizeof(vcuStatus), 10);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 50 == 0) {
                printf("[VCU Publisher] Status: Speed=%.1f km/h, Accel=%.0f%%, Gear=%d\n",
                       vcuStatus.vehicleSpeedKph,
                       vcuStatus.acceleratorPedal * 100.0f,
                       vcuStatus.gearPosition);
            }
        }
        
        /* Sleep for 50Hz (20ms) */
        usleep(1000000 / VCU_PUBLISH_RATE_HZ);
    }
    
    printf("\n[VCU Publisher] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[VCU Publisher] Shutdown complete\n");
    return 0;
}
