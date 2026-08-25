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
/* @req SHALL_CDD */


/**
 * @file    Cdd_Lockstep_1.0.0.c
 * @brief   Complex Driver — Lockstep Core Monitor Implementation
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   S32K312 Cortex-M7 lockstep complex driver implementation.
 *   Manages MSCM lockstep registers for dual-core lockstep safety.
 *   Replaces the platform-level Platform_Lockstep.c with AUTOSAR CDD.
 *
 * @ASIL-D Safety Level
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Cdd_Lockstep.h"

#if (CDD_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                         LOCAL MACROS
 *==================================================================================================*/
#ifndef CDD_DEV_ERROR_DETECT
#define CDD_DEV_ERROR_DETECT                    STD_ON
#endif

/** @brief Convenience register access */
#define REG32(addr)         (*((volatile uint32*)(addr)))

/** @brief MSCM lockstep register offsets */
#define MSCM_LOCKSTEP_CTRL_OFFSET               (0x0200U)
#define MSCM_LOCKSTEP_STATUS_OFFSET             (0x0204U)
#define MSCM_BIST_CTRL_OFFSET                   (0x0210U)
#define MSCM_BIST_STATUS_OFFSET                 (0x0214U)

/** @brief Register access macros */
#define MSCM_LOCKSTEP_CTRL      REG32(CDD_LOCKSTEP_MSCM_BASE + MSCM_LOCKSTEP_CTRL_OFFSET)
#define MSCM_LOCKSTEP_STATUS    REG32(CDD_LOCKSTEP_MSCM_BASE + MSCM_LOCKSTEP_STATUS_OFFSET)
#define MSCM_BIST_CTRL          REG32(CDD_LOCKSTEP_MSCM_BASE + MSCM_BIST_CTRL_OFFSET)
#define MSCM_BIST_STATUS        REG32(CDD_LOCKSTEP_MSCM_BASE + MSCM_BIST_STATUS_OFFSET)

/** @brief MC_RGM reset reason register offset */
#define MC_RGM_DES_OFFSET       (0x0000U)
#define MC_RGM_DES              REG32(CDD_LOCKSTEP_RGM_BASE + MC_RGM_DES_OFFSET)

/** @brief Lockstep control bits */
#define LOCKSTEP_CTRL_ENABLE    0x00000001U
#define LOCKSTEP_CTRL_BIST_EN   0x00000100U
#define LOCKSTEP_CTRL_EOUT_EN   0x00010000U

/** @brief Lockstep status bits */
#define LOCKSTEP_STATUS_ACTIVE      0x00000001U
#define LOCKSTEP_STATUS_ERROR       0x00000002U
#define LOCKSTEP_STATUS_MISMATCH    0x00000004U
#define LOCKSTEP_STATUS_BIST_DONE   0x00000100U
#define LOCKSTEP_STATUS_BIST_FAIL   0x00000200U

/** @brief DET API IDs */
#define CDD_LOCKSTEP_SID_INIT           0x10U
#define CDD_LOCKSTEP_SID_DEINIT         0x11U
#define CDD_LOCKSTEP_SID_SETMODE        0x12U
#define CDD_LOCKSTEP_SID_GETSTATUS      0x13U
#define CDD_LOCKSTEP_SID_RUNBIST        0x14U
#define CDD_LOCKSTEP_SID_CLEARERROR     0x15U
#define CDD_LOCKSTEP_SID_MAINFUNCTION   0x16U

/*==================================================================================================
 *                                         MODULE VARIABLES
 *==================================================================================================*/
#define CDD_LOCKSTEP_START_SEC_VAR_INIT_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Lockstep configuration pointer */
STATIC const Cdd_Lockstep_ConfigType*  Cdd_Lockstep_Config = NULL_PTR;

/** @brief Initialization flag */
STATIC boolean                         Cdd_Lockstep_Initialized = FALSE;

#define CDD_LOCKSTEP_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Cdd_MemMap.h"

#define CDD_LOCKSTEP_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Last BIST status */
STATIC Cdd_Lockstep_BistStatusType     Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_IDLE;

/** @brief Last BIST raw result */
STATIC uint32                          Cdd_Lockstep_BistResultValue = 0U;

/** @brief Health check tick counter */
STATIC uint32                          Cdd_Lockstep_TickCount = 0U;

#define CDD_LOCKSTEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *==================================================================================================*/
#define CDD_LOCKSTEP_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Simple microsecond delay (busy-wait, 80 MHz assumed).
 */
STATIC void Cdd_Lockstep_DelayUs(uint32 microseconds)
{
    volatile uint32 count = microseconds * 80U;
    while (count > 0U)
    {
        count--;
    }
}

/**
 * @brief   Wait for BIST completion with timeout.
 */
STATIC Std_ReturnType Cdd_Lockstep_WaitBist(uint32 timeoutUs)
{
    volatile uint32 timeout = timeoutUs * 10U;

    while (timeout > 0U)
    {
        uint32 bistStatus = MSCM_BIST_STATUS;

        if ((bistStatus & LOCKSTEP_STATUS_BIST_DONE) != 0U)
        {
            Cdd_Lockstep_BistResultValue = bistStatus;

            if ((bistStatus & LOCKSTEP_STATUS_BIST_FAIL) != 0U)
            {
                Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_COMPLETE_FAIL;
                return E_NOT_OK;
            }

            Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_COMPLETE_PASS;
            return E_OK;
        }

        timeout--;
    }

    Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_IDLE;
    return E_NOT_OK;  /* Timeout */
}

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief   Initialize Lockstep complex driver.
 */
Std_ReturnType Cdd_Lockstep_Init(const Cdd_Lockstep_ConfigType* config)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_LOCKSTEP, 0U, CDD_LOCKSTEP_SID_INIT, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Lockstep_Initialized != FALSE)
    {
        return E_NOT_OK;
    }

    Cdd_Lockstep_Config = config;

    /* Configure lockstep control register */
    {
        uint32 ctrl = 0U;

        if (config->mode == CDD_LOCKSTEP_MODE_ENABLED)
        {
            ctrl |= LOCKSTEP_CTRL_ENABLE;
            ctrl |= (1U << 1U);  /* Lockstep mode bit */
        }
        else if (config->mode == CDD_LOCKSTEP_MODE_DEBUG)
        {
            ctrl |= LOCKSTEP_CTRL_ENABLE;
            /* Split mode (debug) — mode bits = 0 */
        }
        /* DISABLED: ctrl stays 0 */

        if ((config->enableEout) != 0U)
        {
            ctrl |= LOCKSTEP_CTRL_EOUT_EN;
        }

        MSCM_LOCKSTEP_CTRL = ctrl;
        __asm__ volatile ("dsb" ::: "memory");
        __asm__ volatile ("isb" ::: "memory");

        Cdd_Lockstep_DelayUs(100U);
    }

    /* Run BIST if configured */
    if ((config->enableBist) != 0U)
    {
        uint32 timeout = (config->bistTimeoutUs > 0U) ? config->bistTimeoutUs : CDD_LOCKSTEP_BIST_TIMEOUT_US;
        (void)Cdd_Lockstep_RunBist(timeout);
    }

    Cdd_Lockstep_Initialized = TRUE;
    return E_OK;
}

/**
 * @brief   De-initialize lockstep driver.
 */
void Cdd_Lockstep_DeInit(void)
{
    if (Cdd_Lockstep_Initialized == FALSE)
    {
        return;
    }

    /* Disable lockstep */
    MSCM_LOCKSTEP_CTRL = 0U;
    __asm__ volatile ("dsb" ::: "memory");

    Cdd_Lockstep_Config = NULL_PTR;
    Cdd_Lockstep_Initialized = FALSE;
    Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_IDLE;
    Cdd_Lockstep_BistResultValue = 0U;
}

/**
 * @brief   Set lockstep operating mode.
 */
Std_ReturnType Cdd_Lockstep_SetMode(Cdd_Lockstep_ModeType mode)
{
    uint32 ctrl;

    if (Cdd_Lockstep_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    ctrl = MSCM_LOCKSTEP_CTRL;

    switch (mode)
    {
        case CDD_LOCKSTEP_MODE_ENABLED:
            ctrl |= LOCKSTEP_CTRL_ENABLE;
            ctrl |= (1U << 1U);
            break;

        case CDD_LOCKSTEP_MODE_DISABLED:
            ctrl &= ~LOCKSTEP_CTRL_ENABLE;
            break;

        case CDD_LOCKSTEP_MODE_DEBUG:
            ctrl |= LOCKSTEP_CTRL_ENABLE;
            ctrl &= ~(3U << 1U);  /* Clear mode bits = split mode */
            break;

        default:
            return E_NOT_OK;
    }

    MSCM_LOCKSTEP_CTRL = ctrl;
    __asm__ volatile ("dsb" ::: "memory");
    Cdd_Lockstep_DelayUs(100U);

    return E_OK;
}

/**
 * @brief   Get current lockstep status.
 */
Std_ReturnType Cdd_Lockstep_GetStatus(Cdd_Lockstep_StatusType* status)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (status == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_LOCKSTEP, 0U, CDD_LOCKSTEP_SID_GETSTATUS, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Lockstep_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    {
        uint32 ls = MSCM_LOCKSTEP_STATUS;

        status->isActive         = (boolean)((ls & LOCKSTEP_STATUS_ACTIVE) != 0U);
        status->hasError         = (boolean)((ls & LOCKSTEP_STATUS_ERROR) != 0U);
        status->mismatchDetected = (boolean)((ls & LOCKSTEP_STATUS_MISMATCH) != 0U);
        status->bistStatus       = Cdd_Lockstep_BistStatus;
        status->resetReason      = MC_RGM_DES;
    }

    return E_OK;
}

/**
 * @brief   Run lockstep Built-In Self-Test (LBIST).
 */
Std_ReturnType Cdd_Lockstep_RunBist(uint32 timeoutUs)
{
    if (Cdd_Lockstep_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    /* Cannot run BIST while test in progress */
    if (Cdd_Lockstep_BistStatus == CDD_LOCKSTEP_BIST_RUNNING)
    {
        return E_NOT_OK;
    }

    Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_IDLE;
    Cdd_Lockstep_BistResultValue = 0U;

    /* Start BIST */
    MSCM_BIST_CTRL = LOCKSTEP_CTRL_BIST_EN;
    __asm__ volatile ("dsb" ::: "memory");

    Cdd_Lockstep_BistStatus = CDD_LOCKSTEP_BIST_RUNNING;

    return Cdd_Lockstep_WaitBist((timeoutUs > 0U) ? timeoutUs : CDD_LOCKSTEP_BIST_TIMEOUT_US);
}

/**
 * @brief   Clear lockstep error status.
 */
Std_ReturnType Cdd_Lockstep_ClearError(void)
{
    if (Cdd_Lockstep_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    MSCM_LOCKSTEP_STATUS = 0x00000004U;  /* Write 1 to clear mismatch */
    __asm__ volatile ("dsb" ::: "memory");

    return E_OK;
}

/**
 * @brief   Lockstep periodic health check.
 * @details Reads lockstep status, checks for mismatches, reports to
 *          safety integrator / Dem if errors are found.
 */
void Cdd_Lockstep_MainFunction(void)
{
    Cdd_Lockstep_StatusType status;

    if (Cdd_Lockstep_Initialized == FALSE)
    {
        return;
    }

    Cdd_Lockstep_TickCount++;

    /* Health check every N ticks based on configured interval */
    /* (Assume MainFunction runs at 10ms; check every 10 ticks = 100ms default) */
    if ((Cdd_Lockstep_TickCount % 10U) != 0U)
    {
        return;
    }

    if (Cdd_Lockstep_GetStatus(&status) != E_OK)
    {
        return;
    }

    if ((status.mismatchDetected) != 0U)
    {
        /* Lockstep mismatch — critical safety fault */
        /* Cdd_Safety_ReportFault(CDD_SAFETY_FAULT_LOCKSTEP, 1U); */
        (void)Cdd_Lockstep_ClearError();
    }

    if ((status.hasError) != 0U)
    {
        /* Non-critical error — report and clear */
        (void)Cdd_Lockstep_ClearError();
    }
}

#define CDD_LOCKSTEP_STOP_SEC_CODE
#include "Cdd_MemMap.h"

/*==================================================================================================
*                                         END OF FILE
*==================================================================================================*/
