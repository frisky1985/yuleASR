/**
 * @file rmw_ethdds.h
 * @brief ROS2 RMW (ROS Middleware) Adapter for Automotive Ethernet DDS
 * @version 1.0
 * @date 2026-04-26
 *
 * This file implements the ROS2 rmw interface adapter layer that maps
 * ROS2 operations to the underlying Automotive Ethernet DDS stack.
 * Supports ROS2 Humble/Iron/Jazzy
 */

#ifndef RMW_ETHDDS_H
#define RMW_ETHDDS_H

#include "rmw/types.h"
#include "rmw/init.h"
#include "rmw/init_options.h"
#include "rmw/event.h"
#include "rmw/ret_types.h"
#include "rmw/subscription_options.h"
#include "rmw/publisher_options.h"
#include "rmw/qos_profiles.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RMW Context - Bridges ROS2 Context to DDS Domain Participant
 * ============================================================================ */
typedef struct rmw_ethdds_context_s {
    rmw_context_t base;
    void *dds_participant;           /* dds_domain_participant_t* */
    rmw_init_options_t init_options;
    bool is_shutdown;
    void *graph_guard_condition;
} rmw_ethdds_context_t;

/* ============================================================================
 * RMW Node - Bridges ROS2 Node to DDS Topic Management
 * ============================================================================ */
typedef struct rmw_ethdds_node_s {
    rmw_node_t base;
    void *dds_participant;           /* Reference to context's participant */
    char namespace_[256];
    void *graph_cache;
    bool graph_enabled;
} rmw_ethdds_node_t;

/* ============================================================================
 * RMW Publisher - Bridges ROS2 Publisher to DDS DataWriter
 * ============================================================================ */
typedef struct rmw_ethdds_publisher_s {
    rmw_publisher_t base;
    void *dds_publisher;             /* dds_publisher_t* */
    void *dds_datawriter;            /* dds_data_writer_t* */
    void *dds_topic;                 /* dds_topic_t* */
    rmw_qos_profile_t qos;
    void *type_support;
    bool secoc_enabled;              /* SecOC protection enabled */
    bool e2e_enabled;                /* E2E protection enabled */
} rmw_ethdds_publisher_t;

/* ============================================================================
 * RMW Subscription - Bridges ROS2 Subscription to DDS DataReader
 * ============================================================================ */
typedef struct rmw_ethdds_subscription_s {
    rmw_subscription_t base;
    void *dds_subscriber;            /* dds_subscriber_t* */
    void *dds_datareader;            /* dds_data_reader_t* */
    void *dds_topic;                 /* dds_topic_t* */
    rmw_qos_profile_t qos;
    void *type_support;
    rmw_message_sequence_t *message_sequence;
    bool secoc_enabled;              /* SecOC verification enabled */
    bool e2e_enabled;                /* E2E verification enabled */
} rmw_ethdds_subscription_t;

/* ============================================================================
 * RMW Service/Client - Bridges ROS2 Service/Client to DDS Request/Reply
 * ============================================================================ */
typedef struct rmw_ethdds_service_s {
    rmw_service_t base;
    void *dds_service;
    void *request_reader;
    void *response_writer;
    rmw_qos_profile_t qos;
} rmw_ethdds_service_t;

typedef struct rmw_ethdds_client_s {
    rmw_client_t base;
    void *dds_client;
    void *request_writer;
    void *response_reader;
    rmw_qos_profile_t qos;
    int64_t sequence_number;
} rmw_ethdds_client_t;

/* ============================================================================
 * RMW Guard Condition - Bridges ROS2 GuardCondition to DDS Condition
 * ============================================================================ */
typedef struct rmw_ethdds_guard_condition_s {
    rmw_guard_condition_t base;
    void *dds_guard_condition;
    bool triggered;
} rmw_ethdds_guard_condition_t;

/* ============================================================================
 * RMW WaitSet - Bridges ROS2 WaitSet to DDS WaitSet
 * ============================================================================ */
typedef struct rmw_ethdds_wait_set_s {
    rmw_wait_set_t base;
    void *dds_wait_set;
    size_t max_conditions;
} rmw_ethdds_wait_set_t;

/* ============================================================================
 * Topic Mapping Configuration
 * ============================================================================ */
typedef struct rmw_ethdds_topic_mapping_s {
    char ros_topic_name[256];
    char dds_topic_name[256];
    char ros_type_name[256];
    char dds_type_name[256];
    rmw_qos_profile_t qos;
    bool secoc_enabled;
    bool e2e_enabled;
    uint8_t e2e_profile;             /* E2E_P04/P05/P06/P07 */
    uint32_t domain_id;
} rmw_ethdds_topic_mapping_t;

/* ============================================================================
 * Type Mapping Configuration
 * ============================================================================ */
typedef struct rmw_ethdds_type_mapping_s {
    char ros_type_name[256];
    char dds_type_name[256];
    size_t serialized_size_max;
    void *(*serialize_fn)(const void *, void *, size_t *);
    void *(*deserialize_fn)(const void *, void *, size_t);
} rmw_ethdds_type_mapping_t;

/* ============================================================================
 * Security Configuration
 * ============================================================================ */
typedef struct rmw_ethdds_security_s {
    bool enable_security;
    char identity_ca_cert_path[512];
    char identity_cert_path[512];
    char identity_key_path[512];
    char permissions_ca_cert_path[512];
    char permissions_xml_path[512];
    char governance_xml_path[512];
} rmw_ethdds_security_t;

/* ============================================================================
 * Initialization Functions
 * ============================================================================ */

/**
 * @brief Initialize rmw_ethdds implementation
 * @param options Initialization options
 * @param context Output context
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_init(const rmw_init_options_t *options, rmw_context_t *context);

/**
 * @brief Shutdown rmw_ethdds implementation
 * @param context RMW context
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_shutdown(rmw_context_t *context);

/**
 * @brief Finalize rmw_ethdds context
 * @param context RMW context
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_context_fini(rmw_context_t *context);

/* ============================================================================
 * Node Functions
 * ============================================================================ */

/**
 * @brief Create a ROS2 node backed by DDS participant
 * @param context RMW context
 * @param name Node name
 * @param namespace_ Node namespace
 * @param domain_id DDS domain ID
 * @param security_options Security options
 * @return RMW node handle or NULL on failure
 */
rmw_node_t* rmw_ethdds_create_node(
    rmw_context_t *context,
    const char *name,
    const char *namespace_,
    size_t domain_id,
    const rmw_security_options_t *security_options);

/**
 * @brief Destroy a ROS2 node
 * @param node RMW node handle
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_destroy_node(rmw_node_t *node);

/* ============================================================================
 * Publisher Functions
 * ============================================================================ */

/**
 * @brief Create a ROS2 publisher backed by DDS DataWriter
 * @param node RMW node handle
 * @param type_support ROS2 type support
 * @param topic_name Topic name
 * @param qos QoS profile
 * @param publisher_options Publisher options
 * @return RMW publisher handle or NULL on failure
 */
rmw_publisher_t* rmw_ethdds_create_publisher(
    const rmw_node_t *node,
    const rosidl_message_type_support_t *type_support,
    const char *topic_name,
    const rmw_qos_profile_t *qos,
    const rmw_publisher_options_t *publisher_options);

/**
 * @brief Destroy a ROS2 publisher
 * @param node RMW node handle
 * @param publisher RMW publisher handle
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_destroy_publisher(rmw_node_t *node, rmw_publisher_t *publisher);

/**
 * @brief Publish a message
 * @param publisher RMW publisher handle
 * @param ros_message ROS message
 * @param allocation Allocation (optional)
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_publish(
    const rmw_publisher_t *publisher,
    const void *ros_message,
    rmw_publisher_allocation_t *allocation);

/* ============================================================================
 * Subscription Functions
 * ============================================================================ */

/**
 * @brief Create a ROS2 subscription backed by DDS DataReader
 * @param node RMW node handle
 * @param type_support ROS2 type support
 * @param topic_name Topic name
 * @param qos QoS profile
 * @param subscription_options Subscription options
 * @return RMW subscription handle or NULL on failure
 */
rmw_subscription_t* rmw_ethdds_create_subscription(
    const rmw_node_t *node,
    const rosidl_message_type_support_t *type_support,
    const char *topic_name,
    const rmw_qos_profile_t *qos,
    const rmw_subscription_options_t *subscription_options);

/**
 * @brief Destroy a ROS2 subscription
 * @param node RMW node handle
 * @param subscription RMW subscription handle
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_destroy_subscription(rmw_node_t *node, rmw_subscription_t *subscription);

/**
 * @brief Take a message from subscription
 * @param subscription RMW subscription handle
 * @param ros_message Output ROS message
 * @param message_info Output message info
 * @param allocation Allocation (optional)
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_take(
    const rmw_subscription_t *subscription,
    void *ros_message,
    bool *taken,
    rmw_subscription_allocation_t *allocation);

/**
 * @brief Take a message with info from subscription
 * @param subscription RMW subscription handle
 * @param ros_message Output ROS message
 * @param message_info Output message info
 * @param allocation Allocation (optional)
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_take_with_info(
    const rmw_subscription_t *subscription,
    void *ros_message,
    bool *taken,
    rmw_message_info_t *message_info,
    rmw_subscription_allocation_t *allocation);

/* ============================================================================
 * Service/Client Functions
 * ============================================================================ */

/**
 * @brief Create a ROS2 service backed by DDS Request/Reply
 * @param node RMW node handle
 * @param type_support ROS2 service type support
 * @param service_name Service name
 * @param qos QoS profile
 * @return RMW service handle or NULL on failure
 */
rmw_service_t* rmw_ethdds_create_service(
    const rmw_node_t *node,
    const rosidl_service_type_support_t *type_support,
    const char *service_name,
    const rmw_qos_profile_t *qos);

/**
 * @brief Destroy a ROS2 service
 * @param node RMW node handle
 * @param service RMW service handle
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_destroy_service(rmw_node_t *node, rmw_service_t *service);

/**
 * @brief Create a ROS2 client backed by DDS Request/Reply
 * @param node RMW node handle
 * @param type_support ROS2 client type support
 * @param service_name Service name
 * @param qos QoS profile
 * @return RMW client handle or NULL on failure
 */
rmw_client_t* rmw_ethdds_create_client(
    const rmw_node_t *node,
    const rosidl_service_type_support_t *type_support,
    const char *service_name,
    const rmw_qos_profile_t *qos);

/**
 * @brief Destroy a ROS2 client
 * @param node RMW node handle
 * @param client RMW client handle
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_destroy_client(rmw_node_t *node, rmw_client_t *client);

/* ============================================================================
 * WaitSet Functions
 * ============================================================================ */

/**
 * @brief Create a WaitSet
 * @param context RMW context
 * @param max_conditions Maximum conditions
 * @return RMW wait set handle or NULL on failure
 */
rmw_wait_set_t* rmw_ethdds_create_wait_set(rmw_context_t *context, size_t max_conditions);

/**
 * @brief Destroy a WaitSet
 * @param wait_set RMW wait set handle
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_destroy_wait_set(rmw_wait_set_t *wait_set);

/**
 * @brief Wait on conditions
 * @param subscriptions Subscription array
 * @param guard_conditions Guard condition array
 * @param services Service array
 * @param clients Client array
 * @param events Event array
 * @param wait_set Wait set
 * @param wait_timeout Timeout (NULL for infinite)
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_wait(
    rmw_subscriptions_t *subscriptions,
    rmw_guard_conditions_t *guard_conditions,
    rmw_services_t *services,
    rmw_clients_t *clients,
    rmw_events_t *events,
    rmw_wait_set_t *wait_set,
    const rmw_time_t *wait_timeout);

/* ============================================================================
 * QoS Mapping Functions
 * ============================================================================ */

/**
 * @brief Map ROS2 QoS to DDS QoS
 * @param ros_qos ROS2 QoS profile
 * @param dds_qos Output DDS QoS structure
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_qos_ros_to_dds(
    const rmw_qos_profile_t *ros_qos,
    void *dds_qos);

/**
 * @brief Map DDS QoS to ROS2 QoS
 * @param dds_qos DDS QoS structure
 * @param ros_qos Output ROS2 QoS profile
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_qos_dds_to_ros(
    const void *dds_qos,
    rmw_qos_profile_t *ros_qos);

/* ============================================================================
 * Topic Mapping Functions
 * ============================================================================ */

/**
 * @brief Convert ROS2 topic name to DDS topic name
 * @param ros_topic_name ROS2 topic name
 * @param dds_topic_name Output DDS topic name buffer
 * @param buffer_size Buffer size
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_topic_ros_to_dds(
    const char *ros_topic_name,
    char *dds_topic_name,
    size_t buffer_size);

/**
 * @brief Convert DDS topic name to ROS2 topic name
 * @param dds_topic_name DDS topic name
 * @param ros_topic_name Output ROS2 topic name buffer
 * @param buffer_size Buffer size
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_topic_dds_to_ros(
    const char *dds_topic_name,
    char *ros_topic_name,
    size_t buffer_size);

/* ============================================================================
 * Security Functions
 * ============================================================================ */

/**
 * @brief Configure security for DDS participant
 * @param participant DDS participant handle
 * @param security_config Security configuration
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_configure_security(
    void *participant,
    const rmw_ethdds_security_t *security_config);

/**
 * @brief Enable SecOC for publisher/subscription
 * @param entity Publisher or subscription handle
 * @param enable Enable flag
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_enable_secoc(void *entity, bool enable);

/**
 * @brief Enable E2E protection for publisher/subscription
 * @param entity Publisher or subscription handle
 * @param enable Enable flag
 * @param profile E2E profile
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_enable_e2e(void *entity, bool enable, uint8_t profile);

/* ============================================================================
 * Discovery Functions
 * ============================================================================ */

/**
 * @brief Get graph information (nodes, topics, services)
 * @param node RMW node handle
 * @param graph_info Output graph information
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_get_graph_info(
    const rmw_node_t *node,
    rmw_topic_names_and_types_t *topic_names_and_types);

/**
 * @brief Count publishers for a topic
 * @param node RMW node handle
 * @param topic_name Topic name
 * @param count Output count
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_count_publishers(
    const rmw_node_t *node,
    const char *topic_name,
    size_t *count);

/**
 * @brief Count subscriptions for a topic
 * @param node RMW node handle
 * @param topic_name Topic name
 * @param count Output count
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_count_subscribers(
    const rmw_node_t *node,
    const char *topic_name,
    size_t *count);

/* ============================================================================
 * Logging and Debugging
 * ============================================================================ */

/**
 * @brief Set logging level for rmw_ethdds
 * @param level Log level (0=error, 1=warn, 2=info, 3=debug)
 */
void rmw_ethdds_set_log_level(int level);

/**
 * @brief Get implementation identifier
 * @return Implementation identifier string
 */
const char* rmw_ethdds_get_implementation_identifier(void);

#ifdef __cplusplus
}
#endif

#endif /* RMW_ETHDDS_H */
