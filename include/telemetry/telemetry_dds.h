/**
 * @file telemetry_dds.h
 * @brief DDS集成 - 埋点数据发布
 */

#ifndef TELEMETRY_DDS_H
#define TELEMETRY_DDS_H

#include "telemetry.h"

/* DDS Topic 定义 */
#define TEL_DDS_TOPIC_NAME      "TelemetryData"
#define TEL_DDS_TOPIC_TYPE      "TelDdsSample"

/* 样本结构 */
typedef struct __attribute__((packed)) {
    uint32_t seq_num;           /* 序列号 */
    uint32_t timestamp;         /* 时间戳 */
    uint16_t event_count;       /* 事件数量 */
    uint8_t  compression;       /* 压缩算法: 0=无, 1=RLE */
    uint8_t  reserved;
    uint8_t  payload[1400];     /* 数据payload, MTU优化 */
} TelDdsSample_t;

#define TEL_DDS_MAX_PAYLOAD     1400
#define TEL_DDS_PUBLISH_INTERVAL_MS 100  /* 默认100ms发送间隔 */

/* DDS发布者配置 */
typedef struct {
    bool enabled;
    uint32_t publish_interval_ms;
    uint16_t min_events;        /* 最小事件数才触发发送 */
    bool compression_enabled;
} TelDdsConfig_t;

/* API */
TelStatus_t Tel_Dds_Init(const TelDdsConfig_t *config);
void Tel_Dds_Deinit(void);
TelStatus_t Tel_Dds_Publish(void);  /* 手动触发发送 */
void Tel_Dds_SetEnabled(bool enabled);
const TelDdsSample_t* Tel_Dds_GetLastSample(void);

/* 内部函数 */
TelStatus_t Tel_Dds_FlushBuffer(void);
uint16_t Tel_Dds_PackEvents(uint8_t *output, uint16_t max_len);

#endif /* TELEMETRY_DDS_H */
