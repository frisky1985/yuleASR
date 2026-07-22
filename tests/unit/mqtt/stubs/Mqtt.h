/**
 * @file Mqtt.h
 * @brief MQTT module stub header for unit testing
 *
 * This stub shadows the real Mqtt.h, avoiding the enum/macro name
 * collision bug (MQTT_E_TIMEOUT defined as both #define and enum value).
 * It provides all types, constants, and function declarations needed to
 * compile and test the Mqtt.c implementation.
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#ifndef MQTT_STUB_H
#define MQTT_STUB_H

/*============================================================================
 * Version information
 *===========================================================================*/
#define MQTT_VENDOR_ID                    (0x01FF)
#define MQTT_MODULE_ID                    (0x00B0)

#define MQTT_SW_MAJOR_VERSION             (1)
#define MQTT_SW_MINOR_VERSION             (0)
#define MQTT_SW_PATCH_VERSION             (0)

#define MQTT_AR_RELEASE_MAJOR_VERSION     (4)
#define MQTT_AR_RELEASE_MINOR_VERSION     (4)
#define MQTT_AR_RELEASE_REVISION_VERSION  (0)

/*============================================================================
 * Includes
 *===========================================================================*/
#include "Std_Types.h"
#include "Mqtt_Cfg.h"

/*============================================================================
 * API service IDs
 *===========================================================================*/
#define MQTT_SID_INIT                     (0x01)
#define MQTT_SID_DEINIT                   (0x02)
#define MQTT_SID_CONNECT                  (0x03)
#define MQTT_SID_DISCONNECT               (0x04)
#define MQTT_SID_PUBLISH                  (0x05)
#define MQTT_SID_SUBSCRIBE                (0x06)
#define MQTT_SID_UNSUBSCRIBE              (0x07)
#define MQTT_SID_GETVERSIONINFO           (0x08)
#define MQTT_SID_MAINFUNCTION             (0x09)
#define MQTT_SID_PING                     (0x0A)

/*============================================================================
 * DET error codes
 *===========================================================================*/
#define MQTT_E_PARAM_POINTER              (0x01)
#define MQTT_E_PARAM_CONFIG               (0x02)
#define MQTT_E_PARAM_CONNECTION           (0x03)
#define MQTT_E_PARAM_TOPIC                (0x04)
#define MQTT_E_PARAM_PAYLOAD              (0x05)
#define MQTT_E_PARAM_QOS                  (0x06)
#define MQTT_E_UNINIT                     (0x07)
#define MQTT_E_ALREADY_INITIALIZED        (0x08)
#define MQTT_E_CONNECTION_FAILED          (0x09)
#define MQTT_E_PUBLISH_FAILED             (0x0A)
#define MQTT_E_SUBSCRIBE_FAILED           (0x0B)
#define MQTT_E_BUFFER_OVERFLOW            (0x0C)
#define MQTT_E_TIMEOUT                    (0x0D)

/*============================================================================
 * Data types
 *===========================================================================*/

/* Connection handle */
typedef uint8 Mqtt_ConnectionIdType;

/* Subscription ID */
typedef uint16 Mqtt_SubscriptionIdType;

/* Return type — uint8 to match the real API's ABI */
typedef uint8 Mqtt_ReturnType;

/* Return value constants */
#define MQTT_OK          ((Mqtt_ReturnType)0x00U)
#define MQTT_E_NOT_OK    ((Mqtt_ReturnType)0x01U)
#define MQTT_E_BUSY      ((Mqtt_ReturnType)0x02U)
#define MQTT_E_NOCONN    ((Mqtt_ReturnType)0x03U)
#define MQTT_E_INVTOPIC  ((Mqtt_ReturnType)0x04U)
#define MQTT_E_INVPAYLOAD ((Mqtt_ReturnType)0x05U)
#define MQTT_E_BUFFERFULL ((Mqtt_ReturnType)0x06U)
#define MQTT_E_DISCONNECTED ((Mqtt_ReturnType)0x07U)
#define MQTT_E_UNIT      ((Mqtt_ReturnType)0x08U)

/* Connection state */
typedef enum {
    MQTT_STATE_UNINIT = 0,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_TCP_CONNECTING,
    MQTT_STATE_MQTT_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_DISCONNECTING,
    MQTT_STATE_RECONNECTING
#if (MQTT_SUPPORT_TLS == STD_ON)
    ,MQTT_STATE_TLS_HANDSHAKING
#endif
} Mqtt_ConnectionStateType;

/* MQTT protocol version */
typedef enum {
    MQTT_VERSION_311 = 4,
    MQTT_VERSION_50  = 5
} Mqtt_ProtocolVersionType;

/* QoS level */
typedef enum {
    MQTT_QOS_0 = 0,
    MQTT_QOS_1 = 1,
    MQTT_QOS_2 = 2
} Mqtt_QoSType;

/* Retain flag */
typedef enum {
    MQTT_RETAIN_FALSE = 0,
    MQTT_RETAIN_TRUE  = 1
} Mqtt_RetainType;

/* Clean session flag */
typedef enum {
    MQTT_CLEAN_SESSION_FALSE = 0,
    MQTT_CLEAN_SESSION_TRUE  = 1
} Mqtt_CleanSessionType;

/* Connection config */
typedef struct {
    const char* brokerHost;
    uint16 brokerPort;
    const char* clientId;
    uint16 keepAliveSeconds;
    Mqtt_CleanSessionType cleanSession;
    Mqtt_ProtocolVersionType version;
    const char* username;
    const char* password;
    uint16 connectTimeoutMs;
    uint16 recvTimeoutMs;
    uint16 sendTimeoutMs;
    boolean autoReconnect;
    uint16 reconnectIntervalMs;
#if (MQTT_SUPPORT_TLS == STD_ON)
    boolean useTls;
    const void* tlsConfig; /* opaque in stub context */
#endif
} Mqtt_ConnectionConfigType;

/* Publish message */
typedef struct {
    const char* topic;
    const uint8* payload;
    uint32 payloadLength;
    Mqtt_QoSType qos;
    Mqtt_RetainType retain;
} Mqtt_PublishMessageType;

/* Received message */
typedef struct {
    const char* topic;
    uint8* payload;
    uint32 payloadLength;
    Mqtt_QoSType qos;
    boolean retain;
    boolean dup;
} Mqtt_ReceivedMessageType;

/* Subscription info */
typedef struct {
    const char* topicFilter;
    Mqtt_QoSType maxQoS;
    Mqtt_SubscriptionIdType subscriptionId;
} Mqtt_SubscriptionType;

/* Connection info (read-only) */
typedef struct {
    Mqtt_ConnectionStateType state;
    uint32 messagesSent;
    uint32 messagesReceived;
    uint32 bytesSent;
    uint32 bytesReceived;
    uint32 connectCount;
    uint32 disconnectCount;
    uint32 reconnectCount;
    uint32 lastErrorCode;
} Mqtt_ConnectionInfoType;

/* Callback types */
typedef void (*Mqtt_ConnectionCallbackType)(
    Mqtt_ConnectionIdType connectionId,
    Mqtt_ConnectionStateType newState,
    Mqtt_ReturnType result
);

typedef void (*Mqtt_MessageCallbackType)(
    Mqtt_ConnectionIdType connectionId,
    const Mqtt_ReceivedMessageType* message
);

typedef void (*Mqtt_PublishCallbackType)(
    Mqtt_ConnectionIdType connectionId,
    Mqtt_SubscriptionIdType subscriptionId,
    Mqtt_ReturnType result
);

/*============================================================================
 * Internal function declarations (needed by Mqtt.c compilation)
 *===========================================================================*/
/* Mqtt_CheckTimeout — called by Mqtt_ProcessStateMachine (static in Mqtt.c) */
boolean Mqtt_CheckTimeout(void* conn, uint16 timeoutMs);

/*============================================================================
 * Public API declarations
 *===========================================================================*/
extern Mqtt_ReturnType Mqtt_Init(const Mqtt_ConfigType* config);
extern Mqtt_ReturnType Mqtt_DeInit(void);
extern Mqtt_ReturnType Mqtt_Connect(Mqtt_ConnectionIdType connectionId,
                                    const Mqtt_ConnectionConfigType* connConfig);
extern Mqtt_ReturnType Mqtt_Disconnect(Mqtt_ConnectionIdType connectionId);
extern Mqtt_ReturnType Mqtt_Publish(Mqtt_ConnectionIdType connectionId,
                                    const Mqtt_PublishMessageType* message,
                                    Mqtt_PublishCallbackType callback);
extern Mqtt_ReturnType Mqtt_Subscribe(Mqtt_ConnectionIdType connectionId,
                                     const Mqtt_SubscriptionType* subscription,
                                     Mqtt_MessageCallbackType msgCallback);
extern Mqtt_ReturnType Mqtt_Unsubscribe(Mqtt_ConnectionIdType connectionId,
                                        const char* topicFilter);
extern Mqtt_ReturnType Mqtt_Ping(Mqtt_ConnectionIdType connectionId);
extern Mqtt_ConnectionStateType Mqtt_GetConnectionState(
    Mqtt_ConnectionIdType connectionId);
extern Mqtt_ReturnType Mqtt_GetConnectionInfo(
    Mqtt_ConnectionIdType connectionId,
    Mqtt_ConnectionInfoType* info);
extern void Mqtt_MainFunction(void);
extern void Mqtt_SetConnectionCallback(
    Mqtt_ConnectionIdType connectionId,
    Mqtt_ConnectionCallbackType callback);

#if (MQTT_VERSION_INFO_API == STD_ON)
extern void Mqtt_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#endif /* MQTT_STUB_H */
