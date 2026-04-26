/**
 * @file rmw_security_bridge.c
 * @brief ROS2-DDS Security Bridge Implementation
 * @version 1.0
 * @date 2026-04-26
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rmw_security_bridge.h"

/* ============================================================================
 * SecOC Bridge Implementation
 * ============================================================================ */

int rmw_secoc_bridge_init(
    rmw_security_bridge_t *bridge,
    const rmw_secoc_config_t *config)
{
    if (bridge == NULL || config == NULL) {
        return -1;
    }

    bridge->secoc = *config;
    bridge->secoc.enabled = true;

    /* Initialize SecOC core if needed */
    SecOC_ConfigType secoc_config;
    memset(&secoc_config, 0, sizeof(secoc_config));
    secoc_config.freshnessValueLength = config->freshness_value_length;
    secoc_config.authInfoLength = config->auth_info_length;

    SecOC_Init(&secoc_config);

    return 0;
}

void rmw_secoc_bridge_fini(rmw_security_bridge_t *bridge)
{
    if (bridge == NULL) {
        return;
    }

    SecOC_DeInit();
    bridge->secoc.enabled = false;
}

int rmw_secoc_protect_message(
    rmw_security_bridge_t *bridge,
    const void *ros_message,
    uint8_t *protected_buffer,
    size_t buffer_size,
    size_t *protected_size)
{
    if (bridge == NULL || ros_message == NULL || protected_buffer == NULL ||
        protected_size == NULL || !bridge->secoc.enabled) {
        return -1;
    }

    /* Serialize ROS message */
    size_t msg_size = 256; /* Placeholder - should get from type support */

    /* Call SecOC to add authentication */
    PduInfoType pdu;
    pdu.SduDataPtr = (uint8_t *)ros_message;
    pdu.SduLength = (uint16_t)msg_size;
    pdu.MetaDataPtr = NULL;

    PduInfoType secured_pdu;
    secured_pdu.SduDataPtr = protected_buffer;
    secured_pdu.SduLength = (uint16_t)buffer_size;
    secured_pdu.MetaDataPtr = NULL;

    Std_ReturnType result = SecOC_IfTransmit(0, &pdu);
    if (result != E_OK) {
        return -1;
    }

    *protected_size = secured_pdu.SduLength;
    return 0;
}

int rmw_secoc_verify_message(
    rmw_security_bridge_t *bridge,
    const uint8_t *protected_buffer,
    size_t protected_size,
    void *ros_message,
    size_t *message_size)
{
    if (bridge == NULL || protected_buffer == NULL || ros_message == NULL ||
        message_size == NULL || !bridge->secoc.enabled) {
        return -1;
    }

    /* Call SecOC to verify and strip authentication */
    PduInfoType secured_pdu;
    secured_pdu.SduDataPtr = (uint8_t *)protected_buffer;
    secured_pdu.SduLength = (uint16_t)protected_size;
    secured_pdu.MetaDataPtr = NULL;

    PduInfoType pdu;
    pdu.SduDataPtr = (uint8_t *)ros_message;
    pdu.SduLength = 256; /* Max expected size */
    pdu.MetaDataPtr = NULL;

    SecOC_VerificationResultType verify_result;
    Std_ReturnType result = SecOC_Verify(0, &secured_pdu, &verify_result);
    if (result != E_OK) {
        return -1;
    }

    *message_size = pdu.SduLength;
    return 0;
}

void rmw_secoc_update_freshness(rmw_security_bridge_t *bridge, uint32_t new_counter)
{
    if (bridge == NULL) {
        return;
    }
    bridge->secoc.freshness_counter = new_counter;
}

/* ============================================================================
 * E2E Bridge Implementation
 * ============================================================================ */

int rmw_e2e_bridge_init(
    rmw_security_bridge_t *bridge,
    const rmw_e2e_config_t *config)
{
    if (bridge == NULL || config == NULL) {
        return -1;
    }

    bridge->e2e = *config;
    bridge->e2e.enabled = true;

    /* Initialize E2E protection */
    E2E_P_ConfigType e2e_config;
    memset(&e2e_config, 0, sizeof(e2e_config));
    e2e_config.dataID = config->data_id;
    e2e_config.sourceID = config->source_id;
    e2e_config.dataLength = config->data_length;
    e2e_config.profile = (E2E_ProfileType)config->profile;
    e2e_config.maxDeltaCounter = config->max_delta_counter;

    E2E_P_Init(&e2e_config);

    return 0;
}

void rmw_e2e_bridge_fini(rmw_security_bridge_t *bridge)
{
    if (bridge == NULL) {
        return;
    }

    E2E_P_DeInit();
    bridge->e2e.enabled = false;
}

int rmw_e2e_protect_message(
    rmw_security_bridge_t *bridge,
    const void *ros_message,
    uint8_t *protected_buffer,
    size_t buffer_size,
    size_t *protected_size)
{
    if (bridge == NULL || ros_message == NULL || protected_buffer == NULL ||
        protected_size == NULL || !bridge->e2e.enabled) {
        return -1;
    }

    /* Copy original message */
    size_t msg_size = bridge->e2e.data_length;
    if (msg_size + E2E_P_HEADER_SIZE > buffer_size) {
        return -1;
    }

    memcpy(protected_buffer + E2E_P_HEADER_SIZE, ros_message, msg_size);

    /* Protect with E2E */
    E2E_P_checkStatusType status = E2E_P_Protect(
        bridge->e2e.data_id,
        protected_buffer + E2E_P_HEADER_SIZE,
        (uint16_t)msg_size,
        protected_buffer);

    if (status != E2E_P_OK) {
        return -1;
    }

    *protected_size = msg_size + E2E_P_HEADER_SIZE;
    return 0;
}

int rmw_e2e_verify_message(
    rmw_security_bridge_t *bridge,
    const uint8_t *protected_buffer,
    size_t protected_size,
    void *ros_message,
    size_t *message_size,
    E2E_P_checkStatusType *status)
{
    if (bridge == NULL || protected_buffer == NULL || ros_message == NULL ||
        message_size == NULL || status == NULL || !bridge->e2e.enabled) {
        return -1;
    }

    if (protected_size < E2E_P_HEADER_SIZE) {
        return -1;
    }

    /* Verify E2E protection */
    size_t data_size = protected_size - E2E_P_HEADER_SIZE;

    *status = E2E_P_Check(
        bridge->e2e.data_id,
        protected_buffer,
        (uint16_t)protected_size,
        protected_buffer + E2E_P_HEADER_SIZE,
        (uint16_t)data_size);

    if (*status == E2E_P_OK || *status == E2E_P_OKSOMELOST) {
        memcpy(ros_message, protected_buffer + E2E_P_HEADER_SIZE, data_size);
        *message_size = data_size;
        return 0;
    }

    return -1;
}

const char* rmw_e2e_status_to_string(E2E_P_checkStatusType status)
{
    switch (status) {
        case E2E_P_OK: return "E2E_P_OK";
        case E2E_P_OKSOMELOST: return "E2E_P_OKSOMELOST";
        case E2E_P_WRONGCRC: return "E2E_P_WRONGCRC";
        case E2E_P_SYNC: return "E2E_P_SYNC";
        case E2E_P_NONEWDATA: return "E2E_P_NONEWDATA";
        case E2E_P_REPEATED: return "E2E_P_REPEATED";
        case E2E_P_WRONGSEQUENCE: return "E2E_P_WRONGSEQUENCE";
        default: return "E2E_P_UNKNOWN";
    }
}

/* ============================================================================
 * Combined Security Implementation
 * ============================================================================ */

int rmw_security_bridge_init(
    rmw_security_bridge_t *bridge,
    const rmw_secoc_config_t *secoc_config,
    const rmw_e2e_config_t *e2e_config)
{
    if (bridge == NULL) {
        return -1;
    }

    memset(bridge, 0, sizeof(rmw_security_bridge_t));

    if (secoc_config != NULL && secoc_config->enabled) {
        if (rmw_secoc_bridge_init(bridge, secoc_config) != 0) {
            return -1;
        }
    }

    if (e2e_config != NULL && e2e_config->enabled) {
        if (rmw_e2e_bridge_init(bridge, e2e_config) != 0) {
            if (bridge->secoc.enabled) {
                rmw_secoc_bridge_fini(bridge);
            }
            return -1;
        }
    }

    bridge->initialized = true;
    return 0;
}

void rmw_security_bridge_fini(rmw_security_bridge_t *bridge)
{
    if (bridge == NULL) {
        return;
    }

    if (bridge->secoc.enabled) {
        rmw_secoc_bridge_fini(bridge);
    }

    if (bridge->e2e.enabled) {
        rmw_e2e_bridge_fini(bridge);
    }

    bridge->initialized = false;
}

int rmw_security_protect(
    rmw_security_bridge_t *bridge,
    const void *ros_message,
    uint8_t *protected_buffer,
    size_t buffer_size,
    size_t *protected_size)
{
    if (bridge == NULL || !bridge->initialized) {
        return -1;
    }

    uint8_t temp_buffer[4096];
    size_t temp_size = 0;
    const void *current_data = ros_message;
    size_t current_size = 256; /* Should get from type support */

    /* First apply E2E if enabled */
    if (bridge->e2e.enabled) {
        if (rmw_e2e_protect_message(bridge, current_data, temp_buffer,
                                     sizeof(temp_buffer), &temp_size) != 0) {
            return -1;
        }
        current_data = temp_buffer;
        current_size = temp_size;
    }

    /* Then apply SecOC if enabled */
    if (bridge->secoc.enabled) {
        uint8_t secoc_buffer[4096];
        size_t secoc_size = 0;
        if (rmw_secoc_protect_message(bridge, current_data, secoc_buffer,
                                       sizeof(secoc_buffer), &secoc_size) != 0) {
            return -1;
        }
        memcpy(temp_buffer, secoc_buffer, secoc_size);
        temp_size = secoc_size;
        current_data = temp_buffer;
        current_size = temp_size;
    }

    /* Copy to output buffer */
    if (current_size > buffer_size) {
        return -1;
    }
    memcpy(protected_buffer, current_data, current_size);
    *protected_size = current_size;

    return 0;
}

int rmw_security_verify(
    rmw_security_bridge_t *bridge,
    const uint8_t *protected_buffer,
    size_t protected_size,
    void *ros_message,
    size_t *message_size,
    bool *secoc_ok,
    E2E_P_checkStatusType *e2e_status)
{
    if (bridge == NULL || !bridge->initialized || protected_buffer == NULL ||
        ros_message == NULL || message_size == NULL) {
        return -1;
    }

    uint8_t temp_buffer[4096];
    size_t temp_size = protected_size;
    memcpy(temp_buffer, protected_buffer, temp_size);

    *secoc_ok = true;
    *e2e_status = E2E_P_OK;

    /* First verify SecOC if enabled */
    if (bridge->secoc.enabled) {
        size_t msg_size = 0;
        if (rmw_secoc_verify_message(bridge, temp_buffer, temp_size,
                                      temp_buffer, &msg_size) != 0) {
            *secoc_ok = false;
            return -1;
        }
        temp_size = msg_size;
    }

    /* Then verify E2E if enabled */
    if (bridge->e2e.enabled) {
        size_t msg_size = 0;
        if (rmw_e2e_verify_message(bridge, temp_buffer, temp_size,
                                    ros_message, &msg_size, e2e_status) != 0) {
            return -1;
        }
        *message_size = msg_size;
    } else {
        memcpy(ros_message, temp_buffer, temp_size);
        *message_size = temp_size;
    }

    return 0;
}

/* ============================================================================
 * RMW Security Integration
 * ============================================================================ */

rmw_ret_t rmw_ethdds_publisher_enable_security(
    rmw_publisher_t *publisher,
    const rmw_secoc_config_t *secoc_config,
    const rmw_e2e_config_t *e2e_config)
{
    if (publisher == NULL) {
        return RMW_RET_INVALID_ARGUMENT;
    }

    rmw_ethdds_publisher_t *eth_pub = (rmw_ethdds_publisher_t *)publisher;

    rmw_security_bridge_t *bridge = calloc(1, sizeof(rmw_security_bridge_t));
    if (bridge == NULL) {
        return RMW_RET_BAD_ALLOC;
    }

    if (rmw_security_bridge_init(bridge, secoc_config, e2e_config) != 0) {
        free(bridge);
        return RMW_RET_ERROR;
    }

    eth_pub->secoc_enabled = (secoc_config != NULL && secoc_config->enabled);
    eth_pub->e2e_enabled = (e2e_config != NULL && e2e_config->enabled);

    return RMW_RET_OK;
}

rmw_ret_t rmw_ethdds_subscription_enable_security(
    rmw_subscription_t *subscription,
    const rmw_secoc_config_t *secoc_config,
    const rmw_e2e_config_t *e2e_config)
{
    if (subscription == NULL) {
        return RMW_RET_INVALID_ARGUMENT;
    }

    rmw_ethdds_subscription_t *eth_sub = (rmw_ethdds_subscription_t *)subscription;

    rmw_security_bridge_t *bridge = calloc(1, sizeof(rmw_security_bridge_t));
    if (bridge == NULL) {
        return RMW_RET_BAD_ALLOC;
    }

    if (rmw_security_bridge_init(bridge, secoc_config, e2e_config) != 0) {
        free(bridge);
        return RMW_RET_ERROR;
    }

    eth_sub->secoc_enabled = (secoc_config != NULL && secoc_config->enabled);
    eth_sub->e2e_enabled = (e2e_config != NULL && e2e_config->enabled);

    return RMW_RET_OK;
}

rmw_ret_t rmw_ethdds_publish_secure(
    const rmw_publisher_t *publisher,
    const void *ros_message)
{
    if (publisher == NULL || ros_message == NULL) {
        return RMW_RET_INVALID_ARGUMENT;
    }

    /* This is a stub - full implementation would use security bridge */
    /* For now, just call regular publish */
    return rmw_ethdds_publish(publisher, ros_message, NULL);
}

rmw_ret_t rmw_ethdds_take_secure(
    const rmw_subscription_t *subscription,
    void *ros_message,
    bool *taken,
    bool *secoc_ok,
    E2E_P_checkStatusType *e2e_status)
{
    if (subscription == NULL || ros_message == NULL || taken == NULL) {
        return RMW_RET_INVALID_ARGUMENT;
    }

    /* This is a stub - full implementation would verify security */
    /* For now, just call regular take */
    return rmw_ethdds_take(subscription, ros_message, taken, NULL);
}
