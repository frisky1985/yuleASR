/**
 * @file NvM_EccHandler_Cfg.h
 * @brief NvM ECC处理模块配置
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef NVM_ECCHANDLER_CFG_H
#define NVM_ECCHANDLER_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define NVM_ECCHANDLER_CFG_VENDOR_ID                    43
#define NVM_ECCHANDLER_CFG_AR_RELEASE_MAJOR_VERSION     4
#define NVM_ECCHANDLER_CFG_AR_RELEASE_MINOR_VERSION     7
#define NVM_ECCHANDLER_CFG_AR_RELEASE_REVISION_VERSION  0
#define NVM_ECCHANDLER_CFG_SW_MAJOR_VERSION             1
#define NVM_ECCHANDLER_CFG_SW_MINOR_VERSION             0
#define NVM_ECCHANDLER_CFG_SW_PATCH_VERSION             0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "NvM_EccHandler.h"

/*==================================================================================================
*                                       配置定义
==================================================================================================*/
/**
 * @brief 使能ECC处理
 */
#define NVM_ECCHANDLER_CFG_ENABLED                      STD_ON

/**
 * @brief 默认重试次数
 */
#define NVM_ECCHANDLER_CFG_DEFAULT_RETRY_COUNT          3U

/**
 * @brief 使能写入验证
 */
#define NVM_ECCHANDLER_CFG_WRITE_VERIFICATION           STD_ON

/**
 * @brief 配置的块数量
 */
#define NVM_ECCHANDLER_CFG_NUM_BLOCKS                   5U

/*==================================================================================================
*                                       块配置
==================================================================================================*/
/**
 * @brief 特殊块ID定义
 */
#define NVM_BLOCK_ID_DEM_ADMIN                          1U
#define NVM_BLOCK_ID_DEM_STATUS                         2U
#define NVM_BLOCK_ID_ECUM_CONFIG                        3U
#define NVM_BLOCK_ID_BSWM_CONFIG                        4U
#define NVM_BLOCK_ID_APP_DATA                           5U

#endif /* NVM_ECCHANDLER_CFG_H */
