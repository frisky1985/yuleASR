/******************************************************************************
 * @file Std_Types.h
 * @brief AutoSAR Standard Types (Native Stub for coverage testing)
 * @details Standalone copy in include/autosar/ so all native builds find it.
 *
 * (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 ******************************************************************************/
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Boolean ─────────────────────────────────────────────────────────── */
typedef unsigned char       boolean;
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

/* ─── Boolean constants ──────────────────────────────────────────────── */
#ifndef TRUE
#define TRUE  1U
#endif
#ifndef FALSE
#define FALSE 0U
#endif

/* ─── AR version info (for Det.h version checks) ────────────────────── */
#define STD_TYPES_AR_RELEASE_MAJOR_VERSION      (4u)
#define STD_TYPES_AR_RELEASE_MINOR_VERSION      (7u)
#define STD_TYPES_AR_RELEASE_REVISION_VERSION   (0u)
#define STD_TYPES_SW_MAJOR_VERSION              (1u)
#define STD_TYPES_SW_MINOR_VERSION              (0u)
#define STD_TYPES_SW_PATCH_VERSION              (0u)
#define STD_TYPES_VENDOR_ID                     (0x00u)
#define STD_TYPES_MODULE_ID                     (0x00u)

/* ─── Std_ReturnType ─────────────────────────────────────────────────── */
typedef uint8 Std_ReturnType;
#ifndef E_OK
#define E_OK     ((Std_ReturnType)0U)
#endif
#ifndef E_NOT_OK
#define E_NOT_OK ((Std_ReturnType)1U)
#endif

/* ─── Std_VersionInfoType ────────────────────────────────────────────── */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

/* ─── Standard constants ────────────────────────────────────────────── */
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#ifndef STD_ON
#define STD_ON  1U
#endif
#ifndef STD_OFF
#define STD_OFF 0U
#endif
#ifndef STD_HIGH
#define STD_HIGH 1U
#endif
#ifndef STD_LOW
#define STD_LOW  0U
#endif
#ifndef STD_ACTIVE
#define STD_ACTIVE  1U
#endif
#ifndef STD_IDLE
#define STD_IDLE    0U
#endif

/* ─── Memory abstraction wrappers ────────────────────────────────────── */
#ifndef MEMMAP_ERROR
#define FUNC(ret, mem)          ret
#define VAR(ty, mem)            ty
#define CONST(ty, mem)          const ty
#define P2VAR(ty, mem, cls)     ty*
#define P2CONST(ty, mem, cls)   const ty*
#define CONSTP2VAR(ty, m, c)    ty* const
#define CONSTP2CONST(ty, m, c)  const ty* const
#define P2FUNC(ret, cls, fn)    ret(*fn)
#define CONSTP2FUNC(ret, c, fn) ret(*const fn)
#endif

/* ─── Compiler abstraction ──────────────────────────────────────────── */
#ifndef AUTOMATIC
#define AUTOMATIC
#endif
#ifndef TYPEDEF
#define TYPEDEF
#endif
#ifndef INLINE
#define INLINE inline
#endif
#ifndef LOCAL_INLINE
#define LOCAL_INLINE static inline
#endif
#ifndef STATIC
#define STATIC static
#endif

/* ─── NULL_PTR derived macros ───────────────────────────────────────── */
#ifndef NULL
#define NULL NULL_PTR
#endif

/* ─── UNUSED ──────────────────────────────────────────────────────────── */
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/* ─── ARRAY_SIZE ──────────────────────────────────────────────────────── */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

#ifdef __cplusplus
}
#endif

#endif /* STD_TYPES_H */
