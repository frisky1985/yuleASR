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
 * @file SomeIpTp_MemMap.h
 * @brief Memory mapping for SomeIpTp module
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef SOMEIPTP_MEMMAP_H
#define SOMEIPTP_MEMMAP_H

/*==================================================================================================
*                                    MEMORY SECTION MAPPING
==================================================================================================*/

/* Code section */
#ifdef SOMEIPTP_START_SEC_CODE
    #undef SOMEIPTP_START_SEC_CODE
    #undef SOMEIPTP_STOP_SEC_CODE
    #define MEMMAP_ERROR
#endif

/* Const data section */
#ifdef SOMEIPTP_START_SEC_CONST_UNSPECIFIED
    #undef SOMEIPTP_START_SEC_CONST_UNSPECIFIED
    #undef SOMEIPTP_STOP_SEC_CONST_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Data section */
#ifdef SOMEIPTP_START_SEC_VAR_INIT_UNSPECIFIED
    #undef SOMEIPTP_START_SEC_VAR_INIT_UNSPECIFIED
    #undef SOMEIPTP_STOP_SEC_VAR_INIT_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

#ifdef SOMEIPTP_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef SOMEIPTP_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef SOMEIPTP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Configuration data section */
#ifdef SOMEIPTP_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef SOMEIPTP_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef SOMEIPTP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Buffer section */
#ifdef SOMEIPTP_START_SEC_VAR_NO_INIT_8BIT
    #undef SOMEIPTP_START_SEC_VAR_NO_INIT_8BIT
    #undef SOMEIPTP_STOP_SEC_VAR_NO_INIT_8BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOMEIPTP_START_SEC_VAR_NO_INIT_UNSPECIFIED
    #undef SOMEIPTP_START_SEC_VAR_NO_INIT_UNSPECIFIED
    #undef SOMEIPTP_STOP_SEC_VAR_NO_INIT_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Error check */
#ifdef MEMMAP_ERROR
    #error "SomeIpTp_MemMap.h: Invalid memory section directive"
    #undef MEMMAP_ERROR
#endif

#endif /* SOMEIPTP_MEMMAP_H */
