/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/** @file WdgIf.h
 * @brief Watchdog Interface header file
 * 
 * AUTOSAR R22-11 compliant WdgIf module
 * ECUAL layer - Watchdog Driver Interface
 */

#ifndef WDGIF_H
#define WDGIF_H

/*============================================================================
 *  AUTOSAR VERSION INFORMATION
 *===========================================================================*/
#define WDGIF_AR_RELEASE_MAJOR_VERSION      4
#define WDGIF_AR_RELEASE_MINOR_VERSION      7
#define WDGIF_AR_RELEASE_REVISION_VERSION   0

#define WDGIF_SW_MAJOR_VERSION              1
#define WDGIF_SW_MINOR_VERSION              0
#define WDGIF_SW_PATCH_VERSION              0

/*============================================================================
 *  INCLUDES
 *===========================================================================*/
#include "Std_Types.h"

/*============================================================================
 *  VERSION CHECK
 *===========================================================================*/
#if (STD_TYPES_AR_RELEASE_MAJOR_VERSION != WDGIF_AR_RELEASE_MAJOR_VERSION)
    #error "WdgIf: AR major version mismatch with Std_Types.h"
#endif

/*============================================================================
 *  MODULE INFORMATION
 *===========================================================================*/
#define WDGIF_MODULE_ID                     43  /* AUTOSAR module ID */
#define WDGIF_VENDOR_ID                     0x5C  /* Vendor ID */

/*============================================================================
 *  DEVELOPMENT ERROR CODES
 *===========================================================================*/
#define WDGIF_E_DRIVER_UNINIT               0x01  /* Driver uninitialized */
#define WDGIF_E_PARAM_DEVICE                0x02  /* Invalid device index */
#define WDGIF_E_PARAM_MODE                  0x03  /* Invalid mode */
#define WDGIF_E_INV_POINTER                 0x04  /* Invalid pointer */

/*============================================================================
 *  API SERVICE IDs
 *===========================================================================*/
#define WDGIF_SID_INIT                      0x01
#define WDGIF_SID_DEINIT                    0x02
#define WDGIF_SID_SETMODE                   0x03
#define WDGIF_SID_TRIGGER                   0x04
#define WDGIF_SID_GETVERSIONINFO            0x05
#define WDGIF_SID_SETTRIGGERCONDITION       0x06

/*============================================================================
 *  DATA TYPES
 *===========================================================================*/

/** @brief WdgIf device index type */
typedef uint8 WdgIf_DeviceType;

/** @brief WdgIf mode type */
typedef enum {
    WDGIF_OFF_MODE = 0,      /* Watchdog disabled */
    WDGIF_SLOW_MODE,         /* Slow trigger mode */
    WDGIF_FAST_MODE          /* Fast trigger mode */
} WdgIf_ModeType;

/** @brief WdgIf status type */
typedef enum {
    WDGIF_UNINIT = 0,        /* Driver uninitialized */
    WDGIF_IDLE,              /* Driver initialized, idle */
    WDGIF_BUSY               /* Driver busy */
} WdgIf_StatusType;

/** @brief WdgIf trigger condition (timeout in ms) */
typedef uint16 WdgIf_TimeoutType;

/** @brief Device configuration structure */
typedef struct {
    WdgIf_DeviceType DeviceIndex;
    uint8 WdgDriverRef;      /* Reference to underlying Wdg driver */
} WdgIf_DeviceConfigType;

/** @brief Module configuration structure */
typedef struct {
    const WdgIf_DeviceConfigType* DeviceConfig;
    uint8 DeviceCount;
} WdgIf_ConfigType;

/*============================================================================
 *  FUNCTION PROTOTYPES
 *===========================================================================*/

/**
 * @brief Initialize WdgIf module
 * @param ConfigPtr Pointer to configuration structure
 */
void WdgIf_Init(const WdgIf_ConfigType* ConfigPtr);

/**
 * @brief Deinitialize WdgIf module
 */
void WdgIf_DeInit(void);

/**
 * @brief Set watchdog mode for specified device
 * @param Device Index of device
 * @param WdgMode Mode to set
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_SetMode(WdgIf_DeviceType Device, WdgIf_ModeType WdgMode);

/**
 * @brief Trigger (kick) the watchdog for specified device
 * @param Device Index of device
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_Trigger(WdgIf_DeviceType Device);

/**
 * @brief Get version information
 * @param VersionInfo Pointer to version info structure
 */
#if (WDGIF_VERSION_INFO_API == STD_ON)
void WdgIf_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif

/**
 * @brief Set trigger condition (timeout) for watchdog
 * @param Device Index of device
 * @param Timeout Timeout value in milliseconds
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType WdgIf_SetTriggerCondition(WdgIf_DeviceType Device, 
                                          WdgIf_TimeoutType Timeout);

#endif /* WDGIF_H */
