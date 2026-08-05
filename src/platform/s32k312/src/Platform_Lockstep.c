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
 * @file Platform_Lockstep.c
 * @brief S32K312平台Lockstep硬件实现
 * 
 * S32K312的Cortex-M7锁步功能实现
 * 通过MSCM (Memory and Security Control Module) 控制Lockstep
 * 
 * @ASIL-D Safety Level
 * @hardware S32K312
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Platform_Lockstep.h"
#include "Mcal.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 软件版本号
 */
#define PLATFORM_LOCKSTEP_SW_VERSION            0x010000U

/**
 * @brief 寄存器访问宏 (REG_READ32/REG_WRITE32 由 Std_Types.h 提供, 含 uintptr 转换)
 */
#ifndef REG_RMW32
#define REG_RMW32(address, mask, value)         \
    REG_WRITE32((address), (REG_READ32(address) & ~(mask)) | ((value) & (mask)))
#endif

/**
 * @brief 时序宏
 */
#define PLATFORM_LOCKSTEP_NOP()                 __asm__ volatile ("nop")
#if defined(__aarch64__)
#define PLATFORM_LOCKSTEP_DSB()                 __asm__ volatile ("dsb sy" ::: "memory")
#define PLATFORM_LOCKSTEP_ISB()                 __asm__ volatile ("isb sy" ::: "memory")
#elif defined(__arm__) || defined(__thumb__) || defined(__ARM_ARCH)
#define PLATFORM_LOCKSTEP_DSB()                 __asm__ volatile ("dsb" ::: "memory")
#define PLATFORM_LOCKSTEP_ISB()                 __asm__ volatile ("isb" ::: "memory")
#else
/* 非 ARM 宿主 (x86 CI/单测): 用编译器屏障代替, 保证可移植编译 */
#define PLATFORM_LOCKSTEP_DSB()                 __asm__ volatile ("" ::: "memory")
#define PLATFORM_LOCKSTEP_ISB()                 __asm__ volatile ("" ::: "memory")
#endif

/**
 * @brief 超时定义
 */
#define BIST_TIMEOUT_DEFAULT_US                 10000U      /* 10ms */
#define LOCKSTEP_SWITCH_DELAY_US                100U        /* 100us */

/**
 * @brief CRC32多项式
 */
#define CRC32_POLYNOMIAL                        0x04C11DB7U

/**
 * @brief FCCU密钥
 */
#define FCCU_CTRLK_KEY                          0x913756B9U
#define FCCU_NCFK_KEY                           0xAB3498DFU

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define PLATFORM_LOCKSTEP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Platform_Lockstep_MemMap.h"

/**
 * @brief 初始化状态
 */
STATIC boolean Platform_Lockstep_Initialized = FALSE;

/**
 * @brief 当前BIST状态
 */
STATIC uint8 Platform_BistStatus = BIST_STATUS_IDLE;

/**
 * @brief BIST结果
 */
STATIC uint32 Platform_BistResults = 0U;

#define PLATFORM_LOCKSTEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Platform_Lockstep_MemMap.h"

/*==================================================================================================
*                                       静态函数宣告
==================================================================================================*/
STATIC void Platform_Lockstep_DelayUs(uint32 microseconds);
STATIC Std_ReturnType Platform_Lockstep_WaitBistComplete(uint32 timeoutUs);
STATIC uint32 Platform_Crc32_Calculate(const uint8* data, uint32 length, uint32 seed);
STATIC void Platform_Fccu_Init(void);
STATIC void Platform_Fccu_SetSafeState(void);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define PLATFORM_LOCKSTEP_START_SEC_CODE
#include "Platform_Lockstep_MemMap.h"

/**
 * @brief 初始化平台Lockstep
 * @ASIL-D: Hardware initialization with error checking
 */
Std_ReturnType Platform_Lockstep_Init(const Lockstep_ConfigType* config)
{
    uint32 regValue;
    Std_ReturnType result = E_NOT_OK;
    
    if (NULL_PTR == config)
    {
        return E_NOT_OK;
    }
    
    /* 禁用中断 */
    Mcal_DisableAllInterrupts();
    
    /* 检查复位原因 */
    uint32 resetReason = MC_RGM->DES;
    if (resetReason & MC_RGM_DES_F_LOCKSTEP)
    {
        /* 上次复位是由于Lockstep错误，需要处理 */
        /* 记录或报告此事件 */
    }
    
    /* 配置Lockstep控制寄存器 */
    regValue = 0U;
    
    if (config->enableLockstep && (LOCKSTEP_MODE_ENABLED == config->mode))
    {
        /* 使能锁步模式 */
        regValue |= MSCM_LOCKSTEP_CTRL_ENABLE;
        regValue |= (1U << MSCM_LOCKSTEP_CTRL_MODE_SHIFT);  /* Lockstep模式 */
    }
    else
    {
        /* 分离模式 (调试) */
        regValue |= (0U << MSCM_LOCKSTEP_CTRL_MODE_SHIFT);
    }
    
    if (config->enableEout)
    {
        /* 使能错误输出 */
        regValue |= MSCM_LOCKSTEP_CTRL_EOUT_EN;
    }
    
    /* 写入配置 */
    MSCM->LOCKSTEP_CTRL = regValue;
    
    /* 数据同步屏障 */
    PLATFORM_LOCKSTEP_DSB();
    PLATFORM_LOCKSTEP_ISB();
    
    /* 延时等待配置生效 */
    Platform_Lockstep_DelayUs(LOCKSTEP_SWITCH_DELAY_US);
    
    /* 验证配置 */
    uint32 actualValue = MSCM->LOCKSTEP_CTRL;
    if ((actualValue & MSCM_LOCKSTEP_CTRL_ENABLE) == (regValue & MSCM_LOCKSTEP_CTRL_ENABLE))
    {
        result = E_OK;
    }
    
    /* 初始化FCCU */
    Platform_Fccu_Init();
    
    /* 运行BIST (如果使能) */
    if (config->enableBist && (result == E_OK))
    {
        result = Platform_Lockstep_RunBist(config->bistTimeoutUs);
    }
    
    /* 恢复中断 */
    Mcal_EnableAllInterrupts();
    
    if (E_OK == result)
    {
        Platform_Lockstep_Initialized = TRUE;
    }
    
    return result;
}

/**
 * @brief 去初始化平台Lockstep
 */
void Platform_Lockstep_DeInit(void)
{
    /* 禁用Lockstep */
    MSCM->LOCKSTEP_CTRL = 0U;
    
    PLATFORM_LOCKSTEP_DSB();
    
    Platform_Lockstep_Initialized = FALSE;
    Platform_BistStatus = BIST_STATUS_IDLE;
    Platform_BistResults = 0U;
}

/**
 * @brief 设置Lockstep模式
 */
Std_ReturnType Platform_Lockstep_SetMode(Lockstep_ModeType mode)
{
    uint32 regValue;
    Std_ReturnType result = E_NOT_OK;
    
    if (Platform_Lockstep_Initialized == 0U)     {
        return E_NOT_OK;
    }
    
    /* 读取当前配置 */
    regValue = MSCM->LOCKSTEP_CTRL;
    
    /* 清除模式位 */
    regValue &= ~MSCM_LOCKSTEP_CTRL_MODE_MASK;
    
    switch (mode)
    {
        case LOCKSTEP_MODE_ENABLED:
            regValue |= MSCM_LOCKSTEP_CTRL_ENABLE;
            regValue |= (1U << MSCM_LOCKSTEP_CTRL_MODE_SHIFT);
            break;
            
        case LOCKSTEP_MODE_DISABLED:
            regValue &= ~MSCM_LOCKSTEP_CTRL_ENABLE;
            break;
            
        case LOCKSTEP_MODE_DEBUG:
            regValue |= MSCM_LOCKSTEP_CTRL_ENABLE;
            regValue |= (0U << MSCM_LOCKSTEP_CTRL_MODE_SHIFT);  /* Split模式 */
            break;
            
        default:
            return E_NOT_OK;
    }
    
    /* 写入配置 */
    MSCM->LOCKSTEP_CTRL = regValue;
    PLATFORM_LOCKSTEP_DSB();
    
    /* 等待生效 */
    Platform_Lockstep_DelayUs(LOCKSTEP_SWITCH_DELAY_US);
    
    /* 验证 */
    uint32 actualValue = MSCM->LOCKSTEP_CTRL;
    if ((actualValue & MSCM_LOCKSTEP_CTRL_MODE_MASK) == (regValue & MSCM_LOCKSTEP_CTRL_MODE_MASK))
    {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief 获取当前Lockstep状态
 */
Std_ReturnType Platform_Lockstep_GetStatus(boolean* isActive, boolean* hasError)
{
    uint32 status;
    
    if ((NULL_PTR == isActive) || (NULL_PTR == hasError))
    {
        return E_NOT_OK;
    }
    
    if (Platform_Lockstep_Initialized == 0U)     {
        *isActive = FALSE;
        *hasError = FALSE;
        return E_NOT_OK;
    }
    
    status = MSCM->LOCKSTEP_STATUS;
    
    *isActive = (status & MSCM_LOCKSTEP_STATUS_ACTIVE) != 0U;
    *hasError = (status & MSCM_LOCKSTEP_STATUS_ERROR) != 0U;
    
    return E_OK;
}

/**
 * @brief 检查Lockstep状态
 */
Std_ReturnType Platform_Lockstep_CheckStatus(boolean* mismatchDetected)
{
    uint32 status;
    
    if (NULL_PTR == mismatchDetected)
    {
        return E_NOT_OK;
    }
    
    if (Platform_Lockstep_Initialized == 0U)     {
        *mismatchDetected = FALSE;
        return E_NOT_OK;
    }
    
    status = MSCM->LOCKSTEP_STATUS;
    
    /* 检查是否检测到不匹配 */
    if (status & MSCM_LOCKSTEP_STATUS_MISMATCH)
    {
        *mismatchDetected = TRUE;
        
        /* 通知FCCU */
        Platform_Fccu_SetSafeState();
        
        return E_NOT_OK;
    }
    
    /* 检查是否有错误 */
    if (status & MSCM_LOCKSTEP_STATUS_ERROR)
    {
        *mismatchDetected = TRUE;
        return E_NOT_OK;
    }
    
    *mismatchDetected = FALSE;
    return E_OK;
}

/**
 * @brief 运行BIST
 * @ASIL-D: Built-in self test
 */
Std_ReturnType Platform_Lockstep_RunBist(uint32 timeoutUs)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 timeout = (timeoutUs == 0U) ? BIST_TIMEOUT_DEFAULT_US : timeoutUs;
    
    if (Platform_Lockstep_Initialized == 0U)     {
        return E_NOT_OK;
    }
    
    /* 检查是否BIST正在运行 */
    if (Platform_BistStatus == BIST_STATUS_RUNNING)
    {
        return E_NOT_OK;
    }
    
    /* 清除之前的BIST状态 */
    Platform_BistStatus = BIST_STATUS_IDLE;
    Platform_BistResults = 0U;
    
    /* 启动BIST */
    MSCM->BIST_CTRL = MSCM_LOCKSTEP_CTRL_BIST_EN;
    PLATFORM_LOCKSTEP_DSB();
    
    Platform_BistStatus = BIST_STATUS_RUNNING;
    
    /* 等待BIST完成 */
    result = Platform_Lockstep_WaitBistComplete(timeout);
    
    return result;
}

/**
 * @brief 获取BIST结果
 */
Std_ReturnType Platform_Lockstep_GetBistResult(uint32* results)
{
    if (NULL_PTR == results)
    {
        return E_NOT_OK;
    }
    
    *results = Platform_BistResults;
    
    /* 检查BIST状态 */
    if (Platform_BistStatus == BIST_STATUS_COMPLETE_PASS)
    {
        return E_OK;
    }
    else if (Platform_BistStatus == BIST_STATUS_COMPLETE_FAIL)
    {
        return E_NOT_OK;
    }
    else
    {
        /* BIST还未完成 */
        return E_NOT_OK;
    }
}

/**
 * @brief 清除Lockstep错误状态
 */
Std_ReturnType Platform_Lockstep_ClearError(void)
{
    if (Platform_Lockstep_Initialized == 0U)     {
        return E_NOT_OK;
    }
    
    /* 写入清除位 (位2) */
    MSCM->LOCKSTEP_STATUS = 0x00000004U;
    PLATFORM_LOCKSTEP_DSB();
    
    /* 验证错误是否清除 */
    uint32 status = MSCM->LOCKSTEP_STATUS;
    if ((status & (MSCM_LOCKSTEP_STATUS_ERROR | MSCM_LOCKSTEP_STATUS_MISMATCH)) == 0U)
    {
        return E_OK;
    }
    
    return E_NOT_OK;
}

/**
 * @brief 获取复位原因
 */
Std_ReturnType Platform_Lockstep_GetResetReason(uint32* resetReason)
{
    if (NULL_PTR == resetReason)
    {
        return E_NOT_OK;
    }
    
    *resetReason = MC_RGM->DES;
    
    return E_OK;
}

/**
 * @brief 触发系统复位
 */
void Platform_System_Reset(uint8 resetType)
{
    (void)resetType;  /* S32K312只支持一种复位 */
    
    /* 通过MC_RGM触发复位 */
    MC_RGM->CTRL = 0x01U;  /* 软件复位 */
    
    /* 等待复位 */
    while (1)
    {
        PLATFORM_LOCKSTEP_NOP();
    }
}

/**
 * @brief 请求安全状态
 */
void Platform_SafeState_Request(uint32 reason)
{
    (void)reason;
    
    /* 设置FCCU安全状态 */
    Platform_Fccu_SetSafeState();
    
    /* 禁用全局中断 */
    Mcal_DisableAllInterrupts();
    
    /* 等待复位 */
    while (1)
    {
        PLATFORM_LOCKSTEP_NOP();
    }
}

/**
 * @brief 计算CRC32
 */
Std_ReturnType Platform_Crc_Calculate(
    const uint8* data,
    uint32 length,
    uint32 seed,
    uint32* result)
{
    if ((NULL_PTR == data) || (NULL_PTR == result) || (length == 0U))
    {
        return E_NOT_OK;
    }
    
    *result = Platform_Crc32_Calculate(data, length, seed);
    
    return E_OK;
}

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief 延时函数
 * 基于系统时钟 (80MHz)
 */
STATIC void Platform_Lockstep_DelayUs(uint32 microseconds)
{
    /* 简单延时 - 假设80MHz时钟 */
    volatile uint32 count = microseconds * 80U;
    while (count--)
    {
        PLATFORM_LOCKSTEP_NOP();
    }
}

/**
 * @brief 等待BIST完成
 */
STATIC Std_ReturnType Platform_Lockstep_WaitBistComplete(uint32 timeoutUs)
{
    volatile uint32 timeout = timeoutUs * 10U;  /* 粗略计数 */
    
    while (timeout > 0U)
    {
        uint32 status = MSCM->BIST_STATUS;
        
        if (status & MSCM_LOCKSTEP_STATUS_BIST_DONE)
        {
            /* BIST完成 */
            Platform_BistResults = status;
            
            if (status & MSCM_LOCKSTEP_STATUS_BIST_FAIL)
            {
                Platform_BistStatus = BIST_STATUS_COMPLETE_FAIL;
                return E_NOT_OK;
            }
            else
            {
                Platform_BistStatus = BIST_STATUS_COMPLETE_PASS;
                return E_OK;
            }
        }
        
        timeout--;
    }
    
    /* 超时 */
    Platform_BistStatus = BIST_STATUS_IDLE;
    return E_NOT_OK;
}

/**
 * @brief CRC32计算 (软件实现)
 */
STATIC uint32 Platform_Crc32_Calculate(const uint8* data, uint32 length, uint32 seed)
{
    uint32 crc = ~seed;
    uint32 i, j;
    
    for (i = 0U; i < length; i++)
    {
        crc ^= ((uint32)data[i] << 24U);
        
        for (j = 0U; j < 8U; j++)
        {
            if (crc & 0x80000000U)
            {
                crc = (crc << 1U) ^ CRC32_POLYNOMIAL;
            }
            else
            {
                crc <<= 1U;
            }
        }
    }
    
    return ~crc;
}

/**
 * @brief 初始化FCCU
 */
STATIC void Platform_Fccu_Init(void)
{
    /* 解锁FCCU */
    FCCU->CTRLK = FCCU_CTRLK_KEY;
    
    /* 配置FCCU模式 */
    FCCU->CFG = 0x00U;  /* 正常模式 */
    
    /* 配置Lockstep故障为关键故障 */
    /* 设置NCF_CFG0的位0为关键故障 */
    FCCU->NCF_CFG0 = 0x00000001U;
    
    /* 使能故障检测 */
    FCCU->NCF_E0 = 0x00000001U;
    
    /* 使能错误输出 */
    FCCU->EINOUT = 0x01U;
}

/**
 * @brief 设置FCCU安全状态
 */
STATIC void Platform_Fccu_SetSafeState(void)
{
    /* 触发FCCU安全状态 */
    FCCU->CTRLK = FCCU_CTRLK_KEY;
    FCCU->CTRL = 0x01U;  /* 进入安全状态 */
}

#define PLATFORM_LOCKSTEP_STOP_SEC_CODE
#include "Platform_Lockstep_MemMap.h"
