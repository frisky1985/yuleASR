/**
 * @file types.h
 * @brief Minimal ROS2 RMW types for Automotive Ethernet DDS
 * @note This is a simplified version for integration without full ROS2 installation
 */

#ifndef RMW_TYPES_H
#define RMW_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Basic RMW Types
 * ============================================================================ */

typedef struct rmw_node_s rmw_node_t;
typedef struct rmw_publisher_s rmw_publisher_t;
typedef struct rmw_subscription_s rmw_subscription_t;
typedef struct rmw_service_s rmw_service_t;
typedef struct rmw_client_s rmw_client_t;
typedef struct rmw_guard_condition_s rmw_guard_condition_t;
typedef struct rmw_wait_set_s rmw_wait_set_t;
typedef struct rmw_context_s rmw_context_t;
typedef struct rmw_event_s rmw_event_t;

typedef struct rmw_init_options_s rmw_init_options_t;
typedef struct rmw_publisher_options_s rmw_publisher_options_t;
typedef struct rmw_subscription_options_s rmw_subscription_options_t;

/* ============================================================================
 * Return Types
 * ============================================================================ */

typedef enum rmw_ret_e {
    RMW_RET_OK = 0,
    RMW_RET_ERROR = 1,
    RMW_RET_TIMEOUT = 2,
    RMW_RET_BAD_ALLOC = 10,
    RMW_RET_INVALID_ARGUMENT = 11,
    RMW_RET_INCORRECT_RMW_IMPLEMENTATION = 12,
    RMW_RET_TOPIC_NAME_INVALID = 13,
    RMW_RET_UNSUPPORTED = 14
} rmw_ret_t;

/* ============================================================================
 * QoS Types
 * ============================================================================ */

typedef enum rmw_qos_reliability_policy_e {
    RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT = 0,
    RMW_QOS_POLICY_RELIABILITY_RELIABLE = 1,
    RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT = 2,
    RMW_QOS_POLICY_RELIABILITY_UNKNOWN = 3
} rmw_qos_reliability_policy_t;

typedef enum rmw_qos_durability_policy_e {
    RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT = 0,
    RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL = 1,
    RMW_QOS_POLICY_DURABILITY_VOLATILE = 2,
    RMW_QOS_POLICY_DURABILITY_UNKNOWN = 3
} rmw_qos_durability_policy_t;

typedef enum rmw_qos_history_policy_e {
    RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT = 0,
    RMW_QOS_POLICY_HISTORY_KEEP_LAST = 1,
    RMW_QOS_POLICY_HISTORY_KEEP_ALL = 2,
    RMW_QOS_POLICY_HISTORY_UNKNOWN = 3
} rmw_qos_history_policy_t;

typedef enum rmw_qos_liveliness_policy_e {
    RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT = 0,
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC = 1,
    RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC = 2,
    RMW_QOS_POLICY_LIVELINESS_UNKNOWN = 3
} rmw_qos_liveliness_policy_t;

typedef struct rmw_time_s {
    int64_t sec;
    uint64_t nsec;
} rmw_time_t;

typedef struct rmw_qos_profile_s {
    enum rmw_qos_history_policy_e history;
    size_t depth;
    enum rmw_qos_reliability_policy_e reliability;
    enum rmw_qos_durability_policy_e durability;
    struct rmw_time_s deadline;
    struct rmw_time_s lifespan;
    enum rmw_qos_liveliness_policy_e liveliness;
    struct rmw_time_s liveliness_lease_duration;
    bool avoid_ros_namespace_conventions;
} rmw_qos_profile_t;

/* ============================================================================
 * Type Support Types
 * ============================================================================ */

typedef struct rosidl_message_type_support_s rosidl_message_type_support_t;
typedef struct rosidl_service_type_support_s rosidl_service_type_support_t;

/* ============================================================================
 * Node Types
 * ============================================================================ */

struct rmw_node_s {
    const char * implementation_identifier;
    void * data;
    const char * name;
    const char * namespace_;
    rmw_context_t * context;
};

/* ============================================================================
 * Publisher Types
 * ============================================================================ */

struct rmw_publisher_s {
    const char * implementation_identifier;
    void * data;
    const char * topic_name;
    const rosidl_message_type_support_t * type_support;
};

struct rmw_publisher_options_s {
    void * rmw_specific_publisher_payload;
};

typedef struct rmw_publisher_allocation_s rmw_publisher_allocation_t;

/* ============================================================================
 * Subscription Types
 * ============================================================================ */

struct rmw_subscription_s {
    const char * implementation_identifier;
    void * data;
    const char * topic_name;
    const rosidl_message_type_support_t * type_support;
};

struct rmw_subscription_options_s {
    bool ignore_local_publications;
    void * rmw_specific_subscription_payload;
};

typedef struct rmw_subscription_allocation_s rmw_subscription_allocation_t;

/* ============================================================================
 * Service Types
 * ============================================================================ */

struct rmw_service_s {
    const char * implementation_identifier;
    void * data;
};

struct rmw_client_s {
    const char * implementation_identifier;
    void * data;
};

/* ============================================================================
 * WaitSet Types
 * ============================================================================ */

struct rmw_wait_set_s {
    const char * implementation_identifier;
    void * data;
};

typedef struct rmw_subscriptions_s {
    size_t subscriber_count;
    void ** subscribers;
} rmw_subscriptions_t;

typedef struct rmw_guard_conditions_s {
    size_t guard_condition_count;
    void ** guard_conditions;
} rmw_guard_conditions_t;

typedef struct rmw_services_s {
    size_t service_count;
    void ** services;
} rmw_services_t;

typedef struct rmw_clients_s {
    size_t client_count;
    void ** clients;
} rmw_clients_t;

typedef struct rmw_events_s {
    size_t event_count;
    void ** events;
} rmw_events_t;

/* ============================================================================
 * Message Types
 * ============================================================================ */

typedef struct rmw_message_info_s {
    int64_t source_timestamp;
    int64_t received_timestamp;
    uint64_t publication_sequence_number;
    uint64_t reception_sequence_number;
    bool from_intra_process;
} rmw_message_info_t;

typedef struct rmw_message_sequence_s {
    void * messages;
    size_t size;
} rmw_message_sequence_t;

/* ============================================================================
 * Init Types
 * ============================================================================ */

struct rmw_init_options_s {
    size_t domain_id;
    void * allocator;
    void * localhost_only;
    void * security_options;
    void * enclave;
};

struct rmw_context_s {
    const char * implementation_identifier;
    void * impl;
    void * options;
};

/* ============================================================================
 * Security Types
 * ============================================================================ */

typedef struct rmw_security_options_s {
    void * security_root_path;
    bool enforce_security;
} rmw_security_options_t;

/* ============================================================================
 * Discovery Types
 * ============================================================================ */

typedef struct rmw_topic_names_and_types_s {
    size_t topic_count;
    char ** topic_names;
    char ** type_names;
} rmw_topic_names_and_types_t;

/* ============================================================================
 * Callback Types
 * ============================================================================ */

typedef void (* rmw_gid_t)(void);
typedef void (* rmw_data_callback_t)(void * user_data);

/* ============================================================================
 * Allocator Functions (Stubs)
 * ============================================================================ */

static inline void* rmw_get_default_allocator(void) {
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* RMW_TYPES_H */
