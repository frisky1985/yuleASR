/** @file Mqtt_Lcfg.c
 * @brief MQTT 模块链接时配置表
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 通过配置工具生成，手动修改可能被覆盖
 */

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Mqtt.h"
#include "Mqtt_Cfg.h"

/*============================================================================
 * 回调函数前向声明
 *===========================================================================*/
extern void Mqtt_DefaultConnectionCallback(uint8 connectionId, uint8 newState);
extern void Mqtt_DefaultMessageCallback(uint8 connectionId, const char* topic,
                                         const uint8* payload, uint32 length);
extern void Mqtt_DefaultErrorCallback(uint8 connectionId, uint8 errorCode);

/*============================================================================
 * 通知回调配置
 *===========================================================================*/
static const Mqtt_NotificationCallbacksType Mqtt_NotificationCallbacks = {
    .connectionStateChanged = Mqtt_DefaultConnectionCallback,
    .messageReceived        = Mqtt_DefaultMessageCallback,
    .errorOccurred          = Mqtt_DefaultErrorCallback
};

/*============================================================================
 * 连接配置表
 *===========================================================================*/
/**
 * @brief 默认连接配置
 */
const Mqtt_ConnectionConfigType Mqtt_DefaultConnectionConfig = {
    .brokerHost        = "localhost",
    .brokerPort        = 1883,
    .clientId          = "AUTOSAR_MQTT_Client",
    .keepAliveSeconds  = MQTT_DEFAULT_KEEP_ALIVE_S,
    .cleanSession      = MQTT_CLEAN_SESSION_TRUE,
    .version           = MQTT_VERSION_311,
    .username          = NULL,
    .password          = NULL,
    .connectTimeoutMs  = MQTT_DEFAULT_CONNECT_TIMEOUT_MS,
    .recvTimeoutMs     = MQTT_DEFAULT_RECV_TIMEOUT_MS,
    .sendTimeoutMs     = 1000,
    .autoReconnect     = TRUE,
    .reconnectIntervalMs = MQTT_DEFAULT_RECONNECT_INTERVAL_MS
};

/*============================================================================
 * 模块级配置
 *===========================================================================*/
const Mqtt_ConfigType Mqtt_Config = {
    .maxConnections       = MQTT_MAX_CONNECTIONS,
    .sendBufferSize       = MQTT_SEND_BUFFER_SIZE,
    .recvBufferSize       = MQTT_RECV_BUFFER_SIZE,
    .messageQueueDepth    = MQTT_MESSAGE_QUEUE_DEPTH,
    .enableAutoReconnect  = (MQTT_SUPPORT_AUTO_RECONNECT == STD_ON),
    .callbacks            = Mqtt_NotificationCallbacks
};

/*============================================================================
 * 配置指针 (外部引用)
 *===========================================================================*/
const Mqtt_ConfigType* Mqtt_ConfigPtr = &Mqtt_Config;

/*============================================================================
 * 默认回调函数定义 (用户可重载)
 *===========================================================================*/
__attribute__((weak)) void Mqtt_DefaultConnectionCallback(
    uint8 connectionId,
    uint8 newState
)
{
    (void)connectionId;
    (void)newState;
    /* 默认空实现 */
}

__attribute__((weak)) void Mqtt_DefaultMessageCallback(
    uint8 connectionId,
    const char* topic,
    const uint8* payload,
    uint32 length
)
{
    (void)connectionId;
    (void)topic;
    (void)payload;
    (void)length;
    /* 默认空实现 */
}

__attribute__((weak)) void Mqtt_DefaultErrorCallback(
    uint8 connectionId,
    uint8 errorCode
)
{
    (void)connectionId;
    (void)errorCode;
    /* 默认空实现 */
}
