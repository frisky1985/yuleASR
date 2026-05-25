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
 * @file LinMaster_Tp.h
 * @brief LIN Master TP (传输协议) 客户端头文件 - ISO 17987
 * @version 1.0.0
 */

#ifndef LINMASTER_TP_H
#define LINMASTER_TP_H

#include "Std_Types.h"
#include <stdint.h>

/* TP 版本 */
#define LINMASTER_TP_MAJOR_VERSION      1
#define LINMASTER_TP_MINOR_VERSION      0
#define LINMASTER_TP_PATCH_VERSION      0

/* 协议参数 */
#define LINMASTER_TP_MAX_FRAME_LEN      4095    /* 最大报文长度 */
#define LINMASTER_TP_MAX_SEG_LEN        6       /* 每帧最大数据 (6字节) */
#define LINMASTER_TP_BS_DEFAULT         5       /* 默认BlockSize */
#define LINMASTER_TP_STMIN_DEFAULT      10      /* 默认STmin (ms) */

/* 超时参数 (ms) */
#define LINMASTER_TP_N_AS_TIMEOUT       1000    /* 发送超时 */
#define LINMASTER_TP_N_BS_TIMEOUT       1000    /* 等待流控超时 */
#define LINMASTER_TP_N_CS_TIMEOUT       1000    /* 连续帧发送超时 */

/* PCI (协议控制信息) 类型 */
typedef enum {
    LINMASTER_TP_PCI_SF = 0x00,  /* 单帧 (0x00-0x07) */
    LINMASTER_TP_PCI_FF = 0x10,  /* 首帧 (0x10-0x1F) */
    LINMASTER_TP_PCI_CF = 0x20,  /* 连续帧 (0x20-0x2F) */
    LINMASTER_TP_PCI_FC = 0x30   /* 流控帧 (0x30-0x3F) */
} LinMaster_Tp_PciType;

/* TP 状态 */
typedef enum {
    LINMASTER_TP_STATE_IDLE = 0,        /* 空闲 */
    LINMASTER_TP_STATE_TX_SF,           /* 发送单帧 */
    LINMASTER_TP_STATE_TX_FF,           /* 发送首帧 */
    LINMASTER_TP_STATE_WAIT_FC,         /* 等待流控 */
    LINMASTER_TP_STATE_TX_CF,           /* 发送连续帧 */
    LINMASTER_TP_STATE_ERROR            /* 错误状态 */
} LinMaster_Tp_StateType;

/* TP 返回状态 */
typedef enum {
    LINMASTER_TP_OK = 0,
    LINMASTER_TP_E_NOT_OK,
    LINMASTER_TP_E_BUSY,
    LINMASTER_TP_E_TIMEOUT,
    LINMASTER_TP_E_OVERSIZE,
    LINMASTER_TP_E_INVALID_PCI,
    LINMASTER_TP_E_INVALID_PARAM
} LinMaster_Tp_StatusType;

/* TP 通道运行时数据 */
typedef struct {
    uint8 ChannelId;
    LinMaster_Tp_StateType State;
    
    /* 发送数据 */
    uint8 TxBuffer[LINMASTER_TP_MAX_FRAME_LEN];
    uint16 TxLength;
    uint16 TxOffset;
    
    /* 接收数据 */
    uint8 RxBuffer[LINMASTER_TP_MAX_FRAME_LEN];
    uint16 RxLength;
    uint16 RxOffset;
    
    /* 流控参数 */
    uint8 BlockSize;
    uint8 STmin;
    uint8 SeqNumber;
    uint8 BS_Counter;
    
    /* 计时器 */
    uint32 LastFrameTime;
    uint32 Timer;
    
    /* 发送等待标志 */
    boolean TxPending;
    uint8 TxPendingLength;
} LinMaster_Tp_ChannelType;

/* 接收完成回调函数类型 */
typedef void (*LinMaster_Tp_RxCallbackFuncType)(
    uint8 ChannelId,
    const uint8* DataPtr,
    uint16 Length
);

/* 发送确认回调函数类型 */
typedef void (*LinMaster_Tp_TxConfirmFuncType)(
    uint8 ChannelId,
    LinMaster_Tp_StatusType Status
);

/* ==================== API函数声明 ==================== */

/**
 * @brief 初始化LIN Master TP模块
 * @return 初始化状态
 */
LinMaster_Tp_StatusType LinMaster_Tp_Init(void);

/**
 * @brief 反初始化LIN Master TP模块
 */
void LinMaster_Tp_DeInit(void);

/**
 * @brief 发送诊断请求 (TP数据)
 * @param Length - 数据长度
 * @param DataPtr - 数据指针
 * @return 操作状态
 * @note 主函数，发送诊断请求到Slave
 */
LinMaster_Tp_StatusType LinMaster_Tp_Transmit(
    uint16 Length,
    const uint8* DataPtr
);

/**
 * @brief 接收TP响应指示
 * @param DataPtr - 接收数据指针
 * @param Length - 数据长度
 * @return 处理状态
 * @note 由底层接收中断调用，处理FC流控帧
 */
LinMaster_Tp_StatusType LinMaster_Tp_RxIndication(
    uint8* DataPtr,
    uint8 Length
);

/**
 * @brief TP主函数 - 处理状态机和超时
 * @note 应在定时器中周期调用
 */
void LinMaster_Tp_MainFunction(void);

/**
 * @brief 注册接收完成回调
 * @param Callback - 回调函数指针
 */
void LinMaster_Tp_RegisterRxCallback(LinMaster_Tp_RxCallbackFuncType Callback);

/**
 * @brief 注册发送确认回调
 * @param Callback - 回调函数指针
 */
void LinMaster_Tp_RegisterTxConfirmCallback(LinMaster_Tp_TxConfirmFuncType Callback);

/**
 * @brief 获取通道当前状态
 * @return 通道状态
 */
LinMaster_Tp_StateType LinMaster_Tp_GetState(void);

/**
 * @brief 检查TP是否忙碌
 * @return TRUE=忙碌, FALSE=空闲
 */
boolean LinMaster_Tp_IsBusy(void);

/**
 * @brief 取消当前传输
 */
void LinMaster_Tp_Cancel(void);

/**
 * @brief 获取版本信息
 * @param VersionInfo - 版本信息结构体指针
 */
void LinMaster_Tp_GetVersionInfo(Std_VersionInfoType* VersionInfo);

#endif /* LINMASTER_TP_H */
