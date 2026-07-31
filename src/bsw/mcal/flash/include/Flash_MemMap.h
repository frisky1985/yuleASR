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

/*==================================================================================================
 *                                      FLASH DRIVER MEMORY MAP
 *==================================================================================================
 * FILENAME: Flash_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping for Flash Driver module (MCAL Layer)
 *
 * All sections map to their defaults (no special memory placement) so the
 * driver compiles on any toolchain. In a production build, sections are
 * assigned by the linker script / #pragma section directives.
 *================================================================================================*/

#ifndef FLASH_MEMMAP_H
#define FLASH_MEMMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_MEMMAP_NOOP(section)

#ifndef FLASH_START_SEC_CODE
#define FLASH_START_SEC_CODE
#endif
#ifndef FLASH_STOP_SEC_CODE
#define FLASH_STOP_SEC_CODE
#endif
#ifndef FLASH_START_SEC_CODE_FAST
#define FLASH_START_SEC_CODE_FAST
#endif
#ifndef FLASH_STOP_SEC_CODE_FAST
#define FLASH_STOP_SEC_CODE_FAST
#endif
#ifndef FLASH_START_SEC_CODE_SLOW
#define FLASH_START_SEC_CODE_SLOW
#endif
#ifndef FLASH_STOP_SEC_CODE_SLOW
#define FLASH_STOP_SEC_CODE_SLOW
#endif
#ifndef FLASH_START_SEC_CONST_8
#define FLASH_START_SEC_CONST_8
#endif
#ifndef FLASH_STOP_SEC_CONST_8
#define FLASH_STOP_SEC_CONST_8
#endif
#ifndef FLASH_START_SEC_CONST_16
#define FLASH_START_SEC_CONST_16
#endif
#ifndef FLASH_STOP_SEC_CONST_16
#define FLASH_STOP_SEC_CONST_16
#endif
#ifndef FLASH_START_SEC_CONST_32
#define FLASH_START_SEC_CONST_32
#endif
#ifndef FLASH_STOP_SEC_CONST_32
#define FLASH_STOP_SEC_CONST_32
#endif
#ifndef FLASH_START_SEC_CONST_UNSPECIFIED
#define FLASH_START_SEC_CONST_UNSPECIFIED
#endif
#ifndef FLASH_STOP_SEC_CONST_UNSPECIFIED
#define FLASH_STOP_SEC_CONST_UNSPECIFIED
#endif
#ifndef FLASH_START_SEC_VAR_CLEARED_UNSPECIFIED
#define FLASH_START_SEC_VAR_CLEARED_UNSPECIFIED
#endif
#ifndef FLASH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#define FLASH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#endif
#ifndef FLASH_START_SEC_VAR_INIT_UNSPECIFIED
#define FLASH_START_SEC_VAR_INIT_UNSPECIFIED
#endif
#ifndef FLASH_STOP_SEC_VAR_INIT_UNSPECIFIED
#define FLASH_STOP_SEC_VAR_INIT_UNSPECIFIED
#endif
#ifndef FLASH_START_SEC_VAR_NO_INIT
#define FLASH_START_SEC_VAR_NO_INIT
#endif
#ifndef FLASH_STOP_SEC_VAR_NO_INIT
#define FLASH_STOP_SEC_VAR_NO_INIT
#endif

#ifdef __cplusplus
}
#endif

#endif /* FLASH_MEMMAP_H */
