/**
 * @file telemetry_diag.h
 * @brief 诊断集成 - 通过UDS读取埋点数据
 */

#ifndef TELEMETRY_DIAG_H
#define TELEMETRY_DIAG_H

#include "telemetry.h"
#include "Std_Types.h"  /* Std_ReturnType/E_OK/E_NOT_OK */

/* DID定义 - 诊断数据标识符 */
#define DID_TEL_STATUS              0xF400  /* 埋点状态 */
#define DID_TEL_STATS               0xF401  /* 统计信息 */
#define DID_TEL_CONFIG              0xF402  /* 配置参数 */
#define DID_TEL_BUFFER_USAGE        0xF403  /* 缓冲区使用率 */
#define DID_TEL_OVERFLOW_COUNT      0xF404  /* 溢出计数 */
#define DID_TEL_EVENT_COUNT         0xF405  /* 事件总数 */
#define DID_TEL_READ_EVENTS         0xF406  /* 读取事件数据 */

/* 控制DID */
#define DID_TEL_CONTROL             0xF410  /* 启用/禁用埋点 */
#define DID_TEL_CLEAR_BUFFER        0xF411  /* 清空缓冲区 */
#define DID_TEL_SET_LEVEL           0xF412  /* 设置日志级别 */

/* DID数据结构 */
typedef struct __attribute__((packed)) {
    uint8_t  enabled;           /* 埋点启用状态 */
    uint8_t  current_level;     /* 当前级别 */
    uint16_t buffer_size;       /* 缓冲区大小 */
    uint16_t buffer_used;       /* 已使用大小 */
    uint8_t  module_mask;       /* 模块使能位图 */
} TelDiagStatus_t;

typedef struct __attribute__((packed)) {
    uint32_t total_events;      /* 总事件数 */
    uint32_t dropped_events;    /* 丢弃事件数 */
    uint16_t overflow_count;    /* 溢出次数 */
    uint8_t  avg_event_size;    /* 平均事件大小 */
} TelDiagStats_t;

/* API */
Std_ReturnType Tel_Diag_ReadData(uint16_t did, uint8_t *data, uint16_t max_len, uint16_t *actual_len);
Std_ReturnType Tel_Diag_WriteData(uint16_t did, const uint8_t *data, uint16_t len);

/* 初始化 */
void Tel_Diag_Init(void);

#endif /* TELEMETRY_DIAG_H */
