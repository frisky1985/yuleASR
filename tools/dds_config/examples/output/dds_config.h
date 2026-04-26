/**
 * @file dds_config.h
 * @brief Auto-generated DDS Configuration
 * @version 1.2.0
 * @generated 2026-04-26T15:44:43.458453
 * @warning This file is auto-generated. Do not edit manually.
 */

#ifndef DDS_CONFIG_H
#define DDS_CONFIG_H

#include "dds/core/dds_core.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * System Configuration
 * ============================================================================ */

#define DDS_SYSTEM_NAME "ADAS_Sensor_Fusion"
#define DDS_SYSTEM_VERSION "1.2.0"

/* ============================================================================
 * Domain Configuration
 * ============================================================================ */

#define DDS_NUM_DOMAINS 2

/* Domain IDs */
#define DDS_DOMAIN_ID_PERCEPTIONDOMAIN 1U
#define DDS_DOMAIN_ID_CONTROLDOMAIN 2U

/* ============================================================================
 * DomainParticipant Configuration
 * ============================================================================ */

#define DDS_NUM_PARTICIPANTS 3

/* Participant: CameraECU (Domain 1) */
extern dds_domain_participant_t *g_participant_CAMERAECU;
/* Participant: RadarECU (Domain 1) */
extern dds_domain_participant_t *g_participant_RADARECU;
/* Participant: PlanningECU (Domain 2) */
extern dds_domain_participant_t *g_participant_PLANNINGECU;

/* ============================================================================
 * Topic Configuration
 * ============================================================================ */

#define DDS_NUM_TOPICS 5

/* Topic: CameraLaneDetection (Type: LaneData) */
#define DDS_TOPIC_NAME_CAMERALANEDETECTION "CameraLaneDetection"
extern dds_topic_t *g_topic_cameralanedetection;

/* Topic: CameraObjectDetection (Type: ObjectDetectionData) */
#define DDS_TOPIC_NAME_CAMERAOBJECTDETECTION "CameraObjectDetection"
extern dds_topic_t *g_topic_cameraobjectdetection;

/* Topic: FusionResult (Type: FusionData) */
#define DDS_TOPIC_NAME_FUSIONRESULT "FusionResult"
extern dds_topic_t *g_topic_fusionresult;

/* Topic: RadarDetection (Type: RadarData) */
#define DDS_TOPIC_NAME_RADARDETECTION "RadarDetection"
extern dds_topic_t *g_topic_radardetection;

/* Topic: VehicleCommand (Type: CommandData) */
#define DDS_TOPIC_NAME_VEHICLECOMMAND "VehicleCommand"
extern dds_topic_t *g_topic_vehiclecommand;


/* ============================================================================
 * QoS Profile Declarations
 * ============================================================================ */

extern const dds_qos_t g_qos_profile_default;
extern const dds_qos_t g_qos_profile_reliable;
extern const dds_qos_t g_qos_profile_sensor_high_reliability;
extern const dds_qos_t g_qos_profile_control_realtime;

/* ============================================================================
 * Function Declarations
 * ============================================================================ */

/**
 * @brief Initialize DDS configuration
 * @return ETH_OK on success
 */
eth_status_t dds_config_init(void);

/**
 * @brief Deinitialize DDS configuration
 */
void dds_config_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CONFIG_H */
