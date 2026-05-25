/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file SomeIpSd_Cfg.h
 * @brief SOME/IP Service Discovery Configuration Header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * 
 * AUTOSAR R20-11 compliant SOME/IP-SD configuration
 * Service Discovery protocol configuration
 */

#ifndef SOMEIPSD_CFG_H
#define SOMEIPSD_CFG_H

/*==================================================================================================
*                                    AUTOSAR VERSION
==================================================================================================*/
#define SOMEIPSD_CFG_AR_RELEASE_MAJOR_VERSION       4
#define SOMEIPSD_CFG_AR_RELEASE_MINOR_VERSION       0
#define SOMEIPSD_CFG_AR_RELEASE_REVISION_VERSION    3

/*==================================================================================================
*                                    MODULE VERSION
==================================================================================================*/
#define SOMEIPSD_CFG_SW_MAJOR_VERSION               1
#define SOMEIPSD_CFG_SW_MINOR_VERSION               0
#define SOMEIPSD_CFG_SW_PATCH_VERSION               0

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 * @implements SOMEIPSD_DEV_ERROR_DETECT
 */
#define SOMEIPSD_DEV_ERROR_DETECT                   (STD_ON)

/**
 * @brief Version information API enable/disable
 * @implements SOMEIPSD_VERSION_INFO_API
 */
#define SOMEIPSD_VERSION_INFO_API                   (STD_ON)

/**
 * @brief Enable/Disable SOME/IP-SD server functionality (offering services)
 */
#define SOMEIPSD_SERVER_SERVICE_DISCOVERY_ENABLED   (STD_ON)

/**
 * @brief Enable/Disable SOME/IP-SD client functionality (finding services)
 */
#define SOMEIPSD_CLIENT_SERVICE_DISCOVERY_ENABLED   (STD_ON)

/**
 * @brief Enable/Disable event group subscription handling
 */
#define SOMEIPSD_EVENTGROUP_SUBSCRIPTION_ENABLED    (STD_ON)

/**
 * @brief Enable/Disable multicast reception
 */
#define SOMEIPSD_MULTICAST_RECEPTION_ENABLED        (STD_ON)

/**
 * @brief Enable/Disable request response delay
 */
#define SOMEIPSD_REQUEST_RESPONSE_DELAY_ENABLED     (STD_ON)

/*==================================================================================================
*                                    MESSAGE CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum SD message size (bytes)
 */
#define SOMEIPSD_MAX_MESSAGE_SIZE                   (1400U)

/**
 * @brief Maximum number of entries per SD message
 */
#define SOMEIPSD_MAX_ENTRIES_PER_MESSAGE            (16U)

/**
 * @brief Maximum number of options per SD message
 */
#define SOMEIPSD_MAX_OPTIONS_PER_MESSAGE            (32U)

/**
 * @brief SD entry header size (bytes)
 */
#define SOMEIPSD_ENTRY_HEADER_SIZE                  (16U)

/**
 * @brief SD IPv4 endpoint option size (bytes)
 */
#define SOMEIPSD_IPV4_ENDPOINT_OPTION_SIZE          (9U)

/**
 * @brief SD IPv6 endpoint option size (bytes)
 */
#define SOMEIPSD_IPV6_ENDPOINT_OPTION_SIZE          (21U)

/**
 * @brief SD configuration option max size (bytes)
 */
#define SOMEIPSD_CONFIGURATION_OPTION_MAX_SIZE      (256U)

/*==================================================================================================
*                                    SERVICE CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of offered services
 */
#define SOMEIPSD_MAX_OFFERED_SERVICES               (8U)

/**
 * @brief Maximum number of found services
 */
#define SOMEIPSD_MAX_FOUND_SERVICES                 (16U)

/**
 * @brief Maximum number of subscribed event groups
 */
#define SOMEIPSD_MAX_SUBSCRIBED_EVENTGROUPS         (16U)

/**
 * @brief Maximum number of event groups offered per service
 */
#define SOMEIPSD_MAX_EVENTGROUPS_PER_SERVICE        (4U)

/*==================================================================================================
*                                    TIMING CONFIGURATION
==================================================================================================*/

/**
 * @brief Initial delay minimum (milliseconds)
 * Random delay between min and max before first message
 */
#define SOMEIPSD_INITIAL_DELAY_MIN_MS               (50U)

/**
 * @brief Initial delay maximum (milliseconds)
 */
#define SOMEIPSD_INITIAL_DELAY_MAX_MS               (150U)

/**
 * @brief Repetition base delay (milliseconds)
 */
#define SOMEIPSD_REPETITION_BASE_DELAY_MS           (100U)

/**
 * @brief Repetition maximum count
 */
#define SOMEIPSD_REPETITION_MAX_COUNT               (3U)

/**
 * @brief Cyclic offer delay (milliseconds)
 * Period for sending cyclic offer messages
 */
#define SOMEIPSD_CYCLIC_OFFER_DELAY_MS              (3000U)

/**
 * @brief Cyclic find delay (milliseconds)
 * Period for sending cyclic find messages
 */
#define SOMEIPSD_CYCLIC_FIND_DELAY_MS               (3000U)

/**
 * @brief Service offer time-to-live (milliseconds)
 */
#define SOMEIPSD_OFFER_TTL_MS                       (3000U)

/**
 * @brief Subscription time-to-live (milliseconds)
 */
#define SOMEIPSD_SUBSCRIPTION_TTL_MS                (3000U)

/**
 * @brief TTL factor for service offers
 */
#define SOMEIPSD_TTL_FACTOR_OFFER                   (3U)

/**
 * @brief TTL factor for subscriptions
 */
#define SOMEIPSD_TTL_FACTOR_SUBSCRIBE               (3U)

/*==================================================================================================
*                                    REQUEST/RESPONSE DELAY
==================================================================================================*/

/**
 * @brief Request response delay minimum (milliseconds)
 * Used to prevent message storms
 */
#define SOMEIPSD_REQUEST_RESPONSE_DELAY_MIN_MS      (10U)

/**
 * @brief Request response delay maximum (milliseconds)
 */
#define SOMEIPSD_REQUEST_RESPONSE_DELAY_MAX_MS      (100U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIODS
==================================================================================================*/

/**
 * @brief Main function period in milliseconds
 */
#define SOMEIPSD_MAIN_FUNCTION_PERIOD_MS            (10U)

/**
 * @brief TTL expiration check period (milliseconds)
 */
#define SOMEIPSD_TTL_CHECK_PERIOD_MS                (100U)

/**
 * @brief Offer cycle period (milliseconds)
 */
#define SOMEIPSD_OFFER_CYCLE_PERIOD_MS              (1000U)

/*==================================================================================================
*                                    NETWORK CONFIGURATION
==================================================================================================*/

/**
 * @brief SOME/IP-SD multicast IPv4 address
 * Default: 224.244.224.245
 */
#define SOMEIPSD_MULTICAST_IP_ADDRESS               (0xE0F4E0F5UL)  /* 224.244.224.245 */

/**
 * @brief SOME/IP-SD multicast port number
 */
#define SOMEIPSD_MULTICAST_PORT                     ((uint16)30490U)

/**
 * @brief SOME/IP-SD unicast port number
 */
#define SOMEIPSD_UNICAST_PORT                       ((uint16)30500U)

/**
 * @brief Local IPv4 address (to be configured per ECU)
 */
#define SOMEIPSD_LOCAL_IP_ADDRESS                   (0xC0A8010AUL)  /* 192.168.1.10 */

/**
 * @brief Subnet mask
 */
#define SOMEIPSD_SUBNET_MASK                        (0xFFFFFF00UL)  /* 255.255.255.0 */

/**
 * @brief Default transport protocol
 */
#define SOMEIPSD_DEFAULT_PROTOCOL                   (SOMEIPSD_PROTO_UDP)

/**
 * @brief SD protocol type
 */
#define SOMEIPSD_PROTOCOL                           (SOMEIPSD_PROTO_UDP)

/*==================================================================================================
*                                    PROTOCOL TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief TCP Protocol type
 */
#define SOMEIPSD_PROTO_TCP                          ((uint8)0x06U)

/**
 * @brief UDP Protocol type
 */
#define SOMEIPSD_PROTO_UDP                          ((uint8)0x11U)

/*==================================================================================================
*                                    ENTRY TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Find Service entry type
 */
#define SOMEIPSD_ENTRY_TYPE_FIND_SERVICE            ((uint8)0x00U)

/**
 * @brief Offer Service entry type
 */
#define SOMEIPSD_ENTRY_TYPE_OFFER_SERVICE           ((uint8)0x01U)

/**
 * @brief Subscribe Event Group entry type
 */
#define SOMEIPSD_ENTRY_TYPE_SUBSCRIBE               ((uint8)0x06U)

/**
 * @brief Subscribe Event Group ACK entry type
 */
#define SOMEIPSD_ENTRY_TYPE_SUBSCRIBE_ACK           ((uint8)0x07U)

/**
 * @brief Stop Offer Service entry type (same as offer with TTL=0)
 */
#define SOMEIPSD_ENTRY_TYPE_STOP_OFFER              ((uint8)0x01U)

/**
 * @brief Stop Subscribe Event Group entry type (same as subscribe with TTL=0)
 */
#define SOMEIPSD_ENTRY_TYPE_STOP_SUBSCRIBE          ((uint8)0x06U)

/*==================================================================================================
*                                    OPTION TYPE DEFINITIONS
==================================================================================================*/

/**
 * @brief Configuration option type
 */
#define SOMEIPSD_OPTION_TYPE_CONFIGURATION          ((uint8)0x01U)

/**
 * @brief Load balancing option type
 */
#define SOMEIPSD_OPTION_TYPE_LOAD_BALANCING         ((uint8)0x02U)

/**
 * @brief IPv4 Endpoint option type
 */
#define SOMEIPSD_OPTION_TYPE_IPV4_ENDPOINT          ((uint8)0x04U)

/**
 * @brief IPv4 Multicast option type
 */
#define SOMEIPSD_OPTION_TYPE_IPV4_MULTICAST         ((uint8)0x14U)

/**
 * @brief IPv6 Endpoint option type
 */
#define SOMEIPSD_OPTION_TYPE_IPV6_ENDPOINT          ((uint8)0x06U)

/**
 * @brief IPv6 Multicast option type
 */
#define SOMEIPSD_OPTION_TYPE_IPV6_MULTICAST         ((uint8)0x16U)

/**
 * @brief Option configuration length
 */
#define SOMEIPSD_OPTION_CONFIG_LENGTH               (2U)

/*==================================================================================================
*                                    FLAG DEFINITIONS
==================================================================================================*/

/**
 * @brief Reboot flag mask
 */
#define SOMEIPSD_FLAG_REBOOT                        ((uint8)0x80U)

/**
 * @brief Unicast flag mask
 */
#define SOMEIPSD_FLAG_UNICAST                       ((uint8)0x40U)

/**
 * @brief Explicit initial data control flag mask
 */
#define SOMEIPSD_FLAG_EXPLICIT_INIT_DATA            ((uint8)0x20U)

/**
 * @brief Default flags (reboot set)
 */
#define SOMEIPSD_FLAGS_DEFAULT                      (SOMEIPSD_FLAG_REBOOT)

/**
 * @brief Flags after reboot (reboot cleared)
 */
#define SOMEIPSD_FLAGS_RUNNING                      ((uint8)0x00U)

/*==================================================================================================
*                                    INSTANCE CONFIGURATION
==================================================================================================*/

/**
 * @brief Default major version
 */
#define SOMEIPSD_DEFAULT_MAJOR_VERSION              ((uint8)1U)

/**
 * @brief Default minor version
 */
#define SOMEIPSD_DEFAULT_MINOR_VERSION              ((uint32)0UL)

/**
 * @brief Wildcard instance ID (find any instance)
 */
#define SOMEIPSD_INSTANCE_ID_WILDCARD               ((uint16)0xFFFFU)

/**
 * @brief Wildcard major version
 */
#define SOMEIPSD_MAJOR_VERSION_WILDCARD             ((uint8)0xFFU)

/**
 * @brief Wildcard minor version
 */
#define SOMEIPSD_MINOR_VERSION_WILDCARD             ((uint32)0xFFFFFFFFUL)

/*==================================================================================================
*                                    SERVICE INSTANCE CONFIGURATION
==================================================================================================*/

/* Service Instance IDs */
#define SOMEIPSD_INSTANCE_ID_VEHICLE_01             ((uint16)0x0001U)
#define SOMEIPSD_INSTANCE_ID_VEHICLE_02             ((uint16)0x0002U)
#define SOMEIPSD_INSTANCE_ID_ENGINE_01              ((uint16)0x0001U)
#define SOMEIPSD_INSTANCE_ID_ENGINE_02              ((uint16)0x0002U)
#define SOMEIPSD_INSTANCE_ID_CHASSIS_01             ((uint16)0x0001U)
#define SOMEIPSD_INSTANCE_ID_BODY_01                ((uint16)0x0001U)

/*==================================================================================================
*                                    EVENT GROUP CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of event groups per service
 */
#define SOMEIPSD_MAX_EVENTGROUPS                    (4U)

/**
 * @brief Event Group: Not Available
 */
#define SOMEIPSD_EVENTGROUP_NOT_AVAILABLE           ((uint16)0x0000U)

/* Vehicle Service Event Groups */
#define SOMEIPSD_EVENTGROUP_VEHICLE_STATUS          ((uint16)0x0001U)
#define SOMEIPSD_EVENTGROUP_VEHICLE_POSITION        ((uint16)0x0002U)
#define SOMEIPSD_EVENTGROUP_VEHICLE_DIAGNOSTIC      ((uint16)0x0003U)

/* Engine Service Event Groups */
#define SOMEIPSD_EVENTGROUP_ENGINE_STATUS           ((uint16)0x0001U)
#define SOMEIPSD_EVENTGROUP_ENGINE_DIAGNOSTIC       ((uint16)0x0002U)

/* Chassis Service Event Groups */
#define SOMEIPSD_EVENTGROUP_CHASSIS_STATUS          ((uint16)0x0001U)

/* Body Service Event Groups */
#define SOMEIPSD_EVENTGROUP_BODY_STATUS             ((uint16)0x0001U)
#define SOMEIPSD_EVENTGROUP_BODY_COMFORT            ((uint16)0x0002U)

/*==================================================================================================
*                                    SERVICE SPECIFIC CONFIGURATION
==================================================================================================*/

/**
 * @brief Number of configured offered services
 */
#define SOMEIPSD_NUM_OFFERED_SERVICES               (4U)

/**
 * @brief Number of configured required services
 */
#define SOMEIPSD_NUM_REQUIRED_SERVICES              (4U)

/**
 * @brief Number of configured event groups
 */
#define SOMEIPSD_NUM_CONFIGURED_EVENTGROUPS         (8U)

/**
 * @brief Number of configured subscriptions
 */
#define SOMEIPSD_NUM_CONFIGURED_SUBSCRIPTIONS       (8U)

/*==================================================================================================
*                                    RETRY CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of find service retries
 */
#define SOMEIPSD_MAX_FIND_SERVICE_RETRIES           (3U)

/**
 * @brief Maximum number of subscribe retries
 */
#define SOMEIPSD_MAX_SUBSCRIBE_RETRIES              (3U)

/**
 * @brief Find service retry delay (milliseconds)
 */
#define SOMEIPSD_FIND_SERVICE_RETRY_DELAY_MS        (1000U)

/**
 * @brief Subscribe retry delay (milliseconds)
 */
#define SOMEIPSD_SUBSCRIBE_RETRY_DELAY_MS           (1000U)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/**
 * @brief Enable service availability callbacks
 */
#define SOMEIPSD_SERVICE_AVAILABILITY_CALLBACKS     (STD_ON)

/**
 * @brief Enable subscription state callbacks
 */
#define SOMEIPSD_SUBSCRIPTION_STATE_CALLBACKS       (STD_ON)

/*==================================================================================================
*                                    CONFIGURATION STRUCTURES
==================================================================================================*/

/* Forward declarations for configuration structures */
typedef struct SomeIpSd_OfferedServiceConfigType_tag SomeIpSd_OfferedServiceConfigType;
typedef struct SomeIpSd_RequiredServiceConfigType_tag SomeIpSd_RequiredServiceConfigType;
typedef struct SomeIpSd_EventGroupConfigType_tag SomeIpSd_EventGroupConfigType;
typedef struct SomeIpSd_SubscriptionConfigType_tag SomeIpSd_SubscriptionConfigType;
typedef struct SomeIpSd_ConfigType_tag SomeIpSd_ConfigType;

/*==================================================================================================
*                                    EXTERNAL CONFIGURATION
==================================================================================================*/

/**
 * @brief External declaration of the SOME/IP-SD configuration
 */
extern const SomeIpSd_ConfigType SomeIpSd_Config;

/**
 * @brief External declaration of offered service configurations
 */
extern const SomeIpSd_OfferedServiceConfigType SomeIpSd_OfferedServices[SOMEIPSD_NUM_OFFERED_SERVICES];

/**
 * @brief External declaration of required service configurations
 */
extern const SomeIpSd_RequiredServiceConfigType SomeIpSd_RequiredServices[SOMEIPSD_NUM_REQUIRED_SERVICES];

/**
 * @brief External declaration of event group configurations
 */
extern const SomeIpSd_EventGroupConfigType SomeIpSd_EventGroups[SOMEIPSD_NUM_CONFIGURED_EVENTGROUPS];

/**
 * @brief External declaration of subscription configurations
 */
extern const SomeIpSd_SubscriptionConfigType SomeIpSd_Subscriptions[SOMEIPSD_NUM_CONFIGURED_SUBSCRIPTIONS];

#endif /* SOMEIPSD_CFG_H */
