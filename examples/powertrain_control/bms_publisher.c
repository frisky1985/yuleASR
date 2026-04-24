/******************************************************************************
 * @file    bms_publisher.c
 * @brief   Battery Management System Publisher
 *
 * Publishes battery status including SOC, voltage, temperature
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
#define BMS_PUBLISHER_NAME      "BMS_Publisher_01"
#define BMS_TOPIC_NAME          "Powertrain/BMS/Status"
#define BMS_TYPE_NAME           "BmsStatusType"
#define BMS_PUBLISH_RATE_HZ     10
#define BMS_DOMAIN_ID           2

#define BMS_CELL_COUNT          96
#define BMS_NOMINAL_VOLTAGE     400.0f
#define BMS_CAPACITY_AH         75.0f

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
    printf("\n[BMS Publisher] Shutting down...\n");
}

/* ============================================================================
 * BMS Simulation Functions
 * ============================================================================ */

/**
 * @brief Generate simulated BMS status
 * @param status Output BMS status
 */
static void generate_bms_status(BmsStatusType* status)
{
    static float32 simTime = 0.0f;
    static float32 soc = 85.0f;
    
    simTime += 0.1f;  /* 10Hz */
    
    status->packId = 0x6001;
    status->state = BMS_STATE_DRIVE;
    
    /* Simulate varying load */
    float32 loadCurrent = 50.0f + 30.0f * sinf(simTime * 0.2f);
    status->packCurrent = loadCurrent;
    
    /* SOC slowly decreasing with some noise */
    soc -= (loadCurrent / 10000.0f) + ((float32)rand() / RAND_MAX - 0.5f) * 0.001f;
    if (soc < 10.0f) soc = 10.0f;
    if (soc > 100.0f) soc = 100.0f;
    status->socPercent = soc;
    
    /* SOH degrading slowly */
    status->sohPercent = 98.5f - (simTime / 100000.0f);
    
    /* Cell voltages with some imbalance */
    float32 baseVoltage = 4.15f;
    status->minCellVoltage = 999.0f;
    status->maxCellVoltage = 0.0f;
    status->avgCellVoltage = 0.0f;
    
    for (uint16_t i = 0; i < BMS_CELL_COUNT; i++) {
        BatteryCellType* cell = &status->cells[i];
        cell->cellId = i;
        
        /* Voltage with slight variations and SOC curve */
        float32 socFactor = (soc / 100.0f) * 0.8f + 3.0f;  /* 3.0V to 4.2V range */
        cell->voltage = socFactor + ((float32)(i % 10) / 1000.0f) + 
                       ((float32)rand() / RAND_MAX - 0.5f) * 0.005f;
        
        /* Temperature varies by position in pack */
        cell->temperature = 25.0f + 15.0f * sinf(simTime * 0.1f + i * 0.01f) + 
                           ((float32)rand() / RAND_MAX - 0.5f) * 2.0f;
        
        cell->internalResistance = 0.001f + ((float32)rand() / RAND_MAX) * 0.0005f;
        cell->status = CELL_STATUS_OK;
        cell->balanceStatus = (uint8_t)(cell->voltage * 100);
        cell->balancingActive = (cell->voltage > baseVoltage + 0.02f);
        
        /* Track min/max */
        if (cell->voltage < status->minCellVoltage) {
            status->minCellVoltage = cell->voltage;
            status->minCellId = i;
        }
        if (cell->voltage > status->maxCellVoltage) {
            status->maxCellVoltage = cell->voltage;
            status->maxCellId = i;
        }
        status->avgCellVoltage += cell->voltage;
    }
    
    status->avgCellVoltage /= BMS_CELL_COUNT;
    status->voltageImbalance = status->maxCellVoltage - status->minCellVoltage;
    status->cellCount = BMS_CELL_COUNT;
    
    /* Pack voltage (series sum) */
    status->packVoltage = BMS_NOMINAL_VOLTAGE + 
                         (status->avgCellVoltage - 4.15f) * BMS_CELL_COUNT;
    
    /* Current limits based on temperature and SOC */
    status->chargeCurrentLimit = 150.0f;
    status->dischargeCurrentLimit = 300.0f;
    if (soc > 95.0f) {
        status->chargeCurrentLimit = 50.0f;
    }
    if (status->maxTemp > 45.0f) {
        status->dischargeCurrentLimit = 200.0f;
    }
    
    /* Temperature statistics */
    status->minTemp = 999.0f;
    status->maxTemp = -999.0f;
    status->avgTemp = 0.0f;
    
    for (uint16_t i = 0; i < BMS_CELL_COUNT; i++) {
        float32 temp = status->cells[i].temperature;
        if (temp < status->minTemp) {
            status->minTemp = temp;
            status->minTempSensorId = i;
        }
        if (temp > status->maxTemp) {
            status->maxTemp = temp;
            status->maxTempSensorId = i;
        }
        status->avgTemp += temp;
    }
    status->avgTemp /= BMS_CELL_COUNT;
    
    /* Energy calculations */
    status->availableEnergyKwh = BMS_CAPACITY_AH * status->packVoltage / 1000.0f * (soc / 100.0f);
    status->remainingCapacityAh = BMS_CAPACITY_AH * (soc / 100.0f);
    
    /* Power */
    status->instantaneousPowerKw = status->packVoltage * status->packCurrent / 1000.0f;
    status->maxDischargePowerKw = status->dischargeCurrentLimit * status->packVoltage / 1000.0f;
    status->maxRegenPowerKw = 100.0f;  /* 100kW regen limit */
    
    /* Contactors */
    status->mainContactorPositive = true;
    status->mainContactorNegative = true;
    status->prechargeContactor = false;
    status->chargeContactor = false;
    
    /* Isolation (always good in simulation) */
    status->isolationResistanceKohm = 500.0f + ((float32)rand() / RAND_MAX) * 100.0f;
    status->isolationFault = (status->isolationResistanceKohm < 100.0f);
    
    /* Configuration */
    status->modulesInSeries = 12;
    status->cellsInParallel = 2;
    
    /* Health */
    status->cycleCount = (uint32_t)(simTime / 100.0f);
    status->faultCount = 0;
    status->warningFlags = 0;
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
    printf("  BMS Status Publisher\n");
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
    g_e2eContext.config.p07.dataId = PT_E2E_DATAID_BMS;
    g_e2eContext.config.p07.dataLength = sizeof(BmsStatusType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create participant */
    dds_domain_participant_t* participant = dds_create_participant(BMS_DOMAIN_ID);
    if (participant == NULL) {
        printf("[ERROR] Failed to create domain participant\n");
        return 1;
    }
    
    dds_qos_t qos = {
        .reliability = DDS_QOS_RELIABILITY_RELIABLE,
        .durability = DDS_QOS_DURABILITY_VOLATILE,
        .deadline_ms = 150,
        .latency_budget_ms = 10,
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
    dds_topic_t* topic = dds_create_topic(participant, BMS_TOPIC_NAME, BMS_TYPE_NAME, &qos);
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
    
    printf("[BMS Publisher] Initialized successfully\n");
    printf("  Topic: %s\n", BMS_TOPIC_NAME);
    printf("  Domain: %d\n", BMS_DOMAIN_ID);
    printf("  Rate: %d Hz\n", BMS_PUBLISH_RATE_HZ);
    printf("  E2E Profile: P07 (CRC32 + Counter)\n\n");
    printf("Publishing data... Press Ctrl+C to stop\n\n");
    
    /* Main publishing loop */
    BmsStatusType bmsStatus;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Generate BMS status */
        generate_bms_status(&bmsStatus);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(BmsStatusType);
        E2E_P07_Protect(&g_e2eContext, &bmsStatus, &dataLength);
        
        /* Publish data */
        status = dds_write(writer, &bmsStatus, sizeof(bmsStatus), 50);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 10 == 0) {
                printf("[BMS Publisher] Status: SOC=%.1f%%, Voltage=%.1fV, Current=%.1fA, Temp=%.1fC\n",
                       bmsStatus.socPercent,
                       bmsStatus.packVoltage,
                       bmsStatus.packCurrent,
                       bmsStatus.avgTemp);
            }
        }
        
        /* Sleep for 10Hz (100ms) */
        usleep(1000000 / BMS_PUBLISH_RATE_HZ);
    }
    
    printf("\n[BMS Publisher] Total frames published: %u\n", publishCount);
    
    /* Cleanup */
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[BMS Publisher] Shutdown complete\n");
    return 0;
}
