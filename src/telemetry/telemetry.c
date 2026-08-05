/**
 * @file telemetry.c
 * @brief 嵌入式埋点系统核心实现
 */

#include "telemetry.h"

/* 静态缓冲区 - 避免动态分配 */
static uint8_t s_tel_buffer[TEL_BUFFER_SIZE];
static TelState_t s_tel_state;

/* 模块配置表 */
static struct {
    bool enabled;
    TelLevel_t min_level;
} s_module_config[TEL_MOD_COUNT] = {
    [TEL_MOD_SYS]   = {TEL_ENABLE_MODULE_SYS,   TEL_LEVEL_INFO},
    [TEL_MOD_ECUM]  = {1, TEL_LEVEL_INFO},
    [TEL_MOD_BSWM]  = {1, TEL_LEVEL_INFO},
    [TEL_MOD_DDS]   = {TEL_ENABLE_MODULE_DDS,   TEL_LEVEL_DEBUG},
    [TEL_MOD_ETH]   = {TEL_ENABLE_MODULE_ETH,   TEL_LEVEL_DEBUG},
    [TEL_MOD_SECOC] = {0, TEL_LEVEL_WARNING},
    [TEL_MOD_DIAG]  = {TEL_ENABLE_MODULE_DIAG,  TEL_LEVEL_INFO},
    [TEL_MOD_OTA]   = {1, TEL_LEVEL_INFO},
    [TEL_MOD_USER]  = {1, TEL_LEVEL_DEBUG}
};

/* 辅助函数定义 */
#define TEL_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief 获取当前时间戳 (需要平台实现)
 */
__attribute__((weak)) uint32_t Tel_Platform_GetTickMs(void) {
    /* 默认实现 - 应在平台层覆盖 */
    static uint32_t tick = 0;
    return tick++;
}

/**
 * @brief 内存屏障 (平台特定)
 */
__attribute__((weak)) void Tel_Platform_MemoryBarrier(void) {
    /* 默认空实现 */
    __asm volatile("" ::: "memory");
}

/* 环形缓冲区实现 */
TelStatus_t Tel_RB_Init(TelRingBuffer_t *rb, uint8_t *buffer, uint16_t size) {
    if (!rb || !buffer || size == 0) {
        return TEL_ERROR_NULL_PTR;
    }
    
    rb->buffer = buffer;
    rb->capacity = size;
    rb->write_idx = 0;
    rb->read_idx = 0;
    rb->overflow_cnt = 0;
    
    memset(buffer, 0, size);
    
    return TEL_OK;
}

uint16_t Tel_RB_GetUsed(const TelRingBuffer_t *rb) {
    if (!rb) return 0;
    
    volatile uint16_t write = rb->write_idx;
    volatile uint16_t read = rb->read_idx;
    
    if (write >= read) {
        return write - read;
    } else {
        return rb->capacity - read + write;
    }
}

uint16_t Tel_RB_GetFree(const TelRingBuffer_t *rb) {
    if (!rb) return 0;
    return rb->capacity - Tel_RB_GetUsed(rb) - 1;
}

bool Tel_RB_Write(TelRingBuffer_t *rb, const uint8_t *data, uint8_t len) {
    if (!rb || !data || len == 0) {
        return false;
    }
    
    if (Tel_RB_GetFree(rb) < len + 1) {
        rb->overflow_cnt++;
        return false;
    }
    
    /* 写入长度字节 */
    rb->buffer[rb->write_idx] = len;
    rb->write_idx = (rb->write_idx + 1) % rb->capacity;
    
    /* 写入数据 */
    for (uint8_t i = 0; i < len; i++) {
        rb->buffer[rb->write_idx] = data[i];
        rb->write_idx = (rb->write_idx + 1) % rb->capacity;
    }
    
    Tel_Platform_MemoryBarrier();
    
    return true;
}

bool Tel_RB_Read(TelRingBuffer_t *rb, uint8_t *data, uint8_t max_len, uint8_t *actual_len) {
    if (!rb || !data || !actual_len) {
        return false;
    }
    
    *actual_len = 0;
    
    if (rb->read_idx == rb->write_idx) {
        return false;
    }
    
    uint8_t len = rb->buffer[rb->read_idx];
    rb->read_idx = (rb->read_idx + 1) % rb->capacity;
    
    uint8_t to_read = TEL_MIN(len, max_len);
    for (uint8_t i = 0; i < to_read; i++) {
        data[i] = rb->buffer[rb->read_idx];
        rb->read_idx = (rb->read_idx + 1) % rb->capacity;
    }
    
    for (uint8_t i = to_read; i < len; i++) {
        rb->read_idx = (rb->read_idx + 1) % rb->capacity;
    }
    
    *actual_len = to_read;
    return true;
}

/* API实现 */
TelStatus_t Tel_Init(uint8_t *buffer, uint16_t size) {
    if (s_tel_state.initialized) {
        return TEL_OK;
    }
    
    if (!buffer) {
        buffer = s_tel_buffer;
        size = TEL_BUFFER_SIZE;
    }
    
    TelStatus_t status = Tel_RB_Init(&s_tel_state.ring_buffer, buffer, size);
    if (status != TEL_OK) {
        return status;
    }
    
    s_tel_state.initialized = true;
    s_tel_state.enabled = true;
    s_tel_state.global_level = TEL_LEVEL_INFO;
    s_tel_state.base_timestamp = Tel_Platform_GetTickMs();
    s_tel_state.last_relative_ts = 0;
    s_tel_state.stats.total_events = 0;
    s_tel_state.stats.dropped_events = 0;
    s_tel_state.stats.overflow_cnt = 0;
    s_tel_state.stats.avg_event_size = sizeof(TelEventHeader_t);
    
    return TEL_OK;
}

void Tel_Deinit(void) {
    if (!s_tel_state.initialized) {
        return;
    }
    
    s_tel_state.enabled = false;
    s_tel_state.initialized = false;
    Tel_ClearBuffer();
}

TelStatus_t Tel_LogEvent(TelModuleId_t module, uint8_t event_id, 
                         TelLevel_t level, const uint8_t *data, uint8_t len) {
    if (module >= TEL_MOD_COUNT) {
        return TEL_ERROR_INVALID_PARAM;
    }
    
    if (!s_tel_state.initialized || !s_tel_state.enabled) {
        return TEL_ERROR_NOT_INITIALIZED;
    }
    
    if (level > s_tel_state.global_level) {
        return TEL_OK;
    }
    
    if (!s_module_config[module].enabled || 
        level > s_module_config[module].min_level) {
        return TEL_OK;
    }
    
    if (len > TEL_MAX_EVENT_SIZE - sizeof(TelEventHeader_t)) {
        len = TEL_MAX_EVENT_SIZE - sizeof(TelEventHeader_t);
    }
    
    TelEvent_t event;
    event.header.event_id = event_id;
    
#if TEL_ENABLE_TIMESTAMP
    uint32_t now = Tel_Platform_GetTickMs();
    uint16_t relative = (uint16_t)(now - s_tel_state.base_timestamp);
    event.header.ts_lo = relative & 0xFF;
    
    if (relative > 255) {
        TelEvent_t ts_event;
        ts_event.header.event_id = 0xFF;
        ts_event.header.ts_lo = (relative >> 8) & 0xFF;
        Tel_RB_Write(&s_tel_state.ring_buffer, (uint8_t*)&ts_event, 2);
        
        s_tel_state.base_timestamp = now;
        s_tel_state.last_relative_ts = 0;
    }
#else
    event.header.ts_lo = 0;
#endif
    
    uint8_t total_len = sizeof(TelEventHeader_t);
    if (data && len > 0) {
        memcpy(event.data, data, len);
        total_len += len;
    }
    
    bool written = Tel_RB_Write(&s_tel_state.ring_buffer, (uint8_t*)&event, total_len);
    
    if (written) {
        s_tel_state.stats.total_events++;
        s_tel_state.stats.avg_event_size = 
            (s_tel_state.stats.avg_event_size * 7 + total_len) / 8;
    } else {
        s_tel_state.stats.dropped_events++;
        return TEL_ERROR_BUFFER_FULL;
    }
    
    return TEL_OK;
}

TelStatus_t Tel_LogCounter(TelModuleId_t module, uint8_t event_id, TelLevel_t level, uint8_t value) {
    return Tel_LogEvent(module, event_id, level, &value, 1);
}

TelStatus_t Tel_LogState(TelModuleId_t module, uint8_t event_id, TelLevel_t level, 
                         uint8_t old_state, uint8_t new_state) {
    uint8_t data[2] = {old_state, new_state};
    return Tel_LogEvent(module, event_id, level, data, 2);
}

TelStatus_t Tel_LogMetric(TelModuleId_t module, uint8_t event_id, TelLevel_t level, uint32_t value) {
    uint8_t data[4] = {
        (value >> 24) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 8) & 0xFF,
        value & 0xFF
    };
    return Tel_LogEvent(module, event_id, level, data, 4);
}

TelStatus_t Tel_ReadEvents(uint8_t *data, uint16_t max_len, uint16_t *actual_len) {
    if (!data || !actual_len) {
        return TEL_ERROR_NULL_PTR;
    }
    
    *actual_len = 0;
    
    if (!s_tel_state.initialized) {
        return TEL_ERROR_NOT_INITIALIZED;
    }
    
    uint16_t total_read = 0;
    uint8_t event_data[TEL_MAX_EVENT_SIZE];
    uint8_t event_len;
    
    while (total_read < max_len - TEL_MAX_EVENT_SIZE) {
        bool has_more = Tel_RB_Read(&s_tel_state.ring_buffer, event_data, 
                                    TEL_MAX_EVENT_SIZE, &event_len);
        if (!has_more || event_len == 0) {
            break;
        }
        
        memcpy(data + total_read, event_data, event_len);
        total_read += event_len;
    }
    
    *actual_len = total_read;
    return TEL_OK;
}

const TelStats_t* Tel_GetStats(void) {
    if (!s_tel_state.initialized) {
        return NULL;
    }
    
    s_tel_state.stats.current_usage = Tel_RB_GetUsed(&s_tel_state.ring_buffer);
    return &s_tel_state.stats;
}

void Tel_ClearBuffer(void) {
    if (!s_tel_state.initialized) {
        return;
    }
    
    s_tel_state.ring_buffer.write_idx = 0;
    s_tel_state.ring_buffer.read_idx = 0;
}

void Tel_SetGlobalLevel(TelLevel_t level) {
    s_tel_state.global_level = level;
}

void Tel_SetModuleEnabled(TelModuleId_t module, bool enabled) {
    if (module < TEL_MOD_COUNT) {
        s_module_config[module].enabled = enabled;
    }
}

uint16_t Tel_CompressRLE(const uint8_t *input, uint16_t input_len, 
                         uint8_t *output, uint16_t output_max) {
    if (!input || !output || input_len == 0) {
        return 0;
    }
    
    uint16_t in = 0, out = 0;
    
    while (in < input_len && out < output_max - 2) {
        uint8_t byte = input[in];
        uint8_t count = 1;
        
        while (in + count < input_len && 
               input[in + count] == byte && 
               count < 255) {
            count++;
        }
        
        if (count > 2) {
            if (out + 3 > output_max) break;
            output[out++] = 0x00;
            output[out++] = count;
            output[out++] = byte;
        } else {
            for (uint8_t i = 0; i < count && out < output_max; i++) {
                output[out++] = byte;
            }
        }
        
        in += count;
    }
    
    return out;
}
