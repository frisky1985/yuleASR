/**=================================================================================================
 * @file Crypto_MemMap.h
 * @brief Memory mapping for Crypto Driver
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 *==================================================================================================*/

#ifndef CRYPTO_MEMMAP_H
#define CRYPTO_MEMMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        START SECTIONS
 *==================================================================================================*/

#ifdef CRYPTO_START_SEC_CODE
    #undef CRYPTO_START_SEC_CODE
    #undef MEMMAP_ERROR
    /* Map to actual compiler/linker directives */
    #pragma section ".text.Crypto" ax
#endif

#ifdef CRYPTO_START_SEC_CONST_UNSPECIFIED
    #undef CRYPTO_START_SEC_CONST_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section ".rodata.Crypto" a
#endif

#ifdef CRYPTO_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CRYPTO_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section ".bss.Crypto" aw
#endif

#ifdef CRYPTO_START_SEC_VAR_INIT_UNSPECIFIED
    #undef CRYPTO_START_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section ".data.Crypto" aw
#endif

#ifdef CRYPTO_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CRYPTO_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section ".rodata.Crypto.Config" a
#endif

/*==================================================================================================
 *                                        STOP SECTIONS
 *==================================================================================================*/

#ifdef CRYPTO_STOP_SEC_CODE
    #undef CRYPTO_STOP_SEC_CODE
    #undef MEMMAP_ERROR
    #pragma section
#endif

#ifdef CRYPTO_STOP_SEC_CONST_UNSPECIFIED
    #undef CRYPTO_STOP_SEC_CONST_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section
#endif

#ifdef CRYPTO_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef CRYPTO_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section
#endif

#ifdef CRYPTO_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef CRYPTO_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section
#endif

#ifdef CRYPTO_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef CRYPTO_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
    #pragma section
#endif

/*==================================================================================================
 *                                        ERROR CHECK
 *==================================================================================================*/
#ifdef MEMMAP_ERROR
    #error "Crypto_MemMap.h: Wrong pragma command or unknown section name"
#endif

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_MEMMAP_H */
