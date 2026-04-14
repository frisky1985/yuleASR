/**
 * @file Std_Types.h
 * @brief AutoSAR 标准类型定义
 * @version 1.0.0
 * @date 2026-04-13
 * @author YuleTech
 * 
 * @copyright Copyright (c) 2026 YuleTech
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
/** @brief 标准返回类型 - 成功 */
#define E_OK        0x00U

/** @brief 标准返回类型 - 失败 */
#define E_NOT_OK    0x01U

/** @brief 标准返回类型 - 忙 */
#define E_BUSY      0x02U

/** @brief 空指针定义 */
#ifndef NULL_PTR
    #define NULL_PTR ((void *)0)
#endif

/*==================================================================================================
*                                          TYPE DEFINITIONS
==================================================================================================*/
/** @brief 标准返回类型 */
typedef uint8_t Std_ReturnType;

/* Unsigned integer types */
typedef uint8_t   uint8;    /**< 8-bit unsigned integer */
typedef uint16_t  uint16;   /**< 16-bit unsigned integer */
typedef uint32_t  uint32;   /**< 32-bit unsigned integer */
typedef uint64_t  uint64;   /**< 64-bit unsigned integer */

/* Signed integer types */
typedef int8_t    sint8;    /**< 8-bit signed integer */
typedef int16_t   sint16;   /**< 16-bit signed integer */
typedef int32_t   sint32;   /**< 32-bit signed integer */
typedef int64_t   sint64;   /**< 64-bit signed integer */

/* Boolean type */
typedef bool      boolean;  /**< Boolean type */

#ifndef TRUE
    #define TRUE    true
#endif

#ifndef FALSE
    #define FALSE   false
#endif

/*==================================================================================================
*                                     VERSION INFORMATION
==================================================================================================*/
/** @brief 供应商 ID */
#define STD_TYPES_VENDOR_ID                     0x0055U  /* YuleTech */

/** @brief 模块 ID */
#define STD_TYPES_MODULE_ID                     0x00FFU

/** @brief 软件版本 - 主版本号 */
#define STD_TYPES_SW_MAJOR_VERSION              1U

/** @brief 软件版本 - 次版本号 */
#define STD_TYPES_SW_MINOR_VERSION              0U

/** @brief 软件版本 - 补丁版本号 */
#define STD_TYPES_SW_PATCH_VERSION              0U

/*==================================================================================================
*                                       GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                       GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       INLINE FUNCTIONS
==================================================================================================*/

#endif /* STD_TYPES_H */
