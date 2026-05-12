/**
 * @file LinSlave_Checksum.h
 * @brief 校验和处理模块头文件
 * @version 1.0.0
 */

#ifndef LINSLAVE_CHECKSUM_H
#define LINSLAVE_CHECKSUM_H

#include "Std_Types.h"

/* 校验和类型 */
typedef enum {
    LINSLAVE_CHECKSUM_CLASSIC = 0,  /* 经典校验和 (仅数据) */
    LINSLAVE_CHECKSUM_ENHANCED      /* 增强校验和 (包含PID) */
} LinSlave_ChecksumType;

/**
 * @brief 计算校验和
 * @param DataPtr - 数据缓冲区指针
 * @param Length - 数据长度
 * @param Pid - PID (增强校验和需要)
 * @param ChecksumType - 校验和类型
 * @return 计算结果
 * @details 算法:
 *          - 经典: 对所有数据字节求和，取反
 *          - 增强: 先加上PID，然后同经典
 */
uint8 LinSlave_CalculateChecksum(
    const uint8* DataPtr,
    uint8 Length,
    uint8 Pid,
    LinSlave_ChecksumType ChecksumType
);

/**
 * @brief 验证校验和
 * @param DataPtr - 数据缓冲区指针
 * @param Length - 数据长度
 * @param Pid - PID
 * @param ChecksumType - 校验和类型
 * @param ReceivedChecksum - 接收到的校验和
 * @return TRUE=验证通过, FALSE=失败
 */
boolean LinSlave_ValidateChecksum(
    const uint8* DataPtr,
    uint8 Length,
    uint8 Pid,
    LinSlave_ChecksumType ChecksumType,
    uint8 ReceivedChecksum
);

#endif /* LINSLAVE_CHECKSUM_H */
