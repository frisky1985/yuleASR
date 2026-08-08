/**
 * @file dds_generated_config.c
 * @brief DDS Configuration Implementation for ADAS_Sensor_Fusion
 * @version 2.1.0
 * @date 2026-04-26 15:17:32
 * @author Auto-generated
 *
 * @note This file is auto-generated. Do not modify manually.
 * @copyright Copyright (c) 2026
 */

#include "dds_generated_config.h"

/* ============================================================================ */
/* QoS Profile Definitions                                                      */
/* ============================================================================ */
/** @brief QoS Profile: qos_sensor_reliable */
const dds_qos_t g_qos_sensor_reliable = {
    .reliability = DDS_QOS_RELIABILITY_RELIABLE,
    .durability = DDS_QOS_DURABILITY_TRANSIENT_LOCAL,
    .deadline_ms = 33U,
    .latency_budget_ms = 0U,
    .history_depth = 10U
};
/** @brief QoS Profile: qos_control_realtime */
const dds_qos_t g_qos_control_realtime = {
    .reliability = DDS_QOS_RELIABILITY_RELIABLE,
    .durability = DDS_QOS_DURABILITY_VOLATILE,
    .deadline_ms = 10U,
    .latency_budget_ms = 0U,
    .history_depth = 1U
};

/* ============================================================================ */
/* Domain Participant Instances                                                 */
/* ============================================================================ */
/** @brief Initialize participant g_camera_ecu */
dds_domain_participant_t *g_camera_ecu = NULL;
/** @brief Initialize participant g_lidar_ecu */
dds_domain_participant_t *g_lidar_ecu = NULL;
/** @brief Initialize participant g_fusion_ecu */
dds_domain_participant_t *g_fusion_ecu = NULL;
/** @brief Initialize participant g_control_ecu */
dds_domain_participant_t *g_control_ecu = NULL;

/* ============================================================================ */
/* Default Callback Functions                                                   */
/* ============================================================================ */
/**
 * @brief Default data received callback
 */
static void dds_generated_on_data_received(const void *data, uint32_t size, void *user_data)
{
    /* MISRA C:2012 Deviation - user_data may be unused */
    (void)user_data;
    
    /* TODO: Implement data processing */
    if ((data != NULL) && (size > 0U))
    {
        /* Process received data */
    }
}
/**
 * @brief Default write complete callback
 */
static void dds_generated_on_write_complete(void *user_data)
{
    /* MISRA C:2012 Deviation - user_data may be unused */
    (void)user_data;
    
    /* TODO: Implement write completion handling */
}
/**
 * @brief Default participant connected callback
 */
static void dds_generated_on_participant_connected(dds_domain_participant_t *participant, void *user_data)
{
    /* MISRA C:2012 Deviation - parameters may be unused */
    (void)participant;
    (void)user_data;
    
    /* TODO: Implement connection handling */
}

/* ============================================================================ */
/* Initialization Functions                                                     */
/* ============================================================================ */

/**
 * @brief Initialize all DDS generated configurations
 * @return eth_status_t ETH_OK on success, error code on failure
 */
eth_status_t dds_init_generated_config(void)
{
    eth_status_t status = ETH_OK;
    (void)status; /* 可能未使用 */

    /* Initialize DDS runtime */
    status = dds_runtime_init();
    if (status != ETH_OK)
    {
        return status;
    }

    /* Create Domain Participants */
    g_camera_ecu = dds_create_participant(DDS_DOMAIN_ID_DDS_DOMAIN_PERCEPTION_DOMAIN);
    if (g_camera_ecu == NULL)
    {
        (void)dds_deinit_generated_config();
        return ETH_ERROR;
    }
    g_lidar_ecu = dds_create_participant(DDS_DOMAIN_ID_DDS_DOMAIN_PERCEPTION_DOMAIN);
    if (g_lidar_ecu == NULL)
    {
        (void)dds_deinit_generated_config();
        return ETH_ERROR;
    }
    g_fusion_ecu = dds_create_participant(DDS_DOMAIN_ID_DDS_DOMAIN_PERCEPTION_DOMAIN);
    if (g_fusion_ecu == NULL)
    {
        (void)dds_deinit_generated_config();
        return ETH_ERROR;
    }
    g_control_ecu = dds_create_participant(DDS_DOMAIN_ID_DDS_DOMAIN_CONTROL_DOMAIN);
    if (g_control_ecu == NULL)
    {
        (void)dds_deinit_generated_config();
        return ETH_ERROR;
    }

    /* Configure Participant QoS */
    (void)dds_set_participant_qos(g_camera_ecu, &g_qos_sensor_reliable);
    (void)dds_set_participant_qos(g_lidar_ecu, &g_qos_sensor_reliable);
    (void)dds_set_participant_qos(g_fusion_ecu, &g_qos_sensor_reliable);
    (void)dds_set_participant_qos(g_control_ecu, &g_qos_control_realtime);

    return ETH_OK;
}

/**
 * @brief Deinitialize all DDS generated configurations
 */
void dds_deinit_generated_config(void)
{
    /* Delete Domain Participants in reverse order */
    if (g_control_ecu != NULL)
    {
        (void)dds_delete_participant(g_control_ecu);
        g_control_ecu = NULL;
    }
    if (g_fusion_ecu != NULL)
    {
        (void)dds_delete_participant(g_fusion_ecu);
        g_fusion_ecu = NULL;
    }
    if (g_lidar_ecu != NULL)
    {
        (void)dds_delete_participant(g_lidar_ecu);
        g_lidar_ecu = NULL;
    }
    if (g_camera_ecu != NULL)
    {
        (void)dds_delete_participant(g_camera_ecu);
        g_camera_ecu = NULL;
    }

    /* Deinitialize DDS runtime */
    dds_runtime_deinit();
}
