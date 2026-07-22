/******************************************************************************
 * @file Platform_Types.h
 * @brief AutoSAR Platform Types (Native Stub for coverage testing)
 ******************************************************************************/
#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifndef PLATFORM_TYPES_H_INCLUDED
#define PLATFORM_TYPES_H_INCLUDED

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef signed char         sint8;
typedef signed short        sint16;
typedef signed int          sint32;
typedef signed long long    sint64;

typedef unsigned int        uint8_least;
typedef unsigned int        uint16_least;
typedef unsigned int        uint32_least;
typedef signed int          sint8_least;
typedef signed int          sint16_least;
typedef signed int          sint32_least;

#ifdef __LP64__
typedef unsigned long       uintptr;
typedef signed long         sintptr;
#else
typedef unsigned int        uintptr;
typedef signed int          sintptr;
#endif

typedef float               float32;
typedef double              float64;

#ifndef TRUE
#define TRUE  1U
#endif
#ifndef FALSE
#define FALSE 0U
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#endif /* PLATFORM_TYPES_H_INCLUDED */
#endif /* PLATFORM_TYPES_H */
