/**
 * @file Std_Types.h
 * @brief AutoSAR Standard Types — bare-metal compatible
 * @version 1.0.0
 * @date 2026-07-12
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;
typedef signed char         sint8;
typedef signed short        sint16;
typedef signed int          sint32;
typedef signed long long    sint64;
typedef float               float32;
typedef double              float64;
typedef signed short        int16;
typedef signed int          int32;
typedef unsigned char       boolean;
typedef uint8               Std_ReturnType;

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
#define NULL                ((void*)0U)
#define NULL_PTR            ((void*)0U)

typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

#endif /* STD_TYPES_H */
