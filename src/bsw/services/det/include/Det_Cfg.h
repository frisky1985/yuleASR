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

/*==================================================================================================
 *                                      DET CONFIGURATION
 *==================================================================================================
 * FILENAME: Det_Cfg.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Pre-compile configuration header for Det module
 *==================================================================================================
 */

#ifndef DET_CFG_H
#define DET_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    PRE-COMPILE OPTIONS
 *==================================================================================================*/

/**
 * @brief Enable/disable version info API
 */
#define DET_VERSION_INFO_API            (STD_ON)

/**
 * @brief Enable/disable forward to Dlt (Diagnostic Log and Trace)
 */
#define DET_FORWARD_TO_DLT              (STD_OFF)

/**
 * @brief Enable/disable forward to Dem (Diagnostic Event Manager)
 */
#define DET_FORWARD_TO_DEM              (STD_ON)

/**
 * @brief Dem event ID for Det errors (used when DET_FORWARD_TO_DEM is STD_ON)
 */
#define DET_DEM_EVENT_ID                (DemConf_DemEventParameter_DetError)

/**
 * @brief Enable/disable error hooks
 */
#define DET_ERROR_HOOKS_ENABLED         (STD_ON)

/**
 * @brief Enable/disable runtime error callouts
 */
#define DET_RUNTIME_ERROR_CALLOUTS      (STD_ON)

/**
 * @brief Enable/disable transient fault callouts
 */
#define DET_TRANSIENT_FAULT_CALLOUTS    (STD_ON)

/**
 * @brief Maximum number of error hooks that can be registered
 */
#define DET_MAX_ERROR_HOOKS             (4u)

/**
 * @brief Maximum number of runtime error callouts
 */
#define DET_MAX_RUNTIME_CALLOUTS        (2u)

/**
 * @brief Maximum number of transient fault callouts
 */
#define DET_MAX_TRANSIENT_CALLOUTS      (2u)

/**
 * @brief Enable/disable Det module (can be disabled in production)
 */
#define DET_ENABLED                     (STD_ON)

/**
 * @brief Enable/disable freeze on error
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
 */
#define DET_RUNTIME_ERROR_REPORTING     STD_ON

/**
 * @brief Enable/disable transient fault reporting
 */
#define DET_TRANSIENT_FAULT_REPORTING   STD_OFF

/**
 * @brief Enable/disable error hook forwarding
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

/*==================================================================================================
 *                                 POST-BUILD CONFIGURATION
 *==================================================================================================*/

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
 *                                    CALLBACK DECLARATIONS
 *==================================================================================================*/

/* External declaration of error hook functions - to be defined in application */
#if (DET_ERROR_HOOKS_ENABLED == STD_ON)
    /* Example: extern void MyErrorHook(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId); */
#endif

#ifdef __cplusplus
}
#endif

#endif /* DET_CFG_H */
