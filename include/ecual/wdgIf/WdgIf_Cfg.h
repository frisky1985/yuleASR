/******************************************************************************
 * @file    WdgIf_Cfg.h
 * @brief   Watchdog Interface Configuration
 *
 * Configuration parameters for the Watchdog Interface module.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef WDGIF_CFG_H
#define WDGIF_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Module Configuration
 ******************************************************************************/

/* Development error detection */
#define WDGIF_DEV_ERROR_DETECT          STD_ON

/* Version info API */
#define WDGIF_VERSION_INFO_API          STD_ON

/* Multicore support */
#define WDGIF_MULTICORE_SUPPORT         STD_OFF

/* External watchdog support */
#define WDGIF_EXTERNAL_WDG_SUPPORTED    STD_ON

/* GPT integration support */
#define WDGIF_GPT_INTEGRATION           STD_ON

/******************************************************************************
 * Device Configuration
 ******************************************************************************/

/* Number of configured watchdog devices */
#define WDGIF_NUM_DEVICES               3U

/* Internal watchdog enabled */
#define WDGIF_INTERNAL_WDG_ENABLED      STD_ON

/* External watchdog 1 enabled */
#define WDGIF_EXTERNAL_WDG1_ENABLED     STD_ON

/* External watchdog 2 enabled */
#define WDGIF_EXTERNAL_WDG2_ENABLED     STD_ON

/******************************************************************************
 * Timeout Configuration
 ******************************************************************************/

/* Default slow mode timeout (ms) */
#define WDGIF_SLOW_MODE_TIMEOUT         500U

/* Default fast mode timeout (ms) */
#define WDGIF_FAST_MODE_TIMEOUT         50U

/* Minimum allowed timeout (ms) */
#define WDGIF_MIN_TIMEOUT               1U

/* Maximum allowed timeout (ms) */
#define WDGIF_MAX_TIMEOUT               10000U

/* Window mode start percentage */
#define WDGIF_WINDOW_START_PERCENT      25U

/* Window mode end percentage */
#define WDGIF_WINDOW_END_PERCENT        75U

/******************************************************************************
 * Safety Configuration
 ******************************************************************************/

/* Enable safety checks */
#define WDGIF_SAFETY_CHECKS_ENABLED     STD_ON

/* Maximum consecutive errors before action */
#define WDGIF_MAX_CONSECUTIVE_ERRORS    3U

/* Enable time window checking */
#define WDGIF_TIME_WINDOW_CHECK         STD_OFF

/* Reset on safety violation */
#define WDGIF_RESET_ON_VIOLATION        STD_OFF

/******************************************************************************
 * GPT Integration Configuration
 ******************************************************************************/

/* Number of GPT channels */
#define WDGIF_GPT_NUM_CHANNELS          3U

/* GPT channel 0 - Internal WDG */
#define WDGIF_GPT_CH0_ENABLED           STD_ON
#define WDGIF_GPT_CH0_PERIOD_MS         100U
#define WDGIF_GPT_CH0_WDG_DEVICE        WDGIF_INTERNAL_WDG

/* GPT channel 1 - External WDG1 */
#define WDGIF_GPT_CH1_ENABLED           STD_ON
#define WDGIF_GPT_CH1_PERIOD_MS         100U
#define WDGIF_GPT_CH1_WDG_DEVICE        WDGIF_EXTERNAL_WDG1

/* GPT channel 2 - External WDG2 */
#define WDGIF_GPT_CH2_ENABLED           STD_ON
#define WDGIF_GPT_CH2_PERIOD_MS         100U
#define WDGIF_GPT_CH2_WDG_DEVICE        WDGIF_EXTERNAL_WDG2

/******************************************************************************
 * DEM Event IDs
 ******************************************************************************/

/* DEM Event IDs - to be configured with actual Dem IDs */
#define WDGIF_DEM_EVENT_TIMEOUT         0x0101U
#define WDGIF_DEM_EVENT_TRIGGER_FAIL    0x0102U
#define WDGIF_DEM_EVENT_MODE_FAIL       0x0103U

/******************************************************************************
 * Hardware Platform Selection
 ******************************************************************************/

/* Select target hardware platform */
/* #define WDGIF_TARGET_AURIX_TC3XX */
/* #define WDGIF_TARGET_AURIX_TC4XX */
/* #define WDGIF_TARGET_S32K3XX */
/* #define WDGIF_TARGET_S32G3 */
#define WDGIF_TARGET_EMULATOR           /* For testing */

/******************************************************************************
 * Callback Configuration
 ******************************************************************************/

/* Enable mode change notification callback */
#define WDGIF_MODE_CHANGE_NOTIFICATION  STD_ON

/* Enable trigger failure notification callback */
#define WDGIF_TRIGGER_FAILURE_NOTIFICATION  STD_ON

#ifdef __cplusplus
}
#endif

#endif /* WDGIF_CFG_H */
