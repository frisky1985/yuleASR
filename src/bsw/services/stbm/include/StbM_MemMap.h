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

/**
 * @file StbM_MemMap.h
 * @brief Memory mapping for StbM module
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef STBM_MEMMAP_H
#define STBM_MEMMAP_H

/*==================================================================================================
*                                    MEMORY SECTION MAPPING
==================================================================================================*/

/* Code section */
#ifdef STBM_START_SEC_CODE
    #undef STBM_START_SEC_CODE
    #undef STBM_STOP_SEC_CODE
    #define MEMMAP_ERROR
#endif

/* Const data section */
#ifdef STBM_START_SEC_CONST_UNSPECIFIED
    #undef STBM_START_SEC_CONST_UNSPECIFIED
    #undef STBM_STOP_SEC_CONST_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Data section */
#ifdef STBM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef STBM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef STBM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

#ifdef STBM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef STBM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef STBM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Configuration data section */
#ifdef STBM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef STBM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef STBM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Error check */
#ifdef MEMMAP_ERROR
    #error "StbM_MemMap.h: Invalid memory section directive"
    #undef MEMMAP_ERROR
#endif

#endif /* STBM_MEMMAP_H */
