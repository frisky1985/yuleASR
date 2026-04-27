/**
 * @file dlt_diag.h
 * @brief DLT与AutoSAR诊断模块(Dcm)的集成
 * 
 * 提供通过UDS服务访问DLT功能
 */

#ifndef DLT_DIAG_H
#define DLT_DIAG_H

#include "dlt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* 诊断数据标识符 (DID)                                                    */
/*===========================================================================*/

#define DID_DLT_STATUS              0xF500  /* DLT模块状态 */
#define DID_DLT_CONFIG              0xF501  /* DLT配置参数 */
#define DID_DLT_STATS               0xF502  /* DLT统计信息 */
#define DID_DLT_BUFFER_USAGE        0xF503  /* 缓冲区使用率 */
#define DID_DLT_CONTEXT_LIST        0xF504  /* 注册的上下文列表 */
#define DID_DLT_LOG_LEVELS          0xF505  /* 各上下文日志级别 */

/* 控制DID */
#define DID_DLT_CONTROL             0xF510  /* 启用/禁用DLT */
#define DID_DLT_SET_DEFAULT_LEVEL   0xF511  /* 设置默认日志级别 */
#define DID_DLT_FLUSH_BUFFER        0xF512  /* 清空缓冲区 */
#define DID_DLT_SET_CONTEXT_LEVEL   0xF513  /* 设置特定上下文日志级别 */
#define DID_DLT_TRIGGER_SNAPSHOT    0xF514  /* 触发快照 */

/*===========================================================================*/
/* DID数据结构                                                            */
/*===========================================================================*/

typedef struct __attribute__((packed)) {
    uint8_t  initialized;       /* 初始化状态 */
    uint8_t  mode;              /* 运行模式 */
    uint8_t  default_level;     /* 默认日志级别 */
    uint8_t  context_count;     /* 注册的上下文数量 */
    uint16_t buffer_size;       /* 缓冲区大小 */
    uint16_t buffer_used;       /* 已使用大小 */
} DltDiagStatus_t;

typedef struct __attribute__((packed)) {
    uint8_t  enable_timestamp;  /* 时间戳使能 */
    uint8_t  enable_ecu_id;     /* ECU ID使能 */
    uint8_t  enable_session_id; /* Session ID使能 */
    uint16_t udp_port;          /* UDP端口 */
    uint8_t  file_output;       /* 文件输出使能 */
} DltDiagConfig_t;

typedef struct __attribute__((packed)) {
    uint32_t messages_sent;     /* 发送的消息数 */
    uint32_t messages_dropped;  /* 丢弃的消息数 */
    uint32_t buffer_overflows;  /* 缓冲区溢出次数 */
    uint32_t bytes_written;     /* 写入字节数 */
    uint32_t bytes_dropped;     /* 丢弃字节数 */
} DltDiagStats_t;

typedef struct __attribute__((packed)) {
    char app_id[4];             /* Application ID */
    char context_id[4];         /* Context ID */
    uint8_t log_level;          /* 当前日志级别 */
    uint8_t trace_status;       /* 追踪状态 */
} DltDiagContextInfo_t;

typedef struct __attribute__((packed)) {
    uint8_t context_index;      /* 上下文索引 */
    uint8_t log_level;          /* 新日志级别 */
} DltDiagSetLevelReq_t;

/*===========================================================================*/
/* API函数                                                              */
/*===========================================================================*/

/**
 * @brief 初始化DLT诊断集成
 */
void Dlt_Diag_Init(void);

/**
 * @brief 读取DID数据
 * @param did 数据标识符
 * @param data 输出缓冲区
 * @param max_len 最大长度
 * @param actual_len 实际读取长度
 * @return Std_ReturnType 操作结果
 */
Std_ReturnType Dlt_Diag_ReadData(uint16_t did, 
                                  uint8_t *data, 
                                  uint16_t max_len, 
                                  uint16_t *actual_len);

/**
 * @brief 写入DID数据
 * @param did 数据标识符
 * @param data 输入数据
 * @param len 数据长度
 * @return Std_ReturnType 操作结果
 */
Std_ReturnType Dlt_Diag_WriteData(uint16_t did, 
                                   const uint8_t *data, 
                                   uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* DLT_DIAG_H */
