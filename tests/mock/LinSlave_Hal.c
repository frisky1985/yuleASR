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
 * @file LinSlave_Hal.c
 * @brief 硬件抽象层模拟实现
 * @version 1.0.0
 * @note 此为模拟实现，实际项目需要针对具体MCU实现
 */

#include "LinSlave_Hal.h"
#include <stdio.h>
#include <time.h>

/* 模拟UART发送函数 */
/** @req SWS_Lin_00060 */
void LinSlave_Hal_UartInit(uint32_t BaudRate)
{
    /* 模拟: 打印初始化信息 */
    printf("[HAL] UART Init: BaudRate=%u\n", (unsigned int)BaudRate);
}

/** @req SWS_Lin_00061 */
void LinSlave_Hal_UartSend(uint8 Data)
{
    /* 模拟: 打印发送的数据 */
    (void)printf("[HAL] UART TX: 0x%02X\n", Data);
}

/** @req SWS_Lin_00062 */
void LinSlave_Hal_UartSendBuffer(const uint8* Buffer, uint8 Length)
{
    uint8 i;
    (void)printf("[HAL] UART TX Buffer [%d bytes]: ", Length);
    for (i = 0; i < Length; i++) {
        (void)printf("0x%02X ", Buffer[i]);
    }
    (void)printf("\n");
}

/** @req SWS_Lin_00063 */
void LinSlave_Hal_EnableRxInterrupt(void)
{
    (void)printf("[HAL] RX Interrupt Enabled\n");
}

/** @req SWS_Lin_00064 */
void LinSlave_Hal_DisableRxInterrupt(void)
{
    (void)printf("[HAL] RX Interrupt Disabled\n");
}

/** @req SWS_Lin_00065 */
void LinSlave_Hal_EnableBreakDetection(void)
{
    (void)printf("[HAL] Break Detection Enabled\n");
}

/** @req SWS_Lin_00066 */
void LinSlave_Hal_DisableBreakDetection(void)
{
    (void)printf("[HAL] Break Detection Disabled\n");
}

/** @req SWS_Lin_00067 */
uint32_t LinSlave_Hal_GetTimestampMs(void)
{
    /* 模拟: 返回模拟的时间戳 */
    static uint32_t mockTime = 0;
    return mockTime++;
}

/** @req SWS_Lin_00068 */
void LinSlave_Hal_DelayUs(uint16_t Microseconds)
{
    /* 模拟: 打印延时信息 */
    (void)printf("[HAL] Delay %u us\n", Microseconds);
}
