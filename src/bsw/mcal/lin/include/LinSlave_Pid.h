/**
 * @file LinSlave_Pid.h
 * @brief PID处理模块头文件
 * @version 1.0.0
 */

#ifndef LINSLAVE_PID_H
#define LINSLAVE_PID_H

#include "Std_Types.h"

/**
 * @brief 计算PID保护位
 * @param Id - 标识符 (0-59)
 * @return 完整的PID (带保护位)
 * @details 算法:
 *          P0 = ID0 ^ ID1 ^ ID2 ^ ID4
 *          P1 = ~(ID1 ^ ID3 ^ ID4 ^ ID5)
 */
uint8 LinSlave_CalculatePid(uint8 Id);

/**
 * @brief 验证PID有效性
 * @param Pid - 待验证的PID
 * @return TRUE=有效, FALSE=无效
 */
boolean LinSlave_ValidatePid(uint8 Pid);

/**
 * @brief 从PID提取ID
 * @param Pid - 完整的PID
 * @return 标识符 (0-59)
 */
uint8 LinSlave_ExtractId(uint8 Pid);

#endif /* LINSLAVE_PID_H */
