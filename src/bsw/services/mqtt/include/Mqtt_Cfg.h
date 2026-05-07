/** @file Mqtt_Cfg.h
 * @brief MQTT 模块配置头文件
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 通过配置工具生成，手动修改可能被覆盖
 */

#ifndef MQTT_CFG_H
#define MQTT_CFG_H

/*============================================================================
 * 版本检查
 *===========================================================================*/
#define MQTT_CFG_MAJOR_VERSION    (1)
#define MQTT_CFG_MINOR_VERSION    (0)
#define MQTT_CFG_PATCH_VERSION    (0)

/*============================================================================
 * 预处理配置
 *===========================================================================*/
/**
 * @brief 开发错误检测使能
 */
#define MQTT_DEV_ERROR_DETECT     (STD_ON)

/**
 * @brief 版本信息API使能
 */
#define MQTT_VERSION_INFO_API     (STD_ON)

/**
 * @brief 调试模式使能
 */
#define MQTT_DEBUG_MODE           (STD_OFF)

/*============================================================================
 * 功能配置
 *===========================================================================*/
/**
 * @brief 支持MQTT 5.0协议
 */
#define MQTT_SUPPORT_V50          (STD_ON)

/**
 * @brief 支持SSL/TLS加密连接
 */
#define MQTT_SUPPORT_TLS          (STD_OFF)

/**
 * @brief 支持自动重连
 */
#define MQTT_SUPPORT_AUTO_RECONNECT  (STD_ON)

/**
 * @brief 支持通道级消息回调
 */
#define MQTT_SUPPORT_CHANNEL_CALLBACKS  (STD_ON)

/**
 * @brief 支持Will消息
 */
#define MQTT_SUPPORT_WILL_MESSAGE  (STD_ON)

/**
 * @brief 支持最后威胁消息
 */
#define MQTT_SUPPORT_LAST_WILL     (STD_ON)

/*============================================================================
 * 数量配置
 *===========================================================================*/
/**
 * @brief 最大连接数量
 */
#define MQTT_MAX_CONNECTIONS      (4U)

/**
 * @brief 每连接最大订阅数量
 */
#define MQTT_MAX_SUBSCRIPTIONS_PER_CONN  (8U)

/**
 * @brief 最大主题名长度(包含终止符)
 */
#define MQTT_MAX_TOPIC_LENGTH     (128U)

/**
 * @brief 最大客户端ID长度(包含终止符)
 */
#define MQTT_MAX_CLIENT_ID_LENGTH (64U)

/**
 * @brief 发送缓冲区大小(字节)
 */
#define MQTT_SEND_BUFFER_SIZE     (2048U)

/**
 * @brief 接收缓冲区大小(字节)
 */
#define MQTT_RECV_BUFFER_SIZE     (2048U)

/**
 * @brief 消息队列深度
 */
#define MQTT_MESSAGE_QUEUE_DEPTH  (8U)

/**
 * @brief 主循环调用周期(毫秒)
 */
#define MQTT_MAIN_FUNCTION_PERIOD_MS  (10U)

/*============================================================================
 * 默认值配置
 *===========================================================================*/
/**
 * @brief 默认保活时间(秒)
 */
#define MQTT_DEFAULT_KEEP_ALIVE_S    (60U)

/**
 * @brief 默认连接超时(毫秒)
 */
#define MQTT_DEFAULT_CONNECT_TIMEOUT_MS  (5000U)

/**
 * @brief 默认接收超时(毫秒)
 */
#define MQTT_DEFAULT_RECV_TIMEOUT_MS     (1000U)

/**
 * @brief 默认重连间隔(毫秒)
 */
#define MQTT_DEFAULT_RECONNECT_INTERVAL_MS  (5000U)

/**
 * @brief 最大重连尝试次数
 */
#define MQTT_MAX_RECONNECT_ATTEMPTS  (5U)

/*============================================================================
 * 内存映射
 *===========================================================================*/
/**
 * @brief 代码段映射
 */
#if !defined(MQTT_CODE)
#define MQTT_CODE
#endif

/**
 * @brief 常量数据段映射
 */
#if !defined(MQTT_CONST)
#define MQTT_CONST    const
#endif

/**
 * @brief 变量数据段映ేc5
 */
#if !defined(MQTT_VAR)
#define MQTT_VAR
#endif

/*============================================================================
 * 配置结构定义
 *===========================================================================*/

/**
 * @brief 通知回调配置
 */
typedef struct {
    void (*connectionStateChanged)(uint8 connectionId, uint8 newState);
    void (*messageReceived)(uint8 connectionId, const char* topic, 
                           const uint8* payload, uint32 length);
    void (*errorOccurred)(uint8 connectionId, uint8 errorCode);
} Mqtt_NotificationCallbacksType;

/**
 * @brief 模块级配置
 */
typedef struct {
    uint8 maxConnections;
    uint16 sendBufferSize;
    uint16 recvBufferSize;
    uint8 messageQueueDepth;
    boolean enableAutoReconnect;
    Mqtt_NotificationCallbacksType callbacks;
} Mqtt_ConfigType;

/*============================================================================
 * 外部依赖
 *===========================================================================*/
#include "ComStack_Types.h"
#include "TcpIp.h"

#if (MQTT_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

#endif /* MQTT_CFG_H */
