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
 * @file LinSlave_Tp.h
 * @brief LIN TP (传输协议) 头文件 - ISO 17987
 * @version 1.0.0
 */

#ifndef LINSLAVE_TP_H
#define LINSLAVE_TP_H

#include "Std_Types.h"
#include "LinSlave_Types.h"

/* TP 版本 */
#define LINSLAVE_TP_MAJOR_VERSION       1
#define LINSLAVE_TP_MINOR_VERSION       0
#define LINSLAVE_TP_PATCH_VERSION       0

/* 协议参数 */
#define LINSLAVE_TP_MAX_PDUs            2
#define LINSLAVE_TP_MAX_FRAME_LEN       4095    /* 最大报文长度 */
#define LINSLAVE_TP_MAX_SEG_LEN         6       /* 每帧最大数据 (6字节) */
#define LINSLAVE_TP_BS_DEFAULT          5       /* 默认BlockSize */
#define LINSLAVE_TP_STMIN_DEFAULT       10      /* 默认STmin (ms) */

/* PCI (协议控制信息) 类型 */
typedef enum {
    LINSLAVE_TP_PCI_SF = 0x00,  /* 单帧 (0x00-0x07) */
    LINSLAVE_TP_PCI_FF = 0x10,  /* 首帧 (0x10-0x1F) */
    LINSLAVE_TP_PCI_CF = 0x20,  /* 连续帧 (0x20-0x2F) */
    LINSLAVE_TP_PCI_FC = 0x30   /* 流控帧 (0x30-0x3F) */
} LinSlave_Tp_PciType;

/* TP 状态 */
typedef enum {
    LINSLAVE_TP_STATE_IDLE = 0,
    LINSLAVE_TP_STATE_RX_FF,        /* 接收首帧 */
    LINSLAVE_TP_STATE_RX_CF,        /* 接收连续帧 */
    LINSLAVE_TP_STATE_TX_FF,        /* 发送首帧 */
    LINSLAVE_TP_STATE_TX_CF,        /* 发送连续帧 */
    LINSLAVE_TP_STATE_WAIT_FC,      /* 等待流控 */
    LINSLAVE_TP_STATE_ERROR         /* 错误状态 */
} LinSlave_Tp_StateType;

/* TP 返回状态 */
typedef enum {
    LINSLAVE_TP_OK = 0,
    LINSLAVE_TP_E_NOT_OK,
    LINSLAVE_TP_E_BUSY,
    LINSLAVE_TP_E_TIMEOUT,
    LINSLAVE_TP_E_OVERSIZE,
    LINSLAVE_TP_E_INVALID_PCI
} LinSlave_Tp_StatusType;

/* 流控参数 */
typedef struct {
    uint8 BlockSize;        /* BS: 连续帧数量 */
    uint8 STmin;            /* STmin: 最小分隔时间 (ms) */
} LinSlave_Tp_FcParamsType;

/* TP 通道配置 */
typedef struct {
    uint8 ChannelId;                                    /* 通道ID */
    uint8 RxPduId;                                      /* 接收PDU ID */
    uint8 TxPduId;                                      /* 发送PDU ID */
    uint8 Nas;                                          /* 发送超时 (ms) */
    uint8 Ncr;                                          /* 接收超时 (ms) */
    LinSlave_Tp_FcParamsType DefaultFc;                 /* 默认流控参数 */
} LinSlave_Tp_ChannelConfigType;

/* TP 通道运行时数据 */
typedef struct {
    LinSlave_Tp_StateType State;                        /* 当前状态 */
    uint8 ChannelId;                                    /* 所属通道 */
    
    /* 接收数据 */
    uint8 RxBuffer[LINSLAVE_TP_MAX_FRAME_LEN];          /* 接收缓冲区 */
    uint16 RxLength;                                    /* 已接收长度 */
    uint16 RxTotalLength;                               /* 总长度 */
    uint8 RxSN;                                         /* 序列号 */
    
    /* 发送数据 */
    const uint8* TxBuffer;                              /* 发送缓冲区 */
    uint16 TxLength;                                    /* 总长度 */
    uint16 TxOffset;                                    /* 已发送偏移 */
    uint8 TxSN;                                         /* 序列号 */
    uint8 BS_Counter;                                   /* 当前Block计数 */
    
    /* 流控 */
    LinSlave_Tp_FcParamsType FcParams;                  /* 流控参数 */
    uint32 Timer;                                       /* 超时计时器 */
} LinSlave_Tp_ChannelType;

/* 回调函数类型 */
/* TP 接收完成回调 */
typedef void (*LinSlave_Tp_RxIndicationFuncType)(
    uint8 ChannelId,
    const uint8* DataPtr,
    uint16 Length
);

/* TP 发送完成回调 */
typedef void (*LinSlave_Tp_TxConfirmationFuncType)(
    uint8 ChannelId,
    LinSlave_Tp_StatusType Status
);

/* ==================== API函数声明 ==================== */

/**
 * @brief 初始化LIN TP模块
 * @return 初始化状态
 */
LinSlave_Tp_StatusType LinSlave_Tp_Init(void);

/**
 * @brief 反初始化LIN TP模块
 */
void LinSlave_Tp_DeInit(void);

/**
 * @brief 发送TP数据
 * @param ChannelId - 通道ID
 * @param DataPtr - 数据指针
 * @param Length - 数据长度
 * @return 操作状态
 */
LinSlave_Tp_StatusType LinSlave_Tp_Transmit(
    uint8 ChannelId,
    const uint8* DataPtr,
    uint16 Length
);

/**
 * @brief 处理接收到的PCI和数据
 * @param Pid - 当前PID
 * @param Pci - PCI字节
 * @param DataPtr - 数据指针
 * @param Length - 数据长度
 * @return 处理状态
 * @note 由LinSlave状态机调用
 */
LinSlave_Tp_StatusType LinSlave_Tp_ProcessFrame(
    uint8 Pid,
    uint8 Pci,
    const uint8* DataPtr,
    uint8 Length
);

/**
 * @brief 主函数 - 处理超时和状态转换
 * @note 应在定时器中周期调用
 */
void LinSlave_Tp_MainFunction(void);

/**
 * @brief 注册接收完成回调
 * @param Callback - 回调函数指针
 */
void LinSlave_Tp_RegisterRxIndication(LinSlave_Tp_RxIndicationFuncType Callback);

/**
 * @brief 注册发送完成回调
 * @param Callback - 回调函数指针
 */
void LinSlave_Tp_RegisterTxConfirmation(LinSlave_Tp_TxConfirmationFuncType Callback);

/**
 * @brief 获取通道当前状态
 * @param ChannelId - 通道ID
 * @return 通道状态
 */
LinSlave_Tp_StateType LinSlave_Tp_GetState(uint8 ChannelId);

/**
 * @brief 检查TP通道是否忙碌
 * @param ChannelId - 通道ID
 * @return TRUE=忙碌, FALSE=空闲
 */
boolean LinSlave_Tp_IsBusy(uint8 ChannelId);

/**
 * @brief 取消当前传输
 * @param ChannelId - 通道ID
 */
void LinSlave_Tp_Cancel(uint8 ChannelId);

#endif /* LINSLAVE_TP_H */
