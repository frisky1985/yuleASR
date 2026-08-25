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
/* @req SWS_RamSafety_00001 @req SWS_RamSafety_00002 @req SWS_RamSafety_00003 */


/**
 * @file RamSafety_Cfg.c
 * @brief RamSafety模块配置实现
 * 
 * 针对S32K312平台的RAM安全检查配置数据
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "RamSafety_Cfg.h"

/*==================================================================================================
*                                       配置数据实现
==================================================================================================*/
#define RAMSAFETY_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "RamSafety_MemMap.h"

/**
 * @brief RAM区域配置数组
 * @ASIL-D: Safety-critical memory region configuration
 */
const RamSafety_RegionType RamSafety_RegionConfig[RAMSAFETY_CFG_NUM_REGIONS] =
{
    /* 区域0: DTCM - 高优先级，启动和运行时都检查 */
    {
        .startAddress = RAMSAFETY_ADDR_DTCM_BASE,
        .size = RAMSAFETY_ADDR_DTCM_SIZE,
        .priority = 255U,                    /* 最高优先级 */
        .startupTest = TRUE,                 /* 启动时检查 */
        .runtimeTest = TRUE,                 /* 运行时检查 */
        .eccEnabled = TRUE,                  /* 硬件ECC使能 */
        .crcSeed = 0xFFFFFFFFU               /* CRC初始值 */
    },
    
    /* 区域1: 主SRAM - 中等优先级，启动检查 */
    {
        .startAddress = RAMSAFETY_ADDR_SRAM_BASE,
        .size = RAMSAFETY_ADDR_SRAM_SIZE,
        .priority = 200U,                    /* 高优先级 */
        .startupTest = TRUE,                 /* 启动时检查 */
        .runtimeTest = TRUE,                 /* 运行时检查 */
        .eccEnabled = TRUE,                  /* 硬件ECC使能 */
        .crcSeed = 0xA55A3CC3U               /* CRC初始值 */
    },
    
    /* 区域2: FlexRAM - 中等优先级，运行时检查 */
    {
        .startAddress = RAMSAFETY_ADDR_FLEXRAM_BASE,
        .size = RAMSAFETY_ADDR_FLEXRAM_SIZE,
        .priority = 150U,                    /* 中等优先级 */
        .startupTest = TRUE,                 /* 启动时检查 */
        .runtimeTest = TRUE,                 /* 运行时检查 */
        .eccEnabled = TRUE,                  /* 硬件ECC使能 */
        .crcSeed = 0x5AA5C33CU               /* CRC初始值 */
    },
    
    /* 区域3: 栈区域 - 低优先级，仅启动时检查 */
    {
        .startAddress = RAMSAFETY_ADDR_STACK_BASE,
        .size = RAMSAFETY_ADDR_STACK_SIZE,
        .priority = 100U,                    /* 低优先级 */
        .startupTest = TRUE,                 /* 启动时检查 */
        .runtimeTest = FALSE,                /* 运行时不检查 (被使用中) */
        .eccEnabled = TRUE,                  /* 硬件ECC使能 */
        .crcSeed = 0x12345678U               /* CRC初始值 */
    }
};

/**
 * @brief RamSafety主配置结构
 */
const RamSafety_ConfigType RamSafety_Config =
{
    .regions = RamSafety_RegionConfig,
    .numRegions = RAMSAFETY_CFG_NUM_REGIONS,
    .runtimePeriodMs = RAMSAFETY_CFG_RUNTIME_PERIOD_MS,
    .useHardwareEcc = RAMSAFETY_CFG_HARDWARE_ECC,
    .maxRuntimeRegionsPerCycle = RAMSAFETY_CFG_MAX_REGIONS_PER_CYCLE
};

/**
 * @brief 调试配置 (减少检查范围，加快启动速度)
 */
const RamSafety_ConfigType RamSafety_ConfigDebug =
{
    .regions = RamSafety_RegionConfig,
    .numRegions = 1U,                        /* 只检查DTCM */
    .runtimePeriodMs = 50U,                  /* 更快的运行时周期 */
    .useHardwareEcc = RAMSAFETY_CFG_HARDWARE_ECC,
    .maxRuntimeRegionsPerCycle = 1U
};

#define RAMSAFETY_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "RamSafety_MemMap.h"
