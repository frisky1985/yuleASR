/**
 * @file Lockstep_Cfg.h
 * @brief Lockstep模块配置头文件
 * 
 * 预处理和配置定义
 * 
 * @ASIL-D Safety Level
 */

#ifndef LOCKSTEP_CFG_H
#define LOCKSTEP_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define LOCKSTEP_CFG_VENDOR_ID                  43
#define LOCKSTEP_CFG_AR_RELEASE_MAJOR_VERSION   4
#define LOCKSTEP_CFG_AR_RELEASE_MINOR_VERSION   7
#define LOCKSTEP_CFG_AR_RELEASE_REVISION_VERSION    0
#define LOCKSTEP_CFG_SW_MAJOR_VERSION           1
#define LOCKSTEP_CFG_SW_MINOR_VERSION           0
#define LOCKSTEP_CFG_SW_PATCH_VERSION           0

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                       配置选项
==================================================================================================*/
/**
 * @brief 开发错误检测
 * @option STD_ON: 使能
 * @option STD_OFF: 禁用
 */
#define LOCKSTEP_DEV_ERROR_DETECT               STD_ON

/**
 * @brief 版本信息API
 */
#define LOCKSTEP_VERSION_INFO_API               STD_ON

/**
 * @brief 监控周期 (毫秒)
 * 建议值: 10ms
 */
#define LOCKSTEP_MONITOR_PERIOD_MS              10U

/**
 * @brief 错误阈值
 * 连续错误次数超过此阈值将触发安全状态
 */
#define LOCKSTEP_ERROR_THRESHOLD                3U

/**
 * @brief 最大监控区域数量
 */
#define LOCKSTEP_MAX_MONITOR_REGIONS            8U

/**
 * @brief 是否使能CRC验证
 */
#define LOCKSTEP_CRC_CHECK_ENABLE               STD_ON

/**
 * @brief 是否使能FCCU集成
 */
#define LOCKSTEP_FCCU_INTEGRATION               STD_ON

/**
 * @brief 是否使能Dem集成
 */
#define LOCKSTEP_DEM_INTEGRATION                STD_ON

/*==================================================================================================
*                                       事件ID (用于Dem集成)
==================================================================================================*/
#if defined(LOCKSTEP_DEM_INTEGRATION) && (LOCKSTEP_DEM_INTEGRATION == STD_ON)
/**
 * @brief Lockstep不匹配事件ID
 */
#define LOCKSTEP_MISMATCH_EVENT_ID              0x01U

/**
 * @brief BIST失败事件ID
 */
#define LOCKSTEP_BIST_FAIL_EVENT_ID             0x02U

/**
 * @brief 监控超时事件ID
 */
#define LOCKSTEP_TIMEOUT_EVENT_ID               0x03U
#endif

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define LOCKSTEP_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Lockstep_MemMap.h"

/**
 * @brief 默认配置
 */
extern const Lockstep_ConfigType Lockstep_Config;

/**
 * @brief 调试配置
 */
extern const Lockstep_ConfigType Lockstep_ConfigDebug;

#define LOCKSTEP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Lockstep_MemMap.h"

/*==================================================================================================
*                                       回调函数声明
==================================================================================================*/
#define LOCKSTEP_START_SEC_CODE
#include "Lockstep_MemMap.h"

/**
 * @brief 事件回调函数
 * 
 * 可在应用层实现此函数处理Lockstep事件
 */
extern void Lockstep_EventCallback(
    Lockstep_EventType event,
    uint32 errorCode,
    const void* context
);

#define LOCKSTEP_STOP_SEC_CODE
#include "Lockstep_MemMap.h"

#endif /* LOCKSTEP_CFG_H */
