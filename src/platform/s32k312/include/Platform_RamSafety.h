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
 * @file Platform_RamSafety.h
 * @brief S32K312平台RAM安全硬件抽象层
 * 
 * 提供S32K312 MSCM (Memory System Controller Module)和ECC功能的硬件抽象
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef PLATFORM_RAMSAFETY_H
#define PLATFORM_RAMSAFETY_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define PLATFORM_RAMSAFETY_VENDOR_ID                    43
#define PLATFORM_RAMSAFETY_AR_RELEASE_MAJOR_VERSION     4
#define PLATFORM_RAMSAFETY_AR_RELEASE_MINOR_VERSION     7
#define PLATFORM_RAMSAFETY_AR_RELEASE_REVISION_VERSION  0
#define PLATFORM_RAMSAFETY_SW_MAJOR_VERSION             1
#define PLATFORM_RAMSAFETY_SW_MINOR_VERSION             0
#define PLATFORM_RAMSAFETY_SW_PATCH_VERSION             0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"
#include "Platform_Types.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief MSCM基地址 (S32K312)
 */
#define PLATFORM_MSCM_BASE_ADDR                         0x401F0000U

/**
 * @brief MSCM寄存器偏移
 */
#define PLATFORM_MSCM_CPXNUM_OFFSET                     0x00U
#define PLATFORM_MSCM_CPXCFG_OFFSET                     0x04U
#define PLATFORM_MSCM_CPXTYPE_OFFSET                    0x08U
#define PLATFORM_MSCM_CP0CFG_OFFSET                     0x0CU
#define PLATFORM_MSCM_CP1CFG_OFFSET                     0x10U
#define PLATFORM_MSCM_RAMRECONFIG_OFFSET                0x20U
#define PLATFORM_MSCM_ECC_CODE_OFFSET                   0x30U
#define PLATFORM_MSCM_ECC_STATUS_OFFSET                 0x34U
#define PLATFORM_MSCM_ECC_ERROR_ADDR_OFFSET             0x38U
#define PLATFORM_MSCM_ECC_ERROR_COUNT_OFFSET            0x3CU
#define PLATFORM_MSCM_ECC_INT_EN_OFFSET                 0x40U

/**
 * @brief ECC错误类型
 */
#define PLATFORM_ECC_ERROR_NONE                         0x00U
#define PLATFORM_ECC_ERROR_SINGLE_BIT                   0x01U
#define PLATFORM_ECC_ERROR_DOUBLE_BIT                   0x02U
#define PLATFORM_ECC_ERROR_BUS_ERROR                    0x04U

/**
 * @brief RAM配置选项
 */
#define PLATFORM_RAM_CONFIG_NORMAL                      0x00U
#define PLATFORM_RAM_CONFIG_ECC_ENABLE                  0x01U
#define PLATFORM_RAM_CONFIG_SCRAMBLE_ENABLE             0x02U

/**
 * @brief CRC多项式
 */
#define PLATFORM_CRC32_POLYNOMIAL                       0x04C11DB7U

/*==================================================================================================
*                                       类型定义
==================================================================================================*/
/**
 * @brief ECC错误信息结构
 */
typedef struct
{
    uint8 errorType;                    /* 错误类型 */
    uint32 errorAddress;                /* 错误地址 */
    uint32 errorCount;                  /* 错误计数 */
    boolean correctable;                /* 是否可纠正 */
} Platform_EccErrorInfoType;

/**
 * @brief MSCM配置结构
 */
typedef struct
{
    uint32 baseAddr;                    /* MSCM基地址 */
    boolean eccEnabled;                 /* ECC使能 */
    boolean interruptEnabled;           /* 中断使能 */
} Platform_MscmConfigType;

/*==================================================================================================
*                                       外部函数声明
==================================================================================================*/
#define PLATFORM_RAMSAFETY_START_SEC_CODE
#include "Platform_MemMap.h"

/**
 * @brief 初始化平台RAM安全功能
 * @ASIL-D: Platform initialization
 * 
 * @param config RamSafety配置指针
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_Init(const RamSafety_ConfigType* config);

/**
 * @brief 去初始化平台RAM安全功能
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_DeInit(void);

/**
 * @brief 检查ECC状态
 * @ASIL-D: ECC monitoring
 * 
 * @param startAddr RAM区域起始地址
 * @param hasError 错误状态输出
 * @param errorCount 错误计数输出 (可为NULL)
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_CheckEccStatus(
    uint32 startAddr,
    boolean* hasError,
    uint32* errorCount
);

/**
 * @brief 获取详细ECC错误信息
 * 
 * @param errorInfo ECC错误信息输出
 * @return E_OK: 有错误信息, E_NOT_OK: 无错误
 */
extern Std_ReturnType Platform_RamSafety_GetEccErrorInfo(Platform_EccErrorInfoType* errorInfo);

/**
 * @brief 清除ECC错误标志
 * 
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_ClearEccErrors(void);

/**
 * @brief 计算RAM区域CRC
 * @ASIL-D: Runtime verification
 * 
 * 使用硬件CRC引擎或软件算法计算CRC
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @param seed CRC初始值
 * @return 计算的CRC值
 */
extern uint32 Platform_RamSafety_CalculateCrc(
    const uint8* data,
    uint32 length,
    uint32 seed
);

/**
 * @brief 获取存储的CRC值
 * 
 * @param regionId RAM区域ID
 * @return 存储的CRC值
 */
extern uint32 Platform_RamSafety_GetStoredCrc(uint8 regionId);

/**
 * @brief 更新存储的CRC值
 * 
 * @param regionId RAM区域ID
 * @param crcValue CRC值
 */
extern void Platform_RamSafety_UpdateStoredCrc(uint8 regionId, uint32 crcValue);

/**
 * @brief 进入安全状态
 * @ASIL-D: Safety response
 * 
 * 通知FCCU进入安全状态
 */
extern void Platform_RamSafety_EnterSafeState(void);

/**
 * @brief 使能指定RAM区域的ECC
 * 
 * @param startAddr RAM起始地址
 * @param size RAM大小
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_EnableEcc(uint32 startAddr, uint32 size);

/**
 * @brief 禁用指定RAM区域的ECC
 * 
 * @param startAddr RAM起始地址
 * @param size RAM大小
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_DisableEcc(uint32 startAddr, uint32 size);

/**
 * @brief 执行RAM自检 (BIST)
 * @ASIL-D: Built-in self-test
 * 
 * @param startAddr RAM起始地址
 * @param size RAM大小
 * @return E_OK: 通过, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_RunBist(uint32 startAddr, uint32 size);

/**
 * @brief 设置MSCM配置
 * 
 * @param config MSCM配置
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_SetMscmConfig(const Platform_MscmConfigType* config);

/**
 * @brief 获取MSCM配置
 * 
 * @param config MSCM配置输出
 * @return E_OK: 成功, E_NOT_OK: 失败
 */
extern Std_ReturnType Platform_RamSafety_GetMscmConfig(Platform_MscmConfigType* config);

#define PLATFORM_RAMSAFETY_STOP_SEC_CODE
#include "Platform_MemMap.h"

#endif /* PLATFORM_RAMSAFETY_H */
