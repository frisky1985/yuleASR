/**
 * @file telemetry.h
 * @brief 嵌入式埋点系统核心头文件
 * 
 * 资源优化设计:
 * - RAM使用: < 4KB (可配置)
 * - 零动态分配
 * - 非阻塞写入
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 版本信息 */
#define TEL_VERSION_MAJOR   1
#define TEL_VERSION_MINOR   0
#define TEL_VERSION_PATCH   0

/* 配置宏 - 可在telemetry_cfg.h中覆盖 */
#ifndef TEL_BUFFER_SIZE
    #define TEL_BUFFER_SIZE     2048    /* 默认2KB */
#endif

#ifndef TEL_MAX_EVENT_SIZE
    #define TEL_MAX_EVENT_SIZE  16      /* 最大事件大小 */
#endif

#ifndef TEL_ENABLE_TIMESTAMP
    #define TEL_ENABLE_TIMESTAMP 1
#endif

/* 模块使能配置 */
#ifndef TEL_ENABLE_MODULE_SYS
    #define TEL_ENABLE_MODULE_SYS   1
#endif
#ifndef TEL_ENABLE_MODULE_DDS
    #define TEL_ENABLE_MODULE_DDS   1
#endif
#ifndef TEL_ENABLE_MODULE_ETH
    #define TEL_ENABLE_MODULE_ETH   1
#endif
#ifndef TEL_ENABLE_MODULE_DIAG
    #define TEL_ENABLE_MODULE_DIAG  0
#endif

/* 错误码 */
typedef enum {
    TEL_OK = 0,
    TEL_ERROR_NULL_PTR,
    TEL_ERROR_BUFFER_FULL,
    TEL_ERROR_INVALID_PARAM,
    TEL_ERROR_NOT_INITIALIZED,
    TEL_ERROR_OVERFLOW
} TelStatus_t;

/* 模块ID */
typedef enum {
    TEL_MOD_SYS = 0,    /* 系统 */
    TEL_MOD_ECUM,       /* EcuM */
    TEL_MOD_BSWM,       /* BswM */
    TEL_MOD_DDS,        /* DDS */
    TEL_MOD_ETH,        /* 以太网 */
    TEL_MOD_SECOC,      /* 安全 */
    TEL_MOD_DIAG,       /* 诊断 */
    TEL_MOD_OTA,        /* OTA */
    TEL_MOD_USER,       /* 用户 */
    TEL_MOD_COUNT
} TelModuleId_t;

/* 事件级别 */
typedef enum {
    TEL_LEVEL_NONE = 0,     /* 禁用 */
    TEL_LEVEL_CRITICAL,     /* 关键故障 */
    TEL_LEVEL_ERROR,        /* 错误 */
    TEL_LEVEL_WARNING,      /* 警告 */
    TEL_LEVEL_INFO,         /* 信息 */
    TEL_LEVEL_DEBUG,        /* 调试 */
    TEL_LEVEL_VERBOSE       /* 详细 */
} TelLevel_t;

/* 事件头 - 2 bytes */
typedef struct __attribute__((packed)) {
    uint8_t  event_id;      /* 事件ID (0-255) */
    uint8_t  ts_lo;         /* 时间戳低8位 */
} TelEventHeader_t;

/* 扩展头 - 2 bytes (可选) */
typedef struct __attribute__((packed)) {
    uint8_t  ts_hi;         /* 时间戳高8位 */
    uint8_t  data_len : 4;  /* 数据长度 */
    uint8_t  flags    : 4;  /* 标志位 */
} TelEventExtHeader_t;

/* 事件结构 - 最大16 bytes */
typedef struct __attribute__((packed)) {
    TelEventHeader_t header;
    uint8_t data[TEL_MAX_EVENT_SIZE - sizeof(TelEventHeader_t)];
} TelEvent_t;

/* 环形缓冲区 */
typedef struct {
    volatile uint16_t write_idx;    /* 写入索引 */
    volatile uint16_t read_idx;     /* 读取索引 */
    uint16_t capacity;              /* 容量 */
    uint16_t overflow_cnt;          /* 溢出计数 */
    uint8_t *buffer;                /* 缓冲区指针 */
} TelRingBuffer_t;

/* 统计信息 */
typedef struct {
    uint32_t total_events;          /* 总事件数 */
    uint32_t dropped_events;        /* 丢弃事件数 */
    uint32_t last_overflow_time;    /* 上次溢出时间 */
    uint16_t current_usage;         /* 当前使用量 */
    uint8_t  avg_event_size;        /* 平均事件大小 */
} TelStats_t;

/* 全局状态 */
typedef struct {
    bool initialized;
    bool enabled;
    TelLevel_t global_level;
    TelRingBuffer_t ring_buffer;
    TelStats_t stats;
    uint32_t base_timestamp;
    uint16_t last_relative_ts;
} TelState_t;

/* 外部API */

/**
 * @brief 初始化埋点系统
 * @param buffer 外部分配的缓冲区
 * @param size 缓冲区大小
 * @return 状态码
 */
TelStatus_t Tel_Init(uint8_t *buffer, uint16_t size);

/**
 * @brief 反初始化
 */
void Tel_Deinit(void);

/**
 * @brief 记录事件
 * @param module 模块ID
 * @param event_id 事件ID
 * @param level 事件级别
 * @param data 数据指针
 * @param len 数据长度
 * @return 状态码
 */
TelStatus_t Tel_LogEvent(TelModuleId_t module, uint8_t event_id, 
                         TelLevel_t level, const uint8_t *data, uint8_t len);

/**
 * @brief 记录瞬时事件 (无payload)
 */
static inline TelStatus_t Tel_LogInstant(TelModuleId_t module, uint8_t event_id, TelLevel_t level) {
    return Tel_LogEvent(module, event_id, level, NULL, 0);
}

/**
 * @brief 记录计数器事件 (1 byte value)
 */
TelStatus_t Tel_LogCounter(TelModuleId_t module, uint8_t event_id, TelLevel_t level, uint8_t value);

/**
 * @brief 记录状态变更 (old + new state)
 */
TelStatus_t Tel_LogState(TelModuleId_t module, uint8_t event_id, TelLevel_t level, 
                         uint8_t old_state, uint8_t new_state);

/**
 * @brief 记录度量值 (32-bit)
 */
TelStatus_t Tel_LogMetric(TelModuleId_t module, uint8_t event_id, TelLevel_t level, uint32_t value);

/**
 * @brief 读取事件数据
 * @param data 输出缓冲区
 * @param max_len 最大长度
 * @param actual_len 实际长度输出
 * @return 状态码
 */
TelStatus_t Tel_ReadEvents(uint8_t *data, uint16_t max_len, uint16_t *actual_len);

/**
 * @brief 获取统计信息
 */
const TelStats_t* Tel_GetStats(void);

/**
 * @brief 清空缓冲区
 */
void Tel_ClearBuffer(void);

/**
 * @brief 设置全局级别
 */
void Tel_SetGlobalLevel(TelLevel_t level);

/**
 * @brief 启用/禁用模块
 */
void Tel_SetModuleEnabled(TelModuleId_t module, bool enabled);

/* 内部函数 - 不推荐直接调用 */
TelStatus_t Tel_RB_Init(TelRingBuffer_t *rb, uint8_t *buffer, uint16_t size);
bool Tel_RB_Write(TelRingBuffer_t *rb, const uint8_t *data, uint8_t len);
bool Tel_RB_Read(TelRingBuffer_t *rb, uint8_t *data, uint8_t max_len, uint8_t *actual_len);
uint16_t Tel_RB_GetUsed(const TelRingBuffer_t *rb);
uint16_t Tel_RB_GetFree(const TelRingBuffer_t *rb);

/* 工具函数 */
uint16_t Tel_CompressRLE(const uint8_t *input, uint16_t input_len, uint8_t *output, uint16_t output_max);
uint32_t Tel_GetTimestamp(void);

#endif /* TELEMETRY_H */
