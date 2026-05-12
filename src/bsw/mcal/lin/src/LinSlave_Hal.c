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
void LinSlave_Hal_UartInit(uint32_t BaudRate)
{
    /* 模拟: 打印初始化信息 */
    printf("[HAL] UART Init: BaudRate=%u\n", (unsigned int)BaudRate);
}

void LinSlave_Hal_UartSend(uint8 Data)
{
    /* 模拟: 打印发送的数据 */
    printf("[HAL] UART TX: 0x%02X\n", Data);
}

void LinSlave_Hal_UartSendBuffer(const uint8* Buffer, uint8 Length)
{
    uint8 i;
    printf("[HAL] UART TX Buffer [%d bytes]: ", Length);
    for (i = 0; i < Length; i++) {
        printf("0x%02X ", Buffer[i]);
    }
    printf("\n");
}

void LinSlave_Hal_EnableRxInterrupt(void)
{
    printf("[HAL] RX Interrupt Enabled\n");
}

void LinSlave_Hal_DisableRxInterrupt(void)
{
    printf("[HAL] RX Interrupt Disabled\n");
}

void LinSlave_Hal_EnableBreakDetection(void)
{
    printf("[HAL] Break Detection Enabled\n");
}

void LinSlave_Hal_DisableBreakDetection(void)
{
    printf("[HAL] Break Detection Disabled\n");
}

uint32_t LinSlave_Hal_GetTimestampMs(void)
{
    /* 模拟: 返回模拟的时间戳 */
    static uint32_t mockTime = 0;
    return mockTime++;
}

void LinSlave_Hal_DelayUs(uint16_t Microseconds)
{
    /* 模拟: 打印延时信息 */
    printf("[HAL] Delay %u us\n", Microseconds);
}
