/**
 * @file telemetry_diag.c
 * @brief 诊断集成实现
 */

#include "telemetry_diag.h"
#include <string.h>

void Tel_Diag_Init(void) {
    /* 注册到DCM - 由外部调用 */
}

Std_ReturnType Tel_Diag_ReadData(uint16_t did, uint8_t *data, uint16_t max_len, uint16_t *actual_len) {
    if (!data || !actual_len || max_len == 0) {
        return E_NOT_OK;
    }
    
    *actual_len = 0;
    const TelStats_t *stats = Tel_GetStats();
    
    switch (did) {
        case DID_TEL_STATUS: {
            if (max_len < sizeof(TelDiagStatus_t)) {
                return E_NOT_OK;
            }
            TelDiagStatus_t *status = (TelDiagStatus_t*)data;
            status->enabled = 1;
            status->current_level = 3; /* INFO */
            status->buffer_size = TEL_BUFFER_SIZE;
            status->buffer_used = stats ? stats->current_usage : 0;
            status->module_mask = 0xFF;
            *actual_len = sizeof(TelDiagStatus_t);
            return E_OK;
        }
        
        case DID_TEL_STATS: {
            if (max_len < sizeof(TelDiagStats_t)) {
                return E_NOT_OK;
            }
            TelDiagStats_t *diag_stats = (TelDiagStats_t*)data;
            if (stats) {
                diag_stats->total_events = stats->total_events;
                diag_stats->dropped_events = stats->dropped_events;
                diag_stats->overflow_count = stats->overflow_cnt;
                diag_stats->avg_event_size = stats->avg_event_size;
            } else {
                memset(diag_stats, 0, sizeof(TelDiagStats_t));
            }
            *actual_len = sizeof(TelDiagStats_t);
            return E_OK;
        }
        
        case DID_TEL_BUFFER_USAGE: {
            if (max_len < 2) {
                return E_NOT_OK;
            }
            uint16_t usage = stats ? stats->current_usage : 0;
            data[0] = (usage >> 8) & 0xFF;
            data[1] = usage & 0xFF;
            *actual_len = 2;
            return E_OK;
        }
        
        case DID_TEL_OVERFLOW_COUNT: {
            if (max_len < 2) {
                return E_NOT_OK;
            }
            uint16_t overflow = stats ? stats->overflow_cnt : 0;
            data[0] = (overflow >> 8) & 0xFF;
            data[1] = overflow & 0xFF;
            *actual_len = 2;
            return E_OK;
        }
        
        case DID_TEL_EVENT_COUNT: {
            if (max_len < 4) {
                return E_NOT_OK;
            }
            uint32_t count = stats ? stats->total_events : 0;
            data[0] = (count >> 24) & 0xFF;
            data[1] = (count >> 16) & 0xFF;
            data[2] = (count >> 8) & 0xFF;
            data[3] = count & 0xFF;
            *actual_len = 4;
            return E_OK;
        }
        
        case DID_TEL_READ_EVENTS: {
            /* 读取原始事件数据 */
            TelStatus_t status = Tel_ReadEvents(data, max_len, actual_len);
            return (status == TEL_OK) ? E_OK : E_NOT_OK;
        }
        
        default:
            return E_NOT_OK;
    }
}

Std_ReturnType Tel_Diag_WriteData(uint16_t did, const uint8_t *data, uint16_t len) {
    if (!data || len == 0) {
        return E_NOT_OK;
    }
    
    switch (did) {
        case DID_TEL_CONTROL: {
            if (len < 1) return E_NOT_OK;
            bool enable = (data[0] != 0);
            /* 启用/禁用埋点 - 需要调用Tel API */
            (void)enable;
            return E_OK;
        }
        
        case DID_TEL_CLEAR_BUFFER: {
            Tel_ClearBuffer();
            return E_OK;
        }
        
        case DID_TEL_SET_LEVEL: {
            if (len < 1) return E_NOT_OK;
            TelLevel_t level = (TelLevel_t)data[0];
            Tel_SetGlobalLevel(level);
            return E_OK;
        }
        
        default:
            return E_NOT_OK;
    }
}
