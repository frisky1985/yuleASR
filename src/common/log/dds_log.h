/*******************************************************************************
 * @file dds_log.h
 * @brief DDS日志系统 - 车载场景
 *
 * 功能特点：
 * - 分级日志 (DEBUG/INFO/WARN/ERROR/FATAL)
 * - 日志轮转
 * - 异步日志
 * - 车载安全审计日志
 * - OTA日志上报支持
 *
 * 安全合规：ISO 26262, AUTOSAR
 ******************************************************************************/

#ifndef DDS_LOG_H
#define DDS_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 版本和定义
 *============================================================================*/
#define DDS_LOG_VERSION_MAJOR   1
#define DDS_LOG_VERSION_MINOR   0
#define DDS_LOG_VERSION_PATCH   0

/* 日志系统名称 */
#define DDS_LOG_SYSTEM_NAME     "DDS_LOG"

/*==============================================================================
 * 日志级别定义
 *============================================================================*/
typedef enum {
    DDS_LOG_LEVEL_NONE  = 0,    /* 关闭所有日志 */
    DDS_LOG_LEVEL_FATAL = 1,    /* 致命错误 */
    DDS_LOG_LEVEL_ERROR = 2,    /* 错误 */
    DDS_LOG_LEVEL_WARN  = 3,    /* 警告 */
    DDS_LOG_LEVEL_INFO  = 4,    /* 信息 */
    DDS_LOG_LEVEL_DEBUG = 5,    /* 调试 */
    DDS_LOG_LEVEL_MAX
} dds_log_level_t;

/* 日志级别名称 */
#define DDS_LOG_LEVEL_FATAL_NAME    "FATAL"
#define DDS_LOG_LEVEL_ERROR_NAME    "ERROR"
#define DDS_LOG_LEVEL_WARN_NAME     "WARN"
#define DDS_LOG_LEVEL_INFO_NAME     "INFO"
#define DDS_LOG_LEVEL_DEBUG_NAME    "DEBUG"

/*==============================================================================
 * 日志类型定义
 *============================================================================*/
typedef enum {
    DDS_LOG_TYPE_RUNTIME    = 0,    /* 运行时日志 */
    DDS_LOG_TYPE_AUDIT      = 1,    /* 审计日志 */
    DDS_LOG_TYPE_SECURITY   = 2,    /* 安全日志 */
    DDS_LOG_TYPE_DIAGNOSTIC = 3,    /* 诊断日志 */
    DDS_LOG_TYPE_OTA        = 4,    /* OTA日志 */
    DDS_LOG_TYPE_MAX
} dds_log_type_t;

/*==============================================================================
 * 常量定义
 *============================================================================*/
#define DDS_LOG_MAX_MESSAGE_LEN     2048    /* 最大消息长度 */
#define DDS_LOG_MAX_PATH_LEN        256     /* 最大路径长度 */
#define DDS_LOG_MAX_TAG_LEN         64      /* 最大标签长度 */
#define DDS_LOG_MAX_MODULE_LEN      32      /* 最大模块名长度 */
#define DDS_LOG_MAX_FILE_NAME       128     /* 最大文件名长度 */
#define DDS_LOG_BUFFER_SIZE         (1024 * 1024)   /* 1MB 缓冲区 */
#define DDS_LOG_QUEUE_SIZE          1024    /* 异步队列大小 */
#define DDS_LOG_MAX_FILES           10      /* 最多保留文件数 */
#define DDS_LOG_FILE_SIZE_MAX       (10 * 1024 * 1024)  /* 单文件最大 10MB */

/* 车载特定定义 */
#define DDS_LOG_UDS_SESSION_ID_LEN  16      /* UDS会话ID长度 */
#define DDS_LOG_ECUID_LEN           8       /* ECU ID长度 */

/*==============================================================================
 * 日志条目结构
 *============================================================================*/
typedef struct dds_log_entry {
    uint64_t            timestamp_ns;           /* 纳秒级时间戳 */
    struct timespec     timestamp;              /* 标准时间 */
    dds_log_level_t     level;                  /* 日志级别 */
    dds_log_type_t      type;                   /* 日志类型 */
    char                module[DDS_LOG_MAX_MODULE_LEN];     /* 模块名 */
    char                tag[DDS_LOG_MAX_TAG_LEN];           /* 标签 */
    char                file[DDS_LOG_MAX_FILE_NAME];        /* 文件名 */
    int                 line;                   /* 行号 */
    uint32_t            thread_id;              /* 线程ID */
    char                message[DDS_LOG_MAX_MESSAGE_LEN];   /* 消息内容 */
    
    /* 车载扩展 */
    char                ecu_id[DDS_LOG_ECUID_LEN];          /* ECU ID */
    char                uds_session[DDS_LOG_UDS_SESSION_ID_LEN]; /* UDS会话 */
    uint32_t            sequence_num;           /* 序列号 */
} dds_log_entry_t;

/*==============================================================================
 * 日志配置结构
 *============================================================================*/
typedef struct dds_log_config {
    /* 基本配置 */
    dds_log_level_t     level;                              /* 全局日志级别 */
    bool                enable_async;                       /* 启用异步 */
    bool                enable_console;                     /* 输出到控制台 */
    bool                enable_file;                        /* 输出到文件 */
    bool                enable_rotation;                    /* 启用日志轮转 */
    
    /* 文件配置 */
    char                log_dir[DDS_LOG_MAX_PATH_LEN];      /* 日志目录 */
    char                log_name[DDS_LOG_MAX_FILE_NAME];    /* 日志名称 */
    uint32_t            max_file_size;                      /* 单文件最大大小 */
    uint32_t            max_files;                          /* 最多文件数 */
    
    /* 异步配置 */
    uint32_t            queue_size;                         /* 队列大小 */
    uint32_t            flush_interval_ms;                  /* 刷新间隔 */
    
    /* 格式配置 */
    bool                enable_color;                       /* 启用颜色 */
    bool                enable_timestamp;                   /* 显示时间戳 */
    bool                enable_location;                    /* 显示位置 */
    bool                enable_thread_id;                   /* 显示线程ID */
    
    /* 车载配置 */
    bool                enable_audit;                       /* 启用审计日志 */
    bool                enable_uds_trace;                   /* UDS追踪 */
    char                ecu_id[DDS_LOG_ECUID_LEN];          /* ECU ID */
} dds_log_config_t;

/*==============================================================================
 * 审计日志事件类型
 *============================================================================*/
typedef enum {
    DDS_AUDIT_EVENT_LOGIN           = 0x0001,
    DDS_AUDIT_EVENT_LOGOUT          = 0x0002,
    DDS_AUDIT_EVENT_CONFIG_CHANGE   = 0x0004,
    DDS_AUDIT_EVENT_SECURITY_ALERT  = 0x0008,
    DDS_AUDIT_EVENT_DATA_ACCESS     = 0x0010,
    DDS_AUDIT_EVENT_SYSTEM_START    = 0x0020,
    DDS_AUDIT_EVENT_SYSTEM_STOP     = 0x0040,
    DDS_AUDIT_EVENT_OTA_START       = 0x0080,
    DDS_AUDIT_EVENT_OTA_COMPLETE    = 0x0100,
    DDS_AUDIT_EVENT_FAULT_DETECTED  = 0x0200,
    DDS_AUDIT_EVENT_DIAG_SESSION    = 0x0400,
    DDS_AUDIT_EVENT_MAX
} dds_audit_event_type_t;

/* 审计事件结构 */
typedef struct dds_audit_event {
    uint64_t                timestamp_ns;
    dds_audit_event_type_t  event_type;
    char                    ecu_id[DDS_LOG_ECUID_LEN];
    char                    uds_session[DDS_LOG_UDS_SESSION_ID_LEN];
    char                    description[DDS_LOG_MAX_MESSAGE_LEN];
    uint32_t                severity;
} dds_audit_event_t;

/*==============================================================================
 * OTA日志上报结构
 *============================================================================*/
typedef struct dds_ota_log_entry {
    uint64_t    timestamp_ns;
    char        ecu_id[DDS_LOG_ECUID_LEN];
    char        uds_session[DDS_LOG_UDS_SESSION_ID_LEN];
    uint32_t    log_count;
    uint32_t    log_size;
    uint8_t*    compressed_data;
    uint32_t    compressed_size;
} dds_ota_log_entry_t;

/* OTA日志上报回调 */
typedef void (*dds_ota_upload_callback_t)(
    const dds_ota_log_entry_t* entry,
    bool success,
    void* user_data
);

/*==============================================================================
 * 日志统计信息
 *============================================================================*/
typedef struct dds_log_stats {
    uint64_t    total_logs;             /* 总日志数 */
    uint64_t    dropped_logs;           /* 丢弃日志数 */
    uint64_t    bytes_written;          /* 写入字节数 */
    uint64_t    last_flush_time;        /* 上次刷新时间 */
    uint32_t    current_queue_size;     /* 当前队列大小 */
    uint32_t    current_file_size;      /* 当前文件大小 */
    uint32_t    rotation_count;         /* 轮转次数 */
} dds_log_stats_t;

/*==============================================================================
 * 回调函数类型
 *============================================================================*/
/* 日志输出回调 */
typedef void (*dds_log_output_callback_t)(
    const dds_log_entry_t* entry,
    void* user_data
);

/* 审计事件回调 */
typedef void (*dds_audit_callback_t)(
    const dds_audit_event_t* event,
    void* user_data
);

/*==============================================================================
 * 全局状态和生命周期
 *============================================================================*/

/**
 * @brief 初始化日志系统
 * @param config 日志配置
 * @return 0 成功, 其他 失败
 */
int dds_log_init(const dds_log_config_t* config);

/**
 * @brief 反初始化日志系统
 */
void dds_log_deinit(void);

/**
 * @brief 检查日志系统是否初始化
 */
bool dds_log_is_initialized(void);

/**
 * @brief 刷新日志缓冲区（异步模式）
 */
void dds_log_flush(void);

/*==============================================================================
 * 日志记录接口
 *============================================================================*/

/**
 * @brief 内部日志记录函数
 */
void dds_log_write(dds_log_level_t level, dds_log_type_t type,
                   const char* module, const char* tag,
                   const char* file, int line,
                   const char* fmt, ...);

/**
 * @brief 变参日志记录函数
 */
void dds_log_vwrite(dds_log_level_t level, dds_log_type_t type,
                    const char* module, const char* tag,
                    const char* file, int line,
                    const char* fmt, va_list args);

/*==============================================================================
 * 便捷宏
 *============================================================================*/
#define DDS_LOG_DEBUG(module, tag, ...) \
    dds_log_write(DDS_LOG_LEVEL_DEBUG, DDS_LOG_TYPE_RUNTIME, module, tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

#define DDS_LOG_INFO(module, tag, ...) \
    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_RUNTIME, module, tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

#define DDS_LOG_WARN(module, tag, ...) \
    dds_log_write(DDS_LOG_LEVEL_WARN, DDS_LOG_TYPE_RUNTIME, module, tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

#define DDS_LOG_ERROR(module, tag, ...) \
    dds_log_write(DDS_LOG_LEVEL_ERROR, DDS_LOG_TYPE_RUNTIME, module, tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

#define DDS_LOG_FATAL(module, tag, ...) \
    dds_log_write(DDS_LOG_LEVEL_FATAL, DDS_LOG_TYPE_RUNTIME, module, tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

/* 审计日志宏 */
#define DDS_LOG_AUDIT(event_type, ...) \
    dds_log_audit(event_type, __FILE__, __LINE__, __VA_ARGS__)

/* 安全日志宏 */
#define DDS_LOG_SECURITY(tag, ...) \
    dds_log_write(DDS_LOG_LEVEL_WARN, DDS_LOG_TYPE_SECURITY, "SECURITY", tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

/* 诊断日志宏 */
#define DDS_LOG_DIAG(level, tag, ...) \
    dds_log_write(level, DDS_LOG_TYPE_DIAGNOSTIC, "DIAG", tag, \
                  __FILE__, __LINE__, __VA_ARGS__)

/*==============================================================================
 * 审计日志接口
 *============================================================================*/

/**
 * @brief 记录审计事件
 */
void dds_log_audit(dds_audit_event_type_t event_type,
                   const char* file, int line,
                   const char* fmt, ...);

/**
 * @brief 注册审计事件回调
 */
int dds_log_register_audit_callback(dds_audit_callback_t callback, void* user_data);

/**
 * @brief 注销审计事件回调
 */
void dds_log_unregister_audit_callback(dds_audit_callback_t callback);

/*==============================================================================
 * 配置和控制接口
 *============================================================================*/

/**
 * @brief 设置日志级别
 */
void dds_log_set_level(dds_log_level_t level);

/**
 * @brief 获取日志级别
 */
dds_log_level_t dds_log_get_level(void);

/**
 * @brief 设置模块日志级别
 */
void dds_log_set_module_level(const char* module, dds_log_level_t level);

/**
 * @brief 获取模块日志级别
 */
dds_log_level_t dds_log_get_module_level(const char* module);

/**
 * @brief 启用/禁用控制台输出
 */
void dds_log_enable_console(bool enable);

/**
 * @brief 启用/禁用文件输出
 */
void dds_log_enable_file(bool enable);

/*==============================================================================
 * OTA日志上报接口
 *============================================================================*/

/**
 * @brief 设置OTA日志回调
 */
void dds_log_set_ota_callback(dds_ota_upload_callback_t callback, void* user_data);

/**
 * @brief 触发OTA日志上报
 * @param session_id UDS会话ID
 * @param compress 是否压缩
 * @return 0 成功, 其他 失败
 */
int dds_log_trigger_ota_upload(const char* session_id, bool compress);

/**
 * @brief 获取OTA日志缓冲区
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param actual_size 实际数据大小
 * @return 0 成功, 其他 失败
 */
int dds_log_get_ota_buffer(uint8_t* buffer, uint32_t buffer_size, uint32_t* actual_size);

/*==============================================================================
 * UDS诊断接口
 *============================================================================*/

/**
 * @brief 设置UDS会话ID
 */
void dds_log_set_uds_session(const char* session_id);

/**
 * @brief 清除UDS会话ID
 */
void dds_log_clear_uds_session(void);

/**
 * @brief 记录UDS诊断事件
 */
void dds_log_uds_event(uint8_t service_id, uint8_t response_code, const char* description);

/*==============================================================================
 * 统计和状态接口
 *============================================================================*/

/**
 * @brief 获取日志统计信息
 */
void dds_log_get_stats(dds_log_stats_t* stats);

/**
 * @brief 获取当前日志文件路径
 */
int dds_log_get_current_file(char* path, size_t path_size);

/**
 * @brief 获取日志文件列表
 * @param files 文件路径数组
 * @param max_files 最大文件数
 * @param actual_count 实际文件数
 * @return 0 成功, 其他 失败
 */
int dds_log_get_file_list(char** files, uint32_t max_files, uint32_t* actual_count);

/*==============================================================================
 * 高级功能
 *============================================================================*/

/**
 * @brief 注册自定义输出回调
 */
int dds_log_register_output_callback(dds_log_output_callback_t callback, void* user_data);

/**
 * @brief 注销自定义输出回调
 */
void dds_log_unregister_output_callback(dds_log_output_callback_t callback);

/**
 * @brief 设置后处理回调（用于故障转储等）
 */
typedef void (*dds_log_fatal_handler_t)(const dds_log_entry_t* entry);
void dds_log_set_fatal_handler(dds_log_fatal_handler_t handler);

/**
 * @brief 导出日志到pcap格式（用于网络分析）
 */
int dds_log_export_to_pcap(const char* log_file, const char* pcap_file);

#ifdef __cplusplus
}
#endif

#endif /* DDS_LOG_H */
