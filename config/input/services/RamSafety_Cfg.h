/**
 * @file RamSafety_Cfg.h
 * @brief RamSafety模块配置头文件
 * 
 * 针对S32K312平台的RAM安全检查配置
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef RAMSAFETY_CFG_H
#define RAMSAFETY_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define RAMSAFETY_CFG_VENDOR_ID                     43
#define RAMSAFETY_CFG_AR_RELEASE_MAJOR_VERSION      4
#define RAMSAFETY_CFG_AR_RELEASE_MINOR_VERSION      7
#define RAMSAFETY_CFG_AR_RELEASE_REVISION_VERSION   0
#define RAMSAFETY_CFG_SW_MAJOR_VERSION              1
#define RAMSAFETY_CFG_SW_MINOR_VERSION              0
#define RAMSAFETY_CFG_SW_PATCH_VERSION              0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "RamSafety.h"

/*==================================================================================================
*                                       配置选项
==================================================================================================*/
/**
 * @brief 开发错误检测使能
 */
#define RAMSAFETY_CFG_DEV_ERROR_DETECT              (STD_ON)

/**
 * @brief 版本信息API使能
 */
#define RAMSAFETY_CFG_VERSION_INFO_API              (STD_ON)

/**
 * @brief 硬件ECC支持 (S32K312支持ECC)
 */
#define RAMSAFETY_CFG_HARDWARE_ECC                  (STD_ON)

/**
 * @brief CRC验证使能
 */
#define RAMSAFETY_CFG_CRC_VERIFICATION              (STD_ON)

/**
 * @brief CRC多项式
 */
#define RAMSAFETY_CFG_CRC_POLYNOMIAL                0x04C11DB7U

/**
 * @brief CRC初始值
 */
#define RAMSAFETY_CFG_CRC_INITIAL                   0xFFFFFFFFU

/**
 * @brief 启动检查超时 (毫秒)
 */
#define RAMSAFETY_CFG_STARTUP_TIMEOUT_MS            5000U

/**
 * @brief 运行时检查周期 (毫秒)
 */
#define RAMSAFETY_CFG_RUNTIME_PERIOD_MS             100U

/**
 * @brief 每次运行时检查最大区域数
 */
#define RAMSAFETY_CFG_MAX_REGIONS_PER_CYCLE         1U

/**
 * @brief 最大RAM区域数量
 */
#define RAMSAFETY_CFG_NUM_REGIONS                   4U

/**
 * @brief 连续失败次数阈值
 */
#define RAMSAFETY_CFG_FAILURE_THRESHOLD             3U

/*==================================================================================================
*                                       RAM区域定义
==================================================================================================*/
/**
 * @brief S32K312 RAM地址映射
 */
#define RAMSAFETY_ADDR_DTCM_BASE                    0x20000000U  /* DTCM (Data TCM) */
#define RAMSAFETY_ADDR_DTCM_SIZE                    0x40000U     /* 256KB DTCM */

#define RAMSAFETY_ADDR_SRAM_BASE                    0x20400000U  /* 主SRAM */
#define RAMSAFETY_ADDR_SRAM_SIZE                    0x80000U     /* 512KB SRAM */

#define RAMSAFETY_ADDR_FLEXRAM_BASE                 0x20480000U  /* FlexRAM */
#define RAMSAFETY_ADDR_FLEXRAM_SIZE                 0x40000U     /* 256KB FlexRAM */

#define RAMSAFETY_ADDR_STACK_BASE                   0x2003F000U  /* 栈区域 */
#define RAMSAFETY_ADDR_STACK_SIZE                   0x1000U      /* 4KB 栈 */

/**
 * @brief RAM区域配置表
 */
/* RAM region configuration (S32K312 memory map defaults) */
#define RAMSAFETY_CFG_NUM_REGIONS                   4U
#define RAMSAFETY_ADDR_DTCM_BASE                    0x20000000U
#define RAMSAFETY_ADDR_DTCM_SIZE                    0x00020000U
#define RAMSAFETY_ADDR_SRAM_BASE                    0x20400000U
#define RAMSAFETY_ADDR_SRAM_SIZE                    0x00040000U
#define RAMSAFETY_ADDR_FLEXRAM_BASE                 0x14000000U
#define RAMSAFETY_ADDR_FLEXRAM_SIZE                 0x00080000U
#define RAMSAFETY_ADDR_STACK_BASE                   0x20020000U
#define RAMSAFETY_ADDR_STACK_SIZE                   0x00008000U

#define RAMSAFETY_REGION_DTCM                       0U
#define RAMSAFETY_REGION_SRAM                       1U
#define RAMSAFETY_REGION_FLEXRAM                    2U
#define RAMSAFETY_REGION_STACK                      3U

/*==================================================================================================
*                                       外部变量宣告
==================================================================================================*/
#define RAMSAFETY_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "RamSafety_MemMap.h"

/**
 * @brief RAM区域配置数组
 */
extern const RamSafety_RegionType RamSafety_RegionConfig[RAMSAFETY_CFG_NUM_REGIONS];

#define RAMSAFETY_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "RamSafety_MemMap.h"

#endif /* RAMSAFETY_CFG_H */
