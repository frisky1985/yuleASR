/******************************************************************************
 * @file    wdgIf.h
 * @brief   Watchdog Interface (WdgIf) Header
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_H
#define WDGIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/autosar_types.h"
#include "common/autosar_errors.h"
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * WdgIf Version Information
 ******************************************************************************/
#define WDGIF_VENDOR_ID                 0x01U
#define WDGIF_MODULE_ID                 0x11U  /* WdgIf module ID per AUTOSAR */
#define WDGIF_SW_MAJOR_VERSION          1U
#define WDGIF_SW_MINOR_VERSION          0U
#define WDGIF_SW_PATCH_VERSION          0U

/******************************************************************************
 * WdgIf Mode Types
 ******************************************************************************/
typedef enum {
    WDGIF_OFF_MODE = 0,                 /* Watchdog disabled */
    WDGIF_SLOW_MODE,                    /* Slow watchdog mode */
    WDGIF_FAST_MODE                     /* Fast watchdog mode */
} WdgIf_ModeType;

/******************************************************************************
 * WdgIf Service IDs
 ******************************************************************************/
#define WDGIF_SID_SETMODE               0x01U
#define WDGIF_SID_TRIGGER               0x02U
#define WDGIF_SID_GETMODE               0x03U
#define WDGIF_SID_GETVERSIONINFO        0x04U

/******************************************************************************
 * WdgIf Device Index
 ******************************************************************************/
typedef uint8 WdgIf_DeviceIndexType;
#define WDGIF_DEFAULT_DEVICE            0U

/******************************************************************************
 * WdgIf API Functions
 ******************************************************************************/

/**
 * @brief Set watchdog mode
 * @param deviceIndex Device index (for multiple watchdogs)
 * @param mode Mode to set
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_SetMode(
    WdgIf_DeviceIndexType deviceIndex,
    WdgIf_ModeType mode
);

/**
 * @brief Trigger watchdog (kick)
 * @param deviceIndex Device index
 */
void WdgIf_Trigger(WdgIf_DeviceIndexType deviceIndex);

/**
 * @brief Get current watchdog mode
 * @param deviceIndex Device index
 * @param mode Pointer to store current mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_GetMode(
    WdgIf_DeviceIndexType deviceIndex,
    WdgIf_ModeType *mode
);

/**
 * @brief Get version information
 * @param versionInfo Pointer to version info structure
 */
void WdgIf_GetVersionInfo(Std_VersionInfoType *versionInfo);

/**
 * @brief Initialize WdgIf
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_Init(void);

/**
 * @brief Deinitialize WdgIf
 * @return E_OK if successful
 */
Std_ReturnType WdgIf_DeInit(void);

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_H */
