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
 * @file LinSlave_Hal.h
 * @brief 硬件抽象层头文件
 * @version 1.0.0
 */

#ifndef LINSLAVE_HAL_H
#define LINSLAVE_HAL_H

#include <stdint.h>
#include "Std_Types.h"

/**
 * @brief UART初始化
 * @param BaudRate - 波特率
 */
void LinSlave_Hal_UartInit(uint32_t BaudRate);

/**
 * @brief 发送单字节
 * @param Data - 待发送字节
 */
void LinSlave_Hal_UartSend(uint8 Data);

/**
 * @brief 发送缓冲区
 * @param Buffer - 数据缓冲区
 * @param Length - 长度
 */
void LinSlave_Hal_UartSendBuffer(const uint8* Buffer, uint8 Length);

/**
 * @brief 使能接收中断
 */
void LinSlave_Hal_EnableRxInterrupt(void);

/**
 * @brief 禁能接收中断
 */
void LinSlave_Hal_DisableRxInterrupt(void);

/**
 * @brief 使能Break检测
 */
void LinSlave_Hal_EnableBreakDetection(void);

/**
 * @brief 禁能Break检测
 */
void LinSlave_Hal_DisableBreakDetection(void);

/**
 * @brief 获取当前时间戳 (毫秒)
 * @return 时间戳
 */
uint32_t LinSlave_Hal_GetTimestampMs(void);

/**
 * @brief 微秒级延时
 * @param Microseconds - 延时微秒数
 */
void LinSlave_Hal_DelayUs(uint16_t Microseconds);

#endif /* LINSLAVE_HAL_H */
