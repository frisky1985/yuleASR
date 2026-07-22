/**
 * @file Det.h
 * @brief DET (Diagnostic Error Tracer) stub for MQTT unit testing
 *
 * Declares a global call counter so Mqtt_test.c can verify DET was called.
 * Both Mqtt.c and Mqtt_test.c include this header and share the same
 * Det_ReportError function (defined in Mqtt_test.c).
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#ifndef DET_STUB_H
#define DET_STUB_H

#include "Std_Types.h"

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/* Global DET call counter — defined in Mqtt_test.c, shared across all TUs */
extern int Mqtt_Det_ReportError_CallCount;

/* DET report function — defined in Mqtt_test.c */
extern Std_ReturnType Det_ReportError(uint16 ModuleId,
                                      uint8 InstanceId,
                                      uint8 ApiId,
                                      uint8 ErrorId);

#endif /* DET_STUB_H */
