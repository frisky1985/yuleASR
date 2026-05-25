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
 * @file LinSlave_Checksum.c
 * @brief 校验和处理模块实现
 * @version 1.0.0
 */

#include "LinSlave_Checksum.h"

/**
 * LIN经典校验和计算
 * 对数据字节求和，然后取反
 * 使用加法进位: 如果和 >= 256，则减去255
 */
static uint8 CalculateClassicChecksum(const uint8* DataPtr, uint8 Length)
{
    uint16 Sum = 0;
    uint8 i;
    
    for (i = 0; i < Length; i++) {
        Sum += DataPtr[i];
        if (Sum >= 256U) {
            Sum -= 255U;
        }
    }
    
    return (uint8)(~Sum);
}

/**
 * LIN增强校验和计算
 * 包含PID在校验和计算中
 */
static uint8 CalculateEnhancedChecksum(uint8 Pid, const uint8* DataPtr, uint8 Length)
{
    uint16 Sum = Pid;
    uint8 i;
    
    for (i = 0; i < Length; i++) {
        Sum += DataPtr[i];
        if (Sum >= 256U) {
            Sum -= 255U;
        }
    }
    
    return (uint8)(~Sum);
}

/**
 * 计算校验和
 */
uint8 LinSlave_CalculateChecksum(
    const uint8* DataPtr,
    uint8 Length,
    uint8 Pid,
    LinSlave_ChecksumType ChecksumType
)
{
    if (DataPtr == NULL_PTR || Length == 0U || Length > 8U) {
        return 0xFFU;  /* 错误返回 */
    }
    
    if (ChecksumType == LINSLAVE_CHECKSUM_ENHANCED) {
        return CalculateEnhancedChecksum(Pid, DataPtr, Length);
    } else {
        return CalculateClassicChecksum(DataPtr, Length);
    }
}

/**
 * 验证校验和
 */
boolean LinSlave_ValidateChecksum(
    const uint8* DataPtr,
    uint8 Length,
    uint8 Pid,
    LinSlave_ChecksumType ChecksumType,
    uint8 ReceivedChecksum
)
{
    uint8 CalculatedChecksum;
    
    if (DataPtr == NULL_PTR || Length == 0U || Length > 8U) {
        return FALSE;
    }
    
    CalculatedChecksum = LinSlave_CalculateChecksum(DataPtr, Length, Pid, ChecksumType);
    
    return (CalculatedChecksum == ReceivedChecksum) ? TRUE : FALSE;
}
