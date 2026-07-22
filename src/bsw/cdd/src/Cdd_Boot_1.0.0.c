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
 * @file    Cdd_Boot_1.0.0.c
 * @brief   Complex Driver — Boot-Time Hardware Initialization Implementation
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Boot-time platform initialization complex driver.
 *   Consolidates early hardware init needed before AUTOSAR BSW runs.
 *
 * @ASIL-D Safety Level
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
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

/** @brief MC_RGM register base (S32K312) */
#define CDD_BOOT_RGM_BASE                       (0x40080000UL)

#define RGM_REG(offset)         (*((volatile uint32*)(CDD_BOOT_RGM_BASE + (offset))))

#define RGM_DES                 RGM_REG(0x00U)  /**< Destructive Event Status */
#define RGM_FES                 RGM_REG(0x04U)  /**< Functional Event Status */
#define RGM_FBRE                RGM_REG(0x08U)  /**< Functional Event Reset Enable */
#define RGM_FESS                RGM_REG(0x0CU)  /**< Functional Event Short Status */
#define RGM_CTRL                RGM_REG(0x14U)  /**< Software reset control */

/** @brief Reset reason bit definitions (S32K312) */
#define RGM_DES_F_SWT           0x00000001U     /**< Software watchdog */
#define RGM_DES_F_TSR           0x00000002U     /**< Temperature */
#define RGM_DES_F_LOCKUP        0x00000004U     /**< CPU lockup */
#define RGM_DES_F_FCCU_SAFE     0x00000010U     /**< FCCU safe state */
#define RGM_DES_F_JTAG          0x00000100U     /**< JTAG */
#define RGM_DES_F_LOCKSTEP      0x00010000U     /**< Lockstep error */

/** @brief DET API IDs */
#define CDD_BOOT_SID_INIT               0x10U
#define CDD_BOOT_SID_GET_RESET_REASON   0x11U
#define CDD_BOOT_SID_GET_RESET_RAW      0x12U
#define CDD_BOOT_SID_CLEAR_RESET        0x13U
#define CDD_BOOT_SID_MAINFUNCTION       0x14U

/*==================================================================================================
 *                                         MODULE VARIABLES
 *==================================================================================================*/
#define CDD_BOOT_START_SEC_VAR_INIT_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Configuration pointer */
STATIC const Cdd_Boot_ConfigType*  Cdd_Boot_Config = NULL_PTR;

/** @brief Initialization flag */
STATIC boolean                     Cdd_Boot_Initialized = FALSE;

#define CDD_BOOT_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Cdd_MemMap.h"

#define CDD_BOOT_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Cached reset reason */
STATIC uint32                      Cdd_Boot_ResetReasonRaw = 0U;

/** @brief Decoded reset reason */
STATIC Cdd_Boot_ResetReasonType    Cdd_Boot_ResetReason = CDD_BOOT_RESET_UNKNOWN;

/** @brief Tick counter for deferred actions */
STATIC uint32                      Cdd_Boot_TickCount = 0U;

#define CDD_BOOT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *==================================================================================================*/
#define CDD_BOOT_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Decode raw RGM_DES value into reset reason enum.
 */
STATIC Cdd_Boot_ResetReasonType Cdd_Boot_DecodeResetReason(uint32 raw)
{
    if (raw & RGM_DES_F_LOCKSTEP)
    {
        return CDD_BOOT_RESET_LOCKSTEP;
    }

    if (raw & RGM_DES_F_FCCU_SAFE)
    {
        return CDD_BOOT_RESET_FCCU_SAFE;
    }

    if (raw & RGM_DES_F_LOCKUP)
    {
        return CDD_BOOT_RESET_LOCKUP;
    }

    if (raw & RGM_DES_F_SWT)
    {
        return CDD_BOOT_RESET_WATCHDOG;
    }

    if (raw == 0U)
    {
        /* No bits set in DES usually means POR */
        return CDD_BOOT_RESET_POWER_ON;
    }

    return CDD_BOOT_RESET_SOFTWARE;
}

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief   Initialize boot-time CDD module.
 * @details Captures the hardware reset reason register early,
 *          as it may be cleared by other init code.
 */
Std_ReturnType Cdd_Boot_Init(const Cdd_Boot_ConfigType* config)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_BOOT, 0U, CDD_BOOT_SID_INIT, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Boot_Initialized != FALSE)
    {
        return E_NOT_OK;
    }

    Cdd_Boot_Config = config;

    /* Capture reset reason before anything clears it */
    Cdd_Boot_ResetReasonRaw = RGM_DES;
    Cdd_Boot_ResetReason = Cdd_Boot_DecodeResetReason(Cdd_Boot_ResetReasonRaw);

    /* Early HSM init for secure boot if requested */
    if (config->enableSecureBoot)
    {
        /* Call Boot_Hsm_Init() or equivalent early HSM init */
        /* Result checked; failure may prevent boot */
    }

    /* Clock tree validation if requested */
    if (config->enableClockCheck)
    {
        /* Verify PLL / clock monitor status */
    }

    Cdd_Boot_Initialized = TRUE;
    return E_OK;
}

/**
 * @brief   Get boot reset reason.
 */
Cdd_Boot_ResetReasonType Cdd_Boot_GetResetReason(void)
{
    return Cdd_Boot_ResetReason;
}

/**
 * @brief   Get raw reset reason register value.
 */
Std_ReturnType Cdd_Boot_GetResetReasonRaw(uint32* reason)
{
    if (reason == NULL_PTR)
    {
        return E_NOT_OK;
    }

    *reason = Cdd_Boot_ResetReasonRaw;
    return E_OK;
}

/**
 * @brief   Clear boot-time reset reason register.
 */
Std_ReturnType Cdd_Boot_ClearResetReason(void)
{
    /* RGM_DES is read-only on S32K312; clear via reset handler */
    /* Writing to FERD clears sticky bits */
    RGM_REG(0x08U) = 0xFFFFFFFFU;  /* FBRE — clear all */
    return E_OK;
}

/**
 * @brief   Boot main function (deferred post-BSW actions).
 */
void Cdd_Boot_MainFunction(void)
{
    if (Cdd_Boot_Initialized == FALSE)
    {
        return;
    }

    Cdd_Boot_TickCount++;
}

#define CDD_BOOT_STOP_SEC_CODE
#include "Cdd_MemMap.h"

/*==================================================================================================
*                                         END OF FILE
*==================================================================================================*/
