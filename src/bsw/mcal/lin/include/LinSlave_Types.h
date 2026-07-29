/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file LinSlave_Types.h
 * @brief LinSlave 模块类型定义
 * @version 1.0.0
 */

#ifndef LINSLAVE_TYPES_H
#define LINSLAVE_TYPES_H

#include "Std_Types.h"

/* 版本信息 */
#define LINSLAVE_SW_MAJOR_VERSION       1
#define LINSLAVE_SW_MINOR_VERSION       0
#define LINSLAVE_SW_PATCH_VERSION       0

/* 服务ID */
#define LINSLAVE_INIT_SID               0x00
#define LINSLAVE_DEINIT_SID             0x01
#define LINSLAVE_SETRESPONSE_SID        0x02
#define LINSLAVE_GETSTATE_SID           0x03
#define LINSLAVE_RXINT_SID              0x04
#define LINSLAVE_BREAKDET_SID           0x05

/* 错误码 */
#define LINSLAVE_E_NOT_INITIALIZED      0x10
#define LINSLAVE_E_INVALID_PARAMETER    0x11
#define LINSLAVE_E_NULL_POINTER         0x12
#define LINSLAVE_E_INVALID_LENGTH       0x13
#define LINSLAVE_E_INVALID_STATE        0x14
#define LINSLAVE_E_TIMEOUT              0x15

/* 状态机状态 */
typedef enum {
    LINSLAVE_STATE_UNINIT = 0,      /* 未初始化 */
    LINSLAVE_STATE_IDLE,            /* 空闲状态 */
    LINSLAVE_STATE_RX_BREAK,        /* 接收 Break */
    LINSLAVE_STATE_RX_SYNC,         /* 接收 Sync */
    LINSLAVE_STATE_RX_PID,          /* 接收 PID */
    LINSLAVE_STATE_RX_DATA,         /* 接收数据 */
    LINSLAVE_STATE_RX_CSUM,         /* 接收校验和 */
    LINSLAVE_STATE_TX_RESPONSE      /* 发送响应 */
} LinSlave_StateType;

/* 错误类型 */
typedef enum {
    LINSLAVE_ERROR_NONE = 0,        /* 无错误 */
    LINSLAVE_ERROR_BREAK,           /* Break 检测错误 */
    LINSLAVE_ERROR_SYNC,            /* 同步字节错误 */
    LINSLAVE_ERROR_PID,             /* PID 校验错误 */
    LINSLAVE_ERROR_CHECKSUM,        /* 校验和错误 */
    LINSLAVE_ERROR_TIMEOUT,         /* 报文超时 */
    LINSLAVE_ERROR_FRAMING,         /* 帧格式错误 */
    LINSLAVE_ERROR_OVERRUN          /* 接收溢出 */
} LinSlave_ErrorType;

/* 操作状态 */
typedef enum {
    LINSLAVE_OK = 0,                /* 成功 */
    LINSLAVE_NOT_OK,                /* 失败 */
    LINSLAVE_BUSY,                  /* 忙碌 */
    LINSLAVE_TIMEOUT                /* 超时 */
} LinSlave_StatusType;

/* 配置结构体 */
typedef struct {
    uint8 NodeId;                   /* 从机节点 ID (0-59) */
    uint8 BaudRate;                 /* 波特率: 0=9600, 1=19200 */
    uint8 ResponseLength;           /* 响应数据长度 (1-8) */
    uint8 ChecksumType;             /* 校验和类型: 0=经典, 1=增强 */
    uint16 BreakThresholdUs;        /* Break 检测阈值 (微秒) */
    uint16 TimeoutMs;               /* 报文超时时间 (毫秒) */
} LinSlave_ConfigType;

/* 回调函数类型 */
typedef void (*LinSlave_RxCallbackFuncType)(
    uint8 Pid,                      /* 接收到的PID */
    uint8* ResponseDataPtr,         /* 响应数据缓冲区指针 (输出) */
    uint8* ResponseLengthPtr,       /* 响应数据长度 (输出) */
    uint8* ChecksumTypePtr          /* 校验和类型 (输出) */
);

typedef void (*LinSlave_ErrorCallbackFuncType)(
    LinSlave_ErrorType ErrorCode,   /* 错误码 */
    uint8 Pid                       /* 相关PID (如果有) */
);

#endif /* LINSLAVE_TYPES_H */
