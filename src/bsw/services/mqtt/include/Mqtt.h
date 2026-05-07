/** @file Mqtt.h
 * @brief MQTT 客户端模块标准头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 本模块为AUTOSAR非标准扩展模块，提供MQTT 3.1.1和5.0协议支持
 * 用于车载系统与云服务的通信
 *
 * 架构位置:
 * - 层级: 服务层 (Services Layer)
 * - 下层依赖: SoAd (Socket Adapter) 或 TCP/IP Stack
 * - 上层用户: 应用软件组件 (ASWCs)
 *
 * 遵循AUTOSAR规范:
 * - SWS编号风格: Mqtt_<FunctionName>
 * - 错误处理: 通过Det模块报告
 * - 配置驱动: 链接时配置 (Lcfg)
 */

#ifndef MQTT_H
#define MQTT_H

/*============================================================================
 * 版本信息
 *===========================================================================*/
#define MQTT_VENDOR_ID                    (0x01FF)  /* YuleTech */
#define MQTT_MODULE_ID                    (0x00B0)  /* 自定义模块ID */

#define MQTT_SW_MAJOR_VERSION             (1)
#define MQTT_SW_MINOR_VERSION             (0)
#define MQTT_SW_PATCH_VERSION             (0)

#define MQTT_AR_RELEASE_MAJOR_VERSION     (4)
#define MQTT_AR_RELEASE_MINOR_VERSION     (4)
#define MQTT_AR_RELEASE_REVISION_VERSION  (0)

/*============================================================================
 * 包含文件
 *===========================================================================*/
#include "Std_Types.h"
#include "Mqtt_Cfg.h"

/*============================================================================
 * 宏定义
 *===========================================================================*/
/**
 * @name API服务ID
 * @{
 */
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
/** @} */

/**
 * @name 开发错误类型
 * @{
 */
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
/** @} */

/*============================================================================
 * 数据类型定义
 *===========================================================================*/

/**
 * @brief MQTT连接句柄类型
 */
typedef uint8 Mqtt_ConnectionIdType;

/**
 * @brief MQTT订阅ID类型
 */
typedef uint16 Mqtt_SubscriptionIdType;

/**
 * @brief MQTT连接状态
 */
typedef enum {
    MQTT_STATE_UNINIT = 0,      /**< 未初始化 */
    MQTT_STATE_DISCONNECTED,    /**< 已断开 */
    MQTT_STATE_CONNECTING,      /**< 连接中 */
    MQTT_STATE_CONNECTED,       /**< 已连接 */
    MQTT_STATE_DISCONNECTING,   /**< 断开中 */
    MQTT_STATE_RECONNECTING     /**< 重连中 */
} Mqtt_ConnectionStateType;

/**
 * @brief MQTT协议版本
 */
typedef enum {
    MQTT_VERSION_311 = 4,       /**< MQTT 3.1.1 */
    MQTT_VERSION_50  = 5        /**< MQTT 5.0 */
} Mqtt_ProtocolVersionType;

/**
 * @brief MQTT服务质量等级
 */
typedef enum {
    MQTT_QOS_0 = 0,             /**< 最多一次传输 */
    MQTT_QOS_1 = 1,             /**< 至少一次传输 */
    MQTT_QOS_2 = 2              /**< 仅一次传输 */
} Mqtt_QoSType;

/**
 * @brief MQTT保留标志
 */
typedef enum {
    MQTT_RETAIN_FALSE = 0,
    MQTT_RETAIN_TRUE  = 1
} Mqtt_RetainType;

/**
 * @brief MQTT清洁会话标志
 */
typedef enum {
    MQTT_CLEAN_SESSION_FALSE = 0,
    MQTT_CLEAN_SESSION_TRUE  = 1
} Mqtt_CleanSessionType;

/**
 * @brief 返回码类型
 */
typedef enum {
    MQTT_OK = 0,                /**< 成功 */
    MQTT_E_NOT_OK,              /**< 通用失败 */
    MQTT_E_BUSY,                /**< 模块忙 */
    MQTT_E_TIMEOUT,             /**< 超时 */
    MQTT_E_NOCONN,              /**< 未连接 */
    MQTT_E_INVtopic,            /**< 无效主题 */
    MQTT_E_INVPAYLOAD,          /**< 无效负载 */
    MQTT_E_BUFFERFULL,          /**< 缓冲区满 */
    MQTT_E_DISCONNECTED         /**< 已断开 */
} Mqtt_ReturnType;

/**
 * @brief 连接配置结构
 */
typedef struct {
    const char* brokerHost;                 /**< 代理地址 */
    uint16 brokerPort;                      /**< 代理端口 */
    const char* clientId;                   /**< 客户端ID */
    uint16 keepAliveSeconds;                /**< 保活时间(秒) */
    Mqtt_CleanSessionType cleanSession;     /**< 清洁会话 */
    Mqtt_ProtocolVersionType version;       /**< 协议版本 */
    const char* username;                   /**< 用户名(可为NULL) */
    const char* password;                   /**< 密码(可为NULL) */
    uint16 connectTimeoutMs;                /**< 连接超时(毫秒) */
    uint16 recvTimeoutMs;                   /**< 接收超时(毫秒) */
    uint16 sendTimeoutMs;                   /**< 发送超时(毫秒) */
    boolean autoReconnect;                  /**< 自动重连 */
    uint16 reconnectIntervalMs;             /**< 重连间隔(毫秒) */
} Mqtt_ConnectionConfigType;

/**
 * @brief 发布消息结构
 */
typedef struct {
    const char* topic;                      /**< 主题名 */
    const uint8* payload;                   /**< 负载数据 */
    uint32 payloadLength;                   /**< 负载长度 */
    Mqtt_QoSType qos;                       /**< 服务质量 */
    Mqtt_RetainType retain;                 /**< 保留标志 */
} Mqtt_PublishMessageType;

/**
 * @brief 接收消息结构
 */
typedef struct {
    const char* topic;                      /**< 主题名 */
    uint8* payload;                         /**< 负载数据 */
    uint32 payloadLength;                   /**< 负载长度 */
    Mqtt_QoSType qos;                       /**< 服务质量 */
    boolean retain;                         /**< 保留标志 */
    boolean dup;                            /**< 重复标志 */
} Mqtt_ReceivedMessageType;

/**
 * @brief 订阅信息结构
 */
typedef struct {
    const char* topicFilter;                /**< 主题过滤器 */
    Mqtt_QoSType maxQoS;                    /**< 最大QoS */
    Mqtt_SubscriptionIdType subscriptionId; /**< 订阅ID */
} Mqtt_SubscriptionType;

/**
 * @brief 连接信息结构(只读)
 */
typedef struct {
    Mqtt_ConnectionStateType state;         /**< 当前状态 */
    uint32 messagesSent;                    /**< 发送消息数 */
    uint32 messagesReceived;                /**< 接收消息数 */
    uint32 bytesSent;                       /**< 发送字节数 */
    uint32 bytesReceived;                   /**< 接收字节数 */
    uint32 connectCount;                    /**< 连接次数 */
    uint32 disconnectCount;                 /**< 断开次数 */
    uint32 reconnectCount;                  /**< 重连次数 */
    uint32 lastErrorCode;                   /**< 最后错误码 */
} Mqtt_ConnectionInfoType;

/**
 * @brief 回调函数类型
 */
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
 * 全局配置指针
 *===========================================================================*/
extern const Mqtt_ConfigType* Mqtt_ConfigPtr;

/*============================================================================
 * 公共API声明
 *===========================================================================*/

/**
 * @brief 初始化MQTT模块
 * @param config 配置数据指针
 * @return 操作结果
 * @req SWS_Mqtt_00001
 */
extern Mqtt_ReturnType Mqtt_Init(const Mqtt_ConfigType* config);

/**
 * @brief 反初始化MQTT模块
 * @return 操作结果
 * @req SWS_Mqtt_00002
 */
extern Mqtt_ReturnType Mqtt_DeInit(void);

/**
 * @brief 连接到MQTT代理
 * @param connectionId 连接ID
 * @param connConfig 连接配置
 * @return 操作结果
 * @req SWS_Mqtt_00003
 */
extern Mqtt_ReturnType Mqtt_Connect(
    Mqtt_ConnectionIdType connectionId,
    const Mqtt_ConnectionConfigType* connConfig
);

/**
 * @brief 从MQTT代理断开连接
 * @param connectionId 连接ID
 * @return 操作结果
 * @req SWS_Mqtt_00004
 */
extern Mqtt_ReturnType Mqtt_Disconnect(Mqtt_ConnectionIdType connectionId);

/**
 * @brief 发布消息
 * @param connectionId 连接ID
 * @param message 发布消息
 * @param callback 回调函数(可为NULL)
 * @return 操作结果
 * @req SWS_Mqtt_00005
 */
extern Mqtt_ReturnType Mqtt_Publish(
    Mqtt_ConnectionIdType connectionId,
    const Mqtt_PublishMessageType* message,
    Mqtt_PublishCallbackType callback
);

/**
 * @brief 订阅主题
 * @param connectionId 连接ID
 * @param subscription 订阅信息
 * @param msgCallback 消息回调
 * @return 操作结果
 * @req SWS_Mqtt_00006
 */
extern Mqtt_ReturnType Mqtt_Subscribe(
    Mqtt_ConnectionIdType connectionId,
    const Mqtt_SubscriptionType* subscription,
    Mqtt_MessageCallbackType msgCallback
);

/**
 * @brief 取消订阅主题
 * @param connectionId 连接ID
 * @param topicFilter 主题过滤器
 * @return 操作结果
 * @req SWS_Mqtt_00007
 */
extern Mqtt_ReturnType Mqtt_Unsubscribe(
    Mqtt_ConnectionIdType connectionId,
    const char* topicFilter
);

/**
 * @brief 发送PING请求(保活)
 * @param connectionId 连接ID
 * @return 操作结果
 * @req SWS_Mqtt_00008
 */
extern Mqtt_ReturnType Mqtt_Ping(Mqtt_ConnectionIdType connectionId);

/**
 * @brief 获取连接状态
 * @param connectionId 连接ID
 * @return 当前状态
 * @req SWS_Mqtt_00009
 */
extern Mqtt_ConnectionStateType Mqtt_GetConnectionState(
    Mqtt_ConnectionIdType connectionId
);

/**
 * @brief 获取连接信息
 * @param connectionId 连接ID
 * @param info 信息结构指针(输出)
 * @return 操作结果
 * @req SWS_Mqtt_00010
 */
extern Mqtt_ReturnType Mqtt_GetConnectionInfo(
    Mqtt_ConnectionIdType connectionId,
    Mqtt_ConnectionInfoType* info
);

/**
 * @brief 主循环函数
 * @req SWS_Mqtt_00011
 */
extern void Mqtt_MainFunction(void);

/**
 * @brief 设置连接状态变化回调
 * @param connectionId 连接ID
 * @param callback 回调函数
 * @req SWS_Mqtt_00012
 */
extern void Mqtt_SetConnectionCallback(
    Mqtt_ConnectionIdType connectionId,
    Mqtt_ConnectionCallbackType callback
);

/**
 * @brief 获取版本信息
 * @param versioninfo 版本信息指针
 * @req SWS_Mqtt_00013
 */
#if (MQTT_VERSION_INFO_API == STD_ON)
extern void Mqtt_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#endif /* MQTT_H */
