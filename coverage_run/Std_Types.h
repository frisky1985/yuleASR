/* Std_Types.h - Coverage test stub (no macro conflicts) */
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stddef.h>

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

/* AUTOSAR version defines for version checks */
#ifndef STD_TYPES_AR_RELEASE_MAJOR_VERSION
#define STD_TYPES_AR_RELEASE_MAJOR_VERSION 4U
#endif
#ifndef STD_TYPES_AR_RELEASE_MINOR_VERSION
#define STD_TYPES_AR_RELEASE_MINOR_VERSION 7U
#endif

/* Std_VersionInfoType */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

#endif
