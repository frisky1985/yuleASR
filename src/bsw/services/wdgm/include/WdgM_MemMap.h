/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/** @file Wdgm_MemMap.h
 * @brief Watchdog Manager memory mapping
 */

#ifndef WDGM_MEMMAP_H
#define WDGM_MEMMAP_H

/*============================================================================
 *  CODE SECTIONS
 *===========================================================================*/

#ifdef WDGM_START_SEC_CODE
    #pragma section ".text.Wdgm" ax
    #undef WDGM_START_SEC_CODE
    #undef MEMMAP_ERROR
#endif

#ifdef WDGM_STOP_SEC_CODE
    #pragma section
    #undef WDGM_STOP_SEC_CODE
    #undef MEMMAP_ERROR
#endif

/*============================================================================
 *  CONFIGURATION DATA SECTIONS
 *===========================================================================*/

#ifdef WDGM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.Wdgm.cfg" a
    #undef WDGM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
    #undef WDGM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

/*============================================================================
 *  VAR SECTIONS
 *===========================================================================*/

#ifdef WDGM_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.Wdgm" aw
    #undef WDGM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
    #undef WDGM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGM_START_SEC_VAR_NOINIT_UNSPECIFIED
    #pragma section ".bss.Wdgm" aw
    #undef WDGM_START_SEC_VAR_NOINIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGM_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    #pragma section
    #undef WDGM_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#endif /* WDGM_MEMMAP_H */
