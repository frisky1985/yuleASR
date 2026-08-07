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
 * @file WdgM.c
 * @brief 看门狗管理模块实现
 * 
 * 功能: 看门狗监控、监督定时、安全状态管理
 * 支持窗口看门狗(WWD)和独立看门狗(IWD)
 * 与Lockstep和RamSafety安全模块集成
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
#include "Det.h"
#include "Mcal.h"

#if (WDGM_CFG_LOCKSTEP_INTEGRATION == STD_ON)
#include "Lockstep.h"
#endif

#if (WDGM_CFG_RAMSAFETY_INTEGRATION == STD_ON)
#include "RamSafety.h"
#endif

#if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
#include "Dem.h"
#endif

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 模块ID (用于Det)
 */
#define WDGM_MODULE_ID                          0x0DU

/**
 * @brief API ID定义
 */
#define WDGM_API_INIT                           0x00U
#define WDGM_API_DEINIT                         0x01U
#define WDGM_API_GET_STATE                      0x02U
#define WDGM_API_SET_MODE                       0x03U
#define WDGM_API_GET_MODE                       0x04U
#define WDGM_API_CHECKPOINT_REACHED             0x0EU
#define WDGM_API_UPDATE_ALIVE                   0x11U
#define WDGM_API_GET_SE_STATE                   0x0CU
#define WDGM_API_DEACTIVATE_SE                  0x12U
#define WDGM_API_ACTIVATE_SE                    0x13U
#define WDGM_API_GET_GLOBAL_STATUS              0x14U
#define WDGM_API_MAIN_FUNCTION                  0x08U
#define WDGM_API_TRIGGER_WATCHDOG               0x09U
#define WDGM_API_IS_DISABLE_ALLOWED             0x15U
#define WDGM_API_PERFORM_RESET                  0x16U
#define WDGM_API_HANDLE_LOCKSTEP_ERROR          0x20U
#define WDGM_API_HANDLE_RAMSAFETY_ERROR         0x21U

/**
 * @brief 安全魔数
 */
#define WDGM_SAFETY_MAGIC_INIT                  0xA55AA55AU
#define WDGM_SAFETY_MAGIC_ACTIVE                0x5AA55AA5U

/**
 * @brief 事件类型定义
 */
#define WDGM_EVENT_SUPERVISION_EXPIRED          0x01U
#define WDGM_EVENT_LOCKSTEP_ERROR               0x02U
#define WDGM_EVENT_RAMSAFETY_ERROR              0x03U
#define WDGM_EVENT_MODE_CHANGE                  0x04U
#define WDGM_EVENT_WATCHDOG_RESET               0x05U

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define WDGM_START_SEC_VAR_INIT_UNSPECIFIED
#include "WdgM_MemMap.h"

/**
 * @brief 初始化状态
 */
STATIC volatile WdgM_StateType WdgM_State = WDGM_STATE_UNINIT;

/**
 * @brief 当前模式
 */
STATIC volatile uint8 WdgM_CurrentMode = WDGM_WATCHDOG_MODE_OFF;

/**
 * @brief 当前配置
 */
STATIC const WdgM_ConfigType* WdgM_CurrentConfig = NULL_PTR;

/**
 * @brief 安全魔数 (用于ASIL-D运行时检查)
 */
STATIC volatile uint32 WdgM_SafetyMagic = 0U;

#define WDGM_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "WdgM_MemMap.h"

#define WDGM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "WdgM_MemMap.h"

/**
 * @brief 监督实体运行时数据
 */
STATIC WdgM_SupervisedEntityType WdgM_SupervisedEntities[WDGM_CFG_MAX_SUPERVISED_ENTITIES];

/**
 * @brief 全局状态信息
 */
STATIC WdgM_GlobalStatusType WdgM_GlobalStatus;

/**
 * @brief 计时器 (用于监督周期控制)
 */
STATIC uint16 WdgM_CycleTimer = 0U;

/**
 * @brief 看门狗触发计时器
 */
STATIC uint16 WdgM_TriggerTimer = 0U;

/**
 * @brief 连续错误计数
 */
STATIC uint8 WdgM_ConsecutiveErrors = 0U;

/**
 * @brief 安全事件回调
 */
STATIC WdgM_SafetyCallbackType WdgM_SafetyCallback = NULL_PTR;
STATIC const void* WdgM_SafetyCallbackContext = NULL_PTR;

/**
 * @brief 第一超时SEID
 */
STATIC uint16 WdgM_FirstExpiredSEID = 0U;

/**
 * @brief 禁用允许标志
 */
STATIC boolean WdgM_DisableAllowed = FALSE;

#define WDGM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "WdgM_MemMap.h"

/*==================================================================================================
*                                       静态函数声明
==================================================================================================*/
STATIC void WdgM_ReportError(uint8 apiId, uint8 errorId);
STATIC void WdgM_NotifyEvent(uint8 eventType, uint32 errorCode);
STATIC Std_ReturnType WdgM_ValidateConfig(const WdgM_ConfigType* config);
STATIC void WdgM_UpdateSupervision(void);
STATIC void WdgM_CheckEntityAlive(uint8 entityIdx);
STATIC void WdgM_HandleExpiredSupervision(uint8 entityIdx);
STATIC Std_ReturnType WdgM_FindEntityIndex(uint16 seId, uint8* index);
STATIC void WdgM_PlatformTrigger(void);
STATIC void WdgM_PlatformSetMode(uint8 mode);
STATIC void WdgM_PerformSafetyAction(uint32 errorCode);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define WDGM_START_SEC_CODE
#include "WdgM_MemMap.h"

/**
 * @brief 初始化WdgM模块
 * @ASIL-D: Safety critical initialization with redundancy check
 */
Std_ReturnType WdgM_Init(const WdgM_ConfigType* config)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
    /* 参数验证 */
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == config)
    {
        WdgM_ReportError(WDGM_API_INIT, WDGM_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    
    if (WdgM_State != WDGM_STATE_UNINIT)
    {
        WdgM_ReportError(WDGM_API_INIT, WDGM_E_ALREADY_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* 配置验证 */
    if (E_OK != WdgM_ValidateConfig(config))
    {
        WdgM_ReportError(WDGM_API_INIT, WDGM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    /* 禁用中断 */
    Mcal_DisableAllInterrupts();
    
    /* 设置状态 */
    WdgM_State = WDGM_STATE_INIT;
    WdgM_CurrentConfig = config;
    WdgM_CurrentMode = WDGM_WATCHDOG_MODE_OFF;
    
    /* 初始化监督实体 */
    for (i = 0U; i < WDGM_CFG_MAX_SUPERVISED_ENTITIES; i++)
    {
        WdgM_SupervisedEntities[i].seId = 0U;
        WdgM_SupervisedEntities[i].state = WDGM_SE_STATE_DEACTIVATED;
        WdgM_SupervisedEntities[i].aliveCounter = 0U;
        WdgM_SupervisedEntities[i].expectedAliveIndications = 0U;
        WdgM_SupervisedEntities[i].timestampStart = 0U;
        WdgM_SupervisedEntities[i].timestampStop = 0U;
        WdgM_SupervisedEntities[i].consecutiveErrors = 0U;
        WdgM_SupervisedEntities[i].deactivated = TRUE;
    }
    
    /* 配置激活的监督实体 */
    if (config->entities != NULL_PTR)
    {
        for (i = 0U; i < config->numEntities; i++)
        {
            if (i < WDGM_CFG_MAX_SUPERVISED_ENTITIES)
            {
                WdgM_SupervisedEntities[i].seId = config->entities[i].seId;
                WdgM_SupervisedEntities[i].deactivated = !config->entities[i].enabled;
                if ((config->entities[i].enabled) != 0U)
                {
                    WdgM_SupervisedEntities[i].state = WDGM_SE_STATE_CORRECT;
                    WdgM_SupervisedEntities[i].deactivated = FALSE;
                }
            }
        }
    }
    
    /* 初始化全局状态 */
    WdgM_GlobalStatus.expiredSupervisionCycles = 0U;
    WdgM_GlobalStatus.totalRefreshes = 0U;
    WdgM_GlobalStatus.failedRefreshes = 0U;
    WdgM_GlobalStatus.lockstepErrors = 0U;
    WdgM_GlobalStatus.ramSafetyErrors = 0U;
    WdgM_GlobalStatus.currentMode = WDGM_WATCHDOG_MODE_OFF;
    
    WdgM_CycleTimer = 0U;
    WdgM_TriggerTimer = 0U;
    WdgM_ConsecutiveErrors = 0U;
    WdgM_FirstExpiredSEID = 0U;
    
    /* 设置安全魔数 */
    WdgM_SafetyMagic = WDGM_SAFETY_MAGIC_INIT;
    WdgM_State = WDGM_STATE_ACTIVE;
    WdgM_SafetyMagic = WDGM_SAFETY_MAGIC_ACTIVE;
    
    /* 恢复中断 */
    Mcal_EnableAllInterrupts();
    
    WdgM_NotifyEvent(WDGM_EVENT_MODE_CHANGE, WDGM_STATE_ACTIVE);
    
    result = E_OK;
    return result;
}

/**
 * @brief 去初始化WdgM模块
 */
Std_ReturnType WdgM_DeInit(void)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        WdgM_ReportError(WDGM_API_DEINIT, WDGM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* 禁用中断 */
    Mcal_DisableAllInterrupts();
    
    /* 检查是否允许禁用 */
    if (!WdgM_IsDisableAllowed())
    {
        Mcal_EnableAllInterrupts();
        WdgM_ReportError(WDGM_API_DEINIT, WDGM_E_DISABLE_NOT_ALLOWED);
        return E_NOT_OK;
    }
    
    /* 停止看门狗 */
    WdgM_PlatformSetMode(WDGM_WATCHDOG_MODE_OFF);
    
    /* 重置状态 */
    WdgM_State = WDGM_STATE_UNINIT;
    WdgM_CurrentMode = WDGM_WATCHDOG_MODE_OFF;
    WdgM_CurrentConfig = NULL_PTR;
    WdgM_SafetyMagic = 0U;
    WdgM_SafetyCallback = NULL_PTR;
    WdgM_SafetyCallbackContext = NULL_PTR;
    
    Mcal_EnableAllInterrupts();
    
    result = E_OK;
    return result;
}

/**
 * @brief 获取当前状态
 */
WdgM_StateType WdgM_GetState(void)
{
    return WdgM_State;
}

/**
 * @brief 设置看门狗模式
 * @ASIL-D: Requires privilege verification
 */
Std_ReturnType WdgM_SetMode(uint8 mode)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        WdgM_ReportError(WDGM_API_SET_MODE, WDGM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    
    if (mode > WDGM_WATCHDOG_MODE_FAST)
    {
        WdgM_ReportError(WDGM_API_SET_MODE, WDGM_E_PARAM_MODE);
        return E_NOT_OK;
    }
#endif
    
    /* 检查是否允许禁用看门狗 */
    if ((mode == WDGM_WATCHDOG_MODE_OFF) && (!WdgM_IsDisableAllowed()))
    {
        WdgM_ReportError(WDGM_API_SET_MODE, WDGM_E_DISABLE_NOT_ALLOWED);
        return E_NOT_OK;
    }
    
    /* 调用底层设置模式 */
    WdgM_PlatformSetMode(mode);
    
    WdgM_CurrentMode = mode;
    WdgM_GlobalStatus.currentMode = mode;
    
    WdgM_NotifyEvent(WDGM_EVENT_MODE_CHANGE, (uint32)mode);
    
#if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
    if (mode == WDGM_WATCHDOG_MODE_OFF)
    {
        /* Dem_ReportErrorStatus(WDGM_DEM_SET_MODE_FAILED_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
    }
#endif
    
    result = E_OK;
    return result;
}

/**
 * @brief 获取当前模式
 */
uint8 WdgM_GetMode(void)
{
    return WdgM_CurrentMode;
}

/**
 * @brief 检查是否允许禁用看门狗
 */
boolean WdgM_IsDisableAllowed(void)
{
    return WdgM_DisableAllowed;
}

/**
 * @brief 检查点报告 (Alive Supervision)
 * @ASIL-D: Runtime safety check
 */
Std_ReturnType WdgM_CheckpointReached(uint16 seId)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 entityIdx;
    
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        WdgM_ReportError(WDGM_API_CHECKPOINT_REACHED, WDGM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    /* 查找监督实体索引 */
    result = WdgM_FindEntityIndex(seId, &entityIdx);
    
    if (E_OK == result)
    {
        /* 更新时间戳 */
        WdgM_SupervisedEntities[entityIdx].timestampStop = WdgM_CycleTimer;
        
        /* 更新活计数器 */
        if (WdgM_SupervisedEntities[entityIdx].aliveCounter < 0xFFFFU)
        {
            WdgM_SupervisedEntities[entityIdx].aliveCounter++;
        }
        
        /* 重置连续错误计数 */
        WdgM_SupervisedEntities[entityIdx].consecutiveErrors = 0U;
        
        /* 更新状态 */
        if (WdgM_SupervisedEntities[entityIdx].state != WDGM_SE_STATE_DEACTIVATED)
        {
            WdgM_SupervisedEntities[entityIdx].state = WDGM_SE_STATE_CORRECT;
        }
    }
    else
    {
        WdgM_ReportError(WDGM_API_CHECKPOINT_REACHED, WDGM_E_CPID_NOT_CONFIGURED);
    }
    
    return result;
}

/**
 * @brief 更新活监督指示
 */
Std_ReturnType WdgM_UpdateAliveIndication(uint16 seId)
{
    return WdgM_CheckpointReached(seId);
}

/**
 * @brief 获取监督实体状态
 */
Std_ReturnType WdgM_GetSEState(uint16 seId, WdgM_SEStateType* state)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 entityIdx;
    
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
    
    if (NULL_PTR == state)
    {
        return E_NOT_OK;
    }
#endif
    
    result = WdgM_FindEntityIndex(seId, &entityIdx);
    
    if (E_OK == result)
    {
        *state = WdgM_SupervisedEntities[entityIdx].state;
    }
    
    return result;
}

/**
 * @brief 去激活监督实体
 */
Std_ReturnType WdgM_DeactivateSupervisionEntity(uint16 seId)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 entityIdx;
    
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif
    
    result = WdgM_FindEntityIndex(seId, &entityIdx);
    
    if (E_OK == result)
    {
        WdgM_SupervisedEntities[entityIdx].deactivated = TRUE;
        WdgM_SupervisedEntities[entityIdx].state = WDGM_SE_STATE_DEACTIVATED;
    }
    
    return result;
}

/**
 * @brief 重新激活监督实体
 */
Std_ReturnType WdgM_ActivateSupervisionEntity(uint16 seId)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 entityIdx;
    
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif
    
    result = WdgM_FindEntityIndex(seId, &entityIdx);
    
    if (E_OK == result)
    {
        WdgM_SupervisedEntities[entityIdx].deactivated = FALSE;
        WdgM_SupervisedEntities[entityIdx].state = WDGM_SE_STATE_CORRECT;
        WdgM_SupervisedEntities[entityIdx].aliveCounter = 0U;
        WdgM_SupervisedEntities[entityIdx].consecutiveErrors = 0U;
    }
    
    return result;
}

/**
 * @brief 获取全局状态信息
 */
Std_ReturnType WdgM_GetGlobalStatus(WdgM_GlobalStatusType* status)
{
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == status)
    {
        return E_NOT_OK;
    }
    
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif
    
    /* 安全拷贝统计信息 */
    status->expiredSupervisionCycles = WdgM_GlobalStatus.expiredSupervisionCycles;
    status->totalRefreshes = WdgM_GlobalStatus.totalRefreshes;
    status->failedRefreshes = WdgM_GlobalStatus.failedRefreshes;
    status->lockstepErrors = WdgM_GlobalStatus.lockstepErrors;
    status->ramSafetyErrors = WdgM_GlobalStatus.ramSafetyErrors;
    status->currentMode = WdgM_GlobalStatus.currentMode;
    
    return E_OK;
}

/**
 * @brief 主循环处理函数
 * @ASIL-D: Periodic safety monitoring
 */
void WdgM_MainFunction(void)
{
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        return;
    }
    
    /* 检查安全魔数 */
    if (WdgM_SafetyMagic != WDGM_SAFETY_MAGIC_ACTIVE)
    {
        WdgM_PerformSafetyAction(WDGM_E_DATA_CORRUPTION);
        return;
    }
    
    /* 更新计时器 */
    WdgM_CycleTimer++;
    WdgM_TriggerTimer++;
    
    /* 执行监督更新 */
    WdgM_UpdateSupervision();
    
    /* 检查看门狗触发周期 */
    if (WdgM_CurrentConfig != NULL_PTR)
    {
        if (WdgM_TriggerTimer >= WdgM_CurrentConfig->supervisionCycleMs)
        {
            WdgM_TriggerTimer = 0U;
            
            /* 触发看门狗 (只有活跃状态才触发) */
            if (WdgM_State == WDGM_STATE_ACTIVE)
            {
                WdgM_TriggerWatchdog();
            }
        }
    }
    
    /* 检查连续错误计数 */
    if (WdgM_ConsecutiveErrors >= WdgM_CurrentConfig->failureThreshold)
    {
        WdgM_PerformSafetyAction(WDGM_E_SUPERVISION_EXPIRED);
    }
}

/**
 * @brief 执行看门狗触发
 * @ASIL-D: Watchdog refresh
 */
void WdgM_TriggerWatchdog(void)
{
    if (WdgM_State == WDGM_STATE_ACTIVE)
    {
        /* 检查安全魔数 */
        if (WdgM_SafetyMagic != WDGM_SAFETY_MAGIC_ACTIVE)
        {
            return;
        }
        
        /* 触发底层看门狗 */
        WdgM_PlatformTrigger();
        
        WdgM_GlobalStatus.totalRefreshes++;
    }
}

/**
 * @brief 执行立即复位
 * @ASIL-D: Emergency reset
 */
void WdgM_PerformReset(void)
{
    WdgM_NotifyEvent(WDGM_EVENT_WATCHDOG_RESET, 0U);
    
    /* 调用平台复位函数 */
    /* Platform_PerformReset(); */
    
    /* 如果复位失败，进入死循环 */
    for (;;)
    {
        /* Infinite loop */
    }
}

/**
 * @brief 获取第一超时值
 */
Std_ReturnType WdgM_GetFirstExpiredSEID(uint16* seId)
{
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == seId)
    {
        return E_NOT_OK;
    }
#endif
    
    if (WdgM_FirstExpiredSEID != 0U)
    {
        *seId = WdgM_FirstExpiredSEID;
        return E_OK;
    }
    
    return E_NOT_OK;
}

/**
 * @brief 处理Lockstep错误事件
 * @ASIL-D: Lockstep integration
 */
void WdgM_HandleLockstepError(uint32 errorCode)
{
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        WdgM_ReportError(WDGM_API_HANDLE_LOCKSTEP_ERROR, WDGM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* 更新统计 */
    WdgM_GlobalStatus.lockstepErrors++;
    
    /* 通知事件 */
    WdgM_NotifyEvent(WDGM_EVENT_LOCKSTEP_ERROR, errorCode);
    
#if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
    /* Dem_ReportErrorStatus(WDGM_DEM_LOCKSTEP_ERROR_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
#endif
    
    /* 执行安全响应 */
    WdgM_PerformSafetyAction(errorCode);
}

/**
 * @brief 处理RamSafety错误事件
 * @ASIL-D: RamSafety integration
 */
void WdgM_HandleRamSafetyError(uint32 errorCode)
{
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    if (WdgM_State == WDGM_STATE_UNINIT)
    {
        WdgM_ReportError(WDGM_API_HANDLE_RAMSAFETY_ERROR, WDGM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* 更新统计 */
    WdgM_GlobalStatus.ramSafetyErrors++;
    
    /* 通知事件 */
    WdgM_NotifyEvent(WDGM_EVENT_RAMSAFETY_ERROR, errorCode);
    
#if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
    /* Dem_ReportErrorStatus(WDGM_DEM_RAMSAFETY_ERROR_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
#endif
    
    /* 执行安全响应 */
    WdgM_PerformSafetyAction(errorCode);
}

/**
 * @brief 注册安全事件回调
 */
Std_ReturnType WdgM_RegisterSafetyCallback(
    WdgM_SafetyCallbackType callback,
    const void* context)
{
    WdgM_SafetyCallback = callback;
    WdgM_SafetyCallbackContext = context;
    return E_OK;
}

#if (WDGM_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 */
void WdgM_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR != versioninfo)
    {
        versioninfo->vendorID = WDGM_VENDOR_ID;
        versioninfo->moduleID = WDGM_MODULE_ID;
        versioninfo->sw_major_version = WDGM_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = WDGM_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = WDGM_SW_PATCH_VERSION;
    }
}
#endif

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief 报告错误
 */
STATIC void WdgM_ReportError(uint8 apiId, uint8 errorId)
{
#if (WDGM_CFG_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(WDGM_MODULE_ID, 0U, apiId, errorId);
#endif
}

/**
 * @brief 通知事件
 */
STATIC void WdgM_NotifyEvent(uint8 eventType, uint32 errorCode)
{
    if (WdgM_SafetyCallback != NULL_PTR)
    {
        WdgM_SafetyCallback(eventType, errorCode, WdgM_SafetyCallbackContext);
    }
}

/**
 * @brief 验证配置
 */
STATIC Std_ReturnType WdgM_ValidateConfig(const WdgM_ConfigType* config)
{
    if (config->numEntities > WDGM_CFG_MAX_SUPERVISED_ENTITIES)
    {
        return E_NOT_OK;
    }
    
    if (config->failureThreshold == 0U)
    {
        return E_NOT_OK;
    }
    
    if (config->supervisionCycleMs == 0U)
    {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief 更新监督状态
 */
STATIC void WdgM_UpdateSupervision(void)
{
    uint8 i;
    
    for (i = 0U; i < WDGM_CFG_MAX_SUPERVISED_ENTITIES; i++)
    {
        if (!WdgM_SupervisedEntities[i].deactivated && 
            (WdgM_SupervisedEntities[i].seId != 0U))
        {
            WdgM_CheckEntityAlive(i);
        }
    }
}

/**
 * @brief 检查实体活性状态
 */
STATIC void WdgM_CheckEntityAlive(uint8 entityIdx)
{
    const WdgM_SupervisedEntityType* entity = &WdgM_SupervisedEntities[entityIdx];
    
    /* 检查是否超时 */
    if (WdgM_CurrentConfig != NULL_PTR)
    {
        if (entity->aliveCounter < entity->expectedAliveIndications)
        {
            /* 未收到足够的活指示 */
            WdgM_SupervisedEntities[entityIdx].consecutiveErrors++;
            
            if (WdgM_SupervisedEntities[entityIdx].consecutiveErrors >= WDGM_CFG_FAILURE_THRESHOLD)
            {
                WdgM_HandleExpiredSupervision(entityIdx);
            }
        }
    }
}

/**
 * @brief 处理超时监督
 */
STATIC void WdgM_HandleExpiredSupervision(uint8 entityIdx)
{
    WdgM_SupervisedEntities[entityIdx].state = WDGM_SE_STATE_EXPIRED;
    WdgM_FirstExpiredSEID = WdgM_SupervisedEntities[entityIdx].seId;
    WdgM_ConsecutiveErrors++;
    
    /* 更新全局状态 */
    WdgM_GlobalStatus.expiredSupervisionCycles++;
    WdgM_State = WDGM_STATE_SUPERVISION_EXPIRED;
    
    WdgM_NotifyEvent(WDGM_EVENT_SUPERVISION_EXPIRED, WdgM_FirstExpiredSEID);
    
#if (WDGM_CFG_DEM_INTEGRATION == STD_ON)
    /* Dem_ReportErrorStatus(WDGM_DEM_SUPERVISION_EXPIRED_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
#endif
}

/**
 * @brief 查找监督实体索引
 */
STATIC Std_ReturnType WdgM_FindEntityIndex(uint16 seId, uint8* index)
{
    uint8 i;
    
    for (i = 0U; i < WDGM_CFG_MAX_SUPERVISED_ENTITIES; i++)
    {
        if (WdgM_SupervisedEntities[i].seId == seId)
        {
            *index = i;
            return E_OK;
        }
    }
    
    return E_NOT_OK;
}

/**
 * @brief 底层平台触发看门狗
 */
STATIC void WdgM_PlatformTrigger(void)
{
    /* 调用配置的触发函数 */
    WdgM_WatchdogTrigger();
}

/**
 * @brief 底层平台设置模式
 */
STATIC void WdgM_PlatformSetMode(uint8 mode)
{
    /* 调用配置的设置模式函数 */
    WdgM_WatchdogSetMode(mode);
}

/**
 * @brief 执行安全响应
 */
STATIC void WdgM_PerformSafetyAction(uint32 errorCode)
{
    (void)errorCode;
    
    /* 记录失败 */
    WdgM_GlobalStatus.failedRefreshes++;
    
    /* 执行复位 */
    WdgM_PerformReset();
}

#define WDGM_STOP_SEC_CODE
#include "WdgM_MemMap.h"
