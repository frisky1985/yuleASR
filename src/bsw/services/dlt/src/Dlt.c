/**
 * @file Dlt.c
 * @brief DLT (Diagnostic Log and Trace) 模块实现
 * 
 * 符合 AutoSAR Classic Platform 4.x 规范的诊断日志和跟踪服务实现
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 * @version 1.0.0
 */

#include "Dlt.h"
#include "Dlt_Internal.h"
#include "Dlt_Cfg.h"
#include "Det.h"
#include <string.h>
#include <stdint.h>

/* ========================================================================== */
/*                          全局变量                                           */
/* ========================================================================== */

/**
 * @brief DLT 内部状态
 */
static Dlt_InternalStateType g_DltState = {
    .moduleState = DLT_STATE_UNINIT,
    .config = NULL_PTR,
    .appTable = {0},
    .appCount = 0U,
    .queue = {0},
    .queueHead = 0U,
    .queueTail = 0U,
    .queueCount = 0U,
    .totalMessagesSent = 0U,
    .totalMessagesDropped = 0U,
    .currentSessionId = DLT_DEFAULT_SESSION_ID,
    .sequenceCounter = 0U,
    .lastTimestamp = 0U
};

/**
 * @brief 下一个可用的应用句柄
 */
static Dlt_AppHandleType g_NextAppHandle = 1U;

/* ========================================================================== */
/*                          配置数据定义                                       */
/* ========================================================================== */

/* 链接期配置已迁移至 Dlt_Lcfg.c（AUTOSAR 三层配置结构，2026-08-15 治理）:
 * - Dlt_TransportConfig / Dlt_FilterConfigTable / Dlt_FilterConfigCount
 * - Dlt_Config（含默认过滤器表）
 * - context 配置体系（Dlt_ContextConfig / Dlt_RuntimeContext, ecual 合并）
 * 此处仅保留 extern 声明引用（见 Dlt_Cfg.h NON-MACRO SEGMENT）。 */

/**
 * @brief 链接期 context 配置表 (Dlt_Lcfg.c 定义)
 */
extern const Dlt_ContextType Dlt_ContextConfig[DLT_MAX_CONTEXT_COUNT];

/**
 * @brief 运行时 context 表 (Dlt_Lcfg.c 定义, Dlt_Init 时由配置表初始化)
 */
extern Dlt_ContextType Dlt_RuntimeContext[DLT_MAX_CONTEXT_COUNT];

/* ========================================================================== */
/*                          API 函数实现                                       */
/* ========================================================================== */

/** @req SWS_Dlt_00001 */
/**
 * @brief 初始化 DLT 模块
 */
void Dlt_Init(const Dlt_ConfigType* ConfigPtr)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_INIT, DLT_E_PARAM_POINTER);
        return;
    }
#endif

    /* 检查是否已初始化 */
    if (g_DltState.moduleState != DLT_STATE_UNINIT) {
        return;
    }

    /* 初始化内部状态 */
    g_DltState.config = ConfigPtr;
    g_DltState.moduleState = DLT_STATE_INIT;
    g_DltState.appCount = 0U;
    g_DltState.queueHead = 0U;
    g_DltState.queueTail = 0U;
    g_DltState.queueCount = 0U;
    g_DltState.totalMessagesSent = 0U;
    g_DltState.totalMessagesDropped = 0U;
    g_NextAppHandle = 1U;

    /* 清空应用表 */
    (void)memset(g_DltState.appTable, 0, sizeof(g_DltState.appTable));

    /* 清空消息队列 */
    (void)memset(g_DltState.queue, 0, sizeof(g_DltState.queue));

    /* 初始化运行时 context 表 (由链接期配置表复制, ecual 合并 2026-08-15) */
    (void)memcpy(Dlt_RuntimeContext, Dlt_ContextConfig, sizeof(Dlt_ContextConfig));

    /* 更新状态为就绪 */
    g_DltState.moduleState = DLT_STATE_READY;
}

/** @req SWS_Dlt_00002 */
/**
 * @brief 反初始化 DLT 模块
 */
void Dlt_DeInit(void)
{
    /* 检查模块状态 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_DEINIT, DLT_E_UNINIT);
        return;
    }
#endif

    /* 清空所有状态 */
    g_DltState.config = NULL_PTR;
    g_DltState.appCount = 0U;
    g_DltState.queueCount = 0U;
    g_DltState.moduleState = DLT_STATE_UNINIT;
}

/**
 * @brief 注册应用到 DLT 模块
 */
Dlt_AppHandleType Dlt_RegisterApp(const Dlt_AppInfoType* AppInfoPtr)
{
    Dlt_AppHandleType handle = DLT_INVALID_APP_HANDLE;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_REGISTER_APP, DLT_E_UNINIT);
        return DLT_INVALID_APP_HANDLE;
    }

    if (AppInfoPtr == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_REGISTER_APP, DLT_E_PARAM_POINTER);
        return DLT_INVALID_APP_HANDLE;
    }
#endif

    /* 检查应用数量是否已满 */
    if (g_DltState.appCount >= DLT_MAX_APPS) {
        return DLT_INVALID_APP_HANDLE;
    }

    /* 分配新句柄 */
    handle = Dlt_AllocateAppHandle();
    if (handle == DLT_INVALID_APP_HANDLE) {
        return DLT_INVALID_APP_HANDLE;
    }

    /* 查找空位并注册应用 */
    for (uint16 i = 0U; i < DLT_MAX_APPS; i++) {
        if (!g_DltState.appTable[i].isActive) {
            g_DltState.appTable[i].handle = handle;
            g_DltState.appTable[i].info = *AppInfoPtr;
            g_DltState.appTable[i].currentLogLevel = DLT_DEFAULT_LOG_LEVEL;
            g_DltState.appTable[i].isActive = TRUE;
            g_DltState.appCount++;
            break;
        }
    }

    return handle;
}

/** @req SWS_Dlt_00005 */
/**
 * @brief 注销应用
 */
Std_ReturnType Dlt_UnregisterApp(Dlt_AppHandleType AppHandle)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_UNREGISTER_APP, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (AppHandle == DLT_INVALID_APP_HANDLE) {
        DLT_DETECT_ERROR(DLT_APIID_UNREGISTER_APP, DLT_E_INVALID_HANDLE);
        return E_NOT_OK;
    }
#endif

    /* 查找并删除应用 */
    Dlt_AppEntryType* appEntry = Dlt_FindAppEntry(AppHandle);
    if (appEntry == NULL_PTR) {
        return E_NOT_OK;
    }

    /* 释放应用条目 */
    appEntry->isActive = FALSE;
    Dlt_FreeAppHandle(AppHandle);
    g_DltState.appCount--;

    return E_OK;
}

/** @req SWS_Dlt_00006 */
/**
 * @brief 发送日志消息
 */
Std_ReturnType Dlt_SendLogMessage(
    Dlt_AppHandleType  AppHandle,
    Dlt_LogLevelType   LogLevel,
    Dlt_MessageIdType  MessageId,
    const uint8*       DataPtr,
    uint16             Length)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_LOG, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (AppHandle == DLT_INVALID_APP_HANDLE) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_LOG, DLT_E_INVALID_HANDLE);
        return E_NOT_OK;
    }

    if (DataPtr == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_LOG, DLT_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Length > DLT_MAX_MSG_SIZE) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_LOG, DLT_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* 应用过滤器 */
    if (!Dlt_ApplyFilter(AppHandle, LogLevel)) {
        return E_OK; /* 消息被过滤，但不算错误 */
    }

    /* 查找应用条目 */
    Dlt_AppEntryType* appEntry = Dlt_FindAppEntry(AppHandle);
    if (appEntry == NULL_PTR) {
        return E_NOT_OK;
    }

    /* 构建消息头 */
    Dlt_MessageHeaderType header;
    Dlt_BuildMessageHeader(&header, appEntry, 
                          DLT_MSG_TYPE_LOG, 
                          (uint8)LogLevel,
                          MessageId, 
                          Length);

    /* 添加时间戳和会话ID */
#if (DLT_TIMESTAMP_ENABLED == STD_ON)
    header.timestamp = Dlt_GetTimestampUs();
#endif

#if (DLT_SESSION_ID_ENABLED == STD_ON)
    header.sessionId = g_DltState.currentSessionId;
#endif

    /* 序列号 */
    header.sequenceCounter = Dlt_GetNextSequenceNumber();

    /* 构建队列项 */
    Dlt_QueueEntryType queueEntry;
    queueEntry.header = header;
    queueEntry.payloadLen = Length;
    (void)memcpy(queueEntry.payload, DataPtr, Length);
    queueEntry.pending = TRUE;
    queueEntry.priority = appEntry->info.priority;

    /* 入队 */
    return Dlt_EnqueueMessage(&queueEntry);
}

/** @req SWS_Dlt_00007 */
/**
 * @brief 发送跟踪消息
 */
Std_ReturnType Dlt_SendTraceMessage(
    Dlt_AppHandleType AppHandle,
    Dlt_TraceType     TraceType,
    Dlt_MessageIdType TraceId,
    const uint8*      DataPtr,
    uint16            Length)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_TRACE, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (AppHandle == DLT_INVALID_APP_HANDLE) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_TRACE, DLT_E_INVALID_HANDLE);
        return E_NOT_OK;
    }

    if (DataPtr == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_TRACE, DLT_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (Length > DLT_MAX_MSG_SIZE) {
        DLT_DETECT_ERROR(DLT_APIID_SEND_TRACE, DLT_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* 查找应用条目 */
    Dlt_AppEntryType* appEntry = Dlt_FindAppEntry(AppHandle);
    if (appEntry == NULL_PTR) {
        return E_NOT_OK;
    }

    /* 构建消息头 */
    Dlt_MessageHeaderType header;
    Dlt_BuildMessageHeader(&header, appEntry,
                          DLT_MSG_TYPE_TRACE,
                          (uint8)TraceType,
                          TraceId,
                          Length);

    /* 添加时间戳和会话ID */
#if (DLT_TIMESTAMP_ENABLED == STD_ON)
    header.timestamp = Dlt_GetTimestampUs();
#endif

#if (DLT_SESSION_ID_ENABLED == STD_ON)
    header.sessionId = g_DltState.currentSessionId;
#endif

    /* 序列号 */
    header.sequenceCounter = Dlt_GetNextSequenceNumber();

    /* 构建队列项 */
    Dlt_QueueEntryType queueEntry;
    queueEntry.header = header;
    queueEntry.payloadLen = Length;
    (void)memcpy(queueEntry.payload, DataPtr, Length);
    queueEntry.pending = TRUE;
    queueEntry.priority = appEntry->info.priority;

    /* 入队 */
    return Dlt_EnqueueMessage(&queueEntry);
}

/** @req SWS_Dlt_00004 */
/**
 * @brief DLT 主函数
 */
void Dlt_MainFunction(void)
{
    /* 检查模块状态 */
    if (g_DltState.moduleState != DLT_STATE_READY) {
        return;
    }

    /* 处理消息队列 */
    Dlt_QueueEntryType queueEntry;
    while (Dlt_DequeueMessage(&queueEntry) == E_OK) {
        /* 构建完整消息 */
        uint8 messageBuffer[DLT_MAX_MSG_SIZE + sizeof(Dlt_MessageHeaderType)];
        uint16 totalLength = sizeof(Dlt_MessageHeaderType) + queueEntry.payloadLen;

        /* 复制消息头 */
        (void)memcpy(messageBuffer, &queueEntry.header, sizeof(Dlt_MessageHeaderType));
        
        /* 复制负载 */
        if (queueEntry.payloadLen > 0U) {
            (void)memcpy(&messageBuffer[sizeof(Dlt_MessageHeaderType)],
                        queueEntry.payload,
                        queueEntry.payloadLen);
        }

        /* 通过传输层发送 (使用抽象接口) */
        Dlt_TransportProtocolType protocol = DLT_TRANSPORT_UDP;
        if ((g_DltState.config != NULL_PTR) && (g_DltState.config->transportConfig != NULL_PTR)) {
            protocol = g_DltState.config->transportConfig->protocol;
        }
        
        Std_ReturnType result = Dlt_TransportSend(messageBuffer, totalLength, protocol);
        
        if (result == E_OK) {
            g_DltState.totalMessagesSent++;
        } else {
            g_DltState.totalMessagesDropped++;
            
            /* 报告传输错误 */
#if (DLT_RUNTIME_ERROR_REPORT == STD_ON)
            /* 报告运行时错误 */
            /* 运行时错误通过 Det 模块报告 - 当前 DLT 传输错误已记录在统计计数器中 */
#endif
        }
    }
    
    /* 检查消息丢失 */
#if (DLT_MESSAGE_LOSS_REPORT == STD_ON)
    if (g_DltState.totalMessagesDropped > DLT_MESSAGE_LOSS_THRESHOLD) {
        /* 报告消息丢失事件 */
        /* 消息丢失事件通过 Dlt_GetStatistics API 查询 - 丢包统计在 totalMessagesDropped 中 */
    }
#endif
}

/** @req SWS_Dlt_00003 */
/**
 * @brief 获取 DLT 模块版本信息
 */
void Dlt_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfoPtr == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_GET_VERSION, DLT_E_PARAM_POINTER);
        return;
    }
#endif

    if (VersionInfoPtr != NULL_PTR) {
        VersionInfoPtr->vendorID = DLT_VENDOR_ID;
        VersionInfoPtr->moduleID = DLT_MODULE_ID;
        VersionInfoPtr->sw_major_version = DLT_SW_MAJOR_VERSION;
        VersionInfoPtr->sw_minor_version = DLT_SW_MINOR_VERSION;
        VersionInfoPtr->sw_patch_version = DLT_SW_PATCH_VERSION;
    }
}

/** @req SWS_Dlt_00008 */
/**
 * @brief 设置消息过滤器
 */
Std_ReturnType Dlt_SetFilter(
    Dlt_AppHandleType AppHandle,
    Dlt_LogLevelType  LogLevel,
    boolean           Enabled)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_SET_FILTER, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (AppHandle == DLT_INVALID_APP_HANDLE) {
        DLT_DETECT_ERROR(DLT_APIID_SET_FILTER, DLT_E_INVALID_HANDLE);
        return E_NOT_OK;
    }
#endif

    /* 查找应用条目 */
    Dlt_AppEntryType* appEntry = Dlt_FindAppEntry(AppHandle);
    if (appEntry == NULL_PTR) {
        return E_NOT_OK;
    }

    /* 更新过滤器配置 */
    appEntry->currentLogLevel = LogLevel;
    appEntry->isActive = Enabled;

    return E_OK;
}

/** @req SWS_Dlt_00009 */
/**
 * @brief 清空消息队列
 */
Std_ReturnType Dlt_FlushQueue(void)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_FLUSH_QUEUE, DLT_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    /* 清空队列 */
    g_DltState.queueHead = 0U;
    g_DltState.queueTail = 0U;
    g_DltState.queueCount = 0U;
    (void)memset(g_DltState.queue, 0, sizeof(g_DltState.queue));

    return E_OK;
}

/**
 * @brief 获取模块状态
 */
Dlt_ModuleStateType Dlt_GetStatus(void)
{
    return g_DltState.moduleState;
}

/* ========================================================================== */
/*                          内部函数实现                                       */
/* ========================================================================== */

/**
 * @brief 构建 DLT 消息头
 */
void Dlt_BuildMessageHeader(
    Dlt_MessageHeaderType* header,
    const Dlt_AppEntryType* appEntry,
    uint8 msgType,
    uint8 subtype,
    uint16 messageId,
    uint16 payloadLen)
{
    if ((header == NULL_PTR) || (appEntry == NULL_PTR)) {
        return;
    }

    /* 填充标准头 */
    header->pattern = DLT_PATTERN;
    header->version = DLT_PROTOCOL_VERSION;
    header->length = sizeof(Dlt_MessageHeaderType) + payloadLen;
    header->ecucVersion = 0x01U;
    header->endianness = DLT_ENDIANESS_LE;
    header->extendedHeader = 0x01U;

    /* 填充应用 ID (安全复制，防止溢出) */
    const char* appId = appEntry->info.appId;
    if (appId != NULL_PTR) {
        uint32 appIdLen = strlen(appId);
        if (appIdLen > 4U) {
            appIdLen = 4U;
        }
        (void)memset(header->applicationId, 0, 4U);
        (void)memcpy(header->applicationId, appId, appIdLen);
    } else {
        (void)memset(header->applicationId, 0, 4U);
    }
    
    /* 填充上下文 ID */
    const char* contextId = "DLT1";
    (void)memcpy(header->contextId, contextId, 4U);

    /* 填充消息类型和 ID */
    header->type = msgType;
    header->subtype = subtype;
    header->messageId = messageId;
    
    /* 初始化时间戳和会话ID (由调用方填充) */
    header->timestamp = 0U;
    header->sessionId = 0U;
    header->sequenceCounter = 0U;
}

/**
 * @brief 将消息入队
 */
Std_ReturnType Dlt_EnqueueMessage(const Dlt_QueueEntryType* queueEntry)
{
    /* 检查队列是否已满 */
    if (g_DltState.queueCount >= DLT_MAX_QUEUE_SIZE) {
        g_DltState.totalMessagesDropped++;
        return E_NOT_OK;
    }

    /* 复制消息到队列 */
    uint16 index = g_DltState.queueTail;
    g_DltState.queue[index] = *queueEntry;

    /* 更新队列指针 */
    g_DltState.queueTail = (g_DltState.queueTail + 1U) % DLT_MAX_QUEUE_SIZE;
    g_DltState.queueCount++;

    return E_OK;
}

/**
 * @brief 将消息出队
 */
Std_ReturnType Dlt_DequeueMessage(Dlt_QueueEntryType* queueEntry)
{
    /* 检查队列是否为空 */
    if (g_DltState.queueCount == 0U) {
        return E_NOT_OK;
    }

    /* 从队列头取出消息 */
    uint16 index = g_DltState.queueHead;
    *queueEntry = g_DltState.queue[index];

    /* 更新队列指针 */
    g_DltState.queueHead = (g_DltState.queueHead + 1U) % DLT_MAX_QUEUE_SIZE;
    g_DltState.queueCount--;

    return E_OK;
}

/**
 * @brief 通过 UDP 发送消息 (简化实现)
 */
Std_ReturnType Dlt_UdpSend(const uint8* data, uint16 length)
{
    /* 
     * UDP 发送接口 - 实际发送依赖底层网络栈集成
     * 典型实现:
     * 1. 获取 socket 描述符
     * 2. 调用 sendto() 发送数据
     * 3. 检查返回值
     */
    
    /* 简化实现: 假设发送成功 */
    (void)data;
    (void)length;
    
    return E_OK;
}

/**
 * @brief 传输层发送接口 (抽象)
 */
Std_ReturnType Dlt_TransportSend(
    const uint8* data,
    uint16 length,
    Dlt_TransportProtocolType protocol)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* 根据协议类型选择传输方式 */
    switch (protocol) {
        case DLT_TRANSPORT_UDP:
            result = Dlt_UdpSend(data, length);
            break;
            
        case DLT_TRANSPORT_TCP:
            /* NOTE: TCP 发送实现 pending 网络栈集成 */
            result = E_NOT_OK;
            break;
            
        case DLT_TRANSPORT_SOMEIP:
            /* NOTE: SOME/IP 发送实现 pending 网络栈集成 */
            result = E_NOT_OK;
            break;
            
        default:
            result = E_NOT_OK;
            break;
    }
    
    return result;
}

/**
 * @brief 应用消息过滤器
 */
boolean Dlt_ApplyFilter(Dlt_AppHandleType appHandle, Dlt_LogLevelType logLevel)
{
    /* 查找应用条目 */
    Dlt_AppEntryType* appEntry = Dlt_FindAppEntry(appHandle);
    if (appEntry == NULL_PTR) {
        return FALSE;
    }

    /* 检查应用是否激活 */
    if (!appEntry->isActive) {
        return FALSE;
    }

    /* 检查日志级别 */
    if ((uint8)logLevel > (uint8)appEntry->currentLogLevel) {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief 查找应用条目
 */
Dlt_AppEntryType* Dlt_FindAppEntry(Dlt_AppHandleType appHandle)
{
    for (uint16 i = 0U; i < DLT_MAX_APPS; i++) {
        if ((g_DltState.appTable[i].handle == appHandle) &&
            g_DltState.appTable[i].isActive) {
            return &g_DltState.appTable[i];
        }
    }
    
    return NULL_PTR;
}

/**
 * @brief 分配应用句柄
 */
Dlt_AppHandleType Dlt_AllocateAppHandle(void)
{
    Dlt_AppHandleType handle = g_NextAppHandle;
    g_NextAppHandle++;
    
    /* 处理句柄溢出 */
    if (g_NextAppHandle == DLT_INVALID_APP_HANDLE) {
        g_NextAppHandle = 1U;
    }
    
    return handle;
}

/**
 * @brief 释放应用句柄
 */
void Dlt_FreeAppHandle(Dlt_AppHandleType appHandle)
{
    /* 简化实现: 句柄池管理可在后续版本中优化 */
    (void)appHandle;
}

/**
 * @brief 获取时间戳 (微秒)
 */
uint64 Dlt_GetTimestampUs(void)
{
    /* 
     * 时间戳获取 - 依赖系统定时器集成
     * 
     * 典型实现:
     * 1. 使用硬件定时器 (如 GPT)
     * 2. 使用系统计数器 (System Counter)
     * 3. 使用RTOS的时钟函数
     * 
     * 示例: return GetSystemCounterUs();
     */
    
    /* 简化实现: 返回0 */
    return 0U;
}

/**
 * @brief 获取下一个序列号
 */
uint32 Dlt_GetNextSequenceNumber(void)
{
    g_DltState.sequenceCounter++;
    
    /* 处理序列号溢出 */
    if (g_DltState.sequenceCounter == UINT32_MAX) {
        g_DltState.sequenceCounter = 0U;
    }
    
    return g_DltState.sequenceCounter;
}

/* ========================================================================== */
/*                          新增 API 函数实现                                  */
/* ========================================================================== */

/** @req SWS_Dlt_00010 */
/**
 * @brief 设置会话ID
 */
Std_ReturnType Dlt_SetSessionId(uint32 sessionId)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_SET_SESSION, DLT_E_UNINIT);
        return E_NOT_OK;
    }
    
    if (sessionId == 0U) {
        DLT_DETECT_ERROR(DLT_APIID_SET_SESSION, DLT_E_SESSION_ERROR);
        return E_NOT_OK;
    }
#endif

    /* 更新会话ID */
    g_DltState.currentSessionId = sessionId;
    
    return E_OK;
}

/** @req SWS_Dlt_00011 */
/**
 * @brief 获取统计信息
 */
void Dlt_GetStatistics(
    uint32* sentCount,
    uint32* droppedCount,
    uint16* queueCount)
{
    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_GET_STATISTICS, DLT_E_UNINIT);
        return;
    }
    
    if ((sentCount == NULL_PTR) || (droppedCount == NULL_PTR) || (queueCount == NULL_PTR)) {
        DLT_DETECT_ERROR(DLT_APIID_GET_STATISTICS, DLT_E_PARAM_POINTER);
        return;
    }
#endif

    /* 返回统计信息 */
    if (sentCount != NULL_PTR) {
        *sentCount = g_DltState.totalMessagesSent;
    }
    
    if (droppedCount != NULL_PTR) {
        *droppedCount = g_DltState.totalMessagesDropped;
    }
    
    if (queueCount != NULL_PTR) {
        *queueCount = g_DltState.queueCount;
    }
}

/* ========================================================================== */
/*                    Context 管理 API 实现 (ecual 合并, 2026-08-15)          */
/* ========================================================================== */

/**
 * @brief 在运行时 context 表中按 appId+contextId 查找
 *
 * @param appId 应用 ID
 * @param contextId 上下文 ID
 * @param contextIndex 输出参数: 命中索引
 *
 * @return boolean TRUE=命中, FALSE=未找到
 */
static boolean Dlt_FindContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    uint16* contextIndex)
{
    for (uint16 i = 0U; i < DLT_MAX_CONTEXT_COUNT; i++) {
        if (Dlt_RuntimeContext[i].registered &&
            (Dlt_RuntimeContext[i].appId == appId) &&
            (Dlt_RuntimeContext[i].contextId == contextId)) {
            if (contextIndex != NULL_PTR) {
                *contextIndex = i;
            }
            return TRUE;
        }
    }
    return FALSE;
}

/** @req SWS_Dlt_00012 */
/**
 * @brief 注册 context
 *
 * @note 语义参考 ecual 版: 重复注册幂等返回 E_OK;
 *       32 个链接期预配置 context 占满表项时返回 E_NOT_OK (需先注销释放槽位)。
 */
Std_ReturnType Dlt_RegisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    const uint8* description,
    uint8 descriptionLength)
{
    uint16 freeIndex = DLT_MAX_CONTEXT_COUNT;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_REGISTER_CONTEXT, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (description == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_REGISTER_CONTEXT, DLT_E_PARAM_POINTER);
        return E_NOT_OK;
    }

    if (descriptionLength > DLT_MAX_CONTEXT_DESCRIPTION) {
        DLT_DETECT_ERROR(DLT_APIID_REGISTER_CONTEXT, DLT_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif

    /* 已注册则幂等返回 */
    if (Dlt_FindContext(appId, contextId, NULL_PTR)) {
        return E_OK;
    }

    /* 查找空闲槽位 */
    for (uint16 i = 0U; i < DLT_MAX_CONTEXT_COUNT; i++) {
        if (!Dlt_RuntimeContext[i].registered) {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex >= DLT_MAX_CONTEXT_COUNT) {
#if (DLT_DEV_ERROR_DETECT == STD_ON)
        DLT_DETECT_ERROR(DLT_APIID_REGISTER_CONTEXT, DLT_E_CONTEXT_FULL);
#endif
        return E_NOT_OK;
    }

    /* 注册新 context */
    Dlt_RuntimeContext[freeIndex].appId = appId;
    Dlt_RuntimeContext[freeIndex].contextId = contextId;
    Dlt_RuntimeContext[freeIndex].logLevel = DLT_DEFAULT_LOG_LEVEL;
    Dlt_RuntimeContext[freeIndex].traceStatus = DLT_DEFAULT_TRACE_STATUS;
    Dlt_RuntimeContext[freeIndex].registered = TRUE;

    /* 复制描述 (先清零再拷贝, 保证定长缓冲内容确定) */
    (void)memset(Dlt_RuntimeContext[freeIndex].description, 0, DLT_MAX_CONTEXT_DESCRIPTION);
    if (descriptionLength > 0U) {
        (void)memcpy(Dlt_RuntimeContext[freeIndex].description, description, descriptionLength);
    }

    return E_OK;
}

/** @req SWS_Dlt_00013 */
/**
 * @brief 注销 context
 */
Std_ReturnType Dlt_UnregisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId)
{
    uint16 contextIndex = 0U;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_UNREGISTER_CONTEXT, DLT_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (!Dlt_FindContext(appId, contextId, &contextIndex)) {
#if (DLT_DEV_ERROR_DETECT == STD_ON)
        DLT_DETECT_ERROR(DLT_APIID_UNREGISTER_CONTEXT, DLT_E_CONTEXT_NOT_FOUND);
#endif
        return E_NOT_OK;
    }

    /* 释放槽位 */
    Dlt_RuntimeContext[contextIndex].registered = FALSE;
    Dlt_RuntimeContext[contextIndex].appId = 0U;
    Dlt_RuntimeContext[contextIndex].contextId = 0U;
    (void)memset(Dlt_RuntimeContext[contextIndex].description, 0, DLT_MAX_CONTEXT_DESCRIPTION);

    return E_OK;
}

/** @req SWS_Dlt_00014 */
/**
 * @brief 设置 context 日志级别
 */
Std_ReturnType Dlt_SetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel)
{
    uint16 contextIndex = 0U;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_SET_LOG_LEVEL, DLT_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (!Dlt_FindContext(appId, contextId, &contextIndex)) {
        return E_NOT_OK;
    }

    Dlt_RuntimeContext[contextIndex].logLevel = logLevel;
    return E_OK;
}

/** @req SWS_Dlt_00015 */
/**
 * @brief 读取 context 日志级别
 */
Std_ReturnType Dlt_GetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel)
{
    uint16 contextIndex = 0U;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_GET_LOG_LEVEL, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (logLevel == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_GET_LOG_LEVEL, DLT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (!Dlt_FindContext(appId, contextId, &contextIndex)) {
        return E_NOT_OK;
    }

    *logLevel = Dlt_RuntimeContext[contextIndex].logLevel;
    return E_OK;
}

/** @req SWS_Dlt_00016 */
/**
 * @brief 设置 context 跟踪状态
 */
Std_ReturnType Dlt_SetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType traceStatus)
{
    uint16 contextIndex = 0U;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_SET_TRACE_STATUS, DLT_E_UNINIT);
        return E_NOT_OK;
    }
#endif

    if (!Dlt_FindContext(appId, contextId, &contextIndex)) {
        return E_NOT_OK;
    }

    Dlt_RuntimeContext[contextIndex].traceStatus = traceStatus;
    return E_OK;
}

/** @req SWS_Dlt_00017 */
/**
 * @brief 读取 context 跟踪状态
 */
Std_ReturnType Dlt_GetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType* traceStatus)
{
    uint16 contextIndex = 0U;

    /* 检查开发错误 */
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (g_DltState.moduleState == DLT_STATE_UNINIT) {
        DLT_DETECT_ERROR(DLT_APIID_GET_TRACE_STATUS, DLT_E_UNINIT);
        return E_NOT_OK;
    }

    if (traceStatus == NULL_PTR) {
        DLT_DETECT_ERROR(DLT_APIID_GET_TRACE_STATUS, DLT_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif

    if (!Dlt_FindContext(appId, contextId, &contextIndex)) {
        return E_NOT_OK;
    }

    *traceStatus = Dlt_RuntimeContext[contextIndex].traceStatus;
    return E_OK;
}

/**
 * @brief Com 发送确认回调 (占位实现, 语义参考 ecual 版)
 *
 * @note 当前无调用方: services 版传输路径为 UDP/TCP 抽象 (Dlt_TransportSend),
 *       未接入 Com 模块。待 Com 集成后消费该回调。
 */
#if (DLT_USE_COM == STD_ON)
/** @req SWS_Dlt_00018 */
void Dlt_ComTxConfirmation(uint8 result)
{
    (void)result;
    /* Handle transmission confirmation */
}
#endif

/**
 * @brief Com 接收指示回调 (占位实现, 语义参考 ecual 版)
 *
 * @note 当前无调用方: 待 Com 集成后处理收到的 DLT 控制消息。
 */
#if (DLT_USE_COM == STD_ON)
/** @req SWS_Dlt_00019 */
void Dlt_ComRxIndication(const uint8* data, uint16 length)
{
    (void)data;
    (void)length;
    /* Handle received DLT control messages */
}
#endif
