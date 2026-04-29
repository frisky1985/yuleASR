/**
 * @file MemMap.h
 * @brief Memory mapping header for AUTOSAR BSW modules
 * @version 1.0.0
 * 
 * This file provides memory section pragmas for code and data placement.
 * In a real implementation, these would map to specific memory sections.
 * 
 * @copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef MEMMAP_H
#define MEMMAP_H

/*==================================================================================================
*                              CODE SECTIONS
==================================================================================================*/

/* Eth Module Code Sections */
#ifdef ETH_START_SEC_CODE
    #undef ETH_START_SEC_CODE
    /* Start of code section */
#endif

#ifdef ETH_STOP_SEC_CODE
    #undef ETH_STOP_SEC_CODE
    /* End of code section */
#endif

/* Eth Module Variable Sections */
#ifdef ETH_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef ETH_START_SEC_VAR_CLEARED_UNSPECIFIED
    /* Start of cleared variable section */
#endif

#ifdef ETH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef ETH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    /* End of cleared variable section */
#endif

/* Eth Module Configuration Data Sections */
#ifdef ETH_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef ETH_START_SEC_CONFIG_DATA_UNSPECIFIED
    /* Start of configuration data section */
#endif

#ifdef ETH_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef ETH_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    /* End of configuration data section */
#endif

/* DET Module Code Sections */
#ifdef DET_START_SEC_CODE
    #undef DET_START_SEC_CODE
#endif

#ifdef DET_STOP_SEC_CODE
    #undef DET_STOP_SEC_CODE
#endif

#endif /* MEMMAP_H */
