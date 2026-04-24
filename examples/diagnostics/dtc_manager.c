/******************************************************************************
 * @file    dtc_manager.c
 * @brief   Diagnostic Trouble Code (DTC) Manager
 *
 * Publishes DTC status and snapshot records
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "diagnostics_types.h"
#include "../../src/dds/core/dds_core.h"
#include "../../src/autosar/e2e/e2e_protection.h"

#define DTC_TOPIC_NAME      "Diagnostics/DTC/Status"
#define DTC_TYPE_NAME       "DtcStatusType"
#define DTC_DOMAIN_ID       5
#define DTC_PUBLISH_HZ      1

static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[DTC Manager] Shutting down...\n");
}

static void generate_dtc_data(DtcStatusType* dtc)
{
    static uint32_t cycle = 0;
    cycle++;
    
    dtc->ecuId = 0xE001;
    dtc->lastClearTime = 1640000000;
    dtc->operationCycleCount = cycle;
    
    /* Simulate some DTCs */
    dtc->dtcCount = 3;
    dtc->confirmedDtcCount = 2;
    dtc->pendingDtcCount = 1;
    
    /* DTC 1: P0301 - Cylinder 1 Misfire */
    dtc->dtcs[0].dtcNumber = 0x0301;
    dtc->dtcs[0].severity = DTC_SEVERITY_CHECK_AT_NEXT_HALT;
    dtc->dtcs[0].statusByte = DTC_STATUS_CONFIRMED_DTC | DTC_STATUS_TEST_FAILED;
    dtc->dtcs[0].origin = DTC_ORIGIN_PRIMARY_MEMORY;
    dtc->dtcs[0].occurrenceCounter = 5;
    dtc->dtcs[0].faultDetectionCounter = 127;
    dtc->dtcs[0].snapshotAvailable = true;
    dtc->dtcs[0].snapshotRecordNumber = 1;
    dtc->dtcs[0].extendedDataAvailable = true;
    dtc->dtcs[0].extendedDataRecordNumber = 1;
    
    /* DTC 2: P0420 - Catalyst Efficiency */
    dtc->dtcs[1].dtcNumber = 0x0420;
    dtc->dtcs[1].severity = DTC_SEVERITY_CHECK_IMMEDIATELY;
    dtc->dtcs[1].statusByte = DTC_STATUS_CONFIRMED_DTC;
    dtc->dtcs[1].origin = DTC_ORIGIN_PRIMARY_MEMORY;
    dtc->dtcs[1].occurrenceCounter = 12;
    dtc->dtcs[1].faultDetectionCounter = 127;
    dtc->dtcs[1].snapshotAvailable = true;
    dtc->dtcs[1].extendedDataAvailable = true;
    
    /* DTC 3: P0455 - EVAP Large Leak */
    dtc->dtcs[2].dtcNumber = 0x0455;
    dtc->dtcs[2].severity = DTC_SEVERITY_CHECK_AT_NEXT_HALT;
    dtc->dtcs[2].statusByte = DTC_STATUS_PENDING_DTC;
    dtc->dtcs[2].origin = DTC_ORIGIN_PRIMARY_MEMORY;
    dtc->dtcs[2].occurrenceCounter = 2;
    dtc->dtcs[2].faultDetectionCounter = 50;
    dtc->dtcs[2].snapshotAvailable = false;
    dtc->dtcs[2].extendedDataAvailable = false;
    
    dtc->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  DTC Manager\n");
    printf("  ASIL-B Safety Level\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (dds_runtime_init() != ETH_OK) {
        printf("[ERROR] DDS init failed\n");
        return 1;
    }
    
    E2E_Init();
    E2E_InitContext(&g_e2eContext, DIAG_E2E_PROFILE);
    g_e2eContext.config.p06.dataId = DIAG_E2E_DATAID_DTC;
    g_e2eContext.config.p06.dataLength = sizeof(DtcStatusType);
    
    dds_domain_participant_t* participant = dds_create_participant(DTC_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_RELIABLE, DDS_QOS_DURABILITY_TRANSIENT_LOCAL, 2000, 50, 10 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, DTC_TOPIC_NAME, DTC_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[DTC Manager] Running...\n");
    
    DtcStatusType dtc;
    uint32_t count = 0;
    
    while (g_running) {
        generate_dtc_data(&dtc);
        uint32_t len = sizeof(dtc);
        E2E_P06_Protect(&g_e2eContext, &dtc, &len);
        
        if (dds_write(writer, &dtc, sizeof(dtc), 500) == ETH_OK) {
            if (++count % 5 == 0) {
                printf("[DTC] Total: %d, Confirmed: %d, Pending: %d (Cycle: %d)\n",
                       dtc.dtcCount, dtc.confirmedDtcCount, dtc.pendingDtcCount, dtc.operationCycleCount);
            }
        }
        usleep(1000000 / DTC_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    return 0;
}
