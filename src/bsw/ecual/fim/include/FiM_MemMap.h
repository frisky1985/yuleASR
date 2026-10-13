/*==================================================================================================
 * FiM_MemMap.h - memory mapping for FiM module
 *
 * All sections map to their defaults (no special memory placement).
 * @file FiM_MemMap.h
 * @brief FiM memory mapping
 *================================================================================================*/
#ifndef FIM_MEMMAP_H
#define FIM_MEMMAP_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif


#ifndef FIM_START_SEC_CODE
#define FIM_START_SEC_CODE
#endif
#ifndef FIM_STOP_SEC_CODE
#define FIM_STOP_SEC_CODE
#endif
#ifndef FIM_START_SEC_CONST_UNSPECIFIED
#define FIM_START_SEC_CONST_UNSPECIFIED
#endif
#ifndef FIM_STOP_SEC_CONST_UNSPECIFIED
#define FIM_STOP_SEC_CONST_UNSPECIFIED
#endif
#ifndef FIM_START_SEC_VAR_CLEARED
#define FIM_START_SEC_VAR_CLEARED
#endif
#ifndef FIM_STOP_SEC_VAR_CLEARED
#define FIM_STOP_SEC_VAR_CLEARED
#endif
#ifndef FIM_START_SEC_VAR_NO_INIT
#define FIM_START_SEC_VAR_NO_INIT
#endif
#ifndef FIM_STOP_SEC_VAR_NO_INIT
#define FIM_STOP_SEC_VAR_NO_INIT
#endif


#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif /* FIM_MEMMAP_H */
