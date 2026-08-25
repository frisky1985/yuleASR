/**
 * @file telemetry_dds.c
 * @brief DDS集成实现
 */
/* @req SHALL_TELEMETRY */


#include "telemetry_dds.h"
#include <string.h>

static TelDdsConfig_t s_dds_config = {
    .enabled = true,
    .publish_interval_ms = TEL_DDS_PUBLISH_INTERVAL_MS,
    .min_events = 10,
    .compression_enabled = true
};

static TelDdsSample_t s_dds_sample;
static uint32_t s_seq_num = 0;
static uint32_t s_last_publish_time = 0;

/* 外部DDS API声明 (实际由DDS层提供) */
__attribute__((weak)) bool DDS_PublishTelemetry(const TelDdsSample_t *sample) {
    /* 桩函数 - 实际由DDS层实现 */
    (void)sample;
    return true;
}

__attribute__((weak)) uint32_t DDS_GetTimestamp(void) {
    return 0;
}

TelStatus_t Tel_Dds_Init(const TelDdsConfig_t *config) {
    if (config) {
        s_dds_config = *config;
    }
    
    memset(&s_dds_sample, 0, sizeof(s_dds_sample));
    s_seq_num = 0;
    
    return TEL_OK;
}

void Tel_Dds_Deinit(void) {
    s_dds_config.enabled = false;
}

void Tel_Dds_SetEnabled(bool enabled) {
    s_dds_config.enabled = enabled;
}

uint16_t Tel_Dds_PackEvents(uint8_t *output, uint16_t max_len) {
    if (!output || (max_len == 0U)) {
        return 0;
    }
    
    uint16_t total_events = 0;
    uint16_t total_bytes = 0;
    
    /* 读取事件并打包 */
    while (total_bytes < (max_len - TEL_MAX_EVENT_SIZE)) {
        uint16_t actual_len = 0;
        TelStatus_t status = Tel_ReadEvents(output + total_bytes, 
                                            max_len - total_bytes, 
                                            &actual_len);
        
        if ((status != TEL_OK) || (actual_len == 0U)) {
            break;
        }
        
        total_bytes += actual_len;
        total_events++;
        
        /* 限制单次发送事件数 */
        if (total_events >= 100U) {
            break;
        }
    }
    
    return total_bytes;
}

TelStatus_t Tel_Dds_FlushBuffer(void) {
    if (!s_dds_config.enabled) {
        return TEL_OK;
    }
    
    /* 准备样本 */
    s_dds_sample.seq_num = s_seq_num;
    s_seq_num++;
    s_dds_sample.timestamp = DDS_GetTimestamp();
    
    /* 打包事件 */
    uint16_t payload_len = Tel_Dds_PackEvents(s_dds_sample.payload, 
                                               TEL_DDS_MAX_PAYLOAD);
    
    if (payload_len == 0U) {
        return TEL_OK; /* 没有数据需要发送 */
    }
    
    /* 压缩 (如果启用) */
    if (s_dds_config.compression_enabled && (payload_len > 100U)) {
        uint8_t compressed[TEL_DDS_MAX_PAYLOAD];
        uint16_t compressed_len = Tel_CompressRLE(s_dds_sample.payload, 
                                                   payload_len,
                                                   compressed, 
                                                   TEL_DDS_MAX_PAYLOAD);
        
        if (compressed_len < payload_len) {
            memcpy(s_dds_sample.payload, compressed, compressed_len);
            payload_len = compressed_len;
            s_dds_sample.compression = 1; /* RLE */
        } else {
            s_dds_sample.compression = 0; /* 无压缩 */
        }
    } else {
        s_dds_sample.compression = 0;
    }
    
    s_dds_sample.event_count = payload_len; /* 实际字节数 */
    
    /* 发布到DDS */
    if (DDS_PublishTelemetry(&s_dds_sample)) {
        s_last_publish_time = DDS_GetTimestamp();
        return TEL_OK;
    } else {
        return TEL_ERROR_NULL_PTR; /* 发布失败 */
    }
}

TelStatus_t Tel_Dds_Publish(void) {
    return Tel_Dds_FlushBuffer();
}

const TelDdsSample_t* Tel_Dds_GetLastSample(void) {
    return &s_dds_sample;
}

/* 定时任务 - 应由Gpt或SchM调用 */
void Tel_Dds_CyclicTask(void) {
    if (!s_dds_config.enabled) {
        return;
    }
    
    uint32_t now = DDS_GetTimestamp();
    uint32_t elapsed = now - s_last_publish_time;
    
    /* 检查是否到达发送间隔 */
    if (elapsed >= s_dds_config.publish_interval_ms) {
        Tel_Dds_FlushBuffer();
    }
}