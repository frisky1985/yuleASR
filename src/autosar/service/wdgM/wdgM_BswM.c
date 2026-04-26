/******************************************************************************
 * @file    wdgM_BswM.c
 * @brief   Watchdog Manager - BswM Integration Implementation
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "autosar/service/wdgM/wdgM_BswM.h"

/******************************************************************************
 * BswM Interface Functions
 ******************************************************************************/

/**
 * @brief BswM requests WdgM mode change
 */
void BswM_WdgM_RequestMode(WdgM_ModeType mode)
{
    /* Forward to WdgM */
    WdgM_RequestMode(mode);
}

/**
 * @brief Get current WdgM mode for BswM
 */
WdgM_ModeType BswM_WdgM_GetCurrentMode(void)
{
    return WdgM_GetCurrentMode();
}

/**
 * @brief BswM notification callback for WdgM status change
 */
void BswM_WdgM_StatusNotification(
    WdgM_GlobalStatusType newStatus,
    WdgM_GlobalStatusType oldStatus)
{
    (void)newStatus;
    (void)oldStatus;
    
    /* BswM can use this to trigger mode changes or error handling */
    /* Example: Transition to safe mode if WdgM reports EXPIRED */
    if (newStatus == WDGM_GLOBAL_STATUS_EXPIRED) {
        /* Trigger BswM rule for safety mode */
        /* This would typically call BswM_RequestMode() or similar */
    }
}

/******************************************************************************
 * WdgM Action Functions for BswM Action Lists
 ******************************************************************************/

/**
 * @brief BswM Action: Set WdgM mode
 */
Std_ReturnType WdgM_BswM_SetModeAction(WdgM_ModeType mode)
{
    return WdgM_SetMode(mode);
}

/**
 * @brief BswM Action: Enable WdgM supervision
 */
Std_ReturnType WdgM_BswM_EnableSupervision(void)
{
    Std_ReturnType result = E_NOT_OK;
    
    /* Only enable if already initialized */
    if (WdgM_GetGlobalStatus() != WDGM_GLOBAL_STATUS_STOPPED) {
        /* Re-enable supervision by setting current mode again */
        WdgM_ModeType currentMode;
        result = WdgM_GetMode(&currentMode);
        if (result == E_OK) {
            result = WdgM_SetMode(currentMode);
        }
    }
    
    return result;
}

/**
 * @brief BswM Action: Disable WdgM supervision
 */
Std_ReturnType WdgM_BswM_DisableSupervision(void)
{
    return WdgM_SetMode(WDGM_MODE_OFF);
}
