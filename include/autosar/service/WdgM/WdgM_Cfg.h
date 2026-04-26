/** @file WdgM_Cfg.h
 * @brief Watchdog Manager Configuration
 * @details AUTOSAR R22-11 compliant configuration header
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#ifndef WDGM_CFG_H
#define WDGM_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/******************************************************************************
 * Module Version Configuration
 ******************************************************************************/
#define WDGM_CFG_VENDOR_ID              0x01U
#define WDGM_CFG_MODULE_ID              0x12U
#define WDGM_CFG_SW_MAJOR_VERSION       1U
#define WDGM_CFG_SW_MINOR_VERSION       0U
#define WDGM_CFG_SW_PATCH_VERSION       0U

/******************************************************************************
 * Development Error Detection
 ******************************************************************************/
#ifndef WDGM_DEV_ERROR_DETECT
#define WDGM_DEV_ERROR_DETECT           STD_ON
#endif

/******************************************************************************
 * DEM Reporting Configuration
 ******************************************************************************/
#ifndef WDGM_DEM_REPORTING_ENABLED
#define WDGM_DEM_REPORTING_ENABLED      STD_ON
#endif

/******************************************************************************
 * Safety Configuration
 ******************************************************************************/
#define WDGM_SAFETY_CHECKS_ENABLED      STD_ON
#define WDGM_IMMEDIATE_RESET_ENABLED    STD_OFF

/******************************************************************************
 * Maximum Configuration Limits
 ******************************************************************************/
#define WDGM_MAX_SUPERVISED_ENTITIES    32U
#define WDGM_MAX_CHECKPOINTS            256U
#define WDGM_MAX_TRANSITIONS            128U
#define WDGM_MAX_MODES                  8U

/******************************************************************************
 * DET Error Reporting Macro
 ******************************************************************************/
#if (WDGM_DEV_ERROR_DETECT == STD_ON)
    #define WDGM_DET_REPORT_ERROR(apiId, errorId) \
        (void)Det_ReportError(WDGM_MODULE_ID, 0U, (apiId), (errorId))
#else
    #define WDGM_DET_REPORT_ERROR(apiId, errorId) ((void)0)
#endif

/******************************************************************************
 * DEM Event Reporting Macros
 ******************************************************************************/
#if (WDGM_DEM_REPORTING_ENABLED == STD_ON)
    #define WDGM_REPORT_DEM_EVENT(eventId, status) \
        (void)Dem_ReportErrorStatus((eventId), (status))
#else
    #define WDGM_REPORT_DEM_EVENT(eventId, status) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* WDGM_CFG_H */
