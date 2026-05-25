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

/**
 * @file Dem_Error.h
 * @brief Diagnostic Event Manager - Error Handling Definitions
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Error handling and reporting definitions
 */

#ifndef DEM_ERROR_H
#define DEM_ERROR_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DEM_ERROR_VENDOR_ID                   (0x01U)
#define DEM_ERROR_MODULE_ID                   (0x54U)
#define DEM_ERROR_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DEM_ERROR_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DEM_ERROR_AR_RELEASE_REVISION_VERSION (0x00U)
#define DEM_ERROR_SW_MAJOR_VERSION            (0x01U)
#define DEM_ERROR_SW_MINOR_VERSION            (0x01U)
#define DEM_ERROR_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    ERROR SEVERITY LEVELS
==================================================================================================*/
typedef enum {
    DEM_ERROR_LEVEL_INFO = 0,        /* Informational, no action required */
    DEM_ERROR_LEVEL_WARNING,         /* Warning, may require attention */
    DEM_ERROR_LEVEL_ERROR,           /* Error, action required */
    DEM_ERROR_LEVEL_CRITICAL         /* Critical error, system may be affected */
} Dem_ErrorLevelType;

/*==================================================================================================
*                                    ERROR TYPES
==================================================================================================*/
typedef enum {
    DEM_ERROR_NONE = 0,
    DEM_ERROR_INIT_FAILED,
    DEM_ERROR_INVALID_CONFIG,
    DEM_ERROR_INVALID_EVENT_ID,
    DEM_ERROR_INVALID_DTC,
    DEM_ERROR_MEMORY_FULL,
    DEM_ERROR_NV_WRITE_FAILED,
    DEM_ERROR_NV_READ_FAILED,
    DEM_ERROR_INTERNAL_ERROR,
    DEM_ERROR_NULL_POINTER
} Dem_ErrorType;

/*==================================================================================================
*                                    ERROR HOOK TYPE
==================================================================================================*/
typedef void (*Dem_ErrorHookType)(Dem_ErrorType Error, Dem_ErrorLevelType Level, uint16 ModuleId, uint8 ApiId);

/*==================================================================================================
*                                    GLOBAL ERROR HOOK
==================================================================================================*/
#define DEM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

extern Dem_ErrorHookType Dem_ErrorHook;

#define DEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DEM_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Report an error
 * @param Error Error type
 * @param Level Error level
 * @param ModuleId Module ID
 * @param ApiId API ID
 */
extern void Dem_ReportError(Dem_ErrorType Error, Dem_ErrorLevelType Level, uint16 ModuleId, uint8 ApiId);

/**
 * @brief Register error hook
 * @param Hook Error hook function
 */
extern void Dem_RegisterErrorHook(Dem_ErrorHookType Hook);

/**
 * @brief Unregister error hook
 */
extern void Dem_UnregisterErrorHook(void);

#define DEM_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DEM_ERROR_H */
