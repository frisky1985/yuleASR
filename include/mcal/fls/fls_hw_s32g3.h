/**
 * @file fls_hw_s32g3.h
 * @brief Fls Hardware Driver for NXP S32G3
 * @version 1.0.0
 * @date 2026
 */

#ifndef FLS_HW_S32G3_H
#define FLS_HW_S32G3_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fls_types.h"

/* Public interface */
Fls_ErrorCode_t Fls_S32g3_Register(void);
void Fls_S32g3_SetEmulationMode(bool enable);
bool Fls_S32g3_GetEmulationMode(void);
uint32_t Fls_S32g3_GetFlashSize(void);

#ifdef __cplusplus
}
#endif

#endif /* FLS_HW_S32G3_H */
