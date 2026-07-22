/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Cdd.c
 * @brief   Complex Device Driver — Coordinator Implementation
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Top-level CDD coordinator that initializes, ticks, and de-initializes
 *   all registered CDD sub-modules:
 *     - Cdd_Hsm       (Hardware Security Module)
 *     - Cdd_RamEcc    (RAM ECC error handler)
 *     - Cdd_Lockstep  (Lockstep core monitor)
 *     - Cdd_Safety    (Safety integrator)
 *     - Cdd_Boot      (Boot-time platform init)
 *
 * @implements AUTOSAR_SWS_CDD
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Cdd.h"
#include "Cdd_Hsm.h"
#include "Cdd_RamEcc.h"
#include "Cdd_Lockstep.h"
#include "Cdd_Safety.h"
#include "Cdd_Boot.h"

#if (CDD_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                         LOCAL MACROS
 *==================================================================================================*/
#ifndef CDD_DEV_ERROR_DETECT
#define CDD_DEV_ERROR_DETECT                    STD_ON
#endif

#define CDD_SID_INIT                            0x10U
#define CDD_SID_DEINIT                          0x11U
#define CDD_SID_MAINFUNCTION                    0x12U
#define CDD_SID_GETVERSIONINFO                  0x13U

/*==================================================================================================
 *                                         MODULE VARIABLES
 *==================================================================================================*/
#define CDD_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief CDD layer initialization status */
STATIC boolean Cdd_Initialized = FALSE;

#define CDD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         MODULE CONFIGURATION
 *==================================================================================================*/
#define CDD_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Default CDD sub-module configuration instances */
STATIC const Cdd_Hsm_ConfigType Cdd_Hsm_DefaultCfg = {
    .enableAes       = TRUE,
    .enableEcc       = TRUE,
    .enableSha       = TRUE,
    .enableTrng      = TRUE,
    .enableKeyStore  = TRUE,
    .timeoutUs       = 10000U
};

STATIC const Cdd_RamEcc_ConfigType Cdd_RamEcc_DefaultCfg = {
    .singleBitPolicy     = CDD_RAMECC_POLICY_CORRECT,
    .doubleBitPolicy     = CDD_RAMECC_POLICY_SAFE_STATE,
    .enableInterrupt     = TRUE,
    .enableLogging       = TRUE,
    .singleBitThreshold  = CDD_RAMECC_SINGLE_BIT_THRESHOLD
};

STATIC const Cdd_Lockstep_ConfigType Cdd_Lockstep_DefaultCfg = {
    .mode                = CDD_LOCKSTEP_MODE_ENABLED,
    .enableBist          = TRUE,
    .enableEout          = TRUE,
    .enableFccuReporting = TRUE,
    .bistTimeoutUs       = CDD_LOCKSTEP_BIST_TIMEOUT_US,
    .checkIntervalMs     = 100U
};

STATIC const Cdd_Safety_ConfigType Cdd_Safety_DefaultCfg = {
    .enableFccu         = TRUE,
    .enableDemReporting = TRUE,
    .enableCrcIntegrity = TRUE,
    .fccuTimeoutUs      = 1000U,
    .checkIntervalMs    = 100U
};

#define CDD_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         FUNCTION IMPLEMENTATIONS
 *==================================================================================================*/
#define CDD_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize all CDD sub-modules.
 * @details
 *   Init order:
 *   1. Cdd_Safety    — FCCU must be configured first for fault reporting
 *   2. Cdd_Lockstep  — Lockstep needs FCCU ready
 *   3. Cdd_RamEcc    — ECC handler relies on safety framework
 *   4. Cdd_Hsm       — HSM init requires lockstep stable
 *   5. Cdd_Boot      — Boot-time init (last, marks completion)
 */
Std_ReturnType Cdd_Init(void)
{
    Std_ReturnType result;
    Std_ReturnType overallResult = E_OK;

    /* Safety integrator first — FCCU must be online for fault reporting */
    result = Cdd_Safety_Init(&Cdd_Safety_DefaultCfg);
    if (result != E_OK)
    {
#if (CDD_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CDD_MODULE_ID_SAFETY, 0U, CDD_SID_INIT, 1U);
#endif
        overallResult = E_NOT_OK;
    }

    /* Lockstep monitor — needs FCCU ready */
    result = Cdd_Lockstep_Init(&Cdd_Lockstep_DefaultCfg);
    if (result != E_OK)
    {
#if (CDD_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CDD_MODULE_ID_LOCKSTEP, 0U, CDD_SID_INIT, 1U);
#endif
        overallResult = E_NOT_OK;
    }

    /* RAM ECC handler — needs safety framework */
    result = Cdd_RamEcc_Init(&Cdd_RamEcc_DefaultCfg);
    if (result != E_OK)
    {
#if (CDD_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CDD_MODULE_ID_RAMECC, 0U, CDD_SID_INIT, 1U);
#endif
        overallResult = E_NOT_OK;
    }

    /* HSM driver */
    result = Cdd_Hsm_Init(&Cdd_Hsm_DefaultCfg);
    if (result != E_OK)
    {
#if (CDD_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(CDD_MODULE_ID_HSM, 0U, CDD_SID_INIT, 1U);
#endif
        overallResult = E_NOT_OK;
    }

    if (overallResult == E_OK)
    {
        Cdd_Initialized = TRUE;
    }

    return overallResult;
}

/**
 * @brief   De-initialize all CDD sub-modules (reverse order).
 */
void Cdd_DeInit(void)
{
    if (Cdd_Initialized == FALSE)
    {
        return;
    }

    Cdd_Hsm_DeInit();
    Cdd_RamEcc_DeInit();
    Cdd_Lockstep_DeInit();
    Cdd_Safety_DeInit();

    Cdd_Initialized = FALSE;
}

/**
 * @brief   CDD MainFunction — tick all sub-modules.
 */
void Cdd_MainFunction(void)
{
    if (Cdd_Initialized == FALSE)
    {
        return;
    }

    Cdd_Hsm_MainFunction();
    Cdd_RamEcc_MainFunction();
    Cdd_Lockstep_MainFunction();
    Cdd_Safety_MainFunction();
}

/**
 * @brief   Get CDD software version information.
 */
void Cdd_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_SAFETY, 0U, CDD_SID_GETVERSIONINFO, 1U);
        return;
    }
#endif

    if (versioninfo != NULL_PTR)
    {
        versioninfo->vendorID            = CDD_VENDOR_ID;
        versioninfo->moduleID            = 0U;  /* not module-specific */
        versioninfo->sw_major_version    = CDD_SW_MAJOR_VERSION;
        versioninfo->sw_minor_version    = CDD_SW_MINOR_VERSION;
        versioninfo->sw_patch_version    = CDD_SW_PATCH_VERSION;
    }
}

#define CDD_STOP_SEC_CODE
#include "Cdd_MemMap.h"

/** @} */

/*==================================================================================================
*                                         END OF FILE
*==================================================================================================*/
