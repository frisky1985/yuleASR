/**
 * @file dlt.c
 * @brief AutoSAR DLT 核心实现
 */

#define DLT_INTERNAL
#include "dlt.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================*/
/* 内部状态                                                                */
/*===========================================================================*/
typedef struct {
    uint8_t data[DLT_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile bool full;
} Dlt_BufferType;

static struct {
    bool initialized;
    Dlt_ConfigType config;
    Dlt_BufferType buffer;
    Dlt_ContextType contexts[DLT_MAX_CONTEXTS];
    uint8_t context_count;
    uint8_t message_counter;
    Dlt_StatisticsType stats;
    uint32_t session_id;
} g_dlt_state = {0};

static const char* g_ecu_id = "ECU1";

/*===========================================================================*/
/* 辅助函数                                                                */
/*===========================================================================*/
static uint32_t pack_fourcc(const char *str) {
    if (!str || strlen(str) < 4) return 0;
    return ((uint32_t)str[0] << 24) |
           ((uint32_t)str[1] << 16) |
           ((uint32_t)str[2] << 8) |
           (uint32_t)str[3];
}

static uint32_t dlt_get_timestamp(void) {
    /* 实际实现需要调用OS提供的时间服务 */
    static uint32_t fake_time = 0;
    return fake_time++;
}

static bool check_log_level(const Dlt_ContextType *ctx, Dlt_LogLevelType msg_level) {
    if (!ctx) return false;
    return (msg_level <= ctx->log_level) && (msg_level <= g_dlt_state.config.default_level);
}

static Dlt_ReturnType buffer_write(const uint8_t *data, uint16_t length) {
    if (!data || length == 0) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    Dlt_BufferType *buf = &g_dlt_state.buffer;
    
    /* 检查空间 */
    uint16_t available;
    if (buf->full) {
        available = 0;
    } else if (buf->head >= buf->tail) {
        available = DLT_BUFFER_SIZE - (buf->head - buf->tail);
    } else {
        available = buf->tail - buf->head;
    }
    
    if (length > available) {
        g_dlt_state.stats.buffer_overflows++;
        g_dlt_state.stats.messages_dropped++;
        return DLT_RETURN_BUFFER_FULL;
    }
    
    /* 写入数据 */
    for (uint16_t i = 0; i < length; i++) {
        buf->data[buf->head] = data[i];
        buf->head = (buf->head + 1) % DLT_BUFFER_SIZE;
    }
    
    if (buf->head == buf->tail) {
        buf->full = true;
    }
    
    g_dlt_state.stats.messages_sent++;
    g_dlt_state.stats.bytes_written += length;
    
    return DLT_RETURN_OK;
}

/*===========================================================================*/
/* API实现                                                                 */
/*===========================================================================*/

Dlt_ReturnType Dlt_Init(const Dlt_ConfigType *config) {
    if (g_dlt_state.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    memset(&g_dlt_state, 0, sizeof(g_dlt_state));
    
    if (config) {
        memcpy(&g_dlt_state.config, config, sizeof(Dlt_ConfigType));
    } else {
        /* 默认配置 */
        g_dlt_state.config.mode = DLT_MODE_EXTERNAL;
        g_dlt_state.config.default_level = DLT_LOG_INFO;
        g_dlt_state.config.enable_timestamp = true;
        g_dlt_state.config.enable_ecu_id = true;
        g_dlt_state.config.enable_session_id = true;
        g_dlt_state.config.buffer_size = DLT_BUFFER_SIZE;
        g_dlt_state.config.udp_port = 3490;
        g_dlt_state.config.enable_file_output = false;
    }
    
    g_dlt_state.session_id = 1;
    g_dlt_state.initialized = true;
    
    /* 发送初始化日志 */
    Dlt_ContextType sys_ctx;
    Dlt_RegisterContext(&sys_ctx, "SYS", "MAIN", "DLT System");
    Dlt_LogString(&sys_ctx, DLT_LOG_INFO, "DLT initialized");
    
    return DLT_RETURN_OK;
}

void Dlt_DeInit(void) {
    if (!g_dlt_state.initialized) {
        return;
    }
    
    Dlt_FlushBuffer();
    g_dlt_state.initialized = false;
}

Dlt_ReturnType Dlt_SetDefaultLogLevel(Dlt_LogLevelType level) {
    if (!g_dlt_state.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    g_dlt_state.config.default_level = level;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_RegisterContext(Dlt_ContextType *ctx,
                                    const char *app_id,
                                    const char *context_id,
                                    const char *description) {
    (void)description;
    
    if (!ctx || !app_id || !context_id) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (!g_dlt_state.initialized) {
        return DLT_RETURN_ERROR;
    }
    
    if (g_dlt_state.context_count >= DLT_MAX_CONTEXTS) {
        return DLT_RETURN_ERROR;
    }
    
    ctx->app_id = pack_fourcc(app_id);
    ctx->context_id = pack_fourcc(context_id);
    ctx->log_level = g_dlt_state.config.default_level;
    ctx->trace_status = 0;
    ctx->user_data = NULL;
    
    /* 注册到全局列表 */
    memcpy(&g_dlt_state.contexts[g_dlt_state.context_count], 
           ctx, sizeof(Dlt_ContextType));
    g_dlt_state.context_count++;
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_UnregisterContext(Dlt_ContextType *ctx) {
    if (!ctx || !g_dlt_state.initialized) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    /* 简单实现：清零 */
    memset(ctx, 0, sizeof(Dlt_ContextType));
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_SetContextLogLevel(Dlt_ContextType *ctx, Dlt_LogLevelType level) {
    if (!ctx || !g_dlt_state.initialized) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    ctx->log_level = level;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_LogString(Dlt_ContextType *ctx,
                              Dlt_LogLevelType level,
                              const char *msg) {
    if (!g_dlt_state.initialized || !ctx || !msg) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (!check_log_level(ctx, level)) {
        return DLT_RETURN_LOGGING_DISABLED;
    }
    
    uint16_t msg_len = strlen(msg);
    if (msg_len > 65535) {
        msg_len = 65535;
    }
    
    /* 构建DLT消息 */
    uint8_t buffer[DLT_MAX_MESSAGE_SIZE];
    uint16_t offset = 0;
    
    /* Standard Header */
    Dlt_StandardHeaderType std_hdr = {0};
    std_hdr.header_type = DLT_HTYP_VERS | (1 << DLT_HTYP_VERS_SHIFT);
    if (g_dlt_state.config.enable_timestamp) {
        std_hdr.header_type |= DLT_HTYP_WTMS;
    }
    if (g_dlt_state.config.enable_ecu_id) {
        std_hdr.header_type |= DLT_HTYP_WEID;
    }
    if (g_dlt_state.config.enable_session_id) {
        std_hdr.header_type |= DLT_HTYP_WSID;
    }
    std_hdr.header_type |= DLT_HTYP_UEH; /* Extended Header */
    std_hdr.message_counter = g_dlt_state.message_counter++;
    
    uint16_t payload_len = 10 + 4 + msg_len; /* Ext header + type info + msg */
    std_hdr.length = sizeof(Dlt_StandardHeaderType) + 
                     (g_dlt_state.config.enable_timestamp ? 4 : 0) +
                     (g_dlt_state.config.enable_ecu_id ? 4 : 0) +
                     (g_dlt_state.config.enable_session_id ? 4 : 0) +
                     sizeof(Dlt_ExtendedHeaderType) + payload_len;
    
    memcpy(buffer + offset, &std_hdr, sizeof(std_hdr));
    offset += sizeof(std_hdr);
    
    /* Extra Header Fields */
    if (g_dlt_state.config.enable_ecu_id) {
        memcpy(buffer + offset, g_ecu_id, 4);
        offset += 4;
    }
    if (g_dlt_state.config.enable_session_id) {
        memcpy(buffer + offset, &g_dlt_state.session_id, 4);
        offset += 4;
    }
    if (g_dlt_state.config.enable_timestamp) {
        uint32_t timestamp = dlt_get_timestamp();
        memcpy(buffer + offset, &timestamp, 4);
        offset += 4;
    }
    
    /* Extended Header */
    Dlt_ExtendedHeaderType ext_hdr = {0};
    ext_hdr.msin = DLT_MSIN_VERB;
    ext_hdr.msin |= (DLT_TYPE_LOG << DLT_MSIN_MSTP_SHIFT);
    ext_hdr.msin |= ((level & 0x0F) << DLT_MSIN_MTIN_SHIFT);
    ext_hdr.noar = 1;
    ext_hdr.apid = ctx->app_id;
    ext_hdr.ctid = ctx->context_id;
    
    memcpy(buffer + offset, &ext_hdr, sizeof(ext_hdr));
    offset += sizeof(ext_hdr);
    
    /* Payload: Type Info + String */
    buffer[offset++] = 0x00; /* String type */
    buffer[offset++] = 0x02;
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x00;
    buffer[offset++] = (msg_len >> 8) & 0xFF;
    buffer[offset++] = msg_len & 0xFF;
    memcpy(buffer + offset, msg, msg_len);
    offset += msg_len;
    
    /* 发送 */
    return Dlt_SendMessage(buffer, std_hdr.length);
}

Dlt_ReturnType Dlt_LogFormatString(Dlt_ContextType *ctx,
                                    Dlt_LogLevelType level,
                                    const char *fmt, ...) {
    if (!g_dlt_state.initialized || !ctx || !fmt) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (!check_log_level(ctx, level)) {
        return DLT_RETURN_LOGGING_DISABLED;
    }
    
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    return Dlt_LogString(ctx, level, buffer);
}

Dlt_ReturnType Dlt_LogData(Dlt_ContextType *ctx,
                            Dlt_LogLevelType level,
                            const uint8_t *data,
                            uint16_t length) {
    if (!g_dlt_state.initialized || !ctx || !data) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (!check_log_level(ctx, level)) {
        return DLT_RETURN_LOGGING_DISABLED;
    }
    
    /* 简化实现：转换为十六进制字符串 */
    char hex_str[DLT_MAX_MESSAGE_SIZE];
    uint16_t hex_len = 0;
    
    for (uint16_t i = 0; i < length && hex_len < sizeof(hex_str) - 4; i++) {
        hex_len += snprintf(hex_str + hex_len, sizeof(hex_str) - hex_len, 
                            "%02X ", data[i]);
    }
    
    return Dlt_LogString(ctx, level, hex_str);
}

Dlt_ReturnType Dlt_TraceFunction(Dlt_ContextType *ctx,
                                  Dlt_MessageSubtype subtype,
                                  const char *func_name) {
    if (!g_dlt_state.initialized || !ctx) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    const char *prefix = (subtype == DLT_TRACE_FUNCTION_IN) ? "ENTER: " :
                         (subtype == DLT_TRACE_FUNCTION_OUT) ? "EXIT: " : "STATE: ";
    
    char msg[256];
    snprintf(msg, sizeof(msg), "%s%s", prefix, func_name ? func_name : "unknown");
    
    return Dlt_LogString(ctx, DLT_LOG_DEBUG, msg);
}

Dlt_ReturnType Dlt_TraceVariable(Dlt_ContextType *ctx,
                                  const char *var_name,
                                  const void *var_value,
                                  uint8_t var_size) {
    if (!g_dlt_state.initialized || !ctx) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    char msg[256];
    uint32_t value = 0;
    
    if (var_value && var_size <= 4) {
        memcpy(&value, var_value, var_size);
    }
    
    snprintf(msg, sizeof(msg), "VAR %s = 0x%X (%u)", 
             var_name ? var_name : "unknown", value, value);
    
    return Dlt_LogString(ctx, DLT_LOG_DEBUG, msg);
}

Dlt_ReturnType Dlt_TraceNetwork(Dlt_ContextType *ctx,
                                 Dlt_MessageSubtype subtype,
                                 const uint8_t *data,
                                 uint16_t length) {
    if (!g_dlt_state.initialized || !ctx || !data) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    const char *net_type = "UNKNOWN";
    switch (subtype) {
        case DLT_NW_TRACE_CAN:      net_type = "CAN"; break;
        case DLT_NW_TRACE_ETHERNET: net_type = "ETH"; break;
        case DLT_NW_TRACE_SOMEIP:   net_type = "SOME/IP"; break;
        default: break;
    }
    
    char msg[64];
    snprintf(msg, sizeof(msg), "NW %s: %d bytes", net_type, length);
    
    return Dlt_LogString(ctx, DLT_LOG_DEBUG, msg);
}

Dlt_ReturnType Dlt_SendControlMessage(uint8_t service_id,
                                       const uint8_t *payload,
                                       uint16_t length) {
    (void)service_id;
    (void)payload;
    (void)length;
    
    /* 控制消息处理 - 需要实现控制通信 */
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Snapshot(const Dlt_ContextType *ctx) {
    if (!g_dlt_state.initialized || !ctx) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    return Dlt_LogString(ctx, DLT_LOG_INFO, "SNAPSHOT");
}

void Dlt_FlushBuffer(void) {
    /* 刷新输出 - 实际实现需要根据输出通道调用相应flush */
}

void Dlt_ClearBuffer(void) {
    if (!g_dlt_state.initialized) {
        return;
    }
    
    g_dlt_state.buffer.head = 0;
    g_dlt_state.buffer.tail = 0;
    g_dlt_state.buffer.full = false;
}

const Dlt_StatisticsType* Dlt_GetStatistics(void) {
    return &g_dlt_state.stats;
}

void Dlt_ResetStatistics(void) {
    memset(&g_dlt_state.stats, 0, sizeof(Dlt_StatisticsType));
}

bool Dlt_IsInitialized(void) {
    return g_dlt_state.initialized;
}

bool Dlt_IsContextRegistered(const Dlt_ContextType *ctx) {
    if (!ctx || !g_dlt_state.initialized) {
        return false;
    }
    
    for (uint8_t i = 0; i < g_dlt_state.context_count; i++) {
        if (&g_dlt_state.contexts[i] == ctx) {
            return true;
        }
    }
    return false;
}

/*===========================================================================*/
/* 内部函数实现                                                            */
/*===========================================================================*/

Dlt_ReturnType Dlt_SendMessage(const uint8_t *data, uint16_t length) {
    if (!data || length == 0) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    /* 写入内部缓冲区 */
    return buffer_write(data, length);
}

Dlt_ReturnType Dlt_EncodeHeader(uint8_t *buffer, 
                                 const Dlt_StandardHeaderType *std_hdr,
                                 const Dlt_ExtendedHeaderType *ext_hdr,
                                 uint16_t *encoded_len) {
    if (!buffer || !std_hdr || !encoded_len) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    uint16_t offset = 0;
    memcpy(buffer + offset, std_hdr, sizeof(*std_hdr));
    offset += sizeof(*std_hdr);
    
    if (ext_hdr && (std_hdr->header_type & DLT_HTYP_UEH)) {
        memcpy(buffer + offset, ext_hdr, sizeof(*ext_hdr));
        offset += sizeof(*ext_hdr);
    }
    
    *encoded_len = offset;
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_ProcessControlMessage(const uint8_t *data, uint16_t length) {
    (void)data;
    (void)length;
    
    /* 控制消息处理逻辑 */
    return DLT_RETURN_OK;
}
