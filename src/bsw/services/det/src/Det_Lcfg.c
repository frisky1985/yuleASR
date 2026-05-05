/*==================================================================================================
* Project          : AUTOSAR Reference Implementation
* File Name        : Det_Lcfg.c
* Description      : Development Error Tracer link-time configuration
*                    Contains configurable tables, hooks, and module configurations.
*==================================================================================================
* (C) Copyright 2024, yuleASR
*==================================================================================================*/

/*==================================================================================================
*                                         INCLUDE FILES
==================================================================================================*/
#include "Det.h"

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL TYPES
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      ERROR HOOK FUNCTIONS
==================================================================================================*/

/**
 * @brief   Default error hook implementation
 * @details This is the default error handler called when a development error is reported.
 *          Can be customized to log errors to non-volatile memory, serial output, etc.
 */
LOCAL FUNC(Std_ReturnType, DET_CODE) Det_DefaultErrorHook(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId)
{
    /* Default behavior: return E_NOT_OK to indicate error was not recovered */
    /* This can be extended to implement custom error handling */
    
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    
    return E_NOT_OK;
}

#if (DET_RUNTIME_ERROR_REPORTING == STD_ON)
/**
 * @brief   Default runtime error hook implementation
 */
LOCAL FUNC(Std_ReturnType, DET_CODE) Det_DefaultRuntimeErrorHook(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) ErrorId)
{
    /* Default behavior: return E_NOT_OK */
    
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    
    return E_NOT_OK;
}
#endif

#if (DET_TRANSIENT_FAULT_REPORTING == STD_ON)
/**
 * @brief   Default transient fault hook implementation
 */
LOCAL FUNC(Std_ReturnType, DET_CODE) Det_DefaultTransientFaultHook(
    VAR(uint16, AUTOMATIC) ModuleId,
    VAR(uint8, AUTOMATIC) InstanceId,
    VAR(uint8, AUTOMATIC) ApiId,
    VAR(uint8, AUTOMATIC) FaultId)
{
    /* Default behavior: return E_NOT_OK */
    
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)FaultId;
    
    return E_NOT_OK;
}
#endif

/*==================================================================================================
*                                      CONFIGURATION TABLES
==================================================================================================*/

#define DET_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief   Default error hooks table
 * @details Array of function pointers to error hook callbacks.
 *          These hooks are called in sequence when Det_ReportError is invoked.
 */
CONST(Det_ErrorHookPtrType, DET_CONST) Det_ErrorHooks[DET_NUMBER_OF_ERROR_HOOKS] =
{
    &Det_DefaultErrorHook
    /* Additional hooks can be added here */
};

#if (DET_RUNTIME_ERROR_REPORTING == STD_ON)
/**
 * @brief   Default runtime error hooks table
 */
CONST(Det_RuntimeErrorHookPtrType, DET_CONST) Det_RuntimeErrorHooks[DET_NUMBER_OF_RUNTIME_ERROR_HOOKS] =
{
    &Det_DefaultRuntimeErrorHook
};
#endif

#if (DET_TRANSIENT_FAULT_REPORTING == STD_ON)
/**
 * @brief   Default transient fault hooks table
 */
CONST(Det_TransientFaultHookPtrType, DET_CONST) Det_TransientFaultHooks[DET_NUMBER_OF_TRANSIENT_FAULT_HOOKS] =
{
    &Det_DefaultTransientFaultHook
};
#endif

#define DET_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      MODULE CONFIGURATION
==================================================================================================*/

#define DET_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/**
 * @brief   Post-build configuration structure
 * @details This structure contains post-build configurable parameters
 *          that can be modified without recompiling the Det module.
 */
CONST(Det_ConfigType, DET_CONST) Det_Config =
{
    /* DetEnableFreezeOnError */    (boolean)DET_FREEZE_ON_ERROR,
    /* DetEnableLogging */          (boolean)DET_ERROR_LOGGING_ENABLED,
    /* DetMaxErrorEntries */        (uint8)DET_MAX_ERROR_ENTRIES
};

#define DET_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      EXTERNAL REFERENCES
==================================================================================================*/

/* 
 * Note: Project-specific error hooks can be defined externally.
 * To add custom error hooks:
 * 1. Define DET_NUMBER_OF_ERROR_HOOKS to the total number of hooks
 * 2. Declare the hook function(s) with Det_ErrorHookPtrType signature
 * 3. Add function pointers to Det_ErrorHooks array above
 */

/* Example custom hook declaration (to be implemented in application):
 * extern Std_ReturnType MyCustomErrorHandler(uint16 ModuleId, uint8 InstanceId, 
 *                                            uint8 ApiId, uint8 ErrorId);
 * 
 * Then add to Det_ErrorHooks array:
 * CONST(Det_ErrorHookPtrType, DET_CONST) Det_ErrorHooks[DET_NUMBER_OF_ERROR_HOOKS] =
 * {
 *     &Det_DefaultErrorHook,
 *     &MyCustomErrorHandler
 * };
 */
