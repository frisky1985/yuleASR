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

/* Minimal config type so that Det.c (real implementation) and any BSW
 * module including this stub header can compile (diagnosis P0-2 follow-up:
 * stub must provide types, not implementations). */
typedef struct {
    uint16 dummy;  /* Placeholder for configuration parameters */
} Det_ConfigType;

/* Standard DET module ID (AUTOSAR SWS_Det) */
#define DET_MODULE_ID                   ((uint16)15U)

/* Standard error codes */
#define DET_E_PARAM_POINTER             ((uint8)0x01U)
#define DET_E_PARAM_CONFIG              ((uint8)0x02U)
#define DET_E_PARAM_DATA                ((uint8)0x03U)

/* Function declarations — real implementation lives in src/bsw/services/det/src/Det.c.
 * (Do NOT define function-like macros here: they would expand inside Det.c and
 *  break compilation — see diagnosis 2026-08-07 P0-2.) */
extern void Det_Init(const Det_ConfigType* ConfigPtr);
extern Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
extern void Det_Start(void);
extern Std_ReturnType Det_ReportRuntimeError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId);
extern Std_ReturnType Det_ReportTransientFault(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 FaultId);

#endif /* DET_H */
