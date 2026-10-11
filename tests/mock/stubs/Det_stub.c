#include "Det.h"

void Det_Init(const Det_ConfigType* ConfigPtr) {
    (void)ConfigPtr;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

void Det_Start(void) {
}

Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)FaultId;
    return E_OK;
}
