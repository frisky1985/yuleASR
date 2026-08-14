/**
 * @file coverage_stubs.h
 * @brief Stub definitions for coverage test compilation
 *
 * Provides minimal stubs for AUTOSAR types and Det module.
 */
#ifndef COVERAGE_STUBS_H
#define COVERAGE_STUBS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* --- Standard AUTOSAR types (test stub, no Std_Types.h conflicts) --- */
typedef uint8_t             uint8;
typedef uint16_t            uint16;
typedef uint32_t            uint32;
typedef uint64_t            uint64;
typedef int8_t              sint8;
typedef int16_t             sint16;
typedef int32_t             sint32;
typedef int64_t             sint64;
typedef float               float32;
typedef double              float64;
typedef uint8_t             boolean;
typedef uint8_t             Std_ReturnType;
typedef uint16_t            PduIdType;

#define E_OK                0x00U
#define E_NOT_OK            0x01U
#define TRUE                1U
#define FALSE               0U
#define STD_ON              1U
#define STD_OFF             0U
#define STD_HIGH            1U
#define STD_LOW             0U
#define STD_ACTIVE          1U
#define STD_IDLE            0U
#ifndef NULL
#define NULL                ((void*)0U)
#endif
#define NULL_PTR            ((void*)0U)
#define STATIC              static

/* Std_VersionInfoType */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

/* Important: This file defines Std_Types types without version check macros.
   Include this BEFORE any AUTOSAR headers that check version compatibility. */

#endif /* COVERAGE_STUBS_H */
