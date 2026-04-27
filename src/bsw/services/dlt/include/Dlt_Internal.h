/**
 * @file Dlt_Internal.h
 * @brief DLT 模块内部类型和数据结构定义
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 * @version 1.0.0
 */

#ifndef DLT_INTERNAL_H
#define DLT_INTERNAL_H

#include "Dlt.h"

/* ========================================================================== */
/*                          内部常量定义                                       */
/* ========================================================================== */

/**
 * @brief 最大应用数量
 */
#define DLT_MAX_APPS  32U

/**
 * @brief 最大消息队列大小
 */
#define DLT_MAX_QUEUE_SIZE  256U

/**
 * @brief 默认缓冲区大小
 */
#define DLT_DEFAULT_BUFFER_SIZE  4096U

/**
 * @brief 最大消息大小
 */
#define DLT_MAX_MESSAGE_SIZE  1400U

/**
 * @brief DLT 协议版本
 */
#define DLT_PROTOCOL_VERSION  0x01U

/**
 * @brief DLT 模式标识
 */
#define DLT_PATTERN  0x01U

/**
 * @brief 大端序标识
 */
#define DLT_ENDIANESS_BE  0x01U

/**
 * @brief 小端序标识
 */
#define DLT_ENDIANESS_LE  0x00U

/* ========================================================================== */
/*                          内部数据结构                                       */
/* ========================================================================== */

/**
 * @brief 应用注册表项
 */
typedef struct {
    Dlt_AppHandleType  handle;          /**< 应用句柄 */
    Dlt_AppInfoType    info;            /**< 应用信息 */
    Dlt_LogLevelType   currentLogLevel; /**< 当前日志级别 */
    boolean            isActive;        /**< 是否激活 */
} Dlt_AppEntryType;

/**
 * @brief 消息队列项
 */
typedef struct {
    Dlt_MessageHeaderType header;     /**< 消息头 */
    uint8                 payload[DLT_MAX_MESSAGE_SIZE]; /**< 负载数据 */
    uint16                payloadLen; /**< 负载长度 */
    boolean               pending;    /**< 是否待发送 */
    Dlt_PriorityType      priority;   /**< 消息优先级 */
} Dlt_QueueEntryType;

/**
 * @brief DLT 内部状态
 */
typedef struct {
    Dlt_ModuleStateType       moduleState;      /**< 模块状态 */
    const Dlt_ConfigType*     config;           /**< 配置指针 */
    Dlt_AppEntryType          appTable[DLT_MAX_APPS]; /**< 应用表 */
    uint16                    appCount;         /**< 应用数量 */
    Dlt_QueueEntryType        queue[DLT_MAX_QUEUE_SIZE]; /**< 消息队列 */
    uint16                    queueHead;        /**< 队列头 */
    uint16                    queueTail;        /**< 队列尾 */
    uint16                    queueCount;       /**< 队列计数 */
    uint32                    totalMessagesSent; /**< 总发送消息数 */
    uint32                    totalMessagesDropped; /**< 总丢弃消息数 */
    uint32                    currentSessionId; /**< 当前会话 ID */
    uint32                    sequenceCounter;  /**< 序列计数器 */
    uint64                    lastTimestamp;    /**< 最后时间戳 */
} Dlt_InternalStateType;

/* ========================================================================== */
/*                          内部函数声明                                       */
/* ========================================================================== */

/**
 * @brief 构建 DLT 消息头
 * 
 * @param header 指向消息头的指针
 * @param appEntry 指向应用条目的指针
 * @param msgType 消息类型
 * @param subtype 子类型
 * @param messageId 消息 ID
 * @param payloadLen 负载长度
 * 
 * @return void
 */
void Dlt_BuildMessageHeader(
    Dlt_MessageHeaderType* header,
    const Dlt_AppEntryType* appEntry,
    uint8 msgType,
    uint8 subtype,
    uint16 messageId,
    uint16 payloadLen
);

/**
 * @brief 将消息入队
 * 
 * @param queueEntry 指向队列项的指针
 * 
 * @return Std_ReturnType
 * @retval E_OK 入队成功
 * @retval E_NOT_OK 队列满
 */
Std_ReturnType Dlt_EnqueueMessage(const Dlt_QueueEntryType* queueEntry);

/**
 * @brief 将消息出队
 * 
 * @param queueEntry 指向队列项的指针
 * 
 * @return Std_ReturnType
 * @retval E_OK 出队成功
 * @retval E_NOT_OK 队列空
 */
Std_ReturnType Dlt_DequeueMessage(Dlt_QueueEntryType* queueEntry);

/**
 * @brief 通过 UDP 发送消息
 * 
 * @param data 指向数据的指针
 * @param length 数据长度
 * 
 * @return Std_ReturnType
 * @retval E_OK 发送成功
 * @retval E_NOT_OK 发送失败
 */
Std_ReturnType Dlt_UdpSend(const uint8* data, uint16 length);

/**
 * @brief 传输层发送接口 (抽象)
 * 
 * @param data 指向数据的指针
 * @param length 数据长度
 * @param protocol 传输协议
 * 
 * @return Std_ReturnType
 */
Std_ReturnType Dlt_TransportSend(
    const uint8* data,
    uint16 length,
    Dlt_TransportProtocolType protocol);

/**
 * @brief 应用消息过滤器
 * 
 * @param appHandle 应用句柄
 * @param logLevel 日志级别
 * 
 * @return boolean
 * @retval TRUE 消息应被发送
 * @retval FALSE 消息应被过滤
 */
boolean Dlt_ApplyFilter(Dlt_AppHandleType appHandle, Dlt_LogLevelType logLevel);

/**
 * @brief 查找应用条目
 * 
 * @param appHandle 应用句柄
 * 
 * @return Dlt_AppEntryType* 指向应用条目的指针，未找到返回 NULL
 */
Dlt_AppEntryType* Dlt_FindAppEntry(Dlt_AppHandleType appHandle);

/**
 * @brief 分配应用句柄
 * 
 * @return Dlt_AppHandleType 新应用句柄
 */
Dlt_AppHandleType Dlt_AllocateAppHandle(void);

/**
 * @brief 释放应用句柄
 * 
 * @param appHandle 应用句柄
 * 
 * @return void
 */
void Dlt_FreeAppHandle(Dlt_AppHandleType appHandle);

/**
 * @brief 获取时间戳 (微秒)
 * 
 * @return uint64 时间戳
 */
uint64 Dlt_GetTimestampUs(void);

/**
 * @brief 获取下一个序列号
 * 
 * @return uint32 序列号
 */
uint32 Dlt_GetNextSequenceNumber(void);

#endif /* DLT_INTERNAL_H */
