/**
 * @file dds_config.c
 * @brief Auto-generated DDS Configuration Implementation
 * @version 1.2.0
 * @generated 2026-04-26T15:44:43.458499
 */

#include "dds_config.h"
#include <string.h>

/* ============================================================================
 * DomainParticipant Instances
 * ============================================================================ */

dds_domain_participant_t *g_participant_CAMERAECU = NULL;
dds_domain_participant_t *g_participant_RADARECU = NULL;
dds_domain_participant_t *g_participant_PLANNINGECU = NULL;

/* ============================================================================
 * Topic Instances
 * ============================================================================ */

dds_topic_t *g_topic_cameralanedetection = NULL;
dds_topic_t *g_topic_cameraobjectdetection = NULL;
dds_topic_t *g_topic_fusionresult = NULL;
dds_topic_t *g_topic_radardetection = NULL;
dds_topic_t *g_topic_vehiclecommand = NULL;

/* ============================================================================
 * Configuration Initialization
 * ============================================================================ */

eth_status_t dds_config_init(void)
{
    eth_status_t status;
    
    /* Initialize runtime */
    status = dds_runtime_init();
    if (status != ETH_OK) {
        return status;
    }
    
    /* Create participant: CameraECU */
    g_participant_CAMERAECU = dds_create_participant(1);
    if (g_participant_CAMERAECU == NULL) {
        return ETH_ERROR;
    }

    /* Create participant: RadarECU */
    g_participant_RADARECU = dds_create_participant(1);
    if (g_participant_RADARECU == NULL) {
        return ETH_ERROR;
    }

    /* Create participant: PlanningECU */
    g_participant_PLANNINGECU = dds_create_participant(2);
    if (g_participant_PLANNINGECU == NULL) {
        return ETH_ERROR;
    }

    return ETH_OK;
}

void dds_config_deinit(void)
{
    if (g_participant_CAMERAECU) {
        dds_delete_participant(g_participant_CAMERAECU);
        g_participant_CAMERAECU = NULL;
    }
    if (g_participant_RADARECU) {
        dds_delete_participant(g_participant_RADARECU);
        g_participant_RADARECU = NULL;
    }
    if (g_participant_PLANNINGECU) {
        dds_delete_participant(g_participant_PLANNINGECU);
        g_participant_PLANNINGECU = NULL;
    }

    dds_runtime_deinit();
}
