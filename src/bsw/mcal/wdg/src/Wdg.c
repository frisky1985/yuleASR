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

#define WDG_WDOG1_BASE_ADDR             (0x30280000UL)
#define WDG_WDOG2_BASE_ADDR             (0x30290000UL)
#define WDG_WDOG3_BASE_ADDR             (0x302A0000UL)

#define WDG_WCR                         (0x00)
#define WDG_WSR                         (0x02)
#define WDG_WRSR                        (0x04)
#define WDG_WICR                        (0x06)
#define WDG_WMCR                        (0x08)

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
    uint32 wtValue = ((uint32)timeoutMs * WDG_CLOCK_FREQ_HZ / 2000U) - 1U;
    if (wtValue > 0xFFU) {
        wtValue = 0xFFU;
    }
    return (uint16)wtValue;
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
    if (ConfigPtr->InitialMode == WDGIF_FAST_MODE) {
        timeout = ConfigPtr->FastModeSettings.TimeoutPeriod;
    } else {
        timeout = ConfigPtr->SlowModeSettings.TimeoutPeriod;
    }

    uint16 wtValue = Wdg_CalculateTimeoutValue(timeout);
    wcrValue = REG_READ16(baseAddr + WDG_WCR);
    wcrValue &= ~WDG_WCR_WT_MASK;
    wcrValue |= ((uint16)wtValue << 8) & WDG_WCR_WT_MASK;
    REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

    /* Configure interrupt if enabled */
    #if (WDG_FAST_MODE_INTERRUPT == STD_ON)
    if (ConfigPtr->FastModeSettings.InterruptMode) {
        uint16 wicrValue = WDG_WICR_WIE;
        wicrValue |= ((uint16)wtValue << 8) & WDG_WICR_WICT_MASK;
        REG_WRITE16(baseAddr + WDG_WICR, wicrValue);
    }
    #endif

    /* Enable watchdog if not OFF mode */
    if (ConfigPtr->InitialMode != WDGIF_OFF_MODE) {
        wcrValue = REG_READ16(baseAddr + WDG_WCR);
        wcrValue |= WDG_WCR_WDE;
        wcrValue |= WDG_WCR_WDT; /* Enable time-out assertion */
        REG_WRITE16(baseAddr + WDG_WCR, wcrValue);
    }

    Wdg_CurrentMode = ConfigPtr->InitialMode;
    Wdg_CurrentTimeout = timeout;
    Wdg_DriverInitialized = TRUE;
}

Std_ReturnType Wdg_SetMode(WdgIf_ModeType Mode)
{
    #if (WDG_DEV_ERROR_DETECT == STD_ON)
    if (Wdg_DriverInitialized == FALSE) {
        Det_ReportError(WDG_MODULE_ID, 0U, WDG_SID_SETMODE, WDG_E_UNINIT);
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

    switch (Mode) {
        case WDGIF_OFF_MODE:
            /* Disable watchdog */
            wcrValue &= ~WDG_WCR_WDE;
            REG_WRITE16(baseAddr + WDG_WCR, wcrValue);
            break;

        case WDGIF_SLOW_MODE:
            /* Enable with slow mode timeout */
            wcrValue |= WDG_WCR_WDE;
            REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

            /* Update timeout */
            uint16 wtValueSlow = Wdg_CalculateTimeoutValue(Wdg_ConfigPtr->SlowModeSettings.TimeoutPeriod);
            wcrValue = REG_READ16(baseAddr + WDG_WCR);
            wcrValue &= ~WDG_WCR_WT_MASK;
            wcrValue |= ((uint16)wtValueSlow << 8) & WDG_WCR_WT_MASK;
            REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

            Wdg_CurrentTimeout = Wdg_ConfigPtr->SlowModeSettings.TimeoutPeriod;
            break;

        case WDGIF_FAST_MODE:
            /* Enable with fast mode timeout */
            wcrValue |= WDG_WCR_WDE;
            REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

            /* Update timeout */
            uint16 wtValueFast = Wdg_CalculateTimeoutValue(Wdg_ConfigPtr->FastModeSettings.TimeoutPeriod);
            wcrValue = REG_READ16(baseAddr + WDG_WCR);
            wcrValue &= ~WDG_WCR_WT_MASK;
            wcrValue |= ((uint16)wtValueFast << 8) & WDG_WCR_WT_MASK;
            REG_WRITE16(baseAddr + WDG_WCR, wcrValue);

            Wdg_CurrentTimeout = Wdg_ConfigPtr->FastModeSettings.TimeoutPeriod;
            break;

        default:
            return E_NOT_OK;
    }

    Wdg_CurrentMode = Mode;
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
        return;
    }

    uint32 baseAddr = Wdg_GetBaseAddr();

    /* Service sequence: write 0x5555 then 0xAAAA */
    REG_WRITE16(baseAddr + WDG_WSR, WDG_WSR_SEQ1);
    REG_WRITE16(baseAddr + WDG_WSR, WDG_WSR_SEQ2);
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
        timeout = WDG_MAX_TIMEOUT;
    }
    if (timeout < WDG_MIN_TIMEOUT) {
        timeout = WDG_MIN_TIMEOUT;
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

#define WDG_STOP_SEC_CODE
#include "MemMap.h"
