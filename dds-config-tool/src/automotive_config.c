/*
 * DDS Configuration Tool - Automotive Configuration Implementation
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "automotive_config.h"
#include "dds_config_parser.h"

/* ============================================================================
 * Utility Macros
 * ============================================================================ */
#define SAFE_STRCPY(dest, src, max) do { \
    if ((src) != NULL) { \
        strncpy((dest), (src), (max) - 1); \
        (dest)[(max) - 1] = '\0'; \
    } else { \
        (dest)[0] = '\0'; \
    } \
} while(0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* ============================================================================
 * Configuration Lifecycle
 * ============================================================================ */

dds_automotive_config_t* dds_automotive_config_create(void)
{
    dds_automotive_config_t *config = calloc(1, sizeof(dds_automotive_config_t));
    if (!config) {
        return NULL;
    }

    /* Initialize default values */
    SAFE_STRCPY(config->ecu_id, "ECU_0", DDS_AUTO_MAX_ECU_NAME_LEN);
    SAFE_STRCPY(config->cluster_name, "DefaultCluster", DDS_AUTO_MAX_CLUSTER_NAME_LEN);
    config->node_id = 0;
    
    /* AUTOSAR Adaptive defaults */
    config->autosar_adaptive.enabled = false;
    config->autosar_adaptive.compliance_mode = DDS_AUTOSAR_STRICT;
    config->autosar_adaptive.ara_com_version = 2206;  /* R22-11 */
    config->autosar_adaptive.endpoint.port = 30509;
    SAFE_STRCPY(config->autosar_adaptive.endpoint.protocol, "someip", 16);
    config->autosar_adaptive.endpoint.ttl = 255;
    config->autosar_adaptive.endpoint.multicast_enabled = true;
    
    /* Service Discovery defaults */
    SAFE_STRCPY(config->autosar_adaptive.service_discovery.sd_multicast_address, 
                "224.244.224.245", 64);
    config->autosar_adaptive.service_discovery.sd_port = 30490;
    config->autosar_adaptive.service_discovery.sd_ttl = 255;
    config->autosar_adaptive.service_discovery.initial_delay_min_ms = 50;
    config->autosar_adaptive.service_discovery.initial_delay_max_ms = 150;
    config->autosar_adaptive.service_discovery.repetition_base_delay_ms = 100;
    config->autosar_adaptive.service_discovery.repetition_max = 3;
    config->autosar_adaptive.service_discovery.cyclic_offer_delay_ms = 2000;
    config->autosar_adaptive.service_discovery.request_response_delay_ms = 200;
    
    /* AUTOSAR Classic defaults */
    config->autosar_classic.enabled = false;
    config->autosar_classic.com_stack_version = 440;
    
    /* TSN defaults */
    config->tsn.enabled = false;
    config->tsn.scheduler = DDS_TSN_SCHED_TAS;
    config->tsn.cbs_enabled = true;
    config->tsn.frame_preemption_enabled = false;
    config->tsn.preemptable_frame_min_size = 64;
    config->tsn.frame_replication_enabled = false;
    
    /* CBS defaults (AVB classes A & B) */
    config->tsn.cbs.class_a_idle_slope = 7864320;   /* 6.25% of 1Gbps */
    config->tsn.cbs.class_a_send_slope = -70778880;
    config->tsn.cbs.class_a_hi_credit = 61440;
    config->tsn.cbs.class_a_lo_credit = -552960;
    
    /* Safety defaults - QM (no safety) */
    config->safety.asil_level = DDS_ASIL_QM;
    config->safety.safety_mechanism = DDS_SAFETY_E2E;
    config->safety.e2e_protection.profile = DDS_E2E_PROFILE_2;
    config->safety.e2e_protection.max_delta_counter = 1;
    config->safety.e2e_protection.crc_enabled = true;
    config->safety.e2e_protection.counter_enabled = true;
    config->safety.e2e_protection.data_id_enabled = true;
    
    config->safety.watchdog_enabled = true;
    config->safety.watchdog_timeout_ms = 100;
    config->safety.watchdog_window_ms = 10;
    
    config->safety.max_error_count = 10;
    config->safety.error_recovery_time_ms = 1000;
    
    /* Deterministic defaults */
    config->deterministic.enabled = false;
    config->deterministic.synchronized = true;
    config->deterministic.sync_domain = 0;
    config->deterministic.sync_tolerance_ns = 1000;  /* 1 microsecond */
    config->deterministic.latency.latency_monitoring_enabled = true;
    
    /* Network Management defaults */
    config->network_mgmt.enabled = false;
    config->network_mgmt.nm_message_cycle_time_ms = 20;
    config->network_mgmt.nm_timeout_time_ms = 500;
    config->network_mgmt.nm_repeat_message_time_ms = 1600;
    config->network_mgmt.nm_wait_bus_sleep_time_ms = 2000;
    config->network_mgmt.nm_remote_sleep_indication_enabled = true;
    config->network_mgmt.initial_state = DDS_NM_STATE_NORMAL_OPERATION;
    
    /* Diagnostic defaults */
    config->diagnostic.uds_enabled = true;
    config->diagnostic.uds_source_address = 0x1A;
    config->diagnostic.uds_target_address = 0x7A;
    config->diagnostic.doip_enabled = false;
    config->diagnostic.doip_logical_address = 0x0001;
    config->diagnostic.diag_session_timeout_ms = 5000;
    
    /* Redundancy defaults */
    config->redundancy.enabled = false;
    config->redundancy.mode = DDS_REDUNDANCY_ACTIVE_STANDBY;
    config->redundancy.switchover_time_ms = 50;
    config->redundancy.seamless = true;
    config->redundancy.duplication_detection_window_ns = 10000000;  /* 10ms */
    
    /* Power Management defaults */
    config->power_mgmt.enabled = true;
    config->power_mgmt.idle_timeout_ms = 5000;
    config->power_mgmt.traffic_threshold_bps = 1000;
    config->power_mgmt.wake_on_lan_enabled = true;
    config->power_mgmt.initial_mode = DDS_PWR_MODE_FULL;
    config->power_mgmt.sleep_mode = DDS_PWR_MODE_SLEEP;
    
    /* Gateway defaults */
    config->gateway.enabled = false;
    config->gateway.max_latency_ns = 1000000;  /* 1ms */
    config->gateway.protocol_conversion_enabled = true;
    
    /* Vehicle architecture */
    config->vehicle_id = 0;
    config->domain_controller_id = 0;
    SAFE_STRCPY(config->vehicle_architecture, "zonal", 32);
    
    return config;
}

void dds_automotive_config_destroy(dds_automotive_config_t *config)
{
    if (!config) return;
    
    /* Free AUTOSAR Adaptive services */
    if (config->autosar_adaptive.services) {
        free(config->autosar_adaptive.services);
    }
    
    /* Free AUTOSAR Classic PDUs */
    if (config->autosar_classic.pdus) {
        free(config->autosar_classic.pdus);
    }
    
    /* Free TSN GCL entries */
    if (config->tsn.gcl_configs) {
        for (size_t i = 0; i < config->tsn.gcl_config_count; i++) {
            if (config->tsn.gcl_configs[i].entries) {
                free(config->tsn.gcl_configs[i].entries);
            }
        }
        free(config->tsn.gcl_configs);
    }
    
    /* Free TSN streams */
    if (config->tsn.streams) {
        free(config->tsn.streams);
    }
    
    /* Free TSN TAS schedule assignments */
    if (config->tsn.tas_schedule.slot_assignments) {
        free(config->tsn.tas_schedule.slot_assignments);
    }
    
    /* Free gateway routes */
    if (config->gateway.routes) {
        free(config->gateway.routes);
    }
    
    free(config);
}

dds_automotive_config_t* dds_automotive_config_clone(const dds_automotive_config_t *config)
{
    if (!config) return NULL;
    
    dds_automotive_config_t *clone = dds_automotive_config_create();
    if (!clone) return NULL;
    
    /* Copy basic fields */
    memcpy(clone, config, sizeof(dds_automotive_config_t));
    
    /* Deep copy AUTOSAR Adaptive services */
    if (config->autosar_adaptive.service_count > 0 && config->autosar_adaptive.services) {
        clone->autosar_adaptive.services = calloc(config->autosar_adaptive.service_count,
                                                   sizeof(dds_autosar_service_t));
        if (clone->autosar_adaptive.services) {
            memcpy(clone->autosar_adaptive.services, config->autosar_adaptive.services,
                   config->autosar_adaptive.service_count * sizeof(dds_autosar_service_t));
        }
    }
    
    /* Deep copy AUTOSAR Classic PDUs */
    if (config->autosar_classic.pdu_count > 0 && config->autosar_classic.pdus) {
        clone->autosar_classic.pdus = calloc(config->autosar_classic.pdu_count,
                                              sizeof(dds_autosar_pdu_t));
        if (clone->autosar_classic.pdus) {
            memcpy(clone->autosar_classic.pdus, config->autosar_classic.pdus,
                   config->autosar_classic.pdu_count * sizeof(dds_autosar_pdu_t));
        }
    }
    
    /* Deep copy TSN GCL configs */
    if (config->tsn.gcl_config_count > 0 && config->tsn.gcl_configs) {
        clone->tsn.gcl_configs = calloc(config->tsn.gcl_config_count,
                                         sizeof(dds_tsn_gcl_config_t));
        if (clone->tsn.gcl_configs) {
            for (size_t i = 0; i < config->tsn.gcl_config_count; i++) {
                memcpy(&clone->tsn.gcl_configs[i], &config->tsn.gcl_configs[i],
                       sizeof(dds_tsn_gcl_config_t));
                
                if (config->tsn.gcl_configs[i].entry_count > 0 && 
                    config->tsn.gcl_configs[i].entries) {
                    clone->tsn.gcl_configs[i].entries = calloc(
                        config->tsn.gcl_configs[i].entry_count,
                        sizeof(dds_tsn_gate_control_entry_t));
                    if (clone->tsn.gcl_configs[i].entries) {
                        memcpy(clone->tsn.gcl_configs[i].entries,
                               config->tsn.gcl_configs[i].entries,
                               config->tsn.gcl_configs[i].entry_count * 
                               sizeof(dds_tsn_gate_control_entry_t));
                    }
                }
            }
        }
    }
    
    /* Deep copy TSN streams */
    if (config->tsn.stream_count > 0 && config->tsn.streams) {
        clone->tsn.streams = calloc(config->tsn.stream_count, sizeof(dds_tsn_stream_t));
        if (clone->tsn.streams) {
            memcpy(clone->tsn.streams, config->tsn.streams,
                   config->tsn.stream_count * sizeof(dds_tsn_stream_t));
        }
    }
    
    /* Deep copy gateway routes */
    if (config->gateway.route_count > 0 && config->gateway.routes) {
        clone->gateway.routes = calloc(config->gateway.route_count, 
                                        sizeof(dds_gateway_route_t));
        if (clone->gateway.routes) {
            memcpy(clone->gateway.routes, config->gateway.routes,
                   config->gateway.route_count * sizeof(dds_gateway_route_t));
        }
    }
    
    return clone;
}

/* ============================================================================
 * Configuration Loading and Validation
 * ============================================================================ */

dds_config_error_t dds_automotive_config_load(dds_automotive_config_t *config,
                                               const dds_configuration_t *dds_config)
{
    if (!config || !dds_config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Extract automotive-specific settings from DDS config */
    /* This is a simplified implementation - real version would parse extensions */
    
    /* Example: Extract domain-specific automotive settings */
    for (size_t i = 0; i < dds_config->domain_count; i++) {
        const dds_domain_config_t *domain = &dds_config->domains[i];
        
        /* Map transport settings to TSN if applicable */
        if (domain->transport.multicast_enabled) {
            config->tsn.enabled = true;
        }
    }
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_automotive_config_load_file(dds_automotive_config_t *config,
                                                    const char *filename)
{
    if (!config || !filename) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* TODO: Implement full automotive YAML config file parsing */
    /* This would parse a dedicated automotive configuration file */
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_automotive_config_validate(const dds_automotive_config_t *config)
{
    if (!config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Validate TSN configuration */
    if (config->tsn.enabled) {
        if (config->tsn.gcl_config_count == 0 && config->tsn.scheduler == DDS_TSN_SCHED_TAS) {
            return DDS_CONFIG_ERR_VALIDATION_FAILED;
        }
        
        /* Validate GCL entries don't exceed cycle time */
        for (size_t i = 0; i < config->tsn.gcl_config_count; i++) {
            const dds_tsn_gcl_config_t *gcl = &config->tsn.gcl_configs[i];
            uint32_t total_time = 0;
            
            for (size_t j = 0; j < gcl->entry_count; j++) {
                total_time += gcl->entries[j].time_interval_ns;
            }
            
            if (total_time > gcl->cycle_time_ns) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
        }
        
        /* Validate stream configurations */
        for (size_t i = 0; i < config->tsn.stream_count; i++) {
            const dds_tsn_stream_t *stream = &config->tsn.streams[i];
            
            if (stream->vlan_id > 4094) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
            
            if (stream->priority > 7) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
        }
    }
    
    /* Validate safety configuration */
    if (config->safety.asil_level != DDS_ASIL_QM) {
        /* ASIL B-D requires E2E protection */
        if (!config->safety.e2e_protection.crc_enabled &&
            !config->safety.e2e_protection.counter_enabled) {
            return DDS_CONFIG_ERR_VALIDATION_FAILED;
        }
        
        /* Watchdog required for ASIL B+ */
        if (config->safety.asil_level >= DDS_ASIL_B && !config->safety.watchdog_enabled) {
            return DDS_CONFIG_ERR_VALIDATION_FAILED;
        }
    }
    
    /* Validate redundancy configuration */
    if (config->redundancy.enabled) {
        if (config->redundancy.path_count < 2) {
            return DDS_CONFIG_ERR_VALIDATION_FAILED;
        }
        
        /* At least one path must be active */
        bool has_active = false;
        for (size_t i = 0; i < config->redundancy.path_count; i++) {
            if (config->redundancy.paths[i].active) {
                has_active = true;
                break;
            }
        }
        
        if (!has_active) {
            return DDS_CONFIG_ERR_VALIDATION_FAILED;
        }
    }
    
    return DDS_CONFIG_OK;
}

bool dds_automotive_config_is_valid(const dds_automotive_config_t *config)
{
    return dds_automotive_config_validate(config) == DDS_CONFIG_OK;
}

/* ============================================================================
 * AUTOSAR Operations
 * ============================================================================ */

dds_config_error_t dds_autosar_add_service(dds_autosar_adaptive_config_t *config,
                                           const dds_autosar_service_t *service)
{
    if (!config || !service) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Check for duplicate service name */
    for (size_t i = 0; i < config->service_count; i++) {
        if (strcmp(config->services[i].name, service->name) == 0) {
            return DDS_CONFIG_ERR_DUPLICATE_TOPIC;
        }
    }
    
    dds_autosar_service_t *new_services = realloc(config->services,
                                                   (config->service_count + 1) * 
                                                   sizeof(dds_autosar_service_t));
    if (!new_services) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    config->services = new_services;
    memcpy(&config->services[config->service_count], service, sizeof(dds_autosar_service_t));
    config->service_count++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_autosar_remove_service(dds_autosar_adaptive_config_t *config,
                                              const char *service_name)
{
    if (!config || !service_name) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    for (size_t i = 0; i < config->service_count; i++) {
        if (strcmp(config->services[i].name, service_name) == 0) {
            /* Shift remaining services */
            if (i < config->service_count - 1) {
                memmove(&config->services[i], &config->services[i + 1],
                        (config->service_count - i - 1) * sizeof(dds_autosar_service_t));
            }
            config->service_count--;
            return DDS_CONFIG_OK;
        }
    }
    
    return DDS_CONFIG_ERR_NOT_FOUND;
}

dds_autosar_service_t* dds_autosar_find_service(dds_autosar_adaptive_config_t *config,
                                                 const char *service_name)
{
    if (!config || !service_name) {
        return NULL;
    }
    
    for (size_t i = 0; i < config->service_count; i++) {
        if (strcmp(config->services[i].name, service_name) == 0) {
            return &config->services[i];
        }
    }
    
    return NULL;
}

/* ============================================================================
 * TSN Operations
 * ============================================================================ */

dds_config_error_t dds_tsn_add_gcl_entry(dds_tsn_gcl_config_t *gcl,
                                          const dds_tsn_gate_control_entry_t *entry)
{
    if (!gcl || !entry) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (gcl->entry_count >= DDS_AUTO_MAX_GATE_CTRL_LIST) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    dds_tsn_gate_control_entry_t *new_entries = realloc(gcl->entries,
                                                         (gcl->entry_count + 1) * 
                                                         sizeof(dds_tsn_gate_control_entry_t));
    if (!new_entries) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    gcl->entries = new_entries;
    memcpy(&gcl->entries[gcl->entry_count], entry, sizeof(dds_tsn_gate_control_entry_t));
    gcl->entry_count++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_tsn_add_stream(dds_tsn_config_t *config,
                                       const dds_tsn_stream_t *stream)
{
    if (!config || !stream) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (config->stream_count >= DDS_AUTO_MAX_STREAM_ENTRIES) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    dds_tsn_stream_t *new_streams = realloc(config->streams,
                                             (config->stream_count + 1) * 
                                             sizeof(dds_tsn_stream_t));
    if (!new_streams) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    config->streams = new_streams;
    memcpy(&config->streams[config->stream_count], stream, sizeof(dds_tsn_stream_t));
    config->stream_count++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_tsn_validate_schedule(const dds_tsn_gcl_config_t *gcl)
{
    if (!gcl) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (gcl->entry_count == 0) {
        return DDS_CONFIG_ERR_VALIDATION_FAILED;
    }
    
    /* Check cycle time is reasonable */
    if (gcl->cycle_time_ns < 1000 || gcl->cycle_time_ns > 1000000000) {
        return DDS_CONFIG_ERR_VALIDATION_FAILED;
    }
    
    /* Calculate total GCL time */
    uint64_t total_time = 0;
    for (size_t i = 0; i < gcl->entry_count; i++) {
        total_time += gcl->entries[i].time_interval_ns;
    }
    
    /* Total GCL time should not exceed cycle time */
    if (total_time > gcl->cycle_time_ns) {
        return DDS_CONFIG_ERR_VALIDATION_FAILED;
    }
    
    /* Validate gate states (8 bits for 8 queues) */
    for (size_t i = 0; i < gcl->entry_count; i++) {
        /* gate_states should be 8 bits */
        if (gcl->entries[i].gate_states > 0xFF) {
            return DDS_CONFIG_ERR_VALIDATION_FAILED;
        }
    }
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Safety Operations
 * ============================================================================ */

dds_config_error_t dds_safety_configure_e2e(dds_safety_critical_config_t *config,
                                             dds_e2e_profile_t profile,
                                             uint16_t data_id)
{
    if (!config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    config->e2e_protection.profile = profile;
    config->e2e_protection.data_id = data_id;
    
    /* Configure profile-specific settings */
    switch (profile) {
        case DDS_E2E_PROFILE_1:
            config->e2e_protection.crc_enabled = true;
            config->e2e_protection.counter_enabled = true;
            config->e2e_protection.data_id_enabled = false;
            break;
            
        case DDS_E2E_PROFILE_2:
            config->e2e_protection.crc_enabled = true;
            config->e2e_protection.counter_enabled = true;
            config->e2e_protection.data_id_enabled = true;
            break;
            
        case DDS_E2E_PROFILE_4:
            config->e2e_protection.crc_enabled = true;
            config->e2e_protection.counter_enabled = true;
            config->e2e_protection.data_id_enabled = true;
            break;
            
        case DDS_E2E_PROFILE_5:
        case DDS_E2E_PROFILE_6:
            config->e2e_protection.crc_enabled = true;
            config->e2e_protection.counter_enabled = true;
            config->e2e_protection.data_id_enabled = true;
            break;
            
        case DDS_E2E_PROFILE_7:
            config->e2e_protection.crc_enabled = true;
            config->e2e_protection.counter_enabled = true;
            config->e2e_protection.data_id_enabled = true;
            break;
            
        default:
            return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_safety_validate_asil_compliance(const dds_automotive_config_t *config)
{
    if (!config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    switch (config->safety.asil_level) {
        case DDS_ASIL_QM:
            /* No specific requirements */
            break;
            
        case DDS_ASIL_A:
            /* Basic E2E protection required */
            if (!config->safety.e2e_protection.crc_enabled) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
            break;
            
        case DDS_ASIL_B:
            /* Enhanced E2E + Watchdog */
            if (!config->safety.e2e_protection.crc_enabled ||
                !config->safety.e2e_protection.counter_enabled) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
            if (!config->safety.watchdog_enabled) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
            break;
            
        case DDS_ASIL_C:
        case DDS_ASIL_D:
            /* Maximum protection */
            if (!config->safety.e2e_protection.crc_enabled ||
                !config->safety.e2e_protection.counter_enabled ||
                !config->safety.e2e_protection.data_id_enabled) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
            if (!config->safety.watchdog_enabled || !config->safety.fail_silent_enabled) {
                return DDS_CONFIG_ERR_VALIDATION_FAILED;
            }
            if (config->redundancy.enabled == false && config->safety.asil_level == DDS_ASIL_D) {
                /* ASIL D recommends redundancy */
            }
            break;
    }
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Redundancy Operations
 * ============================================================================ */

dds_config_error_t dds_redundancy_add_path(dds_redundancy_config_t *config,
                                            const dds_redundancy_path_t *path)
{
    if (!config || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (config->path_count >= DDS_AUTO_MAX_REDUNDANCY_PATHS) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    memcpy(&config->paths[config->path_count], path, sizeof(dds_redundancy_path_t));
    config->path_count++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_redundancy_failover(dds_redundancy_config_t *config,
                                            size_t failed_path_index)
{
    if (!config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (failed_path_index >= config->path_count) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Mark the failed path as inactive */
    config->paths[failed_path_index].active = false;
    
    /* In active-standby mode, activate the next available path */
    if (config->mode == DDS_REDUNDANCY_ACTIVE_STANDBY) {
        for (size_t i = 0; i < config->path_count; i++) {
            if (i != failed_path_index && !config->paths[i].active) {
                config->paths[i].active = true;
                config->paths[i].priority = 1;  /* Promote to active */
                break;
            }
        }
    }
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* dds_asil_to_string(dds_automotive_asil_t asil)
{
    switch (asil) {
        case DDS_ASIL_QM: return "QM";
        case DDS_ASIL_A:  return "ASIL-A";
        case DDS_ASIL_B:  return "ASIL-B";
        case DDS_ASIL_C:  return "ASIL-C";
        case DDS_ASIL_D:  return "ASIL-D";
        default: return "UNKNOWN";
    }
}

const char* dds_autosar_service_type_to_string(dds_autosar_service_type_t type)
{
    switch (type) {
        case DDS_AUTOSAR_EVENT:   return "EVENT";
        case DDS_AUTOSAR_METHOD:  return "METHOD";
        case DDS_AUTOSAR_FIELD:   return "FIELD";
        case DDS_AUTOSAR_TRIGGER: return "TRIGGER";
        default: return "UNKNOWN";
    }
}

const char* dds_tsn_scheduler_to_string(dds_tsn_scheduler_type_t scheduler)
{
    switch (scheduler) {
        case DDS_TSN_SCHED_SP:  return "StrictPriority";
        case DDS_TSN_SCHED_CBS: return "CreditBasedShaper";
        case DDS_TSN_SCHED_TAS: return "TimeAwareShaper";
        case DDS_TSN_SCHED_ETF: return "EarliestTxTimeFirst";
        case DDS_TSN_SCHED_CQF: return "CyclicQueuingForwarding";
        case DDS_TSN_SCHED_ETS: return "EnhancedTransmissionSelection";
        default: return "UNKNOWN";
    }
}

const char* dds_safety_mechanism_to_string(dds_safety_mechanism_t mechanism)
{
    switch (mechanism) {
        case DDS_SAFETY_LOCKSTEP: return "Lockstep";
        case DDS_SAFETY_DMR:      return "DMR";
        case DDS_SAFETY_TMR:      return "TMR";
        case DDS_SAFETY_CRC:      return "CRC";
        case DDS_SAFETY_E2E:      return "EndToEnd";
        default: return "UNKNOWN";
    }
}

const char* dds_e2e_profile_to_string(dds_e2e_profile_t profile)
{
    switch (profile) {
        case DDS_E2E_PROFILE_1: return "PROFILE_1";
        case DDS_E2E_PROFILE_2: return "PROFILE_2";
        case DDS_E2E_PROFILE_4: return "PROFILE_4";
        case DDS_E2E_PROFILE_5: return "PROFILE_5";
        case DDS_E2E_PROFILE_6: return "PROFILE_6";
        case DDS_E2E_PROFILE_7: return "PROFILE_7";
        default: return "UNKNOWN";
    }
}

const char* dds_redundancy_mode_to_string(dds_redundancy_mode_t mode)
{
    switch (mode) {
        case DDS_REDUNDANCY_ACTIVE_STANDBY: return "ActiveStandby";
        case DDS_REDUNDANCY_ACTIVE_ACTIVE:  return "ActiveActive";
        case DDS_REDUNDANCY_LOAD_BALANCE:   return "LoadBalance";
        case DDS_REDUNDANCY_PRP:            return "PRP";
        case DDS_REDUNDANCY_HSR:            return "HSR";
        default: return "UNKNOWN";
    }
}

uint64_t dds_tsn_calculate_slot_time(uint32_t frame_size, uint32_t link_speed_mbps)
{
    /* Ethernet overhead: preamble (8) + header (14) + FCS (4) + IFG (12) = 38 bytes */
    uint32_t total_size = frame_size + 38;
    
    /* Time = (bits) / (bits per nanosecond) */
    /* link_speed_mbps Mbps = link_speed_mbps * 1000000 bits per second */
    /* = link_speed_mbps bits per microsecond */
    /* = link_speed_mbps / 1000 bits per nanosecond */
    
    uint64_t time_ns = ((uint64_t)total_size * 8 * 1000) / link_speed_mbps;
    
    return time_ns;
}

bool dds_tsn_is_schedule_feasible(const dds_tsn_gcl_config_t *gcl, 
                                   uint32_t link_speed_mbps)
{
    if (!gcl || gcl->entry_count == 0) {
        return false;
    }
    
    /* Calculate minimum time needed for all entries */
    uint64_t total_needed_ns = 0;
    for (size_t i = 0; i < gcl->entry_count; i++) {
        total_needed_ns += gcl->entries[i].time_interval_ns;
    }
    
    /* Add overhead for each entry */
    total_needed_ns += gcl->entry_count * 100;  /* 100ns overhead per entry */
    
    /* Check if fits within cycle time */
    return total_needed_ns <= gcl->cycle_time_ns;
}
