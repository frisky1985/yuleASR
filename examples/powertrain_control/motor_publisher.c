/******************************************************************************
 * @file    motor_publisher.c
 * @brief   Motor Status Publisher
 *
 * Publishes motor control status including speed, torque, temperature
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
#define MOTOR_PUBLISHER_NAME    "Motor_Publisher_01"
#define MOTOR_TOPIC_NAME        "Powertrain/Motor/Status"
#define MOTOR_TYPE_NAME         "MotorSystemStatusType"
#define MOTOR_PUBLISH_RATE_HZ   100
#define MOTOR_DOMAIN_ID         2

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
    printf("\n[Motor Publisher] Shutting down...\n");
}

/* ============================================================================
 * Motor Simulation Functions
 * ============================================================================ */

/**
 * @brief Generate simulated motor status
 * @param status Output motor system status
 */
static void generate_motor_status(MotorSystemStatusType* status)
{
    static float32 simTime = 0.0f;
    static uint32_t sequence = 0;
    
    simTime += 0.01f;  /* 100Hz */
    
    status->systemId = 0x5001;
    status->motorCount = 2;  /* Dual motor setup */
    
    /* Motor 1 - Front */
    MotorStatusType* m1 = &status->motors[0];
    m1->motorId = 0x5101;
    m1->state = MOTOR_STATE_RUNNING;
    m1->type = MOTOR_TYPE_PMSM;
    
    /* Simulate varying speed */
    float32 targetSpeed = 3000.0f + 500.0f * sinf(simTime * 0.5f);
    m1->speedRpm = targetSpeed + ((float32)rand() / RAND_MAX - 0.5f) * 10.0f;
    
    /* Torque based on acceleration simulation */
    m1->torqueNm = 150.0f + 50.0f * sinf(simTime * 0.3f);
    m1->torqueCommand = m1->torqueNm;
    
    /* Electrical parameters */
    m1->dcLinkVoltage = 400.0f + ((float32)rand() / RAND_MAX - 0.5f) * 2.0f;
    m1->phaseCurrentA = m1->torqueNm * 2.0f + ((float32)rand() / RAND_MAX - 0.5f);
    m1->phaseCurrentB = m1->torqueNm * 2.0f + ((float32)rand() / RAND_MAX - 0.5f);
    m1->phaseCurrentC = -(m1->phaseCurrentA + m1->phaseCurrentB);
    m1->busCurrent = fabsf(m1->torqueNm * 1.5f);
    
    /* Temperature */
    m1->statorTempC = 65.0f + 10.0f * sinf(simTime * 0.1f) + ((float32)rand() / RAND_MAX);
    m1->inverterTempC = 55.0f + 8.0f * sinf(simTime * 0.15f) + ((float32)rand() / RAND_MAX);
    m1->rotorTempC = m1->statorTempC + 5.0f;
    
    /* Control */
    m1->pwmDutyCycle = (uint16_t)(800 + 100 * sinf(simTime));
    m1->electricalAngle = fmodf(simTime * 377.0f, 2.0f * M_PI);  /* 60Hz electrical */
    m1->modulationIndex = 0.85f;
    
    /* Status */
    m1->faultCode = 0;
    m1->warningFlags = 0;
    m1->activeDischargeEnabled = false;
    m1->prechargeComplete = true;
    m1->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
    
    /* Motor 2 - Rear */
    MotorStatusType* m2 = &status->motors[1];
    memcpy(m2, m1, sizeof(MotorStatusType));
    m2->motorId = 0x5102;
    m2->speedRpm = m1->speedRpm * 0.98f;  /* Slight difference */
    m2->torqueNm = m1->torqueNm * 1.1f;   /* More torque to rear

    /* Combined status */
    status->totalPowerKw = (m1->dcLinkVoltage * m1->busCurrent + 
                           m2->dcLinkVoltage * m2->busCurrent) / 1000.0f;
    status->totalTorqueNm = m1->torqueNm + m2->torqueNm;
    status->mechanicalPowerKw = status->totalTorqueNm * m1->speedRpm * 2.0f * M_PI / 60000.0f / 1000.0f;
    status->electricalPowerKw = status->totalPowerKw;
    status->efficiency = (status->mechanicalPowerKw > 0.1f) ? 
                         (status->mechanicalPowerKw / status->electricalPowerKw) : 0.95f;
    
    /* Thermal management */
    status->coolantTempInC = 45.0f + 5.0f * sinf(simTime * 0.2f);
    status->coolantTempOutC = status->coolantTempInC + 8.0f;
    status->coolantFlowLpm = 15.0f + 2.0f * sinf(simTime * 0.3f);
    status->coolingPumpSpeed = (uint8_t)(60 + 20 * sinf(simTime * 0.4f));
    
    /* Health */
    status->health = PT_HEALTH_OK;
    status->timestampUs = m1->timestampUs;
    
    sequence++;
}

/* ============================================================================
 * Main Function
 * ============================================================================ */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("  Motor Status Publisher\n");
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
    g_e2eContext.config.p07.dataId = PT_E2E_DATAID_MOTOR;
    g_e2eContext.config.p07.dataLength = sizeof(MotorSystemStatusType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create participant */
    dds_domain_participant_t* participant = dds_create_participant(MOTOR_DOMAIN_ID);
    if (participant == NULL) {
        printf("[ERROR] Failed to create domain participant\n");
        return 1;
    }
    
    /* Configure QoS for high-frequency, critical data */
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 15,
        .latency_budget_ms = 2,
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
    dds_topic_t* topic = dds_create_topic(participant, MOTOR_TOPIC_NAME, MOTOR_TYPE_NAME, &qos);
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
    
    printf("[Motor Publisher] Initialized successfully\n");
    printf("  Topic: %s\n", MOTOR_TOPIC_NAME);
    printf("  Domain: %d\n", MOTOR_DOMAIN_ID);
    printf("  Rate: %d Hz\n", MOTOR_PUBLISH_RATE_HZ);
    printf("  E2E Profile: P07 (CRC32 + Counter)\n\n");
    printf("Publishing data... Press Ctrl+C to stop\n\n");
    
    /* Main publishing loop */
    MotorSystemStatusType motorStatus;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Generate motor status */
        generate_motor_status(&motorStatus);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(MotorSystemStatusType);
        E2E_P07_Protect(&g_e2eContext, &motorStatus, &dataLength);
        
        /* Publish data */
        status = dds_write(writer, &motorStatus, sizeof(motorStatus), 5);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 100 == 0) {
                printf("[Motor Publisher] Status: Speed=%.0f RPM, Torque=%.1f Nm, Power=%.2f kW\n",
                       motorStatus.motors[0].speedRpm,
                       motorStatus.totalTorqueNm,
                       motorStatus.totalPowerKw);
            }
        }
        
        /* Sleep for 100Hz (10ms) */
        usleep(1000000 / MOTOR_PUBLISH_RATE_HZ);
    }
    
    printf("\n[Motor Publisher] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[Motor Publisher] Shutdown complete\n");
    return 0;
}
