/**
 * @file telemetry_tools.h
 * @brief 诊断工具 - 数据读取和可视化支持
 */

#ifndef TELEMETRY_TOOLS_H
#define TELEMETRY_TOOLS_H

#include "telemetry.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 事件解析回调 */
typedef void (*TelEventCallback_t)(const TelEntry_t *entry, void *user_data);

/* 文件导出格式 */
typedef enum {
    TEL_FMT_BINARY,     /* 原始二进制 */
    TEL_FMT_CSV,        /* CSV格式 */
    TEL_FMT_JSON,       /* JSON格式 */
    TEL_FMT_PCAP        /* 以太网帧格式 (用于Wireshark) */
} TelExportFmt_t;

/* 过滤条件 */
typedef struct {
    uint8_t  module_mask;       /* 模块掩码 (0=不过滤) */
    uint8_t  min_level;         /* 最小级别 */
    uint32_t start_time;        /* 开始时间戳 (0=不过滤) */
    uint32_t end_time;          /* 结束时间戳 (0=不过滤) */
    uint8_t  event_type;        /* 事件类型 (0xFF=不过滤) */
} TelFilter_t;

/* API函数 */

/**
 * @brief 解析事件缓冲区
 * @param buffer 原始事件数据
 * @param len 数据长度
 * @param callback 解析回调函数
 * @param user_data 用户数据
 */
void Tel_ParseEvents(const uint8_t *buffer, uint16_t len, 
                     TelEventCallback_t callback, void *user_data);

/**
 * @brief 导出事件到文件
 * @param filename 输出文件名
 * @param fmt 导出格式
 * @param filter 过滤条件 (NULL=不过滤)
 */
TelStatus_t Tel_ExportToFile(const char *filename, TelExportFmt_t fmt, 
                              const TelFilter_t *filter);

/**
 * @brief 获取事件字符串描述
 * @param module 模块ID
 * @param event_id 事件ID
 * @return 事件描述字符串
 */
const char* Tel_GetEventName(uint8_t module, uint8_t event_id);

/**
 * @brief 获取模块名称
 * @param module 模块ID
 * @return 模块名称字符串
 */
const char* Tel_GetModuleName(uint8_t module);

/**
 * @brief 格式化事件为字符串
 * @param entry 事件条目
 * @param buffer 输出缓冲区
 * @param max_len 缓冲区大小
 * @return 实际写入长度
 */
int Tel_FormatEvent(const TelEntry_t *entry, char *buffer, size_t max_len);

/**
 * @brief 打印事件到标准输出
 * @param entry 事件条目
 */
void Tel_PrintEvent(const TelEntry_t *entry);

/**
 * @brief 统计事件信息
 * @param buffer 事件数据
 * @param len 数据长度
 * @param stats 输出统计信息
 */
void Tel_AnalyzeEvents(const uint8_t *buffer, uint16_t len, TelStats_t *stats);

/* PC端工具函数 (仅在PC编译时使用) */
#ifndef EMBEDDED_TARGET

/**
 * @brief 从文件读取事件数据
 * @param filename 输入文件名
 * @param buffer 输出缓冲区
 * @param max_len 缓冲区大小
 * @return 实际读取字节数
 */
uint16_t Tel_ReadFromFile(const char *filename, uint8_t *buffer, uint16_t max_len);

/**
 * @brief 生成Wireshark可读的PCAP文件
 * @param input 输入二进制文件
 * @param output 输出PCAP文件
 */
TelStatus_t Tel_ConvertToPcap(const char *input, const char *output);

/**
 * @brief 实时监控模式
 * @param device 串口设备路径
 * @param baud 波特率
 */
TelStatus_t Tel_RealtimeMonitor(const char *device, uint32_t baud);

#endif /* EMBEDDED_TARGET */

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_TOOLS_H */
