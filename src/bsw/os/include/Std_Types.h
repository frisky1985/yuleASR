/******************************************************************************
 * @file Std_Types.h
 * @brief Standard Type Definitions (AutoSAR)
 * @details This file contains the standard type definitions required by
 *          the AutoSAR OS implementation, following AUTOSAR_SWS_StandardTypes.
 *
 * @author YuleTech
 * @version 1.0.0
 * @date 2026-04-30
 ******************************************************************************/

#ifndef STD_TYPES_H
#define STD_TYPES_H

/*******************************************************************************
 * Platform Types (simulated - should come from PlatformTypes.h)
 ******************************************************************************/
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

/* Plain int types for compatibility */
typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;
typedef signed long long    int64;

typedef unsigned int        uint8_least;
typedef unsigned int        uint16_least;
typedef unsigned int        uint32_least;

typedef signed int          sint8_least;
typedef signed int          sint16_least;
typedef signed int          sint32_least;

#if defined(__x86_64__) || defined(__aarch64__)
typedef unsigned long       uintptr;
typedef signed long         sintptr;
#else
typedef unsigned int        uintptr;
typedef signed int          sintptr;
#endif

typedef float               float32;
typedef double              float64;

#ifndef TRUE
    #define TRUE    (1U)
#endif

#ifndef FALSE
    #define FALSE   (0U)
#endif

#endif /* PLATFORM_TYPES_H_INCLUDED */

/*******************************************************************************
 * Standard Types
 ******************************************************************************/

/* Boolean type */
typedef uint8 boolean;

#ifndef TRUE
    #define TRUE    ((boolean)1U)
#endif

#ifndef FALSE
    #define FALSE   ((boolean)0U)
#endif

/* Standard return type */
typedef uint8 Std_ReturnType;

/* Return values */
#ifndef E_OK
    #define E_OK        ((Std_ReturnType)0U)
#endif

#ifndef E_NOT_OK
    #define E_NOT_OK    ((Std_ReturnType)1U)
#endif

/*******************************************************************************
 * Standard Version Info Type
 ******************************************************************************/
#ifndef STD_VERSIONINFO_TYPE_DEFINED
#define STD_VERSIONINFO_TYPE_DEFINED
typedef struct
{
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;
#endif

/*******************************************************************************
 * Standard Macros
 ******************************************************************************/

/* NULL pointer definition */
#ifndef NULL
    #define NULL    ((void *)0)
#ifndef NULL_PTR
    #define NULL_PTR ((void *)0)
#endif
#endif

/* On/Off constants */
#ifndef STD_ON
    #define STD_ON      (1U)
#endif

#ifndef STD_OFF
    #define STD_OFF     (0U)
#endif

/* Active/Inactive constants */
#ifndef STD_ACTIVE
    #define STD_ACTIVE  (1U)
#endif

#ifndef STD_IDLE
    #define STD_IDLE    (0U)
#endif

/* High/Low constants */
#ifndef STD_HIGH
    #define STD_HIGH    (1U)
#endif

#ifndef STD_LOW
    #define STD_LOW     (0U)
#endif

/*******************************************************************************
 * Null Pointer Check Macro
 ******************************************************************************/
#define STD_NULL_PTR_CHECK(ptr, ret)    \
    do {                                \
        if ((ptr) == NULL) {            \
            return (ret);               \
        }                               \
    } while(0)

/*******************************************************************************
 * Memory Section Abstraction (compiler specific)
 ******************************************************************************/
#ifndef MEMMAP_ERROR
    /* Default memory section macros */
    #define VAR(vartype, memclass)                  vartype
    #define CONST(consttype, memclass)              const consttype
    #define FUNC(rettype, memclass)                 rettype
    #define P2VAR(ptrtype, memclass, ptrclass)      ptrtype *
    #define P2CONST(ptrtype, memclass, ptrclass)    const ptrtype *
    #define CONSTP2VAR(ptrtype, memclass, ptrclass) ptrtype * const
    #define CONSTP2CONST(ptrtype, memclass, ptrclass) const ptrtype * const
    #define P2FUNC(rettype, ptrclass, fctname)      rettype (*fctname)
    #define CONSTP2FUNC(rettype, ptrclass, fctname) rettype (* const fctname)
#endif

/*******************************************************************************
 * Compiler Abstraction
 ******************************************************************************/
#ifdef __GNUC__
    #define INLINE              inline
    #define LOCAL_INLINE        static inline
    #define FUNC_P2CONST(rettype, ptrclass, memclass) const rettype * memclass
    #define FUNC_P2VAR(rettype, ptrclass, memclass)   rettype * memclass
#elif defined(__IAR_SYSTEMS_ICC__)
    #define INLINE              inline
    #define LOCAL_INLINE        static inline
    #define FUNC_P2CONST(rettype, ptrclass, memclass) const rettype * memclass
    #define FUNC_P2VAR(rettype, ptrclass, memclass)   rettype * memclass
#elif defined(__TASKING__)
    #define INLINE              inline
    #define LOCAL_INLINE        static inline
    #define FUNC_P2CONST(rettype, ptrclass, memclass) const rettype * memclass
    #define FUNC_P2VAR(rettype, ptrclass, memclass)   rettype * memclass
#else
    #define INLINE
    #define LOCAL_INLINE        static
    #define FUNC_P2CONST(rettype, ptrclass, memclass) const rettype *
    #define FUNC_P2VAR(rettype, ptrclass, memclass)   rettype *
#endif

/*******************************************************************************
 * Unused Parameter Macro
 ******************************************************************************/
#ifndef UNUSED
    #define UNUSED(x)   ((void)(x))
#endif

/*******************************************************************************
 * Alignment Macros
 ******************************************************************************/
#ifndef ALIGN
    #if defined(__GNUC__)
        #define ALIGN(n)    __attribute__((aligned(n)))
    #elif defined(__IAR_SYSTEMS_ICC__)
        #define ALIGN(n)    _Pragma(#n)
    #else
        #define ALIGN(n)
    #endif
#endif

/*******************************************************************************
 * Packed Structure Macro
 ******************************************************************************/
#ifndef PACKED
    #if defined(__GNUC__)
        #define PACKED      __attribute__((packed))
    #elif defined(__IAR_SYSTEMS_ICC__)
        #define PACKED      _Pragma("pack(1)")
    #else
        #define PACKED
    #endif
#endif

/*******************************************************************************
 * Volatile Access Macro
 ******************************************************************************/
#ifndef REG_READ32
    #define REG_READ32(address)         (*(volatile uint32 *)(uintptr)(address))
#endif

#ifndef REG_WRITE32
    #define REG_WRITE32(address, value) (*(volatile uint32 *)(uintptr)(address) = (value))
#endif

#ifndef REG_READ16
    #define REG_READ16(address)         (*(volatile uint16 *)(uintptr)(address))
#endif

#ifndef REG_WRITE16
    #define REG_WRITE16(address, value) (*(volatile uint16 *)(uintptr)(address) = (value))
#endif

#ifndef REG_READ8
    #define REG_READ8(address)          (*(volatile uint8 *)(uintptr)(address))
#endif

#ifndef REG_WRITE8
    #define REG_WRITE8(address, value)  (*(volatile uint8 *)(uintptr)(address) = (value))
#endif

/*******************************************************************************
 * Bit Manipulation Macros
 ******************************************************************************/
#define SET_BIT(reg, bit)       ((reg) |= (1U << (bit)))
#define CLEAR_BIT(reg, bit)     ((reg) &= ~(1U << (bit)))
#define TOGGLE_BIT(reg, bit)    ((reg) ^= (1U << (bit)))
#define READ_BIT(reg, bit)      (((reg) >> (bit)) & 1U)
#define CLEAR_REG(reg)          ((reg) = 0U)
#define WRITE_REG(reg, val)     ((reg) = (val))
#define READ_REG(reg)           ((reg))

/*******************************************************************************
 * Array Size Macro
 ******************************************************************************/
#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#endif

/*******************************************************************************
 * Min/Max Macros
 ******************************************************************************/
#ifndef MIN
    #define MIN(a, b)   (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
    #define MAX(a, b)   (((a) > (b)) ? (a) : (b))
#endif

/*******************************************************************************
 * Swap Endian Macros
 ******************************************************************************/
#define SWAP_UINT16(val)    ((((val) & 0x00FFU) << 8) | \
                             (((val) & 0xFF00U) >> 8))

#define SWAP_UINT32(val)    ((((val) & 0x000000FFUL) << 24) | \
                             (((val) & 0x0000FF00UL) << 8)  | \
                             (((val) & 0x00FF0000UL) >> 8)  | \
                             (((val) & 0xFF000000UL) >> 24))

/*******************************************************************************
 * Critical Section Macros (for small critical sections)
 ******************************************************************************/
#define ENTER_CRITICAL_SECTION()    Os_DisableAllInterrupts()
#define EXIT_CRITICAL_SECTION()     Os_EnableAllInterrupts()

/*******************************************************************************
 * Version Information
 ******************************************************************************/
#define STD_TYPES_VENDOR_ID             (0x00U)
#define STD_TYPES_MODULE_ID             (0x00U)
#define STD_TYPES_SW_MAJOR_VERSION      (1U)
#define STD_TYPES_SW_MINOR_VERSION      (0U)
#define STD_TYPES_SW_PATCH_VERSION      (0U)

#define STD_TYPES_AR_RELEASE_MAJOR_VERSION      (4U)
#define STD_TYPES_AR_RELEASE_MINOR_VERSION      (7U)
#define STD_TYPES_AR_RELEASE_REVISION_VERSION   (0U)

/*******************************************************************************
 * Legacy Support
 ******************************************************************************/
/* Support legacy naming conventions */
/* NOTE: int8_t/uint8_t/... are NOT defined here; they are provided by the
 * standard <stdint.h> (host libc or the freestanding headers used for
 * bare-metal cross builds).  AUTOSAR code should use sint8/uint8/... */

#endif /* STD_TYPES_H */
