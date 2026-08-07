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
 * @file Wdg.c
 * @brief WDG Driver implementation for i.MX8M Mini (WDOG)
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "Wdg.h"
#include "Wdg_Cfg.h"
#include "Det.h"
#include <stdint.h>

#ifdef S32K312
#include "S32K312.h"
#include <stdint.h>
#define WDG_WDOG1_BASE_ADDR             (S32K312_WDOG_BASE)
#define WDG_WDOG2_BASE_ADDR             (S32K312_WDOG_BASE)
#define WDG_WDOG3_BASE_ADDR             (S32K312_WDOG_BASE)
#define WDG_WCR                         WDOG_CS_OFF
#define WDG_WSR                         (0x02U)
#define WDG_WRSR                        (0x04)
#define WDG_WICR                        (0x06U)
#define WDG_WMCR                        (0x08)
#else
#define WDG_WDOG1_BASE_ADDR             (0x30280000UL)
#define WDG_WDOG2_BASE_ADDR             (0x30290000UL)
#define WDG_WDOG3_BASE_ADDR             (0x302A0000UL)
#define WDG_WCR                         (0x00)
#define WDG_WSR                         (0x02)
#define WDG_WRSR                        (0x04)
#define WDG_WICR                        (0x06)
#define WDG_WMCR                        (0x08)
#endif

#define WDG_WCR_WDE                     (0x0004U)
#define WDG_WCR_WDZST                   (0x0008U)
#define WDG_WCR_WDBG                    (0x0010U)
#define WDG_WCR_WDT                     (0x0020U)
#define WDG_WCR_SRS                     (0x0040U)
#define WDG_WCR_WDA                     (0x0080U)
#define WDG_WCR_WT_MASK                 (0xFF00U)

#define WDG_WSR_SEQ1                    (0x5555U)
#define WDG_WSR_SEQ2                    (0xAAAAU)

#define WDG_WRSR_SFTW                   (0x0001U)
#define WDG_WRSR_TOUT                   (0x0002U)
#define WDG_WRSR_POR                    (0x0010U)

#define WDG_WICR_WIE                    (0x0001U)
#define WDG_WICR_WTIS                   (0x0002U)
#define WDG_WICR_WICT_MASK              (0xFF00U)

#define WDG_WMCR_PDE                    (0x0001U)

#define WDG_TIMEOUT_VALUE_MS            (1000U)
#define WDG_CLOCK_FREQ_HZ               (32000U)

#define WDG_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean Wdg_DriverInitialized = FALSE;
static WdgIf_ModeType Wdg_CurrentMode = WDGIF_OFF_MODE;
static Wdg_TimeoutType Wdg_CurrentTimeout = 0U;
static const Wdg_ConfigType* Wdg_ConfigPtr = NULL_PTR;
static Wdg_StateType Wdg_ModuleState = WDG_STATE_UNINIT;  /* 新增状态机 */
static uint32 Wdg_TriggerCounter = 0U;  /* 触发计数器 */
static uint32 Wdg_LastTriggerTime = 0U;  /* 上次触发时间戳 */

#define WDG_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static uint32 Wdg_GetBaseAddr(void)
{
    return WDG_WDOG1_BASE_ADDR;
}

static void Wdg_EnableClock(void)
{
}

static void Wdg_DisableClock(void)
{
}

static uint16 Wdg_CalculateTimeoutValue(Wdg_TimeoutType timeoutMs)
{
    /* WDOG timeout = (WCR[WT] + 1) * 2 / WDOG clock frequency */
    /* WCR[WT] = (timeoutMs * clockFreq / 2000) - 1 */
    
    /* 防止溢出 */
    if (timeoutMs == 0U) {
        return 0U;
    }
    
    uint64 intermediate = (uint64)timeoutMs * WDG_CLOCK_FREQ_HZ;
    
    /* 检查中间值是否溢出 */
    if (intermediate > UINT32_MAX) {
        /* 使用最大值 */
        return 0xFFU;
    }
    
    uint32 wtValue = (uint32)(intermediate / 2000U) - 1U;
    
    /* 边界检查 */
    if (wtValue > 0xFFU) {
        wtValue = 0xFFU;
    }
    
    return (uint16)wtValue;
}

/**
 * @brief 验证窗口模式触发是否合法
 * @param currentTime 当前时间戳(ms)
 * @param modeSettings 当前模式配置
 * @return Wdg_TriggerResultType 触发结果
 */
static Wdg_TriggerResultType Wdg_ValidateWindowTrigger(
    uint32 currentTime,
    const Wdg_ModeSettingsType* modeSettings)
{
    if (modeSettings->WindowModeEnabled == FALSE) {
        return WDG_TRIGGER_OK;  /* 非窗口模式，直接允许 */
    }
    
    uint32 timeSinceLastTrigger = currentTime - Wdg_LastTriggerTime;
    
    /* 检查是否在窗口期前 */
    if (timeSinceLastTrigger < modeSettings->WindowStart) {
        return WDG_TRIGGER_WINDOW_EARLY;
    }
    
    /* 检查是否在窗口期后 */
    if (timeSinceLastTrigger > modeSettings->WindowEnd) {
        return WDG_TRIGGER_WINDOW_LATE;
    }
    
    return WDG_TRIGGER_OK;  /* 在窗口期内 */
}

/**
 * @brief 获取当前系统时间戳
 * @return 当前时间戳(ms)
 */
static uint32 Wdg_GetCurrentTimeMs(void)
{
    /* 时间戳获取 - 依赖系统定时器集成 (可用 GPT 定时器或系统计数器) */
    return 0U;
}

/**
 * @brief 调用超时前预警回调
 */
static void Wdg_InvokePreWarningCallback(uint32 TimeRemainingUs)
{
    if ((Wdg_ConfigPtr != NULL_PTR) && 
        (Wdg_ConfigPtr->PreWarningCallback != NULL_PTR)) {
        Wdg_ConfigPtr->PreWarningCallback(TimeRemainingUs);
    }
}

/**
 * @brief 调用窗口违规回调
 */
static void Wdg_InvokeWindowViolationCallback(void)
{
    if ((Wdg_ConfigPtr != NULL_PTR) && 
        (Wdg_ConfigPtr->WindowViolationCallback != NULL_PTR)) {
        Wdg_ConfigPtr->WindowViolationCallback();
    }
}

#define WDG_START_SEC_CODE
#include "MemMap.h"

void Wdg_Init(const Wdg_ConfigType* ConfigPtr)
{
    #if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_INIT, WDG_E_PARAM_CONFIG);
        return;
    }
    if (Wdg_DriverInitialized == TRUE) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_INIT, WDG_E_ALREADY_INITIALIZED);
        return;
    }
    #endif

    Wdg_ConfigPtr = ConfigPtr;

    uint32 baseAddr = Wdg_GetBaseAddr();

    Wdg_EnableClock();

    /* Disable watchdog during configuration */
    uint16 wcrValue = REG_READ16(baseAddr + WDG_WCR);
    wcrValue &= ~WDG_WCR_WDE;
    REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

    /* Configure timeout */
    Wdg_TimeoutType timeout;
    const Wdg_ModeSettingsType* modeSettings;
    
    if (ConfigPtr->InitialMode == WDGIF_FAST_MODE) {
        modeSettings = &ConfigPtr->FastModeSettings;
    } else if (ConfigPtr->InitialMode == WDGIF_SLOW_MODE) {
        modeSettings = &ConfigPtr->SlowModeSettings;
    } else {
        modeSettings = NULL_PTR;  /* OFF mode */
    }
    
    if (modeSettings != NULL_PTR) {
        timeout = modeSettings->TimeoutPeriod;
        uint16 wtValue = Wdg_CalculateTimeoutValue(timeout);
        wcrValue = REG_READ16(baseAddr + WDG_WCR);
        wcrValue &= ~WDG_WCR_WT_MASK;
        wcrValue |= ((uint16)wtValue << 8) & WDG_WCR_WT_MASK;
        REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

        /* Configure interrupt and window mode if enabled */
        if (modeSettings->InterruptMode) {
            uint16 wicrValue = WDG_WICR_WIE;
            wicrValue |= ((uint16)wtValue << 8) & WDG_WICR_WICT_MASK;
            REG_WRITE16(baseAddr + WDG_WICR, wicrValue);
        }
    } else {
        timeout = 0U;
    }

    /* Enable watchdog if not OFF mode */
    if (ConfigPtr->InitialMode != WDGIF_OFF_MODE) {
        wcrValue = REG_READ16(baseAddr + WDG_WCR);
        wcrValue |= WDG_WCR_WDE;
        wcrValue |= WDG_WCR_WDT; /* Enable time-out assertion */
        REG_WRITE16(baseAddr + WDG_WCR, wcrValue);
        
        Wdg_ModuleState = WDG_STATE_RUNNING;
    } else {
        Wdg_ModuleState = WDG_STATE_IDLE;
    }

    Wdg_CurrentMode = ConfigPtr->InitialMode;
    Wdg_CurrentTimeout = timeout;
    Wdg_TriggerCounter = 0U;
    Wdg_LastTriggerTime = Wdg_GetCurrentTimeMs();
    Wdg_DriverInitialized = TRUE;
}

Std_ReturnType Wdg_SetMode(WdgIf_ModeType Mode)
{
    #if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (Wdg_DriverInitialized == FALSE) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETMODE, WDG_E_UNINIT);
        return E_NOT_OK;
    }
    
    /* 检查模式转换合法性 */
    if ((Wdg_CurrentMode == WDGIF_OFF_MODE) && 
        (Mode != WDGIF_FAST_MODE) && (Mode != WDGIF_SLOW_MODE)) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETMODE, WDG_E_MODE_TRANSITION);
        return E_NOT_OK;
    }
    #endif

    #if (WDG_DISABLE_ALLOWED == STD_OFF)
    if (Mode == WDGIF_OFF_MODE) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETMODE, WDG_E_DISABLE_NOT_ALLOWED);
        return E_NOT_OK;
    }
    #endif

    uint32 baseAddr = Wdg_GetBaseAddr();
    uint16 wcrValue = REG_READ16(baseAddr + WDG_WCR);
    const Wdg_ModeSettingsType* modeSettings;

    switch (Mode) {
        case WDGIF_OFF_MODE:
            /* Disable watchdog */
            wcrValue &= ~WDG_WCR_WDE;
            REG_WRITE16(baseAddr + WDG_WCR, wcrValue);
            Wdg_ModuleState = WDG_STATE_IDLE;
            modeSettings = NULL_PTR;
            break;

        case WDGIF_SLOW_MODE:
            modeSettings = &Wdg_ConfigPtr->SlowModeSettings;
            break;

        case WDGIF_FAST_MODE:
            modeSettings = &Wdg_ConfigPtr->FastModeSettings;
            break;

        default:
            #if (WDG_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETMODE, WDG_E_PARAM_MODE);
            #endif
            return E_NOT_OK;
    }
    
    /* 配置新的模式 */
    if (modeSettings != NULL_PTR) {
        /* Enable watchdog */
        wcrValue |= WDG_WCR_WDE;
        REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

        /* Update timeout */
        uint16 wtValue = Wdg_CalculateTimeoutValue(modeSettings->TimeoutPeriod);
        wcrValue = REG_READ16(baseAddr + WDG_WCR);
        wcrValue &= ~WDG_WCR_WT_MASK;
        wcrValue |= ((uint16)wtValue << 8) & WDG_WCR_WT_MASK;
        REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

        Wdg_CurrentTimeout = modeSettings->TimeoutPeriod;
        Wdg_ModuleState = WDG_STATE_RUNNING;
    }

    Wdg_CurrentMode = Mode;
    Wdg_LastTriggerTime = Wdg_GetCurrentTimeMs();  /* 重置触发时间 */
    return E_OK;
}

void Wdg_Trigger(void)
{
    #if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (Wdg_DriverInitialized == FALSE) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_TRIGGER, WDG_E_UNINIT);
        return;
    }
    #endif

    if (Wdg_CurrentMode == WDGIF_OFF_MODE) {
        #if (WDG_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_TRIGGER, WDG_E_FORBIDDEN_INVOCATION);
        #endif
        return;
    }
    
    if (Wdg_ModuleState != WDG_STATE_RUNNING) {
        return;  /* 不在运行状态，忽略触发 */
    }

    uint32 baseAddr = Wdg_GetBaseAddr();
    uint32 currentTime = Wdg_GetCurrentTimeMs();
    
    /* 窗口模式验证 */
    const Wdg_ModeSettingsType* modeSettings;
    if (Wdg_CurrentMode == WDGIF_FAST_MODE) {
        modeSettings = &Wdg_ConfigPtr->FastModeSettings;
    } else {
        modeSettings = &Wdg_ConfigPtr->SlowModeSettings;
    }
    
    #if (WDG_VALIDATE_WINDOW_MODE == STD_ON)
    Wdg_TriggerResultType triggerResult = Wdg_ValidateWindowTrigger(currentTime, modeSettings);
    
    if (triggerResult != WDG_TRIGGER_OK) {
        /* 窗口违规 */
        #if (WDG_DEV_ERROR_DETECT == STD_ON)
        if (triggerResult == WDG_TRIGGER_WINDOW_EARLY) {
            Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_TRIGGER, WDG_E_WINDOW_VIOLATION);
        } else if (triggerResult == WDG_TRIGGER_WINDOW_LATE) {
            Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_TRIGGER, WDG_E_WINDOW_VIOLATION);
        }
        #endif
        
        /* 调用窗口违规回调 */
        Wdg_InvokeWindowViolationCallback();
        
        /* 根据配置决定是否执行复位 */
        #if (WDG_WINDOW_ERROR_ACTION == WDG_WINDOW_ERROR_RESET)
        /* 触发系统复位 - 复位功能通过平台 Wdg_Platform_Reset() 实现 */
        #endif
        
        return;
    }
    #endif
    
    /* 检查是否接近超时，调用预警回调 */
    #if (WDG_TIMEOUT_PRE_WARNING == STD_ON)
    if (modeSettings->InterruptMode && (modeSettings->TimeoutPreWarningUs > 0U)) {
        uint32 timeSinceLastTrigger = currentTime - Wdg_LastTriggerTime;
        uint32 timeoutRemaining = modeSettings->TimeoutPeriod - timeSinceLastTrigger;
        
        /* 如果剩余时间小于预警时间，调用回调 */
        uint32 timeoutRemainingUs = timeoutRemaining * 1000U;
        if (timeoutRemainingUs <= modeSettings->TimeoutPreWarningUs) {
            Wdg_InvokePreWarningCallback(timeoutRemainingUs);
        }
    }
    #endif

    /* Service sequence: write 0x5555 then 0xAAAA */
    REG_WRITE16(baseAddr + WDG_WSR, WDG_WSR_SEQ1);
    REG_WRITE16(baseAddr + WDG_WSR, WDG_WSR_SEQ2);
    
    /* 更新触发记录 */
    Wdg_TriggerCounter++;
    Wdg_LastTriggerTime = currentTime;
}

void Wdg_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_GETVERSIONINFO, WDG_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = WDG_VENDOR_ID;
    versioninfo->moduleID = WDG_MODULE_ID;
    versioninfo->sw_major_version = WDG_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = WDG_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = WDG_SW_PATCH_VERSION;
}

Std_ReturnType Wdg_SetTriggerCondition(uint16 timeout)
{
    #if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (Wdg_DriverInitialized == FALSE) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETTRIGGERCONDITION, WDG_E_UNINIT);
        return E_NOT_OK;
    }
    #endif

    if (Wdg_CurrentMode == WDGIF_OFF_MODE) {
        return E_NOT_OK;
    }

    /* Validate timeout */
    if (timeout > WDG_MAX_TIMEOUT) {
        #if (WDG_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETTRIGGERCONDITION, WDG_E_PARAM_TIMEOUT);
        #endif
        return E_NOT_OK;
    }
    if (timeout < WDG_MIN_TIMEOUT) {
        #if (WDG_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETTRIGGERCONDITION, WDG_E_PARAM_TIMEOUT);
        #endif
        return E_NOT_OK;
    }

    uint32 baseAddr = Wdg_GetBaseAddr();

    /* Update timeout value */
    uint16 wtValue = Wdg_CalculateTimeoutValue(timeout);
    uint16 wcrValue = REG_READ16(baseAddr + WDG_WCR);
    wcrValue &= ~WDG_WCR_WT_MASK;
    wcrValue |= ((uint16)wtValue << 8) & WDG_WCR_WT_MASK;
    REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

    /* Trigger watchdog */
    Wdg_Trigger();

    Wdg_CurrentTimeout = timeout;
    return E_OK;
}

/**
 * @brief 获取看门狗当前状态
 * @return Wdg_StateType 当前状态
 */
Wdg_StateType Wdg_GetStatus(void)
{
    return Wdg_ModuleState;
}

/**
 * @brief 获取触发计数器值
 * @return uint32 触发次数
 */
uint32 Wdg_GetTriggerCounter(void)
{
    return Wdg_TriggerCounter;
}

/**
 * @brief 获取上次触发时间
 * @return uint32 上次触发时间戳(ms)
 */
uint32 Wdg_GetLastTriggerTime(void)
{
    return Wdg_LastTriggerTime;
}

#define WDG_STOP_SEC_CODE
#include "MemMap.h"
