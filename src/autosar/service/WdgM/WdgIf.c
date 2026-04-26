/******************************************************************************
 * @file    wdgIf.c
 * @brief   Watchdog Interface (WdgIf) Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/service/wdgM/wdgIf.h"
#include <string.h>

/******************************************************************************
 * Module Internal Constants
 ******************************************************************************/
#define WDGIF_MAX_DEVICES               1U
#define WDGIF_INVALID_MODE              0xFFU

/******************************************************************************
 * Module Variables
 ******************************************************************************/
static WdgIf_ModeType WdgIf_CurrentMode = WDGIF_OFF_MODE;
static boolean WdgIf_Initialized = FALSE;

/******************************************************************************
 * External Functions (from Wdg Driver - to be linked)
 ******************************************************************************/
/* These would normally be provided by the Wdg driver */
extern Std_ReturnType Wdg_SetMode(WdgIf_ModeType mode);
extern void Wdg_Trigger(void);
extern Std_ReturnType Wdg_GetMode(WdgIf_ModeType *mode);

/******************************************************************************
 * Public API Functions
 ******************************************************************************/

/**
 * @brief Initialize WdgIf
 */
Std_ReturnType WdgIf_Init(void)
{
    Std_ReturnType result = E_NOT_OK;

    if (!WdgIf_Initialized) {
        WdgIf_CurrentMode = WDGIF_OFF_MODE;
        WdgIf_Initialized = TRUE;
        result = E_OK;
    }

    return result;
}

/**
 * @brief Deinitialize WdgIf
 */
Std_ReturnType WdgIf_DeInit(void)
{
    Std_ReturnType result = E_NOT_OK;

    if (WdgIf_Initialized) {
        /* Set watchdog to OFF mode before deinitialization */
        (void)WdgIf_SetMode(WDGIF_DEFAULT_DEVICE, WDGIF_OFF_MODE);
        WdgIf_Initialized = FALSE;
        result = E_OK;
    }

    return result;
}

/**
 * @brief Set watchdog mode
 */
Std_ReturnType WdgIf_SetMode(WdgIf_DeviceIndexType deviceIndex, WdgIf_ModeType mode)
{
    Std_ReturnType result = E_NOT_OK;

    /* Check initialization */
    if (!WdgIf_Initialized) {
        return E_NOT_OK;
    }

    /* Check device index */
    if (deviceIndex >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    /* Check mode validity */
    if (mode > WDGIF_FAST_MODE) {
        return E_NOT_OK;
    }

    /* Call driver function (stubbed for now) */
    #ifdef WDG_DRIVER_AVAILABLE
    result = Wdg_SetMode(mode);
    #else
    /* Stub implementation - always succeed */
    WdgIf_CurrentMode = mode;
    result = E_OK;
    #endif

    if (result == E_OK) {
        WdgIf_CurrentMode = mode;
    }

    return result;
}

/**
 * @brief Trigger watchdog
 */
void WdgIf_Trigger(WdgIf_DeviceIndexType deviceIndex)
{
    /* Check initialization */
    if (!WdgIf_Initialized) {
        return;
    }

    /* Check device index */
    if (deviceIndex >= WDGIF_MAX_DEVICES) {
        return;
    }

    /* Check if watchdog is enabled */
    if (WdgIf_CurrentMode == WDGIF_OFF_MODE) {
        return;
    }

    /* Call driver function (stubbed for now) */
    #ifdef WDG_DRIVER_AVAILABLE
    Wdg_Trigger();
    #else
    /* Stub implementation - no action */
    #endif
}

/**
 * @brief Get current watchdog mode
 */
Std_ReturnType WdgIf_GetMode(WdgIf_DeviceIndexType deviceIndex, WdgIf_ModeType *mode)
{
    Std_ReturnType result = E_NOT_OK;

    /* Check parameters */
    if (mode == NULL) {
        return E_NOT_OK;
    }

    /* Check initialization */
    if (!WdgIf_Initialized) {
        return E_NOT_OK;
    }

    /* Check device index */
    if (deviceIndex >= WDGIF_MAX_DEVICES) {
        return E_NOT_OK;
    }

    #ifdef WDG_DRIVER_AVAILABLE
    result = Wdg_GetMode(mode);
    #else
    /* Stub implementation */
    *mode = WdgIf_CurrentMode;
    result = E_OK;
    #endif

    return result;
}

/**
 * @brief Get version information
 */
void WdgIf_GetVersionInfo(Std_VersionInfoType *versionInfo)
{
    if (versionInfo != NULL) {
        versionInfo->vendorID = WDGIF_VENDOR_ID;
        versionInfo->moduleID = WDGIF_MODULE_ID;
        versionInfo->sw_major_version = WDGIF_SW_MAJOR_VERSION;
        versionInfo->sw_minor_version = WDGIF_SW_MINOR_VERSION;
        versionInfo->sw_patch_version = WDGIF_SW_PATCH_VERSION;
    }
}
