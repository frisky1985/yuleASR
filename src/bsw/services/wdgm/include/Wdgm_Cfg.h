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
 * @file WdgM_Cfg.h
 * @brief 看门狗管理模块配置头文件
 * 
 * 预处理和配置定义
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef WDGM_CFG_H
#define WDGM_CFG_H

/*==================================================================================================
*                                       版本信息
==================================================================================================*/
#define WDGM_CFG_VENDOR_ID                      43
#define WDGM_CFG_AR_RELEASE_MAJOR_VERSION       4
#define WDGM_CFG_AR_RELEASE_MINOR_VERSION       7
#define WDGM_CFG_AR_RELEASE_REVISION_VERSION    0
#define WDGM_CFG_SW_MAJOR_VERSION               1
#define WDGM_CFG_SW_MINOR_VERSION               0
#define WDGM_CFG_SW_PATCH_VERSION               0

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
#define WDGM_CFG_DEV_ERROR_DETECT               STD_ON

/**
 * @brief 版本信息API
 */
#define WDGM_CFG_VERSION_INFO_API               STD_ON

/**
 * @brief 使能窗口看门狗 (WWD)
 */
#define WDGM_CFG_WWD_ENABLE                     STD_ON

/**
 * @brief 使能独立看门狗 (IWD)
 */
#define WDGM_CFG_IWD_ENABLE                     STD_ON

/**
 * @brief 看门狗监督周期 (ms)
 */
#define WDGM_CFG_SUPERVISION_CYCLE_MS           10U

/**
 * @brief 错误阈值
 * 连续错误次数超过此阈值将触发复位
 */
#define WDGM_CFG_FAILURE_THRESHOLD              3U

/**
 * @brief 最大监督实体数量
 */
#define WDGM_CFG_MAX_SUPERVISED_ENTITIES        8U

/**
 * @brief Lockstep模块集成
 */
#define WDGM_CFG_LOCKSTEP_INTEGRATION           STD_ON

/**
 * @brief RamSafety模块集成
 */
#define WDGM_CFG_RAMSAFETY_INTEGRATION          STD_ON

/**
 * @brief Dem诊断事件报告
 */
#define WDGM_CFG_DEM_INTEGRATION                STD_ON

/*==================================================================================================
*                                       窗口看门狗配置
==================================================================================================*/
#if (WDGM_CFG_WWD_ENABLE == STD_ON)
/**
 * @brief WWD触发周期 (ms)
 */
#define WDGM_CFG_WWD_TRIGGER_PERIOD_MS          50U

/**
 * @brief WWD窗口开始 (占周期百分比, 0-100)
 */
#define WDGM_CFG_WWD_WINDOW_START_PERCENT       50U

/**
 * @brief WWD窗口结束 (占周期百分比, 0-100)
 */
#define WDGM_CFG_WWD_WINDOW_END_PERCENT         100U

/**
 * @brief WWD超时时间 (ms)
 */
#define WDGM_CFG_WWD_TIMEOUT_MS                 100U
#endif

/*==================================================================================================
*                                       独立看门狗配置
==================================================================================================*/
#if (WDGM_CFG_IWD_ENABLE == STD_ON)
/**
 * @brief IWD触发周期 (ms)
 */
#define WDGM_CFG_IWD_TRIGGER_PERIOD_MS          100U

/**
 * @brief IWD超时时间 (ms)
 */
#define WDGM_CFG_IWD_TIMEOUT_MS                 200U
#endif

/*==================================================================================================
*                                       监督实体ID定义
==================================================================================================*/
/**
 * @brief 监督实体ID
 */
#define WDGM_SEID_MAIN_CYCLE                    0x0001U
#define WDGM_SEID_COMMUNICATION                 0x0002U
#define WDGM_SEID_DIAGNOSTICS                   0x0003U
#define WDGM_SEID_STORAGE                       0x0004U
#define WDGM_SEID_SAFETY_MONITOR                0x0005U
#define WDGM_SEID_LOCKSTEP                      0x0006U
#define WDGM_SEID_RAMSAFETY                     0x0007U

/*==================================================================================================
*                                       Dem事件ID (用于Dem集成)
==================================================================================================*/
#if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
/**
 * @brief 监督超时事件ID
 */
#define WDGM_DEM_SUPERVISION_EXPIRED_EVENT_ID   0x01U

/**
 * @brief 看门狗设置模式失败事件ID
 */
#define WDGM_DEM_SET_MODE_FAILED_EVENT_ID       0x02U

/**
 * @brief Lockstep错误事件ID
 */
#define WDGM_DEM_LOCKSTEP_ERROR_EVENT_ID        0x03U

/**
 * @brief RamSafety错误事件ID
 */
#define WDGM_DEM_RAMSAFETY_ERROR_EVENT_ID       0x04U
#endif

/*==================================================================================================
*                                       回调函数声明
==================================================================================================*/
#define WDGM_START_SEC_CODE
#include "WdgM_MemMap.h"

/**
 * @brief 看门狗触发函数
 * 
 * 由WdgM调用，实现应调用MCAL Wdg驱动
 */
extern void WdgM_WatchdogTrigger(void);

/**
 * @brief 看门狗模式设置函数
 * 
 * @param mode 模式 (WDGM_WATCHDOG_MODE_OFF/SLOW/FAST)
 */
extern void WdgM_WatchdogSetMode(uint8 mode);

/**
 * @brief 安全事件回调
 * 
 * 处理安全相关事件，可在应用层实现
 * 
 * @param eventType 事件类型
 * @param errorCode 错误码
 * @param context 上下文
 */
extern void WdgM_SafetyEventCallback(
    uint8 eventType,
    uint32 errorCode,
    const void* context
);

#define WDGM_STOP_SEC_CODE
#include "WdgM_MemMap.h"

/*==================================================================================================
*                                       外部变量声明
==================================================================================================*/
#define WDGM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "WdgM_MemMap.h"

/**
 * @brief 默认配置
 */
extern const WdgM_ConfigType WdgM_Config;

/**
 * @brief 调试配置 (较低监督强度)
 */
extern const WdgM_ConfigType WdgM_ConfigDebug;

#define WDGM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "WdgM_MemMap.h"

#endif /* WDGM_CFG_H */
