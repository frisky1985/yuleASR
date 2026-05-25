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
 * @file LinSlave_CfgTable.h
 * @brief LinSlave 配置表定义
 * @version 2.1.0
 * @note 支持多Unconditional Frame、Event Triggered Frame、Sporadic Frame和诊断帧配置
 */

#ifndef LINSLAVE_CFGTABLE_H
#define LINSAVE_CFGTABLE_H

#include "Std_Types.h"
#include "LinSlave_Types.h"
#include "LinSlave_Checksum.h"

/* 配置表版本 */
#define LINSLAVE_CFGTABLE_MAJOR_VERSION     2
#define LINSLAVE_CFGTABLE_MINOR_VERSION     1
#define LINSLAVE_CFGTABLE_PATCH_VERSION     0

/* 配置限制 */
#define LINSLAVE_MAX_UNCONDITIONAL_FRAMES   20  /* 最大Unconditional Frame数 */
#define LINSLAVE_MAX_EVENT_FRAMES           4   /* 最大Event Triggered Frame数 */
#define LINSLAVE_MAX_SPORADIC_FRAMES        4   /* 最大Sporadic Frame数 */
#define LINSLAVE_MAX_TP_CHANNELS          2

/* 报文方向定义 */
typedef enum {
    LINSLAVE_DIR_NONE = 0,       /* 无方向 */
    LINSLAVE_DIR_RX,                /* 接收 */
    LINSLAVE_DIR_TX,                /* 发送 */
    LINSLAVE_DIR_RX_TX              /* 双向 */
} LinSlave_DirectionType;

/* 报文类型 */
typedef enum {
    LINSLAVE_MSG_TYPE_UNCONDITIONAL = 0,   /* 无条件报文 */
    LINSLAVE_MSG_TYPE_EVENT,               /* 事件触发报文 */
    LINSLAVE_MSG_TYPE_SPORADIC,          /* 偶发报文 */
    LINSLAVE_MSG_TYPE_DIAGNOSTIC,         /* 诊断报文 */
    LINSLAVE_MSG_TYPE_RESERVED            /* 保留 */
} LinSlave_MsgTypeType;

/* 传输协议类型 */
typedef enum {
    LINSLAVE_TP_NONE = 0,        /* 无传输协议 */
    LINSLAVE_TP_LIN_TP           /* LIN TP (ISO 17987) */
} LinSlave_TPTypeType;

/* 报文状态 */
typedef enum {
    LINSLAVE_FRAME_STATUS_IDLE = 0,         /* 空闲 */
    LINSLAVE_FRAME_STATUS_UPDATED,            /* 已更新 */
    LINSLAVE_FRAME_STATUS_ERROR             /* 错误 */
} LinSlave_FrameStatusType;

/* 回调函数类型定义 */

/* Unconditional Frame 接收回调 */
typedef void (*LinSlave_UnconditionalRxCallbackType)(
    uint8 FrameIndex,           /* 帧索引 */
    const uint8* DataPtr,       /* 数据指针 */
    uint8 Length,               /* 数据长度 */
    void* UserData              /* 用户数据 */
);

/* Unconditional Frame 发送回调 */
typedef void (*LinSlave_UnconditionalTxCallbackType)(
    uint8 FrameIndex,           /* 帧索引 */
    uint8* DataPtr,             /* 数据缓冲区 */
    uint8* LengthPtr,           /* 数据长度 */
    void* UserData              /* 用户数据 */
);

/* 通用数据回调 */
typedef void (*LinSlave_DataCallbackFuncType)(
    uint8 Pid,
    const uint8* DataPtr,
    uint8 Length,
    LinSlave_DirectionType Direction
);

/* 诊断数据回调 */
typedef void (*LinSlave_DiagCallbackFuncType)(
    uint8 Pid,
    const uint8* DataPtr,
    uint8 Length,
    uint8* ResponsePtr,
    uint8* ResponseLengthPtr
);

/* 错误处理回调 */
typedef void (*LinSlave_FrameErrorCallbackFuncType)(
    uint8 Pid,
    LinSlave_ErrorType ErrorCode
);

/* ==========================================
 * Unconditional Frame 配置
 *========================================== */

typedef struct {
    uint8 Pid;                          /* PID (带保护位) */
    uint8 Length;                       /* 数据长度 (0-8) */
    LinSlave_DirectionType Direction;   /* 方向 */
    LinSlave_ChecksumType ChecksumType; /* 校验和类型 */
    
    /* 回调函数 */
    LinSlave_UnconditionalRxCallbackType RxCallback;  /* 接收回调 */
    LinSlave_UnconditionalTxCallbackType TxCallback;  /* 发送回调 */
    LinSlave_FrameErrorCallbackFuncType ErrorCallback; /* 错误回调 */
    
    void* UserData;                     /* 用户数据 */
    
    /* 运行时状态 */
    LinSlave_FrameStatusType Status;    /* 帧状态 */
    uint8 LastData[8];                  /* 最后数据缓存 */
    uint8 UpdateFlag;                   /* 更新标志 */
} LinSlave_UnconditionalFrameConfigType;

/* ==========================================
 * Event Triggered Frame 配置
 * ========================================== */

typedef struct {
    uint8 Pid;                          /* Event Frame PID */
    uint8 AssociatedFrameCount;         /* 关联的Unconditional Frame数量 */
    const uint8* AssociatedPids;        /* 关联的PID列表 */
} LinSlave_EventFrameConfigType;

/* ==========================================
 * Sporadic Frame 配置
 * ========================================== */

typedef struct {
    uint8 Pid;                          /* Sporadic Frame PID */
    uint8 AssociatedFrameCount;
    const uint8* AssociatedPids;
} LinSlave_SporadicFrameConfigType;

/* ==========================================
 * 诊断报文配置
 * ========================================== */

typedef struct {
    uint8 RequestPid;                   /* 诊断请求PID */
    uint8 ResponsePid;                  /* 诊断响应PID */
    LinSlave_DiagCallbackFuncType Callback;
    LinSlave_FrameErrorCallbackFuncType ErrorCallback;
    void* UserData;
} LinSlave_DiagnosticFrameConfigType;

/* ==========================================
 * 统一配置表结构
 * ========================================== */

typedef struct {
    /* 版本信息 */
    uint8 VersionMajor;
    uint8 VersionMinor;
    uint8 VersionPatch;
    
    /* 节点信息 */
    uint8 NodeId;                       /* LIN节点ID */
    uint32 BaudRate;                    /* 波特率 */
    
    /* Unconditional Frames */
    uint8 UnconditionalFrameCount;
    const LinSlave_UnconditionalFrameConfigType* UnconditionalFrames;
    
    /* Event Triggered Frames */
    uint8 EventFrameCount;
    const LinSlave_EventFrameConfigType* EventFrames;
    
    /* Sporadic Frames */
    uint8 SporadicFrameCount;
    const LinSlave_SporadicFrameConfigType* SporadicFrames;
    
    /* Diagnostic Frames */
    const LinSlave_DiagnosticFrameConfigType* DiagnosticFrames;
    boolean UseDiagnostic;              /* 是否使用诊断 */
    
    /* 全局回调 */
    LinSlave_FrameErrorCallbackFuncType GlobalErrorCallback;
    
} LinSlave_ConfigTableType;

/* 全局配置表实例 */
extern const LinSlave_ConfigTableType LinSlave_ConfigTable;

/* ==========================================
 * API函数声明
 * ========================================== */

/* 配置表初始化 */
LinSlave_StatusType LinSlave_CfgTable_Init(const LinSlave_ConfigTableType* ConfigTable);

/* PID查找 */
const LinSlave_UnconditionalFrameConfigType* LinSlave_CfgTable_FindUnconditionalByPid(uint8 Pid);
const LinSlave_EventFrameConfigType* LinSlave_CfgTable_FindEventFrame(uint8 Pid);
const LinSlave_SporadicFrameConfigType* LinSlave_CfgTable_FindSporadicFrame(uint8 Pid);

/* 通过索引获取 */
const LinSlave_UnconditionalFrameConfigType* LinSlave_CfgTable_GetUnconditionalEntry(uint8 Index);

/* 获取数量 */
uint8 LinSlave_CfgTable_GetUnconditionalCount(void);

/* 方向匹配检查 */
boolean LinSlave_CfgTable_IsDirectionMatching(
    const LinSlave_UnconditionalFrameConfigType* Entry,
    LinSlave_DirectionType Direction
);

/* 特定类型查找 */
const LinSlave_EventFrameConfigType* LinSlave_CfgTable_FindEventFrame(uint8 Pid);
const LinSlave_SporadicFrameConfigType* LinSlave_CfgTable_FindSporadicFrame(uint8 Pid);

/* 诊断配置获取 */
const LinSlave_DiagnosticFrameConfigType* LinSlave_CfgTable_GetDiagnosticConfig(void);

/* 帧状态管理 */
LinSlave_FrameStatusType LinSlave_CfgTable_GetFrameStatus(uint8 FrameIndex);
void LinSlave_CfgTable_SetFrameStatus(uint8 FrameIndex, LinSlave_FrameStatusType Status);
void LinSlave_CfgTable_ClearUpdateFlag(uint8 FrameIndex);
uint8 LinSlave_CfgTable_IsFrameUpdated(uint8 FrameIndex);

/* 数据缓存访问 */
const uint8* LinSlave_CfgTable_GetFrameData(uint8 FrameIndex);
void LinSlave_CfgTable_SetFrameData(uint8 FrameIndex, const uint8* DataPtr, uint8 Length);

/* 工具函数 */
uint8 LinSlave_CfgTable_GetPidByIndex(uint8 Index);
uint8 LinSlave_CfgTable_GetIndexByPid(uint8 Pid);
uint8 LinSlave_CfgTable_GetAllUnconditionalPids(uint8* PidList, uint8 MaxCount);
boolean LinSlave_CfgTable_IsUnconditionalFrame(uint8 Pid);
boolean LinSlave_CfgTable_IsEventFrame(uint8 Pid);
boolean LinSlave_CfgTable_IsSporadicFrame(uint8 Pid);
boolean LinSlave_CfgTable_IsDiagnosticFrame(uint8 Pid);

#endif /* LINSLAVE_CFGTABLE_H */
