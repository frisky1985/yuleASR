/**
 * @file dlt_adapter.c
 * @brief DLT-Telemetry 适配层实现
 */

#include "dlt_adapter.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================*/
/* 内部状态                                                                */
/*===========================================================================*/

static struct {
    bool initialized;
    Dlt_AdapterConfigType config;
    Dlt_ModuleMappingType mappings[TEL_MOD_MAX];
    Dlt_ContextType contexts[TEL_MOD_MAX];
} g_adapter_state = {0};

/* 默认模块映射表 */
static const Dlt_ModuleMappingType default_mappings[] = {
    {TEL_MOD_SYS,  "SYS", "MAIN", DLT_LOG_INFO, NULL},
    {TEL_MOD_ECU,  "ECU", "MGMT", DLT_LOG_INFO, NULL},
    {TEL_MOD_BSWM, "BSW", "MGMT", DLT_LOG_INFO, NULL},
    {TEL_MOD_DDS,  "DDS", "COMM", DLT_LOG_DEBUG, NULL},
    {TEL_MOD_ETH,  "ETH", "LINK", DLT_LOG_DEBUG, NULL},
    {TEL_MOD_SEC,  "SEC", "CRYPT", DLT_LOG_WARNING, NULL},
    {TEL_MOD_DIAG, "UDS", "DIAG", DLT_LOG_INFO, NULL},
    {TEL_MOD_OTA,  "OTA", "UPDT", DLT_LOG_INFO, NULL}
};

const Dlt_ModuleMappingType* g_dlt_default_mappings = default_mappings;
const uint8_t g_dlt_default_mapping_count = sizeof(default_mappings) / sizeof(default_mappings[0]);

/*===========================================================================*/
/* 辅助函数                                                                */
/*===========================================================================*/

static const char* get_event_name(uint8_t module, uint8_t event_id) {
    /* 根据模块和事件ID返回名称 */
    (void)module;
    (void)event_id;
    return "EVENT";
}

static Dlt_LogLevelType map_tel_level_to_dlt(TelLevel_t tel_level) {
    switch (tel_level) {
        case TEL_LEVEL_FATAL: return DLT_LOG_FATAL;
        case TEL_LEVEL_ERROR: return DLT_LOG_ERROR;
        case TEL_LEVEL_WARNING: return DLT_LOG_WARN;
        case TEL_LEVEL_INFO: return DLT_LOG_INFO;
        case TEL_LEVEL_DEBUG: return DLT_LOG_DEBUG;
        default: return DLT_LOG_VERBOSE;
    }
}

static const char* get_event_type_name(TelEventType_t type) {
    switch (type) {
        case TEL_TYPE_INSTANT: return "INSTANT";
        case TEL_TYPE_COUNTER: return "COUNTER";
        case TEL_TYPE_STATE: return "STATE";
        case TEL_TYPE_METRIC: return "METRIC";
        case TEL_TYPE_LOG: return "LOG";
        default: return "UNKNOWN";
    }
}

/*===========================================================================*/
/* API实现                                                                 */
/*===========================================================================*/

Dlt_ReturnType Dlt_Adapter_Init(const Dlt_AdapterConfigType *config) {
    memset(&g_adapter_state, 0, sizeof(g_adapter_state));
    
    if (config) {
        memcpy(&g_adapter_state.config, config, sizeof(Dlt_AdapterConfigType));
    } else {
        g_adapter_state.config.auto_register_contexts = true;
        g_adapter_state.config.default_level = DLT_LOG_INFO;
        g_adapter_state.config.enable_event_mapping = true;
        g_adapter_state.config.enable_network_trace = true;
    }
    
    /* 初始化默认映射 */
    if (g_adapter_state.config.auto_register_contexts) {
        for (uint8_t i = 0; i < g_dlt_default_mapping_count; i++) {
            const Dlt_ModuleMappingType *def_map = &default_mappings[i];
            Dlt_ReturnType ret = Dlt_Adapter_RegisterModule(
                def_map->tel_module,
                def_map->app_id,
                def_map->context_id
            );
            if (ret != DLT_RETURN_OK) {
                return ret;
            }
        }
    }
    
    g_adapter_state.initialized = true;
    return DLT_RETURN_OK;
}

void Dlt_Adapter_DeInit(void) {
    if (!g_adapter_state.initialized) {
        return;
    }
    
    /* 注销所有上下文 */
    for (uint8_t i = 0; i < TEL_MOD_MAX; i++) {
        if (g_adapter_state.mappings[i].ctx != NULL) {
            Dlt_UnregisterContext(g_adapter_state.mappings[i].ctx);
        }
    }
    
    memset(&g_adapter_state, 0, sizeof(g_adapter_state));
}

Dlt_ReturnType Dlt_Adapter_RegisterModule(uint8_t tel_module,
                                           const char *app_id,
                                           const char *context_id) {
    if (tel_module >= TEL_MOD_MAX || !app_id || !context_id) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (g_adapter_state.mappings[tel_module].ctx != NULL) {
        /* 已注册，先注销 */
        Dlt_UnregisterContext(g_adapter_state.mappings[tel_module].ctx);
    }
    
    /* 注册DLT上下文 */
    Dlt_ContextType *ctx = &g_adapter_state.contexts[tel_module];
    Dlt_ReturnType ret = Dlt_RegisterContext(ctx, app_id, context_id, NULL);
    if (ret != DLT_RETURN_OK) {
        return ret;
    }
    
    /* 保存映射 */
    g_adapter_state.mappings[tel_module].tel_module = tel_module;
    strncpy(g_adapter_state.mappings[tel_module].app_id, app_id, 4);
    g_adapter_state.mappings[tel_module].app_id[4] = '\0';
    strncpy(g_adapter_state.mappings[tel_module].context_id, context_id, 4);
    g_adapter_state.mappings[tel_module].context_id[4] = '\0';
    g_adapter_state.mappings[tel_module].log_level = g_adapter_state.config.default_level;
    g_adapter_state.mappings[tel_module].ctx = ctx;
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Adapter_UnregisterModule(uint8_t tel_module) {
    if (tel_module >= TEL_MOD_MAX) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (g_adapter_state.mappings[tel_module].ctx != NULL) {
        Dlt_UnregisterContext(g_adapter_state.mappings[tel_module].ctx);
        memset(&g_adapter_state.mappings[tel_module], 0, 
               sizeof(Dlt_ModuleMappingType));
    }
    
    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_Adapter_ConvertEvent(const TelEntry_t *tel_event) {
    if (!g_adapter_state.initialized || !tel_event) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    uint8_t module = tel_event->header.module_id;
    if (module >= TEL_MOD_MAX || g_adapter_state.mappings[module].ctx == NULL) {
        return DLT_RETURN_ERROR;
    }
    
    Dlt_ContextType *ctx = g_adapter_state.mappings[module].ctx;
    Dlt_LogLevelType level = map_tel_level_to_dlt(tel_event->header.level);
    
    char msg[256];
    const char *event_name = get_event_name(module, tel_event->header.event_id);
    
    switch (tel_event->header.event_type) {
        case TEL_TYPE_INSTANT:
            snprintf(msg, sizeof(msg), "[%s] Instant event", event_name);
            return Dlt_LogString(ctx, level, msg);
            
        case TEL_TYPE_COUNTER:
            snprintf(msg, sizeof(msg), "[%s] Counter: %u", 
                     event_name, tel_event->payload.counter_value);
            return Dlt_LogString(ctx, level, msg);
            
        case TEL_TYPE_STATE:
            snprintf(msg, sizeof(msg), "[%s] State: %u -> %u",
                     event_name, 
                     tel_event->payload.state.old_state,
                     tel_event->payload.state.new_state);
            return Dlt_LogString(ctx, level, msg);
            
        case TEL_TYPE_METRIC:
            snprintf(msg, sizeof(msg), "[%s] Metric: %d",
                     event_name, tel_event->payload.metric_value);
            return Dlt_LogString(ctx, level, msg);
            
        case TEL_TYPE_LOG:
            snprintf(msg, sizeof(msg), "[%s] %s",
                     event_name, tel_event->payload.log_msg);
            return Dlt_LogString(ctx, level, msg);
            
        default:
            snprintf(msg, sizeof(msg), "[%s] Unknown type %u",
                     event_name, tel_event->header.event_type);
            return Dlt_LogString(ctx, level, msg);
    }
}

void Dlt_Adapter_TelemetryCallback(const TelEntry_t *entry, void *user_data) {
    (void)user_data;
    
    if (!g_adapter_state.initialized || !entry) {
        return;
    }
    
    Dlt_Adapter_ConvertEvent(entry);
}

Dlt_ReturnType Dlt_Adapter_SetModuleLevel(uint8_t tel_module, 
                                           Dlt_LogLevelType level) {
    if (tel_module >= TEL_MOD_MAX) {
        return DLT_RETURN_WRONG_PARAMETER;
    }
    
    if (g_adapter_state.mappings[tel_module].ctx == NULL) {
        return DLT_RETURN_ERROR;
    }
    
    g_adapter_state.mappings[tel_module].log_level = level;
    return Dlt_SetContextLogLevel(g_adapter_state.mappings[tel_module].ctx, level);
}

Dlt_ContextType* Dlt_Adapter_GetContext(uint8_t tel_module) {
    if (tel_module >= TEL_MOD_MAX) {
        return NULL;
    }
    return g_adapter_state.mappings[tel_module].ctx;
}
