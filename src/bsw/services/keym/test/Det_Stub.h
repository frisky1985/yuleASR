/**
 * @file Det_Stub.h
 * @brief DET (Diagnostic Error Tracer) stub header for unit testing
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * This header provides a minimal stub interface for DET.
 * Include this in test files instead of the full Det.h when
 * you don't need actual error reporting functionality.
 */

#ifndef DET_STUB_H
#define DET_STUB_H

#include "Std_Types.h"

static inline Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

#endif /* DET_STUB_H */
