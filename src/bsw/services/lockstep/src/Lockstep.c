/**
 * @file Lockstep.c
 * @brief Lockstep监控器模块实现
 * 
 * 功能: 锁步处理器监控、BIST自测试、错误检测
 * 支持S32K312硬件锁步功能
 * 
 * @ASIL-D Safety Level
 * @author yuleASR Team
 * @version 1.0.0
 */

/*==================================================================================================
*                                       包含头文件
==================================================================================================*/
#include "Lockstep.h"
#include "Platform_Lockstep.h"
#include "Det.h"
#include "Mcal.h"

/*==================================================================================================
*                                       宏定义
==================================================================================================*/
/**
 * @brief 模块ID (用于Det)
 */
#define LOCKSTEP_MODULE_ID                      0x1AU

/**
 * @brief API ID定义
 */
#define LOCKSTEP_API_INIT                       0x01U
#define LOCKSTEP_API_DEINIT                     0x02U
#define LOCKSTEP_API_GET_STATE                  0x03U
#define LOCKSTEP_API_SET_MODE                   0x04U
#define LOCKSTEP_API_RUN_BIST                   0x05U
#define LOCKSTEP_API_TRIGGER_CHECK              0x06U
#define LOCKSTEP_API_MAIN_FUNCTION              0x07U

/**
 * @brief 安全魔数
 */
#define LOCKSTEP_SAFETY_MAGIC_INIT              0xA55AA55AU
#define LOCKSTEP_SAFETY_MAGIC_ACTIVE            0x5AA55AA5U

/*==================================================================================================
*                                       全局变量
==================================================================================================*/
#define LOCKSTEP_START_SEC_VAR_INIT_UNSPECIFIED
#include "Lockstep_MemMap.h"

/**
 * @brief 初始化状态
 */
STATIC volatile Lockstep_StateType Lockstep_State = LOCKSTEP_STATE_UNINIT;

/**
 * @brief 当前模式
 */
STATIC volatile Lockstep_ModeType Lockstep_Mode = LOCKSTEP_MODE_DISABLED;

/**
 * @brief 当前配置
 */
STATIC const Lockstep_ConfigType* Lockstep_CurrentConfig = NULL_PTR;

/**
 * @brief 安全魔数 (用于ASIL-D运行时检查)
 */
STATIC volatile uint32 Lockstep_SafetyMagic = 0U;

#define LOCKSTEP_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Lockstep_MemMap.h"

#define LOCKSTEP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Lockstep_MemMap.h"

/**
 * @brief 统计信息
 */
STATIC Lockstep_StatisticsType Lockstep_Stats;

/**
 * @brief 事件回调
 */
STATIC Lockstep_EventCallbackType Lockstep_Callback = NULL_PTR;
STATIC const void* Lockstep_CallbackContext = NULL_PTR;

/**
 * @brief 连续错误计数
 */
STATIC uint8 Lockstep_ConsecutiveErrors = 0U;

/**
 * @brief 监控定时器
 */
STATIC uint16 Lockstep_MonitorTimer = 0U;

#define LOCKSTEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Lockstep_MemMap.h"

/*==================================================================================================
*                                       静态函数宣告
==================================================================================================*/
STATIC void Lockstep_ReportError(uint8 apiId, uint8 errorId);
STATIC void Lockstep_NotifyEvent(Lockstep_EventType event, uint32 errorCode);
STATIC Std_ReturnType Lockstep_ValidateConfig(const Lockstep_ConfigType* config);
STATIC void Lockstep_UpdateStatistics(boolean isError, uint32 errorCode);
STATIC void Lockstep_HandleError(uint32 errorCode);
STATIC Std_ReturnType Lockstep_PlatformInit(const Lockstep_ConfigType* config);
STATIC void Lockstep_PlatformDeInit(void);

/*==================================================================================================
*                                       函数实现
==================================================================================================*/
#define LOCKSTEP_START_SEC_CODE
#include "Lockstep_MemMap.h"

/**
 * @brief 初始化Lockstep模块
 * @ASIL-D: Safety critical initialization with redundancy check
 */
Std_ReturnType Lockstep_Init(const Lockstep_ConfigType* config)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* 参数验证 */
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == config)
    {
        Lockstep_ReportError(LOCKSTEP_API_INIT, LOCKSTEP_E_INIT_FAILED);
        return E_NOT_OK;
    }
    
    if (Lockstep_State != LOCKSTEP_STATE_UNINIT)
    {
        Lockstep_ReportError(LOCKSTEP_API_INIT, LOCKSTEP_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif
    
    /* 配置验证 */
    if (E_OK != Lockstep_ValidateConfig(config))
    {
        Lockstep_ReportError(LOCKSTEP_API_INIT, LOCKSTEP_E_INIT_FAILED);
        return E_NOT_OK;
    }
    
    /* 设置状态 */
    Lockstep_State = LOCKSTEP_STATE_INIT;
    Lockstep_CurrentConfig = config;
    Lockstep_Mode = config->mode;
    
    /* 初始化统计 */
    Lockstep_Stats.mismatchCount = 0U;
    Lockstep_Stats.bistPassCount = 0U;
    Lockstep_Stats.bistFailCount = 0U;
    Lockstep_Stats.recoveryCount = 0U;
    Lockstep_Stats.lastErrorCode = LOCKSTEP_E_NO_ERROR;
    Lockstep_Stats.uptimeSeconds = 0U;
    
    Lockstep_ConsecutiveErrors = 0U;
    Lockstep_MonitorTimer = 0U;
    
    /* 平台初始化 */
    result = Lockstep_PlatformInit(config);
    
    if (E_OK == result)
    {
        /* 设置安全魔数 (表示初始化完成) */
        Lockstep_SafetyMagic = LOCKSTEP_SAFETY_MAGIC_INIT;
        
        if (LOCKSTEP_MODE_ENABLED == config->mode)
        {
            Lockstep_State = LOCKSTEP_STATE_ACTIVE;
            Lockstep_SafetyMagic = LOCKSTEP_SAFETY_MAGIC_ACTIVE;
        }
        else
        {
            Lockstep_State = LOCKSTEP_STATE_SPLIT;
        }
        
        Lockstep_NotifyEvent(LOCKSTEP_EVENT_STATE_CHANGE, (uint32)Lockstep_State);
    }
    else
    {
        Lockstep_State = LOCKSTEP_STATE_ERROR;
        Lockstep_SafetyMagic = 0U;
        Lockstep_ReportError(LOCKSTEP_API_INIT, LOCKSTEP_E_INIT_FAILED);
    }
    
    return result;
}

/**
 * @brief 去初始化Lockstep模块
 */
Std_ReturnType Lockstep_DeInit(void)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        Lockstep_ReportError(LOCKSTEP_API_DEINIT, LOCKSTEP_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif
    
    /* 平台去初始化 */
    Lockstep_PlatformDeInit();
    
    /* 重置状态 */
    Lockstep_State = LOCKSTEP_STATE_UNINIT;
    Lockstep_Mode = LOCKSTEP_MODE_DISABLED;
    Lockstep_CurrentConfig = NULL_PTR;
    Lockstep_SafetyMagic = 0U;
    Lockstep_Callback = NULL_PTR;
    Lockstep_CallbackContext = NULL_PTR;
    
    result = E_OK;
    
    return result;
}

/**
 * @brief 获取当前状态
 */
Lockstep_StateType Lockstep_GetState(void)
{
    return Lockstep_State;
}

/**
 * @brief 获取当前模式
 */
Lockstep_ModeType Lockstep_GetMode(void)
{
    return Lockstep_Mode;
}

/**
 * @brief 设置锁步模式
 * @ASIL-D: Requires privilege verification
 */
Std_ReturnType Lockstep_SetMode(Lockstep_ModeType mode)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        Lockstep_ReportError(LOCKSTEP_API_SET_MODE, LOCKSTEP_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif
    
    /* 检查是否允许切换 */
    if (Lockstep_State == LOCKSTEP_STATE_ERROR)
    {
        /* 错误状态下只允许切换到禁用 */
        if (mode != LOCKSTEP_MODE_DISABLED)
        {
            return E_NOT_OK;
        }
    }
    
    /* 调用平台API切换模式 */
    result = Platform_Lockstep_SetMode(mode);
    
    if (E_OK == result)
    {
        Lockstep_Mode = mode;
        
        if (LOCKSTEP_MODE_ENABLED == mode)
        {
            Lockstep_State = LOCKSTEP_STATE_ACTIVE;
            Lockstep_SafetyMagic = LOCKSTEP_SAFETY_MAGIC_ACTIVE;
        }
        else if (LOCKSTEP_MODE_DISABLED == mode)
        {
            Lockstep_State = LOCKSTEP_STATE_DEGRADED;
        }
        else
        {
            Lockstep_State = LOCKSTEP_STATE_SPLIT;
        }
        
        Lockstep_NotifyEvent(LOCKSTEP_EVENT_STATE_CHANGE, (uint32)Lockstep_State);
    }
    
    return result;
}

/**
 * @brief 运行BIST自测试
 * @ASIL-D: Built-in self test
 */
Std_ReturnType Lockstep_RunBist(uint32 timeoutMs)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        Lockstep_ReportError(LOCKSTEP_API_RUN_BIST, LOCKSTEP_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif
    
    /* 调用平台BIST函数 */
    result = Platform_Lockstep_RunBist(timeoutMs);
    
    if (E_OK == result)
    {
        Lockstep_Stats.bistPassCount++;
        Lockstep_NotifyEvent(LOCKSTEP_EVENT_BIST_COMPLETE, 0U);
    }
    else
    {
        Lockstep_Stats.bistFailCount++;
        Lockstep_NotifyEvent(LOCKSTEP_EVENT_BIST_FAILURE, LOCKSTEP_E_BIST_FAILURE);
        Lockstep_HandleError(LOCKSTEP_E_BIST_FAILURE);
    }
    
    return result;
}

/**
 * @brief 获取BIST结果
 */
Std_ReturnType Lockstep_GetBistResult(uint32* results)
{
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == results)
    {
        Lockstep_ReportError(LOCKSTEP_API_RUN_BIST, LOCKSTEP_E_INIT_FAILED);
        return E_NOT_OK;
    }
#endif
    
    return Platform_Lockstep_GetBistResult(results);
}

/**
 * @brief 手动触发锁步检查
 * @ASIL-D: Runtime safety check
 */
Std_ReturnType Lockstep_TriggerCheck(void)
{
    Std_ReturnType result = E_NOT_OK;
    boolean mismatchDetected = FALSE;
    
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        Lockstep_ReportError(LOCKSTEP_API_TRIGGER_CHECK, LOCKSTEP_E_INVALID_STATE);
        return E_NOT_OK;
    }
#endif
    
    /* 检查安全魔数 (软件级别检查) */
    if (LOCKSTEP_STATE_ACTIVE == Lockstep_State)
    {
        if (Lockstep_SafetyMagic != LOCKSTEP_SAFETY_MAGIC_ACTIVE)
        {
            mismatchDetected = TRUE;
        }
    }
    
    /* 调用平台硬件检查 */
    if (!mismatchDetected)
    {
        result = Platform_Lockstep_CheckStatus(&mismatchDetected);
    }
    
    if (mismatchDetected || (E_NOT_OK == result))
    {
        Lockstep_Stats.mismatchCount++;
        Lockstep_UpdateStatistics(TRUE, LOCKSTEP_E_MISMATCH_DETECTED);
        Lockstep_NotifyEvent(LOCKSTEP_EVENT_MISMATCH, LOCKSTEP_E_MISMATCH_DETECTED);
        Lockstep_HandleError(LOCKSTEP_E_MISMATCH_DETECTED);
        result = E_NOT_OK;
    }
    else
    {
        /* 检查通过，重置连续错误计数 */
        Lockstep_ConsecutiveErrors = 0U;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief 获取统计信息
 */
Std_ReturnType Lockstep_GetStatistics(Lockstep_StatisticsType* stats)
{
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == stats)
    {
        return E_NOT_OK;
    }
    
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif
    
    /* 安全拷贝统计信息 (防止越界) */
    stats->mismatchCount = Lockstep_Stats.mismatchCount;
    stats->bistPassCount = Lockstep_Stats.bistPassCount;
    stats->bistFailCount = Lockstep_Stats.bistFailCount;
    stats->recoveryCount = Lockstep_Stats.recoveryCount;
    stats->lastErrorCode = Lockstep_Stats.lastErrorCode;
    stats->uptimeSeconds = Lockstep_Stats.uptimeSeconds;
    
    return E_OK;
}

/**
 * @brief 清除统计信息
 */
Std_ReturnType Lockstep_ClearStatistics(void)
{
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        return E_NOT_OK;
    }
#endif
    
    Lockstep_Stats.mismatchCount = 0U;
    Lockstep_Stats.bistPassCount = 0U;
    Lockstep_Stats.bistFailCount = 0U;
    Lockstep_Stats.recoveryCount = 0U;
    Lockstep_Stats.lastErrorCode = LOCKSTEP_E_NO_ERROR;
    
    return E_OK;
}

/**
 * @brief 注册事件回调
 */
Std_ReturnType Lockstep_RegisterCallback(
    Lockstep_EventCallbackType callback,
    const void* context)
{
    Lockstep_Callback = callback;
    Lockstep_CallbackContext = context;
    return E_OK;
}

/**
 * @brief 主循环处理函数
 * @ASIL-D: Periodic safety monitoring
 */
void Lockstep_MainFunction(void)
{
    if (Lockstep_State == LOCKSTEP_STATE_UNINIT)
    {
        return;
    }
    
    /* 更新运行时间 */
    Lockstep_Stats.uptimeSeconds++;
    
    /* 检查监控周期 */
    Lockstep_MonitorTimer++;
    
    if (Lockstep_CurrentConfig != NULL_PTR)
    {
        if (Lockstep_MonitorTimer >= Lockstep_CurrentConfig->monitorPeriodMs)
        {
            Lockstep_MonitorTimer = 0U;
            
            /* 执行定期检查 */
            (void)Lockstep_TriggerCheck();
        }
    }
    
    /* 检查连续错误计数 */
    if (Lockstep_ConsecutiveErrors >= LOCKSTEP_ERROR_THRESHOLD)
    {
        Lockstep_EnterSafeState(LOCKSTEP_E_MISMATCH_DETECTED);
    }
}

/**
 * @brief 强制进入安全状态
 * @ASIL-D: Emergency safety response
 */
void Lockstep_EnterSafeState(uint32 reason)
{
    /* 记录错误 */
    Lockstep_Stats.lastErrorCode = reason;
    
    /* 设置错误状态 */
    Lockstep_State = LOCKSTEP_STATE_ERROR;
    
    /* 禁用锁步模式 */
    (void)Platform_Lockstep_SetMode(LOCKSTEP_MODE_DISABLED);
    Lockstep_Mode = LOCKSTEP_MODE_DISABLED;
    
    /* 通知事件 */
    Lockstep_NotifyEvent(LOCKSTEP_EVENT_STATE_CHANGE, reason);
    
    /* 触发系统安全响应 (调用EcuM或FCCU) */
    Platform_SafeState_Request(reason);
}

/**
 * @brief 验证内存区域CRC
 * @ASIL-D: Memory integrity check
 */
Std_ReturnType Lockstep_VerifyRegionCrc(uint8 regionIndex, uint32* crc)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == crc)
    {
        return E_NOT_OK;
    }
    
    if (Lockstep_CurrentConfig == NULL_PTR)
    {
        return E_NOT_OK;
    }
    
    if (regionIndex >= Lockstep_CurrentConfig->numMonitorRegions)
    {
        return E_NOT_OK;
    }
#endif
    
    if (Lockstep_CurrentConfig->monitorRegions != NULL_PTR)
    {
        const Lockstep_MonitorRegionType* region = 
            &Lockstep_CurrentConfig->monitorRegions[regionIndex];
        
        if (region->enableMonitor)
        {
            result = Platform_Crc_Calculate(
                (const uint8*)region->startAddress,
                region->size,
                region->crc32Seed,
                crc
            );
        }
    }
    
    return result;
}

#if (LOCKSTEP_VERSION_INFO_API == STD_ON)
/**
 * @brief 获取版本信息
 */
void Lockstep_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (NULL_PTR != versioninfo)
    {
        versioninfo->vendorID = LOCKSTEP_VENDOR_ID;
        versioninfo->moduleID = LOCKSTEP_MODULE_ID;
        versioninfo->sw_major_version = LOCKSTEP_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = LOCKSTEP_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = LOCKSTEP_SW_PATCH_VERSION;
    }
}
#endif

/*==================================================================================================
*                                       静态函数实现
==================================================================================================*/

/**
 * @brief 报告错误
 */
STATIC void Lockstep_ReportError(uint8 apiId, uint8 errorId)
{
#if (LOCKSTEP_DEV_ERROR_DETECT == STD_ON)
    (void)Det_ReportError(LOCKSTEP_MODULE_ID, 0U, apiId, errorId);
#endif
}

/**
 * @brief 通知事件
 */
STATIC void Lockstep_NotifyEvent(Lockstep_EventType event, uint32 errorCode)
{
    if (Lockstep_Callback != NULL_PTR)
    {
        Lockstep_Callback(event, errorCode, Lockstep_CallbackContext);
    }
}

/**
 * @brief 验证配置
 */
STATIC Std_ReturnType Lockstep_ValidateConfig(const Lockstep_ConfigType* config)
{
    if (config->numMonitorRegions > LOCKSTEP_MAX_MONITOR_REGIONS)
    {
        return E_NOT_OK;
    }
    
    if (config->errorThreshold == 0U)
    {
        return E_NOT_OK;
    }
    
    if (config->monitorPeriodMs == 0U)
    {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief 更新统计信息
 */
STATIC void Lockstep_UpdateStatistics(boolean isError, uint32 errorCode)
{
    if (isError)
    {
        Lockstep_Stats.lastErrorCode = errorCode;
        Lockstep_ConsecutiveErrors++;
    }
}

/**
 * @brief 处理错误
 */
STATIC void Lockstep_HandleError(uint32 errorCode)
{
    if (Lockstep_ConsecutiveErrors >= LOCKSTEP_ERROR_THRESHOLD)
    {
        Lockstep_EnterSafeState(errorCode);
    }
}

/**
 * @brief 平台初始化
 */
STATIC Std_ReturnType Lockstep_PlatformInit(const Lockstep_ConfigType* config)
{
    return Platform_Lockstep_Init(config);
}

/**
 * @brief 平台去初始化
 */
STATIC void Lockstep_PlatformDeInit(void)
{
    Platform_Lockstep_DeInit();
}

#define LOCKSTEP_STOP_SEC_CODE
#include "Lockstep_MemMap.h"
