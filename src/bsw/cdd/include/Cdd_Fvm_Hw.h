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
 * @file    Cdd_Fvm_Hw.h
 * @brief   Complex Driver — Flash Virtual Memory (FVM) Hardware Abstraction
 * @version 1.0.0
 * @date    2026-08-09
 *
 * @details
 *   Hardware backend interface of the FVM complex driver (same layering
 *   pattern as Fls_Hw.h).  The module logic in Cdd_Fvm_1.0.0.c never
 *   touches flash directly — it always goes through this interface.
 *
 *   Two backends are provided (selected by CDD_FVM_BACKEND in Cdd_Fvm_Cfg.h):
 *     - CDD_FVM_BACKEND_RAM : in-RAM mirror (native build / SIL / unit tests)
 *     - CDD_FVM_BACKEND_FLS : real flash through the MCAL Fls driver (target)
 *
 * @ASIL-D Safety Level
 */

#ifndef CDD_FVM_HW_H
#define CDD_FVM_HW_H

#include "Std_Types.h"
#include "Cdd_Fvm_Cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                         HW API
 *==================================================================================================*/

/**
 * @brief   Initialize the hardware backend
 * @details RAM backend: the mirror is brought into the erased (0xFF)
 *          state.  Fls backend: verifies the Fls driver is initialized.
 * @return  E_OK on success, E_NOT_OK if the backend is not usable
 */
extern Std_ReturnType Cdd_Fvm_HwInit(void);

/**
 * @brief   Erase a flash region (sector granularity)
 * @param   startAddr  [in] Start address of the region
 * @param   size       [in] Size in bytes (must be sector aligned)
 * @return  E_OK on success, E_NOT_OK on backend failure
 */
extern Std_ReturnType Cdd_Fvm_HwErase(uint32 startAddr, uint32 size);

/**
 * @brief   Write data to a flash region
 * @param   targetAddr [in] Target address
 * @param   data       [in] Source data pointer (must not be NULL_PTR)
 * @param   length     [in] Number of bytes to write
 * @return  E_OK on success, E_NOT_OK on backend failure
 */
extern Std_ReturnType Cdd_Fvm_HwWrite(uint32 targetAddr, const uint8* data, uint32 length);

/**
 * @brief   Read data from a flash region
 * @param   srcAddr    [in] Source address
 * @param   data       [out] Destination buffer (must not be NULL_PTR)
 * @param   length     [in] Number of bytes to read
 * @return  E_OK on success, E_NOT_OK on backend failure
 */
extern Std_ReturnType Cdd_Fvm_HwRead(uint32 srcAddr, uint8* data, uint32 length);

#ifdef __cplusplus
}
#endif

#endif /* CDD_FVM_HW_H */
