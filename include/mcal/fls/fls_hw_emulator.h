/**
 * @file fls_hw_emulator.h
 * @brief Fls Hardware Emulator Header
 * @version 1.0.0
 * @date 2026
 */

#ifndef FLS_HW_EMULATOR_H
#define FLS_HW_EMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fls_types.h"

/* Public interface */
Fls_ErrorCode_t Fls_Emu_Register(void);
Fls_ErrorCode_t Fls_Emu_Dump(uint32_t address, uint32_t length);
Fls_ErrorCode_t Fls_Emu_GetStats(uint32_t* totalReads, uint32_t* totalWrites,
                                 uint32_t* totalErases, uint32_t* maxEraseCount);
uint8_t* Fls_Emu_GetMemoryPtr(void);
uint32_t Fls_Emu_GetFlashSize(void);

#ifdef __cplusplus
}
#endif

#endif /* FLS_HW_EMULATOR_H */
