/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : DET Mock Implementation
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "mock_det.h"

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
Det_MockDataType Det_MockData = {0};

/*==================================================================================================
*                                      MOCK IMPLEMENTATION
==================================================================================================*/
void Det_Init(void)
{
    Det_Mock_Reset();
}

void Det_Start(void)
{
    /* Nothing to do in mock */
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    Det_MockData.ModuleId = ModuleId;
    Det_MockData.InstanceId = InstanceId;
    Det_MockData.ApiId = ApiId;
    Det_MockData.ErrorId = ErrorId;
    Det_MockData.CallCount++;
    Det_MockData.LastCallValid = TRUE;
    
    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    return Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId);
}

Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    return Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId);
}

/*==================================================================================================
*                                      MOCK CONTROL FUNCTIONS
==================================================================================================*/
void Det_Mock_Reset(void)
{
    Det_MockData.ModuleId = 0;
    Det_MockData.InstanceId = 0;
    Det_MockData.ApiId = 0;
    Det_MockData.ErrorId = 0;
    Det_MockData.CallCount = 0;
    Det_MockData.LastCallValid = FALSE;
}

uint32 Det_Mock_GetCallCount(void)
{
    return Det_MockData.CallCount;
}

void Det_Mock_GetLastError(uint16* ModuleId, uint8* InstanceId, uint8* ApiId, uint8* ErrorId)
{
    if (ModuleId != NULL)
    {
        *ModuleId = Det_MockData.ModuleId;
    }
    if (InstanceId != NULL)
    {
        *InstanceId = Det_MockData.InstanceId;
    }
    if (ApiId != NULL)
    {
        *ApiId = Det_MockData.ApiId;
    }
    if (ErrorId != NULL)
    {
        *ErrorId = Det_MockData.ErrorId;
    }
}
