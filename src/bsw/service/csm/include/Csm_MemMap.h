/*==================================================================================================
 *                                CRYPTO SERVICES MANAGER (Csm)
 *==================================================================================================
 * FILENAME: Csm_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping header file for Crypto Services Manager module
 *==================================================================================================
 */

#ifndef CSM_MEMMAP_H
#define CSM_MEMMAP_H

/*==================================================================================================
 *                                    MEMORY SECTION MAPPING
 *==================================================================================================*/

#ifdef CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CSM_START_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section ".bss.Csm"
#endif

#ifdef CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CSM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section
#endif

#ifdef CSM_START_SEC_VAR_INIT_UNSPECIFIED
    #undef CSM_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.Csm"
#endif

#ifdef CSM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef CSM_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
#endif

#ifdef CSM_START_SEC_CONST_UNSPECIFIED
    #undef CSM_START_SEC_CONST_UNSPECIFIED
    #pragma section ".rodata.Csm"
#endif

#ifdef CSM_STOP_SEC_CONST_UNSPECIFIED
    #undef CSM_STOP_SEC_CONST_UNSPECIFIED
    #pragma section
#endif

#ifdef CSM_START_SEC_CODE
    #undef CSM_START_SEC_CODE
    #pragma section ".text.Csm"
#endif

#ifdef CSM_STOP_SEC_CODE
    #undef CSM_STOP_SEC_CODE
    #pragma section
#endif

#ifdef CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CSM_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.Csm.Config"
#endif

#ifdef CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CSM_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
#endif

#endif /* CSM_MEMMAP_H */
