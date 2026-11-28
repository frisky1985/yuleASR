/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Yule_McalRtdContract.h
 * @brief MCAL -> NXP S32K3 RTD adaptation contract
 *
 * Machine-readable contract between the in-tree MCAL drivers and the NXP
 * Real-Time Drivers (RTD) that will replace them on the production target.
 * One record per MCAL module lists the RTD package it maps to, the number of
 * public APIs the module must provide, and the current adaptation status.
 *
 * Build switch: YULE_USE_NXP_RTD_DRIVERS (src/bsw/mcal/CMakeLists.txt).
 * The switch stays OFF while only RTD register headers are vendored
 * (third_party/nxp/s32k312/include); no behaviour changes when OFF.
 *
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef YULE_MCAL_RTD_CONTRACT_H
#define YULE_MCAL_RTD_CONTRACT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

#define YULE_MCAL_RTD_MODULE_COUNT      (20U)

/** @brief Adaptation status of a single MCAL module */
typedef enum
{
    YULE_RTD_HEADERS_ONLY = 0,  /**< Only RTD register headers vendored */
    YULE_RTD_ADAPTER_READY,     /**< In-tree adapter seam in place */
    YULE_RTD_DRIVER_READY       /**< Bound to RTD driver sources */
} Yule_McalRtdStatusType;

/** @brief Per-module RTD adaptation record */
typedef struct
{
    const char*           moduleName;     /**< MCAL module name (lower case) */
    const char*           rtdPackage;     /**< NXP RTD driver package name */
    uint16                requiredApiCount; /**< Public APIs the module exposes */
    Yule_McalRtdStatusType status;        /**< Current adaptation status */
} Yule_McalRtdModuleContractType;

/** @brief Contract table, one record per MCAL module */
extern const Yule_McalRtdModuleContractType Yule_McalRtdContract[YULE_MCAL_RTD_MODULE_COUNT];

/**
 * @brief Looks up a module contract record
 * @param moduleName Module name (case sensitive, e.g. "can")
 * @return Record pointer, or NULL_PTR when unknown
 */
const Yule_McalRtdModuleContractType* Yule_McalRtd_GetModule(const char* moduleName);

/**
 * @brief TRUE when the module is fully bound to RTD driver sources
 * @param moduleName Module name
 */
boolean Yule_McalRtd_IsDriverAvailable(const char* moduleName);

/** @brief Number of modules with status YULE_RTD_DRIVER_READY */
uint8 Yule_McalRtd_GetReadyModuleCount(void);

/** @brief Human readable name of an adaptation status */
const char* Yule_McalRtd_GetStatusString(Yule_McalRtdStatusType status);

#ifdef __cplusplus
}
#endif

#endif /* YULE_MCAL_RTD_CONTRACT_H */

/*==================[end of file]===========================================*/
