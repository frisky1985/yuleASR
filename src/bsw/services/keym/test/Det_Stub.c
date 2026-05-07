/**
 * @file Det_Stub.c
 * @brief DET (Diagnostic Error Tracer) stub for unit testing
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * This is a minimal stub implementation of DET for use in unit tests.
 * It provides a dummy Det_ReportError function that silently ignores
 * all error reports, allowing tests to run without full DET stack.
 */

#include "Std_Types.h"

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}
