/*==================================================================================================
* Project          : AUTOSAR Reference Implementation
* File Name        : Det.c
* Description      : Development Error Tracer implementation
*                    Provides core functionality for error detection, logging,
*                    freeze-on-error, and callback hooks.
*==================================================================================================
* (C) Copyright 2024, yuleASR
*==================================================================================================*/

/*==================================================================================================
*                                         INCLUDE FILES
==================================================================================================*/
#include "Det.h"
#include "SchM_Det.h"

#if (DET_FORWARD_TO_DEM == STD_ON)
#include "Dem.h"
#endif

/*==================================================================================================
*                                      LOCAL DEFINES
==================================================================================================*/
#define DET_UNINITIALIZED               (0x00U)
#define DET_INITIALIZED                 (0x01U)

/*==================================================================================================
*                                      LOCAL TYPES
==================================================================================================*/

typedef struct
{
    uint8 initStatus;
    uint8 errorIndex;
    boolean freezeOnError;
    boolean loggingEnabled;
    uint8 maxErrorEntries;
} Det_InternalStatusType;

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC VAR(Det_InternalStatusType, DET_VAR) Det_InternalStatus;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define DET_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

VAR(Det_ErrorEntryType, DET_VAR) Det_ErrorLog[DET_MAX_ERROR_ENTRIES];
VAR(uint8, DET_VAR) Det_ErrorCount;

#define DET_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      EXTERNAL VARIABLES
==================================================================================================*/
#define DET_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern CONST(Det_ErrorHookPtrType, DET_CONST) Det_ErrorHooks[DET_NUMBER_OF_ERROR_HOOKS];
extern CONST(Det_RuntimeErrorHookPtrType, DET_CONST) Det_RuntimeErrorHooks[DET_NUMBER_OF_RUNTIME_ERROR_HOOKS];
extern CONST(Det_TransientFaultHookPtrType, DET_CONST) Det_TransientFaultHooks[DET_NUMBER_OF_TRANSIENT_FAULT_HOOKS];

#define DET_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/
#define DET_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief   Freeze the system for debugging purposes
 * @details Infinite loop that can be broken by debugger
 */
LOCAL FUNC(void, DET_CODE) Det_FreezeOnError(void)
{
    volatile boolean freeze = TRUE;
    
    while (freeze == TRUE)
    {
        /* Busy wait - can be broken by debugger */
        ;
    }
}

/**
 * @brief   Log an error entry to the error buffer
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where error occurred
 * @param   ErrorId     ID of the detected error
 */
LOCAL FUNC(void, DET_CODE) Det_LogError(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId
)
{
    uint8 index;
    
    if (Det_InternalStatus.loggingEnabled == TRUE)
    {
        index = Det_InternalStatus.errorIndex;
        
        Det_ErrorLog[index].ModuleId = ModuleId;
        Det_ErrorLog[index].InstanceId = InstanceId;
        Det_ErrorLog[index].ApiId = ApiId;
        Det_ErrorLog[index].ErrorId = ErrorId;
        
        Det_InternalStatus.errorIndex++;
        if (Det_InternalStatus.errorIndex >= Det_InternalStatus.maxErrorEntries)
        {
            Det_InternalStatus.errorIndex = 0U;
        }
        
        if (Det_ErrorCount < 0xFFU)
        {
            Det_ErrorCount++;
        }
    }
}

/**
 * @brief   Call registered error hooks
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where error occurred
 * @param   ErrorId     ID of the detected error
 * @return  E_OK if at least one hook returned E_OK, E_NOT_OK otherwise
 */
LOCAL FUNC(Std_ReturnType, DET_CODE) Det_CallErrorHooks(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId
)
{
    Std_ReturnType hookResult = E_NOT_OK;
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
#if (DET_ERROR_HOOK_FORWARD == STD_ON)
    for (i = 0U; i < DET_NUMBER_OF_ERROR_HOOKS; i++)
    {
        if (Det_ErrorHooks[i] != NULL_PTR)
        {
            hookResult = Det_ErrorHooks[i](ModuleId, InstanceId, ApiId, ErrorId);
            if (hookResult == E_OK)
            {
                result = E_OK;
            }
        }
    }
#endif

    return result;
}

/**
 * @brief   Call registered runtime error hooks
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where error occurred
 * @param   ErrorId     ID of the detected error
 * @return  E_OK if at least one hook returned E_OK, E_NOT_OK otherwise
 */
LOCAL FUNC(Std_ReturnType, DET_CODE) Det_CallRuntimeErrorHooks(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId
)
{
    Std_ReturnType hookResult = E_NOT_OK;
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
#if (DET_RUNTIME_ERROR_HOOK_FORWARD == STD_ON)
    for (i = 0U; i < DET_NUMBER_OF_RUNTIME_ERROR_HOOKS; i++)
    {
        if (Det_RuntimeErrorHooks[i] != NULL_PTR)
        {
            hookResult = Det_RuntimeErrorHooks[i](ModuleId, InstanceId, ApiId, ErrorId);
            if (hookResult == E_OK)
            {
                result = E_OK;
            }
        }
    }
#endif

    return result;
}

/**
 * @brief   Call registered transient fault hooks
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where error occurred
 * @param   FaultId     ID of the detected fault
 * @return  E_OK if at least one hook returned E_OK, E_NOT_OK otherwise
 */
LOCAL FUNC(Std_ReturnType, DET_CODE) Det_CallTransientFaultHooks(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) FaultId
)
{
    Std_ReturnType hookResult = E_NOT_OK;
    Std_ReturnType result = E_NOT_OK;
    uint8 i;
    
#if (DET_TRANSIENT_FAULT_HOOK_FORWARD == STD_ON)
    for (i = 0U; i < DET_NUMBER_OF_TRANSIENT_FAULT_HOOKS; i++)
    {
        if (Det_TransientFaultHooks[i] != NULL_PTR)
        {
            hookResult = Det_TransientFaultHooks[i](ModuleId, InstanceId, ApiId, FaultId);
            if (hookResult == E_OK)
            {
                result = E_OK;
            }
        }
    }
#endif

    return result;
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   Initializes the Development Error Tracer
 * @param   ConfigPtr   Pointer to the configuration structure
 * @return  None
 */
FUNC(void, DET_CODE) Det_Init(
    P2CONST(Det_ConfigType, AUTOMATIC, DET_CONST) ConfigPtr)
{
    uint8 i;
    
#if (DET_DEV_ERROR_DETECT == STD_ON)
    if (Det_InternalStatus.initStatus == DET_INITIALIZED)
    {
        /* Report to self if already initialized */
        return;
    }
    
    if (ConfigPtr == NULL_PTR)
    {
        /* Report to self if NULL pointer */
        return;
    }
#endif
    
    /* Initialize internal status */
    Det_InternalStatus.errorIndex = 0U;
    Det_ErrorCount = 0U;
    Det_InternalStatus.freezeOnError = ConfigPtr->DetEnableFreezeOnError;
    Det_InternalStatus.loggingEnabled = ConfigPtr->DetEnableLogging;
    Det_InternalStatus.maxErrorEntries = ConfigPtr->DetMaxErrorEntries;
    
    /* Clear error log */
    for (i = 0U; i < DET_MAX_ERROR_ENTRIES; i++)
    {
        Det_ErrorLog[i].ModuleId = 0U;
        Det_ErrorLog[i].InstanceId = 0U;
        Det_ErrorLog[i].ApiId = 0U;
        Det_ErrorLog[i].ErrorId = 0U;
    }
    
    Det_InternalStatus.initStatus = DET_INITIALIZED;
}

/**
 * @brief   Deinitializes the Development Error Tracer
 * @return  None
 */
FUNC(void, DET_CODE) Det_DeInit(void)
{
    Det_InternalStatus.initStatus = DET_UNINITIALIZED;
    Det_InternalStatus.errorIndex = 0U;
    Det_ErrorCount = 0U;
}

/**
 * @brief   Reports a development error
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where error occurred
 * @param   ErrorId     ID of the detected error
 * @return  E_OK if error was handled, E_NOT_OK otherwise
 */
FUNC(Std_ReturnType, DET_CODE) Det_ReportError(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (DET_ENABLED == STD_ON)
    
#if (DET_DEV_ERROR_DETECT == STD_ON)
    if (Det_InternalStatus.initStatus != DET_INITIALIZED)
    {
        return E_NOT_OK;
    }
#endif
    
    /* Enter critical section */
    SchM_Enter_Det_DET_EXCLUSIVE_AREA_0();
    
    /* Log the error */
    Det_LogError(ModuleId, InstanceId, ApiId, ErrorId);
    
    /* Call error hooks */
    result = Det_CallErrorHooks(ModuleId, InstanceId, ApiId, ErrorId);
    
    /* Report to Dem if configured */
#if (DET_FORWARD_TO_DEM == STD_ON)
    Dem_ReportErrorStatus(DET_DEM_EVENT_ID, DEM_EVENT_STATUS_FAILED);
#endif
    
    /* Exit critical section */
    SchM_Exit_Det_DET_EXCLUSIVE_AREA_0();
    
    /* Freeze on error if enabled */
#if (DET_FREEZE_ON_ERROR == STD_ON)
    if (Det_InternalStatus.freezeOnError == TRUE)
    {
        Det_FreezeOnError();
    }
#endif
    
#else
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
#endif /* DET_ENABLED */

    return result;
}

/**
 * @brief   Starts the Development Error Tracer
 * @details This function is called after initialization and can be used
 *          to perform post-initialization checks or enable notifications.
 * @return  None
 */
FUNC(void, DET_CODE) Det_Start(void)
{
    /* Post-startup actions if needed */
}

#if (DET_VERSION_INFO_API == STD_ON)
/**
 * @brief   Returns the version information of the Det module
 * @param   versioninfo   Pointer to store version information
 * @return  None
 */
FUNC(void, DET_CODE) Det_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, DET_APPL_DATA) versioninfo)
{
#if (DET_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        (void)Det_ReportError(DET_MODULE_ID, 0U, DET_SID_GET_VERSION_INFO, DET_E_PARAM_POINTER);
        return;
    }
#endif
    
    versioninfo->vendorID = DET_VENDOR_ID;
    versioninfo->moduleID = DET_MODULE_ID;
    versioninfo->sw_major_version = DET_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = DET_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = DET_SW_PATCH_VERSION;
}
#endif

#if (DET_RUNTIME_ERROR_REPORTING == STD_ON)
/**
 * @brief   Reports a runtime error
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where error occurred
 * @param   ErrorId     ID of the detected error
 * @return  E_OK if error was handled, E_NOT_OK otherwise
 */
FUNC(Std_ReturnType, DET_CODE) Det_ReportRuntimeError(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (DET_ENABLED == STD_ON)
    
#if (DET_DEV_ERROR_DETECT == STD_ON)
    if (Det_InternalStatus.initStatus != DET_INITIALIZED)
    {
        return E_NOT_OK;
    }
#endif
    
    /* Enter critical section */
    SchM_Enter_Det_DET_EXCLUSIVE_AREA_0();
    
    /* Log the error */
    Det_LogError(ModuleId, InstanceId, ApiId, ErrorId);
    
    /* Call runtime error hooks */
    result = Det_CallRuntimeErrorHooks(ModuleId, InstanceId, ApiId, ErrorId);
    
    /* Exit critical section */
    SchM_Exit_Det_DET_EXCLUSIVE_AREA_0();
    
#else
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
#endif /* DET_ENABLED */

    return result;
}
#endif

#if (DET_TRANSIENT_FAULT_REPORTING == STD_ON)
/**
 * @brief   Reports a transient fault
 * @param   ModuleId    ID of the reporting module
 * @param   InstanceId  ID of the module instance
 * @param   ApiId       ID of the API where fault occurred
 * @param   FaultId     ID of the detected fault
 * @return  E_OK if fault was handled, E_NOT_OK otherwise
 */
FUNC(Std_ReturnType, DET_CODE) Det_ReportTransientFault(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) FaultId)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (DET_ENABLED == STD_ON)
    
#if (DET_DEV_ERROR_DETECT == STD_ON)
    if (Det_InternalStatus.initStatus != DET_INITIALIZED)
    {
        return E_NOT_OK;
    }
#endif
    
    /* Enter critical section */
    SchM_Enter_Det_DET_EXCLUSIVE_AREA_0();
    
    /* Call transient fault hooks */
    result = Det_CallTransientFaultHooks(ModuleId, InstanceId, ApiId, FaultId);
    
    /* Exit critical section */
    SchM_Exit_Det_DET_EXCLUSIVE_AREA_0();
    
#else
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)FaultId;
#endif /* DET_ENABLED */

    return result;
}
#endif

#define DET_STOP_SEC_CODE
#include "MemMap.h"
