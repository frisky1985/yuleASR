/**
 * @file Xcp_MemMap.h
 * @brief XCP Memory Mapping File
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef XCP_MEMMAP_H
#define XCP_MEMMAP_H

/*==================================================================================================
*                                          START SECTIONS
==================================================================================================*/

#ifdef XCP_START_SEC_VAR_INIT_UNSPECIFIED
    #undef XCP_START_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_START_SEC_VAR_NOINIT_UNSPECIFIED
    #undef XCP_START_SEC_VAR_NOINIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef XCP_START_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_START_SEC_CONST_UNSPECIFIED
    #undef XCP_START_SEC_CONST_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_START_SEC_CODE
    #undef XCP_START_SEC_CODE
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef XCP_START_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

/*==================================================================================================
*                                          STOP SECTIONS
==================================================================================================*/

#ifdef XCP_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef XCP_STOP_SEC_VAR_INIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    #undef XCP_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef XCP_STOP_SEC_VAR_CLEARED_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_STOP_SEC_CONST_UNSPECIFIED
    #undef XCP_STOP_SEC_CONST_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_STOP_SEC_CODE
    #undef XCP_STOP_SEC_CODE
    #undef MEMMAP_ERROR
#endif

#ifdef XCP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef XCP_STOP_SEC_CONFIG_DATA_UNSPECIFIED
    #undef MEMMAP_ERROR
#endif

#endif /* XCP_MEMMAP_H */
