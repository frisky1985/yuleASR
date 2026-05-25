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
 *                              CRYPTO INTERFACE (CryIf)
 *==================================================================================================
 * FILENAME: CryIf_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping header file for CryIf module
 *==================================================================================================
 */

#ifndef CRYIF_MEMMAP_H
#define CRYIF_MEMMAP_H

/*==================================================================================================
 *                                         SECTIONS
 *==================================================================================================*/

/* 
 * VAR_CLEARED_UNSPECIFIED Section 
 * Used for uninitialized variables
 */
#ifdef CRYIF_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CRYIF_START_SEC_VAR_CLEARED_UNSPECIFIED
    #define START_SEC_VAR_CLEARED_UNSPECIFIED
#endif

#ifdef CRYIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CRYIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #define STOP_SEC_VAR_CLEARED_UNSPECIFIED
#endif

/*
 * VAR_INIT_UNSPECIFIED Section
 * Used for initialized variables
 */
#ifdef CRYIF_START_SEC_VAR_INIT_UNSPECIFIED
    #undef CRYIF_START_SEC_VAR_INIT_UNSPECIFIED
    #define START_SEC_VAR_INIT_UNSPECIFIED
#endif

#ifdef CRYIF_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef CRYIF_STOP_SEC_VAR_INIT_UNSPECIFIED
    #define STOP_SEC_VAR_INIT_UNSPECIFIED
#endif

/*
 * CONST_UNSPECIFIED Section
 * Used for constants
 */
#ifdef CRYIF_START_SEC_CONST_UNSPECIFIED
    #undef CRYIF_START_SEC_CONST_UNSPECIFIED
    #define START_SEC_CONST_UNSPECIFIED
#endif

#ifdef CRYIF_STOP_SEC_CONST_UNSPECIFIED
    #undef CRYIF_STOP_SEC_CONST_UNSPECIFIED
    #define STOP_SEC_CONST_UNSPECIFIED
#endif

/*
 * CODE Section
 * Used for code
 */
#ifdef CRYIF_START_SEC_CODE
    #undef CRYIF_START_SEC_CODE
    #define START_SEC_CODE
#endif

#ifdef CRYIF_STOP_SEC_CODE
    #undef CRYIF_STOP_SEC_CODE
    #define STOP_SEC_CODE
#endif

/*
 * CONFIG_DATA_UNSPECIFIED Section
 * Used for configuration data
 */
#ifdef CRYIF_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CRYIF_START_SEC_CONFIG_DATA_UNSPECIFIED
    #define START_SEC_CONFIG_DATA_UNSPECIFIED
#endif

#ifdef CRYIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CRYIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #define STOP_SEC_CONFIG_DATA_UNSPECIFIED
#endif

#endif /* CRYIF_MEMMAP_H */
