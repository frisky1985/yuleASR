/**
 * @file Det.h
 * @brief AUTOSAR Default Error Tracer (Native Stub)
 *
 * Minimal stub for native compilation. Provides Det_ReportError
 * as a no-op so that yuleASR BSW modules compile on x86_64/Darwin.
 */
#ifndef DET_H
#define DET_H

#include "Std_Types.h"

/* Standard DET module ID (AUTOSAR SWS_Det) */
#define DET_MODULE_ID                   ((uint16)15U)

/* Standard error codes */
#define DET_E_PARAM_POINTER             ((uint8)0x01U)
#define DET_E_PARAM_CONFIG              ((uint8)0x02U)
#define DET_E_PARAM_DATA                ((uint8)0x03U)

/* No-op implementation for native compilation */
#define Det_ReportError(ModuleId, InstanceId, ApiId, ErrorId)  ((void)0)

#endif /* DET_H */
