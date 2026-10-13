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
* @file Mem_MemMap.h
* @brief Mem memory mapping
*================================================================================================*/

/*==================================================================================================
 *                                      MEMORY SERVICE (Mem)
 *==================================================================================================
 * FILENAME: Mem_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping header file for Memory Service module
 *==================================================================================================
 */

#ifndef MEM_MEMMAP_H
#define MEM_MEMMAP_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif


/*==================================================================================================
 *                                    MEMORY SECTION MAPPING
 *==================================================================================================*/

#ifdef MEM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section ".bss.Mem"
#endif

#ifdef MEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section
#endif

#ifdef MEM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef MEM_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.Mem"
#endif

#ifdef MEM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef MEM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
#endif

#ifdef MEM_START_SEC_CONST_UNSPECIFIED
    #undef MEM_START_SEC_CONST_UNSPECIFIED
    #pragma section ".rodata.Mem"
#endif

#ifdef MEM_STOP_SEC_CONST_UNSPECIFIED
    #undef MEM_STOP_SEC_CONST_UNSPECIFIED
    #pragma section
#endif

#ifdef MEM_START_SEC_CODE
    #undef MEM_START_SEC_CODE
    #pragma section ".text.Mem"
#endif

#ifdef MEM_STOP_SEC_CODE
    #undef MEM_STOP_SEC_CODE
    #pragma section
#endif

#ifdef MEM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.Mem.Config"
#endif

#ifdef MEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
#endif


#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif /* MEM_MEMMAP_H */
