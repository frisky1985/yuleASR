/**
 * @file Mqtt_Cfg.h
 * @brief MQTT module configuration stub for unit testing
 *
 * This stub shadows the real Mqtt_Cfg.h for test isolation.
 * It defines build-time configuration needed to compile Mqtt.c.
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#ifndef MQTT_CFG_STUB_H
#define MQTT_CFG_STUB_H

/*============================================================================
 * Version check
 *===========================================================================*/
#define MQTT_CFG_MAJOR_VERSION    (1)
#define MQTT_CFG_MINOR_VERSION    (0)
#define MQTT_CFG_PATCH_VERSION    (0)

/*============================================================================
 * Preprocessing configuration
 *===========================================================================*/
#define MQTT_DEV_ERROR_DETECT     (STD_ON)
#define MQTT_VERSION_INFO_API     (STD_ON)
#define MQTT_DEBUG_MODE           (STD_OFF)

/*============================================================================
 * Feature configuration
 *===========================================================================*/
#define MQTT_SUPPORT_V50          (STD_ON)
#define MQTT_SUPPORT_TLS          (STD_ON)
#define MQTT_SUPPORT_MTLS         (STD_OFF)
#define MQTT_SUPPORT_SSL_V30      (STD_OFF)
#define MQTT_DEFAULT_TLS_VERSION  (0)   /* MQTT_TLS_VERSION_1_2 */
#define MQTT_SUPPORT_AUTO_RECONNECT  (STD_ON)
#define MQTT_SUPPORT_CHANNEL_CALLBACKS (STD_ON)
#define MQTT_SUPPORT_WILL_MESSAGE (STD_ON)
#define MQTT_SUPPORT_LAST_WILL    (STD_ON)

/*============================================================================
 * Quantity configuration
 *===========================================================================*/
#define MQTT_MAX_CONNECTIONS              (4U)
#define MQTT_MAX_SUBSCRIPTIONS_PER_CONN   (8U)
#define MQTT_MAX_TOPIC_LENGTH             (128U)
#define MQTT_MAX_CLIENT_ID_LENGTH         (64U)
#define MQTT_SEND_BUFFER_SIZE             (2048U)
#define MQTT_RECV_BUFFER_SIZE             (2048U)
#define MQTT_MESSAGE_QUEUE_DEPTH          (8U)
#define MQTT_MAIN_FUNCTION_PERIOD_MS      (10U)

/*============================================================================
 * TLS/SSL configuration
 *===========================================================================*/
#if (MQTT_SUPPORT_TLS == STD_ON)
#define MQTT_TLS_SEND_BUFFER_SIZE         (4096U)
#define MQTT_TLS_RECV_BUFFER_SIZE         (4096U)
#define MQTT_TLS_HANDSHAKE_TIMEOUT_MS     (10000U)
#define MQTT_TLS_SESSION_CACHE_SIZE       (1024U)
#define MQTT_TLS_MAX_CERT_CHAIN_DEPTH     (3U)
#define MQTT_TLS_VERIFY_EXPIRY            (STD_ON)
#define MQTT_TLS_VERIFY_HOSTNAME          (STD_ON)
#define MQTT_TLS_SECURE_RENEGOTIATION     (STD_ON)
#define MQTT_TLS_SESSION_RESUMPTION       (STD_ON)
#define MQTT_TLS_HW_ACCELERATION          (STD_OFF)
#endif

/*============================================================================
 * Default timing values
 *===========================================================================*/
#define MQTT_DEFAULT_KEEP_ALIVE_S           (60U)
#define MQTT_DEFAULT_CONNECT_TIMEOUT_MS     (5000U)
#define MQTT_DEFAULT_RECV_TIMEOUT_MS        (1000U)
#define MQTT_DEFAULT_RECONNECT_INTERVAL_MS  (5000U)
#define MQTT_MAX_RECONNECT_ATTEMPTS         (5U)

/*============================================================================
 * Memory mapping
 *===========================================================================*/
#if !defined(MQTT_CODE)
#define MQTT_CODE
#endif
#if !defined(MQTT_CONST)
#define MQTT_CONST    const
#endif
#if !defined(MQTT_VAR)
#define MQTT_VAR
#endif

/*============================================================================
 * Configuration structure forward declarations
 *===========================================================================*/
/* Notification callbacks */
typedef struct {
    void (*connectionStateChanged)(uint8 connectionId, uint8 newState);
    void (*messageReceived)(uint8 connectionId, const char* topic,
                           const uint8* payload, uint32 length);
    void (*errorOccurred)(uint8 connectionId, uint8 errorCode);
} Mqtt_NotificationCallbacksType;

/* Module-level configuration */
typedef struct {
    uint8 maxConnections;
    uint16 sendBufferSize;
    uint16 recvBufferSize;
    uint8 messageQueueDepth;
    boolean enableAutoReconnect;
    Mqtt_NotificationCallbacksType callbacks;
} Mqtt_ConfigType;

/*============================================================================
 * External dependency includes
 *===========================================================================*/
/* These are included by the real Mqtt_Cfg.h; we stub them locally. */
#include "ComStack_Types.h"
#include "TcpIp.h"
#include "Mqtt_Tls.h"
#include "Det.h"

#endif /* MQTT_CFG_STUB_H */
