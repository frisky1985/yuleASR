/** @file WdgIf.c
 *  @brief Watchdog Interface implementation (AUTOSAR R22-11)
 *  @copyright Copyright (c) 2026 YuleTech
 *
 *  @implements AUTOSAR_SWS_WatchdogInterface.pdf
 */

#include "WdgIf.h"
#include "WdgIf_Cfg.h"
#include "Det.h"

#define WDGIF_NUM_DEVICES              4U

typedef enum {
    WDGIF_INTERNAL_UNINIT = 0,
    WDGIF_INTERNAL_INIT,
    WDGIF_INTERNAL_ONLINE
} WdgIf_InternalStateType;

typedef struct {
    WdgIf_InternalStateType internalState;
    uint8 deviceCount;
    const WdgIf_ConfigType* configPtr;
} WdgIf_InternalType;

static WdgIf_InternalType WdgIf_State = {
    WDGIF_INTERNAL_UNINIT,
    0U,
    NULL_PTR
};

/* Forward declarations for WDG MCAL driver functions */
extern void Wdg_Init(void);
extern Std_ReturnType Wdg_SetMode(WdgIf_ModeType Mode);
extern void Wdg_DeInit(void);

void WdgIf_Init(const WdgIf_ConfigType* ConfigPtr)
{
#if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_INIT, WDGIF_E_INV_POINTER);
        return;
    }
#endif
    WdgIf_State.configPtr = ConfigPtr;
    WdgIf_State.deviceCount = ConfigPtr->DeviceCount;
    WdgIf_State.internalState = WDGIF_INTERNAL_INIT;
}

void WdgIf_DeInit(void)
{
    WdgIf_State.internalState = WDGIF_INTERNAL_UNINIT;
    WdgIf_State.configPtr = NULL_PTR;
    WdgIf_State.deviceCount = 0U;
}

Std_ReturnType WdgIf_SetMode(WdgIf_DeviceType Device, WdgIf_ModeType WdgMode)
{
#if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_State.internalState < WDGIF_INTERNAL_INIT) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_SETMODE, WDGIF_E_DRIVER_UNINIT);
        return E_NOT_OK;
    }
    if (Device >= WdgIf_State.deviceCount) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_SETMODE, WDGIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
    if (WdgMode > WDGIF_FAST_MODE) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_SETMODE, WDGIF_E_PARAM_MODE);
        return E_NOT_OK;
    }
#endif
    (void)Device;
    return Wdg_SetMode(WdgMode);
}

Std_ReturnType WdgIf_Trigger(WdgIf_DeviceType Device)
{
#if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_State.internalState < WDGIF_INTERNAL_INIT) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_TRIGGER, WDGIF_E_DRIVER_UNINIT);
        return E_NOT_OK;
    }
    if (Device >= WdgIf_State.deviceCount) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_TRIGGER, WDGIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

Std_ReturnType WdgIf_SetTriggerCondition(WdgIf_DeviceType Device, WdgIf_TimeoutType Timeout)
{
#if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (WdgIf_State.internalState < WDGIF_INTERNAL_INIT) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_SETTRIGGERCONDITION, WDGIF_E_DRIVER_UNINIT);
        return E_NOT_OK;
    }
    if (Device >= WdgIf_State.deviceCount) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_SETTRIGGERCONDITION, WDGIF_E_PARAM_DEVICE);
        return E_NOT_OK;
    }
#endif
    (void)Timeout;
    return E_OK;
}

#if (WDGIF_VERSION_INFO_API == STD_ON)
void WdgIf_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
#if (WDGIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(WDGIF_MODULE_ID, 0U, WDGIF_SID_GETVERSIONINFO, WDGIF_E_INV_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = WDGIF_VENDOR_ID;
    VersionInfo->moduleID = WDGIF_MODULE_ID;
    VersionInfo->sw_major_version = WDGIF_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = WDGIF_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = WDGIF_SW_PATCH_VERSION;
}
#endif