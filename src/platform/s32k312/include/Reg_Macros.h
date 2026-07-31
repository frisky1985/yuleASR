/*==================================================================================================
 * Reg_Macros.h - S32K312 register access macros (yuleASR platform)
 *
 * Provides the common register read/write primitives used by platform code.
 * The S32K312 platform layer currently defines its own per-module register
 * access macros; this header keeps the standard primitives available.
 *
 * TODO: Replace with SDK-provided register macros for production builds.
 *================================================================================================*/
#ifndef REG_MACROS_H
#define REG_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/* Basic register access macros */
#define REG8(addr)      (*((volatile uint8*)(addr)))
#define REG16(addr)     (*((volatile uint16*)(addr)))
#define REG32(addr)     (*((volatile uint32*)(addr)))

/* Bit manipulation helpers */
#define BIT_MASK(pos)   (1UL << (pos))
#define SET_BIT(reg, pos)   ((reg) |= BIT_MASK(pos))
#define CLEAR_BIT(reg, pos) ((reg) &= ~BIT_MASK(pos))
#define GET_BIT(reg, pos)   (((reg) >> (pos)) & 1U)

#ifdef __cplusplus
}
#endif

#endif /* REG_MACROS_H */
