/*==================================================================================================
* Project          : AUTOSAR Reference Implementation
* File Name        : Det_Cfg.h
* Description      : Development Error Tracer configuration header file
*                    Contains configurable parameters and feature switches.
*==================================================================================================
* (C) Copyright 2024, yuleASR
*==================================================================================================*/

#ifndef DET_CFG_H
#define DET_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                         INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    CONFIGURATION VERSIONS
==================================================================================================*/
#define DET_CFG_MAJOR_VERSION           (0x01U)
#define DET_CFG_MINOR_VERSION           (0x00U)
#define DET_CFG_PATCH_VERSION           (0x00U)

/*==================================================================================================
*                               PRE-COMPILE TIME CONFIGURATION
==================================================================================================*/

/**
 * @brief Enable/disable Development Error Detection within Det module itself
 * STD_ON  : Development error detection is enabled
 * STD_OFF : Development error detection is disabled
 */
#define DET_DEV_ERROR_DETECT            STD_ON

/**
 * @brief Enable/disable Version Info API
 * STD_ON  : Det_GetVersionInfo API is available
 * STD_OFF : Det_GetVersionInfo API is not available
 */
#define DET_VERSION_INFO_API            STD_ON

/**
 * @brief Enable/disable the Det module
 * STD_ON  : Det is enabled and functional
 * STD_OFF : Det is disabled (all APIs return immediately)
 */
#define DET_ENABLED                     STD_ON

/**
 * @brief Enable/disable freeze on error
 * When enabled, system halts on detected development errors for debugging
 * STD_ON  : Freeze system on error
 * STD_OFF : Continue execution on error
 */
#define DET_FREEZE_ON_ERROR             STD_ON

/**
 * @brief Enable/disable error logging
 * When enabled, errors are stored in a circular buffer
 */
#define DET_ERROR_LOGGING_ENABLED       STD_ON

/**
 * @brief Maximum number of error entries in the error log
 */
#define DET_MAX_ERROR_ENTRIES           (10U)

/**
 * @brief Enable/disable runtime error reporting
 * STD_ON  : Det_ReportRuntimeError API is available
 * STD_OFF : Runtime error reporting is disabled
 */
#define DET_RUNTIME_ERROR_REPORTING     STD_ON

/**
 * @brief Enable/disable transient fault reporting
 * STD_ON  : Det_ReportTransientFault API is available
 * STD_OFF : Transient fault reporting is disabled
 */
#define DET_TRANSIENT_FAULT_REPORTING   STD_OFF

/**
 * @brief Enable/disable forward error hooks
 * When enabled, registered error hooks are called on error detection
 */
#define DET_ERROR_HOOK_FORWARD          STD_ON

/**
 * @brief Enable/disable runtime error hook forward
 */
#define DET_RUNTIME_ERROR_HOOK_FORWARD  STD_ON

/**
 * @brief Enable/disable transient fault hook forward
 */
#define DET_TRANSIENT_FAULT_HOOK_FORWARD STD_ON

/**
 * @brief Enable/disable Det to call OS service for error handling
 */
#define DET_FORWARD_TO_OS               STD_OFF

/**
 * @brief Enable/disable Dem error reporting from Det
 */
#define DET_FORWARD_TO_DEM              STD_OFF

/**
 * @brief Dem event ID for Det errors (if DET_FORWARD_TO_DEM is STD_ON)
 */
#define DET_DEM_EVENT_ID                (0x0000U)

/*==================================================================================================
*                                  POST-BUILD CONFIGURATION
==================================================================================================*/

/**
 * @brief Number of configured error hooks
 */
#define DET_NUMBER_OF_ERROR_HOOKS       (1U)

/**
 * @brief Number of configured runtime error hooks
 */
#define DET_NUMBER_OF_RUNTIME_ERROR_HOOKS (1U)

/**
 * @brief Number of configured transient fault hooks
 */
#define DET_NUMBER_OF_TRANSIENT_FAULT_HOOKS (1U)

/*==================================================================================================
*                                    MODULE ID FILTERS
==================================================================================================*/

/**
 * @brief Enable filtering of errors by module ID
 * STD_ON  : Only report errors from modules in the filter list
 * STD_OFF : Report errors from all modules
 */
#define DET_MODULE_FILTER_ENABLED       STD_OFF

/**
 * @brief List of module IDs to report errors from (if filtering enabled)
 */
#define DET_FILTERED_MODULE_COUNT       (0U)

/*==================================================================================================
*                                      CALLBACK MACROS
==================================================================================================*/

/**
 * @brief Default error hook callback (if enabled)
 */
#if defined(DET_ERROR_HOOK_FORWARD) && (DET_ERROR_HOOK_FORWARD == STD_ON)
#define DET_ERROR_HOOK()
#endif

/**
 * @brief Default runtime error hook callback (if enabled)
 */
#if defined(DET_RUNTIME_ERROR_HOOK_FORWARD) && (DET_RUNTIME_ERROR_HOOK_FORWARD == STD_ON)
#define DET_RUNTIME_ERROR_HOOK()
#endif

/**
 * @brief Default transient fault hook callback (if enabled)
 */
#if defined(DET_TRANSIENT_FAULT_HOOK_FORWARD) && (DET_TRANSIENT_FAULT_HOOK_FORWARD == STD_ON)
#define DET_TRANSIENT_FAULT_HOOK()
#endif

#ifdef __cplusplus
}
#endif

#endif /* DET_CFG_H */
