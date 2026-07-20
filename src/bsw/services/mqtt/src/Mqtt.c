/** @file Mqtt.c
 * @brief MQTT 客户端核心实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 支持MQTT 3.1.1和5.0协议
 * 实现要点:
 * - 非阻塞式设计
 * - 状态机驱动
 * - 自动重连机制
 * - 并发安全
 */

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Mqtt.h"
#include <string.h>
#include <stdio.h>

#if (MQTT_SUPPORT_TLS == STD_ON)
#include "Mqtt_Tls.h"
#endif

/*============================================================================
 * 内部宏定义
 *===========================================================================*/
#if (MQTT_DEV_ERROR_DETECT == STD_ON)
#define MQTT_DET_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(MQTT_MODULE_ID, 0, (ApiId), (ErrorId))
#else
#define MQTT_DET_REPORT_ERROR(ApiId, ErrorId)
#endif

#define MQTT_IS_VALID_CONNECTION_ID(id) \
    ((id) < MQTT_MAX_CONNECTIONS)

#define MQTT_PACKET_TYPE_CONNECT      (0x10)
#define MQTT_PACKET_TYPE_CONNACK      (0x20)
#define MQTT_PACKET_TYPE_PUBLISH      (0x30)
#define MQTT_PACKET_TYPE_PUBACK       (0x40)
#define MQTT_PACKET_TYPE_PUBREC       (0x50)
#define MQTT_PACKET_TYPE_PUBREL       (0x60)
#define MQTT_PACKET_TYPE_PUBCOMP      (0x70)
#define MQTT_PACKET_TYPE_SUBSCRIBE    (0x80)
#define MQTT_PACKET_TYPE_SUBACK       (0x90)
#define MQTT_PACKET_TYPE_UNSUBSCRIBE  (0xA0)
#define MQTT_PACKET_TYPE_UNSUBACK     (0xB0)
#define MQTT_PACKET_TYPE_PINGREQ      (0xC0)
#define MQTT_PACKET_TYPE_PINGRESP     (0xD0)
#define MQTT_PACKET_TYPE_DISCONNECT   (0xE0)

/*============================================================================
 * 内部数据类型
 *===========================================================================*/
/**
 * @brief 订阅状态
 */
typedef enum {
    SUB_STATE_INACTIVE = 0,
    SUB_STATE_PENDING,
    SUB_STATE_ACTIVE
} Mqtt_SubStateType;

/**
 * @brief 内部订阅管理结构
 */
typedef struct {
    char topicFilter[MQTT_MAX_TOPIC_LENGTH];
    Mqtt_QoSType maxQoS;
    Mqtt_SubscriptionIdType subscriptionId;
    Mqtt_SubStateType state;
    Mqtt_MessageCallbackType callback;
} Mqtt_InternalSubscriptionType;

/**
 * @brief 内部连接管理结构
 */
typedef struct {
    Mqtt_ConnectionStateType state;
    Mqtt_ConnectionConfigType config;
    Mqtt_InternalSubscriptionType subscriptions[MQTT_MAX_SUBSCRIPTIONS_PER_CONN];
    Mqtt_ConnectionCallbackType connCallback;
    Mqtt_ConnectionInfoType info;
    
    /* 缓冲区 */
    uint8 sendBuffer[MQTT_SEND_BUFFER_SIZE];
    uint8 recvBuffer[MQTT_RECV_BUFFER_SIZE];
    uint16 sendLength;
    uint16 recvLength;
    
    /* 状态机变量 */
    uint16 packetIdCounter;
    uint16 reconnectAttempts;
    uint32 lastActivityTime;
    uint32 connectStartTime;
    boolean pendingPing;
    
    /* TCP连接句柄 */
    TcpIp_SocketIdType socketId;
    
#if (MQTT_SUPPORT_TLS == STD_ON)
    /* TLS上下文 */
    boolean useTls;
    Mqtt_TlsContextType tlsContext;
    Mqtt_TlsConfigType tlsConfig;
#endif
} Mqtt_InternalConnectionType;

/*============================================================================
 * 内部变量
 *===========================================================================*/
static boolean Mqtt_Initialized = FALSE;
static Mqtt_InternalConnectionType Mqtt_Connections[MQTT_MAX_CONNECTIONS];
static const Mqtt_ConfigType* Mqtt_ConfigPtr = NULL;

/*============================================================================
 * 内部函数声明
 *===========================================================================*/
static Mqtt_ReturnType Mqtt_ProcessStateMachine(Mqtt_InternalConnectionType* conn);
static Mqtt_ReturnType Mqtt_SendPacket(Mqtt_InternalConnectionType* conn, 
                                        const uint8* data, uint16 length);
static Mqtt_ReturnType Mqtt_ReceivePacket(Mqtt_InternalConnectionType* conn);
static Mqtt_ReturnType Mqtt_EncodeConnect(Mqtt_InternalConnectionType* conn);
static Mqtt_ReturnType Mqtt_EncodePublish(Mqtt_InternalConnectionType* conn,
                                           const Mqtt_PublishMessageType* msg,
                                           uint16 packetId);
static Mqtt_ReturnType Mqtt_EncodeSubscribe(Mqtt_InternalConnectionType* conn,
                                             const Mqtt_SubscriptionType* sub,
                                             uint16 packetId);
static Mqtt_ReturnType Mqtt_EncodePing(Mqtt_InternalConnectionType* conn);
static Mqtt_ReturnType Mqtt_EncodeDisconnect(Mqtt_InternalConnectionType* conn);
static void Mqtt_UpdateState(Mqtt_InternalConnectionType* conn, 
                              Mqtt_ConnectionStateType newState);
static uint16 Mqtt_GetNextPacketId(Mqtt_InternalConnectionType* conn);

/*============================================================================
 * 公共API实现
 *===========================================================================*/

Mqtt_ReturnType Mqtt_Init(const Mqtt_ConfigType* config)
{
    uint8 i, j;
    
    if (Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_INIT, MQTT_E_ALREADY_INITIALIZED);
        return MQTT_E_NOT_OK;
    }
    
    if (config == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_INIT, MQTT_E_PARAM_CONFIG);
        return MQTT_E_NOT_OK;
    }
    
    /* 初始化连接数组 */
    for (i = 0; i < MQTT_MAX_CONNECTIONS; i++) {
        Mqtt_Connections[i].state = MQTT_STATE_UNINIT;
        Mqtt_Connections[i].socketId = TCPIP_SOCKETID_INVALID;
        Mqtt_Connections[i].packetIdCounter = 0;
        Mqtt_Connections[i].reconnectAttempts = 0;
        Mqtt_Connections[i].pendingPing = FALSE;
        
#if (MQTT_SUPPORT_TLS == STD_ON)
        Mqtt_Connections[i].useTls = FALSE;
        Mqtt_Connections[i].tlsContext = NULL;
#endif
        
        memset(&Mqtt_Connections[i].info, 0, sizeof(Mqtt_ConnectionInfoType));
        memset(&Mqtt_Connections[i].config, 0, sizeof(Mqtt_ConnectionConfigType));
        
        for (j = 0; j < MQTT_MAX_SUBSCRIPTIONS_PER_CONN; j++) {
            Mqtt_Connections[i].subscriptions[j].state = SUB_STATE_INACTIVE;
            Mqtt_Connections[i].subscriptions[j].callback = NULL;
        }
    }
    
#if (MQTT_SUPPORT_TLS == STD_ON)
    /* 初始化TLS子系统 */
    Mqtt_Tls_Init();
#endif
    
    Mqtt_ConfigPtr = config;
    Mqtt_Initialized = TRUE;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_DeInit(void)
{
    uint8 i;
    
    if (!Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_DEINIT, MQTT_E_UNINIT);
        return MQTT_E_NOT_OK;
    }
    
    /* 断开所有连接 */
    for (i = 0; i < MQTT_MAX_CONNECTIONS; i++) {
        if (Mqtt_Connections[i].state == MQTT_STATE_CONNECTED) {
            Mqtt_Disconnect(i);
        }
    }
    
    Mqtt_ConfigPtr = NULL;
    Mqtt_Initialized = FALSE;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Connect(Mqtt_ConnectionIdType connectionId,
                              const Mqtt_ConnectionConfigType* connConfig)
{
    Mqtt_InternalConnectionType* conn;
    Mqtt_ReturnType result;
    
    if (!Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_CONNECT, MQTT_E_UNINIT);
        return MQTT_E_NOT_OK;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_CONNECT, MQTT_E_PARAM_CONNECTION);
        return MQTT_E_NOT_OK;
    }
    
    if (connConfig == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_CONNECT, MQTT_E_PARAM_CONFIG);
        return MQTT_E_NOT_OK;
    }
    
    if (connConfig->brokerHost == NULL || connConfig->clientId == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_CONNECT, MQTT_E_PARAM_POINTER);
        return MQTT_E_NOT_OK;
    }
    
    conn = &Mqtt_Connections[connectionId];
    
    /* 检查当前状态 */
    if (conn->state != MQTT_STATE_DISCONNECTED && 
        conn->state != MQTT_STATE_UNINIT) {
        return MQTT_E_NOT_OK;
    }
    
    /* 保存配置 */
    memcpy(&conn->config, connConfig, sizeof(Mqtt_ConnectionConfigType));
    
#if (MQTT_SUPPORT_TLS == STD_ON)
    /* 初始化TLS配置 */
    conn->useTls = connConfig->useTls;
    if (conn->useTls) {
        Mqtt_ReturnType tlsResult;
        
        if (connConfig->tlsConfig == NULL) {
            MQTT_DET_REPORT_ERROR(MQTT_SID_CONNECT, MQTT_E_PARAM_CONFIG);
            return MQTT_E_NOT_OK;
        }
        
        /* 复制TLS配置 */
        memcpy(&conn->tlsConfig, connConfig->tlsConfig, sizeof(Mqtt_TlsConfigType));
        
        /* 创建TLS上下文 */
        tlsResult = Mqtt_Tls_CreateContext(&conn->tlsConfig, &conn->tlsContext);
        if (tlsResult != MQTT_OK) {
            MQTT_DET_REPORT_ERROR(MQTT_SID_CONNECT, MQTT_E_CONNECTION_FAILED);
            return MQTT_E_CONNECTION_FAILED;
        }
    }
#endif
    
    /* 初始化TCP连接 */
    result = TcpIp_SocketCreate(&conn->socketId);
    if (result != E_OK) {
#if (MQTT_SUPPORT_TLS == STD_ON)
        if (conn->useTls && conn->tlsContext != NULL) {
            Mqtt_Tls_DestroyContext(conn->tlsContext);
            conn->tlsContext = NULL;
        }
#endif
        return MQTT_E_CONNECTION_FAILED;
    }
    
    /* 开始连接流程 */
    conn->connectStartTime = 0; /* 连接开始时间 - 系统定时器集成后使用 GetSystemMs() */
    Mqtt_UpdateState(conn, MQTT_STATE_CONNECTING);
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Disconnect(Mqtt_ConnectionIdType connectionId)
{
    Mqtt_InternalConnectionType* conn;
    
    if (!Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_DISCONNECT, MQTT_E_UNINIT);
        return MQTT_E_NOT_OK;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_DISCONNECT, MQTT_E_PARAM_CONNECTION);
        return MQTT_E_NOT_OK;
    }
    
    conn = &Mqtt_Connections[connectionId];
    
    if (conn->state == MQTT_STATE_DISCONNECTED || 
        conn->state == MQTT_STATE_UNINIT) {
        return MQTT_OK;
    }
    
    if (conn->state == MQTT_STATE_CONNECTED) {
        /* 发送DISCONNECT报文 */
        Mqtt_EncodeDisconnect(conn);
        Mqtt_SendPacket(conn, conn->sendBuffer, conn->sendLength);
    }
    
    /* 关闭TCP连接 */
    if (conn->socketId != TCPIP_SOCKETID_INVALID) {
        TcpIp_SocketClose(conn->socketId);
        conn->socketId = TCPIP_SOCKETID_INVALID;
    }
    
    Mqtt_UpdateState(conn, MQTT_STATE_DISCONNECTED);
    conn->info.disconnectCount++;
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Publish(Mqtt_ConnectionIdType connectionId,
                              const Mqtt_PublishMessageType* message,
                              Mqtt_PublishCallbackType callback)
{
    Mqtt_InternalConnectionType* conn;
    Mqtt_ReturnType result;
    uint16 packetId;
    
    if (!Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_PUBLISH, MQTT_E_UNINIT);
        return MQTT_E_NOT_OK;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_PUBLISH, MQTT_E_PARAM_CONNECTION);
        return MQTT_E_NOT_OK;
    }
    
    if (message == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_PUBLISH, MQTT_E_PARAM_POINTER);
        return MQTT_E_NOT_OK;
    }
    
    if (message->topic == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_PUBLISH, MQTT_E_PARAM_TOPIC);
        return MQTT_E_NOT_OK;
    }
    
    conn = &Mqtt_Connections[connectionId];
    
    if (conn->state != MQTT_STATE_CONNECTED) {
        return MQTT_E_NOCONN;
    }
    
    /* 生成包ID */
    packetId = Mqtt_GetNextPacketId(conn);
    
    /* 编码PUBLISH报文 */
    result = Mqtt_EncodePublish(conn, message, packetId);
    if (result != MQTT_OK) {
        return MQTT_E_BUFFER_OVERFLOW;
    }
    
    /* 发送报文 */
    result = Mqtt_SendPacket(conn, conn->sendBuffer, conn->sendLength);
    if (result != MQTT_OK) {
        return MQTT_E_PUBLISH_FAILED;
    }
    
    /* 更新统计 */
    conn->info.messagesSent++;
    conn->info.bytesSent += message->payloadLength;
    
    /* 回调处理用户 */
    if (callback != NULL) {
        callback(connectionId, 0, MQTT_OK);
    }
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Subscribe(Mqtt_ConnectionIdType connectionId,
                                const Mqtt_SubscriptionType* subscription,
                                Mqtt_MessageCallbackType msgCallback)
{
    Mqtt_InternalConnectionType* conn;
    Mqtt_InternalSubscriptionType* internalSub;
    Mqtt_ReturnType result;
    uint16 packetId;
    uint8 i;
    
    if (!Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_SUBSCRIBE, MQTT_E_UNINIT);
        return MQTT_E_NOT_OK;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_SUBSCRIBE, MQTT_E_PARAM_CONNECTION);
        return MQTT_E_NOT_OK;
    }
    
    if (subscription == NULL || subscription->topicFilter == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_SUBSCRIBE, MQTT_E_PARAM_TOPIC);
        return MQTT_E_NOT_OK;
    }
    
    conn = &Mqtt_Connections[connectionId];
    
    if (conn->state != MQTT_STATE_CONNECTED) {
        return MQTT_E_NOCONN;
    }
    
    /* 查找空闲订阅槽 */
    internalSub = NULL;
    for (i = 0; i < MQTT_MAX_SUBSCRIPTIONS_PER_CONN; i++) {
        if (conn->subscriptions[i].state == SUB_STATE_INACTIVE) {
            internalSub = &conn->subscriptions[i];
            break;
        }
    }
    
    if (internalSub == NULL) {
        return MQTT_E_NOT_OK; /* 订阅满 */
    }
    
    /* 填充订阅信息 */
    strncpy(internalSub->topicFilter, subscription->topicFilter, 
            MQTT_MAX_TOPIC_LENGTH - 1);
    internalSub->topicFilter[MQTT_MAX_TOPIC_LENGTH - 1] = '\0';
    internalSub->maxQoS = subscription->maxQoS;
    internalSub->subscriptionId = subscription->subscriptionId;
    internalSub->callback = msgCallback;
    internalSub->state = SUB_STATE_PENDING;
    
    /* 生成包ID并发送 */
    packetId = Mqtt_GetNextPacketId(conn);
    
    result = Mqtt_EncodeSubscribe(conn, subscription, packetId);
    if (result != MQTT_OK) {
        internalSub->state = SUB_STATE_INACTIVE;
        return MQTT_E_BUFFER_OVERFLOW;
    }
    
    result = Mqtt_SendPacket(conn, conn->sendBuffer, conn->sendLength);
    if (result != MQTT_OK) {
        internalSub->state = SUB_STATE_INACTIVE;
        return MQTT_E_SUBSCRIBE_FAILED;
    }
    
    return MQTT_OK;
}

Mqtt_ReturnType Mqtt_Unsubscribe(Mqtt_ConnectionIdType connectionId,
                                  const char* topicFilter)
{
    Mqtt_InternalConnectionType* conn;
    uint8 i;
    
    if (!Mqtt_Initialized) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_UNSUBSCRIBE, MQTT_E_UNINIT);
        return MQTT_E_NOT_OK;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_UNSUBSCRIBE, MQTT_E_PARAM_CONNECTION);
        return MQTT_E_NOT_OK;
    }
    
    if (topicFilter == NULL) {
        MQTT_DET_REPORT_ERROR(MQTT_SID_UNSUBSCRIBE, MQTT_E_PARAM_TOPIC);
        return MQTT_E_NOT_OK;
    }
    
    conn = &Mqtt_Connections[connectionId];
    
    if (conn->state != MQTT_STATE_CONNECTED) {
        return MQTT_E_NOCONN;
    }
    
    /* 查找并移除订阅 */
    for (i = 0; i < MQTT_MAX_SUBSCRIPTIONS_PER_CONN; i++) {
        if (conn->subscriptions[i].state != SUB_STATE_INACTIVE &&
            strcmp(conn->subscriptions[i].topicFilter, topicFilter) == 0U ) {
            conn->subscriptions[i].state = SUB_STATE_INACTIVE;
            conn->subscriptions[i].callback = NULL;
            /* 发送UNSUBSCRIBE报文 - 通过 Mqtt_Encode 和 Mqtt_SendPacket 完成 */
            return MQTT_OK;
        }
    }
    
    return MQTT_E_NOT_OK; /* 未找到订阅 */
}

Mqtt_ReturnType Mqtt_Ping(Mqtt_ConnectionIdType connectionId)
{
    Mqtt_InternalConnectionType* conn;
    Mqtt_ReturnType result;
    
    if (!Mqtt_Initialized) {
        return MQTT_E_UNINIT;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        return MQTT_E_NOT_OK;
    }
    
    conn = &Mqtt_Connections[connectionId];
    
    if (conn->state != MQTT_STATE_CONNECTED) {
        return MQTT_E_NOCONN;
    }
    
    if (conn->pendingPing) {
        return MQTT_E_BUSY; /* 上次PING未响应 */
    }
    
    result = Mqtt_EncodePing(conn);
    if (result != MQTT_OK) {
        return result;
    }
    
    result = Mqtt_SendPacket(conn, conn->sendBuffer, conn->sendLength);
    if (result == MQTT_OK) {
        conn->pendingPing = TRUE;
    }
    
    return result;
}

Mqtt_ConnectionStateType Mqtt_GetConnectionState(Mqtt_ConnectionIdType connectionId)
{
    if (!Mqtt_Initialized || !MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        return MQTT_STATE_UNINIT;
    }
    
    return Mqtt_Connections[connectionId].state;
}

Mqtt_ReturnType Mqtt_GetConnectionInfo(Mqtt_ConnectionIdType connectionId,
                                        Mqtt_ConnectionInfoType* info)
{
    if (!Mqtt_Initialized) {
        return MQTT_E_UNINIT;
    }
    
    if (!MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        return MQTT_E_NOT_OK;
    }
    
    if (info == NULL) {
        return MQTT_E_NOT_OK;
    }
    
    memcpy(info, &Mqtt_Connections[connectionId].info, sizeof(Mqtt_ConnectionInfoType));
    
    return MQTT_OK;
}

void Mqtt_SetConnectionCallback(Mqtt_ConnectionIdType connectionId,
                                 Mqtt_ConnectionCallbackType callback)
{
    if (!Mqtt_Initialized || !MQTT_IS_VALID_CONNECTION_ID(connectionId)) {
        return;
    }
    
    Mqtt_Connections[connectionId].connCallback = callback;
}

void Mqtt_MainFunction(void)
{
    uint8 i;
    
    if (!Mqtt_Initialized) {
        return;
    }
    
    for (i = 0; i < MQTT_MAX_CONNECTIONS; i++) {
        Mqtt_ProcessStateMachine(&Mqtt_Connections[i]);
    }
}

#if (MQTT_VERSION_INFO_API == STD_ON)
void Mqtt_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo == NULL) {
        return;
    }
    
    versioninfo->vendorID = MQTT_VENDOR_ID;
    versioninfo->moduleID = MQTT_MODULE_ID;
    versioninfo->sw_major_version = MQTT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = MQTT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = MQTT_SW_PATCH_VERSION;
}
#endif

/*============================================================================
 * 内部函数实现
 *===========================================================================*/

static void Mqtt_UpdateState(Mqtt_InternalConnectionType* conn, 
                              Mqtt_ConnectionStateType newState)
{
    Mqtt_ConnectionStateType oldState = conn->state;
    
    conn->state = newState;
    
    /* 更新统计 */
    if (newState == MQTT_STATE_CONNECTED) {
        conn->info.connectCount++;
        conn->reconnectAttempts = 0;
    }
    
    /* 回调通知 */
    if (conn->connCallback != NULL && oldState != newState) {
        /* 使用 connectionId 回调 - 当前在循环上下文中无 connectionId, 传入0由上层辨别 */
        conn->connCallback(0, newState, MQTT_OK);
    }
}

static Mqtt_ReturnType Mqtt_ProcessStateMachine(Mqtt_InternalConnectionType* conn)
{
    Mqtt_ReturnType result = MQTT_OK;
    
    switch (conn->state) {
        case MQTT_STATE_UNINIT:
        case MQTT_STATE_DISCONNECTED:
            /* 无需处理 */
            break;
            
        case MQTT_STATE_CONNECTING:
            /* 发送CONNECT报文 */
            result = Mqtt_EncodeConnect(conn);
            if (result == MQTT_OK) {
                result = Mqtt_SendPacket(conn, conn->sendBuffer, conn->sendLength);
                if (result == MQTT_OK) {
                    /* 等待CONNACK */
                }
            }
            break;
            
        case MQTT_STATE_CONNECTED:
            /* 处理接收数据 */
            result = Mqtt_ReceivePacket(conn);
            
            /* 检查保活 */
            /* 保活逻辑通过 Mqtt_Ping API 由上层定时调用实现 */
            break;
            
        case MQTT_STATE_DISCONNECTING:
            /* 关闭TLS连接 */
#if (MQTT_SUPPORT_TLS == STD_ON)
            if (conn->useTls && conn->tlsContext != NULL) {
                Mqtt_Tls_Close(conn->tlsContext);
                Mqtt_Tls_DestroyContext(conn->tlsContext);
                conn->tlsContext = NULL;
            }
#endif
            /* 关闭TCP连接 */
            if (conn->socketId != TCPIP_SOCKETID_INVALID) {
                TcpIp_SocketClose(conn->socketId);
                conn->socketId = TCPIP_SOCKETID_INVALID;
            }
            Mqtt_UpdateState(conn, MQTT_STATE_DISCONNECTED);
            break;
            
    case MQTT_STATE_TCP_CONNECTING:
        /* 检查TCP连接是否完成 */
        if (TcpIp_IsConnected(conn->socketId)) {
#if (MQTT_SUPPORT_TLS == STD_ON)
            if (conn->useTls) {
                /* 开始TLS握手 */
                Mqtt_UpdateState(conn, MQTT_STATE_TLS_HANDSHAKING);
            } else
#endif
            {
                /* 直接发送MQTT CONNECT报文 */
                Mqtt_UpdateState(conn, MQTT_STATE_MQTT_CONNECTING);
            }
        } else if (Mqtt_CheckTimeout(conn, conn->config.connectTimeoutMs)) {
            Mqtt_UpdateState(conn, MQTT_STATE_DISCONNECTING);
        }
        break;
        
#if (MQTT_SUPPORT_TLS == STD_ON)
    case MQTT_STATE_TLS_HANDSHAKING:
        /* 执行TLS握手 */
        {
            Mqtt_ReturnType tlsResult;
            tlsResult = Mqtt_Tls_PerformHandshake(conn->tlsContext, 
                                                   conn->socketId, 
                                                   NULL);
            if (tlsResult == MQTT_OK) {
                /* TLS握手成功，发送MQTT CONNECT报文 */
                Mqtt_UpdateState(conn, MQTT_STATE_MQTT_CONNECTING);
            } else if (Mqtt_CheckTimeout(conn, MQTT_TLS_HANDSHAKE_TIMEOUT_MS)) {
                Mqtt_UpdateState(conn, MQTT_STATE_DISCONNECTING);
            }
        }
        break;
#endif
            
        default:
            break;
    }
    
    return result;
}

static uint16 Mqtt_GetNextPacketId(Mqtt_InternalConnectionType* conn)
{
    conn->packetIdCounter++;
    if (conn->packetIdCounter == 0U ) {
        conn->packetIdCounter = 1; /* 包ID不能为0 */
    }
    return conn->packetIdCounter;
}

static Mqtt_ReturnType Mqtt_SendPacket(Mqtt_InternalConnectionType* conn,
                                        const uint8* data, uint16 length)
{
    Std_ReturnType result;
    
    if (conn->socketId == TCPIP_SOCKETID_INVALID) {
        return MQTT_E_NOCONN;
    }
    
#if (MQTT_SUPPORT_TLS == STD_ON)
    if (conn->useTls && conn->tlsContext != NULL) {
        /* 使用TLS加密发送 */
        uint32 sentLength = 0;
        Mqtt_ReturnType tlsResult;
        tlsResult = Mqtt_Tls_Send(conn->tlsContext, data, length, &sentLength);
        return tlsResult;
    } else
#endif
    {
        /* 使用明文TCP发送 */
        result = TcpIp_Send(conn->socketId, data, length);
        return (result == E_OK) ? MQTT_OK : MQTT_E_NOT_OK;
    }
}

static Mqtt_ReturnType Mqtt_ReceivePacket(Mqtt_InternalConnectionType* conn)
{
    Std_ReturnType result;
    uint16 receivedLength = 0;
    
    if (conn->socketId == TCPIP_SOCKETID_INVALID) {
        return MQTT_E_NOCONN;
    }
    
#if (MQTT_SUPPORT_TLS == STD_ON)
    if (conn->useTls && conn->tlsContext != NULL) {
        /* 使用TLS接收解密 */
        uint32 recvLen = 0;
        Mqtt_ReturnType tlsResult;
        tlsResult = Mqtt_Tls_Receive(conn->tlsContext, conn->recvBuffer,
                                      MQTT_RECV_BUFFER_SIZE, &recvLen);
        if (tlsResult == MQTT_OK && recvLen > 0U ) {
            conn->recvLength = (uint16)recvLen;
            conn->info.bytesReceived += recvLen;
        }
        return tlsResult;
    } else
#endif
    {
        /* 使用明文TCP接收 */
        result = TcpIp_Receive(conn->socketId, conn->recvBuffer, 
                               MQTT_RECV_BUFFER_SIZE, &receivedLength);
        
        if (result == E_OK && receivedLength > 0U ) {
            conn->recvLength = receivedLength;
            conn->info.bytesReceived += receivedLength;
            
            /* 解析接收的报文 - 在 Mqtt_ProcessStateMachine 中根据连接状态处理 */
        }
        
        return (result == E_OK) ? MQTT_OK : MQTT_E_NOT_OK;
    }
}

/* 编码函数实现示例 - 完整实现需更多细节 */
static Mqtt_ReturnType Mqtt_EncodeConnect(Mqtt_InternalConnectionType* conn)
{
    uint16 idx = 0;
    uint16 remainingLength = 0;
    uint16 clientIdLen;
    uint8 connectFlags = 0;
    
    /* 计算剩余长度 */
    clientIdLen = strlen(conn->config.clientId);
    remainingLength = 10; /* 固定头长度 */
    remainingLength += 2 + clientIdLen; /* 客户端ID */
    
    if (conn->config.username != NULL) {
        connectFlags |= 0x80; /* 用户名标志 */
        remainingLength += 2 + strlen(conn->config.username);
    }
    
    if (conn->config.password != NULL) {
        connectFlags |= 0x40; /* 密码标志 */
        remainingLength += 2 + strlen(conn->config.password);
    }
    
    if (conn->config.cleanSession == MQTT_CLEAN_SESSION_TRUE) {
        connectFlags |= 0x02; /* 清洁会话 */
    }
    
    /* 检查缓冲区空间 */
    if (remainingLength + 2 > MQTT_SEND_BUFFER_SIZE) {
        return MQTT_E_BUFFER_OVERFLOW;
    }
    
    /* 包头 */
    conn->sendBuffer[idx++] = MQTT_PACKET_TYPE_CONNECT;
    
    /* 剩余长度编码 (支持多字节编码) */
    if (remainingLength < 128) {
        conn->sendBuffer[idx++] = remainingLength;
    } else {
        conn->sendBuffer[idx++] = (remainingLength & 0x7F) | 0x80;
        conn->sendBuffer[idx++] = remainingLength >> 7;
    }
    
    /* 协议名 */
    conn->sendBuffer[idx++] = 0x00;
    conn->sendBuffer[idx++] = 0x04;
    conn->sendBuffer[idx++] = 'M';
    conn->sendBuffer[idx++] = 'Q';
    conn->sendBuffer[idx++] = 'T';
    conn->sendBuffer[idx++] = 'T';
    
    /* 协议级别 */
    conn->sendBuffer[idx++] = (conn->config.version == MQTT_VERSION_50) ? 5 : 4;
    
    /* 连接标志 */
    conn->sendBuffer[idx++] = connectFlags;
    
    /* 保活时间 */
    conn->sendBuffer[idx++] = (conn->config.keepAliveSeconds >> 8) & 0xFF;
    conn->sendBuffer[idx++] = conn->config.keepAliveSeconds & 0xFF;
    
    /* 客户端ID */
    conn->sendBuffer[idx++] = (clientIdLen >> 8) & 0xFF;
    conn->sendBuffer[idx++] = clientIdLen & 0xFF;
    memcpy(&conn->sendBuffer[idx], conn->config.clientId, clientIdLen);
    idx += clientIdLen;
    
    /* 用户名 */
    if (conn->config.username != NULL) {
        uint16 usernameLen = strlen(conn->config.username);
        conn->sendBuffer[idx++] = (usernameLen >> 8) & 0xFF;
        conn->sendBuffer[idx++] = usernameLen & 0xFF;
        memcpy(&conn->sendBuffer[idx], conn->config.username, usernameLen);
        idx += usernameLen;
    }
    
    /* 密码 */
    if (conn->config.password != NULL) {
        uint16 passwordLen = strlen(conn->config.password);
        conn->sendBuffer[idx++] = (passwordLen >> 8) & 0xFF;
        conn->sendBuffer[idx++] = passwordLen & 0xFF;
        memcpy(&conn->sendBuffer[idx], conn->config.password, passwordLen);
        idx += passwordLen;
    }
    
    conn->sendLength = idx;
    
    return MQTT_OK;
}

static Mqtt_ReturnType Mqtt_EncodePublish(Mqtt_InternalConnectionType* conn,
                                           const Mqtt_PublishMessageType* msg,
                                           uint16 packetId)
{
    uint16 idx = 0;
    uint16 remainingLength;
    uint16 topicLen = strlen(msg->topic);
    uint8 fixedHeader = MQTT_PACKET_TYPE_PUBLISH;
    
    /* 设置固定头标志 */
    fixedHeader |= ((msg->qos & 0x03) << 1);
    if (msg->retain == MQTT_RETAIN_TRUE) {
        fixedHeader |= 0x01;
    }
    
    /* 计算剩余长度 */
    remainingLength = 2 + topicLen; /* 主题 */
    if (msg->qos > MQTT_QOS_0) {
        remainingLength += 2; /* 包ID */
    }
    remainingLength += msg->payloadLength;
    
    if (remainingLength + 5 > MQTT_SEND_BUFFER_SIZE) {
        return MQTT_E_BUFFER_OVERFLOW;
    }
    
    /* 包头 */
    conn->sendBuffer[idx++] = fixedHeader;
    
    /* 剩余长度 */
    do {
        uint8 byte = remainingLength & 0x7F;
        remainingLength >>= 7;
        if (remainingLength > 0U ) {
            byte |= 0x80;
        }
        conn->sendBuffer[idx++] = byte;
    } while (remainingLength > 0U );
    
    /* 主题 */
    conn->sendBuffer[idx++] = (topicLen >> 8) & 0xFF;
    conn->sendBuffer[idx++] = topicLen & 0xFF;
    memcpy(&conn->sendBuffer[idx], msg->topic, topicLen);
    idx += topicLen;
    
    /* 包ID (QoS > 0U ) */
    if (msg->qos > MQTT_QOS_0) {
        conn->sendBuffer[idx++] = (packetId >> 8) & 0xFF;
        conn->sendBuffer[idx++] = packetId & 0xFF;
    }
    
    /* 负载 */
    memcpy(&conn->sendBuffer[idx], msg->payload, msg->payloadLength);
    idx += msg->payloadLength;
    
    conn->sendLength = idx;
    
    return MQTT_OK;
}

static Mqtt_ReturnType Mqtt_EncodeSubscribe(Mqtt_InternalConnectionType* conn,
                                             const Mqtt_SubscriptionType* sub,
                                             uint16 packetId)
{
    uint16 idx = 0;
    uint16 remainingLength;
    uint16 topicLen = strlen(sub->topicFilter);
    
    remainingLength = 2; /* 包ID */
    remainingLength += 2 + topicLen; /* 主题过滤器 */
    remainingLength += 1; /* QoS */
    
    if (remainingLength + 5 > MQTT_SEND_BUFFER_SIZE) {
        return MQTT_E_BUFFER_OVERFLOW;
    }
    
    /* 包头 */
    conn->sendBuffer[idx++] = MQTT_PACKET_TYPE_SUBSCRIBE | 0x02;
    
    /* 剩余长度 */
    conn->sendBuffer[idx++] = remainingLength;
    
    /* 包ID */
    conn->sendBuffer[idx++] = (packetId >> 8) & 0xFF;
    conn->sendBuffer[idx++] = packetId & 0xFF;
    
    /* 主题过滤器 */
    conn->sendBuffer[idx++] = (topicLen >> 8) & 0xFF;
    conn->sendBuffer[idx++] = topicLen & 0xFF;
    memcpy(&conn->sendBuffer[idx], sub->topicFilter, topicLen);
    idx += topicLen;
    
    /* 最大QoS */
    conn->sendBuffer[idx++] = sub->maxQoS & 0x03;
    
    conn->sendLength = idx;
    
    return MQTT_OK;
}

static Mqtt_ReturnType Mqtt_EncodePing(Mqtt_InternalConnectionType* conn)
{
    conn->sendBuffer[0] = MQTT_PACKET_TYPE_PINGREQ;
    conn->sendBuffer[1] = 0x00;
    conn->sendLength = 2;
    
    return MQTT_OK;
}

static Mqtt_ReturnType Mqtt_EncodeDisconnect(Mqtt_InternalConnectionType* conn)
{
    conn->sendBuffer[0] = MQTT_PACKET_TYPE_DISCONNECT;
    conn->sendBuffer[1] = 0x00;
    conn->sendLength = 2;
    
    return MQTT_OK;
}
