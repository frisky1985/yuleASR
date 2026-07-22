/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
* Dependencies         : GNU GCC / ARMCC
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Cdd_MemMap.h
 * @brief   Memory mapping header for CDD layer modules
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   AUTOSAR-compliant memory section management for all CDD modules.
 *   Handles the following section types:
 *     - CODE          : Executable code
 *     - CONST_*       : Read-only data (constants, config data)
 *     - VAR_CLEARED_* : Zero-initialized variables (.bss)
 *     - VAR_INIT_*    : Pre-initialized variables (.data)
 *
 *   Each CDD module uses CDD_<MODULE>_START/STOP_* macros to declare
 *   memory sections.  This header maps them to the appropriate
 *   pragma / __attribute__ directives depending on the compiler.
 *
 * @implements AUTOSAR_SWS_MemMap
 */

#ifndef CDD_MEMMAP_H
#define CDD_MEMMAP_H

/*==================================================================================================
 *  DISCLAIMER
 *  The actual memory mapping is tool-chain specific.  This implementation
 *  uses section pragmas for GCC-compatible toolchains.  For ARMCC/EWARM
 *  or other compilers, adapt the #pragma / __attribute__ as needed.
 *==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------------------------------
 *  START OF CODE SECTION
 *--------------------------------------------------------------------------------------------------*/
#ifdef CDD_START_SEC_CODE
    #undef  CDD_START_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section ".text.Cdd" ax
#endif

#ifdef CDD_HSM_START_SEC_CODE
    #undef  CDD_HSM_START_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section ".text.Cdd.Hsm" ax
#endif

#ifdef CDD_RAMECC_START_SEC_CODE
    #undef  CDD_RAMECC_START_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section ".text.Cdd.RamEcc" ax
#endif

#ifdef CDD_LOCKSTEP_START_SEC_CODE
    #undef  CDD_LOCKSTEP_START_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section ".text.Cdd.Lockstep" ax
#endif

#ifdef CDD_SAFETY_START_SEC_CODE
    #undef  CDD_SAFETY_START_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section ".text.Cdd.Safety" ax
#endif

#ifdef CDD_BOOT_START_SEC_CODE
    #undef  CDD_BOOT_START_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section ".text.Cdd.Boot" ax
#endif

/*--------------------------------------------------------------------------------------------------
 *  STOP OF CODE SECTION
 *--------------------------------------------------------------------------------------------------*/
#ifdef CDD_STOP_SEC_CODE
    #undef  CDD_STOP_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_HSM_STOP_SEC_CODE
    #undef  CDD_HSM_STOP_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_RAMECC_STOP_SEC_CODE
    #undef  CDD_RAMECC_STOP_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_LOCKSTEP_STOP_SEC_CODE
    #undef  CDD_LOCKSTEP_STOP_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_SAFETY_STOP_SEC_CODE
    #undef  CDD_SAFETY_STOP_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_BOOT_STOP_SEC_CODE
    #undef  CDD_BOOT_STOP_SEC_CODE
    #undef  MEMMAP_ERROR
    #pragma section
#endif

/*--------------------------------------------------------------------------------------------------
 *  CONST (read-only data)
 *--------------------------------------------------------------------------------------------------*/
#ifdef CDD_START_SEC_CONST_UNSPECIFIED
    #undef  CDD_START_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd" a
#endif

#ifdef CDD_HSM_START_SEC_CONST_UNSPECIFIED
    #undef  CDD_HSM_START_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd.Hsm" a
#endif

#ifdef CDD_RAMECC_START_SEC_CONST_UNSPECIFIED
    #undef  CDD_RAMECC_START_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd.RamEcc" a
#endif

#ifdef CDD_LOCKSTEP_START_SEC_CONST_UNSPECIFIED
    #undef  CDD_LOCKSTEP_START_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd.Lockstep" a
#endif

#ifdef CDD_SAFETY_START_SEC_CONST_UNSPECIFIED
    #undef  CDD_SAFETY_START_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd.Safety" a
#endif

#ifdef CDD_BOOT_START_SEC_CONST_UNSPECIFIED
    #undef  CDD_BOOT_START_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd.Boot" a
#endif

#ifdef CDD_STOP_SEC_CONST_UNSPECIFIED
    #undef  CDD_STOP_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_HSM_STOP_SEC_CONST_UNSPECIFIED
    #undef  CDD_HSM_STOP_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_RAMECC_STOP_SEC_CONST_UNSPECIFIED
    #undef  CDD_RAMECC_STOP_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_LOCKSTEP_STOP_SEC_CONST_UNSPECIFIED
    #undef  CDD_LOCKSTEP_STOP_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_SAFETY_STOP_SEC_CONST_UNSPECIFIED
    #undef  CDD_SAFETY_STOP_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_BOOT_STOP_SEC_CONST_UNSPECIFIED
    #undef  CDD_BOOT_STOP_SEC_CONST_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

/*--------------------------------------------------------------------------------------------------
 *  VAR_CLEARED (BSS — zero-initialized)
 *--------------------------------------------------------------------------------------------------*/
#ifdef CDD_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".bss.Cdd" aw
#endif

#ifdef CDD_HSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_HSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".bss.Cdd.Hsm" aw
#endif

#ifdef CDD_RAMECC_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_RAMECC_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".bss.Cdd.RamEcc" aw
#endif

#ifdef CDD_LOCKSTEP_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_LOCKSTEP_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".bss.Cdd.Lockstep" aw
#endif

#ifdef CDD_SAFETY_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_SAFETY_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".bss.Cdd.Safety" aw
#endif

#ifdef CDD_BOOT_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_BOOT_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".bss.Cdd.Boot" aw
#endif

#ifdef CDD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_HSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_HSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_RAMECC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_RAMECC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_LOCKSTEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_LOCKSTEP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_SAFETY_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_SAFETY_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_BOOT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  CDD_BOOT_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

/*--------------------------------------------------------------------------------------------------
 *  VAR_INIT (.data — pre-initialized)
 *--------------------------------------------------------------------------------------------------*/
#ifdef CDD_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".data.Cdd" aw
#endif

#ifdef CDD_HSM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_HSM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".data.Cdd.Hsm" aw
#endif

#ifdef CDD_RAMECC_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_RAMECC_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".data.Cdd.RamEcc" aw
#endif

#ifdef CDD_LOCKSTEP_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_LOCKSTEP_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".data.Cdd.Lockstep" aw
#endif

#ifdef CDD_SAFETY_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_SAFETY_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".data.Cdd.Safety" aw
#endif

#ifdef CDD_BOOT_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_BOOT_START_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".data.Cdd.Boot" aw
#endif

#ifdef CDD_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_HSM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_HSM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_RAMECC_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_RAMECC_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_LOCKSTEP_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_LOCKSTEP_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_SAFETY_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_SAFETY_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

#ifdef CDD_BOOT_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  CDD_BOOT_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

/*--------------------------------------------------------------------------------------------------
 *  CONFIG DATA (link-time configuration constants)
 *--------------------------------------------------------------------------------------------------*/
#ifdef CDD_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef  CDD_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section ".rodata.Cdd.Config" a
#endif

#ifdef CDD_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef  CDD_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef  MEMMAP_ERROR
    #pragma section
#endif

/*--------------------------------------------------------------------------------------------------
 *  ERROR CHECK — any unhandled Start/Stop macro triggers compilation failure
 *--------------------------------------------------------------------------------------------------*/
#ifdef MEMMAP_ERROR
    #error "Cdd_MemMap.h: Unknown or duplicate section macro — check section name"
#endif

#ifdef __cplusplus
}
#endif

#endif /* CDD_MEMMAP_H */
