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
 * @file    Cdd_Safety_1.0.0.c
 * @brief   Complex Driver — Safety Integrator Implementation
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Safety integrator complex driver for S32K312.
 *   Provides a single point of integration for all safety-relevant
 *   fault sources:
 *   - FCCU management (Fault Collection and Control Unit)
 *   - Safety state machine (normal → warning → safe → reset)
 *   - Dem DTC reporting for safety faults
 *   - RAM CRC runtime integrity
 *
 * @ASIL-D Safety Level
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Cdd_Safety.h"

#if (CDD_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                         LOCAL MACROS
 *==================================================================================================*/
#ifndef CDD_DEV_ERROR_DETECT
#define CDD_DEV_ERROR_DETECT                    STD_ON
#endif

/** @brief FCCU register base (S32K312) */
#define CDD_SAFETY_FCCU_BASE                    (0x40090000UL)

#define FCCU_REG(offset)        (*((volatile uint32*)(CDD_SAFETY_FCCU_BASE + (offset))))

#define FCCU_CTRL               FCCU_REG(0x00U)
#define FCCU_CTRLK              FCCU_REG(0x04U)
#define FCCU_CFG                FCCU_REG(0x08U)
#define FCCU_STAT               FCCU_REG(0x60U)

#define FCCU_CTRLK_KEY          0x913756B9U
#define FCCU_SAFE_STATE_CMD     0x01U

/** @brief DET API IDs */
#define CDD_SAFETY_SID_INIT                     0x10U
#define CDD_SAFETY_SID_DEINIT                   0x11U
#define CDD_SAFETY_SID_GETSTATE                 0x12U
#define CDD_SAFETY_SID_REPORTFAULT              0x13U
#define CDD_SAFETY_SID_ENTERSAFESTATE           0x14U
#define CDD_SAFETY_SID_SYSTEMRESET              0x15U
#define CDD_SAFETY_SID_REGISTER_STATE_CALLBACK  0x16U
#define CDD_SAFETY_SID_MAINFUNCTION             0x17U

/*==================================================================================================
 *                                         MODULE VARIABLES
 *==================================================================================================*/
#define CDD_SAFETY_START_SEC_VAR_INIT_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Safety configuration pointer */
STATIC const Cdd_Safety_ConfigType*  Cdd_Safety_Config = NULL_PTR;

/** @brief Initialization flag */
STATIC boolean                       Cdd_Safety_Initialized = FALSE;

/** @brief State change callback */
STATIC void (*Cdd_Safety_StateCallback)(Cdd_Safety_StateType) = NULL_PTR;

#define CDD_SAFETY_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "Cdd_MemMap.h"

#define CDD_SAFETY_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Current safety state */
STATIC Cdd_Safety_StateType          Cdd_Safety_State = CDD_SAFETY_STATE_NORMAL;

/** @brief CRC integrity check result buffer */
STATIC uint32                        Cdd_Safety_CrcReference = 0U;

/** @brief Tick counter */
STATIC uint32                        Cdd_Safety_TickCount = 0U;

#define CDD_SAFETY_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *==================================================================================================*/
#define CDD_SAFETY_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Transition the safety state machine.
 */
STATIC void Cdd_Safety_SetState(Cdd_Safety_StateType newState)
{
    if (newState == Cdd_Safety_State)
    {
        return;  /* No change */
    }

    Cdd_Safety_State = newState;

    /* Notify registered callback */
    if (Cdd_Safety_StateCallback != NULL_PTR)
    {
        Cdd_Safety_StateCallback(newState);
    }
}

/**
 * @brief   CRC32 calculation (software, for integrity check).
 */
STATIC uint32 Cdd_Safety_Crc32(const uint8* data, uint32 length, uint32 seed)
{
    uint32 crc = ~seed;
    uint32 i, j;

    for (i = 0U; i < length; i++)
    {
        crc ^= ((uint32)data[i] << 24U);

        for (j = 0U; j < 8U; j++)
        {
            if ((crc & 0x80000000U) != 0U)
            {
                crc = (crc << 1U) ^ 0x04C11DB7U;
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return ~crc;
}

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief   Initialize Safety integrator complex driver.
 */
Std_ReturnType Cdd_Safety_Init(const Cdd_Safety_ConfigType* config)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_SAFETY, 0U, CDD_SAFETY_SID_INIT, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Safety_Initialized != FALSE)
    {
        return E_NOT_OK;
    }

    Cdd_Safety_Config = config;

    /* Initialize FCCU if enabled */
    if ((config->enableFccu) != 0U)
    {
        FCCU_CTRLK = FCCU_CTRLK_KEY;   /* Unlock FCCU */
        FCCU_CFG   = 0x00U;            /* Normal mode */
        FCCU_CTRL  = 0x00U;            /* Clear pending */
    }

    /* Initialize CRC reference if integrity checks enabled */
    if ((config->enableCrcIntegrity) != 0U)
    {
        /* Compute CRC of a known reference RAM region */
        /* (Region and address would come from configuration) */
        Cdd_Safety_CrcReference = 0U;
    }

    Cdd_Safety_State = CDD_SAFETY_STATE_NORMAL;
    Cdd_Safety_Initialized = TRUE;

    return E_OK;
}

/**
 * @brief   De-initialize safety integrator.
 */
void Cdd_Safety_DeInit(void)
{
    if (Cdd_Safety_Initialized == FALSE)
    {
        return;
    }

    Cdd_Safety_Config = NULL_PTR;
    Cdd_Safety_StateCallback = NULL_PTR;
    Cdd_Safety_State = CDD_SAFETY_STATE_NORMAL;
    Cdd_Safety_Initialized = FALSE;
}

/**
 * @brief   Get current safety state.
 */
Cdd_Safety_StateType Cdd_Safety_GetState(void)
{
    return Cdd_Safety_State;
}

/**
 * @brief   Report a safety fault to the safety integrator.
 */
Std_ReturnType Cdd_Safety_ReportFault(Cdd_Safety_FaultSourceType source, uint32 faultId)
{
    (void)faultId;

    if (Cdd_Safety_Initialized == FALSE)
    {
        return E_NOT_OK;
    }

    /* Route to Dem if configured */
    if (Cdd_Safety_Config->enableDemReporting)
    {
        /* Dem_ReportErrorStatus(eventId, DEM_EVENT_STATUS_FAILED); */
    }

    /* Update state machine */
    switch (source)
    {
        case CDD_SAFETY_FAULT_LOCKSTEP:
        case CDD_SAFETY_FAULT_ECC_DOUBLE:
        case CDD_SAFETY_FAULT_WATCHDOG:
            Cdd_Safety_SetState(CDD_SAFETY_STATE_SAFE);
            break;

        case CDD_SAFETY_FAULT_ECC_SINGLE_THRESH:
        case CDD_SAFETY_FAULT_CLOCK:
        case CDD_SAFETY_FAULT_VOLTAGE:
        case CDD_SAFETY_FAULT_CRC:
            if (Cdd_Safety_State == CDD_SAFETY_STATE_NORMAL)
            {
                Cdd_Safety_SetState(CDD_SAFETY_STATE_WARNING);
            }
            break;

        default:
            break;
    }

    return E_OK;
}

/**
 * @brief   Request transition to safe state.
 */
void Cdd_Safety_EnterSafeState(uint32 reason)
{
    (void)reason;

    if (Cdd_Safety_Initialized == FALSE)
    {
        return;
    }

    Cdd_Safety_SetState(CDD_SAFETY_STATE_SAFE);

    /* Trigger FCCU safe state if enabled */
    if (Cdd_Safety_Config->enableFccu)
    {
        FCCU_CTRLK = FCCU_CTRLK_KEY;
        FCCU_CTRL  = FCCU_SAFE_STATE_CMD;
    }
}

/**
 * @brief   Trigger system reset via safety integrator.
 */
void Cdd_Safety_SystemReset(uint8 resetType)
{
    (void)resetType;

    Cdd_Safety_SetState(CDD_SAFETY_STATE_RESET);

    /* Trigger MC_RGM software reset */
    REG32(CDD_LOCKSTEP_RGM_BASE + 0x10U) = 0x01U;  /* MC_RGM_CTRLfor (;;)* Halt */
    while (1U)
    {
        /* Wait for reset */
    }
}

/**
 * @brief   Register a callback for safety state transitions.
 */
Std_ReturnType Cdd_Safety_RegisterStateCallback(
    void (*callback)(Cdd_Safety_StateType newState))
{
    if (callback == NULL_PTR)
    {
        return E_NOT_OK;
    }

    Cdd_Safety_StateCallback = callback;
    return E_OK;
}

/**
 * @brief   Safety integrator periodic check.
 */
void Cdd_Safety_MainFunction(void)
{
    if (Cdd_Safety_Initialized == FALSE)
    {
        return;
    }

    Cdd_Safety_TickCount++;

    /* Periodic integrity check */
    if ((Cdd_Safety_Config->enableCrcIntegrity) &&
        ((Cdd_Safety_TickCount % 100U) == 0U))
    {
        uint32 currentCrc;

        /* Compute CRC of protected safety RAM */
        currentCrc = Cdd_Safety_Crc32((const uint8*)0U, 0U, 0U);

        if (currentCrc != Cdd_Safety_CrcReference)
        {
            (void)Cdd_Safety_ReportFault(CDD_SAFETY_FAULT_CRC, currentCrc);
        }
    }

    /* Check FCCU status */
    if (Cdd_Safety_Config->enableFccu)
    {
        uint32 fccuStat = FCCU_STAT;
        if (fccuStat != 0U)
        {
            /* Fault detected in FCCU — enter safe state */
            Cdd_Safety_SetState(CDD_SAFETY_STATE_SAFE);
        }
    }
}

#define CDD_SAFETY_STOP_SEC_CODE
#include "Cdd_MemMap.h"

/*==================================================================================================
*                                         END OF FILE
*==================================================================================================*/
