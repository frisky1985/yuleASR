/* Det.c - Development Error Tracer stub for coverage testing */
#include "Det.h"

Det_MockDataType Det_MockData = {0};

void Det_Init(void)
{
    Det_Mock_Reset();
}

void Det_Start(void)
{
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                                uint8 ApiId, uint8 ErrorId)
{
    Det_MockData.ModuleId = ModuleId;
    Det_MockData.InstanceId = InstanceId;
    Det_MockData.ApiId = ApiId;
    Det_MockData.ErrorId = ErrorId;
    Det_MockData.CallCount++;
    Det_MockData.LastCallValid = 1;
    return E_OK;
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId,
                                       uint8 ApiId, uint8 ErrorId)
{
    return Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId);
}

Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId,
                                         uint8 ApiId, uint8 ErrorId)
{
    return Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId);
}

void Det_Mock_Reset(void)
{
    Det_MockData.ModuleId = 0;
    Det_MockData.InstanceId = 0;
    Det_MockData.ApiId = 0;
    Det_MockData.ErrorId = 0;
    Det_MockData.CallCount = 0;
    Det_MockData.LastCallValid = 0;
}
