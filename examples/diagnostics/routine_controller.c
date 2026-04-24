/******************************************************************************
 * @file    routine_controller.c
 * @brief   Diagnostic Routine Controller
 *
 * Publishes routine execution status
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

#define ROUTINE_TOPIC_NAME  "Diagnostics/Routine/Results"
#define ROUTINE_TYPE_NAME   "RoutineResultType"
#define ROUTINE_DOMAIN_ID   5
#define ROUTINE_PUBLISH_HZ  2

static volatile bool g_running = true;
static E2E_ContextType g_e2eContext;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = false;
    printf("\n[Routine Controller] Shutting down...\n");
}

static void generate_routine_data(RoutineResultType* routine)
{
    static uint32_t progress = 0;
    static RoutineStatusType state = ROUTINE_STATUS_NOT_STARTED;
    
    routine->rid = 0xFF01;  /* Test routine */
    strcpy(routine->name, "Memory Self Test");
    routine->status = state;
    
    if (state == ROUTINE_STATUS_NOT_STARTED) {
        state = ROUTINE_STATUS_IN_PROGRESS;
        progress = 0;
    } else if (state == ROUTINE_STATUS_IN_PROGRESS) {
        progress += 10;
        if (progress >= 100) {
            progress = 100;
            state = ROUTINE_STATUS_COMPLETED;
        }
    } else if (state == ROUTINE_STATUS_COMPLETED) {
        /* Reset after a while */
        if (progress > 105) {
            state = ROUTINE_STATUS_NOT_STARTED;
        }
        progress++;
    }
    
    routine->progressPercent = progress;
    routine->estimatedDurationMs = 5000;
    routine->elapsedTimeMs = progress * 50;
    routine->remainingTimeMs = (progress < 100) ? (100 - progress) * 50 : 0;
    
    /* Results */
    if (state == ROUTINE_STATUS_COMPLETED) {
        routine->resultDataSize = 4;
        routine->resultData[0] = 0x00;  /* Pass */
        routine->resultData[1] = 0x00;
        routine->resultData[2] = 0x00;
        routine->resultData[3] = 0x00;
    }
    
    routine->securityLevelRequired = 1;
    routine->securityAccessGranted = true;
    routine->timestampUs = (uint64_t)(clock() * 1000000 / CLOCKS_PER_SEC);
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    
    printf("========================================\n");
    printf("  Routine Controller\n");
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
    g_e2eContext.config.p06.dataId = DIAG_E2E_DATAID_ROUTINE;
    g_e2eContext.config.p06.dataLength = sizeof(RoutineResultType);
    
    dds_domain_participant_t* participant = dds_create_participant(ROUTINE_DOMAIN_ID);
    dds_qos_t qos = { DDS_QOS_RELIABILITY_RELIABLE, DDS_QOS_DURABILITY_TRANSIENT_LOCAL, 600, 50, 5 };
    dds_publisher_t* publisher = dds_create_publisher(participant, &qos);
    dds_topic_t* topic = dds_create_topic(participant, ROUTINE_TOPIC_NAME, ROUTINE_TYPE_NAME, &qos);
    dds_data_writer_t* writer = dds_create_data_writer(publisher, topic, &qos);
    
    printf("[Routine Controller] Running...\n");
    
    RoutineResultType routine;
    uint32_t count = 0;
    
    while (g_running) {
        generate_routine_data(&routine);
        uint32_t len = sizeof(routine);
        E2E_P06_Protect(&g_e2eContext, &routine, &len);
        
        if (dds_write(writer, &routine, sizeof(routine), 200) == ETH_OK) {
            if (++count % 2 == 0) {
                const char* status_str = "Unknown";
                switch (routine.status) {
                    case ROUTINE_STATUS_NOT_STARTED: status_str = "Not Started"; break;
                    case ROUTINE_STATUS_IN_PROGRESS: status_str = "In Progress"; break;
                    case ROUTINE_STATUS_COMPLETED: status_str = "Completed"; break;
                    case ROUTINE_STATUS_ABORTED: status_str = "Aborted"; break;
                    case ROUTINE_STATUS_FAILED: status_str = "Failed"; break;
                }
                printf("[Routine] %s: %s - %d%%\n", routine.name, status_str, routine.progressPercent);
            }
        }
        usleep(1000000 / ROUTINE_PUBLISH_HZ);
    }
    
    dds_delete_data_writer(writer);
    dds_delete_topic(topic);
    dds_delete_publisher(publisher);
    dds_delete_participant(participant);
    E2E_DeinitContext(&g_e2eContext);
    dds_runtime_deinit();
    
    return 0;
}
