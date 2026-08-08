/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
* Dependencies         : AUTOSAR 4.7
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Cdd.h
 * @brief   Complex Device Driver (CDD) Layer — Unified Header
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   CDD layer according to AUTOSAR SWS_CDD:
 *   - Cdd_Hsm       — Hardware Security Module complex driver
 *   - Cdd_RamEcc    — RAM ECC error handler (single/double-bit)
 *   - Cdd_Lockstep  — Lockstep core monitor & BIST
 *   - Cdd_Safety    — Safety integrator (FCCU, WdgM, Dem integration)
 *   - Cdd_Boot      — Boot-time hardware init (HSM, RGM, safety)
 *   - Cdd_Fvm       — Flash Virtual Memory (multi-bank, failover, XMEN-style)
 *
 *   All CDD modules use Cdd_MemMap.h for memory partitioning.
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD
 */

#ifndef CDD_H
#define CDD_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_VENDOR_ID                       43U
#define CDD_AR_RELEASE_MAJOR_VERSION        4U
#define CDD_AR_RELEASE_MINOR_VERSION        7U
#define CDD_AR_RELEASE_REVISION_VERSION     0U
#define CDD_SW_MAJOR_VERSION                1U
#define CDD_SW_MINOR_VERSION                0U
#define CDD_SW_PATCH_VERSION                0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                         MODULE IDS
 *==================================================================================================*/
#define CDD_MODULE_ID_HSM                   0x80U   /**< Cdd_Hsm module ID */
#define CDD_MODULE_ID_RAMECC                0x81U   /**< Cdd_RamEcc module ID */
#define CDD_MODULE_ID_LOCKSTEP              0x82U   /**< Cdd_Lockstep module ID */
#define CDD_MODULE_ID_SAFETY                0x83U   /**< Cdd_Safety module ID */
#define CDD_MODULE_ID_BOOT                  0x84U   /**< Cdd_Boot module ID */
#define CDD_MODULE_ID_FVM                   0x85U   /**< Cdd_Fvm module ID (Flash Virtual Memory) */

/*==================================================================================================
 *                                         CDD API FUNCTIONS
 *==================================================================================================*/
#define CDD_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   CDD Layer initialization entry
 * @details Calls Init on all registered CDD sub-modules.
 *          Called once from EcuM_Init after MCAL BSW init.
 * @param   void
 * @return  E_OK  — All CDD modules initialized
 *          E_NOT_OK — At least one module failed
 */
extern Std_ReturnType Cdd_Init(void);

/**
 * @brief   CDD Layer de-initialization
 */
extern void Cdd_DeInit(void);

/**
 * @brief   CDD MainFunction — periodic tick for all CDD sub-modules
 * @details Schedules periodic operations for:
 *          - Cdd_Lockstep: lockstep health check
 *          - Cdd_RamEcc:   ECC error threshold monitoring
 *          - Cdd_Safety:   safety integrity check
 */
extern void Cdd_MainFunction(void);

/**
 * @brief   Get CDD software version information
 * @param   versioninfo  [out] Pointer to version info structure
 */
extern void Cdd_GetVersionInfo(Std_VersionInfoType* versioninfo);

#define CDD_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_H */
