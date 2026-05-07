/*==================================================================================================
 *                              MEMORY MAP
 *==================================================================================================
 * FILENAME: MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping header file
 *==================================================================================================
 */

#ifndef MEMMAP_H
#define MEMMAP_H

/*==================================================================================================
 *                                     SECTION PRAGMAS
 *==================================================================================================*/

/* VAR_CLEARED_UNSPECIFIED */
#ifdef START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef START_SEC_VAR_CLEARED_UNSPECIFIED
    /* #pragma section .bss */
#endif

#ifdef STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef STOP_SEC_VAR_CLEARED_UNSPECIFIED
    /* #pragma section */
#endif

/* VAR_INIT_UNSPECIFIED */
#ifdef START_SEC_VAR_INIT_UNSPECIFIED
    #undef START_SEC_VAR_INIT_UNSPECIFIED
    /* #pragma section .data */
#endif

#ifdef STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef STOP_SEC_VAR_INIT_UNSPECIFIED
    /* #pragma section */
#endif

/* CONST_UNSPECIFIED */
#ifdef START_SEC_CONST_UNSPECIFIED
    #undef START_SEC_CONST_UNSPECIFIED
    /* #pragma section .rodata */
#endif

#ifdef STOP_SEC_CONST_UNSPECIFIED
    #undef STOP_SEC_CONST_UNSPECIFIED
    /* #pragma section */
#endif

/* CODE */
#ifdef START_SEC_CODE
    #undef START_SEC_CODE
    /* #pragma section .text */
#endif

#ifdef STOP_SEC_CODE
    #undef STOP_SEC_CODE
    /* #pragma section */
#endif

/* CONFIG_DATA_UNSPECIFIED */
#ifdef START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef START_SEC_CONFIG_DATA_UNSPECIFIED
    /* #pragma section .rodata */
#endif

#ifdef STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef STOP_SEC_CONFIG_DATA_UNSPECIFIED
    /* #pragma section */
#endif

#endif /* MEMMAP_H */
