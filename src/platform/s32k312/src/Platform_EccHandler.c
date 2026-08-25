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
/* @req SHALL_S32K312 */


/**
 * @file Platform_EccHandler.c
 * @brief S32K312 ECC中断处理实现
 * 
 * 实现ECC错误的中断级别处理，包括：
 * - 单位错误纠正 (可恢复)
 * - 双位错误检测 (严重故障)
 * - NvM数据损坏处理
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Platform_EccHandler.h"
#include "Platform_RamSafety.h"
#include "RamSafety.h"
#include "Mcal.h"
#include "Det.h"
#include "Dem.h"
#include "NvM.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief MSCM寄存器访问
 */
#define MSCM_ECC_STATUS_REG             (*((volatile uint32*)0x401F0034U))
#define MSCM_ECC_ERROR_ADDR_REG         (*((volatile uint32*)0x401F0038U))
#define MSCM_ECC_ERROR_COUNT_REG        (*((volatile uint32*)0x401F003CU))
#define MSCM_ECC_INT_EN_REG             (*((volatile uint32*)0x401F0040U))
#define MSCM_ECC_INT_CLEAR_REG          (*((volatile uint32*)0x401F0044U))

/**
 * @brief ECC状态位
 */
#define ECC_STATUS_SINGLE_BIT           0x01U
#define ECC_STATUS_DOUBLE_BIT           0x02U
#define ECC_STATUS_BUS_ERROR            0x04U
#define ECC_STATUS_OVERFLOW             0x08U

/**
 * @brief 模块ID
 */
#define PLATFORM_ECCHANDLER_MODULE_ID   0x1CU

/**
 * @brief API ID
 */
#define ECCHANDLER_API_INIT             0x01U
#define ECCHANDLER_API_ISR              0x10U
#define ECCHANDLER_API_HANDLE_SINGLE    0x20U
#define ECCHANDLER_API_HANDLE_DOUBLE    0x21U

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define PLATFORM_ECCHANDLER_START_SEC_VAR_INIT_UNSPECIFIED
#include "Platform_MemMap.h"

/**
 * @brief 配置指针
 */
STATIC const Platform_EccHandlerConfigType* EccHandler_Config = NULL_PTR;

/**
 * @brief 初始化状态
 */
STATIC boolean EccHandler_Initialized = FALSE;

/**
 * @brief 错误回调
 */
STATIC Platform_EccErrorCallbackType EccHandler_Callback = NULL_PTR;

#define PLATFORM_ECCHANDLER_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Platform_MemMap.h"

#define PLATFORM_ECCHANDLER_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Platform_MemMap.h"

/**
 * @brief 错误记录数组
 */
STATIC Platform_EccErrorInfoType EccHandler_ErrorLog[PLATFORM_ECC_MAX_ERROR_LOG];

/**
 * @brief 错误记录索引
 */
STATIC uint8 EccHandler_ErrorIndex = 0U;

/**
 * @brief 错误计数
 */
STATIC uint32 EccHandler_SingleBitCount = 0U;
STATIC uint32 EccHandler_DoubleBitCount = 0U;

/**
 * @brief 当前处理策略
 */
STATIC uint8 EccHandler_CurrentPolicy = PLATFORM_ECC_HANDLER_POLICY_CORRECT | 
                                        PLATFORM_ECC_HANDLER_POLICY_NOTIFY;

#define PLATFORM_ECCHANDLER_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Platform_MemMap.h"

/*==================================================================================================
*                                       静态函数声明
==================================================================================================*/
STATIC void EccHandler_LogError(const Platform_EccErrorInfoType* errorInfo);
STATIC void EccHandler_ApplyPolicy(uint8 errorType, const Platform_EccErrorInfoType* errorInfo);
STATIC boolean EccHandler_CorrectSingleBitError(uint32 address, uint8* correctedValue);
STATIC void EccHandler_EnterSafeState(const Platform_EccErrorInfoType* errorInfo);
STATIC void EccHandler_NotifyDem(uint8 errorType, const Platform_EccErrorInfoType* errorInfo);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define PLATFORM_ECCHANDLER_START_SEC_CODE
#include "Platform_MemMap.h"

/**
 * @brief 初始化ECC处理器
 * @ASIL-D: Safety critical initialization
 */
Std_ReturnType Platform_EccHandler_Init(const Platform_EccHandlerConfigType* config)
{
    uint8 i;
    
    if (NULL_PTR == config)
    {
        #if (STD_ON == DET_ERROR_DETECT)
        (void)Det_ReportError(PLATFORM_ECCHANDLER_MODULE_ID, 0U, ECCHANDLER_API_INIT, 0x01U);
        #endif
        return E_NOT_OK;
    }
    
    if (EccHandler_Initialized)
    {
        return E_NOT_OK;
    }
    
    /* 保存配置 */
    EccHandler_Config = config;
    
    /* 清空错误记录 */
    for (i = 0U; i < PLATFORM_ECC_MAX_ERROR_LOG; i++)
    {
        EccHandler_ErrorLog[i].errorType = PLATFORM_ECC_ERROR_NONE;
        EccHandler_ErrorLog[i].errorAddress = 0U;
        EccHandler_ErrorLog[i].errorCount = 0U;
        EccHandler_ErrorLog[i].timestamp = 0U;
        EccHandler_ErrorLog[i].correctedData = 0U;
        EccHandler_ErrorLog[i].isNvMBlock = FALSE;
        EccHandler_ErrorLog[i].nvMBlockId = 0U;
    }
    
    EccHandler_ErrorIndex = 0U;
    EccHandler_SingleBitCount = 0U;
    EccHandler_DoubleBitCount = 0U;
    
    /* 设置当前策略 */
    EccHandler_CurrentPolicy = config->singleBitPolicy;
    
    /* 使能ECC中断 */
    if (config->enableInterrupt)
    {
        MSCM_ECC_INT_EN_REG = 0x03U;  /* 使能单位错误和双位错误中断 */
    }
    
    EccHandler_Initialized = TRUE;
    
    return E_OK;
}

/**
 * @brief 去初始化ECC处理器
 */
Std_ReturnType Platform_EccHandler_DeInit(void)
{
    if (EccHandler_Initialized == 0U)     {
        return E_NOT_OK;
    }
    
    /* 禁用ECC中断 */
    MSCM_ECC_INT_EN_REG = 0x00U;
    
    /* 清除状态 */
    EccHandler_Config = NULL_PTR;
    EccHandler_Initialized = FALSE;
    EccHandler_Callback = NULL_PTR;
    
    return E_OK;
}

/**
 * @brief ECC中断处理函数
 * @ASIL-D: ISR function - Critical for safety
 */
void Platform_EccHandler_Isr(void)
{
    uint32 eccStatus;
    uint32 errorAddr;
    Platform_EccErrorInfoType errorInfo;
    uint16 nvMBlockId;
    
    /* 读取ECC状态 */
    eccStatus = MSCM_ECC_STATUS_REG;
    errorAddr = MSCM_ECC_ERROR_ADDR_REG;
    
    /* 清除中断标志 */
    MSCM_ECC_INT_CLEAR_REG = eccStatus;
    
    /* 初始化错误信息 */
    errorInfo.errorAddress = errorAddr;
    errorInfo.errorCount = MSCM_ECC_ERROR_COUNT_REG;
    errorInfo.timestamp = 0U;  /* 可从RTC获取 */
    errorInfo.correctedData = 0U;
    
    /* 检查是否是NvM地址 */
    errorInfo.isNvMBlock = Platform_EccHandler_IsNvMAddress(errorAddr, &nvMBlockId);
    errorInfo.nvMBlockId = nvMBlockId;
    
    /* 处理单位错误 */
    if ((eccStatus & ECC_STATUS_SINGLE_BIT) != 0U)
    {
        errorInfo.errorType = PLATFORM_ECC_ERROR_SINGLE_BIT_CORRECTED;
        EccHandler_SingleBitCount++;
        
        /* 记录错误 */
        if ((EccHandler_Config != NULL_PTR) && (EccHandler_Config->logErrors))
        {
            EccHandler_LogError(&errorInfo);
        }
        
        /* 处理单位错误 */
        (void)Platform_EccHandler_HandleSingleBitError(&errorInfo);
        
        /* 通知DEM */
        EccHandler_NotifyDem(PLATFORM_ECC_ERROR_SINGLE_BIT_CORRECTED, &errorInfo);
        
        /* 应用策略 */
        if (EccHandler_Config != NULL_PTR)
        {
            EccHandler_ApplyPolicy(EccHandler_Config->singleBitPolicy, &errorInfo);
        }
    }
    
    /* 处理双位错误 - 更严重 */
    if ((eccStatus & ECC_STATUS_DOUBLE_BIT) != 0U)
    {
        errorInfo.errorType = PLATFORM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE;
        EccHandler_DoubleBitCount++;
        
        /* 记录错误 */
        if ((EccHandler_Config != NULL_PTR) && (EccHandler_Config->logErrors))
        {
            EccHandler_LogError(&errorInfo);
        }
        
        /* 处理双位错误 */
        Platform_EccHandler_HandleDoubleBitError(&errorInfo);
        
        /* 通知DEM */
        EccHandler_NotifyDem(PLATFORM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE, &errorInfo);
        
        /* 应用策略 - 双位错误通常需要立即处理 */
        if (EccHandler_Config != NULL_PTR)
        {
            EccHandler_ApplyPolicy(EccHandler_Config->doubleBitPolicy, &errorInfo);
        }
    }
    
    /* 处理总线错误 */
    if ((eccStatus & ECC_STATUS_BUS_ERROR) != 0U)
    {
        errorInfo.errorType = PLATFORM_ECC_ERROR_BUS_ERROR;
        
        if ((EccHandler_Config != NULL_PTR) && (EccHandler_Config->logErrors))
        {
            EccHandler_LogError(&errorInfo);
        }
        
        EccHandler_NotifyDem(PLATFORM_ECC_ERROR_BUS_ERROR, &errorInfo);
        
        if (EccHandler_Config != NULL_PTR)
        {
            EccHandler_ApplyPolicy(EccHandler_Config->busErrorPolicy, &errorInfo);
        }
    }
    
    /* 调用用户回调 */
    if (EccHandler_Callback != NULL_PTR)
    {
        EccHandler_Callback(&errorInfo);
    }
}

/**
 * @brief 注册错误回调函数
 */
Std_ReturnType Platform_EccHandler_RegisterCallback(Platform_EccErrorCallbackType callback)
{
    if (EccHandler_Initialized == 0U)     {
        return E_NOT_OK;
    }
    
    EccHandler_Callback = callback;
    return E_OK;
}

/**
 * @brief 获取错误记录
 */
Std_ReturnType Platform_EccHandler_GetErrorLog(uint8 index, Platform_EccErrorInfoType* errorInfo)
{
    if (index >= PLATFORM_ECC_MAX_ERROR_LOG)
    {
        return E_NOT_OK;
    }
    
    if (NULL_PTR == errorInfo)
    {
        return E_NOT_OK;
    }
    
    *errorInfo = EccHandler_ErrorLog[index];
    return E_OK;
}

/**
 * @brief 获取错误计数
 */
Std_ReturnType Platform_EccHandler_GetErrorCounts(uint32* singleBitCount, uint32* doubleBitCount)
{
    if (NULL_PTR != singleBitCount)
    {
        *singleBitCount = EccHandler_SingleBitCount;
    }
    
    if (NULL_PTR != doubleBitCount)
    {
        *doubleBitCount = EccHandler_DoubleBitCount;
    }
    
    return E_OK;
}

/**
 * @brief 清除错误记录
 */
Std_ReturnType Platform_EccHandler_ClearErrorLog(void)
{
    uint8 i;
    
    for (i = 0U; i < PLATFORM_ECC_MAX_ERROR_LOG; i++)
    {
        EccHandler_ErrorLog[i].errorType = PLATFORM_ECC_ERROR_NONE;
    }
    
    EccHandler_ErrorIndex = 0U;
    EccHandler_SingleBitCount = 0U;
    EccHandler_DoubleBitCount = 0U;
    
    return E_OK;
}

/**
 * @brief 处理单位错误
 * @ASIL-D: Error correction attempt
 */
Std_ReturnType Platform_EccHandler_HandleSingleBitError(const Platform_EccErrorInfoType* errorInfo)
{
    uint8 correctedValue;
    
    if (NULL_PTR == errorInfo)
    {
        return E_NOT_OK;
    }
    
    /* 尝试纠正单位错误 */
    if (EccHandler_CorrectSingleBitError(errorInfo->errorAddress, &correctedValue))
    {
        /* 纠正成功 */
        
        /* 如果是NvM块，标记为需要重写 */
        if (errorInfo->isNvMBlock)
        {
            (void)Platform_EccHandler_NotifyNvMDataCorruption(
                errorInfo->nvMBlockId, 
                errorInfo->errorAddress
            );
        }
        
        return E_OK;
    }
    
    return E_NOT_OK;
}

/**
 * @brief 处理双位错误
 * @ASIL-D: Critical error handling
 */
void Platform_EccHandler_HandleDoubleBitError(const Platform_EccErrorInfoType* errorInfo)
{
    if (NULL_PTR == errorInfo)
    {
        return;
    }
    
    /* 双位错误不可纠正，必须立即处理 */
    
    /* 1. 如果是NvM数据，标记为无效 */
    if (errorInfo->isNvMBlock)
    {
        (void)Platform_EccHandler_NotifyNvMDataCorruption(
            errorInfo->nvMBlockId,
            errorInfo->errorAddress
        );
    }
    
    /* 2. 报告给Dem */
    #ifdef DEM_E_RAM_ECC_DOUBLE_BIT_ERROR
    Dem_ReportErrorStatus(
        DEM_E_RAM_ECC_DOUBLE_BIT_ERROR,
        DEM_EVENT_STATUS_FAILED
    );
    #endif
    
    /* 3. 如果配置了安全状态，立即进入 */
    if ((EccHandler_CurrentPolicy & PLATFORM_ECC_HANDLER_POLICY_SAFE_STATE) != 0U)
    {
        EccHandler_EnterSafeState(errorInfo);
    }
    
    /* 4. 如果配置了复位，触发复位 */
    if ((EccHandler_CurrentPolicy & PLATFORM_ECC_HANDLER_POLICY_RESET) != 0U)
    {
        Mcal_ResetSystem();
    }
}

/**
 * @brief 检查地址是否属于NvM块
 */
boolean Platform_EccHandler_IsNvMAddress(uint32 address, uint16* blockId)
{
    /* 
     * 查询NvM配置，检查地址是否在NvM块范围内
     * 这需要NvM提供地址映射表
     */
    
    /* 示例: 检查常见的NvM存储区域 */
    /* S32K312的NvM通常存储在: */
    #define NVM_STORAGE_START   0x20400000U  /* SRAM区域 */
    #define NVM_STORAGE_END     0x20480000U
    
    if ((address >= NVM_STORAGE_START) && (address < NVM_STORAGE_END))
    {
        /* 计算块ID (简化示例) */
        if (NULL_PTR != blockId)
        {
            *blockId = (uint16)((address - NVM_STORAGE_START) / 1024U);
        }
        return TRUE;
    }
    
    if (NULL_PTR != blockId)
    {
        *blockId = 0xFFFFU;
    }
    return FALSE;
}

/**
 * @brief 通知NvM数据损坏
 */
Std_ReturnType Platform_EccHandler_NotifyNvMDataCorruption(uint16 blockId, uint32 address)
{
    /* 调用NvM的API标记块为损坏 */
    #ifdef NVM_SET_BLOCK_PROTECTION
    /* 标记块需要重写或恢复 */
    (void)NvM_SetBlockProtection(blockId, TRUE);
    #endif
    
    /* 记录NvM特定的错误 */
    #ifdef DEM_E_NVM_DATA_CORRUPTION
    Dem_ReportErrorStatus(
        DEM_E_NVM_DATA_CORRUPTION,
        DEM_EVENT_STATUS_FAILED
    );
    #endif
    
    (void)blockId;
    (void)address;
    
    return E_OK;
}

/**
 * @brief 设置错误处理策略
 */
Std_ReturnType Platform_EccHandler_SetPolicy(uint8 errorType, uint8 policy)
{
    if (EccHandler_Initialized == 0U)     {
        return E_NOT_OK;
    }
    
    /* 更新策略 */
    switch (errorType)
    {
        case PLATFORM_ECC_ERROR_SINGLE_BIT_CORRECTED:
            if (EccHandler_Config != NULL_PTR)
            {
                ((Platform_EccHandlerConfigType*)EccHandler_Config)->singleBitPolicy = policy;
            }
            break;
            
        case PLATFORM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE:
            if (EccHandler_Config != NULL_PTR)
            {
                ((Platform_EccHandlerConfigType*)EccHandler_Config)->doubleBitPolicy = policy;
            }
            break;
            
        case PLATFORM_ECC_ERROR_BUS_ERROR:
            if (EccHandler_Config != NULL_PTR)
            {
                ((Platform_EccHandlerConfigType*)EccHandler_Config)->busErrorPolicy = policy;
            }
            break;
            
        default:
            return E_NOT_OK;
    }
    
    return E_OK;
}

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief 记录错误到日志
 */
STATIC void EccHandler_LogError(const Platform_EccErrorInfoType* errorInfo)
{
    if (NULL_PTR == errorInfo)
    {
        return;
    }
    
    /* 保存到环形缓冲区 */
    EccHandler_ErrorLog[EccHandler_ErrorIndex] = *errorInfo;
    EccHandler_ErrorIndex++;
    
    if (EccHandler_ErrorIndex >= PLATFORM_ECC_MAX_ERROR_LOG)
    {
        EccHandler_ErrorIndex = 0U;
    }
}

/**
 * @brief 应用错误处理策略
 */
STATIC void EccHandler_ApplyPolicy(uint8 policy, const Platform_EccErrorInfoType* errorInfo)
{
    (void)errorInfo;
    
    /* 忽略 - 仅记录 */
    if ((policy & PLATFORM_ECC_HANDLER_POLICY_IGNORE) != 0U)
    {
        return;
    }
    
    /* 通知 */
    if ((policy & PLATFORM_ECC_HANDLER_POLICY_NOTIFY) != 0U)
    {
        /* 可调用应用层回调 */
    }
    
    /* 安全状态 */
    if ((policy & PLATFORM_ECC_HANDLER_POLICY_SAFE_STATE) != 0U)
    {
        EccHandler_EnterSafeState(errorInfo);
    }
    
    /* 复位 */
    if ((policy & PLATFORM_ECC_HANDLER_POLICY_RESET) != 0U)
    {
        Mcal_ResetSystem();
    }
}

/**
 * @brief 纠正单位错误
 * @note S32K312的ECC硬件自动纠正单位错误，
 *       软件只需要读取数据即可
 */
STATIC boolean EccHandler_CorrectSingleBitError(uint32 address, uint8* correctedValue)
{
    volatile uint8* ptr;
    uint8 value;
    
    if (correctedValue == NULL_PTR)
    {
        return FALSE;
    }
    
    /* 读取数据 - 硬件自动纠正 */
    ptr = (volatile uint8*)(uintptr)address;
    value = *ptr;
    *correctedValue = value;
    
    /* 重写纠正后的数据 */
    *ptr = value;
    
    return TRUE;
}

/**
 * @brief 进入安全状态
 */
STATIC void EccHandler_EnterSafeState(const Platform_EccErrorInfoType* errorInfo)
{
    /* 禁用中断 */
    Mcal_DisableAllInterrupts();
    
    /* 通知FCCU */
    #ifdef PLATFORM_FCCU_FAULT_RAM_ECC
    Platform_Fccu_NonFaultyFault(PLATFORM_FCCU_FAULT_RAM_ECC);
    #endif
    
    /* 通知RamSafety模块 */
    RamSafety_EnterSafeState(errorInfo->errorType);
    
    /* 进入无限循环或安全状态 */
    while (1)
    {
        /* 可添加看门狗喂狗或等待复位 */
    }
}

/**
 * @brief 通知DEM
 */
STATIC void EccHandler_NotifyDem(uint8 errorType, const Platform_EccErrorInfoType* errorInfo)
{
    (void)errorInfo;
    
    switch (errorType)
    {
        case PLATFORM_ECC_ERROR_SINGLE_BIT_CORRECTED:
            #ifdef DEM_E_RAM_ECC_SINGLE_BIT_ERROR
            Dem_ReportErrorStatus(
                DEM_E_RAM_ECC_SINGLE_BIT_ERROR,
                DEM_EVENT_STATUS_PREFAILED
            );
            #endif
            break;
            
        case PLATFORM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE:
            #ifdef DEM_E_RAM_ECC_DOUBLE_BIT_ERROR
            Dem_ReportErrorStatus(
                DEM_E_RAM_ECC_DOUBLE_BIT_ERROR,
                DEM_EVENT_STATUS_FAILED
            );
            #endif
            break;
            
        case PLATFORM_ECC_ERROR_BUS_ERROR:
            #ifdef DEM_E_RAM_BUS_ERROR
            Dem_ReportErrorStatus(
                DEM_E_RAM_BUS_ERROR,
                DEM_EVENT_STATUS_FAILED
            );
            #endif
            break;
            
        default:
            break;
    }
}

#define PLATFORM_ECCHANDLER_STOP_SEC_CODE
#include "Platform_MemMap.h"
