/**
 * @file Platform_Types.h
 * @brief AUTOSAR Platform Types - stub for compilation
 */
#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* AUTOSAR standard type definitions */
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t   sint8;
typedef int16_t  sint16;
typedef int32_t  sint32;
typedef int64_t  sint64;

typedef uint8_t  UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef uint64_t UInt64;

typedef int8_t   Sint8;
typedef int16_t  Sint16;
typedef int32_t  Sint32;
typedef int64_t  Sint64;

typedef volatile int8_t   vint8_t;
typedef volatile int16_t  vint16_t;
typedef volatile int32_t  vint32_t;
typedef volatile int64_t  vint64_t;

typedef volatile uint8_t  vuint8_t;
typedef volatile uint16_t vuint16_t;
typedef volatile uint32_t vuint32_t;
typedef volatile uint64_t vuint64_t;

typedef float   float32;
typedef double  float64;

typedef uint8  boolean;
typedef uint32 Std_ReturnType;
typedef uint32 uint32_least;
typedef uint16 uint16_least;
typedef uint8  uint8_least;

#ifndef TRUE
#define TRUE  1u
#endif
#ifndef FALSE
#define FALSE 0u
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#ifndef E_OK
#define E_OK       0u
#endif
#ifndef E_NOT_OK
#define E_NOT_OK   1u
#endif

#endif /* PLATFORM_TYPES_H */
