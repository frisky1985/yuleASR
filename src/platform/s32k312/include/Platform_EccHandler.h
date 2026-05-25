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
 * @file Platform_EccHandler.h
 * @brief S32K312 ECC中断处理头文件
 * 
 * 实现ECC错误的中断级别处理，包括:
 * - 单位错误纠正 (可恢复)
 * - 双位错误检测 (严重故障)
 * - NvM数据损坏处理
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef PLATFORM_ECCHANDLER_H
#define PLATFORM_ECCHANDLER_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define PLATFORM_ECCHANDLER_VENDOR_ID                   43
#define PLATFORM_ECCHANDLER_AR_RELEASE_MAJOR_VERSION    4
#define PLATFORM_ECCHANDLER_AR_RELEASE_MINOR_VERSION    7
#define PLATFORM_ECCHANDLER_AR_RELEASE_REVISION_VERSION 0
#define PLATFORM_ECCHANDLER_SW_MAJOR_VERSION            1
#define PLATFORM_ECCHANDLER_SW_MINOR_VERSION            0
#define PLATFORM_ECCHANDLER_SW_PATCH_VERSION            0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"
#include "Platform_Types.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief ECC错误类型
 */
#define PLATFORM_ECC_ERROR_NONE                         0x00U
#define PLATFORM_ECC_ERROR_SINGLE_BIT_CORRECTED         0x01U
#define PLATFORM_ECC_ERROR_DOUBLE_BIT_UNCORRECTABLE     0x02U
#define PLATFORM_ECC_ERROR_BUS_ERROR                    0x04U
#define PLATFORM_ECC_ERROR_OVERFLOW                     0x08U
#define PLATFORM_ECC_ERROR_NVM_DATA_CORRUPTED           0x10U
#define PLATFORM_ECC_ERROR_SYSTEM_RAM_CORRUPTED         0x20U

/**
 * @brief ECC处理策略
 */
#define PLATFORM_ECC_HANDLER_POLICY_IGNORE              0x00U  /* 仅记录 */
#define PLATFORM_ECC_HANDLER_POLICY_CORRECT             0x01U  /* 尝试纠正 */
#define PLATFORM_ECC_HANDLER_POLICY_NOTIFY              0x02U  /* 通知应用层 */
#define PLATFORM_ECC_HANDLER_POLICY_SAFE_STATE          0x04U  /* 进入安全状态 */
#define PLATFORM_ECC_HANDLER_POLICY_RESET               0x08U  /* 系统复位 */

/**
 * @brief 最大错误记录数
 */
#define PLATFORM_ECC_MAX_ERROR_LOG                      16U

/**
 * @brief 连续错误阈值
 */
#define PLATFORM_ECC_SINGLE_BIT_THRESHOLD               10U     /* 10次单位错误触发警告 */
#define PLATFORM_ECC_DOUBLE_BIT_THRESHOLD               1U      /* 1次双位错误立即处理 */

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief ECC错误信息
 */
typedef struct
{
    uint8 errorType;                    /* 错误类型 */
    uint32 errorAddress;                /* 错误地址 */
    uint32 errorCount;                  /* 错误计数 */
    uint32 timestamp;                   /* 时间戳 (如果有实时时钟) */
    uint32 correctedData;               /* 纠正后的数据 (如果可纠正) */
    boolean isNvMBlock;                 /* 是否是NvM块 */
    uint16 nvMBlockId;                  /* NvM块ID (如果是NvM块) */
} Platform_EccErrorInfoType;

/**
 * @brief ECC处理策略配置
 */
typedef struct
{
    uint8 singleBitPolicy;              /* 单位错误处理策略 */
    uint8 doubleBitPolicy;              /* 双位错误处理策略 */
    uint8 busErrorPolicy;               /* 总线错误处理策略 */
    boolean enableInterrupt;            /* 使能中断 */
    boolean logErrors;                  /* 记录错误 */
    uint8 singleBitThreshold;           /* 单位错误阈值 */
} Platform_EccHandlerConfigType;

/**
 * @brief ECC错误回调函数类型
 */
typedef void (*Platform_EccErrorCallbackType)(
    const Platform_EccErrorInfoType* errorInfo
);

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define PLATFORM_ECCHANDLER_START_SEC_CODE
#include "Platform_MemMap.h"

/**
 * @brief 初始化ECC处理器
 * @ASIL-D: Safety critical initialization
 * 
 * @param config 配置指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_Init(
    const Platform_EccHandlerConfigType* config
);

/**
 * @brief 去初始化ECC处理器
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_DeInit(void);

/**
 * @brief ECC中断处理函数
 * @ASIL-D: ISR function
 * 
 * 在S32K312上连接到MSCM_ECC中断向量
 */
extern void Platform_EccHandler_Isr(void);

/**
 * @brief 注册错误回调函数
 * 
 * @param callback 回调函数
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_RegisterCallback(
    Platform_EccErrorCallbackType callback
);

/**
 * @brief 获取错误记录
 * 
 * @param index 记录索引
 * @param errorInfo 错误信息输出
 * @return E_OK: 成功, E_NOT_OK: 无效索引
 */
extern Std_ReturnType Platform_EccHandler_GetErrorLog(
    uint8 index,
    Platform_EccErrorInfoType* errorInfo
);

/**
 * @brief 获取错误计数
 * 
 * @param singleBitCount 单位错误计数输出 (可为NULL)
 * @param doubleBitCount 双位错误计数输出 (可为NULL)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_GetErrorCounts(
    uint32* singleBitCount,
    uint32* doubleBitCount
);

/**
 * @brief 清除错误记录
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_ClearErrorLog(void);

/**
 * @brief 处理单位错误 (被ISR调用)
 * @ASIL-D: Error correction
 * 
 * @param errorInfo 错误信息
 * @return E_OK: 成功纠正, E_NOT_OK: 纠正失败
 */
extern Std_ReturnType Platform_EccHandler_HandleSingleBitError(
    const Platform_EccErrorInfoType* errorInfo
);

/**
 * @brief 处理双位错误 (被ISR调用)
 * @ASIL-D: Critical error handling
 * 
 * @param errorInfo 错误信息
 */
extern void Platform_EccHandler_HandleDoubleBitError(
    const Platform_EccErrorInfoType* errorInfo
);

/**
 * @brief 检查地址是否属于NvM块
 * 
 * @param address 地址
 * @param blockId NvM块ID输出 (可为NULL)
 * @return TRUE: 是NvM块, FALSE: 不是
 */
extern boolean Platform_EccHandler_IsNvMAddress(uint32 address, uint16* blockId);

/**
 * @brief 通知NvM数据损坏
 * 
 * @param blockId NvM块ID
 * @param address 损坏地址
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_NotifyNvMDataCorruption(
    uint16 blockId,
    uint32 address
);

/**
 * @brief 设置错误处理策略
 * 
 * @param errorType 错误类型
 * @param policy 处理策略
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_EccHandler_SetPolicy(
    uint8 errorType,
    uint8 policy
);

#define PLATFORM_ECCHANDLER_STOP_SEC_CODE
#include "Platform_MemMap.h"

#endif /* PLATFORM_ECCHANDLER_H */
