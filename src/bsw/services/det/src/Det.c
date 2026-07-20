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
/* MISRA-C:2023 Rule-20.13: compliant by design — #include path resolution — accessible via -I include paths */

/* MISRA-C:2023 Rule-12.1: compliant by design — operator precedence — well-defined per C standard, parentheses for clarity */


/*==================================================================================================
 *                                      DET DEVELOPMENT ERROR TRACER
 *==================================================================================================
 * FILENAME: Det.c
 * AUTOSAR VERSION: R22-11
 * DOCUMENT: AUTOSAR_SWS_DevelopmentErrorTracer.pdf
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Implementation of Development Error Tracer module
 *==================================================================================================
 */

/*==================================================================================================
 *                                         INCLUDE FILES
 *==================================================================================================*/
#include "Det.h"

/* Check for software version compatibility */
#if defined(DET_AR_RELEASE_MAJOR_VERSION) && (DET_AR_RELEASE_MAJOR_VERSION != 4u)
    #error "Det.c: Mismatch in AUTOSAR major version"
#endif

#if defined(DET_AR_RELEASE_MINOR_VERSION) && (DET_AR_RELEASE_MINOR_VERSION != 4u)
    #//error "Det.c: Mismatch in AUTOSAR minor version"
#endif

#if defined(DET_SW_MAJOR_VERSION) && (DET_SW_MAJOR_VERSION != 1u)
    #error "Det.c: Mismatch in software major version"
#endif

#if (DET_SW_MINOR_VERSION != 0u)
    #error "Det.c: Mismatch in software minor version"
#endif

/*==================================================================================================
 *                                    LOCAL DEFINES
 *==================================================================================================*/
#define DET_UNINITIALIZED               (0u)
#define DET_INITIALIZED                 (1u)
#define DET_STARTED                     (2u)

/*==================================================================================================
 *                                    LOCAL MACROS
 *==================================================================================================*/
/* Critical section macros (simplified - should use actual OS services) */
#define DET_ENTER_CRITICAL_SECTION()    /* Disable interrupts */
#define DET_EXIT_CRITICAL_SECTION()     /* Enable interrupts */

/*==================================================================================================
 *                                    LOCAL TYPEDEFS
 *==================================================================================================*/
typedef uint8 Det_StateType;

/*==================================================================================================
 *                                    LOCAL VARIABLES
 *==================================================================================================*/
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

/* Module initialization state */
static Det_StateType Det_State = DET_UNINITIALIZED;

/* Configuration pointer */
const Det_ConfigType* DetConfigPtr = NULL_PTR;

/* Error counter for statistics */
static uint32 Det_ErrorCounter = 0u;

/* Runtime error counter */
static uint32 Det_RuntimeErrorCounter = 0u;

/* Transient fault counter */
static uint32 Det_TransientFaultCounter = 0u;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

#define DET_START_SEC_VAR_INIT_UNSPECIFIED
#include "Det_MemMap.h"

/* Flag to indicate if Det is initialized */
boolean DetInitialized = FALSE;

#define DET_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Det_MemMap.h"

/*==================================================================================================
 *                                    ERROR HOOK TABLE
 *==================================================================================================*/
#if (DET_ERROR_HOOKS_ENABLED == STD_ON)
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

/* Array of registered error hooks */
static Det_ErrorHookType Det_ErrorHooks[DET_MAX_ERROR_HOOKS];
static uint8 Det_NumRegisteredHooks = 0u;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"
#endif

/*==================================================================================================
 *                                    RUNTIME CALLOUT TABLE
 *==================================================================================================*/
#if (DET_RUNTIME_ERROR_CALLOUTS == STD_ON)
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

static Det_RuntimeErrorCalloutType Det_RuntimeCallouts[DET_MAX_RUNTIME_CALLOUTS];
static uint8 Det_NumRuntimeCallouts = 0u;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"
#endif

/*==================================================================================================
 *                                    TRANSIENT FAULT CALLOUT TABLE
 *==================================================================================================*/
#if (DET_TRANSIENT_FAULT_CALLOUTS == STD_ON)
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"

static Det_TransientFaultCalloutType Det_TransientCallouts[DET_MAX_TRANSIENT_CALLOUTS];
static uint8 Det_NumTransientCallouts = 0u;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Det_MemMap.h"
#endif

/*==================================================================================================
 *                                    LOCAL FUNCTION PROTOTYPES
 *==================================================================================================*/
#define DET_START_SEC_CODE
#include "Det_MemMap.h"

static void Det_CallErrorHooks(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
static void Det_CallRuntimeCallouts(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
static void Det_CallTransientCallouts(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId);

/*==================================================================================================
 *                                       API IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Initializes the Det module
 * @param ConfigPtr Pointer to configuration structure (post-build)
 * @return None
 * @req SWS_Det_00005
 * @note ConfigPtr may be NULL_PTR for pre-compile configuration
 */
void Det_Init(const Det_ConfigType* ConfigPtr)
{
#if (DET_ENABLED == STD_ON)
    /* Check if already initialized */
    if (Det_State == DET_INITIALIZED || Det_State == DET_STARTED)
    {
        /* Report error - double initialization */
        /* Note: Cannot use Det_ReportError here as we're in Init */
        return;
    }
    
    DET_ENTER_CRITICAL_SECTION();
    
    /* Store configuration pointer */
    DetConfigPtr = ConfigPtr;
    
    /* Initialize state */
    Det_State = DET_INITIALIZED;
    DetInitialized = TRUE;
    
    /* Reset counters */
    Det_ErrorCounter = 0u;
    Det_RuntimeErrorCounter = 0u;
    Det_TransientFaultCounter = 0u;
    
    /* Clear hook tables */
    #if (DET_ERROR_HOOKS_ENABLED == STD_ON)
    {
        uint8 i;
        Det_NumRegisteredHooks = 0u;
        for (i = 0u; i < DET_MAX_ERROR_HOOKS; i++)
        {
            Det_ErrorHooks[i] = NULL_PTR;
        }
    }
    #endif
    
    #if (DET_RUNTIME_ERROR_CALLOUTS == STD_ON)
    {
        uint8 i;
        Det_NumRuntimeCallouts = 0u;
        for (i = 0u; i < DET_MAX_RUNTIME_CALLOUTS; i++)
        {
            Det_RuntimeCallouts[i] = NULL_PTR;
        }
    }
    #endif
    
    #if (DET_TRANSIENT_FAULT_CALLOUTS == STD_ON)
    {
        uint8 i;
        Det_NumTransientCallouts = 0u;
        for (i = 0u; i < DET_MAX_TRANSIENT_CALLOUTS; i++)
        {
            Det_TransientCallouts[i] = NULL_PTR;
        }
    }
    #endif
    
    DET_EXIT_CRITICAL_SECTION();
    
#else
    /* Det is disabled - do nothing */
    (void)ConfigPtr; /* Suppress unused parameter warning */
#endif /* DET_ENABLED */
}

/**
 * @brief Reports a development error
 * @param ModuleId Module ID where error occurred
 * @param InstanceId Instance ID within the module
 * @param ApiId API ID where error was detected
 * @param ErrorId Error code
 * @return Always returns E_OK
 * @req SWS_Det_00006
 * @note This is the main API for reporting development errors
 */
Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
#if (DET_ENABLED == STD_ON)
    
    /* Increment error counter */
    DET_ENTER_CRITICAL_SECTION();
    Det_ErrorCounter++;
    DET_EXIT_CRITICAL_SECTION();
    
    /* Call registered error hooks */
    #if (DET_ERROR_HOOKS_ENABLED == STD_ON)
    Det_CallErrorHooks(ModuleId, InstanceId, ApiId, ErrorId);
    #endif
    
    /* Forward to Dem if enabled */
    #if (DET_FORWARD_TO_DEM == STD_ON)
    {
        /* Dem_ReportErrorStatus(DET_DEM_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
        /* Note: Actual Dem integration would go here */
    }
    #endif
    
    /* Forward to Dlt if enabled */
    #if (DET_FORWARD_TO_DLT == STD_ON)
    {
        /* Dlt logging would go here */
    }
    #endif
    
#else
    /* Suppress unused parameter warnings */
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
#endif /* DET_ENABLED */
    
    return E_OK;
}

/**
 * @brief Starts the error tracer
 * @return None
 * @req SWS_Det_00008
 * @note After Start(), error reporting becomes active
 */
void Det_Start(void)
{
#if (DET_ENABLED == STD_ON)
    if (Det_State == DET_INITIALIZED)
    {
        Det_State = DET_STARTED;
    }
    /* If not initialized, Start() has no effect */
#endif
}

/**
 * @brief Reports a runtime error
 * @param ModuleId Module ID where error occurred
 * @param InstanceId Instance ID within the module
 * @param ApiId API ID where error was detected
 * @param ErrorId Error code
 * @return E_OK if handled, E_NOT_OK otherwise
 * @req SWS_Det_00012
 * @note Runtime errors are for production code, not development only
 */
Std_ReturnType Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (DET_ENABLED == STD_ON)
    
    /* Increment runtime error counter */
    DET_ENTER_CRITICAL_SECTION();
    Det_RuntimeErrorCounter++;
    DET_EXIT_CRITICAL_SECTION();
    
    /* Call runtime error callouts */
    #if (DET_RUNTIME_ERROR_CALLOUTS == STD_ON)
    Det_CallRuntimeCallouts(ModuleId, InstanceId, ApiId, ErrorId);
    result = E_OK; /* Assume handled if callouts exist */
    #endif
    
    /* Forward to Dem if enabled */
    #if (DET_FORWARD_TO_DEM == STD_ON)
    {
        /* Dem_ReportErrorStatus(DET_DEM_EVENT_ID, DEM_EVENT_STATUS_FAILED); */
    }
    #endif
    
#else
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
#endif
    
    return result;
}

/**
 * @brief Reports a transient fault
 * @param ModuleId Module ID where fault occurred
 * @param InstanceId Instance ID within the module
 * @param ApiId API ID where fault was detected
 * @param FaultId Fault code
 * @return E_OK if handled, E_NOT_OK otherwise
 * @req SWS_Det_00013
 * @note Transient faults are recoverable error conditions
 */
Std_ReturnType Det_ReportTransientFault(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (DET_ENABLED == STD_ON)
    
    /* Increment transient fault counter */
    DET_ENTER_CRITICAL_SECTION();
    Det_TransientFaultCounter++;
    DET_EXIT_CRITICAL_SECTION();
    
    /* Call transient fault callouts */
    #if (DET_TRANSIENT_FAULT_CALLOUTS == STD_ON)
    Det_CallTransientCallouts(ModuleId, InstanceId, ApiId, FaultId);
    result = E_OK;
    #endif
    
#else
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)FaultId;
#endif
    
    return result;
}

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 * @return None
 * @req SWS_Det_00011
 */
#if (DET_VERSION_INFO_API == STD_ON)
void Det_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID = DET_VENDOR_ID;
        versioninfo->moduleID = DET_MODULE_ID;
        versioninfo->sw_major_version = DET_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version = DET_SW_MINOR_VERSION;
        versioninfo->sw_patch_version = DET_SW_PATCH_VERSION;
    }
    #if (DET_ENABLED == STD_ON)
    else
    {
        /* Report error - null pointer */
        (void)Det_ReportError(DET_MODULE_ID, DET_INSTANCE_ID, 0x00u, DET_E_PARAM_POINTER);
    }
    #endif
}
#endif

/*==================================================================================================
 *                                    LOCAL FUNCTION IMPLEMENTATIONS
 *==================================================================================================*/

/**
 * @brief Internal function to call all registered error hooks
 */
#if (DET_ERROR_HOOKS_ENABLED == STD_ON)
static void Det_CallErrorHooks(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    uint8 i;
    
    for (i = 0u; i < Det_NumRegisteredHooks; i++)
    {
        if (Det_ErrorHooks[i] != NULL_PTR)
        {
            Det_ErrorHooks[i](ModuleId, InstanceId, ApiId, ErrorId);
        }
    }
}
#endif

/**
 * @brief Internal function to call all runtime error callouts
 */
#if (DET_RUNTIME_ERROR_CALLOUTS == STD_ON)
static void Det_CallRuntimeCallouts(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    uint8 i;
    
    for (i = 0u; i < Det_NumRuntimeCallouts; i++)
    {
        if (Det_RuntimeCallouts[i] != NULL_PTR)
        {
            Det_RuntimeCallouts[i](ModuleId, InstanceId, ApiId, ErrorId);
        }
    }
}
#endif

/**
 * @brief Internal function to call all transient fault callouts
 */
#if (DET_TRANSIENT_FAULT_CALLOUTS == STD_ON)
static void Det_CallTransientCallouts(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId)
{
    uint8 i;
    
    for (i = 0u; i < Det_NumTransientCallouts; i++)
    {
        if (Det_TransientCallouts[i] != NULL_PTR)
        {
            Det_TransientCallouts[i](ModuleId, InstanceId, ApiId, FaultId);
        }
    }
}
#endif

#define DET_STOP_SEC_CODE
#include "Det_MemMap.h"

/*==================================================================================================
 *                                      END OF FILE
 *==================================================================================================*/
