/**
 * @file Std_Types.h
 * @brief Standard Type Definitions (AutoSAR) — Bare-metal compatible
 * @version 1.0.0
 * @details Defines AUTOSAR standard types without relying on <stdint.h>,
 *          compatible with arm-none-eabi bare-metal compilation.
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

/*******************************************************************************
 * Platform Types (bare-metal compatible — no stdint.h dependency)
 ******************************************************************************/
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef signed char         sint8;
typedef signed short        sint16;
typedef signed int          sint32;
typedef signed long long    sint64;

#ifndef PLATFORM_TYPES_H_INCLUDED
/* These are only defined if Platform_Types.h was not included first */
typedef unsigned char       uint8_least;
typedef unsigned short      uint16_least;
typedef unsigned int        uint32_least;
typedef signed char         sint8_least;
typedef signed short        sint16_least;
typedef signed int          sint32_least;
#endif

typedef float               float32;
typedef double              float64;

/*******************************************************************************
 * Boolean Type
 ******************************************************************************/
typedef unsigned char       boolean;

/*******************************************************************************
 * Standard Return Type
 ******************************************************************************/
typedef uint8               Std_ReturnType;

/*******************************************************************************
 * Common Return Values
 ******************************************************************************/
#ifndef E_OK
#define E_OK                0x00U
#endif

#ifndef E_NOT_OK
#define E_NOT_OK            0x01U
#endif

/*******************************************************************************
 * Boolean Values
 ******************************************************************************/
#ifndef TRUE
#define TRUE                1U
#endif

#ifndef FALSE
#define FALSE               0U
#endif

/*******************************************************************************
 * Standard On/Off
 ******************************************************************************/
#define STD_ON              1U
#define STD_OFF             0U

#define STD_HIGH            1U
#define STD_LOW             0U

#define STD_ACTIVE          1U
#define STD_IDLE            0U

/*******************************************************************************
 * NULL Pointer
 ******************************************************************************/
#ifndef NULL
#define NULL                ((void*)0U)
#endif

#ifndef NULL_PTR
#define NULL_PTR            ((void*)0U)
#endif

/*******************************************************************************
 * Version Info Type
 ******************************************************************************/
#ifndef STD_VERSIONINFO_TYPE_DEFINED
#define STD_VERSIONINFO_TYPE_DEFINED
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;
#endif

/*******************************************************************************
 * Module Version Check Macro
 ******************************************************************************/
#define STD_VERSION_CHECK(exp, act)                                             \
    do {                                                                        \
        if ((exp) != (act)) {                                                   \
            /* Version mismatch — handled by caller */                          \
        }                                                                       \
    } while (0U)

#endif /* STD_TYPES_H */
