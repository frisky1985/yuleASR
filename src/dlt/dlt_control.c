/**
 * @file dlt_control.c
 * @brief DLT控制消息处理实现
 */

#include "dlt/dlt_control.h"
#include "dlt/dlt_payload.h"
#include <string.h>

/*===========================================================================*/
/* 内部状态                                                            */
/*===========================================================================*/

static struct {
    Dlt_ControlConfigType config;
    Dlt_ControlStateType state;
    bool initialized;
} g_control_ctx = {0};

/*===========================================================================*/
/* 辅助函数                                                            */
/*===========================================================================*/

static Dlt_ControlCallback_t find_service_callback(Dlt_ServiceIdType service_id) {
    for (uint8_t i = 0; i < g_control_ctx.config.service_count; i++) {
        if (g_control_ctx.config.services[i].service_id == service_id) {
            return g_control_ctx.config.services[i].callback;
        }
    }
    return NULL;
}

static void set_response_code(uint8_t *response, Dlt_ControlResponseType code) {
    if (response != NULL) {
        response[0] = code;
    }
}

/*===========================================================================*/
/* API实现                                                            */
/*===========================================================================*/

Dlt_ReturnType Dlt_Control_Init(const Dlt_ControlConfigType *config) {
    if (g_control_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    if (config != NULL) {
        memcpy(&g_control_ctx.config, config, sizeof(Dlt_ControlConfigType));
    } else {
        memset(&g_control_ctx.config, 0, sizeof(Dlt_ControlConfigType));
        g_control_ctx.config.allow_unknown_services = false;
    }
    
    /* 默认状态 */
    g_control_ctx.state.verbose_mode = true;
    g_control_ctx.state.message_filtering = true;
    g_control_ctx.state.timing_packets = false;
    g_control_ctx.state.use_ecu_id = true;
    g_control_ctx.state.use_session_id = true;
    g_control_ctx.state.use_timestamp = true;
    g_control_ctx.state.use_extended_header = true;
    g_control_ctx.state.default_log_level = DLT_LOG_INFO;
    g_control_ctx.state.default_trace_status = true;
    g_control_ctx.state.max_bandwidth = 0;
    
    g_control_ctx.initialized = true;
    return DLT_RETURN_OK;
}

void Dlt_Control_DeInit(void) {
    if (!g_control_ctx.initialized) {
        return;
    }
    
    memset(&g_control_ctx, 0, sizeof(g_control_ctx));
}

Dlt_ReturnType Dlt_Control_RegisterService(
    Dlt_ServiceIdType service_id,
    Dlt_ControlCallback_t callback,
    void *user_data) {
    
    if (!g_control_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    if (callback == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (g_control_ctx.config.service_count >= DLT_MAX_CONTROL_SERVICES) {
        return DLT_RETURN_ERROR;
    }
    
    /* 检查是否已存在 */
    for (uint8_t i = 0; i < g_control_ctx.config.service_count; i++) {
        if (g_control_ctx.config.services[i].service_id == service_id) {
            return DLT_RETURN_ERROR;
        }
    }
    
    uint8_t idx = g_control_ctx.config.service_count++;
    g_control_ctx.config.services[idx].service_id = service_id;
    g_control_ctx.config.services[idx].callback = callback;
    g_control_ctx.config.services[idx].user_data = user_data;
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_UnregisterService(Dlt_ServiceIdType service_id) {
    if (!g_control_ctx.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    for (uint8_t i = 0; i < g_control_ctx.config.service_count; i++) {
        if (g_control_ctx.config.services[i].service_id == service_id) {
            /* 移动后面的元素 */
            for (uint8_t j = i; j < g_control_ctx.config.service_count - 1; j++) {
                g_control_ctx.config.services[j] = g_control_ctx.config.services[j + 1];
            }
            g_control_ctx.config.service_count--;
            return DLT_RETURN_OK;
        }
    }
    
    return DLT_RETURN_ERROR;
}

Dlt_ReturnType Dlt_Control_ProcessMessage(const uint8_t *data, uint16_t length) {
    if (!g_control_ctx.initialized || data == NULL || length < DLT_MIN_MESSAGE_LENGTH) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    /* 解析DLT头部 */
    const Dlt_StandardHeaderType *header = (const Dlt_StandardHeaderType *)data;
    
    /* 检查协议版本 */
    if (!DLT_IS_VERSION_VALID(header->htyp)) {
        return DLT_RETURN_ERROR;
    }
    
    /* 获取消息长度 */
    uint16_t msg_length = DLT_GET_LENGTH(header->len);
    if (msg_length > length) {
        return DLT_RETURN_ERROR;
    }
    
    /* 检查是否为控制消息 */
    bool has_extended = DLT_HAS_EXTENDED_HEADER(header->htyp);
    if (!has_extended) {
        return DLT_RETURN_ERROR;
    }
    
    /* 定位扩展头 */
    uint8_t header_size = DLT_HAS_ECU_ID(header->htyp) ? DLT_EXT_HEADER_SIZE : DLT_EXT_HEADER_SIZE_NO_EID;
    if (DLT_HAS_SESSION_ID(header->htyp)) header_size += 4;
    if (DLT_HAS_TIMESTAMP(header->htyp)) header_size += 4;
    
    const Dlt_ExtendedHeaderType *ext_header = (const Dlt_ExtendedHeaderType *)(data + header_size - DLT_EXT_HEADER_SIZE);
    
    uint8_t msin = ext_header->msin;
    if (Dlt_GetMessageTypeFromMsin(msin) != DLT_TYPE_CONTROL) {
        return DLT_RETURN_ERROR;
    }
    
    /* 获取payload */
    const uint8_t *payload = data + header_size;
    uint16_t payload_length = msg_length - header_size;
    
    if (payload_length < 4) {
        return DLT_RETURN_ERROR;
    }
    
    /* 解析服务ID */
    Dlt_ServiceIdType service_id = (payload[0] << 8) | payload[1];
    uint16_t service_payload_length = (payload[2] << 8) | payload[3];
    const uint8_t *service_payload = payload + 4;
    
    if (service_payload_length > payload_length - 4) {
        return DLT_RETURN_ERROR;
    }
    
    /* 查找服务处理器 */
    Dlt_ControlCallback_t callback = find_service_callback(service_id);
    
    if (callback == NULL) {
        if (g_control_ctx.config.allow_unknown_services) {
            return DLT_RETURN_OK;
        }
        return DLT_RETURN_ERROR;
    }
    
    /* 调用服务处理器 */
    uint8_t response[256];
    uint16_t response_length = sizeof(response);
    
    Dlt_ReturnType result = callback(service_id, service_payload, service_payload_length,
                                     response, &response_length, NULL);
    
    return result;
}

Dlt_ReturnType Dlt_Control_BuildResponse(
    Dlt_ServiceIdType service_id,
    Dlt_ControlResponseType response_code,
    const uint8_t *response_data,
    uint16_t response_data_length,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *out_length) {
    
    if (buffer == NULL || out_length == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    uint16_t total_length = 4 + 1 + response_data_length;
    if (total_length > buffer_size) {
        return DLT_RETURN_ERROR;
    }
    
    /* 服务ID */
    buffer[0] = (service_id >> 8) & 0xFF;
    buffer[1] = service_id & 0xFF;
    
    /* 响应数据长度 */
    uint16_t resp_data_len = 1 + response_data_length;
    buffer[2] = (resp_data_len >> 8) & 0xFF;
    buffer[3] = resp_data_len & 0xFF;
    
    /* 响应码 */
    buffer[4] = response_code;
    
    /* 响应数据 */
    if (response_data != NULL && response_data_length > 0) {
        memcpy(buffer + 5, response_data, response_data_length);
    }
    
    *out_length = total_length;
    return DLT_RETURN_OK;
}

const Dlt_ControlStateType* Dlt_Control_GetState(void) {
    return &g_control_ctx.state;
}

/*===========================================================================*/
/* 标准服务实现                                                      */
/*===========================================================================*/

Dlt_ReturnType Dlt_Control_SetLogLevel(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 6) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    /* 解析payload: APID(4) + CTID(4) + LogLevel(1) */
    char apid[5] = {0};
    char ctid[5] = {0};
    memcpy(apid, payload, 4);
    memcpy(ctid, payload + 4, 4);
    
    Dlt_LogLevelType new_level = payload[8] & 0x07;
    
    /* TODO: 更新上下文的日志级别 */
    (void)apid;
    (void)ctid;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetTraceStatus(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 9) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    char apid[5] = {0};
    char ctid[5] = {0};
    memcpy(apid, payload, 4);
    memcpy(ctid, payload + 4, 4);
    bool new_status = payload[8] != 0;
    
    (void)apid;
    (void)ctid;
    (void)new_status;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_GetLogInfo(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    /* 返回日志信息结构 */
    set_response_code(response, DLT_RESPONSE_OK);
    
    /* 添加模式和日志级别 */
    response[1] = 0x00; /* 单个APID模式 */
    response[2] = g_control_ctx.state.default_log_level;
    response[3] = g_control_ctx.state.default_trace_status ? 0x01 : 0x00;
    
    *response_length = 4;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_GetDefaultLogLevel(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    set_response_code(response, DLT_RESPONSE_OK);
    response[1] = g_control_ctx.state.default_log_level;
    *response_length = 2;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_StoreConfig(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    /* TODO: 保存配置到持久化存储 */
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_ResetToFactoryDefault(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    g_control_ctx.state.verbose_mode = true;
    g_control_ctx.state.message_filtering = true;
    g_control_ctx.state.timing_packets = false;
    g_control_ctx.state.use_ecu_id = true;
    g_control_ctx.state.use_session_id = true;
    g_control_ctx.state.use_timestamp = true;
    g_control_ctx.state.use_extended_header = true;
    g_control_ctx.state.default_log_level = DLT_LOG_INFO;
    g_control_ctx.state.default_trace_status = true;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetComInterfaceStatus(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetComInterfaceMaxBandwidth(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 4) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    uint32_t bandwidth = ((uint32_t)payload[0] << 24) |
                         ((uint32_t)payload[1] << 16) |
                         ((uint32_t)payload[2] << 8) |
                         (uint32_t)payload[3];
    
    g_control_ctx.state.max_bandwidth = bandwidth;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetVerboseMode(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.verbose_mode = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetMessageFiltering(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.message_filtering = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetTimingPackets(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.timing_packets = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_GetLocalTime(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    /* 返回当前时间 */
    set_response_code(response, DLT_RESPONSE_OK);
    
    /* UTC时间 (微秒) */
    memset(response + 1, 0, 8);
    /* DLT时间戳 */
    memset(response + 9, 0, 4);
    
    *response_length = 13;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_UseEcuId(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.use_ecu_id = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_UseSessionId(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.use_session_id = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_UseTimestamp(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.use_timestamp = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_UseExtendedHeader(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.use_extended_header = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetDefaultLogLevel(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.default_log_level = payload[0] & 0x07;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_SetDefaultTraceStatus(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    if (payload_length < 1) {
        set_response_code(response, DLT_RESPONSE_PARAMETER_ERROR);
        *response_length = 1;
        return DLT_RETURN_OK;
    }
    
    g_control_ctx.state.default_trace_status = payload[0] != 0;
    
    set_response_code(response, DLT_RESPONSE_OK);
    *response_length = 1;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Control_GetSoftwareVersion(
    const uint8_t *payload, uint16_t payload_length,
    uint8_t *response, uint16_t *response_length) {
    
    (void)payload;
    (void)payload_length;
    
    set_response_code(response, DLT_RESPONSE_OK);
    
    /* 软件版本字符串 */
    const char *version = "yuleASR-DLT v2.1.0";
    uint16_t len = strlen(version);
    
    response[1] = (len >> 8) & 0xFF;
    response[2] = len & 0xFF;
    memcpy(response + 3, version, len);
    
    *response_length = 3 + len;
    return DLT_RETURN_OK;
}
