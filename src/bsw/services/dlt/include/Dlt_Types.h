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
} Dlt_MessageCategoryType;

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

/**
 * @brief 日志关闭标记 (ecual 兼容扩展)
 *
 * 注意: services 版枚举以 DLT_LOG_FATAL=0 为基线（无 DLT_LOG_OFF 枚举成员），
 * 为保留 ecual 版 Dlt_DefaultLogLevels 配置表中 "State 0: Production - No logging"
 * 语义，以宏形式补充关闭标记（位于枚举取值域之外）。
 */
#define DLT_LOG_OFF  ((Dlt_LogLevelType)0xFFU)

/* ========================================================================== */
/*                          DLT 跟踪状态                                       */
/* ========================================================================== */

/**
 * @brief DLT 跟踪状态类型 (ecual 兼容, 用于 context 配置表)
 */
typedef enum {
    DLT_TRACE_STATUS_OFF = 0U,  /**< 跟踪关闭 */
    DLT_TRACE_STATUS_ON = 1U    /**< 跟踪开启 */
} Dlt_TraceStatusType;

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
/*                          DLT 消息优先级                                     */
/* ========================================================================== */

/**
 * @brief DLT 消息优先级
 */
typedef enum {
    DLT_PRIORITY_LOW = 0U,      /**< 低优先级 */
    DLT_PRIORITY_NORMAL = 1U,   /**< 普通优先级 */
    DLT_PRIORITY_HIGH = 2U,     /**< 高优先级 */
    DLT_PRIORITY_CRITICAL = 3U  /**< 关键优先级 */
} Dlt_PriorityType;

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
 * @brief DLT 应用 ID 类型 (ecual 兼容)
 *
 * 以 4 字节打包 ASCII 形式表示 (如 0x44454641 == "DEFA")，
 * 与 Dlt_ContextConfig 链接期配置表保持一致。
 */
typedef uint32 Dlt_ApplicationIdType;

/**
 * @brief DLT 上下文 ID 类型
 *
 * @note 2026-08-15 (ecual 合并): 由 uint16 加宽为 uint32，与 ecual 版
 * Dlt_ContextConfig 表的 4 字节打包 ASCII 语义对齐（如 0x434D444C == "CMDL"）。
 * 仓库内无既有代码依赖 uint16 宽度（原仅类型定义，无使用点），加宽零影响。
 */
typedef uint32 Dlt_ContextIdType;

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
    Dlt_PriorityType priority;  /**< 消息优先级 */
    uint32      sessionId;      /**< 会话 ID */
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
    Dlt_MessageCategoryType    messageType;  /**< 消息类型 */
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
/*                      DLT 上下文/缓冲区配置类型 (ecual 合并)                */
/* ========================================================================== */

/**
 * @brief DLT 上下文配置类型
 *
 * @note 语义对齐 ecual 版 Dlt_ContextConfig 链接期配置表；description 定长 32 字节
 * 与 Dlt_Cfg.h 中 DLT_MAX_CONTEXT_DESCRIPTION (32U) 保持一致。
 */
typedef struct {
    Dlt_ApplicationIdType appId;                    /**< 应用 ID (4 字节打包 ASCII) */
    Dlt_ContextIdType     contextId;                /**< 上下文 ID (4 字节打包 ASCII) */
    Dlt_LogLevelType      logLevel;                 /**< 默认日志级别 */
    Dlt_TraceStatusType   traceStatus;              /**< 默认跟踪状态 */
    uint8                 description[32U];         /**< 上下文描述 (定长, 同 DLT_MAX_CONTEXT_DESCRIPTION) */
    boolean               registered;               /**< 是否已注册 */
} Dlt_ContextType;

/**
 * @brief DLT 缓冲配置类型
 *
 * @note data 定长 4096 字节与 Dlt_Cfg.h 中 DLT_BUFFER_SIZE (4096U) 保持一致
 * (services 版取 4096U，ecual 版为 2048U)。
 */
typedef struct {
    uint8   data[4096U];  /**< 缓冲数据区 (同 DLT_BUFFER_SIZE) */
    uint16  writeIndex;   /**< 写索引 */
    uint16  readIndex;    /**< 读索引 */
    uint16  count;        /**< 消息计数 */
    boolean locked;       /**< 锁定标志 */
} Dlt_BufferType;

/**
 * @brief DLT 上下文分组配置类型 (按 appId 分组, 用于批量操作)
 */
typedef struct {
    Dlt_ApplicationIdType appId;            /**< 应用 ID */
    uint16                contextStartIndex; /**< 起始 context 索引 */
    uint16                contextCount;      /**< context 数量 */
} Dlt_ContextGroupType;

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
    uint32 timestamp;         /**< 时间戳 (微秒) */
    uint32 sessionId;         /**< 会话 ID */
    uint32 sequenceCounter;   /**< 序列计数器 */
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
    DLT_STATE_INIT = 1U,      /**< 初始化中 */
    DLT_STATE_READY = 2U,     /**< 就绪 */
    DLT_STATE_BUSY = 3U,      /**< 忙 */
    DLT_STATE_ERROR = 4U,     /**< 错误状态 */
    DLT_STATE_STOPPED = 5U    /**< 已停止 */
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
