/** @file Det.c
 * @brief Default Error Tracer (Det) implementation
 * @details AUTOSAR R22-11 compliant Det module
 * @copyright yuLiang Embedded Technology Co., Ltd.
 */

#include "Det.h"
#include <stdio.h>

/*=============================================================================
 * Configuration
 *=============================================================================*/
#ifndef DET_DEBUG_PRINT
#define DET_DEBUG_PRINT 1
#endif

/*=============================================================================
 * Internal Error Log
 *=============================================================================*/
#define DET_MAX_ERRORS 32U

typedef struct {
    uint16 moduleId;
    uint8 instanceId;
    uint8 apiId;
    uint8 errorId;
    uint32 timestamp;
} Det_ErrorLogType;

static Det_ErrorLogType Det_ErrorLog[DET_MAX_ERRORS];
static uint16 Det_ErrorCount = 0U;
static uint32 Det_Timestamp = 0U;

/*=============================================================================
 * Helper Functions
 *=============================================================================*/
static void Det_LogError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    if (Det_ErrorCount < DET_MAX_ERRORS) {
        Det_ErrorLog[Det_ErrorCount].moduleId = ModuleId;
        Det_ErrorLog[Det_ErrorCount].instanceId = InstanceId;
        Det_ErrorLog[Det_ErrorCount].apiId = ApiId;
        Det_ErrorLog[Det_ErrorCount].errorId = ErrorId;
        Det_ErrorLog[Det_ErrorCount].timestamp = Det_Timestamp++;
        Det_ErrorCount++;
    }
    
#if DET_DEBUG_PRINT
    /* Log to stderr for development debugging */
    fprintf(stderr, "[DET] Module=0x%02X, Instance=%d, API=0x%02X, Error=0x%02X\n",
            ModuleId, InstanceId, ApiId, ErrorId);
#endif
}

static const char* Det_GetModuleName(uint16 ModuleId) {
    switch (ModuleId) {
        case 0x11U: return "WdgIf";
        case 0x12U: return "WdgM";
        default: return "Unknown";
    }
}

static const char* Det_GetWdgMErrorName(uint8 ErrorId) {
    switch (ErrorId) {
        case WDGM_E_NO_INIT: return "E_NO_INIT";
        case WDGM_E_ALREADY_INITIALIZED: return "E_ALREADY_INITIALIZED";
        case WDGM_E_PARAM_CONFIG: return "E_PARAM_CONFIG";
        case WDGM_E_PARAM_MODE: return "E_PARAM_MODE";
        case WDGM_E_INV_POINTER: return "E_INV_POINTER";
        case WDGM_E_CPID_OUT_OF_RANGE: return "E_CPID_OUT_OF_RANGE";
        case WDGM_E_SupervisedEntityId: return "E_SupervisedEntityId";
        case WDGM_E_ClearSEStatusFailed: return "E_ClearSEStatusFailed";
        case WDGM_E_DeinitNotAllowed: return "E_DeinitNotAllowed";
        default: return "UNKNOWN";
    }
}

static const char* Det_GetWdgIfErrorName(uint8 ErrorId) {
    switch (ErrorId) {
        case WDGIF_E_PARAM_DEVICE: return "E_PARAM_DEVICE";
        case WDGIF_E_INV_POINTER: return "E_INV_POINTER";
        case WDGIF_E_INIT_FAILED: return "E_INIT_FAILED";
        case WDGIF_E_MODE_FAILED: return "E_MODE_FAILED";
        default: return "UNKNOWN";
    }
}

/*=============================================================================
 * Det API Implementation
 *=============================================================================*/
Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    Det_LogError(ModuleId, InstanceId, ApiId, ErrorId);
    
#if DET_DEBUG_PRINT
    const char* moduleName = Det_GetModuleName(ModuleId);
    const char* errorName = NULL_PTR;
    
    if (ModuleId == WDGM_MODULE_ID) {
        errorName = Det_GetWdgMErrorName(ErrorId);
    } else if (ModuleId == WDGIF_MODULE_ID) {
        errorName = Det_GetWdgIfErrorName(ErrorId);
    }
    
    if (errorName != NULL_PTR) {
        fprintf(stderr, "[DET] %s Error: %s\n", moduleName, errorName);
    }
#endif
    
    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    /* Runtime errors are logged similarly to development errors */
    Det_LogError(ModuleId, InstanceId, ApiId, ErrorId);
    return E_OK;
}

Std_ReturnType Det_ReportTransientFault(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId)
{
    /* Transient faults are logged but may not require full error handling */
    Det_LogError(ModuleId, InstanceId, ApiId, FaultId);
    return E_OK;
}
