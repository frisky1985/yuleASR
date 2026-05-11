/**
 * @file SoAd_MemMap.h
 * @brief Memory mapping for SoAd module
 * @version 4.7.0
 * @date 2026-04-29
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef SOAD_MEMMAP_H
#define SOAD_MEMMAP_H

/*==================================================================================================
*                                    MEMORY SECTION MAPPING
==================================================================================================*/

/* Code section */
#ifdef SOAD_START_SEC_CODE
    #undef SOAD_START_SEC_CODE
    #undef SOAD_STOP_SEC_CODE
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CODE_ASIL_B
    #undef SOAD_START_SEC_CODE_ASIL_B
    #undef SOAD_STOP_SEC_CODE_ASIL_B
    #define MEMMAP_ERROR
#endif

/* Const data section */
#ifdef SOAD_START_SEC_CONST_UNSPECIFIED
    #undef SOAD_START_SEC_CONST_UNSPECIFIED
    #undef SOAD_STOP_SEC_CONST_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CONST_8BIT
    #undef SOAD_START_SEC_CONST_8BIT
    #undef SOAD_STOP_SEC_CONST_8BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CONST_16BIT
    #undef SOAD_START_SEC_CONST_16BIT
    #undef SOAD_STOP_SEC_CONST_16BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CONST_32BIT
    #undef SOAD_START_SEC_CONST_32BIT
    #undef SOAD_STOP_SEC_CONST_32BIT
    #define MEMMAP_ERROR
#endif

/* Data section */
#ifdef SOAD_START_SEC_VAR_INIT_UNSPECIFIED
    #undef SOAD_START_SEC_VAR_INIT_UNSPECIFIED
    #undef SOAD_STOP_SEC_VAR_INIT_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef SOAD_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef SOAD_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_CLEARED_BOOLEAN
    #undef SOAD_START_SEC_VAR_CLEARED_BOOLEAN
    #undef SOAD_STOP_SEC_VAR_CLEARED_BOOLEAN
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_CLEARED_8BIT
    #undef SOAD_START_SEC_VAR_CLEARED_8BIT
    #undef SOAD_STOP_SEC_VAR_CLEARED_8BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_CLEARED_16BIT
    #undef SOAD_START_SEC_VAR_CLEARED_16BIT
    #undef SOAD_STOP_SEC_VAR_CLEARED_16BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_CLEARED_32BIT
    #undef SOAD_START_SEC_VAR_CLEARED_32BIT
    #undef SOAD_STOP_SEC_VAR_CLEARED_32BIT
    #define MEMMAP_ERROR
#endif

/* Configuration data section */
#ifdef SOAD_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef SOAD_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef SOAD_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CONFIG_DATA_8BIT
    #undef SOAD_START_SEC_CONFIG_DATA_8BIT
    #undef SOAD_STOP_SEC_CONFIG_DATA_8BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CONFIG_DATA_16BIT
    #undef SOAD_START_SEC_CONFIG_DATA_16BIT
    #undef SOAD_STOP_SEC_CONFIG_DATA_16BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_CONFIG_DATA_32BIT
    #undef SOAD_START_SEC_CONFIG_DATA_32BIT
    #undef SOAD_STOP_SEC_CONFIG_DATA_32BIT
    #define MEMMAP_ERROR
#endif

/* Buffer section */
#ifdef SOAD_START_SEC_VAR_NO_INIT_8BIT
    #undef SOAD_START_SEC_VAR_NO_INIT_8BIT
    #undef SOAD_STOP_SEC_VAR_NO_INIT_8BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_NO_INIT_16BIT
    #undef SOAD_START_SEC_VAR_NO_INIT_16BIT
    #undef SOAD_STOP_SEC_VAR_NO_INIT_16BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_NO_INIT_32BIT
    #undef SOAD_START_SEC_VAR_NO_INIT_32BIT
    #undef SOAD_STOP_SEC_VAR_NO_INIT_32BIT
    #define MEMMAP_ERROR
#endif

#ifdef SOAD_START_SEC_VAR_NO_INIT_UNSPECIFIED
    #undef SOAD_START_SEC_VAR_NO_INIT_UNSPECIFIED
    #undef SOAD_STOP_SEC_VAR_NO_INIT_UNSPECIFIED
    #define MEMMAP_ERROR
#endif

/* Error check */
#ifdef MEMMAP_ERROR
    #error "SoAd_MemMap.h: Invalid memory section directive"
    #undef MEMMAP_ERROR
#endif

#endif /* SOAD_MEMMAP_H */
