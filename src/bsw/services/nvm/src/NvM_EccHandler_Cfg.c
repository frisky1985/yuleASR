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
 * @file NvM_EccHandler_Cfg.c
 * @brief NvM ECC处理模块配置数据
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "NvM_EccHandler_Cfg.h"

/*==================================================================================================
*                                       ROM默认值
==================================================================================================*/
#define NVM_ECCHANDLER_START_SEC_CONFIG_DATA_8
#include "NvM_MemMap.h"

/**
 * @brief Dem管理块的ROM默认值
 */
STATIC const uint8 NvM_EccRom_DemAdmin[NVM_CFG_BLOCK_DEM_ADMIN_SIZE] = {
    0x00U, 0x00U, 0x00U, 0x00U  /* 默认状态 */
};

/**
 * @brief Dem状态块的ROM默认值
 */
STATIC const uint8 NvM_EccRom_DemStatus[NVM_CFG_BLOCK_DEM_STATUS_SIZE] = {
    0x00U  /* 默认无故障 */
};

/**
 * @brief EcuM配置块的ROM默认值
 */
STATIC const uint8 NvM_EccRom_EcuMConfig[NVM_CFG_BLOCK_ECUM_CFG_SIZE] = {
    0x00U  /* 默认配置 */
};

/**
 * @brief BswM配置块的ROM默认值
 */
STATIC const uint8 NvM_EccRom_BswMConfig[NVM_CFG_BLOCK_BSWM_CFG_SIZE] = {
    0x00U  /* 默认配置 */
};

/**
 * @brief 应用数据块的ROM默认值
 */
STATIC const uint8 NvM_EccRom_AppData[NVM_CFG_BLOCK_APP_DATA_SIZE] = {
    0xFFU  /* 擦除状态 */
};

#define NVM_ECCHANDLER_STOP_SEC_CONFIG_DATA_8
#include "NvM_MemMap.h"

/*==================================================================================================
*                                       块配置表
==================================================================================================*/
#define NVM_ECCHANDLER_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "NvM_MemMap.h"

/**
 * @brief ECC块配置表
 * @ASIL-D: Safety-critical block configuration
 */
const NvM_EccBlockConfigType NvM_EccDefaultConfig[NVM_ECCHANDLER_CFG_NUM_BLOCKS] = {
    /* 块1: Dem管理数据 - 关键安全块 */
    {
        .blockId = NVM_BLOCK_ID_DEM_ADMIN,
        .enableEccCheck = TRUE,
        .enableWriteVerify = TRUE,
        .recoveryStrategy = NVM_ECC_RECOVERY_USE_ROM_DEFAULT,
        .maxRetries = NVM_ECCHANDLER_CFG_DEFAULT_RETRY_COUNT,
        .romDefaultData = NvM_EccRom_DemAdmin
    },
    
    /* 块2: Dem状态数据 */
    {
        .blockId = NVM_BLOCK_ID_DEM_STATUS,
        .enableEccCheck = TRUE,
        .enableWriteVerify = TRUE,
        .recoveryStrategy = NVM_ECC_RECOVERY_USE_ROM_DEFAULT,
        .maxRetries = NVM_ECCHANDLER_CFG_DEFAULT_RETRY_COUNT,
        .romDefaultData = NvM_EccRom_DemStatus
    },
    
    /* 块3: EcuM配置 - 使用冗余副本 */
    {
        .blockId = NVM_BLOCK_ID_ECUM_CONFIG,
        .enableEccCheck = TRUE,
        .enableWriteVerify = TRUE,
        .recoveryStrategy = NVM_ECC_RECOVERY_USE_REDUNDANT_COPY,
        .maxRetries = NVM_ECCHANDLER_CFG_DEFAULT_RETRY_COUNT,
        .romDefaultData = NvM_EccRom_EcuMConfig
    },
    
    /* 块4: BswM配置 */
    {
        .blockId = NVM_BLOCK_ID_BSWM_CONFIG,
        .enableEccCheck = TRUE,
        .enableWriteVerify = TRUE,
        .recoveryStrategy = NVM_ECC_RECOVERY_USE_ROM_DEFAULT,
        .maxRetries = NVM_ECCHANDLER_CFG_DEFAULT_RETRY_COUNT,
        .romDefaultData = NvM_EccRom_BswMConfig
    },
    
    /* 块5: 应用数据 - 标记为无效策略 */
    {
        .blockId = NVM_BLOCK_ID_APP_DATA,
        .enableEccCheck = TRUE,
        .enableWriteVerify = FALSE,
        .recoveryStrategy = NVM_ECC_RECOVERY_MARK_INVALID,
        .maxRetries = 1U,  /* 应用数据不重试 */
        .romDefaultData = NvM_EccRom_AppData
    }
};

/**
 * @brief 配置的块数量
 */
const uint16 NvM_EccNumConfiguredBlocks = NVM_ECCHANDLER_CFG_NUM_BLOCKS;

#define NVM_ECCHANDLER_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "NvM_MemMap.h"
