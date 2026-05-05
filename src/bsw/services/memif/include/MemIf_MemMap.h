/*==================================================================================================
 *                                MEMORY INTERFACE (MemIf)
 *==================================================================================================
 * FILENAME: MemIf_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping header file for Memory Interface module
 *==================================================================================================
 */

#ifndef MEMIF_MEMMAP_H
#define MEMIF_MEMMAP_H

/*==================================================================================================
 *                                    MEMORY SECTION MAPPING
 *==================================================================================================*/

#ifdef MEMIF_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEMIF_START_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section ".bss.MemIf"
#endif

#ifdef MEMIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEMIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section
#endif

#ifdef MEMIF_START_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMIF_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.MemIf"
#endif

#ifdef MEMIF_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMIF_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
#endif

#ifdef MEMIF_START_SEC_CONST_UNSPECIFIED
    #undef MEMIF_START_SEC_CONST_UNSPECIFIED
    #pragma section ".rodata.MemIf"
#endif

#ifdef MEMIF_STOP_SEC_CONST_UNSPECIFIED
    #undef MEMIF_STOP_SEC_CONST_UNSPECIFIED
    #pragma section
#endif

#ifdef MEMIF_START_SEC_CODE
    #undef MEMIF_START_SEC_CODE
    #pragma section ".text.MemIf"
#endif

#ifdef MEMIF_STOP_SEC_CODE
    #undef MEMIF_STOP_SEC_CODE
    #pragma section
#endif

#ifdef MEMIF_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMIF_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.MemIf.Config"
#endif

#ifdef MEMIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
#endif

#endif /* MEMIF_MEMMAP_H */
