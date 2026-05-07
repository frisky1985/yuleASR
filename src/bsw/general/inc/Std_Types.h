/**
 * @file Std_Types.h
 * @brief Standard types header for AUTOSAR BSW modules
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Standard Types
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

/*==================================================================================================
*                                          INCLUDES
==================================================================================================*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*==================================================================================================
*                                    STANDARD RETURN TYPE
==================================================================================================*/
typedef uint8 Std_ReturnType;

/*==================================================================================================
*                                    STANDARD RETURN VALUES
==================================================================================================*/
#ifndef E_OK
#define E_OK            (0x00U)
#endif

#ifndef E_NOT_OK
#define E_NOT_OK        (0x01U)
#endif

/*==================================================================================================
*                                    STANDARD BOOLEAN VALUES
==================================================================================================*/
#ifndef TRUE
#define TRUE            (1U)
#endif

#ifndef FALSE
#define FALSE           (0U)
#endif

typedef bool boolean;

/*==================================================================================================
*                                    NULL POINTER
==================================================================================================*/
#ifndef NULL_PTR
#define NULL_PTR        ((void*)0)
#endif

/*==================================================================================================
*                                    STD_ON/OFF
==================================================================================================*/
#ifndef STD_ON
#define STD_ON          (1U)
#endif

#ifndef STD_OFF
#define STD_OFF         (0U)
#endif

#ifndef STD_HIGH
#define STD_HIGH        (1U)
#endif

#ifndef STD_LOW
#define STD_LOW         (0U)
#endif

#ifndef STD_ACTIVE
#define STD_ACTIVE      (1U)
#endif

#ifndef STD_IDLE
#define STD_IDLE        (0U)
#endif

/*==================================================================================================
*                                    VERSION INFO TYPE
==================================================================================================*/
typedef struct {
    uint16 vendorID;
    uint16 moduleID;
    uint8 sw_major_version;
    uint8 sw_minor_version;
    uint8 sw_patch_version;
} Std_VersionInfoType;

#endif /* STD_TYPES_H */
