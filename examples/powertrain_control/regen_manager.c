/******************************************************************************
 * @file    regen_manager.c
 * @brief   Energy Recovery (Regeneration) Manager
 *
 * Subscribes to BMS and VCU data, manages regenerative braking
 * with E2E protection for ASIL-D safety requirements.
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "powertrain_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */
#define REGEN_MANAGER_NAME      "Regen_Manager_01"
#define REGEN_TOPIC_NAME        "Powertrain/Regen/Status"
#define REGEN_TYPE_NAME         "RegenStatusType"
#define REGEN_PUBLISH_RATE_HZ   50
#define REGEN_DOMAIN_ID         2

#define BMS_TOPIC_NAME          "Powertrain/BMS/Status"
#define VCU_TOPIC_NAME          "Powertrain/VCU/Status"

/* ============================================================================
 * Global Variables
 * ============================================================================ */
static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static BmsStatusType g_bmsData;
static VcuStatusType g_vcuData;
static bool g_bmsDataValid = false;
static bool g_vcuDataValid = false;

static float32 g_totalEnergyRecoveredKwh = 0.0f;
static uint32_t g_regenEvents = 0;

/* ============================================================================
 * Signal Handler
 * ============================================================================ */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Regen Manager] Shutting down...\n");
}

/* ============================================================================
 * Data Callbacks
 * ============================================================================ */

static void on_bms_received(const void* data, uint32_t size, void* user_data)
{
    (void)user_data;
    (void)size;
    
    const BmsStatusType* bms = (const BmsStatusType*)data;
    
    /* Verify E2E protection */
    uint16_t e2eStatus;
    E2E_P07_Check(&g_e2eContext, data, size, &e2eStatus);
    
    if (e2eStatus == E2E_P_OK) {
        memcpy(&g_bmsData, bms, sizeof(BmsStatusType));
        g_bmsDataValid = true;
    }
}

static void on_vcu_received(const void* data, uint32_t size, void* user_data)
{
    (void)user_data;
    (void)size;
    
    const VcuStatusType* vcu = (const VcuStatusType*)data;
    
    /* Verify E2E protection */
    uint16_t e2eStatus;
    E2E_P07_Check(&g_e2eContext, data, size, &e2eStatus);
    
    if (e2eStatus == E2E_P_OK) {
        memcpy(&g_vcuData, vcu, sizeof(VcuStatusType));
        g_vcuDataValid = true;
    }
}

/* ============================================================================
 * Regen Calculation Functions
 * ============================================================================ */

/**
 * @brief Calculate regenerative braking parameters
 * @param regen Output regen status
 */
static void calculate_regen(RegenStatusType* regen)
{
    regen->systemId = 0x8001;
    regen->mode = REGEN_MODE_AUTO;
    
    if (!g_bmsDataValid || !g_vcuDataValid) {
        regen->enabled = false;
        regen->regenCurrent = 0.0f;
        regen->regenPowerKw = 0.0f;
        regen->regenTorqueNm = 0.0f;
        return;
    }
    
    /* Determine if regen should be active */
    bool brakeActive = (g_vcuData.brakePedal > 0.05f);
    bool acceleratorReleased = (g_vcuData.acceleratorPedal < 0.05f);
    bool speedAboveThreshold = (g_vcuData.vehicleSpeedKph > 10.0f);
    
    /* SOC-based limits */
    bool socAllowsRegen = (g_bmsData.socPercent < 95.0f);
    
    /* Temperature limits */
    bool tempAllowsRegen = (g_bmsData.maxTemp < 50.0f);
    
    /* Enable regen if conditions met */
    regen->enabled = (brakeActive || acceleratorReleased) && 
                     speedAboveThreshold && 
                     socAllowsRegen && 
                     tempAllowsRegen;
    
    if (regen->enabled) {
        /* Calculate regen level based on brake pedal or deceleration */
        float32 regenLevel;
        if (brakeActive) {
            regenLevel = g_vcuData.brakePedal;
        } else {
            regenLevel = 0.3f;  /* Coast regen */
        }
        regen->regenLevel = regenLevel;
        
        /* Calculate regen torque */
        regen->maxRegenTorqueNm = 200.0f;  /* Max regen torque */
        regen->regenTorqueNm = regenLevel * regen->maxRegenTorqueNm;
        
        /* Calculate regen power (simplified) */
        regen->maxRegenPowerKw = g_bmsData.maxRegenPowerKw;
        float32 targetPower = regen->regenTorqueNm * g_vcuData.vehicleSpeedKph / 9.5488f / 1000.0f;
        
        /* Limit by BMS max regen */
        if (targetPower > regen->maxRegenPowerKw) {
            targetPower = regen->maxRegenPowerKw;
        }
        
        /* SOC-based derating */
        if (g_bmsData.socPercent > 90.0f) {
            targetPower *= (95.0f - g_bmsData.socPercent) / 5.0f;
        }
        
        regen->regenPowerKw = targetPower;
        
        /* Calculate regen current */
        if (g_bmsData.packVoltage > 0.0f) {
            regen->regenCurrent = (regen->regenPowerKw * 1000.0f) / g_bmsData.packVoltage;
        } else {
            regen->regenCurrent = 0.0f;
        }
        
        /* Calculate efficiency (typically 90-95%) */
        regen->regenEfficiency = 0.92f + ((float32)rand() / RAND_MAX) * 0.03f;
        
        /* Update statistics */
        float32 energyThisCycle = regen->regenPowerKw * regen->regenEfficiency * (1.0f / 3600.0f / 50.0f);
        g_totalEnergyRecoveredKwh += energyThisCycle;
        
        if (regen->regenPowerKw > 1.0f) {
            g_regenEvents++;
        }
    } else {
        regen->regenCurrent = 0.0f;
        regen->regenPowerKw = 0.0f;
        regen->regenTorqueNm = 0.0f;
        regen->regenEfficiency = 0.0f;
    }
    
    /* Configuration */
    regen->regenSpeedThreshold = 10.0f;  /* km/h */
    regen->socRegenLimitStart = 90.0f;
    regen->socRegenLimitEnd = 95.0f;
    regen->batteryTempLimitC = 50.0f;
    regen->motorTempLimitC = 80.0f;
    
    /* Statistics */
    regen->energyRecoveredKwh = g_totalEnergyRecoveredKwh;
    regen->totalDistanceRegenKm = g_totalEnergyRecoveredKwh * 8.0f;  /* Approx 8km per kWh */
    regen->regenEvents = g_regenEvents;
    
    regen->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

/* ============================================================================
 * Main Function
 * ============================================================================ */
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("  Regeneration Manager\n");
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
    g_e2eContext.config.p07.dataId = PT_E2E_DATAID_REGEN;
    g_e2eContext.config.p07.dataLength = sizeof(RegenStatusType);
    g_e2eContext.config.p07.crcOffset = 0;
    g_e2eContext.config.p07.counterOffset = 4;
    
    /* Create participant */
    dds_domain_participant_t* participant = dds_create_participant(REGEN_DOMAIN_ID);
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
    
    /* Create publisher and writer for regen status */
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* regenTopic = dds_create_topic(participant, REGEN_TOPIC_NAME, REGEN_TYPE_NAME, &qos);
    dds_data_writer_t* regenWriter = dds_create_data_writer(publisher, regenTopic, &qos);
    
    /* Create subscriber and readers for input data */
    dds_subscriber_t* subscriber = dds_create_subscriber(participant, &qos);
    
    dds_topic_t* bmsTopic = dds_create_topic(participant, BMS_TOPIC_NAME, "BmsStatusType", &qos);
    dds_data_reader_t* bmsReader = dds_create_data_reader(subscriber, bmsTopic, &qos);
    dds_register_data_callback(bmsReader, on_bms_received, NULL);
    
    dds_topic_t* vcuTopic = dds_create_topic(participant, VCU_TOPIC_NAME, "VcuStatusType", &qos);
    dds_data_reader_t* vcuReader = dds_create_data_reader(subscriber, vcuTopic, &qos);
    dds_register_data_callback(vcuReader, on_vcu_received, NULL);
    
    printf("[Regen Manager] Initialized successfully\n");
    printf("  Subscribed to: %s, %s\n", BMS_TOPIC_NAME, VCU_TOPIC_NAME);
    printf("  Publishing to: %s\n", REGEN_TOPIC_NAME);
    printf("  Domain: %d\n", REGEN_DOMAIN_ID);
    printf("  Rate: %d Hz\n\n", REGEN_PUBLISH_RATE_HZ);
    printf("Managing regeneration... Press Ctrl+C to stop\n\n");
    
    /* Main loop */
    RegenStatusType regenStatus;
    uint32_t publishCount = 0;
    
    while (g_running) {
        /* Process incoming data */
        dds_spin_once(1);
        
        /* Calculate regen parameters */
        calculate_regen(&regenStatus);
        
        /* Apply E2E protection */
        uint32_t dataLength = sizeof(RegenStatusType);
        E2E_P07_Protect(&g_e2eContext, &regenStatus, &dataLength);
        
        /* Publish regen status */
        status = dds_write(regenWriter, &regenStatus, sizeof(regenStatus), 10);
        
        if (status == ETH_OK) {
            publishCount++;
            if (publishCount % 50 == 0) {
                printf("[Regen Manager] Status: %s, Power=%.2f kW, Energy=%.3f kWh\n",
                       regenStatus.enabled ? "ACTIVE" : "INACTIVE",
                       regenStatus.regenPowerKw,
                       regenStatus.energyRecoveredKwh);
            }
        }
        
        usleep(1000000 / REGEN_PUBLISH_RATE_HZ);
    }
    
    printf("\n[Regen Manager] Total frames published: %u\n", publishCount);
    printf("[Regen Manager] Total energy recovered: %.3f kWh\n", g_totalEnergyRecoveredKwh);
    
    /* Cleanup */
    dds_delete_data_reader(bmsReader);
    dds_delete_data_reader(vcuReader);
    dds_delete_data_writer(regenWriter);
    dds_delete_topic(bmsTopic);
    dds_delete_topic(vcuTopic);
    dds_delete_topic(regenTopic);
    dds_delete_subscriber(subscriber);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    printf("[Regen Manager] Shutdown complete\n");
    return 0;
}
