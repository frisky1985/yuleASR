/**
 * @file Std_Types.h
 * @brief 标准类型定义
 * @version 1.0.0
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>

/* 版本信息 */
#define STD_TYPES_MAJOR_VERSION         1
#define STD_TYPES_MINOR_VERSION         0
#define STD_TYPES_PATCH_VERSION         0

/* 基本数据类型 - 使用标准stdint类型 */
typedef uint8_t             uint8;
typedef uint16_t            uint16;
typedef uint32_t            uint32;
typedef int8_t              sint8;
typedef int16_t             sint16;
typedef int32_t             sint32;
typedef float               float32;
typedef double              float64;

/* 布尔类型 */
typedef unsigned char       boolean;

/* 标准返回类型 */
typedef uint8 Std_ReturnType;

/* 常见返回值 */
#ifndef E_OK
#define E_OK                    0x00
#endif

#ifndef E_NOT_OK
#define E_NOT_OK                0x01
#endif

/* 布尔值 */
#ifndef TRUE
#define TRUE                    1
#endif

#ifndef FALSE
#define FALSE                   0
#endif

/* 使能/禁能宏 */
#define STD_ON                  1
#define STD_OFF                 0

/* NULL指针 */
#ifndef NULL
#define NULL                    ((void*)0)
#endif

#ifndef NULL_PTR
#define NULL_PTR                ((void*)0)
#endif

/* 版本信息类型 */
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8  sw_major_version;
    uint8  sw_minor_version;
    uint8  sw_patch_version;
} Std_VersionInfoType;

#endif /* STD_TYPES_H */
