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
 * @file LinSlave_Pid.c
 * @brief PID处理模块实现
 * @version 1.0.0
 */

#include "LinSlave_Pid.h"

/**
 * PID保护位计算
 * LIN协议使用奇偶校验位保护PID
 * P0 = ID0 ^ ID1 ^ ID2 ^ ID4
 * P1 = ~(ID1 ^ ID3 ^ ID4 ^ ID5)
 */
uint8 LinSlave_CalculatePid(uint8 Id)
{
    uint8 P0, P1;
    uint8 Pid;
    
    /* 限制ID范围在0-59 */
    Id = (uint8)(Id & 0x3FU);
    
    /* 计算P0 - 奇校验位 */
    P0 = (uint8)((Id >> 0U) & 0x01U) ^ 
         (uint8)((Id >> 1U) & 0x01U) ^ 
         (uint8)((Id >> 2U) & 0x01U) ^ 
         (uint8)((Id >> 4U) & 0x01U);
    
    /* 计算P1 - 偶校验位 (取反) */
    P1 = (uint8)((unsigned int)(~((uint8)((Id >> 1U) & 0x01U) ^ 
           (uint8)((Id >> 3U) & 0x01U) ^ 
           ((Id >> 4) & 0x01U) ^ 
           (uint8)((Id >> 5U) & 0x01U))) & 0x01U);
    
    /* 组合PID */
    Pid = Id | (uint8)(P0 << 6U) | (uint8)(P1 << 7U);
    
    return Pid;
}

/**
 * 验证PID有效性
 * 通过重新计算保护位并比较
 */
boolean LinSlave_ValidatePid(uint8 Pid)
{
    uint8 Id;
    uint8 CalculatedPid;
    
    /* 提取ID (4-6位) */
    Id = (uint8)(Pid & 0x3FU);
    
    /* 检查ID范围 (0-59) */
    if (Id > 59U) {
        return FALSE;
    }
    
    /* 计算预期的PID */
    CalculatedPid = LinSlave_CalculatePid(Id);
    
    /* 比较 */
    return (Pid == CalculatedPid) ? TRUE : FALSE;
}

/**
 * 从PID提取ID
 */
uint8 LinSlave_ExtractId(uint8 Pid)
{
    return (uint8)(Pid & 0x3FU);
}
