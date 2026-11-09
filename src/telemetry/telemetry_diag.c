/**
 * @file telemetry_diag.c
 * @brief 诊断集成实现
 */
/* @req SHALL_TELEMETRY */


#include "telemetry_diag.h"
#include <string.h>

void Tel_Diag_Init(void) {    /* 注册到DCM - 由外部调用 */
}

/* Tel_Diag_ReadData 按 DID 读取助手 (等价重构): 各 DID 的长度校验、
 * stats 空指针兜底与字节序组装逻辑逐路径保留 */
static Std_ReturnType Tel_Diag_ReadU16Field(uint8_t *data, uint16_t max_len,
                                            uint16_t *actual_len, uint16_t value) {
    if (max_len < 2U) {
        return E_NOT_OK;
    }
    data[0] = (value >> 8) & 0xFFU;
    data[1] = value & 0xFFU;
    *actual_len = 2;
    return E_OK;
}

static Std_ReturnType Tel_Diag_ReadU32Field(uint8_t *data, uint16_t max_len,
                                            uint16_t *actual_len, uint32_t value) {
    if (max_len < 4U) {
        return E_NOT_OK;
    }
    data[0] = (value >> 24) & 0xFFU;
    data[1] = (value >> 16) & 0xFFU;
    data[2] = (value >> 8) & 0xFFU;
    data[3] = value & 0xFFU;
    *actual_len = 4;
    return E_OK;
}

static Std_ReturnType Tel_Diag_ReadByDid(uint16_t did, uint8_t *data, uint16_t max_len,
                                         uint16_t *actual_len, const TelStats_t *stats) {
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
                (void)memset(diag_stats, 0, sizeof(TelDiagStats_t));
            }
            *actual_len = sizeof(TelDiagStats_t);
            return E_OK;
        }

        case DID_TEL_BUFFER_USAGE:
            return Tel_Diag_ReadU16Field(data, max_len, actual_len,
                                         (uint16_t)(stats ? stats->current_usage : 0));

        case DID_TEL_OVERFLOW_COUNT:
            return Tel_Diag_ReadU16Field(data, max_len, actual_len,
                                         (uint16_t)(stats ? stats->overflow_cnt : 0));

        case DID_TEL_EVENT_COUNT:
            return Tel_Diag_ReadU32Field(data, max_len, actual_len,
                                         (uint32_t)(stats ? stats->total_events : 0));

        case DID_TEL_READ_EVENTS: {
            /* 读取原始事件数据 */
            TelStatus_t status = Tel_ReadEvents(data, max_len, actual_len);
            return (status == TEL_OK) ? E_OK : E_NOT_OK;
        }

        default:
            return E_NOT_OK;
    }
}

Std_ReturnType Tel_Diag_ReadData(uint16_t did, uint8_t *data, uint16_t max_len, uint16_t *actual_len) {
    if (!data || !actual_len || (max_len == 0U)) {
        return E_NOT_OK;
    }

    *actual_len = 0;
    return Tel_Diag_ReadByDid(did, data, max_len, actual_len, Tel_GetStats());
}

Std_ReturnType Tel_Diag_WriteData(uint16_t did, const uint8_t *data, uint16_t len) {
    if (!data || (len == 0U)) {
        return E_NOT_OK;
    }
    
    switch (did) {
        case DID_TEL_CONTROL: {
            if (len < 1U) { return E_NOT_OK; }
            bool enable = (data[0] != 0U);
            /* 启用/禁用埋点 - 需要调用Tel API */
            (void)enable;
            return E_OK;
        }
        
        case DID_TEL_CLEAR_BUFFER: {
            Tel_ClearBuffer();
            return E_OK;
        }
        
        case DID_TEL_SET_LEVEL: {
            if (len < 1U) { return E_NOT_OK; }
            TelLevel_t level = (TelLevel_t)data[0];
            Tel_SetGlobalLevel(level);
            return E_OK;
        }
        
        default:
            return E_NOT_OK;
    }
}
