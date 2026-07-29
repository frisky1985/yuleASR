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

/**
 * @file Dem_Lcfg.h
 * @brief Diagnostic Event Manager - Link-Time Configuration Header
 * @version 1.1.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *
 * CRITICAL FIX: Link-time configuration header for Dem module
 * Contains configuration data structures and external declarations
 */

#ifndef DEM_LCFG_H
#define DEM_LCFG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Dem_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DEM_LCFG_VENDOR_ID                   (0x01U)
#define DEM_LCFG_MODULE_ID                   (0x54U)
#define DEM_LCFG_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DEM_LCFG_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DEM_LCFG_AR_RELEASE_REVISION_VERSION (0x00U)
#define DEM_LCFG_SW_MAJOR_VERSION            (0x01U)
#define DEM_LCFG_SW_MINOR_VERSION            (0x01U)
#define DEM_LCFG_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    CONFIGURATION VARIANT
==================================================================================================*/
/* Link-time configuration variant */
#define DEM_CONFIGURATION_VARIANT_POSTBUILD  (0x00U)
#define DEM_CONFIGURATION_VARIANT_LINKTIME   (0x01U)
#define DEM_CONFIGURATION_VARIANT_PRECOMPILE (0x02U)

#define DEM_CONFIGURATION_VARIANT DEM_CONFIGURATION_VARIANT_LINKTIME

/*==================================================================================================
*                                    EXTERNAL CONFIGURATION
==================================================================================================*/
#define DEM_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/* External reference to the main DEM configuration */
extern CONST(Dem_ConfigType, DEM_CONST) Dem_Config;

/* Event parameter configuration */
extern CONST(Dem_EventParameterType, DEM_CONST) Dem_EventParameters[DEM_NUM_EVENTS];

/* DTC parameter configuration */
extern CONST(Dem_DtcParameterType, DEM_CONST) Dem_DtcParameters[DEM_NUM_DTCS];

/* Freeze frame record configuration */
extern CONST(Dem_FreezeFrameRecordType, DEM_CONST) Dem_FreezeFrameRecords[DEM_NUM_FREEZE_FRAME_RECORDS];

/* Extended data record configuration */
extern CONST(Dem_ExtendedDataRecordType, DEM_CONST) Dem_ExtendedDataRecords[DEM_NUM_EXTENDED_DATA_RECORDS];

/* Indicator configuration */
extern CONST(Dem_IndicatorType, DEM_CONST) Dem_Indicators[DEM_NUM_INDICATORS];

#define DEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    CONFIGURATION CHECKS
==================================================================================================*/
/* Ensure configuration consistency */
#if (DEM_NUM_EVENTS > 65535)
#error "DEM_NUM_EVENTS exceeds maximum value"
#endif

#if (DEM_NUM_DTCS > 255)
#error "DEM_NUM_DTCS exceeds maximum value"
#endif

#if (DEM_NUM_FREEZE_FRAME_RECORDS < 1)
#error "At least one freeze frame record must be configured"
#endif

#if (DEM_NUM_EXTENDED_DATA_RECORDS < 1)
#error "At least one extended data record must be configured"
#endif

#endif /* DEM_LCFG_H */
