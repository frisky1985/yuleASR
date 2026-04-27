/**
 * @file dlt.h
 * @brief AutoSAR DLT (Diagnostic and Logging Trace) 核心头文件
 * @version R21-11
 */

#ifndef DLT_H
#define DLT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* 版本和配置                                                              */
/*===========================================================================*/
#define DLT_MAJOR_VERSION       1
#define DLT_MINOR_VERSION       0
#define DLT_PATCH_VERSION       0

#ifndef DLT_BUFFER_SIZE
#define DLT_BUFFER_SIZE         (64 * 1024)  /* 64KB 默认缓冲区 */
#endif

#ifndef DLT_MAX_CONTEXTS
#define DLT_MAX_CONTEXTS        32
#endif

#ifndef DLT_MAX_MESSAGE_SIZE
#define DLT_MAX_MESSAGE_SIZE    4096
#endif

/*===========================================================================*/
/* 返回类型                                                                */
/*===========================================================================*/
typedef enum {
    DLT_RETURN_OK = 0,
    DLT_RETURN_ERROR,
    DLT_RETURN_WRONG_PARAMETER,
    DLT_RETURN_BUFFER_FULL,
    DLT_RETURN_PIPE_FULL,
    DLT_RETURN_PIPE_ERROR,
    DLT_RETURN_LOGGING_DISABLED,
    DLT_RETURN_USER_BUFFER_FULL
} Dlt_ReturnType;

/*===========================================================================*/
/* 日志级别                                                                */
/*===========================================================================*/
typedef enum {
    DLT_LOG_OFF = 0x00,
    DLT_LOG_FATAL = 0x01,
    DLT_LOG_ERROR = 0x02,
    DLT_LOG_WARN = 0x03,
    DLT_LOG_INFO = 0x04,
    DLT_LOG_DEBUG = 0x05,
    DLT_LOG_VERBOSE = 0x06
} Dlt_LogLevelType;

/*===========================================================================*/
/* 消息类型                                                                */
/*===========================================================================*/
typedef enum {
    DLT_TYPE_LOG = 0x00,
    DLT_TYPE_APP_TRACE = 0x01,
    DLT_TYPE_NW_TRACE = 0x02,
    DLT_TYPE_CONTROL = 0x03
} Dlt_MessageType;

/*===========================================================================*/
/* 消息子类型                                                              */
/*===========================================================================*/
typedef enum {
    /* Log messages */
    DLT_TRACE_VARIABLE = 0x01,
    DLT_TRACE_FUNCTION_IN = 0x02,
    DLT_TRACE_FUNCTION_OUT = 0x03,
    DLT_TRACE_STATE = 0x04,
    DLT_TRACE_VFB = 0x05,
    
    /* Network messages */
    DLT_NW_TRACE_IPC = 0x01,
    DLT_NW_TRACE_CAN = 0x02,
    DLT_NW_TRACE_FLEXRAY = 0x03,
    DLT_NW_TRACE_MOST = 0x04,
    DLT_NW_TRACE_ETHERNET = 0x05,
    DLT_NW_TRACE_SOMEIP = 0x06,
    
    /* Control messages */
    DLT_CONTROL_REQUEST = 0x01,
    DLT_CONTROL_RESPONSE = 0x02,
    DLT_CONTROL_TIME = 0x03
} Dlt_MessageSubtype;

/*===========================================================================*/
/* 消息头部结构                                                              */
/*===========================================================================*/
typedef struct __attribute__((packed)) {
    uint8_t  header_type;       /* 头部类型标志 */
    uint8_t  message_counter;   /* 消息计数器 */
    uint16_t length;            /* 完整消息长度 */
} Dlt_StandardHeaderType;

/* Header Type 标志位 */
#define DLT_HTYP_UEH            0x01  /* 使用Extended Header */
#define DLT_HTYP_MSBF           0x02  /* 大端字节序 */
#define DLT_HTYP_WEID           0x04  /* With ECU ID */
#define DLT_HTYP_WSID           0x08  /* With Session ID */
#define DLT_HTYP_WTMS           0x10  /* With Timestamp */
#define DLT_HTYP_VERS           0xE0  /* 版本号 (1-7) */
#define DLT_HTYP_VERS_SHIFT     5

typedef struct __attribute__((packed)) {
    uint32_t msb;               /* ECU ID */
    uint32_t session_id;        /* Session ID */
    uint32_t timestamp;         /* 时间戳 (DMT 毫秒) */
} Dlt_StandardHeaderExtraType;

typedef struct __attribute__((packed)) {
    uint8_t  msin;              /* Message Info */
    uint8_t  noar;              /* Number of Arguments */
    uint32_t apid;              /* Application ID */
    uint32_t ctid;              /* Context ID */
} Dlt_ExtendedHeaderType;

/* Message Info 字段 */
#define DLT_MSIN_VERB           0x01  /* Verbose mode */
#define DLT_MSIN_MSTP_MASK      0x0E  /* Message Type */
#define DLT_MSIN_MSTP_SHIFT     1
#define DLT_MSIN_MTIN_MASK      0xF0  /* Message Type Info */
#define DLT_MSIN_MTIN_SHIFT     4

/*===========================================================================*/
/* DLT上下文                                                             */
/*===========================================================================*/
typedef struct {
    uint32_t app_id;            /* Application ID (4 chars) */
    uint32_t context_id;        /* Context ID (4 chars) */
    Dlt_LogLevelType log_level; /* 当前日志级别 */
    uint8_t  trace_status;      /* 追踪状态 */
    void    *user_data;         /* 用户数据 */
} Dlt_ContextType;

/*===========================================================================*/
/* DLT配置                                                               */
/*===========================================================================*/
typedef enum {
    DLT_MODE_OFF = 0,
    DLT_MODE_EXTERNAL,
    DLT_MODE_INTERNAL,
    DLT_MODE_BOTH
} Dlt_ModeType;

typedef struct {
    Dlt_ModeType mode;
    Dlt_LogLevelType default_level;
    bool enable_timestamp;
    bool enable_ecu_id;
    bool enable_session_id;
    uint32_t buffer_size;
    uint16_t udp_port;
    bool enable_file_output;
    char log_file_path[256];
} Dlt_ConfigType;

/*===========================================================================*/
/* DLT统计                                                               */
/*===========================================================================*/
typedef struct {
    uint32_t messages_sent;
    uint32_t messages_dropped;
    uint32_t buffer_overflows;
    uint32_t bytes_written;
    uint32_t bytes_dropped;
} Dlt_StatisticsType;

/*===========================================================================*/
/* API函数声明                                                            */
/*===========================================================================*/

/* 初始化和配置 */
Dlt_ReturnType Dlt_Init(const Dlt_ConfigType *config);
void Dlt_DeInit(void);
Dlt_ReturnType Dlt_SetDefaultLogLevel(Dlt_LogLevelType level);

/* 上下文管理 */
Dlt_ReturnType Dlt_RegisterContext(Dlt_ContextType *ctx,
                                    const char *app_id,
                                    const char *context_id,
                                    const char *description);
Dlt_ReturnType Dlt_UnregisterContext(Dlt_ContextType *ctx);
Dlt_ReturnType Dlt_SetContextLogLevel(Dlt_ContextType *ctx, Dlt_LogLevelType level);

/* 基础日志记录 */
Dlt_ReturnType Dlt_LogString(Dlt_ContextType *ctx,
                              Dlt_LogLevelType level,
                              const char *msg);

/* 格式化日志 */
Dlt_ReturnType Dlt_LogFormatString(Dlt_ContextType *ctx,
                                    Dlt_LogLevelType level,
                                    const char *fmt, ...);

/* 带数据的日志 */
Dlt_ReturnType Dlt_LogData(Dlt_ContextType *ctx,
                            Dlt_LogLevelType level,
                            const uint8_t *data,
                            uint16_t length);

/* 追踪函数 */
Dlt_ReturnType Dlt_TraceFunction(Dlt_ContextType *ctx,
                                  Dlt_MessageSubtype subtype,
                                  const char *func_name);

Dlt_ReturnType Dlt_TraceVariable(Dlt_ContextType *ctx,
                                  const char *var_name,
                                  const void *var_value,
                                  uint8_t var_size);

/* 网络追踪 */
Dlt_ReturnType Dlt_TraceNetwork(Dlt_ContextType *ctx,
                                 Dlt_MessageSubtype subtype,
                                 const uint8_t *data,
                                 uint16_t length);

/* 控制消息 */
Dlt_ReturnType Dlt_SendControlMessage(uint8_t service_id,
                                       const uint8_t *payload,
                                       uint16_t length);

/* 快照功能 */
Dlt_ReturnType Dlt_Snapshot(const Dlt_ContextType *ctx);

/* 缓冲区操作 */
void Dlt_FlushBuffer(void);
void Dlt_ClearBuffer(void);

/* 统计信息 */
const Dlt_StatisticsType* Dlt_GetStatistics(void);
void Dlt_ResetStatistics(void);

/* 状态检查 */
bool Dlt_IsInitialized(void);
bool Dlt_IsContextRegistered(const Dlt_ContextType *ctx);

/*===========================================================================*/
/* 宏定义 - 便捷日志记录                                                      */
/*===========================================================================*/

#define DLT_LOG_FATAL(ctx, msg)     Dlt_LogString(ctx, DLT_LOG_FATAL, msg)
#define DLT_LOG_ERROR(ctx, msg)     Dlt_LogString(ctx, DLT_LOG_ERROR, msg)
#define DLT_LOG_WARNING(ctx, msg)   Dlt_LogString(ctx, DLT_LOG_WARN, msg)
#define DLT_LOG_INFO(ctx, msg)      Dlt_LogString(ctx, DLT_LOG_INFO, msg)
#define DLT_LOG_DEBUG(ctx, msg)     Dlt_LogString(ctx, DLT_LOG_DEBUG, msg)
#define DLT_LOG_VERBOSE(ctx, msg)   Dlt_LogString(ctx, DLT_LOG_VERBOSE, msg)

#define DLT_LOGF_FATAL(ctx, ...)    Dlt_LogFormatString(ctx, DLT_LOG_FATAL, __VA_ARGS__)
#define DLT_LOGF_ERROR(ctx, ...)    Dlt_LogFormatString(ctx, DLT_LOG_ERROR, __VA_ARGS__)
#define DLT_LOGF_WARNING(ctx, ...)  Dlt_LogFormatString(ctx, DLT_LOG_WARN, __VA_ARGS__)
#define DLT_LOGF_INFO(ctx, ...)     Dlt_LogFormatString(ctx, DLT_LOG_INFO, __VA_ARGS__)
#define DLT_LOGF_DEBUG(ctx, ...)    Dlt_LogFormatString(ctx, DLT_LOG_DEBUG, __VA_ARGS__)
#define DLT_LOGF_VERBOSE(ctx, ...)  Dlt_LogFormatString(ctx, DLT_LOG_VERBOSE, __VA_ARGS__)

/* 追踪宏 */
#define DLT_TRACE_IN(ctx, func)     Dlt_TraceFunction(ctx, DLT_TRACE_FUNCTION_IN, func)
#define DLT_TRACE_OUT(ctx, func)    Dlt_TraceFunction(ctx, DLT_TRACE_FUNCTION_OUT, func)
#define DLT_TRACE_STATE(ctx, state) Dlt_TraceFunction(ctx, DLT_TRACE_STATE, state)

/* 网络追踪宏 */
#define DLT_NET_CAN(ctx, data, len)     Dlt_TraceNetwork(ctx, DLT_NW_TRACE_CAN, data, len)
#define DLT_NET_ETH(ctx, data, len)     Dlt_TraceNetwork(ctx, DLT_NW_TRACE_ETHERNET, data, len)
#define DLT_NET_SOMEIP(ctx, data, len)  Dlt_TraceNetwork(ctx, DLT_NW_TRACE_SOMEIP, data, len)

/*===========================================================================*/
/* 内部函数 (仅内部使用)                                                      */
/*===========================================================================*/
#ifdef DLT_INTERNAL
Dlt_ReturnType Dlt_SendMessage(const uint8_t *data, uint16_t length);
Dlt_ReturnType Dlt_EncodeHeader(uint8_t *buffer, 
                                 const Dlt_StandardHeaderType *std_hdr,
                                 const Dlt_ExtendedHeaderType *ext_hdr,
                                 uint16_t *encoded_len);
Dlt_ReturnType Dlt_ProcessControlMessage(const uint8_t *data, uint16_t length);
#endif

#ifdef __cplusplus
}
#endif

#endif /* DLT_H */
