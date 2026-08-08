/*
 * DDS Configuration Tool - Automotive Specific Configuration
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 * 
 * Automotive-grade DDS configuration for AUTOSAR, TSN, and safety-critical systems
 */

#ifndef AUTOMOTIVE_CONFIG_H
#define AUTOMOTIVE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "dds_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Automotive Constants
 * ============================================================================ */
#define DDS_AUTO_MAX_ECU_NAME_LEN       64
#define DDS_AUTO_MAX_CLUSTER_NAME_LEN   64
#define DDS_AUTO_MAX_VLAN_NAME_LEN      32
#define DDS_AUTO_MAX_GATE_CTRL_LIST     1024
#define DDS_AUTO_MAX_SHAPER_LIST        256
#define DDS_AUTO_MAX_FDB_ENTRIES        4096
#define DDS_AUTO_MAX_STREAM_ENTRIES     256
#define DDS_AUTO_MAX_SCHEDULE_ENTRIES   4096
#define DDS_AUTO_MAX_TT_SLOTS           1024
#define DDS_AUTO_MAX_REDUNDANCY_PATHS   8
#define DDS_AUTO_MAX_PD_ENTRIES         256
#define DDS_AUTO_MAX_WATCHDOG_ENTRIES   64
#define DDS_AUTO_MAX_EOL_CONFIGS        16

/* ============================================================================
 * ASIL (Automotive Safety Integrity Level) Definitions
 * ============================================================================ */
typedef enum {
    DDS_ASIL_QM = 0,    /* Quality Management - no safety requirements */
    DDS_ASIL_A,         /* ASIL A - lowest safety level */
    DDS_ASIL_B,         /* ASIL B */
    DDS_ASIL_C,         /* ASIL C */
    DDS_ASIL_D          /* ASIL D - highest safety level */
} dds_automotive_asil_t;

/* ============================================================================
 * AUTOSAR Adaptive DDS Configuration
 * ============================================================================ */
typedef enum {
    DDS_AUTOSAR_EVENT = 0,      /* Event-based communication */
    DDS_AUTOSAR_METHOD,         /* Method-based (request/response) */
    DDS_AUTOSAR_FIELD,          /* Field-based communication */
    DDS_AUTOSAR_TRIGGER        /* Trigger-based communication */
} dds_autosar_service_type_t;

typedef enum {
    DDS_AUTOSAR_STRICT = 0,     /* Strict AUTOSAR compliance */
    DDS_AUTOSAR_RELAXED,        /* Relaxed mode for non-AR apps */
    DDS_AUTOSAR_CUSTOM          /* Custom configuration */
} dds_autosar_compliance_t;

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    dds_autosar_service_type_t type;
    uint32_t service_id;
    uint32_t instance_id;
    uint32_t eventgroup_id;
    char interface_version[16];
    bool reliable;
    uint32_t max_sample_count;
    dds_automotive_asil_t safety_level;
} dds_autosar_service_t;

typedef struct {
    uint16_t port;
    char protocol[16];          /* "tcp", "udp", "someip" */
    uint32_t ttl;
    bool multicast_enabled;
} dds_autosar_network_endpoint_t;

typedef struct {
    char sd_multicast_address[64];
    uint16_t sd_port;
    uint32_t sd_ttl;
    uint32_t initial_delay_min_ms;
    uint32_t initial_delay_max_ms;
    uint32_t repetition_base_delay_ms;
    uint32_t repetition_max;
    uint32_t cyclic_offer_delay_ms;
    uint32_t request_response_delay_ms;
} dds_autosar_sd_config_t;

typedef struct {
    bool enabled;
    dds_autosar_compliance_t compliance_mode;
    dds_autosar_service_t *services;
    size_t service_count;
    dds_autosar_network_endpoint_t endpoint;
    dds_autosar_sd_config_t service_discovery;
    bool e2e_protection_enabled;
    bool secure_dds_enabled;
    uint32_t ara_com_version;   /* ARA::COM version */
} dds_autosar_adaptive_config_t;

/* ============================================================================
 * AUTOSAR Classic DDS Configuration
 * ============================================================================ */
typedef enum {
    DDS_AUTOSAR_CLASSIC_SEND = 0,
    DDS_AUTOSAR_CLASSIC_RECEIVE,
    DDS_AUTOSAR_CLASSIC_SEND_RECEIVE
} dds_autosar_classic_direction_t;

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    uint32_t pdu_id;
    uint32_t length;
    dds_autosar_classic_direction_t direction;
    uint32_t source_ecu_id;
    uint32_t destination_ecu_id;
    dds_automotive_asil_t safety_level;
    bool e2e_protected;
    uint8_t e2e_profile;        /* E2E Profile 1-7 */
} dds_autosar_pdu_t;

typedef struct {
    bool enabled;
    dds_autosar_pdu_t *pdus;
    size_t pdu_count;
    uint32_t com_stack_version;
    bool com_extension_enabled;
} dds_autosar_classic_config_t;

/* ============================================================================
 * TSN (Time-Sensitive Networking) Configuration
 * ============================================================================ */
typedef enum {
    DDS_TSN_SCHED_SP = 0,       /* Strict Priority */
    DDS_TSN_SCHED_CBS,          /* Credit-Based Shaper */
    DDS_TSN_SCHED_TAS,          /* Time-Aware Shaper */
    DDS_TSN_SCHED_ETF,          /* Earliest TxTime First */
    DDS_TSN_SCHED_CQF,          /* Cyclic Queuing and Forwarding */
    DDS_TSN_SCHED_ETS           /* Enhanced Transmission Selection */
} dds_tsn_scheduler_type_t;

typedef enum {
    DDS_TSN_GCL_SET_GATE_STATE = 0,
    DDS_TSN_GCL_SET_AND_HOLD,
    DDS_TSN_GCL_SET_AND_RELEASE
} dds_tsn_gcl_operation_t;

typedef struct {
    uint64_t start_time_ns;
    uint32_t time_interval_ns;
    uint8_t gate_states;        /* 8 bits for 8 queues */
    dds_tsn_gcl_operation_t operation;
    uint32_t queue_priority;
} dds_tsn_gate_control_entry_t;

typedef struct {
    char name[DDS_AUTO_MAX_VLAN_NAME_LEN];
    uint32_t cycle_time_ns;
    uint32_t cycle_time_extension_ns;
    uint32_t base_time_ns;
    dds_tsn_gate_control_entry_t *entries;
    size_t entry_count;
    bool enable;
    uint8_t config_change;
    uint64_t config_change_time;
    uint32_t max_sdu_size;
} dds_tsn_gcl_config_t;

typedef struct {
    uint32_t class_a_idle_slope;
    uint32_t class_a_send_slope;
    uint32_t class_a_hi_credit;
    uint32_t class_a_lo_credit;
    uint32_t class_b_idle_slope;
    uint32_t class_b_send_slope;
    uint32_t class_b_hi_credit;
    uint32_t class_b_lo_credit;
} dds_tsn_cbs_shaper_t;

typedef struct {
    uint64_t slot_duration_ns;
    uint32_t slot_count;
    uint32_t *slot_assignments; /* Per-queue slot assignment */
} dds_tsn_tas_schedule_t;

typedef struct {
    uint64_t tx_time_ns;
    uint32_t max_delay_ns;
    uint8_t traffic_class;
} dds_tsn_etf_config_t;

typedef struct {
    uint32_t cq_cycle_ns;
    uint32_t cq_tx_window_ns;
    uint32_t cq_rx_window_ns;
} dds_tsn_cqf_config_t;

typedef struct {
    char stream_id[32];         /* 64-bit stream ID as string */
    uint8_t destination_address[6];
    uint16_t vlan_id;
    uint8_t priority;
    uint32_t max_frame_size;
    uint32_t max_interval_frames;
    uint32_t accumulated_latency_ns;
    bool redundant;
    uint8_t redundancy_paths;
} dds_tsn_stream_t;

typedef struct {
    bool enabled;
    uint8_t priority;
    uint64_t offset_ns;
    uint64_t period_ns;
    uint32_t deadline_ns;
    uint32_t jitter_ns;
} dds_tsn_time_triggered_t;

/* TSN Talker Configuration */
typedef struct {
    uint64_t accumulated_latency;
    dds_tsn_etf_config_t etf;
    dds_tsn_cqf_config_t cqf;
    uint8_t traffic_specification;
} dds_tsn_talker_attributes_t;

/* TSN Listener Configuration */
typedef struct {
    uint64_t accumulated_latency;
    uint8_t interface_capabilities;
} dds_tsn_listener_attributes_t;

typedef struct {
    bool enabled;
    dds_tsn_scheduler_type_t scheduler;
    
    /* Gate Control List for TAS */
    dds_tsn_gcl_config_t *gcl_configs;
    size_t gcl_config_count;
    
    /* Credit-Based Shaper for AVB */
    dds_tsn_cbs_shaper_t cbs;
    bool cbs_enabled;
    
    /* Time-Aware Shaper */
    dds_tsn_tas_schedule_t tas_schedule;
    bool tas_enabled;
    
    /* ETF Configuration */
    dds_tsn_etf_config_t etf_config;
    bool etf_enabled;
    
    /* CQF Configuration */
    dds_tsn_cqf_config_t cqf_config;
    bool cqf_enabled;
    
    /* Stream reservations */
    dds_tsn_stream_t *streams;
    size_t stream_count;
    
    /* Talker/Listener attributes */
    dds_tsn_talker_attributes_t talker_attr;
    dds_tsn_listener_attributes_t listener_attr;
    
    /* Preemption configuration */
    bool frame_preemption_enabled;
    uint32_t preemptable_frame_min_size;
    
    /* Frame replication and elimination */
    bool frame_replication_enabled;
    uint8_t redundancy_sequence_number;
} dds_tsn_config_t;

/* ============================================================================
 * Safety-Critical Configuration (ISO 26262 / ASIL)
 * ============================================================================ */
typedef enum {
    DDS_SAFETY_LOCKSTEP = 0,    /* Lockstep execution */
    DDS_SAFETY_DMR,             /* Dual Modular Redundancy */
    DDS_SAFETY_TMR,             /* Triple Modular Redundancy */
    DDS_SAFETY_CRC,             /* CRC checking only */
    DDS_SAFETY_E2E              /* End-to-End protection */
} dds_safety_mechanism_t;

typedef enum {
    DDS_E2E_PROFILE_1 = 1,      /* CRC8, Sequence Counter */
    DDS_E2E_PROFILE_2,          /* CRC8, Sequence Counter, Data ID */
    DDS_E2E_PROFILE_4,          /* CRC32, Sequence Counter, Data ID */
    DDS_E2E_PROFILE_5,          /* CRC64, Sequence Counter, Data ID */
    DDS_E2E_PROFILE_6,          /* CRC64, Sequence Counter, Data ID, Length */
    DDS_E2E_PROFILE_7           /* CRC32, Sequence Counter, Data ID (flexible) */
} dds_e2e_profile_t;

typedef struct {
    dds_e2e_profile_t profile;
    uint16_t data_id;
    uint16_t data_id_mode;      /* Nibble, alternative, etc. */
    uint32_t max_delta_counter;
    bool crc_enabled;
    bool counter_enabled;
    bool data_id_enabled;
} dds_e2e_protection_t;

typedef struct {
    uint32_t refresh_period_ms;
    uint32_t repetition;
    uint32_t timeout_ms;
    uint32_t tolerance_ms;
    bool alive_check_enabled;
    bool sequence_check_enabled;
} dds_safety_monitoring_t;

typedef struct {
    dds_automotive_asil_t asil_level;
    dds_safety_mechanism_t safety_mechanism;
    dds_e2e_protection_t e2e_protection;
    dds_safety_monitoring_t monitoring;
    
    /* Watchdog configuration */
    bool watchdog_enabled;
    uint32_t watchdog_timeout_ms;
    uint32_t watchdog_window_ms;
    
    /* Safe monitoring */
    bool safe_monitor_enabled;
    uint32_t sm_state_check_period_ms;
    
    /* Error handling */
    uint32_t max_error_count;
    uint32_t error_recovery_time_ms;
    bool fail_silent_enabled;
} dds_safety_critical_config_t;

/* ============================================================================
 * Deterministic Communication Configuration
 * ============================================================================ */
typedef struct {
    uint64_t offset_ns;
    uint64_t period_ns;
    uint64_t deadline_ns;
    uint32_t priority;
    bool time_triggered;
} dds_det_timing_t;

typedef struct {
    uint32_t max_latency_ns;
    uint32_t max_jitter_ns;
    uint32_t min_latency_ns;
    uint32_t avg_latency_ns;
    bool latency_monitoring_enabled;
} dds_det_latency_config_t;

typedef struct {
    uint32_t bucket_depth;
    uint32_t bucket_rate;
    bool shaping_enabled;
} dds_det_traffic_shaping_t;

typedef struct {
    bool enabled;
    dds_det_timing_t timing;
    dds_det_latency_config_t latency;
    dds_det_traffic_shaping_t shaping;
    bool synchronized;
    uint32_t sync_domain;
    uint64_t sync_tolerance_ns;
} dds_deterministic_config_t;

/* ============================================================================
 * Network Management / Diagnostic Configuration
 * ============================================================================ */
typedef enum {
    DDS_NM_STATE_BUS_SLEEP = 0,
    DDS_NM_STATE_PREPARE_BUS_SLEEP,
    DDS_NM_STATE_READY_SLEEP,
    DDS_NM_STATE_NORMAL_OPERATION,
    DDS_NM_STATE_REPEAT_MESSAGE,
    DDS_NM_STATE_SYNCHRONIZE
} dds_network_mgmt_state_t;

typedef struct {
    bool enabled;
    uint32_t nm_message_cycle_time_ms;
    uint32_t nm_timeout_time_ms;
    uint32_t nm_repeat_message_time_ms;
    uint32_t nm_wait_bus_sleep_time_ms;
    bool nm_remote_sleep_indication_enabled;
    uint32_t nm_node_id;
    dds_network_mgmt_state_t initial_state;
} dds_network_mgmt_config_t;

typedef struct {
    bool uds_enabled;
    uint16_t uds_source_address;
    uint16_t uds_target_address;
    bool doip_enabled;
    uint16_t doip_logical_address;
    char doip_vehicle_id[32];
    uint32_t diag_session_timeout_ms;
} dds_diagnostic_config_t;

/* ============================================================================
 * Redundancy Configuration
 * ============================================================================ */
typedef enum {
    DDS_REDUNDANCY_ACTIVE_STANDBY = 0,
    DDS_REDUNDANCY_ACTIVE_ACTIVE,
    DDS_REDUNDANCY_LOAD_BALANCE,
    DDS_REDUNDANCY_PRP,         /* Parallel Redundancy Protocol */
    DDS_REDUNDANCY_HSR          /* High-availability Seamless Redundancy */
} dds_redundancy_mode_t;

typedef struct {
    char interface_name[32];
    uint8_t priority;
    bool active;
    uint32_t latency_ns;
} dds_redundancy_path_t;

typedef struct {
    bool enabled;
    dds_redundancy_mode_t mode;
    dds_redundancy_path_t paths[DDS_AUTO_MAX_REDUNDANCY_PATHS];
    size_t path_count;
    uint32_t switchover_time_ms;
    bool seamless;
    uint32_t duplication_detection_window_ns;
} dds_redundancy_config_t;

/* ============================================================================
 * Power Management / Energy Efficiency
 * ============================================================================ */
typedef enum {
    DDS_PWR_MODE_FULL = 0,
    DDS_PWR_MODE_LOW,
    DDS_PWR_MODE_SLEEP,
    DDS_PWR_MODE_DEEP_SLEEP
} dds_power_mode_t;

typedef struct {
    bool enabled;
    uint32_t idle_timeout_ms;
    uint32_t traffic_threshold_bps;
    bool wake_on_lan_enabled;
    bool wake_on_pattern_enabled;
    dds_power_mode_t initial_mode;
    dds_power_mode_t sleep_mode;
} dds_power_mgmt_config_t;

/* ============================================================================
 * Gateway / Routing Configuration
 * ============================================================================ */
typedef enum {
    DDS_GATEWAY_PROTOCOL_DDS = 0,
    DDS_GATEWAY_PROTOCOL_SOMEIP,
    DDS_GATEWAY_PROTOCOL_CAN,
    DDS_GATEWAY_PROTOCOL_LIN,
    DDS_GATEWAY_PROTOCOL_FLEXRAY,
    DDS_GATEWAY_PROTOCOL_ETH_RAW
} dds_gateway_protocol_t;

typedef struct {
    dds_gateway_protocol_t source_protocol;
    dds_gateway_protocol_t target_protocol;
    char source_topic[DDS_CONFIG_MAX_NAME_LEN];
    char target_topic[DDS_CONFIG_MAX_NAME_LEN];
    uint32_t source_id;
    uint32_t target_id;
    bool translation_enabled;
    uint32_t buffer_size;
    uint32_t timeout_ms;
} dds_gateway_route_t;

typedef struct {
    bool enabled;
    dds_gateway_route_t *routes;
    size_t route_count;
    uint32_t max_latency_ns;
    bool protocol_conversion_enabled;
} dds_gateway_config_t;

/* ============================================================================
 * Main Automotive Configuration
 * ============================================================================ */
typedef struct {
    /* ECU identification */
    char ecu_id[DDS_AUTO_MAX_ECU_NAME_LEN];
    char cluster_name[DDS_AUTO_MAX_CLUSTER_NAME_LEN];
    uint32_t node_id;
    
    /* AUTOSAR configurations */
    dds_autosar_adaptive_config_t autosar_adaptive;
    dds_autosar_classic_config_t autosar_classic;
    
    /* TSN configuration */
    dds_tsn_config_t tsn;
    
    /* Safety configuration */
    dds_safety_critical_config_t safety;
    
    /* Deterministic communication */
    dds_deterministic_config_t deterministic;
    
    /* Network management */
    dds_network_mgmt_config_t network_mgmt;
    
    /* Diagnostics */
    dds_diagnostic_config_t diagnostic;
    
    /* Redundancy */
    dds_redundancy_config_t redundancy;
    
    /* Power management */
    dds_power_mgmt_config_t power_mgmt;
    
    /* Gateway */
    dds_gateway_config_t gateway;
    
    /* Vehicle-wide settings */
    uint32_t vehicle_id;
    uint32_t domain_controller_id;
    char vehicle_architecture[32];  /* "zonal", "centralized", "hybrid" */
} dds_automotive_config_t;

/* ============================================================================
 * API Functions - Automotive Configuration
 * ============================================================================ */

/* Configuration lifecycle */
dds_automotive_config_t* dds_automotive_config_create(void);
void dds_automotive_config_destroy(dds_automotive_config_t *config);
dds_automotive_config_t* dds_automotive_config_clone(const dds_automotive_config_t *config);

/* Configuration loading and validation */
dds_config_error_t dds_automotive_config_load(dds_automotive_config_t *config,
                                               const dds_configuration_t *dds_config);
dds_config_error_t dds_automotive_config_load_file(dds_automotive_config_t *config,
                                                    const char *filename);
dds_config_error_t dds_automotive_config_validate(const dds_automotive_config_t *config);
bool dds_automotive_config_is_valid(const dds_automotive_config_t *config);

/* AUTOSAR operations */
dds_config_error_t dds_autosar_add_service(dds_autosar_adaptive_config_t *config,
                                           const dds_autosar_service_t *service);
dds_config_error_t dds_autosar_remove_service(dds_autosar_adaptive_config_t *config,
                                              const char *service_name);
dds_autosar_service_t* dds_autosar_find_service(dds_autosar_adaptive_config_t *config,
                                                 const char *service_name);

/* TSN operations */
dds_config_error_t dds_tsn_add_gcl_entry(dds_tsn_gcl_config_t *gcl,
                                          const dds_tsn_gate_control_entry_t *entry);
dds_config_error_t dds_tsn_add_stream(dds_tsn_config_t *config,
                                       const dds_tsn_stream_t *stream);
dds_config_error_t dds_tsn_validate_schedule(const dds_tsn_gcl_config_t *gcl);

/* Safety operations */
dds_config_error_t dds_safety_configure_e2e(dds_safety_critical_config_t *config,
                                             dds_e2e_profile_t profile,
                                             uint16_t data_id);
dds_config_error_t dds_safety_validate_asil_compliance(const dds_automotive_config_t *config);

/* Redundancy operations */
dds_config_error_t dds_redundancy_add_path(dds_redundancy_config_t *config,
                                            const dds_redundancy_path_t *path);
dds_config_error_t dds_redundancy_failover(dds_redundancy_config_t *config,
                                            size_t failed_path_index);

/* Utility functions */
const char* dds_asil_to_string(dds_automotive_asil_t asil);
const char* dds_autosar_service_type_to_string(dds_autosar_service_type_t type);
const char* dds_tsn_scheduler_to_string(dds_tsn_scheduler_type_t scheduler);
const char* dds_safety_mechanism_to_string(dds_safety_mechanism_t mechanism);
const char* dds_e2e_profile_to_string(dds_e2e_profile_t profile);
const char* dds_redundancy_mode_to_string(dds_redundancy_mode_t mode);

uint64_t dds_tsn_calculate_slot_time(uint32_t frame_size, uint32_t link_speed_mbps);
bool dds_tsn_is_schedule_feasible(const dds_tsn_gcl_config_t *gcl, 
                                   uint32_t link_speed_mbps);

#ifdef __cplusplus
}
#endif

#endif /* AUTOMOTIVE_CONFIG_H */
