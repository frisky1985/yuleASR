/**
 * @file ComStack_Types.h
 * @brief ComStack types stub for MQTT unit testing
 *
 * Provides minimal type definitions needed by Mqtt_Cfg.h for compilation.
 *
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 */

#ifndef COMSTACK_TYPES_STUB_H
#define COMSTACK_TYPES_STUB_H

#include "Std_Types.h"

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/* BufReq_ReturnType — used in COM stack (minimal def for compilation) */
typedef uint8 BufReq_ReturnType;

#define BUFREQ_OK        ((BufReq_ReturnType)0x00U)
#define BUFREQ_E_NOT_OK  ((BufReq_ReturnType)0x01U)
#define BUFREQ_E_BUSY    ((BufReq_ReturnType)0x02U)
#define BUFREQ_E_OVFL    ((BufReq_ReturnType)0x03U)

#endif /* COMSTACK_TYPES_STUB_H */
