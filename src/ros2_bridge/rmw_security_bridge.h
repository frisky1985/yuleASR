/**
 * @file rmw_security_bridge.h
 * @brief ROS2-DDS Security Bridge for SecOC and E2E Protection
 * @version 1.0
 * @date 2026-04-26
 *
 * Bridges ROS2 security features with Automotive Ethernet DDS
 * security mechanisms (SecOC, E2E protection).
 */

#ifndef RMW_SECURITY_BRIDGE_H
#define RMW_SECURITY_BRIDGE_H

#include "rmw_ethdds.h"
#include "../crypto_stack/secoc/secoc_core.h"
#include "../autosar/e2e/e2e_protection.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Security Bridge Configuration
 * ============================================================================ */
typedef struct rmw_secoc_config_s {
    bool enabled;
    uint16_t freshness_value_length;    /* Freshness value length in bits */
    uint16_t auth_info_length;          /* Authentication info length in bits */
    uint32_t freshness_counter;         /* Current freshness counter */
    uint32_t freshness_counter_threshold; /* Acceptable counter difference */
    SecOC_VerificationStatusCallout verification_callback;
} rmw_secoc_config_t;

typedef struct rmw_e2e_config_s {
    bool enabled;
    uint8_t profile;                     /* E2E_PROFILE_P04/P05/P06/P07 */
    uint32_t data_id;                    /* Data ID for E2E protection */
    uint32_t source_id;                  /* Source ECU ID */
    uint16_t data_length;                /* Protected data length */
    uint8_t max_delta_counter;           /* Max delta counter for window check */
} rmw_e2e_config_t;

typedef struct rmw_security_bridge_s {
    rmw_secoc_config_t secoc;
    rmw_e2e_config_t e2e;
    void *dds_entity;                    /* Associated DDS entity */
    bool initialized;
} rmw_security_bridge_t;

/* ============================================================================
 * SecOC Bridge Functions
 * ============================================================================ */

/**
 * @brief Initialize SecOC for a ROS2 publisher/subscription
 * @param bridge Security bridge structure
 * @param config SecOC configuration
 * @return 0 on success
 */
int rmw_secoc_bridge_init(
    rmw_security_bridge_t *bridge,
    const rmw_secoc_config_t *config);

/**
 * @brief Finalize SecOC bridge
 * @param bridge Security bridge structure
 */
void rmw_secoc_bridge_fini(rmw_security_bridge_t *bridge);

/**
 * @brief Apply SecOC protection to ROS message before DDS publish
 * @param bridge Security bridge
 * @param ros_message Original ROS message
 * @param protected_buffer Output buffer with SecOC header
 * @param buffer_size Buffer size
 * @param protected_size Output protected data size
 * @return 0 on success
 */
int rmw_secoc_protect_message(
    rmw_security_bridge_t *bridge,
    const void *ros_message,
    uint8_t *protected_buffer,
    size_t buffer_size,
    size_t *protected_size);

/**
 * @brief Verify SecOC protection on received DDS message
 * @param bridge Security bridge
 * @param protected_buffer Received buffer with SecOC header
 * @param protected_size Received data size
 * @param ros_message Output verified ROS message
 * @param message_size Output message size
 * @return 0 on success, -1 on verification failure
 */
int rmw_secoc_verify_message(
    rmw_security_bridge_t *bridge,
    const uint8_t *protected_buffer,
    size_t protected_size,
    void *ros_message,
    size_t *message_size);

/**
 * @brief Update freshness counter for SecOC
 * @param bridge Security bridge
 * @param new_counter New counter value
 */
void rmw_secoc_update_freshness(rmw_security_bridge_t *bridge, uint32_t new_counter);

/* ============================================================================
 * E2E Bridge Functions
 * ============================================================================ */

/**
 * @brief Initialize E2E protection for a ROS2 publisher/subscription
 * @param bridge Security bridge structure
 * @param config E2E configuration
 * @return 0 on success
 */
int rmw_e2e_bridge_init(
    rmw_security_bridge_t *bridge,
    const rmw_e2e_config_t *config);

/**
 * @brief Finalize E2E bridge
 * @param bridge Security bridge structure
 */
void rmw_e2e_bridge_fini(rmw_security_bridge_t *bridge);

/**
 * @brief Apply E2E protection to ROS message before DDS publish
 * @param bridge Security bridge
 * @param ros_message Original ROS message
 * @param protected_buffer Output buffer with E2E header
 * @param buffer_size Buffer size
 * @param protected_size Output protected data size
 * @return 0 on success
 */
int rmw_e2e_protect_message(
    rmw_security_bridge_t *bridge,
    const void *ros_message,
    uint8_t *protected_buffer,
    size_t buffer_size,
    size_t *protected_size);

/**
 * @brief Verify E2E protection on received DDS message
 * @param bridge Security bridge
 * @param protected_buffer Received buffer with E2E header
 * @param protected_size Received data size
 * @param ros_message Output verified ROS message
 * @param message_size Output message size
 * @param status Output E2E check status
 * @return 0 on success, -1 on error
 */
int rmw_e2e_verify_message(
    rmw_security_bridge_t *bridge,
    const uint8_t *protected_buffer,
    size_t protected_size,
    void *ros_message,
    size_t *message_size,
    E2E_P_checkStatusType *status);

/**
 * @brief Get E2E error string from status
 * @param status E2E check status
 * @return Error string
 */
const char* rmw_e2e_status_to_string(E2E_P_checkStatusType status);

/* ============================================================================
 * Combined Security Functions
 * ============================================================================ */

/**
 * @brief Initialize combined security (SecOC + E2E)
 * @param bridge Security bridge
 * @param secoc_config SecOC configuration (can be NULL)
 * @param e2e_config E2E configuration (can be NULL)
 * @return 0 on success
 */
int rmw_security_bridge_init(
    rmw_security_bridge_t *bridge,
    const rmw_secoc_config_t *secoc_config,
    const rmw_e2e_config_t *e2e_config);

/**
 * @brief Finalize security bridge
 * @param bridge Security bridge
 */
void rmw_security_bridge_fini(rmw_security_bridge_t *bridge);

/**
 * @brief Apply combined security protection
 * @param bridge Security bridge
 * @param ros_message Original message
 * @param protected_buffer Output buffer
 * @param buffer_size Buffer size
 * @param protected_size Output size
 * @return 0 on success
 */
int rmw_security_protect(
    rmw_security_bridge_t *bridge,
    const void *ros_message,
    uint8_t *protected_buffer,
    size_t buffer_size,
    size_t *protected_size);

/**
 * @brief Verify combined security protection
 * @param bridge Security bridge
 * @param protected_buffer Protected buffer
 * @param protected_size Protected size
 * @param ros_message Output message
 * @param message_size Output size
 * @param secoc_ok Output SecOC verification result
 * @param e2e_status Output E2E status
 * @return 0 on success
 */
int rmw_security_verify(
    rmw_security_bridge_t *bridge,
    const uint8_t *protected_buffer,
    size_t protected_size,
    void *ros_message,
    size_t *message_size,
    bool *secoc_ok,
    E2E_P_checkStatusType *e2e_status);

/* ============================================================================
 * Security Integration with RMW
 * ============================================================================ */

/**
 * @brief Enable security for RMW publisher
 * @param publisher RMW publisher
 * @param secoc_config SecOC config (NULL to disable)
 * @param e2e_config E2E config (NULL to disable)
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_publisher_enable_security(
    rmw_publisher_t *publisher,
    const rmw_secoc_config_t *secoc_config,
    const rmw_e2e_config_t *e2e_config);

/**
 * @brief Enable security for RMW subscription
 * @param subscription RMW subscription
 * @param secoc_config SecOC config (NULL to disable)
 * @param e2e_config E2E config (NULL to disable)
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_subscription_enable_security(
    rmw_subscription_t *subscription,
    const rmw_secoc_config_t *secoc_config,
    const rmw_e2e_config_t *e2e_config);

/**
 * @brief Publish with security protection
 * @param publisher RMW publisher
 * @param ros_message ROS message
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_publish_secure(
    const rmw_publisher_t *publisher,
    const void *ros_message);

/**
 * @brief Take with security verification
 * @param subscription RMW subscription
 * @param ros_message Output message
 * @param taken Output taken flag
 * @param secoc_ok Output SecOC result
 * @param e2e_status Output E2E status
 * @return RMW_RET_OK on success
 */
rmw_ret_t rmw_ethdds_take_secure(
    const rmw_subscription_t *subscription,
    void *ros_message,
    bool *taken,
    bool *secoc_ok,
    E2E_P_checkStatusType *e2e_status);

#ifdef __cplusplus
}
#endif

#endif /* RMW_SECURITY_BRIDGE_H */
