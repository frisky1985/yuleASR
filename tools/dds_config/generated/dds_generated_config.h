/**
 * @file dds_generated_config.h
 * @brief DDS Configuration for ADAS_Sensor_Fusion
 * @version 2.1.0
 * @date 2026-04-26 15:17:32
 * @author Auto-generated
 *
 * @note This file is auto-generated. Do not modify manually.
 * @copyright Copyright (c) 2026
 */

#ifndef DDS_GENERATED_CONFIG_H
#define DDS_GENERATED_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "dds/core/dds_core.h"
#include "common/types/eth_types.h"

/* Generated for ADAS_Sensor_Fusion v2.1.0 */
#define DDS_CONFIG_GENERATED_VERSION "2.1.0"

/* ============================================================================ */
/* Domain ID Definitions                                                        */
/* ============================================================================ */
/** @brief Domain ID for DDS_DOMAIN_PERCEPTION_DOMAIN */
#define DDS_DOMAIN_ID_DDS_DOMAIN_PERCEPTION_DOMAIN (1U)
/** @brief Domain ID for DDS_DOMAIN_CONTROL_DOMAIN */
#define DDS_DOMAIN_ID_DDS_DOMAIN_CONTROL_DOMAIN (2U)

/* ============================================================================ */
/* Topic Name Definitions                                                       */
/* ============================================================================ */
/** @brief Topic: g_camera_object_detection (ObjectDetectionData) */
#define DDS_TOPIC_G_CAMERA_OBJECT_DETECTION "g_camera_object_detection"
/** @brief Topic: g_lidar_point_cloud (PointCloudData) */
#define DDS_TOPIC_G_LIDAR_POINT_CLOUD "g_lidar_point_cloud"
/** @brief Topic: g_vehicle_control_command (ControlCommandData) */
#define DDS_TOPIC_G_VEHICLE_CONTROL_COMMAND "g_vehicle_control_command"

/* ============================================================================ */
/* QoS Profile Declarations                                                     */
/* ============================================================================ */
/** @brief QoS Profile: qos_sensor_reliable */
extern const dds_qos_t g_qos_sensor_reliable;
/** @brief QoS Profile: qos_control_realtime */
extern const dds_qos_t g_qos_control_realtime;

/* ============================================================================ */
/* Domain Participant Declarations                                              */
/* ============================================================================ */
/** @brief Domain Participant: g_camera_ecu (Domain 1) */
extern dds_domain_participant_t* g_camera_ecu;
/** @brief Domain Participant: g_lidar_ecu (Domain 1) */
extern dds_domain_participant_t* g_lidar_ecu;
/** @brief Domain Participant: g_fusion_ecu (Domain 1) */
extern dds_domain_participant_t* g_fusion_ecu;
/** @brief Domain Participant: g_control_ecu (Domain 2) */
extern dds_domain_participant_t* g_control_ecu;

/* ============================================================================ */
/* Initialization Functions                                                     */
/* ============================================================================ */

/**
 * @brief Initialize all DDS generated configurations
 * @return eth_status_t ETH_OK on success, error code on failure
 */
eth_status_t dds_init_generated_config(void);

/**
 * @brief Deinitialize all DDS generated configurations
 */
void dds_deinit_generated_config(void);

#endif /* DDS_GENERATED_CONFIG_H */
