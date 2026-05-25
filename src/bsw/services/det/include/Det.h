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
 *                                      DET DEVELOPMENT ERROR TRACER
 *==================================================================================================
 * FILENAME: Det.h
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_DevelopmentErrorTracer.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Public header file for Development Error Tracer module
 *==================================================================================================
 */

#ifndef DET_H
#define DET_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Std_Types.h"      /* AUTOSAR standard types */
#include "Det_Cfg.h"        /* Det configuration */

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define DET_VENDOR_ID                   (100u)
#define DET_MODULE_ID                   (15u)
#define DET_INSTANCE_ID                 (0u)

#define DET_AR_RELEASE_MAJOR_VERSION    (4u)
#define DET_AR_RELEASE_MINOR_VERSION    (7u)
#define DET_AR_RELEASE_REVISION_VERSION (0u)

#define DET_SW_MAJOR_VERSION            (1u)
#define DET_SW_MINOR_VERSION            (0u)
#define DET_SW_PATCH_VERSION            (0u)

/*==================================================================================================
 *                                    FILE VERSION CHECKS
 *==================================================================================================*/
#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if ((DET_AR_RELEASE_MAJOR_VERSION != STD_TYPES_AR_RELEASE_MAJOR_VERSION) || \
         (DET_AR_RELEASE_MINOR_VERSION != STD_TYPES_AR_RELEASE_MINOR_VERSION))
        #error "AutoSAR Version Numbers of Det.h and Std_Types.h are different"
    #endif
#endif

/*==================================================================================================
 *                                    ERROR CODES
 *==================================================================================================*/
/* Development error codes for Det module */
#define DET_E_PARAM_POINTER             (0x01u)  /* API called with NULL pointer */
#define DET_E_UNINIT                    (0x02u)  /* API called before initialization */
#define DET_E_ALREADY_INITIALIZED       (0x03u)  /* Multiple initialization call */

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/
/**
 * @brief Configuration structure for Det module
 */
typedef struct {
    uint16 dummy;  /* Placeholder for configuration parameters */
} Det_ConfigType;

/**
 * @brief Error hook function type
 * @param ModuleId Module ID where error occurred
 * @param InstanceId Instance ID within the module
 * @param ApiId API ID where error occurred
 * @param ErrorId Error code
 */
typedef void (*Det_ErrorHookType)(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

/**
 * @brief Runtime error callback function type
 */
typedef void (*Det_RuntimeErrorCalloutType)(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

/**
 * @brief Transient fault callback function type
 */
typedef void (*Det_TransientFaultCalloutType)(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId
);

/*==================================================================================================
 *                                    GLOBAL VARIABLES (extern)
 *==================================================================================================*/
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

/* Extern declarations for global variables */
extern boolean DetInitialized;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

#define DET_START_SEC_CONST_UNSPECIFIED
#include "Det_MemMap.h"

/* Configuration pointer (post-build configuration) */
extern const Det_ConfigType* DetConfigPtr;

#define DET_STOP_SEC_CONST_UNSPECIFIED
#include "Det_MemMap.h"

/*==================================================================================================
 *                                     API DECLARATIONS
 *==================================================================================================*/
#define DET_START_SEC_CODE
#include "Det_MemMap.h"

/**
 * @brief Initializes the Det module
 * @param ConfigPtr Pointer to configuration structure
 * @return None
 * @req SWS_Det_00005
 */
extern void Det_Init(const Det_ConfigType* ConfigPtr);

/**
 * @brief Reports a development error
 * @param ModuleId Module ID of calling module
 * @param InstanceId Instance ID or 0 if single instance
 * @param ApiId API service ID where error was detected
 * @param ErrorId Error code
 * @return Always returns E_OK (for compatibility)
 * @req SWS_Det_00006
 */
extern Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

/**
 * @brief Starts the error tracer (enables error reporting)
 * @return None
 * @req SWS_Det_00008
 */
extern void Det_Start(void);

/**
 * @brief Reports a runtime error
 * @param ModuleId Module ID of calling module
 * @param InstanceId Instance ID or 0 if single instance
 * @param ApiId API service ID where error was detected
 * @param ErrorId Error code
 * @return E_OK if error was handled, E_NOT_OK otherwise
 * @req SWS_Det_00012
 */
extern Std_ReturnType Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);

/**
 * @brief Reports a transient fault
 * @param ModuleId Module ID of calling module
 * @param InstanceId Instance ID or 0 if single instance
 * @param ApiId API service ID where fault was detected
 * @param FaultId Fault code
 * @return E_OK if fault was handled, E_NOT_OK otherwise
 * @req SWS_Det_00013
 */
extern Std_ReturnType Det_ReportTransientFault(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId
);

/**
 * @brief Gets version information of Det module
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Det_00011
 */
#if (DET_VERSION_INFO_API == STD_ON)
extern void Det_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

#define DET_STOP_SEC_CODE
#include "Det_MemMap.h"

#ifdef __cplusplus
}
#endif

#endif /* DET_H */
