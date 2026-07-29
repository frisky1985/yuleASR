/**
 * @file Mcal.h
 * @brief MCAL Abstraction Header - stub for compilation
 */
#ifndef MCAL_H
#define MCAL_H

#include "Std_Types.h"
#include "Platform_Types.h"

/* MCAL wrapper functions */
extern void Mcal_DisableAllInterrupts(void);
extern void Mcal_EnableAllInterrupts(void);
extern void Mcal_ResetSystem(void);
extern void Mcal_MemCopy(void* dest, const void* src, uint32 size);

/* Inline stubs */
static inline void Mcal_DisableAllInterrupts(void) { }
static inline void Mcal_EnableAllInterrupts(void) { }
static inline void Mcal_ResetSystem(void) { }
static inline void Mcal_MemCopy(void* dest, const void* src, uint32 size) {
    uint32 i;
    for (i = 0; i < size; i++) {
        ((uint8*)dest)[i] = ((const uint8*)src)[i];
    }
}

#endif /* MCAL_H */
