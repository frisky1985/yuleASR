/**
 * @file dds_qos_config.c
 * @brief Auto-generated QoS Configuration
 * @version 1.2.0
 * @generated 2026-04-26T15:44:43.458552
 */

#include "dds_config.h"
#include <string.h>

/* ============================================================================
 * QoS Profiles
 * ============================================================================ */

/* QoS Profile: default */
const dds_qos_t g_qos_profile_default = {
    .reliability = DDS_BEST_EFFORT,
    .history_kind = DDS_KEEP_LAST,
    .history_depth = 1,
};

/* QoS Profile: reliable */
const dds_qos_t g_qos_profile_reliable = {
    .reliability = DDS_RELIABLE,
    .durability = DDS_TRANSIENT_LOCAL,
    .history_kind = DDS_KEEP_LAST,
    .history_depth = 10,
};

/* QoS Profile: sensor_high_reliability */
const dds_qos_t g_qos_profile_sensor_high_reliability = {
    .reliability = DDS_RELIABLE,
    .durability = DDS_VOLATILE,
    .history_kind = DDS_KEEP_LAST,
    .history_depth = 5,
};

/* QoS Profile: control_realtime */
const dds_qos_t g_qos_profile_control_realtime = {
    .reliability = DDS_RELIABLE,
    .history_kind = DDS_KEEP_LAST,
    .history_depth = 1,
};

