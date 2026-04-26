/******************************************************************************
 * @file    wdgM_BswM.h
 * @brief   Watchdog Manager - BswM Integration Interface
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGM_BSWM_H
#define WDGM_BSWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/autosar_types.h"
#include "autosar/service/wdgM/wdgM.h"

/******************************************************************************
 * BswM Mode Request Port for WdgM
 ******************************************************************************/

typedef enum {
    WDGM_BSWM_MODE_OFF = 0,
    WDGM_BSWM_MODE_SLOW,
    WDGM_BSWM_MODE_FAST
} WdgM_BswMModeType;

/******************************************************************************
 * BswM Interface Functions
 ******************************************************************************/

/**
 * @brief BswM requests WdgM mode change
 * @param mode Requested WdgM mode
 */
void BswM_WdgM_RequestMode(WdgM_ModeType mode);

/**
 * @brief Get current WdgM mode for BswM
 * @return Current WdgM mode
 */
WdgM_ModeType BswM_WdgM_GetCurrentMode(void);

/**
 * @brief BswM notification callback for WdgM status change
 * @param newStatus New global status
 * @param oldStatus Old global status
 */
void BswM_WdgM_StatusNotification(
    WdgM_GlobalStatusType newStatus,
    WdgM_GlobalStatusType oldStatus
);

/******************************************************************************
 * WdgM Action Functions for BswM Action Lists
 ******************************************************************************/

/**
 * @brief BswM Action: Set WdgM mode
 * @param mode Mode to set
 * @return E_OK if successful
 */
Std_ReturnType WdgM_BswM_SetModeAction(WdgM_ModeType mode);

/**
 * @brief BswM Action: Enable WdgM supervision
 * @return E_OK if successful
 */
Std_ReturnType WdgM_BswM_EnableSupervision(void);

/**
 * @brief BswM Action: Disable WdgM supervision
 * @return E_OK if successful
 */
Std_ReturnType WdgM_BswM_DisableSupervision(void);

#ifdef __cplusplus
}
#endif

#endif /* WDGM_BSWM_H */
