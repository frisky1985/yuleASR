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
 * @file LinMaster_Hal.c
 * @brief LinMaster HAL层示例实现
 * @version 1.0.0
 * @note 此为示例实现，需要根据具体硬件平台进行适配
 */

#include "LinMaster_Hal.h"
#include "LinMaster_Cfg.h"

/* 模拟硬件寄存器 (实际应用中替换为真实硬件寄存器) */
static volatile uint8 MockUartDataReg;
static volatile uint8 MockUartStatusReg;
static volatile uint8 MockBusState;

/* 当前波特率 */
static uint16 LinMaster_Hal_CurrentBaudRate = LINMASTER_DEFAULT_BAUDRATE;

/**
 * @brief 初始化UART
 */
LinMaster_StatusType LinMaster_Hal_UartInit(uint16 BaudRate)
{
    /* 保存波特率 */
    LinMaster_Hal_CurrentBaudRate = BaudRate;
    
    /* 初始化硬件UART:
     * 1. 配置GPIO
     * 2. 设置波特率
     * 3. 配置数据位、停止位、校验位
     * 4. 使能UART
     */
    
    /* 示例: 清零模拟寄存器 */
    MockUartDataReg = 0;
    MockUartStatusReg = 0;
    MockBusState = 1; /* 默认总线空闲(隐性) */
    
    return LINMASTER_OK;
}

/**
 * @brief 发送单个字节
 */
LinMaster_StatusType LinMaster_Hal_SendByte(uint8 Byte)
{
    /* 实际实现:
     * 1. 等待发送缓冲区空
     * 2. 写入数据寄存器
     * 3. 等待发送完成 (或使能中断)
     */
    
    MockUartDataReg = Byte;
    
    /* 模拟发送完成 */
    return LINMASTER_OK;
}

/**
 * @brief 发送Break字段 (13+ 位显性电平)
 * 
 * LIN Break字段要求:
 * - 至少13个位时间的显性电平 (0)
 * - 接着是至少1位的隐性电平 (停止位)
 * - 然后是同步字节 (0x55)
 */
LinMaster_StatusType LinMaster_Hal_SendBreak(void)
{
    /* 实际实现方法1: 使用UART的Break功能
     * - 设置UART的Break控制位
     * - 硬件会自动产生13位以上的显性电平
     */
    
    /* 实际实现方法2: 使用IO口模拟
     * - 将TX设为GPIO输出
     * - 拉低13位时间
     * - 恢复UART功能
     */
    
    /* 计算Break持续时间:
     * @9600bps: 1位 = 104.17us, 13位 = 1354us
     * @19200bps: 1位 = 52.08us, 13位 = 677us
     */
    uint16 breakDurationUs;
    
    if (LinMaster_Hal_CurrentBaudRate == 9600) {
        breakDurationUs = 1354; /* 13位 @ 9600bps */
    } else if (LinMaster_Hal_CurrentBaudRate == 19200) {
        breakDurationUs = 677;  /* 13位 @ 19200bps */
    } else {
        breakDurationUs = (uint16)((13000000UL / LinMaster_Hal_CurrentBaudRate) + 1);
    }
    
    /* 模拟发送Break:
     * 实际应用中，这里需要操作硬件产生显性电平
     */
    MockBusState = 0; /* 总线显性 */
    
    /* 延时等待 ( 实际应用中使用精确延时 ) */
    /* 注意: 在实际应用中不建议使用软件延时，应使用定时器 */
    volatile uint32 delay = breakDurationUs * 10; /* 模拟延时 */
    while (delay--) {
        /* 空循环延时 */
    }
    
    MockBusState = 1; /* 总线恢复隐性 */
    
    return LINMASTER_OK;
}

/**
 * @brief 发送数据块
 */
LinMaster_StatusType LinMaster_Hal_SendBlock(const uint8* DataPtr, uint8 Length)
{
    uint8 i;
    
    if (DataPtr == NULL_PTR || Length == 0U ) {
        return LINMASTER_NOT_OK;
    }
    
    for (i = 0; i < Length; i++) {
        if (LinMaster_Hal_SendByte(DataPtr[i]) != LINMASTER_OK) {
            return LINMASTER_NOT_OK;
        }
    }
    
    return LINMASTER_OK;
}

/**
 * @brief 使能接收中断
 */
void LinMaster_Hal_EnableRxInterrupt(void)
{
    /* 实际实现: 使能UART接收中断 */
    MockUartStatusReg |= 0x20; /* 使能RX中断标志 */
}

/**
 * @brief 禁能接收中断
 */
void LinMaster_Hal_DisableRxInterrupt(void)
{
    /* 实际实现: 禁能UART接收中断 */
    MockUartStatusReg &= ~0x20; /* 禁能RX中断标志 */
}

/**
 * @brief 使能发送完成中断
 */
void LinMaster_Hal_EnableTxInterrupt(void)
{
    /* 实际实现: 使能UART发送完成中断 */
    MockUartStatusReg |= 0x40; /* 使能TX中断标志 */
}

/**
 * @brief 禁能发送完成中断
 */
void LinMaster_Hal_DisableTxInterrupt(void)
{
    /* 实际实现: 禁能UART发送完成中断 */
    MockUartStatusReg &= ~0x40; /* 禁能TX中断标志 */
}

/**
 * @brief 获取当前时间 (毫秒)
 * 
 * 注意: 此为模拟实现，实际应用需要使用系统时钟/定时器
 */
static uint32 MockCurrentTimeMs = 0;

uint32 LinMaster_Hal_GetCurrentTimeMs(void)
{
    /* 实际实现:
     * - 使用系统tick
     * - 或使用硬件定时器
     */
    MockCurrentTimeMs++; /* 模拟时间递增 */
    return MockCurrentTimeMs;
}

/**
 * @brief 延迟函数 (毫秒)
 * 
 * 注意: 此为模拟实现，实际应用不建议使用软件延时
 */
void LinMaster_Hal_DelayMs(uint16 DelayMs)
{
    volatile uint32 delay = DelayMs * 1000;
    while (delay--) {
        /* 空循环延时 */
    }
}

/**
 * @brief 获取总线状态
 * @return 总线状态 (0=显性, 1=隐性)
 */
uint8 LinMaster_Hal_GetBusState(void)
{
    /* 实际实现: 读取GPIO状态或UART状态寄存器 */
    return MockBusState;
}
