/**
 * @file dlt_diag.c
 * @brief DLT诊断集成实现
 */

#include "dlt_diag.h"
#include <string.h>

void Dlt_Diag_Init(void) {
    /* 注册到DCM - 由外部调用 */
}

Std_ReturnType Dlt_Diag_ReadData(uint16_t did, 
                                  uint8_t *data, 
                                  uint16_t max_len, 
                                  uint16_t *actual_len) {
    if (!data || !actual_len || max_len == 0) {
        return E_NOT_OK;
    }
    
    *actual_len = 0;
    
    switch (did) {
        case DID_DLT_STATUS: {
            if (max_len < sizeof(DltDiagStatus_t)) {
                return E_NOT_OK;
            }
            DltDiagStatus_t *status = (DltDiagStatus_t*)data;
            status->initialized = Dlt_IsInitialized() ? 1 : 0;
            status->mode = DLT_MODE_EXTERNAL;
            status->default_level = DLT_LOG_INFO;
            status->context_count = 8; /* 默认注册的上下文数量 */
            status->buffer_size = DLT_BUFFER_SIZE;
            status->buffer_used = 0; /* 需要实现查询 */
            *actual_len = sizeof(DltDiagStatus_t);
            return E_OK;
        }
        
        case DID_DLT_CONFIG: {
            if (max_len < sizeof(DltDiagConfig_t)) {
                return E_NOT_OK;
            }
            DltDiagConfig_t *config = (DltDiagConfig_t*)data;
            config->enable_timestamp = 1;
            config->enable_ecu_id = 1;
            config->enable_session_id = 1;
            config->udp_port = 3490;
            config->file_output = 0;
            *actual_len = sizeof(DltDiagConfig_t);
            return E_OK;
        }
        
        case DID_DLT_STATS: {
            if (max_len < sizeof(DltDiagStats_t)) {
                return E_NOT_OK;
            }
            const Dlt_StatisticsType *stats = Dlt_GetStatistics();
            DltDiagStats_t *diag_stats = (DltDiagStats_t*)data;
            if (stats) {
                diag_stats->messages_sent = stats->messages_sent;
                diag_stats->messages_dropped = stats->messages_dropped;
                diag_stats->buffer_overflows = stats->buffer_overflows;
                diag_stats->bytes_written = stats->bytes_written;
                diag_stats->bytes_dropped = stats->bytes_dropped;
            } else {
                memset(diag_stats, 0, sizeof(DltDiagStats_t));
            }
            *actual_len = sizeof(DltDiagStats_t);
            return E_OK;
        }
        
        case DID_DLT_BUFFER_USAGE: {
            if (max_len < 2) {
                return E_NOT_OK;
            }
            /* 简化实现 */
            data[0] = 0;
            data[1] = 0;
            *actual_len = 2;
            return E_OK;
        }
        
        default:
            return E_NOT_OK;
    }
}

Std_ReturnType Dlt_Diag_WriteData(uint16_t did, 
                                   const uint8_t *data, 
                                   uint16_t len) {
    if (!data || len == 0) {
        return E_NOT_OK;
    }
    
    switch (did) {
        case DID_DLT_CONTROL: {
            if (len < 1) return E_NOT_OK;
            bool enable = (data[0] != 0);
            if (enable) {
                if (!Dlt_IsInitialized()) {
                    Dlt_Init(NULL);
                }
            } else {
                Dlt_DeInit();
            }
            return E_OK;
        }
        
        case DID_DLT_SET_DEFAULT_LEVEL: {
            if (len < 1) return E_NOT_OK;
            Dlt_LogLevelType level = (Dlt_LogLevelType)data[0];
            Dlt_SetDefaultLogLevel(level);
            return E_OK;
        }
        
        case DID_DLT_FLUSH_BUFFER: {
            Dlt_ClearBuffer();
            return E_OK;
        }
        
        case DID_DLT_SET_CONTEXT_LEVEL: {
            if (len < sizeof(DltDiagSetLevelReq_t)) return E_NOT_OK;
            const DltDiagSetLevelReq_t *req = (const DltDiagSetLevelReq_t*)data;
            /* 需要获取上下文指针，简化实现 */
            (void)req;
            return E_OK;
        }
        
        case DID_DLT_TRIGGER_SNAPSHOT: {
            /* 触发快照 */
            Dlt_ContextType ctx;
            Dlt_RegisterContext(&ctx, "DIAG", "SNAP", "Snapshot");
            Dlt_Snapshot(&ctx);
            return E_OK;
        }
        
        default:
            return E_NOT_OK;
    }
}
