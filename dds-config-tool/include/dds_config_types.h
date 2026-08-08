/*
 * DDS Configuration Tool - Type Definitions
 * 
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#ifndef DDS_CONFIG_TYPES_H
#define DDS_CONFIG_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Version Information
 * ============================================================================ */
#define DDS_CONFIG_VERSION_MAJOR 1
#define DDS_CONFIG_VERSION_MINOR 0
#define DDS_CONFIG_VERSION_PATCH 0
#define DDS_CONFIG_VERSION_STRING "1.0.0"

/* ============================================================================
 * Error Codes
 * ============================================================================ */
typedef enum {
    DDS_CONFIG_OK = 0,
    DDS_CONFIG_ERR_INVALID_FORMAT = -1,
    DDS_CONFIG_ERR_SCHEMA_VIOLATION = -2,
    DDS_CONFIG_ERR_INVALID_DOMAIN = -3,
    DDS_CONFIG_ERR_DUPLICATE_TOPIC = -4,
    DDS_CONFIG_ERR_INVALID_QOS = -5,
    DDS_CONFIG_ERR_FILE_IO = -6,
    DDS_CONFIG_ERR_MEMORY = -7,
    DDS_CONFIG_ERR_NOT_FOUND = -8,
    DDS_CONFIG_ERR_INVALID_ARGUMENT = -9,
    DDS_CONFIG_ERR_PARSE_ERROR = -10,
    DDS_CONFIG_ERR_VALIDATION_FAILED = -11,
    DDS_CONFIG_ERR_INTERNAL = -99
} dds_config_error_t;

/* ============================================================================
 * Constants
 * ============================================================================ */
#define DDS_CONFIG_MAX_NAME_LEN 256
#define DDS_CONFIG_MAX_DESCRIPTION_LEN 1024
#define DDS_CONFIG_MAX_TOPICS 1024
#define DDS_CONFIG_MAX_QOS_PROFILES 256
#define DDS_CONFIG_MAX_DOMAINS 231
#define DDS_CONFIG_MAX_PARTICIPANTS 256
#define DDS_CONFIG_MAX_FIELDS 128

#define DDS_DOMAIN_ID_MIN 0
#define DDS_DOMAIN_ID_MAX 230

/* ============================================================================
 * QoS Enumerations
 * ============================================================================ */

typedef enum {
    DDS_RELIABILITY_BEST_EFFORT = 0,
    DDS_RELIABILITY_RELIABLE
} dds_reliability_kind_t;

typedef enum {
    DDS_DURABILITY_VOLATILE = 0,
    DDS_DURABILITY_TRANSIENT_LOCAL,
    DDS_DURABILITY_TRANSIENT,
    DDS_DURABILITY_PERSISTENT
} dds_durability_kind_t;

typedef enum {
    DDS_HISTORY_KEEP_LAST = 0,
    DDS_HISTORY_KEEP_ALL
} dds_history_kind_t;

typedef enum {
    DDS_OWNERSHIP_SHARED = 0,
    DDS_OWNERSHIP_EXCLUSIVE
} dds_ownership_kind_t;

typedef enum {
    DDS_LIVELINESS_AUTOMATIC = 0,
    DDS_LIVELINESS_MANUAL_BY_PARTICIPANT,
    DDS_LIVELINESS_MANUAL_BY_TOPIC
} dds_liveliness_kind_t;

typedef enum {
    DDS_DESTINATION_ORDER_BY_RECEPTION_TIMESTAMP = 0,
    DDS_DESTINATION_ORDER_BY_SOURCE_TIMESTAMP
} dds_destination_order_kind_t;

typedef enum {
    DDS_TRANSPORT_UDPv4 = 0,
    DDS_TRANSPORT_UDPv6,
    DDS_TRANSPORT_SHMEM,
    DDS_TRANSPORT_TCPv4,
    DDS_TRANSPORT_TCPv6
} dds_transport_protocol_t;

typedef enum {
    DDS_LOG_FATAL = 0,
    DDS_LOG_ERROR,
    DDS_LOG_WARN,
    DDS_LOG_INFO,
    DDS_LOG_DEBUG,
    DDS_LOG_TRACE
} dds_log_level_t;

typedef enum {
    DDS_LOG_OUTPUT_STDOUT = 0,
    DDS_LOG_OUTPUT_STDERR,
    DDS_LOG_OUTPUT_FILE,
    DDS_LOG_OUTPUT_SYSLOG
} dds_log_output_t;

typedef enum {
    DDS_AUTH_NONE = 0,
    DDS_AUTH_PKI_DH,
    DDS_AUTH_PSK
} dds_authentication_kind_t;

typedef enum {
    DDS_ACCESS_NONE = 0,
    DDS_ACCESS_PERMISSIONS
} dds_access_control_kind_t;

typedef enum {
    DDS_CRYPTO_NONE = 0,
    DDS_CRYPTO_AES_GCM_GMAC
} dds_encryption_kind_t;

/* ============================================================================
 * Duration Type
 * ============================================================================ */
typedef struct {
    int64_t sec;
    uint32_t nanosec;
    bool infinite;
} dds_duration_t;

/* ============================================================================
 * QoS Policy Structures
 * ============================================================================ */

typedef struct {
    dds_reliability_kind_t kind;
    dds_duration_t max_blocking_time;
} dds_reliability_qos_t;

typedef struct {
    dds_durability_kind_t kind;
    dds_duration_t service_cleanup_delay;
} dds_durability_qos_t;

typedef struct {
    dds_history_kind_t kind;
    int32_t depth;
} dds_history_qos_t;

typedef struct {
    dds_duration_t period;
} dds_deadline_qos_t;

typedef struct {
    dds_duration_t duration;
} dds_latency_budget_qos_t;

typedef struct {
    dds_duration_t duration;
} dds_lifespan_qos_t;

typedef struct {
    dds_ownership_kind_t kind;
} dds_ownership_qos_t;

typedef struct {
    int32_t value;
} dds_ownership_strength_qos_t;

typedef struct {
    dds_liveliness_kind_t kind;
    dds_duration_t lease_duration;
} dds_liveliness_qos_t;

typedef struct {
    dds_destination_order_kind_t kind;
} dds_destination_order_qos_t;

typedef struct {
    int32_t max_samples;
    int32_t max_instances;
    int32_t max_samples_per_instance;
} dds_resource_limits_qos_t;

typedef struct {
    int32_t value;
} dds_transport_priority_qos_t;

typedef struct {
    bool autoenable_created_entities;
} dds_entity_factory_qos_t;

/* Combined QoS Profile */
typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    char description[DDS_CONFIG_MAX_DESCRIPTION_LEN];
    char base_profile[DDS_CONFIG_MAX_NAME_LEN];
    
    dds_reliability_qos_t reliability;
    dds_durability_qos_t durability;
    dds_history_qos_t history;
    dds_deadline_qos_t deadline;
    dds_latency_budget_qos_t latency_budget;
    dds_lifespan_qos_t lifespan;
    dds_ownership_qos_t ownership;
    dds_ownership_strength_qos_t ownership_strength;
    dds_liveliness_qos_t liveliness;
    dds_destination_order_qos_t destination_order;
    dds_resource_limits_qos_t resource_limits;
    dds_transport_priority_qos_t transport_priority;
    dds_entity_factory_qos_t entity_factory;
    
    bool has_reliability;
    bool has_durability;
    bool has_history;
    bool has_deadline;
    bool has_latency_budget;
    bool has_lifespan;
    bool has_ownership;
    bool has_ownership_strength;
    bool has_liveliness;
    bool has_destination_order;
    bool has_resource_limits;
    bool has_transport_priority;
    bool has_entity_factory;
} dds_qos_profile_t;

/* ============================================================================
 * Domain Configuration
 * ============================================================================ */

typedef struct {
    bool enabled;
    dds_transport_protocol_t protocol;
    char **initial_peers;
    size_t initial_peer_count;
    dds_duration_t lease_duration;
    dds_duration_t liveliness_check_interval;
} dds_discovery_config_t;

typedef struct {
    char **unicast_addresses;
    size_t unicast_address_count;
    bool multicast_enabled;
    char multicast_address[64];
} dds_transport_config_t;

typedef struct {
    bool enabled;
    dds_authentication_kind_t auth_kind;
    char identity_ca[512];
    char identity_certificate[512];
    char identity_crl[512];
    char private_key[512];
    dds_access_control_kind_t access_kind;
    char permissions_ca[512];
    char governance[512];
    char permissions[512];
    dds_encryption_kind_t crypto_kind;
    bool encrypt_rtps;
    bool encrypt_payload;
} dds_security_config_t;

typedef struct {
    uint32_t domain_id;
    char name[DDS_CONFIG_MAX_NAME_LEN];
    char description[DDS_CONFIG_MAX_DESCRIPTION_LEN];
    dds_discovery_config_t discovery;
    dds_transport_config_t transport;
    dds_security_config_t security;
} dds_domain_config_t;

/* ============================================================================
 * Topic Configuration
 * ============================================================================ */

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    char type_name[DDS_CONFIG_MAX_NAME_LEN];
    uint32_t domain_id;
    char qos_profile[DDS_CONFIG_MAX_NAME_LEN];
    char **partitions;
    size_t partition_count;
    char content_filter[512];
    dds_duration_t time_based_filter;
    bool has_time_based_filter;
} dds_topic_config_t;

/* ============================================================================
 * Participant Configuration
 * ============================================================================ */

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    uint32_t domain_id;
    char qos_profile[DDS_CONFIG_MAX_NAME_LEN];
    dds_entity_factory_qos_t entity_factory;
} dds_participant_config_t;

/* ============================================================================
 * Data Type Definitions
 * ============================================================================ */

typedef enum {
    DDS_TYPE_STRUCT = 0,
    DDS_TYPE_UNION,
    DDS_TYPE_ENUM,
    DDS_TYPE_TYPEDEF,
    DDS_TYPE_MODULE
} dds_type_kind_t;

typedef enum {
    DDS_BASE_TYPE_SHORT = 0,
    DDS_BASE_TYPE_LONG,
    DDS_BASE_TYPE_USHORT,
    DDS_BASE_TYPE_ULONG,
    DDS_BASE_TYPE_FLOAT,
    DDS_BASE_TYPE_DOUBLE,
    DDS_BASE_TYPE_BOOLEAN,
    DDS_BASE_TYPE_CHAR,
    DDS_BASE_TYPE_OCTET,
    DDS_BASE_TYPE_STRING,
    DDS_BASE_TYPE_WSTRING,
    DDS_BASE_TYPE_LONGLONG,
    DDS_BASE_TYPE_ULONGLONG,
    DDS_BASE_TYPE_LONG_DOUBLE,
    DDS_BASE_TYPE_WCHAR,
    DDS_BASE_TYPE_USER_DEFINED
} dds_base_type_t;

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    char type_name[DDS_CONFIG_MAX_NAME_LEN];
    int32_t array_bounds[8];
    size_t array_dimension;
    bool is_key;
    bool is_optional;
    dds_base_type_t base_type;  /* Added base type for code generation */
} dds_field_t;

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    dds_type_kind_t kind;
    dds_field_t fields[DDS_CONFIG_MAX_FIELDS];
    size_t field_count;
} dds_type_def_t;

/* ============================================================================
 * Logging Configuration
 * ============================================================================ */

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    dds_log_level_t level;
} dds_log_category_t;

typedef struct {
    bool enabled;
    dds_log_level_t level;
    dds_log_output_t output;
    char log_file[512];
    dds_log_category_t *categories;
    size_t category_count;
} dds_logging_config_t;

/* ============================================================================
 * Monitoring Configuration
 * ============================================================================ */

typedef enum {
    DDS_METRIC_THROUGHPUT = 0x01,
    DDS_METRIC_LATENCY = 0x02,
    DDS_METRIC_DISCOVERY = 0x04,
    DDS_METRIC_RESOURCE_USAGE = 0x08
} dds_metric_kind_t;

typedef struct {
    bool enabled;
    bool statistics_enabled;
    dds_duration_t statistics_interval;
    uint32_t metrics;  /* Bitmask of dds_metric_kind_t */
} dds_monitoring_config_t;

/* ============================================================================
 * System Configuration (Root)
 * ============================================================================ */

typedef struct {
    char name[DDS_CONFIG_MAX_NAME_LEN];
    char description[DDS_CONFIG_MAX_DESCRIPTION_LEN];
    char version[32];
} dds_system_info_t;

typedef struct {
    char version[16];
    dds_system_info_t system;
    
    dds_domain_config_t *domains;
    size_t domain_count;
    
    dds_topic_config_t *topics;
    size_t topic_count;
    
    dds_qos_profile_t *qos_profiles;
    size_t qos_profile_count;
    
    dds_participant_config_t *participants;
    size_t participant_count;
    
    dds_type_def_t *types;
    size_t type_count;
    
    dds_logging_config_t logging;
    dds_monitoring_config_t monitoring;
} dds_configuration_t;

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* dds_config_error_string(dds_config_error_t error);
dds_duration_t dds_duration_from_string(const char *str);
const char* dds_qos_reliability_to_string(dds_reliability_kind_t kind);
const char* dds_qos_durability_to_string(dds_durability_kind_t kind);
const char* dds_qos_history_to_string(dds_history_kind_t kind);
const char* dds_qos_ownership_to_string(dds_ownership_kind_t kind);
const char* dds_qos_liveliness_to_string(dds_liveliness_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CONFIG_TYPES_H */
