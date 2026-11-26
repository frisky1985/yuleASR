/*==================================================================================================
 * Reg_Macros.h - S32K312 register access macros (yuleASR platform)
 *
 * Provides the common register read/write primitives used by platform code.
 *
 * When YULE_USE_NXP_RTD is defined (target build), the NXP RTD official
 * register access macros from S32K312_COMMON.h are used. Otherwise, the
 * built-in fallback macros are used for host-based simulation and testing.
 *================================================================================================*/
#ifndef REG_MACROS_H
#define REG_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

#if defined(YULE_USE_NXP_RTD)
/* Use NXP RTD official register access macros */
#include "S32K312_COMMON.h"
#endif

/* Basic register access macros (fallback if not provided by RTD) */
#ifndef REG8
#define REG8(addr)      (*((volatile uint8*)(addr)))
#endif
#ifndef REG16
#define REG16(addr)     (*((volatile uint16*)(addr)))
#endif
#ifndef REG32
#define REG32(addr)     (*((volatile uint32*)(addr)))
#endif

/* Bit manipulation helpers */
#ifndef BIT_MASK
#define BIT_MASK(pos)   (1UL << (pos))
#endif
#ifndef SET_BIT
#define SET_BIT(reg, pos)   ((reg) |= BIT_MASK(pos))
#endif
#ifndef CLEAR_BIT
#define CLEAR_BIT(reg, pos) ((reg) &= ~BIT_MASK(pos))
#endif
#ifndef GET_BIT
#define GET_BIT(reg, pos)   (((reg) >> (pos)) & 1U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* REG_MACROS_H */
