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
/* @req SHALL_CDD */


/**
 * @file    Cdd_Fvm_Hw.c
 * @brief   Complex Driver — Flash Virtual Memory (FVM) Hardware Backend
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Backend implementations selected by CDD_FVM_BACKEND:
 *
 *   1. CDD_FVM_BACKEND_RAM (default for native / SIL / unit tests):
 *      an in-RAM mirror of the flash address space.  Addresses are
 *      offset against CDD_FVM_RAM_BASE_ADDR into a static pool.  This
 *      keeps every FVM feature (failover, copy, protection, integrity)
 *      fully executable and testable on the host.
 *
 *   2. CDD_FVM_BACKEND_FLS (production S32K312 target):
 *      synchronous wrappers around the MCAL Fls driver.  A job is
 *      started and the driver status is polled until idle (bounded by
 *      CDD_FVM_FLS_TIMEOUT_MS), then Fls_GetJobResult is evaluated.
 *      Erase and write address/size requirements are delegated to Fls.
 *
 * @ASIL-D Safety Level
 */

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Cdd_Fvm_Hw.h"

/* Defensive fallback: see Cdd_Fvm_1.0.0.c — Std_Types.h may resolve from
 * the OS include tree without the STATIC macro. */
#ifndef STATIC
#define STATIC                                     static
#endif

#if (CDD_FVM_BACKEND == CDD_FVM_BACKEND_FLS)
#include "Fls.h"
#endif

/*==================================================================================================
 *                                         RAM BACKEND
 *==================================================================================================*/
#if (CDD_FVM_BACKEND == CDD_FVM_BACKEND_RAM)

#include <string.h>

#define CDD_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

/** @brief RAM mirror of the flash address space */
STATIC uint8 Cdd_Fvm_RamPool[CDD_FVM_RAM_POOL_SIZE];

#define CDD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "Cdd_MemMap.h"

Std_ReturnType Cdd_Fvm_HwInit(void)
{
    /* Bring the mirror into the erased (0xFF) state */
    (void)memset(Cdd_Fvm_RamPool, 0xFF, sizeof(Cdd_Fvm_RamPool));
    return E_OK;
}

/**
 * @brief Map a flash address to the RAM pool; NULL_PTR if out of range
 */
STATIC uint8* Cdd_Fvm_HwRamMap(uint32 addr, uint32 length)
{
    uint8* result = NULL_PTR;

    if ((addr >= CDD_FVM_RAM_BASE_ADDR) && (length <= CDD_FVM_RAM_POOL_SIZE))
    {
        uint32 offset = addr - CDD_FVM_RAM_BASE_ADDR;
        if (offset <= (CDD_FVM_RAM_POOL_SIZE - length))
        {
            result = &Cdd_Fvm_RamPool[offset];
        }
    }
    return result;
}

Std_ReturnType Cdd_Fvm_HwErase(uint32 startAddr, uint32 size)
{
    uint8* region;
    Std_ReturnType result = E_NOT_OK;

    region = Cdd_Fvm_HwRamMap(startAddr, size);
    if (region != NULL_PTR)
    {
        (void)memset(region, 0xFF, (size_t)size);
        result = E_OK;
    }
    return result;
}

Std_ReturnType Cdd_Fvm_HwWrite(uint32 targetAddr, const uint8* data, uint32 length)
{
    uint8* region;
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        region = Cdd_Fvm_HwRamMap(targetAddr, length);
        if (region != NULL_PTR)
        {
            (void)memcpy(region, data, (size_t)length);
            result = E_OK;
        }
    }
    return result;
}

Std_ReturnType Cdd_Fvm_HwRead(uint32 srcAddr, uint8* data, uint32 length)
{
    const uint8* region;
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        region = Cdd_Fvm_HwRamMap(srcAddr, length);
        if (region != NULL_PTR)
        {
            (void)memcpy(data, region, (size_t)length);
            result = E_OK;
        }
    }
    return result;
}

#endif /* CDD_FVM_BACKEND_RAM */

/*==================================================================================================
 *                                         FLS BACKEND (TARGET)
 *==================================================================================================*/
#if (CDD_FVM_BACKEND == CDD_FVM_BACKEND_FLS)

/** @brief Poll timeout for one synchronous Fls job in milliseconds */
#define CDD_FVM_FLS_TIMEOUT_MS               (1000u)

/**
 * @brief Wait for an asynchronous Fls job to finish
 * @return E_OK when the job completed with MEMIF_JOB_OK
 */
STATIC Std_ReturnType Cdd_Fvm_HwFlsWait(void)
{
    uint32 elapsed;
    Std_ReturnType result = E_NOT_OK;

    for (elapsed = 0u; elapsed < CDD_FVM_FLS_TIMEOUT_MS; elapsed++)
    {
        if (Fls_GetStatus() == FLS_IDLE)
        {
            if (Fls_GetJobResult() == MEMIF_JOB_OK)
            {
                result = E_OK;
            }
            break;
        }
        /* 1 ms tick granularity: Fls_MainFunction is scheduled by BSW scheduler */
    }
    return result;
}

Std_ReturnType Cdd_Fvm_HwErase(uint32 startAddr, uint32 size)
{
    Std_ReturnType result = E_NOT_OK;

    if (Fls_Erase((Fls_AddressType)startAddr, (Fls_LengthType)size) == E_OK)
    {
        result = Cdd_Fvm_HwFlsWait();
    }
    return result;
}

Std_ReturnType Cdd_Fvm_HwWrite(uint32 targetAddr, const uint8* data, uint32 length)
{
    Std_ReturnType result = E_NOT_OK;

    if ((data != NULL_PTR) &&
        (Fls_Write((Fls_AddressType)targetAddr, data, (Fls_LengthType)length) == E_OK))
    {
        result = Cdd_Fvm_HwFlsWait();
    }
    return result;
}

Std_ReturnType Cdd_Fvm_HwRead(uint32 srcAddr, uint8* data, uint32 length)
{
    Std_ReturnType result = E_NOT_OK;

    if (data != NULL_PTR)
    {
        Fls_Read((Fls_AddressType)srcAddr, data, (Fls_LengthType)length);
        result = E_OK;
    }
    return result;
}

#endif /* CDD_FVM_BACKEND_FLS */
