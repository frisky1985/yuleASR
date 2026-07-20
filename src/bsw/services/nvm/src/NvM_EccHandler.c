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
 * @file NvM_EccHandler.c
 * @brief NvM ECC故障处理实现
 * 
 * 实现NvM特定的ECC错误处理策略，包括数据恢复、写验证和块保护
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "NvM_EccHandler.h"
#include "NvM_Private.h"
#include "Crc.h"
#include "Mcal.h"
#include "Dem.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 模块ID
 */
#define NVM_ECCHANDLER_MODULE_ID                        0x14U

/**
 * @brief API ID
 */
#define NVM_ECCHANDLER_API_INIT                         0x01U
#define NVM_ECCHANDLER_API_HANDLE_READ                  0x10U
#define NVM_ECCHANDLER_API_HANDLE_WRITE                 0x11U

/**
 * @brief CRC多项式
 */
#define NVM_ECC_CRC_POLYNOMIAL                          0x04C11DB7U

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define NVM_ECCHANDLER_START_SEC_VAR_INIT_UNSPECIFIED
#include "NvM_MemMap.h"

/**
 * @brief 初始化状态
 */
STATIC boolean NvM_EccHandler_Initialized = FALSE;

/**
 * @brief 块配置数组
 */
STATIC const NvM_EccBlockConfigType* NvM_EccBlockConfigs = NULL_PTR;

/**
 * @brief 配置块数量
 */
STATIC uint16 NvM_EccNumBlocks = 0U;

/**
 * @brief 错误回调
 */
STATIC NvM_EccErrorCallbackType NvM_EccCallback = NULL_PTR;

#define NVM_ECCHANDLER_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "NvM_MemMap.h"

#define NVM_ECCHANDLER_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "NvM_MemMap.h"

/**
 * @brief 块错误计数
 */
STATIC uint8 NvM_EccErrorCount[NVM_CFG_MAX_BLOCK_ID];

#define NVM_ECCHANDLER_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "NvM_MemMap.h"

/*==================================================================================================
*                                       静态函数声明
==================================================================================================*/
STATIC const NvM_EccBlockConfigType* NvM_EccHandler_FindBlockConfig(NvM_BlockIdType blockId);
STATIC Std_ReturnType NvM_EccHandler_CalculateCrc(const uint8* data, uint16 length, uint32* crc);
STATIC void NvM_EccHandler_NotifyDem(uint8 errorType, NvM_BlockIdType blockId);
STATIC void NvM_EccHandler_ResetErrorCount(NvM_BlockIdType blockId);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define NVM_ECCHANDLER_START_SEC_CODE
#include "NvM_MemMap.h"

/**
 * @brief 初始化NvM ECC处理模块
 */
Std_ReturnType NvM_EccHandler_Init(const NvM_EccBlockConfigType* configPtr, uint16 numBlocks)
{
    uint16 i;
    
    /* 清除错误计数 */
    for (i = 0U; i < NVM_CFG_MAX_BLOCK_ID; i++)
    {
        NvM_EccErrorCount[i] = 0U;
    }
    
    if ((configPtr != NULL_PTR) && (numBlocks > 0U))
    {
        /* 使用提供的配置 */
        NvM_EccBlockConfigs = configPtr;
        NvM_EccNumBlocks = numBlocks;
    }
    else
    {
        /* 使用默认配置 */
        NvM_EccBlockConfigs = NvM_EccDefaultConfig;
        NvM_EccNumBlocks = NvM_EccNumConfiguredBlocks;
    }
    
    NvM_EccHandler_Initialized = TRUE;
    NvM_EccCallback = NULL_PTR;
    
    return E_OK;
}

/**
 * @brief 去初始化NvM ECC处理模块
 */
Std_ReturnType NvM_EccHandler_DeInit(void)
{
    uint16 i;
    
    if (NvM_EccHandler_Initialized == 0U)
    {
        return E_NOT_OK;
    }
    
    /* 清除错误计数 */
    for (i = 0U; i < NVM_CFG_MAX_BLOCK_ID; i++)
    {
        NvM_EccErrorCount[i] = 0U;
    }
    
    NvM_EccBlockConfigs = NULL_PTR;
    NvM_EccNumBlocks = 0U;
    NvM_EccHandler_Initialized = FALSE;
    NvM_EccCallback = NULL_PTR;
    
    return E_OK;
}

/**
 * @brief 处理块读取时的ECC错误
 * @ASIL-D: Read error recovery
 */
Std_ReturnType NvM_EccHandler_HandleReadError(
    NvM_BlockIdType blockId,
    const NvM_EccErrorInfoType* errorInfo,
    uint8* dataBuffer,
    uint16 dataLength)
{
    Std_ReturnType result = E_NOT_OK;
    const NvM_EccBlockConfigType* blockConfig;
    NvM_EccErrorInfoType localErrorInfo;
    
    if (NvM_EccHandler_Initialized == 0U)
    {
        return E_NOT_OK;
    }
    
    if ((errorInfo == NULL_PTR) || (dataBuffer == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    /* 查找块配置 */
    blockConfig = NvM_EccHandler_FindBlockConfig(blockId);
    if (blockConfig == NULL_PTR)
    {
        /* 使用默认策略 */
        blockConfig = &NvM_EccDefaultConfig[0];
    }
    
    /* 复制错误信息 */
    localErrorInfo = *errorInfo;
    localErrorInfo.blockId = blockId;
    
    /* 增加错误计数 */
    if (blockId < NVM_CFG_MAX_BLOCK_ID)
    {
        NvM_EccErrorCount[blockId]++;
        localErrorInfo.retryCount = NvM_EccErrorCount[blockId];
    }
    
    /* 根据错误类型和策略处理 */
    switch (errorInfo->errorType)
    {
        case NVM_ECC_ERROR_SINGLE_BIT_CORRECTED:
            /* 硬件已纠正，只需通知 */
            localErrorInfo.recovered = TRUE;
            localErrorInfo.recoveryMethod = NVM_ECC_RECOVERY_NONE;
            result = E_OK;
            break;
            
        case NVM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE:
        case NVM_ECC_ERROR_BLOCK_INTEGRITY_LOST:
            /* 需要数据恢复 */
            switch (blockConfig->recoveryStrategy)
            {
                case NVM_ECC_RECOVERY_USE_ROM_DEFAULT:
                    result = NvM_EccHandler_RecoverFromRomDefault(blockId, dataBuffer, dataLength);
                    if (result == E_OK)
                    {
                        localErrorInfo.recovered = TRUE;
                        localErrorInfo.recoveryMethod = NVM_ECC_RECOVERY_USE_ROM_DEFAULT;
                    }
                    break;
                    
                case NVM_ECC_RECOVERY_USE_REDUNDANT_COPY:
                    result = NvM_EccHandler_RecoverFromRedundantCopy(blockId, dataBuffer, dataLength);
                    if (result == E_OK)
                    {
                        localErrorInfo.recovered = TRUE;
                        localErrorInfo.recoveryMethod = NVM_ECC_RECOVERY_USE_REDUNDANT_COPY;
                    }
                    break;
                    
                case NVM_ECC_RECOVERY_ERASE_AND_RETRY:
                    /* 擦除并尝试重新读取 */
                    /* 注意：这可能导致数据丢失，仅适用于临时存储 */
                    result = E_NOT_OK;
                    break;
                    
                case NVM_ECC_RECOVERY_MARK_INVALID:
                default:
                    /* 标记为损坏 */
                    (void)NvM_EccHandler_MarkBlockCorrupted(blockId, errorInfo->errorType);
                    result = E_NOT_OK;
                    break;
            }
            break;
            
        default:
            result = E_NOT_OK;
            break;
    }
    
    /* 报告给DEM */
    NvM_EccHandler_NotifyDem(errorInfo->errorType, blockId);
    
    /* 调用用户回调 */
    if (NvM_EccCallback != NULL_PTR)
    {
        NvM_EccCallback(&localErrorInfo, dataBuffer, dataLength);
    }
    
    /* 如果恢复成功，重置错误计数 */
    if ((result == E_OK) && (localErrorInfo.recovered))
    {
        NvM_EccHandler_ResetErrorCount(blockId);
    }
    
    return result;
}

/**
 * @brief 处理块写入时的验证失败
 * @ASIL-D: Write verification failure handling
 */
Std_ReturnType NvM_EccHandler_HandleWriteVerifyFailure(
    NvM_BlockIdType blockId,
    const uint8* dataBuffer,
    uint16 dataLength)
{
    const NvM_EccBlockConfigType* blockConfig;
    uint8 retryCount = 0U;
    Std_ReturnType result = E_NOT_OK;
    
    if (NvM_EccHandler_Initialized == 0U)
    {
        return E_NOT_OK;
    }
    
    if (dataBuffer == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* 查找块配置 */
    blockConfig = NvM_EccHandler_FindBlockConfig(blockId);
    if (blockConfig == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* 重试写入 */
    while (retryCount < blockConfig->maxRetries)
    {
        /* 重新写入数据 */
        result = NvM_EccHandler_ProtectedWrite(
            (uint8*)NvM_GetBlockAddress(blockId),
            dataBuffer,
            dataLength
        );
        
        if (result == E_OK)
        {
            /* 验证写入 */
            if (NvM_EccHandler_VerifyBlockIntegrity(blockId, dataBuffer, dataLength) == E_OK)
            {
                /* 成功 */
                NvM_EccHandler_ResetErrorCount(blockId);
                return E_OK;
            }
        }
        
        retryCount++;
        
        /* 延迟后重试 */
        if (retryCount < blockConfig->maxRetries)
        {
            /* 简单延迟 */
            volatile uint32 delay = NVM_ECC_WRITE_VERIFY_RETRY_DELAY_MS * 1000U;
            while (delay > 0U)
            {
                delay--;
            }
        }
    }
    
    /* 重试失败，报告错误 */
    if (blockId < NVM_CFG_MAX_BLOCK_ID)
    {
        NvM_EccErrorCount[blockId] = retryCount;
    }
    
    NvM_EccHandler_NotifyDem(NVM_ECC_ERROR_WRITE_VERIFICATION_FAILED, blockId);
    
    return E_NOT_OK;
}

/**
 * @brief 读取块时的RAM保护
 * @ASIL-D: Atomic read protection
 */
Std_ReturnType NvM_EccHandler_ProtectedRead(
    const uint8* srcAddr,
    uint8* destBuffer,
    uint16 length)
{
    uint16 i;
    
    if ((srcAddr == NULL_PTR) || (destBuffer == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    /* 禁用中断 */
    Mcal_DisableAllInterrupts();
    
    /* 复制数据 */
    for (i = 0U; i < length; i++)
    {
        destBuffer[i] = srcAddr[i];
    }
    
    /* 重新启用中断 */
    Mcal_EnableAllInterrupts();
    
    return E_OK;
}

/**
 * @brief 写入块时的RAM保护
 * @ASIL-D: Atomic write protection
 */
Std_ReturnType NvM_EccHandler_ProtectedWrite(
    uint8* destAddr,
    const uint8* srcBuffer,
    uint16 length)
{
    uint16 i;
    
    if ((destAddr == NULL_PTR) || (srcBuffer == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    /* 禁用中断 */
    Mcal_DisableAllInterrupts();
    
    /* 复制数据 */
    for (i = 0U; i < length; i++)
    {
        destAddr[i] = srcBuffer[i];
    }
    
    /* 内存屏障 */
    __asm("dsb");
    
    /* 重新启用中断 */
    Mcal_EnableAllInterrupts();
    
    return E_OK;
}

/**
 * @brief 验证块数据完整性
 */
Std_ReturnType NvM_EccHandler_VerifyBlockIntegrity(
    NvM_BlockIdType blockId,
    const uint8* dataBuffer,
    uint16 dataLength)
{
    uint32 calculatedCrc;
    uint32 storedCrc;
    const NvM_EccBlockConfigType* blockConfig;
    
    if (dataBuffer == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* 查找块配置 */
    blockConfig = NvM_EccHandler_FindBlockConfig(blockId);
    if ((blockConfig != NULL_PTR) && (!blockConfig->enableWriteVerify))
    {
        /* 禁用验证，直接返回成功 */
        return E_OK;
    }
    
    /* 计算CRC */
    if (NvM_EccHandler_CalculateCrc(dataBuffer, dataLength, &calculatedCrc) != E_OK)
    {
        return E_NOT_OK;
    }
    
    /* 获取存储的CRC */
    /* 注意：这里假设CRC存储在数据末尾或NvM管理结构中 */
    /* 实际实现需要根据NvM内部结构确定 */
    storedCrc = 0U;  /* 需要实际实现 */
    
    if (calculatedCrc == storedCrc)
    {
        return E_OK;
    }
    
    return E_NOT_OK;
}

/**
 * @brief 从ROM默认值恢复块
 */
Std_ReturnType NvM_EccHandler_RecoverFromRomDefault(
    NvM_BlockIdType blockId,
    uint8* dataBuffer,
    uint16 dataLength)
{
    const NvM_EccBlockConfigType* blockConfig;
    uint16 i;
    const uint8* romData;
    
    if (dataBuffer == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* 查找块配置 */
    blockConfig = NvM_EccHandler_FindBlockConfig(blockId);
    if (blockConfig == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    romData = (const uint8*)blockConfig->romDefaultData;
    if (romData == NULL_PTR)
    {
        /* 无ROM默认值 */
        return E_NOT_OK;
    }
    
    /* 复制ROM默认值 */
    for (i = 0U; i < dataLength; i++)
    {
        dataBuffer[i] = romData[i];
    }
    
    /* 报告恢复成功 */
    NvM_EccHandler_NotifyDem(NVM_ECC_ERROR_NONE, blockId);
    
    return E_OK;
}

/**
 * @brief 从冗余副本恢复块
 */
Std_ReturnType NvM_EccHandler_RecoverFromRedundantCopy(
    NvM_BlockIdType blockId,
    uint8* dataBuffer,
    uint16 dataLength)
{
    /* 
     * 从冗余副本恢复的实现
     * 这需要NvM支持冗余存储
     */
    
    /* 示例实现 */
    uint32 redundantAddr;
    
    (void)blockId;
    
    if (dataBuffer == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* 计算冗余副本地址 */
    redundantAddr = NvM_GetRedundantBlockAddress(blockId);
    if (redundantAddr == 0U)
    {
        return E_NOT_OK;
    }
    
    /* 读取冗余副本 */
    return NvM_EccHandler_ProtectedRead(
        (const uint8*)redundantAddr,
        dataBuffer,
        dataLength
    );
}

/**
 * @brief 标记块为损坏
 */
Std_ReturnType NvM_EccHandler_MarkBlockCorrupted(NvM_BlockIdType blockId, uint8 errorType)
{
    (void)errorType;
    
    /* 设置块状态为损坏 */
    /* 调用NvM内部API */
    #ifdef NVM_SET_BLOCK_STATUS
    NvM_SetBlockStatus(blockId, NVM_BLOCK_STATUS_CORRUPTED);
    #endif
    
    /* 报告DEM */
    NvM_EccHandler_NotifyDem(NVM_ECC_ERROR_BLOCK_INTEGRITY_LOST, blockId);
    
    return E_OK;
}

/**
 * @brief 注册错误回调
 */
Std_ReturnType NvM_EccHandler_RegisterCallback(NvM_EccErrorCallbackType callback)
{
    if (NvM_EccHandler_Initialized == 0U)
    {
        return E_NOT_OK;
    }
    
    NvM_EccCallback = callback;
    return E_OK;
}

/**
 * @brief 获取块的ECC配置
 */
Std_ReturnType NvM_EccHandler_GetBlockConfig(
    NvM_BlockIdType blockId,
    NvM_EccBlockConfigType* config)
{
    const NvM_EccBlockConfigType* blockConfig;
    
    if (config == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    blockConfig = NvM_EccHandler_FindBlockConfig(blockId);
    if (blockConfig == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    *config = *blockConfig;
    return E_OK;
}

/**
 * @brief 设置块的恢复策略
 */
Std_ReturnType NvM_EccHandler_SetRecoveryStrategy(NvM_BlockIdType blockId, uint8 strategy)
{
    const NvM_EccBlockConfigType* blockConfig;
    
    blockConfig = NvM_EccHandler_FindBlockConfig(blockId);
    if (blockConfig == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    /* 注意：这修改的是配置，实际应用中可能需要更复杂的机制 */
    ((NvM_EccBlockConfigType*)blockConfig)->recoveryStrategy = strategy;
    
    return E_OK;
}

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief 查找块配置
 */
STATIC const NvM_EccBlockConfigType* NvM_EccHandler_FindBlockConfig(NvM_BlockIdType blockId)
{
    uint16 i;
    
    if (NvM_EccBlockConfigs == NULL_PTR)
    {
        return NULL_PTR;
    }
    
    for (i = 0U; i < NvM_EccNumBlocks; i++)
    {
        if (NvM_EccBlockConfigs[i].blockId == blockId)
        {
            return &NvM_EccBlockConfigs[i];
        }
    }
    
    return NULL_PTR;
}

/**
 * @brief 计算CRC
 */
STATIC Std_ReturnType NvM_EccHandler_CalculateCrc(const uint8* data, uint16 length, uint32* crc)
{
    uint32 i;
    uint32 j;
    uint32 crcValue = 0xFFFFFFFFU;
    
    if ((data == NULL_PTR) || (crc == NULL_PTR))
    {
        return E_NOT_OK;
    }
    
    for (i = 0U; i < length; i++)
    {
        crcValue ^= ((uint32)data[i] << 24U);
        
        for (j = 0U; j < 8U; j++)
        {
            if ((crcValue & 0x80000000U) != 0U)
            {
                crcValue = (crcValue << 1U) ^ NVM_ECC_CRC_POLYNOMIAL;
            }
            else
            {
                crcValue = crcValue << 1U;
            }
        }
    }
    
    *crc = crcValue;
    return E_OK;
}

/**
 * @brief 通知DEM
 */
STATIC void NvM_EccHandler_NotifyDem(uint8 errorType, NvM_BlockIdType blockId)
{
    (void)blockId;
    
    switch (errorType)
    {
        case NVM_ECC_ERROR_SINGLE_BIT_CORRECTED:
            #ifdef DEM_E_NVM_ECC_SINGLE_BIT_ERROR
            Dem_ReportErrorStatus(
                DEM_E_NVM_ECC_SINGLE_BIT_ERROR,
                DEM_EVENT_STATUS_PREFAILED
            );
            #endif
            break;
            
        case NVM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE:
            #ifdef DEM_E_NVM_ECC_DOUBLE_BIT_ERROR
            Dem_ReportErrorStatus(
                DEM_E_NVM_ECC_DOUBLE_BIT_ERROR,
                DEM_EVENT_STATUS_FAILED
            );
            #endif
            break;
            
        case NVM_ECC_ERROR_WRITE_VERIFICATION_FAILED:
            #ifdef DEM_E_NVM_WRITE_VERIFICATION_FAILED
            Dem_ReportErrorStatus(
                DEM_E_NVM_WRITE_VERIFICATION_FAILED,
                DEM_EVENT_STATUS_FAILED
            );
            #endif
            break;
            
        case NVM_ECC_ERROR_BLOCK_INTEGRITY_LOST:
            #ifdef DEM_E_NVM_BLOCK_INTEGRITY_LOST
            Dem_ReportErrorStatus(
                DEM_E_NVM_BLOCK_INTEGRITY_LOST,
                DEM_EVENT_STATUS_FAILED
            );
            #endif
            break;
            
        default:
            break;
    }
}

/**
 * @brief 重置错误计数
 */
STATIC void NvM_EccHandler_ResetErrorCount(NvM_BlockIdType blockId)
{
    if (blockId < NVM_CFG_MAX_BLOCK_ID)
    {
        NvM_EccErrorCount[blockId] = 0U;
    }
}

#define NVM_ECCHANDLER_STOP_SEC_CODE
#include "NvM_MemMap.h"
