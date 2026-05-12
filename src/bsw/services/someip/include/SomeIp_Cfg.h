/**
 * @file SomeIp_Cfg.h
 * @brief SOME/IP Configuration Header
 * @version 1.0.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * 
 * AUTOSAR R20-11 compliant SOME/IP configuration
 * Scalable service-Oriented Middleware over IP (SOME/IP)
 */

#ifndef SOMEIP_CFG_H
#define SOMEIP_CFG_H

/*==================================================================================================
*                                    AUTOSAR VERSION
==================================================================================================*/
#define SOMEIP_CFG_AR_RELEASE_MAJOR_VERSION       4
#define SOMEIP_CFG_AR_RELEASE_MINOR_VERSION       0
#define SOMEIP_CFG_AR_RELEASE_REVISION_VERSION    3

/*==================================================================================================
*                                    MODULE VERSION
==================================================================================================*/
#define SOMEIP_CFG_SW_MAJOR_VERSION               1
#define SOMEIP_CFG_SW_MINOR_VERSION               0
#define SOMEIP_CFG_SW_PATCH_VERSION               0

/*==================================================================================================
*                                    PRE-COMPILE CONFIGURATION
==================================================================================================*/

/**
 * @brief Development error detection enable/disable
 * @implements SOMEIP_DEV_ERROR_DETECT
 */
#define SOMEIP_DEV_ERROR_DETECT                   (STD_ON)

/**
 * @brief Version information API enable/disable
 * @implements SOMEIP_VERSION_INFO_API
 */
#define SOMEIP_VERSION_INFO_API                   (STD_ON)

/**
 * @brief Enable/Disable support for SOME/IP transformer
 */
#define SOMEIP_TRANSFORMER_SUPPORT                (STD_ON)

/**
 * @brief Enable/Disable event handling
 */
#define SOMEIP_EVENT_HANDLING_ENABLED             (STD_ON)

/**
 * @brief Enable/Disable method call handling
 */
#define SOMEIP_METHOD_CALL_HANDLING_ENABLED       (STD_ON)

/*==================================================================================================
*                                    MESSAGE BUFFER CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum message buffer size (bytes)
 * @implements SOMEIP_MAX_MESSAGE_SIZE
 */
#define SOMEIP_MAX_MESSAGE_SIZE                   (4096U)

/**
 * @brief Maximum payload length (bytes)
 */
#define SOMEIP_MAX_PAYLOAD_SIZE                   (4072U)  /* SOMEIP_MAX_MESSAGE_SIZE - SOMEIP_HEADER_SIZE */

/**
 * @brief Number of message buffers for reception
 */
#define SOMEIP_NUM_RX_BUFFERS                     (8U)

/**
 * @brief Number of message buffers for transmission
 */
#define SOMEIP_NUM_TX_BUFFERS                     (8U)

/**
 * @brief Total number of message buffers
 */
#define SOMEIP_TOTAL_NUM_BUFFERS                  (SOMEIP_NUM_RX_BUFFERS + SOMEIP_NUM_TX_BUFFERS)

/**
 * @brief Rx buffer size per buffer
 */
#define SOMEIP_RX_BUFFER_SIZE                     (SOMEIP_MAX_MESSAGE_SIZE)

/**
 * @brief Tx buffer size per buffer
 */
#define SOMEIP_TX_BUFFER_SIZE                     (SOMEIP_MAX_MESSAGE_SIZE)

/*==================================================================================================
*                                    SERVICE CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of services
 */
#define SOMEIP_MAX_SERVICES                       (16U)

/**
 * @brief Maximum number of service instances per service
 */
#define SOMEIP_MAX_SERVICE_INSTANCES              (4U)

/**
 * @brief Maximum number of methods per service
 */
#define SOMEIP_MAX_METHODS_PER_SERVICE            (8U)

/**
 * @brief Maximum number of events per service
 */
#define SOMEIP_MAX_EVENTS_PER_SERVICE             (8U)

/**
 * @brief Maximum number of fields per service
 */
#define SOMEIP_MAX_FIELDS_PER_SERVICE             (8U)

/**
 * @brief Maximum number of event groups per service
 */
#define SOMEIP_MAX_EVENTGROUPS_PER_SERVICE        (4U)

/**
 * @brief Total maximum methods across all services
 */
#define SOMEIP_MAX_METHODS_TOTAL                  (64U)

/**
 * @brief Total maximum events across all services
 */
#define SOMEIP_MAX_EVENTS_TOTAL                   (64U)

/**
 * @brief Total maximum fields across all services
 */
#define SOMEIP_MAX_FIELDS_TOTAL                   (32U)

/*==================================================================================================
*                                    CLIENT CONFIGURATION
==================================================================================================*/

/**
 * @brief Maximum number of clients
 */
#define SOMEIP_MAX_CLIENTS                        (8U)

/**
 * @brief Maximum number of pending requests per client
 */
#define SOMEIP_MAX_PENDING_REQUESTS               (4U)

/**
 * @brief Starting session ID
 */
#define SOMEIP_INITIAL_SESSION_ID                 (1U)

/**
 * @brief Maximum session ID (will wrap to initial after this)
 */
#define SOMEIP_MAX_SESSION_ID                     (0xFFFFU)

/**
 * @brief Starting client ID
 */
#define SOMEIP_INITIAL_CLIENT_ID                  (0x0001U)

/*==================================================================================================
*                                    TIMEOUT CONFIGURATION
==================================================================================================*/

/**
 * @brief Request timeout in milliseconds
 */
#define SOMEIP_REQUEST_TIMEOUT_MS                 (5000U)

/**
 * @brief Response timeout in milliseconds
 */
#define SOMEIP_RESPONSE_TIMEOUT_MS                (5000U)

/**
 * @brief Method call timeout in milliseconds
 */
#define SOMEIP_METHOD_CALL_TIMEOUT_MS             (5000U)

/**
 * @brief Event subscription timeout in milliseconds
 */
#define SOMEIP_EVENT_SUBSCRIPTION_TIMEOUT_MS      (3000U)

/**
 * @brief Maximum time to wait for transmission confirmation
 */
#define SOMEIP_TX_CONFIRMATION_TIMEOUT_MS         (1000U)

/**
 * @brief Connection establishment timeout
 */
#define SOMEIP_CONNECTION_TIMEOUT_MS              (3000U)

/*==================================================================================================
*                                    MAIN FUNCTION PERIODS
==================================================================================================*/

/**
 * @brief Main function period in milliseconds
 */
#define SOMEIP_MAIN_FUNCTION_PERIOD_MS            (10U)

/**
 * @brief Rx main function period in milliseconds
 */
#define SOMEIP_MAIN_FUNCTION_RX_PERIOD_MS         (10U)

/**
 * @brief Tx main function period in milliseconds
 */
#define SOMEIP_MAIN_FUNCTION_TX_PERIOD_MS         (10U)

/**
 * @brief Timeout supervision main function period in milliseconds
 */
#define SOMEIP_MAIN_FUNCTION_TIMEOUT_PERIOD_MS    (10U)

/*==================================================================================================
*                                    TRANSMISSION CONFIGURATION
==================================================================================================*/

/**
 * @brief Enable separate transmission thread
 */
#define SOMEIP_SEPARATE_TX_THREAD                 (STD_OFF)

/**
 * @brief Enable retry for failed transmissions
 */
#define SOMEIP_RETRY_FAILED_TRANSMISSION          (STD_ON)

/**
 * @brief Maximum number of transmission retries
 */
#define SOMEIP_MAX_TX_RETRIES                     (3U)

/**
 * @brief Time between transmission retries (milliseconds)
 */
#define SOMEIP_TX_RETRY_DELAY_MS                  (100U)

/*==================================================================================================
*                                    SERVICE ID DEFINITIONS
==================================================================================================*/

/**
 * @brief SOME/IP SD Service ID
 */
#define SOMEIP_SERVICE_ID_SD                      ((SomeIp_ServiceIdType)0xFFFFU)

/**
 * @brief Example: Vehicle Service ID
 */
#define SOMEIP_SERVICE_ID_VEHICLE                 ((SomeIp_ServiceIdType)0x0001U)

/**
 * @brief Example: Engine Service ID
 */
#define SOMEIP_SERVICE_ID_ENGINE                  ((SomeIp_ServiceIdType)0x0002U)

/**
 * @brief Example: Chassis Service ID
 */
#define SOMEIP_SERVICE_ID_CHASSIS                 ((SomeIp_ServiceIdType)0x0003U)

/**
 * @brief Example: Body Service ID
 */
#define SOMEIP_SERVICE_ID_BODY                    ((SomeIp_ServiceIdType)0x0004U)

/**
 * @brief Example: Diagnostic Service ID
 */
#define SOMEIP_SERVICE_ID_DIAGNOSTIC              ((SomeIp_ServiceIdType)0x0005U)

/**
 * @brief Example: Infotainment Service ID
 */
#define SOMEIP_SERVICE_ID_INFOTAINMENT            ((SomeIp_ServiceIdType)0x0006U)

/**
 * @brief Example: ADAS Service ID
 */
#define SOMEIP_SERVICE_ID_ADAS                    ((SomeIp_ServiceIdType)0x0007U)

/**
 * @brief Example: Powertrain Service ID
 */
#define SOMEIP_SERVICE_ID_POWERTRAIN              ((SomeIp_ServiceIdType)0x0008U)

/*==================================================================================================
*                                    METHOD ID DEFINITIONS
==================================================================================================*/

/* Vehicle Service Methods (Service ID 0x0001) */
#define SOMEIP_METHOD_ID_VEHICLE_GET_SPEED        ((SomeIp_MethodIdType)0x0001U)
#define SOMEIP_METHOD_ID_VEHICLE_GET_POSITION     ((SomeIp_MethodIdType)0x0002U)
#define SOMEIP_METHOD_ID_VEHICLE_SET_MODE         ((SomeIp_MethodIdType)0x0003U)
#define SOMEIP_METHOD_ID_VEHICLE_GET_STATUS       ((SomeIp_MethodIdType)0x0004U)

/* Engine Service Methods (Service ID 0x0002) */
#define SOMEIP_METHOD_ID_ENGINE_GET_RPM           ((SomeIp_MethodIdType)0x0001U)
#define SOMEIP_METHOD_ID_ENGINE_GET_TEMP          ((SomeIp_MethodIdType)0x0002U)
#define SOMEIP_METHOD_ID_ENGINE_START             ((SomeIp_MethodIdType)0x0003U)
#define SOMEIP_METHOD_ID_ENGINE_STOP              ((SomeIp_MethodIdType)0x0004U)

/* Chassis Service Methods (Service ID 0x0003) */
#define SOMEIP_METHOD_ID_CHASSIS_GET_STEERING     ((SomeIp_MethodIdType)0x0001U)
#define SOMEIP_METHOD_ID_CHASSIS_GET_BRAKE        ((SomeIp_MethodIdType)0x0002U)

/*==================================================================================================
*                                    EVENT ID DEFINITIONS
==================================================================================================*/

/* Vehicle Service Events (Service ID 0x0001) */
#define SOMEIP_EVENT_ID_VEHICLE_SPEED_CHANGED     ((SomeIp_MethodIdType)0x8001U)
#define SOMEIP_EVENT_ID_VEHICLE_POSITION_CHANGED  ((SomeIp_MethodIdType)0x8002U)
#define SOMEIP_EVENT_ID_VEHICLE_STATUS_CHANGED    ((SomeIp_MethodIdType)0x8003U)

/* Engine Service Events (Service ID 0x0002) */
#define SOMEIP_EVENT_ID_ENGINE_RPM_CHANGED        ((SomeIp_MethodIdType)0x8001U)
#define SOMEIP_EVENT_ID_ENGINE_TEMP_CHANGED       ((SomeIp_MethodIdType)0x8002U)
#define SOMEIP_EVENT_ID_ENGINE_STATUS_CHANGED     ((SomeIp_MethodIdType)0x8003U)

/*==================================================================================================
*                                    EVENT GROUP ID DEFINITIONS
==================================================================================================*/

/* Vehicle Event Groups */
#define SOMEIP_EVENTGROUP_ID_VEHICLE_BASIC        ((uint16)0x0001U)
#define SOMEIP_EVENTGROUP_ID_VEHICLE_EXTENDED     ((uint16)0x0002U)

/* Engine Event Groups */
#define SOMEIP_EVENTGROUP_ID_ENGINE_BASIC         ((uint16)0x0001U)
#define SOMEIP_EVENTGROUP_ID_ENGINE_DIAGNOSTIC    ((uint16)0x0002U)

/*==================================================================================================
*                                    FIELD ID DEFINITIONS
==================================================================================================*/

/* Vehicle Fields */
#define SOMEIP_FIELD_ID_VEHICLE_SPEED             ((SomeIp_MethodIdType)0x9001U)
#define SOMEIP_FIELD_ID_VEHICLE_ODOMETER          ((SomeIp_MethodIdType)0x9002U)
#define SOMEIP_FIELD_ID_VEHICLE_GEAR_POSITION     ((SomeIp_MethodIdType)0x9003U)

/* Engine Fields */
#define SOMEIP_FIELD_ID_ENGINE_RPM                ((SomeIp_MethodIdType)0x9001U)
#define SOMEIP_FIELD_ID_ENGINE_TEMPERATURE        ((SomeIp_MethodIdType)0x9002U)
#define SOMEIP_FIELD_ID_ENGINE_HOURS              ((SomeIp_MethodIdType)0x9003U)

/*==================================================================================================
*                                    INSTANCE ID DEFINITIONS
==================================================================================================*/

/**
 * @brief Default service instance ID
 */
#define SOMEIP_INSTANCE_ID_DEFAULT                ((uint16)0x0001U)

/**
 * @brief Secondary service instance ID
 */
#define SOMEIP_INSTANCE_ID_SECONDARY              ((uint16)0x0002U)

/**
 * @brief Any instance ID (for find service)
 */
#define SOMEIP_INSTANCE_ID_ANY                    ((uint16)0xFFFFU)

/*==================================================================================================
*                                    PROTOCOL CONFIGURATION
==================================================================================================*/

/**
 * @brief Protocol version
 */
#define SOMEIP_CFG_PROTOCOL_VERSION               ((SomeIp_ProtocolVersionType)0x01U)

/**
 * @brief Interface version
 */
#define SOMEIP_CFG_INTERFACE_VERSION              ((SomeIp_InterfaceVersionType)0x01U)

/*==================================================================================================
*                                    ENDIANNESS CONFIGURATION
==================================================================================================*/

/**
 * @brief Endianness: 0=Little Endian, 1=Big Endian (Network)
 */
#define SOMEIP_ENDIANNESS                         (1U)  /* Network Byte Order (Big Endian) */

/**
 * @brief Header fields are in network byte order
 */
#define SOMEIP_HEADER_NETWORK_BYTE_ORDER          (STD_ON)

/*==================================================================================================
*                                    CALLBACK CONFIGURATION
==================================================================================================*/

/**
 * @brief Enable asynchronous method calls
 */
#define SOMEIP_ASYNC_METHOD_CALL_ENABLED          (STD_ON)

/**
 * @brief Enable separate callback for each method
 */
#define SOMEIP_SEPARATE_METHOD_CALLBACKS          (STD_ON)

/**
 * @brief Enable separate callback for each event
 */
#define SOMEIP_SEPARATE_EVENT_CALLBACKS           (STD_ON)

/*==================================================================================================
*                                    TRANSFORMER CONFIGURATION
==================================================================================================*/

/**
 * @brief Enable SOME/IP transformer for serialization
 */
#define SOMEIP_ENABLE_TRANSFORMER                 (STD_ON)

/**
 * @brief Enable E2E protection
 */
#define SOMEIP_E2E_PROTECTION_ENABLED             (STD_OFF)

/**
 * @brief Enable wire type configuration
 */
#define SOMEIP_WIRE_TYPE_CONFIG_ENABLED           (STD_ON)

/*==================================================================================================
*                                    ECU CONFIGURATION
==================================================================================================*/

/**
 * @brief ECU specific client ID base
 */
#define SOMEIP_CLIENT_ID_BASE                     ((SomeIp_ClientIdType)0x0001U)

/**
 * @brief Maximum number of concurrent method calls
 */
#define SOMEIP_MAX_CONCURRENT_METHOD_CALLS        (16U)

/**
 * @brief Maximum number of active event subscriptions
 */
#define SOMEIP_MAX_EVENT_SUBSCRIPTIONS            (32U)

/*==================================================================================================
*                                    CONFIGURATION ARRAY SIZES
==================================================================================================*/

/**
 * @brief Number of configured services
 */
#define SOMEIP_NUM_CONFIGURED_SERVICES            (4U)

/**
 * @brief Number of configured methods
 */
#define SOMEIP_NUM_CONFIGURED_METHODS             (12U)

/**
 * @brief Number of configured events
 */
#define SOMEIP_NUM_CONFIGURED_EVENTS              (8U)

/**
 * @brief Number of configured fields
 */
#define SOMEIP_NUM_CONFIGURED_FIELDS              (6U)

/**
 * @brief Number of configured event groups
 */
#define SOMEIP_NUM_CONFIGURED_EVENTGROUPS         (6U)

/**
 * @brief Number of configured clients
 */
#define SOMEIP_NUM_CONFIGURED_CLIENTS             (4U)

/*==================================================================================================
*                                    CONFIGURATION STRUCTURES
==================================================================================================*/

/* Forward declarations for configuration structures */
typedef struct SomeIp_ServiceConfigType_tag SomeIp_ServiceConfigType;
typedef struct SomeIp_MethodConfigType_tag SomeIp_MethodConfigType;
typedef struct SomeIp_EventConfigType_tag SomeIp_EventConfigType;
typedef struct SomeIp_FieldConfigType_tag SomeIp_FieldConfigType;
typedef struct SomeIp_EventGroupConfigType_tag SomeIp_EventGroupConfigType;
typedef struct SomeIp_ClientConfigType_tag SomeIp_ClientConfigType;
typedef struct SomeIp_ConfigType_tag SomeIp_ConfigType;

/*==================================================================================================
*                                    EXTERNAL CONFIGURATION
==================================================================================================*/

/**
 * @brief External declaration of the SOME/IP configuration
 */
extern const SomeIp_ConfigType SomeIp_Config;

/**
 * @brief External declaration of service configurations
 */
extern const SomeIp_ServiceConfigType SomeIp_Services[SOMEIP_NUM_CONFIGURED_SERVICES];

/**
 * @brief External declaration of method configurations
 */
extern const SomeIp_MethodConfigType SomeIp_Methods[SOMEIP_NUM_CONFIGURED_METHODS];

/**
 * @brief External declaration of event configurations
 */
extern const SomeIp_EventConfigType SomeIp_Events[SOMEIP_NUM_CONFIGURED_EVENTS];

/**
 * @brief External declaration of field configurations
 */
extern const SomeIp_FieldConfigType SomeIp_Fields[SOMEIP_NUM_CONFIGURED_FIELDS];

/**
 * @brief External declaration of event group configurations
 */
extern const SomeIp_EventGroupConfigType SomeIp_EventGroups[SOMEIP_NUM_CONFIGURED_EVENTGROUPS];

/**
 * @brief External declaration of client configurations
 */
extern const SomeIp_ClientConfigType SomeIp_Clients[SOMEIP_NUM_CONFIGURED_CLIENTS];

#endif /* SOMEIP_CFG_H */
