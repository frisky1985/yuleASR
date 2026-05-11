/*==================================================================================================
 *                              SECURE ONBOARD COMMUNICATION (SecOC)
 *==================================================================================================
 * FILENAME: SecOC_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping header file for Secure Onboard Communication module
 *==================================================================================================
 */

#ifndef SECOC_MEMMAP_H
#define SECOC_MEMMAP_H

/*==================================================================================================
 *                                    MEMORY SECTION MAPPING
 *==================================================================================================*/

#ifdef SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef SECOC_START_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section ".bss.SecOC"
#endif

#ifdef SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef SECOC_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section
#endif

#ifdef SECOC_START_SEC_VAR_INIT_UNSPECIFIED
    #undef SECOC_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.SecOC"
#endif

#ifdef SECOC_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef SECOC_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
#endif

#ifdef SECOC_START_SEC_CONST_UNSPECIFIED
    #undef SECOC_START_SEC_CONST_UNSPECIFIED
    #pragma section ".rodata.SecOC"
#endif

#ifdef SECOC_STOP_SEC_CONST_UNSPECIFIED
    #undef SECOC_STOP_SEC_CONST_UNSPECIFIED
    #pragma section
#endif

#ifdef SECOC_START_SEC_CODE
    #undef SECOC_START_SEC_CODE
    #pragma section ".text.SecOC"
#endif

#ifdef SECOC_STOP_SEC_CODE
    #undef SECOC_STOP_SEC_CODE
    #pragma section
#endif

#ifdef SECOC_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef SECOC_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.SecOC.Config"
#endif

#ifdef SECOC_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef SECOC_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
#endif

#endif /* SECOC_MEMMAP_H */
