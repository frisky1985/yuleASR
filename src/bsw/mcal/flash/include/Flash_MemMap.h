/*==================================================================================================
 *                                      FLASH DRIVER MEMORY MAP
 *==================================================================================================
 * FILENAME: Flash_MemMap.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Memory mapping for Flash Driver module (MCAL Layer)
 *==================================================================================================
 */

#ifndef FLASH_MEMMAP_H
#define FLASH_MEMMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                    MEMORY SECTION MAPPING
 *==================================================================================================*/

#if defined(FLASH_START_SEC_CODE)
    #undef FLASH_START_SEC_CODE
    #pragma section ".text.Flash" ax
#elif defined(FLASH_STOP_SEC_CODE)
    #undef FLASH_STOP_SEC_CODE
    #pragma section

#elif defined(FLASH_START_SEC_CODE_FAST)
    #undef FLASH_START_SEC_CODE_FAST
    #pragma section ".text.Flash.Fast" ax
#elif defined(FLASH_STOP_SEC_CODE_FAST)
    #undef FLASH_STOP_SEC_CODE_FAST
    #pragma section

#elif defined(FLASH_START_SEC_CODE_SLOW)
    #undef FLASH_START_SEC_CODE_SLOW
    #pragma section ".text.Flash.Slow" ax
#elif defined(FLASH_STOP_SEC_CODE_SLOW)
    #undef FLASH_STOP_SEC_CODE_SLOW
    #pragma section

#elif defined(FLASH_START_SEC_CONST_8)
    #undef FLASH_START_SEC_CONST_8
    #pragma section ".rodata.Flash" a
#elif defined(FLASH_STOP_SEC_CONST_8)
    #undef FLASH_STOP_SEC_CONST_8
    #pragma section

#elif defined(FLASH_START_SEC_CONST_16)
    #undef FLASH_START_SEC_CONST_16
    #pragma section ".rodata.Flash" a
#elif defined(FLASH_STOP_SEC_CONST_16)
    #undef FLASH_STOP_SEC_CONST_16
    #pragma section

#elif defined(FLASH_START_SEC_CONST_32)
    #undef FLASH_START_SEC_CONST_32
    #pragma section ".rodata.Flash" a
#elif defined(FLASH_STOP_SEC_CONST_32)
    #undef FLASH_STOP_SEC_CONST_32
    #pragma section

#elif defined(FLASH_START_SEC_CONST_UNSPECIFIED)
    #undef FLASH_START_SEC_CONST_UNSPECIFIED
    #pragma section ".rodata.Flash" a
#elif defined(FLASH_STOP_SEC_CONST_UNSPECIFIED)
    #undef FLASH_STOP_SEC_CONST_UNSPECIFIED
    #pragma section

#elif defined(FLASH_START_SEC_VAR_CLEARED_8)
    #undef FLASH_START_SEC_VAR_CLEARED_8
    #pragma section ".bss.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_CLEARED_8)
    #undef FLASH_STOP_SEC_VAR_CLEARED_8
    #pragma section

#elif defined(FLASH_START_SEC_VAR_CLEARED_16)
    #undef FLASH_START_SEC_VAR_CLEARED_16
    #pragma section ".bss.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_CLEARED_16)
    #undef FLASH_STOP_SEC_VAR_CLEARED_16
    #pragma section

#elif defined(FLASH_START_SEC_VAR_CLEARED_32)
    #undef FLASH_START_SEC_VAR_CLEARED_32
    #pragma section ".bss.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_CLEARED_32)
    #undef FLASH_STOP_SEC_VAR_CLEARED_32
    #pragma section

#elif defined(FLASH_START_SEC_VAR_CLEARED_UNSPECIFIED)
    #undef FLASH_START_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section ".bss.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_CLEARED_UNSPECIFIED)
    #undef FLASH_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #pragma section

#elif defined(FLASH_START_SEC_VAR_INIT_8)
    #undef FLASH_START_SEC_VAR_INIT_8
    #pragma section ".data.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_INIT_8)
    #undef FLASH_STOP_SEC_VAR_INIT_8
    #pragma section

#elif defined(FLASH_START_SEC_VAR_INIT_16)
    #undef FLASH_START_SEC_VAR_INIT_16
    #pragma section ".data.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_INIT_16)
    #undef FLASH_STOP_SEC_VAR_INIT_16
    #pragma section

#elif defined(FLASH_START_SEC_VAR_INIT_32)
    #undef FLASH_START_SEC_VAR_INIT_32
    #pragma section ".data.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_INIT_32)
    #undef FLASH_STOP_SEC_VAR_INIT_32
    #pragma section

#elif defined(FLASH_START_SEC_VAR_INIT_UNSPECIFIED)
    #undef FLASH_START_SEC_VAR_INIT_UNSPECIFIED
    #pragma section ".data.Flash" aw
#elif defined(FLASH_STOP_SEC_VAR_INIT_UNSPECIFIED)
    #undef FLASH_STOP_SEC_VAR_INIT_UNSPECIFIED
    #pragma section

#elif defined(FLASH_START_SEC_VAR_NO_INIT_8)
    #undef FLASH_START_SEC_VAR_NO_INIT_8
    #pragma section ".bss.Flash.NoInit" aw
#elif defined(FLASH_STOP_SEC_VAR_NO_INIT_8)
    #undef FLASH_STOP_SEC_VAR_NO_INIT_8
    #pragma section

#elif defined(FLASH_START_SEC_VAR_NO_INIT_16)
    #undef FLASH_START_SEC_VAR_NO_INIT_16
    #pragma section ".bss.Flash.NoInit" aw
#elif defined(FLASH_STOP_SEC_VAR_NO_INIT_16)
    #undef FLASH_STOP_SEC_VAR_NO_INIT_16
    #pragma section

#elif defined(FLASH_START_SEC_VAR_NO_INIT_32)
    #undef FLASH_START_SEC_VAR_NO_INIT_32
    #pragma section ".bss.Flash.NoInit" aw
#elif defined(FLASH_STOP_SEC_VAR_NO_INIT_32)
    #undef FLASH_STOP_SEC_VAR_NO_INIT_32
    #pragma section

#elif defined(FLASH_START_SEC_VAR_NO_INIT_UNSPECIFIED)
    #undef FLASH_START_SEC_VAR_NO_INIT_UNSPECIFIED
    #pragma section ".bss.Flash.NoInit" aw
#elif defined(FLASH_STOP_SEC_VAR_NO_INIT_UNSPECIFIED)
    #undef FLASH_STOP_SEC_VAR_NO_INIT_UNSPECIFIED
    #pragma section

#elif defined(FLASH_START_SEC_CALIB_8)
    #undef FLASH_START_SEC_CALIB_8
    #pragma section ".rodata.Flash.Calib" a
#elif defined(FLASH_STOP_SEC_CALIB_8)
    #undef FLASH_STOP_SEC_CALIB_8
    #pragma section

#elif defined(FLASH_START_SEC_CALIB_16)
    #undef FLASH_START_SEC_CALIB_16
    #pragma section ".rodata.Flash.Calib" a
#elif defined(FLASH_STOP_SEC_CALIB_16)
    #undef FLASH_STOP_SEC_CALIB_16
    #pragma section

#elif defined(FLASH_START_SEC_CALIB_32)
    #undef FLASH_START_SEC_CALIB_32
    #pragma section ".rodata.Flash.Calib" a
#elif defined(FLASH_STOP_SEC_CALIB_32)
    #undef FLASH_STOP_SEC_CALIB_32
    #pragma section

#elif defined(FLASH_START_SEC_CALIB_UNSPECIFIED)
    #undef FLASH_START_SEC_CALIB_UNSPECIFIED
    #pragma section ".rodata.Flash.Calib" a
#elif defined(FLASH_STOP_SEC_CALIB_UNSPECIFIED)
    #undef FLASH_STOP_SEC_CALIB_UNSPECIFIED
    #pragma section

#else
    /* No section defined - issue warning */
    #error "Flash_MemMap.h: No valid memory section defined"
#endif

#ifdef __cplusplus
}
#endif

#endif /* FLASH_MEMMAP_H */
