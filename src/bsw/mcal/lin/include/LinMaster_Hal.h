/**
 * @file LinMaster_Hal.h
 * @brief LinMaster HAL层接口 - 硬件抽象层
 * @version 1.0.0
 */

#ifndef LINMASTER_HAL_H
#define LINMASTER_HAL_H

#include <stdint.h>
#include "Std_Types.h"
#include "LinMaster_Types.h"

/**
 * @brief 初始化UART
 * @param BaudRate - 波特率
 * @return 操作状态
 */
LinMaster_StatusType LinMaster_Hal_UartInit(uint16 BaudRate);

/**
 * @brief 发送单个字节
 * @param Byte - 要发送的字节
 * @return 操作状态
 */
LinMaster_StatusType LinMaster_Hal_SendByte(uint8 Byte);

/**
 * @brief 发送Break字段 (13+ 位显性电平)
 * @return 操作状态
 * @note 产生至少13个位时间的显性电平 (0)
 */
LinMaster_StatusType LinMaster_Hal_SendBreak(void);

/**
 * @brief 发送数据块
 * @param DataPtr - 数据指针
 * @param Length - 数据长度
 * @return 操作状态
 */
LinMaster_StatusType LinMaster_Hal_SendBlock(const uint8* DataPtr, uint8 Length);

/**
 * @brief 使能接收中断
 */
void LinMaster_Hal_EnableRxInterrupt(void);

/**
 * @brief 禁能接收中断
 */
void LinMaster_Hal_DisableRxInterrupt(void);

/**
 * @brief 使能发送完成中断
 */
void LinMaster_Hal_EnableTxInterrupt(void);

/**
 * @brief 禁能发送完成中断
 */
void LinMaster_Hal_DisableTxInterrupt(void);

/**
 * @brief 获取当前时间 (毫秒)
 * @return 当前时间戳 (毫秒)
 */
uint32 LinMaster_Hal_GetCurrentTimeMs(void);

/**
 * @brief 延迟函数 (毫秒)
 * @param DelayMs - 延迟时间 (毫秒)
 */
void LinMaster_Hal_DelayMs(uint16 DelayMs);

/**
 * @brief 获取总线状态
 * @return 总线状态 (0=显性, 1=隐性)
 */
uint8 LinMaster_Hal_GetBusState(void);

#endif /* LINMASTER_HAL_H */
