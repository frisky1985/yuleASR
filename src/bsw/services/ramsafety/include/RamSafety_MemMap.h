/*==================================================================================================
 * RamSafety_MemMap.h - memory mapping for RamSafety module
 *
 * All sections map to their defaults (no special memory placement).
 * @file RamSafety_MemMap.h
 * @brief RamSafety memory mapping
 *================================================================================================*/
#ifndef RAMSAFETY_MEMMAP_H
#define RAMSAFETY_MEMMAP_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif


#ifndef RAMSAFETY_START_SEC_CODE
#define RAMSAFETY_START_SEC_CODE
#endif
#ifndef RAMSAFETY_STOP_SEC_CODE
#define RAMSAFETY_STOP_SEC_CODE
#endif
#ifndef RAMSAFETY_START_SEC_CONST_UNSPECIFIED
#define RAMSAFETY_START_SEC_CONST_UNSPECIFIED
#endif
#ifndef RAMSAFETY_STOP_SEC_CONST_UNSPECIFIED
#define RAMSAFETY_STOP_SEC_CONST_UNSPECIFIED
#endif
#ifndef RAMSAFETY_START_SEC_VAR_CLEARED
#define RAMSAFETY_START_SEC_VAR_CLEARED
#endif
#ifndef RAMSAFETY_STOP_SEC_VAR_CLEARED
#define RAMSAFETY_STOP_SEC_VAR_CLEARED
#endif
#ifndef RAMSAFETY_START_SEC_VAR_NO_INIT
#define RAMSAFETY_START_SEC_VAR_NO_INIT
#endif
#ifndef RAMSAFETY_STOP_SEC_VAR_NO_INIT
#define RAMSAFETY_STOP_SEC_VAR_NO_INIT
#endif


#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif /* RAMSAFETY_MEMMAP_H */
