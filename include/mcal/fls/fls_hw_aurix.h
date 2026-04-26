/**
 * @file fls_hw_aurix.h
 * @brief Fls Hardware Driver for Infineon Aurix TC3xx
 * @version 1.0.0
 * @date 2026
 */

#ifndef FLS_HW_AURIX_H
#define FLS_HW_AURIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fls_types.h"

/* Public interface */
Fls_ErrorCode_t Fls_Tc3xx_Register(void);
void Fls_Tc3xx_SetEmulationMode(bool enable);
bool Fls_Tc3xx_GetEmulationMode(void);
uint32_t Fls_Tc3xx_GetPflashSize(void);
uint32_t Fls_Tc3xx_GetDflashSize(void);

#ifdef __cplusplus
}
#endif

#endif /* FLS_HW_AURIX_H */
