/**
 * @file dlt_adapter.h
 * @brief DLT与Telemetry模块的适配层
 * 
 * 提供从Telemetry埋点事件到DLT消息的映射和转换
 */

#ifndef DLT_ADAPTER_H
#define DLT_ADAPTER_H

#include "dlt.h"
#include "telemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* 适配器配置                                                            */
/*===========================================================================*/
typedef struct {
    bool auto_register_contexts;    /* 自动为埋点模块注册DLT上下文 */
    Dlt_LogLevelType default_level; /* 默认日志级别 */
    bool enable_event_mapping;      /* 启用事件映射 */
    bool enable_network_trace;      /* 启用网络追踪 */
} Dlt_AdapterConfigType;

/*===========================================================================*/
/* 模块到DLT上下文的映射                                                 */
/*===========================================================================*/
typedef struct {
    uint8_t tel_module;             /* Telemetry模块ID */
    char app_id[5];                 /* DLT Application ID (4字符+\0) */
    char context_id[5];             /* DLT Context ID (4字符+\0) */
    Dlt_LogLevelType log_level;     /* 日志级别 */
    Dlt_ContextType *ctx;           /* DLT上下文指针 */
} Dlt_ModuleMappingType;

/*===========================================================================*/
/* 事件到日志消息的映射                                                   */
/*===========================================================================*/
typedef struct {
    uint8_t tel_event_id;           /* Telemetry事件ID */
    uint8_t tel_module;             /* 所属模块 */
    Dlt_LogLevelType level;         /* 日志级别 */
    const char *message_template;   /* 消息模板 */
} Dlt_EventMappingType;

/*===========================================================================*/
/* API函数                                                              */
/*===========================================================================*/

/**
 * @brief 初始化DLT适配器
 */
Dlt_ReturnType Dlt_Adapter_Init(const Dlt_AdapterConfigType *config);

/**
 * @brief 反初始化
 */
void Dlt_Adapter_DeInit(void);

/**
 * @brief 注册埋点模块映射
 */
Dlt_ReturnType Dlt_Adapter_RegisterModule(uint8_t tel_module,
                                           const char *app_id,
                                           const char *context_id);

/**
 * @brief 注销埋点模块映射
 */
Dlt_ReturnType Dlt_Adapter_UnregisterModule(uint8_t tel_module);

/**
 * @brief 将Telemetry事件转换为DLT消息
 */
Dlt_ReturnType Dlt_Adapter_ConvertEvent(const TelEntry_t *tel_event);

/**
 * @brief 处理埋点事件回调
 */
void Dlt_Adapter_TelemetryCallback(const TelEntry_t *entry, void *user_data);

/**
 * @brief 设置模块日志级别
 */
Dlt_ReturnType Dlt_Adapter_SetModuleLevel(uint8_t tel_module, 
                                           Dlt_LogLevelType level);

/**
 * @brief 获取模块的DLT上下文
 */
Dlt_ContextType* Dlt_Adapter_GetContext(uint8_t tel_module);

/*===========================================================================*/
/* 预定义的模块映射                                                      */
/*===========================================================================*/

/* 默认模块映射表 */
extern const Dlt_ModuleMappingType g_dlt_default_mappings[];
extern const uint8_t g_dlt_default_mapping_count;

/* 预定义映射 */
#define DLT_MAP_SYS     "SYS", "MAIN"
#define DLT_MAP_ECU     "ECU", "MGMT"
#define DLT_MAP_BSWM    "BSW", "MGMT"
#define DLT_MAP_DDS     "DDS", "COMM"
#define DLT_MAP_ETH     "ETH", "LINK"
#define DLT_MAP_SEC     "SEC", "CRYPT"
#define DLT_MAP_DIAG    "UDS", "DIAG"
#define DLT_MAP_OTA     "OTA", "UPDT"

#ifdef __cplusplus
}
#endif

#endif /* DLT_ADAPTER_H */
