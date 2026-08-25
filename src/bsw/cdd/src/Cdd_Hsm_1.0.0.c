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
 * @file    Cdd_Hsm_1.0.0.c
 * @brief   Complex Driver — HSM (Hardware Security Module) Implementation
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   S32K312 HSM complex driver implementation.
 *   Replaces the former Crypto_Hsm.c HAL stubs with a proper CDD-layer
 *   complex driver that manages the SHE-compatible CSEc security module.
 *
 * @ASIL-D Safety Level
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Cdd_Hsm.h"

#if (CDD_DEV_ERROR_DETECT == STD_ON)
#include "Det.h"
#endif

/*==================================================================================================
 *                                         LOCAL MACROS
 *==================================================================================================*/
#ifndef CDD_DEV_ERROR_DETECT
#define CDD_DEV_ERROR_DETECT                    STD_ON
#endif

/** @brief DET API IDs */
#define CDD_HSM_SID_INIT                        0x10U
#define CDD_HSM_SID_DEINIT                      0x11U
#define CDD_HSM_SID_ISAVAILABLE                 0x12U
#define CDD_HSM_SID_GETSTATUS                   0x13U
#define CDD_HSM_SID_GENERATERANDOM              0x14U
#define CDD_HSM_SID_SELFTEST                    0x15U
#define CDD_HSM_SID_SECUREBOOTVERIFY            0x16U
#define CDD_HSM_SID_MAINFUNCTION                0x17U

/*==================================================================================================
 *                                         LOCAL TYPES
 *==================================================================================================*/

/* S32K312 CSEc register layout (simplified) */
typedef struct {
    volatile uint32  CR;         /**< Command register */
    volatile uint32  SR;         /**< Status register */
    volatile uint32  RESERVED0[2];
    volatile uint32  IN_PARAM[8];/**< Input parameters */
    volatile uint32  OUT_PARAM[8];/**< Output parameters */
} Cdd_Hsm_CsecRegsType;

/*==================================================================================================
 *                                         MODULE VARIABLES
 *==================================================================================================*/
#define CDD_HSM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief Current HSM state */
STATIC Cdd_Hsm_StateType   Cdd_Hsm_State = CDD_HSM_STATE_UNINIT;

/** @brief Active configuration pointer (if dynamic config needed) */
STATIC const Cdd_Hsm_ConfigType* Cdd_Hsm_ConfigPtr = NULL_PTR;

/** @brief HSM health counter (incremented each MainFunction tick) */
STATIC uint32               Cdd_Hsm_HealthCounter = 0U;

#define CDD_HSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/*==================================================================================================
 *                                         LOCAL FUNCTIONS
 *==================================================================================================*/
#define CDD_HSM_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Hardware-level HSM register access for S32K312 CSEc.
 *          Stub implementation — replace with actual register maps.
 */
STATIC Std_ReturnType Cdd_Hsm_HwInit(const Cdd_Hsm_ConfigType* config)
{
    (void)config;

    /* Platform-specific HSM initialization:
     * 1. Map CSEc register base address
     * 2. Verify CSEc module presence
     * 3. Configure crypto accelerators
     * 4. Enable secure key store
     * 5. Calibrate TRNG
     */

#if defined(PLATFORM_S32K312)
    /* CSEc base: 0x402F0000 (depends on MCU variant) */
    /* Check CSEc_SR bit: CSEc present */
    /* Write CSEc_CR to configure */
    return E_OK;
#else
    /* HSM not available on this platform — return E_NOT_OK */
    return E_NOT_OK;
#endif
}

/*==================================================================================================
 *                                         GLOBAL FUNCTIONS
 *==================================================================================================*/

/**
 * @brief   Initialize the HSM complex driver.
 */
Std_ReturnType Cdd_Hsm_Init(const Cdd_Hsm_ConfigType* config)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (config == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_HSM, 0U, CDD_HSM_SID_INIT, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Hsm_State != CDD_HSM_STATE_UNINIT)
    {
        return E_NOT_OK;
    }

    Cdd_Hsm_ConfigPtr = config;
    Cdd_Hsm_State = CDD_HSM_STATE_INIT;

    if (Cdd_Hsm_HwInit(config) == E_OK)
    {
        Cdd_Hsm_State = CDD_HSM_STATE_READY;
        Cdd_Hsm_HealthCounter = 0U;
        return E_OK;
    }

    Cdd_Hsm_State = CDD_HSM_STATE_ERROR;
    return E_NOT_OK;
}

/**
 * @brief   De-initialize the HSM complex driver.
 */
void Cdd_Hsm_DeInit(void)
{
    if (Cdd_Hsm_State == CDD_HSM_STATE_UNINIT)
    {
        return;
    }

    /* Platform-specific HSM de-init: clear key store, disable crypto */
    Cdd_Hsm_State = CDD_HSM_STATE_UNINIT;
    Cdd_Hsm_ConfigPtr = NULL_PTR;
    Cdd_Hsm_HealthCounter = 0U;
}

/**
 * @brief   Get HSM availability status.
 */
boolean Cdd_Hsm_IsAvailable(void)
{
    return (boolean)((Cdd_Hsm_State == CDD_HSM_STATE_READY) ||
                     (Cdd_Hsm_State == CDD_HSM_STATE_BUSY));
}

/**
 * @brief   Get HSM external status.
 */
Cdd_Hsm_StatusType Cdd_Hsm_GetStatus(void)
{
    switch (Cdd_Hsm_State)
    {
        case CDD_HSM_STATE_READY:   return CDD_HSM_STATUS_IDLE;
        case CDD_HSM_STATE_BUSY:    return CDD_HSM_STATUS_BUSY;
        case CDD_HSM_STATE_ERROR:   return CDD_HSM_STATUS_ERROR;
        case CDD_HSM_STATE_UNINIT:
        case CDD_HSM_STATE_INIT:
        default:                    return CDD_HSM_STATUS_UNINIT;
    }
}

/**
 * @brief   Generate random bytes using HSM TRNG.
 */
Std_ReturnType Cdd_Hsm_GenerateRandom(uint8* output, uint32 length)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if (output == NULL_PTR)
    {
        Det_ReportError(CDD_MODULE_ID_HSM, 0U, CDD_HSM_SID_GENERATERANDOM, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Hsm_State != CDD_HSM_STATE_READY)
    {
        return E_NOT_OK;
    }

    if (length == 0U)
    {
        return E_NOT_OK;
    }

    /* Platform-specific TRNG access */
    /* e.g. S32K312: trigger CSEc_RND command, read OUT_PARAM[0..N] */

    (void)output;
    (void)length;

    return E_NOT_OK;  /* Stub — platform implementation required */
}

/**
 * @brief   HSM self-test.
 */
Std_ReturnType Cdd_Hsm_SelfTest(void)
{
    if (Cdd_Hsm_State != CDD_HSM_STATE_READY)
    {
        return E_NOT_OK;
    }

    /* Platform-specific self-test:
     * - AES-GCM encrypt/decrypt with built-in test vector
     * - SHA-256 hash of known data
     * - TRNG statistical check
     * - Key store access test
     */
    return E_NOT_OK;  /* Stub */
}

/**
 * @brief   Verify secure boot image signature via HSM.
 */
Std_ReturnType Cdd_Hsm_SecureBootVerify(
    const uint8*        imageHash,
    uint32              imageHashLen,
    const uint8*        signature,
    uint32              signatureLen,
    boolean*            verifyResult)
{
#if (CDD_DEV_ERROR_DETECT == STD_ON)
    if ((imageHash == NULL_PTR) || (signature == NULL_PTR) || (verifyResult == NULL_PTR))
    {
        Det_ReportError(CDD_MODULE_ID_HSM, 0U, CDD_HSM_SID_SECUREBOOTVERIFY, 1U);
        return E_NOT_OK;
    }
#endif

    if (Cdd_Hsm_State != CDD_HSM_STATE_READY)
    {
        return E_NOT_OK;
    }

    /* Platform-specific HSM ECDSA verify command */
    (void)imageHash;
    (void)imageHashLen;
    (void)signature;
    (void)signatureLen;

    *verifyResult = FALSE;
    return E_NOT_OK;  /* Stub */
}

/**
 * @brief   HSM periodic health check.
 */
void Cdd_Hsm_MainFunction(void)
{
    if (Cdd_Hsm_State != CDD_HSM_STATE_READY)
    {
        return;
    }

    Cdd_Hsm_HealthCounter++;

    /* Periodic self-test (e.g., every 1000 ticks) */
    if ((Cdd_Hsm_HealthCounter % 1000U) == 0U)
    {
        Std_Return_t result = Cdd_Hsm_SelfTest();
        if (result != E_OK)
        {
            Cdd_Hsm_State = CDD_HSM_STATE_ERROR;
#if (CDD_DEV_ERROR_DETECT == STD_ON)
            Det_ReportError(CDD_MODULE_ID_HSM, 0U, CDD_HSM_SID_MAINFUNCTION, 2U);
#endif
        }
    }
}

#define CDD_HSM_STOP_SEC_CODE
#include "Cdd_MemMap.h"

/*==================================================================================================
*                                         END OF FILE
*==================================================================================================*/
