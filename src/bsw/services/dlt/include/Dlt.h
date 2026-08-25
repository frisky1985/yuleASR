/**
 * @file Dlt.h
 * @brief DLT (Diagnostic Log and Trace) 模块主头文件
 * 
 * 提供符合 AutoSAR Classic Platform 4.x 规范的诊断日志和跟踪服务
 * 
 * 功能特性:
 * - 日志消息传输 (Log Messages)
 * - 跟踪消息传输 (Trace Messages)  
 * - 多种传输协议支持 (UDP/TCP/SOME/IP)
 * - 消息优先级和过滤
 * - 应用注册和管理
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 * @version 1.0.0
 */

#ifndef DLT_H
#define DLT_H

/* ========================================================================== */
/*                              包含头文件                                     */
/* ========================================================================== */

#include "Std_Types.h"
#include "Dlt_Types.h"
#include "Dlt_Cfg.h"
#include "ComStack_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                              版本号定义                                     */
/* ========================================================================== */

/**
 * @brief DLT 模块 ID
 */
#define DLT_MODULE_ID  254U

/**
 * @brief DLT 实例 ID
 */
#define DLT_INSTANCE_ID 0U

/**
 * @brief DLT 供应商 ID
 */
#define DLT_VENDOR_ID  1U

/**
 * @brief DLT 软件版本号
 */
#define DLT_SW_MAJOR_VERSION  1U
#define DLT_SW_MINOR_VERSION  0U
#define DLT_SW_PATCH_VERSION  0U

/* ========================================================================== */
/*                              API 函数声明                                   */
/* ========================================================================== */

/** @req SWS_Dlt_00001 */
/**
 * @brief 初始化 DLT 模块
 * 
 * @param ConfigPtr 指向配置数据的指针
 * 
 * @return void
 * 
 * @details
 * - 初始化 DLT 传输层
 * - 配置消息过滤器
 * - 分配内部缓冲区
 * - 将模块状态设置为 READY
 * 
 * @note 必须在调用其他 DLT API 之前调用此函数
 * 
 * AUTOSAR SWS DLT_00001
 */
void Dlt_Init(const Dlt_ConfigType* ConfigPtr);

/** @req SWS_Dlt_00002 */
/**
 * @brief 反初始化 DLT 模块
 * 
 * @return void
 * 
 * @details
 * - 释放所有分配的资源
 * - 清空消息队列
 * - 将模块状态设置为 UNINIT
 * 
 * AUTOSAR SWS DLT_00002
 */
void Dlt_DeInit(void);

/**
 * @brief 注册应用到 DLT 模块
 * 
 * @param AppInfoPtr 指向应用信息的指针
 * 
 * @return Dlt_AppHandleType 应用句柄
 * @retval DLT_INVALID_APP_HANDLE 注册失败
 * 
 * @details
 * - 为应用分配唯一句柄
 * - 存储应用信息
 * - 配置应用的日志级别
 * 
 * AUTOSAR SWS DLT_00003
 */
Dlt_AppHandleType Dlt_RegisterApp(const Dlt_AppInfoType* AppInfoPtr);

/** @req SWS_Dlt_00005 */
/**
 * @brief 注销应用
 * 
 * @param AppHandle 应用句柄
 * 
 * @return Std_ReturnType
 * @retval E_OK 注销成功
 * @retval E_NOT_OK 注销失败 (无效句柄)
 * 
 * AUTOSAR SWS DLT_00004
 */
Std_ReturnType Dlt_UnregisterApp(Dlt_AppHandleType AppHandle);

/**
 * @brief 发送日志消息
 * 
 * @param AppHandle 应用句柄
 * @param LogLevel 日志级别
 * @param MessageId 消息 ID
 * @param DataPtr 指向数据缓冲区的指针
 * @param Length 数据长度
 * 
 * @return Std_ReturnType
 * @retval E_OK 发送成功
 * @retval E_NOT_OK 发送失败
 * @retval E_PENDING 发送挂起
 * 
 * @details
 * - 构建 DLT 日志消息
 * - 应用消息过滤器
 * - 通过传输层发送
 * 
 * AUTOSAR SWS DLT_00005
 */
/** @req SWS_Dlt_00006 */
Std_ReturnType Dlt_SendLogMessage(
    Dlt_AppHandleType  AppHandle,
    Dlt_LogLevelType   LogLevel,
    Dlt_MessageIdType  MessageId,
    const uint8*       DataPtr,
    uint16             Length
);

/** @req SWS_Dlt_00007 */
/**
 * @brief 发送跟踪消息
 * 
 * @param AppHandle 应用句柄
 * @param TraceType 跟踪类型
 * @param TraceId 跟踪 ID
 * @param DataPtr 指向数据缓冲区的指针
 * @param Length 数据长度
 * 
 * @return Std_ReturnType
 * @retval E_OK 发送成功
 * @retval E_NOT_OK 发送失败
 * @retval E_PENDING 发送挂起
 * 
 * AUTOSAR SWS DLT_00006
 */
Std_ReturnType Dlt_SendTraceMessage(
    Dlt_AppHandleType AppHandle,
    Dlt_TraceType     TraceType,
    Dlt_MessageIdType TraceId,
    const uint8*      DataPtr,
    uint16            Length
);

/** @req SWS_Dlt_00004 */
/**
 * @brief DLT 主函数
 * 
 * @return void
 * 
 * @details
 * - 处理消息队列中的待发送消息
 * - 处理接收到的控制消息
 * - 更新模块状态
 * 
 * @note 应周期性调用 (建议 10ms)
 * 
 * AUTOSAR SWS DLT_00007
 */
void Dlt_MainFunction(void);

/** @req SWS_Dlt_00003 */
/**
 * @brief 获取 DLT 模块版本信息
 * 
 * @param VersionInfoPtr 指向版本信息结构的指针
 * 
 * @return void
 * 
 * AUTOSAR SWS DLT_00008
 */
void Dlt_GetVersionInfo(Std_VersionInfoType* VersionInfoPtr);

/** @req SWS_Dlt_00008 */
/**
 * @brief 设置消息过滤器
 * 
 * @param AppHandle 应用句柄
 * @param LogLevel 日志级别阈值
 * @param Enabled 启用/禁用
 * 
 * @return Std_ReturnType
 * @retval E_OK 设置成功
 * @retval E_NOT_OK 设置失败
 * 
 * AUTOSAR SWS DLT_00009
 */
Std_ReturnType Dlt_SetFilter(
    Dlt_AppHandleType AppHandle,
    Dlt_LogLevelType  LogLevel,
    boolean           Enabled
);

/** @req SWS_Dlt_00009 */
/**
 * @brief 清空消息队列
 * 
 * @return Std_ReturnType
 * @retval E_OK 清空成功
 * @retval E_NOT_OK 清空失败
 * 
 * AUTOSAR SWS DLT_00010
 */
Std_ReturnType Dlt_FlushQueue(void);

/**
 * @brief 获取模块状态
 * 
 * @return Dlt_ModuleStateType 模块状态
 * 
 * AUTOSAR SWS DLT_00011
 */
Dlt_ModuleStateType Dlt_GetStatus(void);

/** @req SWS_Dlt_00010 */
/**
 * @brief 设置会话ID
 * 
 * @param sessionId 会话ID
 * 
 * @return Std_ReturnType
 * @retval E_OK 设置成功
 * @retval E_NOT_OK 设置失败
 * 
 * AUTOSAR SWS DLT_00012
 */
Std_ReturnType Dlt_SetSessionId(uint32 sessionId);

/** @req SWS_Dlt_00011 */
/**
 * @brief 获取统计信息
 * 
 * @param sentCount 发送消息计数指针
 * @param droppedCount 丢弃消息计数指针
 * @param queueCount 当前队列消息计数指针
 * 
 * @return void
 * 
 * AUTOSAR SWS DLT_00013
 */
void Dlt_GetStatistics(
    uint32* sentCount,
    uint32* droppedCount,
    uint16* queueCount
);

/* ========================================================================== */
/*                      Context 管理 API (ecual 合并, 2026-08-15)              */
/* ========================================================================== */

/** @req SWS_Dlt_00012 */
/**
 * @brief 注册 context
 * 
 * @param appId 应用 ID (4 字节打包 ASCII, 如 0x44454641 == "DEFA")
 * @param contextId 上下文 ID (4 字节打包 ASCII)
 * @param description context 描述 (定长 32 字节, 见 DLT_MAX_CONTEXT_DESCRIPTION)
 * @param descriptionLength 描述长度 (<= DLT_MAX_CONTEXT_DESCRIPTION)
 * 
 * @return Std_ReturnType
 * @retval E_OK 注册成功 (或已注册, 幂等)
 * @retval E_NOT_OK 注册失败 (context 表满 / 参数错误)
 * 
 * @note 32 个链接期预配置 context 占满表项时, 需先 Dlt_UnregisterContext 释放槽位。
 */
Std_ReturnType Dlt_RegisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    const uint8* description,
    uint8 descriptionLength
);

/** @req SWS_Dlt_00013 */
/**
 * @brief 注销 context
 * 
 * @param appId 应用 ID
 * @param contextId 上下文 ID
 * 
 * @return Std_ReturnType
 * @retval E_OK 注销成功
 * @retval E_NOT_OK 未找到匹配 context
 */
Std_ReturnType Dlt_UnregisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId
);

/** @req SWS_Dlt_00014 */
/**
 * @brief 设置 context 日志级别
 * 
 * @param appId 应用 ID
 * @param contextId 上下文 ID
 * @param logLevel 日志级别
 * 
 * @return Std_ReturnType
 * @retval E_OK 设置成功
 * @retval E_NOT_OK 未找到匹配 context
 */
Std_ReturnType Dlt_SetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel
);

/** @req SWS_Dlt_00015 */
/**
 * @brief 读取 context 日志级别
 * 
 * @param appId 应用 ID
 * @param contextId 上下文 ID
 * @param logLevel 输出指针
 * 
 * @return Std_ReturnType
 * @retval E_OK 读取成功
 * @retval E_NOT_OK 未找到匹配 context 或空指针
 */
Std_ReturnType Dlt_GetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel
);

/** @req SWS_Dlt_00016 */
/**
 * @brief 设置 context 跟踪状态
 * 
 * @param appId 应用 ID
 * @param contextId 上下文 ID
 * @param traceStatus 跟踪状态
 * 
 * @return Std_ReturnType
 * @retval E_OK 设置成功
 * @retval E_NOT_OK 未找到匹配 context
 */
Std_ReturnType Dlt_SetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType traceStatus
);

/** @req SWS_Dlt_00017 */
/**
 * @brief 读取 context 跟踪状态
 * 
 * @param appId 应用 ID
 * @param contextId 上下文 ID
 * @param traceStatus 输出指针
 * 
 * @return Std_ReturnType
 * @retval E_OK 读取成功
 * @retval E_NOT_OK 未找到匹配 context 或空指针
 */
Std_ReturnType Dlt_GetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType* traceStatus
);

/**
 * @brief Com 发送确认回调 (DLT 消息经 Com 发送)
 * 
 * @param result 发送结果
 * 
 * @return void
 * 
 * @note 当前为占位实现 (语义参考 ecual 版), 无调用方;
 *       待 Com 集成接入后消费。
 */
#if (DLT_USE_COM == STD_ON)
/** @req SWS_Dlt_00018 */
void Dlt_ComTxConfirmation(uint8 result);
#endif

/**
 * @brief Com 接收指示回调 (接收 DLT 控制消息)
 * 
 * @param data 接收数据指针
 * @param length 数据长度
 * 
 * @return void
 * 
 * @note 当前为占位实现 (语义参考 ecual 版), 无调用方;
 *       待 Com 集成接入后消费。
 */
#if (DLT_USE_COM == STD_ON)
/** @req SWS_Dlt_00019 */
void Dlt_ComRxIndication(const uint8* data, uint16 length);
#endif

/* ========================================================================== */
/*                          开发错误检测 (DET)                                 */
/* ========================================================================== */

#if (DLT_DEV_ERROR_DETECT == STD_ON)

/**
 * @brief DLT 开发错误检测启用
 */
#define DLT_DETECT_ERROR(ApiId, ErrorId) \
    Det_ReportError(DLT_MODULE_ID, DLT_INSTANCE_ID, ApiId, ErrorId)

#else

/**
 * @brief DLT 开发错误检测禁用
 */
#define DLT_DETECT_ERROR(ApiId, ErrorId) ((void)0)

#endif

/* DLT 错误代码 */
#define DLT_E_PARAM_CONFIG    0x01U  /**< 配置参数错误 */
#define DLT_E_PARAM_POINTER   0x02U  /**< 空指针参数 */
#define DLT_E_PARAM_LENGTH    0x03U  /**< 长度参数错误 */
#define DLT_E_UNINIT          0x10U  /**< 模块未初始化 */
#define DLT_E_INVALID_HANDLE  0x11U  /**< 无效应用句柄 */
#define DLT_E_QUEUE_FULL      0x20U  /**< 消息队列满 */
#define DLT_E_TRANSPORT_ERROR 0x30U  /**< 传输错误 */
#define DLT_E_FILTER_ERROR    0x31U  /**< 过滤器错误 */
#define DLT_E_TIMESTAMP_ERROR 0x32U  /**< 时间戳错误 */
#define DLT_E_SESSION_ERROR   0x33U  /**< 会话错误 */
#define DLT_E_PRIORITY_ERROR  0x34U  /**< 优先级错误 */
#define DLT_E_BUFFER_OVERFLOW 0x40U  /**< 缓冲区溢出 */
#define DLT_E_CONTEXT_NOT_FOUND 0x41U /**< 未找到匹配 context */
#define DLT_E_CONTEXT_FULL 0x42U      /**< context 表满 */

/* API ID 定义 */
#define DLT_APIID_INIT            0x00U
#define DLT_APIID_DEINIT          0x01U
#define DLT_APIID_REGISTER_APP    0x02U
#define DLT_APIID_UNREGISTER_APP  0x03U
#define DLT_APIID_SEND_LOG        0x04U
#define DLT_APIID_SEND_TRACE      0x05U
#define DLT_APIID_MAIN_FUNCTION   0x06U
#define DLT_APIID_GET_VERSION     0x07U
#define DLT_APIID_SET_FILTER      0x08U
#define DLT_APIID_FLUSH_QUEUE     0x09U
#define DLT_APIID_GET_STATUS      0x0AU
#define DLT_APIID_SET_SESSION     0x0BU
#define DLT_APIID_GET_STATISTICS  0x0CU
#define DLT_APIID_REGISTER_CONTEXT    0x0DU
#define DLT_APIID_UNREGISTER_CONTEXT  0x0EU
#define DLT_APIID_SET_LOG_LEVEL       0x0FU
#define DLT_APIID_GET_LOG_LEVEL       0x10U
#define DLT_APIID_SET_TRACE_STATUS    0x11U
#define DLT_APIID_GET_TRACE_STATUS    0x12U
#define DLT_APIID_COM_TX_CONFIRMATION 0x13U
#define DLT_APIID_COM_RX_INDICATION   0x14U

#ifdef __cplusplus
}
#endif

#endif /* DLT_H */
