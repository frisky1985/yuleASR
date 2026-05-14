/**
 * @file LinSlave_Cfg.h
 * @brief LinSlave 模块配置参数
 * @version 1.0.0
 */

#ifndef LINSLAVE_CFG_H
#define LINSLAVE_CFG_H

#include "Std_Types.h"

/* 版本检查 */
#define LINSLAVE_DEV_ERROR_DETECT       STD_ON
#define LINSLAVE_VERSION_INFO_API       STD_ON

/* 节点配置 */
#define LINSLAVE_NODE_ID                5               /* 从机节点ID: 0-59 */
#define LINSLAVE_BAUDRATE               1U              /* 0=9600, 1=19200 */
#define LINSLAVE_RESPONSE_LENGTH        8U              /* 响应数据长度 */
#define LINSLAVE_CHECKSUM_TYPE          1U              /* 0=经典, 1=增强 */

/* 缓冲区大小 */
#define LINSLAVE_MAX_DATA_LENGTH        8U
#define LINSLAVE_MAX_FRAME_LENGTH       12U             /* 最大报文长度 */

/* 时间参数 */
#define LINSLAVE_BREAK_THRESHOLD_US     1000U           /* Break 检测阈值 (微秒) */
#define LINSLAVE_FRAME_TIMEOUT_MS       100U            /* 报文超时 (毫秒) */
#define LINSLAVE_INTERBYTE_TIMEOUT_US   1000U           /* 字节间超时 (微秒) */

/* 外部中断处理函数名 - 由应用层实现 */
#define LINSLAVE_UART_IRQ_HANDLER       LinSlave_RxInterruptHandler

/* 默认配置 */
extern const LinSlave_ConfigType LinSlave_DefaultConfig;

#endif /* LINSLAVE_CFG_H */
