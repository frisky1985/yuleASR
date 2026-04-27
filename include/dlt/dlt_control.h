/**
 * @file dlt_control.h
 * @brief DLT控制消息处理
 * 
 * 完整实现AutoSAR DLT规范的控制消息接收、解析和响应功能
 * 支持所有标准服务ID和用户自定义服务
 */

#ifndef DLT_CONTROL_H
#define DLT_CONTROL_H

#include "dlt.h"
#include "dlt_payload.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* 控制消息处理回调类型                                                    */
/*===========================================================================*/

typedef Dlt_ReturnType (*Dlt_ControlCallback_t)(
    Dlt_ServiceIdType service_id,
    const uint8_t *payload,
    uint16_t payload_length,
    uint8_t *response,
    uint16_t *response_length,
    void *user_data
);

/*===========================================================================*/
/* 控制服务配置                                                         */
/*===========================================================================*/

typedef struct {
    Dlt_ServiceIdType service_id;
    Dlt_ControlCallback_t callback;
    void *user_data;
} Dlt_ControlServiceType;

#define DLT_MAX_CONTROL_SERVICES 32

typedef struct {
    Dlt_ControlServiceType services[DLT_MAX_CONTROL_SERVICES];
    uint8_t service_count;
    bool allow_unknown_services;
} Dlt_ControlConfigType;

/*===========================================================================*/
/* API函数声明                                                          */
/*===========================================================================*/

/**
 * @brief 初始化控制消息处理模块
 */
Dlt_ReturnType Dlt_Control_Init(const Dlt_ControlConfigType *config);

/**
 * @brief 反初始化控制消息处理模块
 */
void Dlt_Control_DeInit(void);

/**
 * @brief 注册自定义控制服务回调
 */
Dlt_ReturnType Dlt_Control_RegisterService(
    Dlt_ServiceIdType service_id,
    Dlt_ControlCallback_t callback,
    void *user_data
);

/**
 * @brief 注销控制服务
 */
Dlt_ReturnType Dlt_Control_UnregisterService(Dlt_ServiceIdType service_id);

/**
 * @brief 处理接收到的控制消息
 * 
 * @param data 完整的DLT消息数据（包含头部）
 * @param length 数据长度
 * @return Dlt_ReturnType 处理结果
 */
Dlt_ReturnType Dlt_Control_ProcessMessage(const uint8_t *data, uint16_t length);

/**
 * @brief 构建控制消息响应
 * 
 * @param service_id 服务ID
 * @param response_code 响应码
 * @param response_data 响应数据（可为NULL）
 * @param response_data_length 响应数据长度
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param out_length 输出实际长度
 */
Dlt_ReturnType Dlt_Control_BuildResponse(
    Dlt_ServiceIdType service_id,
    Dlt_ControlResponseType response_code,
    const uint8_t *response_data,
    uint16_t response_data_length,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *out_length
);

/*===========================================================================*/
/* 标准控制服务实现                                                     */
/*===========================================================================*/

/**
 * @brief 设置日志级别服务 (0x01)
 */
Dlt_ReturnType Dlt_Control_SetLogLevel(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置Trace状态服务 (0x02)
 */
Dlt_ReturnType Dlt_Control_SetTraceStatus(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 获取日志信息服务 (0x03)
 */
Dlt_ReturnType Dlt_Control_GetLogInfo(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 获取默认日志级别服务 (0x04)
 */
Dlt_ReturnType Dlt_Control_GetDefaultLogLevel(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 保存配置服务 (0x05)
 */
Dlt_ReturnType Dlt_Control_StoreConfig(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 重置到出厂默认值服务 (0x06)
 */
Dlt_ReturnType Dlt_Control_ResetToFactoryDefault(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置通信接口状态服务 (0x07)
 */
Dlt_ReturnType Dlt_Control_SetComInterfaceStatus(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置通信接口带宽服务 (0x08)
 */
Dlt_ReturnType Dlt_Control_SetComInterfaceMaxBandwidth(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置详细模式服务 (0x09)
 */
Dlt_ReturnType Dlt_Control_SetVerboseMode(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置消息过滤服务 (0x0A)
 */
Dlt_ReturnType Dlt_Control_SetMessageFiltering(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置时序包服务 (0x0B)
 */
Dlt_ReturnType Dlt_Control_SetTimingPackets(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 获取本地时间服务 (0x0C)
 */
Dlt_ReturnType Dlt_Control_GetLocalTime(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 使用ECU ID服务 (0x0D)
 */
Dlt_ReturnType Dlt_Control_UseEcuId(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 使用Session ID服务 (0x0E)
 */
Dlt_ReturnType Dlt_Control_UseSessionId(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 使用时间戳服务 (0x0F)
 */
Dlt_ReturnType Dlt_Control_UseTimestamp(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 使用扩展头服务 (0x10)
 */
Dlt_ReturnType Dlt_Control_UseExtendedHeader(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置默认日志级别服务 (0x11)
 */
Dlt_ReturnType Dlt_Control_SetDefaultLogLevel(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 设置默认Trace状态服务 (0x12)
 */
Dlt_ReturnType Dlt_Control_SetDefaultTraceStatus(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/**
 * @brief 获取软件版本服务 (0x13)
 */
Dlt_ReturnType Dlt_Control_GetSoftwareVersion(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length);

/*===========================================================================*/
/* 内部状态获取函数                                                      */
/*===========================================================================*/

typedef struct {
    bool verbose_mode;
    bool message_filtering;
    bool timing_packets;
    bool use_ecu_id;
    bool use_session_id;
    bool use_timestamp;
    bool use_extended_header;
    Dlt_LogLevelType default_log_level;
    bool default_trace_status;
    uint32_t max_bandwidth;
} Dlt_ControlStateType;

const Dlt_ControlStateType* Dlt_Control_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* DLT_CONTROL_H */
