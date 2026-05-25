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
 * @file WdgM_Cfg.c
 * @brief 看门狗管理模块配置实现
 * 
 * S32K312平台WdgM配置
 * ASIL-D安全配置
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "WdgM.h"
#include "WdgM_Cfg.h"
#include "WdgM_MemMap.h"

/*==================================================================================================
*                                       配置数据
==================================================================================================*/
#define WDGM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "WdgM_MemMap.h"

#if (WDGM_CFG_WWD_ENABLE == STD_ON)
/**
 * @brief 窗口看门狗配置
 * 
 * 配置参数:
 * - triggerPeriodMs: 50ms触发周期
 * - windowStartMs: 25ms窗口开始 (周期的50%)
 * - windowEndMs: 50ms窗口结束 (周期的100%)
 */
static const WdgM_WatchdogConfigType WdgM_WwdConfig =
{
    .type = WDGM_WATCHDOG_WWD,
    .triggerPeriodMs = WDGM_CFG_WWD_TRIGGER_PERIOD_MS,
    .windowStartMs = (WDGM_CFG_WWD_TRIGGER_PERIOD_MS * WDGM_CFG_WWD_WINDOW_START_PERCENT) / 100U,
    .windowEndMs = (WDGM_CFG_WWD_TRIGGER_PERIOD_MS * WDGM_CFG_WWD_WINDOW_END_PERCENT) / 100U,
    .enabled = TRUE
};
#endif

#if (WDGM_CFG_IWD_ENABLE == STD_ON)
/**
 * @brief 独立看门狗配置
 * 
 * 配置参数:
 * - triggerPeriodMs: 100ms触发周期
 * - windowStartMs: 0ms (无窗口限制)
 * - windowEndMs: 100ms
 */
static const WdgM_WatchdogConfigType WdgM_IwdConfig =
{
    .type = WDGM_WATCHDOG_IWD,
    .triggerPeriodMs = WDGM_CFG_IWD_TRIGGER_PERIOD_MS,
    .windowStartMs = 0U,
    .windowEndMs = WDGM_CFG_IWD_TIMEOUT_MS,
    .enabled = TRUE
};
#endif

/**
 * @brief 看门狗配置数组
 */
static const WdgM_WatchdogConfigType WdgM_WatchdogConfigs[WDGM_MAX_WATCHDOGS] =
{
#if (WDGM_CFG_WWD_ENABLE == STD_ON)
    WdgM_WwdConfig,
#endif
#if (WDGM_CFG_IWD_ENABLE == STD_ON)
    WdgM_IwdConfig
#endif
};

/**
 * @brief 监督实体配置数组
 * 
 * 定义监督的软件模块:
 * 1. 主循环 (Main Cycle) - 必须每周期报告
 * 2. 通信模块 (Communication)
 * 3. 诊断模块 (Diagnostics)
 * 4. 存储模块 (Storage)
 * 5. 安全监控模块 (Safety Monitor)
 * 6. Lockstep监督
 * 7. RamSafety监督
 */
static const WdgM_SupervisedEntityConfigType WdgM_EntityConfigs[WDGM_CFG_MAX_SUPERVISED_ENTITIES] =
{
    /* 监督实体0: 主循环 */
    {
        .seId = WDGM_SEID_MAIN_CYCLE,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 1U,     /* 每个周期期望1次 */
            .aliveSupMin = 1U,          /* 最小1次 */
            .aliveSupMax = 2U           /* 最大2次 (允许轻微波动) */
        }
    },
    /* 监督实体1: 通信模块 */
    {
        .seId = WDGM_SEID_COMMUNICATION,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 10U,    /* 每10个周期 */
            .aliveSupMin = 1U,
            .aliveSupMax = 5U
        }
    },
    /* 监督实体2: 诊断模块 */
    {
        .seId = WDGM_SEID_DIAGNOSTICS,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 100U,   /* 每100个周期 (1s) */
            .aliveSupMin = 1U,
            .aliveSupMax = 10U
        }
    },
    /* 监督实体3: 存储模块 */
    {
        .seId = WDGM_SEID_STORAGE,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 50U,    /* 每50个周期 (500ms) */
            .aliveSupMin = 1U,
            .aliveSupMax = 5U
        }
    },
    /* 监督实体4: 安全监控模块 */
    {
        .seId = WDGM_SEID_SAFETY_MONITOR,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 1U,     /* 每个周期必须报告 */
            .aliveSupMin = 1U,
            .aliveSupMax = 1U           /* 严格每周期一次 */
        }
    },
    /* 监督实体5: Lockstep监督 */
    {
        .seId = WDGM_SEID_LOCKSTEP,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 10U,    /* 每100ms */
            .aliveSupMin = 1U,
            .aliveSupMax = 2U
        }
    },
    /* 监督实体6: RamSafety监督 */
    {
        .seId = WDGM_SEID_RAMSAFETY,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = TRUE,
        .config.alive = {
            .aliveSupRefCycle = 10U,    /* 每100ms */
            .aliveSupMin = 1U,
            .aliveSupMax = 2U
        }
    },
    /* 监督实体7: 保留 */
    {
        .seId = 0x0008U,
        .supervisionType = WDGM_SUPERVISION_ALIVE,
        .enabled = FALSE,
        .config.alive = {
            .aliveSupRefCycle = 1U,
            .aliveSupMin = 1U,
            .aliveSupMax = 1U
        }
    }
};

/**
 * @brief WdgM初始化配置
 * 
 * ASIL-D安全配置:
 * - 10ms监督周期
 * - 错误阈值3次
 * - Lockstep和RamSafety集成使能
 */
const WdgM_ConfigType WdgM_Config =
{
    .watchdogs = WdgM_WatchdogConfigs,
    .numWatchdogs = WDGM_MAX_WATCHDOGS,
    .entities = WdgM_EntityConfigs,
    .numEntities = 7U,                          /* 使能7个监督实体 */
    .failureThreshold = WDGM_CFG_FAILURE_THRESHOLD,
    .supervisionCycleMs = WDGM_CFG_SUPERVISION_CYCLE_MS,
    .lockstepIntegration = WDGM_CFG_LOCKSTEP_INTEGRATION,
    .ramSafetyIntegration = WDGM_CFG_RAMSAFETY_INTEGRATION
};

/**
 * @brief 调试配置 (较低监督强度)
 * 
 * 用于开发和调试阶段
 */
const WdgM_ConfigType WdgM_ConfigDebug =
{
    .watchdogs = WdgM_WatchdogConfigs,
    .numWatchdogs = WDGM_MAX_WATCHDOGS,
    .entities = WdgM_EntityConfigs,
    .numEntities = 3U,                          /* 只使能核心监督实体 */
    .failureThreshold = 10U,                    /* 较高阈值 */
    .supervisionCycleMs = 100U,                 /* 较长周期 */
    .lockstepIntegration = STD_OFF,
    .ramSafetyIntegration = STD_OFF
};

#define WDGM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "WdgM_MemMap.h"

/*==================================================================================================
*                                       回调函数实现
==================================================================================================*/
#define WDGM_START_SEC_CODE
#include "WdgM_MemMap.h"

/**
 * @brief 看门狗触发函数
 * 
 * 由WdgM调用，实现应调用MCAL Wdg驱动
 */
void WdgM_WatchdogTrigger(void)
{
#if (WDGM_CFG_WWD_ENABLE == STD_ON)
    /* 触发窗口看门狗 */
    /* Wdg_SetTriggerCondition(WDGM_CFG_WWD_TIMEOUT_MS); */
#endif

#if (WDGM_CFG_IWD_ENABLE == STD_ON)
    /* 触发独立看门狗 */
    /* Wdg_SetTriggerCondition(WDGM_CFG_IWD_TIMEOUT_MS); */
#endif
}

/**
 * @brief 看门狗模式设置函数
 * 
 * @param mode 模式 (WDGM_WATCHDOG_MODE_OFF/SLOW/FAST)
 */
void WdgM_WatchdogSetMode(uint8 mode)
{
    (void)mode;

    /* 根据模式设置看门狗 */
    switch (mode)
    {
        case WDGM_WATCHDOG_MODE_OFF:
            /* 禁用看门狗 */
            /* Wdg_SetMode(WDGIF_OFF_MODE); */
            break;
            
        case WDGM_WATCHDOG_MODE_SLOW:
            /* 慢速模式 */
            /* Wdg_SetMode(WDGIF_SLOW_MODE); */
            break;
            
        case WDGM_WATCHDOG_MODE_FAST:
            /* 快速模式 */
            /* Wdg_SetMode(WDGIF_FAST_MODE); */
            break;
            
        default:
            /* 无效模式 */
            break;
    }
}

/**
 * @brief 安全事件回调
 * 
 * 处理安全相关事件，可在应用层实现
 * 
 * @param eventType 事件类型
 * @param errorCode 错误码
 * @param context 上下文
 */
void WdgM_SafetyEventCallback(
    uint8 eventType,
    uint32 errorCode,
    const void* context)
{
    (void)context;  /* 未使用 */
    
    switch (eventType)
    {
        case 0x01U:  /* 监督超时 */
            /* 监督实体超时检测到 */
            /* 可触发Dem诊断码 */
            /* Dem_ReportErrorStatus(WDGM_DEM_SUPERVISION_EXPIRED_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
            break;
            
        case 0x02U:  /* Lockstep错误 */
            /* Lockstep错误检测到 */
            /* Dem_ReportErrorStatus(WDGM_DEM_LOCKSTEP_ERROR_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
            break;
            
        case 0x03U:  /* RamSafety错误 */
            /* RAM安全错误检测到 */
            /* Dem_ReportErrorStatus(WDGM_DEM_RAMSAFETY_ERROR_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
            break;
            
        case 0x04U:  /* 模式改变 */
            /* 看门狗模式改变 */
            break;
            
        case 0x05U:  /* 复位 */
            /* 执行复位 */
            break;
            
        default:
            /* 未知事件 */
            break;
    }
    
    /* 可以在这里添加自定义的安全响应 */
    (void)errorCode;
}

#define WDGM_STOP_SEC_CODE
#include "WdgM_MemMap.h"
