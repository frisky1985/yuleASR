/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
*
* SPDX-License-Identifier: MIT
*
* @file NvM_MemMap.h
* @brief NvM memory mapping
*================================================================================================*/

/*==================================================================================================
 *                                NvM ECC HANDLER MEMORY MAPPING
 *==================================================================================================
 * FILENAME: NvM_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 *
 * DESCRIPTION: Memory mapping header for NvM ECC Handler module.
 * All sections are mapped to their defaults (no special memory placement).
 *
 *================================================================================================*/

#ifndef NVM_MEMMAP_H
#define NVM_MEMMAP_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif


/* All sections map to their default (no special section assignment).
 * In a production build, these would be defined by the linker script.
 */

#ifndef NVM_START_SEC_CONST_UNSPECIFIED
#define NVM_START_SEC_CONST_UNSPECIFIED
#endif

#ifndef NVM_STOP_SEC_CONST_UNSPECIFIED
#define NVM_STOP_SEC_CONST_UNSPECIFIED
#endif

#ifndef NVM_START_SEC_CODE
#define NVM_START_SEC_CODE
#endif

#ifndef NVM_STOP_SEC_CODE
#define NVM_STOP_SEC_CODE
#endif

#ifndef NVM_START_SEC_VAR_CLEARED
#define NVM_START_SEC_VAR_CLEARED
#endif

#ifndef NVM_STOP_SEC_VAR_CLEARED
#define NVM_STOP_SEC_VAR_CLEARED
#endif

#ifndef NVM_START_SEC_VAR_NO_INIT
#define NVM_START_SEC_VAR_NO_INIT
#endif

#ifndef NVM_STOP_SEC_VAR_NO_INIT
#define NVM_STOP_SEC_VAR_NO_INIT
#endif


#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif /* NVM_MEMMAP_H */
