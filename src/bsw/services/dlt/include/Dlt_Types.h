/**
 * @file Dlt_Types.h
 * @brief DLT (Diagnostic Log and Trace) 类型定义
 * 
 * 符合 AutoSAR Classic Platform 4.x 规范
 * 
 * @company 上海予乐电子科技有限公司
 * @author YuleTech Team
 * @date 2026-04-27
 * @version 1.0.0
 */

#ifndef DLT_TYPES_H
#define DLT_TYPES_H

#include "Std_Types.h"

/* ========================================================================== */
/*                              版本信息类型                                   */
/* ========================================================================== */

/**
 * @brief DLT 版本信息类型
 */
typedef struct {
    uint16  vendorID;         /**< 供应商 ID */
    uint16  moduleID;         /**< 模块 ID */
    uint8   sw_major_version; /**< 软件主版本号 */
    uint8   sw_minor_version; /**< 软件次版本号 */
    uint8   sw_patch_version; /**< 软件补丁版本号 */
} Dlt_VersionInfoType;

/* ========================================================================== */
/*                          DLT 传输协议类型                                   */
/* ========================================================================== */

/**
 * @brief DLT 传输协议类型
 */
typedef enum {
    DLT_TRANSPORT_UDP = 0U,   /**< UDP 传输协议 */
    DLT_TRANSPORT_TCP = 1U,   /**< TCP 传输协议 */
    DLT_TRANSPORT_SOMEIP = 2U /**< SOME/IP 传输协议 */
} Dlt_TransportProtocolType;

/* ========================================================================== */
/*                          DLT 消息类型                                       */
/* ========================================================================== */

/**
 * @brief DLT 消息类型
 */
typedef enum {
    DLT_MSG_TYPE_LOG = 0U,      /**< 日志消息 */
    DLT_MSG_TYPE_TRACE = 1U,    /**< 跟踪消息 */
    DLT_MSG_TYPE_CONTROL = 2U,  /**< 控制消息 */
    DLT_MSG_TYPE_NW_TRACE = 3U  /**< 网络跟踪消息 */
} Dlt_MessageType;

/* ========================================================================== */
/*                          DLT 日志级别                                       */
/* ========================================================================== */

/**
 * @brief DLT 日志级别 (根据 AutoSAR 规范)
 */
typedef enum {
    DLT_LOG_FATAL = 0U,       /**< 致命错误 */
    DLT_LOG_ERROR = 1U,       /**< 错误 */
    DLT_LOG_WARN = 2U,        /**< 警告 */
    DLT_LOG_INFO = 3U,        /**< 信息 */
    DLT_LOG_DEBUG = 4U,       /**< 调试 */
    DLT_LOG_VERBOSE = 5U      /**< 详细 */
} Dlt_LogLevelType;

/* ========================================================================== */
/*                          DLT 跟踪类型                                       */
/* ========================================================================== */

/**
 * @brief DLT 跟踪类型
 */
typedef enum {
    DLT_TRACE_VARIABLE = 0U,      /**< 变量跟踪 */
    DLT_TRACE_FUNCTION = 1U,      /**< 函数调用跟踪 */
    DLT_TRACE_STATE = 2U,         /**< 状态跟踪 */
    DLT_TRACE_BUFFER = 3U         /**< 缓冲区跟踪 */
} Dlt_TraceType;

/* ========================================================================== */
/*                          DLT 应用句柄                                       */
/* ========================================================================== */

/**
 * @brief DLT 应用句柄类型
 */
typedef uint16 Dlt_AppHandleType;

/**
 * @brief 无效应用句柄
 */
#define DLT_INVALID_APP_HANDLE 0xFFFFU

/* ========================================================================== */
/*                          DLT 消息 ID                                        */
/* ========================================================================== */

/**
 * @brief DLT 消息 ID 类型
 */
typedef uint16 Dlt_MessageIdType;

/**
 * @brief DLT 上下文 ID 类型
 */
typedef uint16 Dlt_ContextIdType;

/* ========================================================================== */
/*                          DLT 应用信息                                       */
/* ========================================================================== */

/**
 * @brief DLT 应用信息结构
 */
typedef struct {
    const char* appId;          /**< 应用 ID 字符串 */
    const char* appDescription; /**< 应用描述 */
    uint8       maxLogLevel;    /**< 最大日志级别 */
} Dlt_AppInfoType;

/* ========================================================================== */
/*                          DLT 配置类型                                       */
/* ========================================================================== */

/**
 * @brief DLT 传输配置
 */
typedef struct {
    Dlt_TransportProtocolType protocol;     /**< 传输协议 */
    uint16                    port;          /**< 端口号 */
    uint32                    bufferSize;    /**< 缓冲区大小 (字节) */
    uint32                    maxMessageSize; /**< 最大消息大小 (字节) */
} Dlt_TransportConfigType;

/**
 * @brief DLT 过滤器配置
 */
typedef struct {
    Dlt_AppHandleType  appHandle;    /**< 应用句柄 */
    Dlt_MessageType    messageType;  /**< 消息类型 */
    Dlt_LogLevelType   minLogLevel;  /**< 最小日志级别 */
    boolean            enabled;      /**< 是否启用 */
} Dlt_FilterConfigType;

/**
 * @brief DLT 配置结构 (由配置工具生成)
 */
typedef struct {
    const Dlt_TransportConfigType* transportConfig;  /**< 传输配置 */
    const Dlt_FilterConfigType*    filterConfig;     /**< 过滤器配置 */
    uint16                         filterCount;      /**< 过滤器数量 */
    uint32                         queueSize;        /**< 消息队列大小 */
} Dlt_ConfigType;

/* ========================================================================== */
/*                          DLT 消息结构                                       */
/* ========================================================================== */

/**
 * @brief DLT 消息头结构 (符合 DLT 协议规范)
 */
typedef struct {
    uint8  pattern;           /**< 模式标识 (固定为 0x01) */
    uint8  version;           /**< DLT 协议版本 */
    uint16 length;            /**< 消息总长度 */
    uint8  ecucVersion;       /**< ECU 软件版本 */
    uint8  endianness;        /**< 字节序 (0=LE, 1=BE) */
    uint8  extendedHeader;    /**< 扩展头标志 */
    uint8  applicationId[4];  /**< 应用 ID */
    uint8  contextId[4];      /**< 上下文 ID */
    uint8  type;              /**< 消息类型 */
    uint8  subtype;           /**< 子类型 (日志级别/跟踪类型) */
    uint16 messageId;         /**< 消息 ID */
} Dlt_MessageHeaderType;

/**
 * @brief DLT 消息结构
 */
typedef struct {
    Dlt_MessageHeaderType header;     /**< 消息头 */
    const uint8*          payload;    /**< 负载数据 */
    uint16                payloadLen; /**< 负载长度 */
} Dlt_MessageType;

/* ========================================================================== */
/*                          DLT 状态类型                                       */
/* ========================================================================== */

/**
 * @brief DLT 模块状态
 */
typedef enum {
    DLT_STATE_UNINIT = 0U,    /**< 未初始化 */
    DLT_STATE_INIT = 1U,      /**< 已初始化 */
    DLT_STATE_READY = 2U      /**< 就绪 */
} Dlt_ModuleStateType;

/**
 * @brief DLT 返回码
 */
typedef enum {
    DLT_OK = 0U,              /**< 成功 */
    DLT_NOT_OK = 1U,          /**< 失败 */
    DLT_PENDING = 2U,         /**< 挂起 */
    DLT_BUSY = 3U             /**< 忙 */
} Dlt_ReturnType;

#endif /* DLT_TYPES_H */
