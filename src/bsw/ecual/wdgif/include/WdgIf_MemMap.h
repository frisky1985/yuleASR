/** @file WdgIf_MemMap.h
 * @brief Watchdog Interface memory mapping
 */

#ifndef WDGIF_MEMMAP_H
#define WDGIF_MEMMAP_H

/*============================================================================
 *  CODE SECTIONS
 *===========================================================================*/

#ifdef WDGIF_START_SEC_CODE
    #pragma section ".text.WdgIf" ax
    #undef WDGIF_START_SEC_CODE
    #undef MEMMAP_ERROR
#endif

#ifdef WDGIF_STOP_SEC_CODE
    #pragma section
    #undef WDGIF_STOP_SEC_CODE
    #undef MEMMAP_ERROR
#endif

/*============================================================================
 *  CONFIGURATION DATA SECTIONS
 *===========================================================================*/

#ifdef WDGIF_START_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section ".rodata.WdgIf.cfg" a
    #undef WDGIF_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #pragma section
    #undef WDGIF_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

/*============================================================================
 *  VAR SECTIONS
 *===========================================================================*/

#ifdef WDGIF_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.WdgIf" aw
    #undef WDGIF_START_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGIF_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section
    #undef WDGIF_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGIF_START_SEC_VAR_NOINIT_UNSPECIFIED
    #pragma section ".bss.WdgIf" aw
    #undef WDGIF_START_SEC_VAR_NOINIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef WDGIF_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    #pragma section
    #undef WDGIF_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#endif /* WDGIF_MEMMAP_H */
