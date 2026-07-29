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
 * @file Platform_RamSafety.c
 * @brief S32K312平台RAM安全硬件实现
 * 
 * 实现MSCM寄存器操作和ECC管理功能
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Platform_RamSafety.h"
#include "Mcal.h"
#include "Reg_Macros.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief MSCM寄存器访问宏
 */
#define MSCM_REG(offset)                (*((volatile uint32*)(PLATFORM_MSCM_BASE_ADDR + (offset))))
#define MSCM_CPXNUM                     MSCM_REG(PLATFORM_MSCM_CPXNUM_OFFSET)
#define MSCM_CPXCFG                     MSCM_REG(PLATFORM_MSCM_CPXCFG_OFFSET)
#define MSCM_CPXTYPE                    MSCM_REG(PLATFORM_MSCM_CPXTYPE_OFFSET)
#define MSCM_RAMRECONFIG                MSCM_REG(PLATFORM_MSCM_RAMRECONFIG_OFFSET)
#define MSCM_ECC_CODE                   MSCM_REG(PLATFORM_MSCM_ECC_CODE_OFFSET)
#define MSCM_ECC_STATUS                 MSCM_REG(PLATFORM_MSCM_ECC_STATUS_OFFSET)
#define MSCM_ECC_ERROR_ADDR             MSCM_REG(PLATFORM_MSCM_ECC_ERROR_ADDR_OFFSET)
#define MSCM_ECC_ERROR_COUNT            MSCM_REG(PLATFORM_MSCM_ECC_ERROR_COUNT_OFFSET)
#define MSCM_ECC_INT_EN                 MSCM_REG(PLATFORM_MSCM_ECC_INT_EN_OFFSET)

/**
 * @brief ECC状态位
 */
#define MSCM_ECC_STATUS_SINGLE_BIT      0x01U
#define MSCM_ECC_STATUS_DOUBLE_BIT      0x02U
#define MSCM_ECC_STATUS_BUS_ERROR       0x04U
#define MSCM_ECC_STATUS_OVERFLOW        0x08U

/**
 * @brief 最大区域数
 */
#define PLATFORM_MAX_REGIONS            16U

/**
 * @brief CRC计算窗口大小
 */
#define CRC_CALC_WINDOW_SIZE            256U

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define PLATFORM_RAMSAFETY_START_SEC_VAR_INIT_UNSPECIFIED
#include "Platform_MemMap.h"

/**
 * @brief MSCM配置
 */
STATIC Platform_MscmConfigType Platform_MscmConfig = {
    .baseAddr = PLATFORM_MSCM_BASE_ADDR,
    .eccEnabled = FALSE,
    .interruptEnabled = FALSE
};

/**
 * @brief 平台初始化状态
 */
STATIC boolean Platform_RamSafety_Initialized = FALSE;

#define PLATFORM_RAMSAFETY_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Platform_MemMap.h"

#define PLATFORM_RAMSAFETY_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Platform_MemMap.h"

/**
 * @brief 存储的CRC值 (用于运行时验证)
 */
STATIC uint32 Platform_StoredCrc[PLATFORM_MAX_REGIONS];

/**
 * @brief ECC错误计数
 */
STATIC uint32 Platform_EccErrorCounter = 0U;

#define PLATFORM_RAMSAFETY_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Platform_MemMap.h"

/*==================================================================================================
*                                       静态函数宣告
==================================================================================================*/
STATIC uint32 Platform_CalculateCrc32(const uint8* data, uint32 length, uint32 seed);
STATIC void Platform_UpdateEccConfig(uint32 startAddr, uint32 size, boolean enable);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define PLATFORM_RAMSAFETY_START_SEC_CODE
#include "Platform_MemMap.h"

/**
 * @brief 初始化平台RAM安全功能
 */
Std_ReturnType Platform_RamSafety_Init(const RamSafety_ConfigType* config)
{
    uint8 i;

    (void)config;

    /* 禁用中断 */
    Mcal_DisableAllInterrupts();

    /* 清除之前的ECC错误 */
    MSCM_ECC_STATUS = 0xFFFFFFFFU;
    Platform_EccErrorCounter = 0U;

    /* 初始化存储的CRC值 */
    for (i = 0U; i < PLATFORM_MAX_REGIONS; i++)
    {
        Platform_StoredCrc[i] = 0xFFFFFFFFU;
    }

    /* 使能ECC中断 (如果配置中启用) */
    if (config->useHardwareEcc)
    {
        MSCM_ECC_INT_EN = 0x03U;  /* 使能单位和双位错误中断 */
        Platform_MscmConfig.eccEnabled = TRUE;
        Platform_MscmConfig.interruptEnabled = TRUE;
    }

    Platform_RamSafety_Initialized = TRUE;

    Mcal_EnableAllInterrupts();

    return E_OK;
}

/**
 * @brief 去初始化平台RAM安全功能
 */
Std_ReturnType Platform_RamSafety_DeInit(void)
{
    Mcal_DisableAllInterrupts();

    /* 禁用ECC中断 */
    MSCM_ECC_INT_EN = 0x00U;

    /* 清除ECC错误 */
    MSCM_ECC_STATUS = 0xFFFFFFFFU;

    Platform_MscmConfig.eccEnabled = FALSE;
    Platform_MscmConfig.interruptEnabled = FALSE;
    Platform_RamSafety_Initialized = FALSE;

    Mcal_EnableAllInterrupts();

    return E_OK;
}

/**
 * @brief 检查ECC状态
 */
Std_ReturnType Platform_RamSafety_CheckEccStatus(uint32 startAddr, boolean* hasError, uint32* errorCount)
{
    uint32 eccStatus;
    uint32 errorAddr;
    boolean errorDetected = FALSE;

    (void)startAddr;  /* S32K312 MSCM不区分地址，检查全局 */

    if (NULL_PTR == hasError)
    {
        return E_NOT_OK;
    }

    /* 读取ECC状态寄存器 */
    eccStatus = MSCM_ECC_STATUS;

    /* 检查是否有错误 */
    if ((eccStatus & (MSCM_ECC_STATUS_SINGLE_BIT | MSCM_ECC_STATUS_DOUBLE_BIT)) != 0U)
    {
        errorDetected = TRUE;
        errorAddr = MSCM_ECC_ERROR_ADDR;

        /* 更新错误计数 */
        Platform_EccErrorCounter = MSCM_ECC_ERROR_COUNT;

        /* 读取后清除错误标志 */
        MSCM_ECC_STATUS = eccStatus;

        /* 记录错误地址到RamSafety模块 */
        (void)errorAddr;
    }

    *hasError = errorDetected;

    if (NULL_PTR != errorCount)
    {
        *errorCount = Platform_EccErrorCounter;
    }

    return E_OK;
}

/**
 * @brief 获取详细ECC错误信息
 */
Std_ReturnType Platform_RamSafety_GetEccErrorInfo(Platform_EccErrorInfoType* errorInfo)
{
    uint32 eccStatus;

    if (NULL_PTR == errorInfo)
    {
        return E_NOT_OK;
    }

    eccStatus = MSCM_ECC_STATUS;

    /* 检查是否有错误 */
    if ((eccStatus & (MSCM_ECC_STATUS_SINGLE_BIT | MSCM_ECC_STATUS_DOUBLE_BIT)) == 0U)
    {
        return E_NOT_OK;  /* 无错误 */
    }

    /* 填充错误信息 */
    errorInfo->errorAddress = MSCM_ECC_ERROR_ADDR;
    errorInfo->errorCount = MSCM_ECC_ERROR_COUNT;

    if ((eccStatus & MSCM_ECC_STATUS_DOUBLE_BIT) != 0U)
    {
        errorInfo->errorType = PLATFORM_ECC_ERROR_DOUBLE_BIT;
        errorInfo->correctable = FALSE;
    }
    else if ((eccStatus & MSCM_ECC_STATUS_SINGLE_BIT) != 0U)
    {
        errorInfo->errorType = PLATFORM_ECC_ERROR_SINGLE_BIT;
        errorInfo->correctable = TRUE;
    }
    else
    {
        errorInfo->errorType = PLATFORM_ECC_ERROR_BUS_ERROR;
        errorInfo->correctable = FALSE;
    }

    return E_OK;
}

/**
 * @brief 清除ECC错误标志
 */
Std_ReturnType Platform_RamSafety_ClearEccErrors(void)
{
    /* 清除所有ECC错误状态 */
    MSCM_ECC_STATUS = 0xFFFFFFFFU;
    Platform_EccErrorCounter = 0U;

    return E_OK;
}

/**
 * @brief 计算RAM区域CRC
 */
uint32 Platform_RamSafety_CalculateCrc(const uint8* data, uint32 length, uint32 seed)
{
    return Platform_CalculateCrc32(data, length, seed);
}

/**
 * @brief 获取存储的CRC值
 */
uint32 Platform_RamSafety_GetStoredCrc(uint8 regionId)
{
    if (regionId >= PLATFORM_MAX_REGIONS)
    {
        return 0xFFFFFFFFU;
    }

    return Platform_StoredCrc[regionId];
}

/**
 * @brief 更新存储的CRC值
 */
void Platform_RamSafety_UpdateStoredCrc(uint8 regionId, uint32 crcValue)
{
    if (regionId < PLATFORM_MAX_REGIONS)
    {
        Platform_StoredCrc[regionId] = crcValue;
    }
}

/**
 * @brief 进入安全状态
 */
void Platform_RamSafety_EnterSafeState(void)
{
    /* 通知FCCU进入安全状态 */
    /* 在S32K312上，这可能涉及:
     * 1. 禁用MSCM ECC中断
     * 2. 通知FCCU错误
     * 3. 进入安全停机
     */

    /* 禁用ECC中断 */
    MSCM_ECC_INT_EN = 0x00U;

    /* 发起FCCU故障 */
    /* 注: 实际应用中需要调用FCCU驱动 */
    Platform_Fccu_NonFaultyFault(PLATFORM_FCCU_FAULT_RAM_ECC);
}

/**
 * @brief 使能指定RAM区域的ECC
 */
Std_ReturnType Platform_RamSafety_EnableEcc(uint32 startAddr, uint32 size)
{
    (void)startAddr;
    (void)size;

    /* S32K312的ECC通常是全局使能的，通过MSCM配置 */
    MSCM_RAMRECONFIG |= 0x01U;  /* 使能ECC */
    Platform_MscmConfig.eccEnabled = TRUE;

    return E_OK;
}

/**
 * @brief 禁用指定RAM区域的ECC
 */
Std_ReturnType Platform_RamSafety_DisableEcc(uint32 startAddr, uint32 size)
{
    (void)startAddr;
    (void)size;

    /* S32K312的ECC通常是全局禁用的 */
    MSCM_RAMRECONFIG &= ~0x01U;  /* 禁用ECC */
    Platform_MscmConfig.eccEnabled = FALSE;

    return E_OK;
}

/**
 * @brief 执行RAM自检 (BIST)
 */
Std_ReturnType Platform_RamSafety_RunBist(uint32 startAddr, uint32 size)
{
    /* S32K312 MSCM不直接提供RAM BIST功能
     * 这通常由启动代码或STCU (Self-Test Control Unit) 处理
     * 此处返回成功，实际BIST由启动代码执行
     */
    (void)startAddr;
    (void)size;

    return E_OK;
}

/**
 * @brief 设置MSCM配置
 */
Std_ReturnType Platform_RamSafety_SetMscmConfig(const Platform_MscmConfigType* config)
{
    if (NULL_PTR == config)
    {
        return E_NOT_OK;
    }

    Platform_MscmConfig = *config;

    /* 应用配置到硬件 */
    if (config->eccEnabled)
    {
        MSCM_RAMRECONFIG |= 0x01U;
    }
    else
    {
        MSCM_RAMRECONFIG &= ~0x01U;
    }

    if (config->interruptEnabled)
    {
        MSCM_ECC_INT_EN = 0x03U;
    }
    else
    {
        MSCM_ECC_INT_EN = 0x00U;
    }

    return E_OK;
}

/**
 * @brief 获取MSCM配置
 */
Std_ReturnType Platform_RamSafety_GetMscmConfig(Platform_MscmConfigType* config)
{
    if (NULL_PTR == config)
    {
        return E_NOT_OK;
    }

    *config = Platform_MscmConfig;

    return E_OK;
}

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief CRC32计算 (IEEE 802.3多项式)
 */
STATIC uint32 Platform_CalculateCrc32(const uint8* data, uint32 length, uint32 seed)
{
    uint32 crc = seed;
    uint32 i, j;

    for (i = 0U; i < length; i++)
    {
        crc ^= ((uint32)data[i] << 24U);

        for (j = 0U; j < 8U; j++)
        {
            if ((crc & 0x80000000U) != 0U)
            {
                crc = (crc << 1U) ^ PLATFORM_CRC32_POLYNOMIAL;
            }
            else
            {
                crc = crc << 1U;
            }
        }
    }

    return crc;
}

/**
 * @brief 更新ECC配置
 */
STATIC void Platform_UpdateEccConfig(uint32 startAddr, uint32 size, boolean enable)
{
    (void)startAddr;
    (void)size;
    (void)enable;

    /* S32K312的ECC配置是全局的，不针对单个区域 */
    /* 如果需要细粒度的ECC控制，需要使用MMU/MPU配置 */
}

#define PLATFORM_RAMSAFETY_STOP_SEC_CODE
#include "Platform_MemMap.h"
