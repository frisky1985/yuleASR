/**
 * @file Mcu_Reg.h
 * @brief MCU Register Definitions - stub for compilation
 */
#ifndef MCU_REG_H
#define MCU_REG_H

#include "Std_Types.h"
#include "Platform_Types.h"

/* MCU base addresses */
#define MCU_GPC_BASE_ADDR               (0x303A0000UL)
#define MCU_CCM_BASE_ADDR               (0x30380000UL)
#define MCU_SRC_BASE_ADDR               (0x30390000UL)

/* GPC registers */
#define MCU_GPC_PGC_CPU_MAPPING         (MCU_GPC_BASE_ADDR + 0x0EC)
#define MCU_GPC_PU_PGC_SW_PUP_REQ       (MCU_GPC_BASE_ADDR + 0x0F8)
#define MCU_GPC_PU_PGC_SW_PDN_REQ       (MCU_GPC_BASE_ADDR + 0x104)

/* CCM registers */
#define MCU_CCM_CCR                     (MCU_CCM_BASE_ADDR + 0x0000)
#define MCU_CCM_CSR                     (MCU_CCM_BASE_ADDR + 0x0008)
#define MCU_CCM_CCSR                    (MCU_CCM_BASE_ADDR + 0x000C)
#define MCU_CCM_CACRR                   (MCU_CCM_BASE_ADDR + 0x0010)
#define MCU_CCM_CBCDR                   (MCU_CCM_BASE_ADDR + 0x0014)
#define MCU_CCM_CBCMR                   (MCU_CCM_BASE_ADDR + 0x0018)

/* SRC registers */
#define MCU_SRC_SCR                     (MCU_SRC_BASE_ADDR + 0x0000)
#define MCU_SRC_SRSR                    (MCU_SRC_BASE_ADDR + 0x0004)
#define MCU_SRC_SBMR1                   (MCU_SRC_BASE_ADDR + 0x0008)
#define MCU_SRC_SBMR2                   (MCU_SRC_BASE_ADDR + 0x001C)

/* Register macros */
#define REG32(addr)                     (*(volatile uint32*)(addr))
#define REG16(addr)                     (*(volatile uint16*)(addr))
#define REG8(addr)                      (*(volatile uint8*)(addr))

/* Register bit macros */
#define SET_BIT(reg, bit)               ((reg) |= (1u << (bit)))
#define CLR_BIT(reg, bit)               ((reg) &= ~(1u << (bit)))
#define GET_BIT(reg, bit)               (((reg) >> (bit)) & 1u)
#define SET_REG(reg, mask, val)         ((reg) = ((reg) & ~(mask)) | ((val) & (mask)))
#define GET_REG(reg, mask)              ((reg) & (mask))
#define WAIT_BIT_SET(reg, bit, timeout) \
    do { uint32 _wait = 0; while (!GET_BIT((reg), (bit)) && (_wait < (timeout))) { _wait++; } } while(0)
#define WAIT_BIT_CLR(reg, bit, timeout) \
    do { uint32 _wait = 0; while (GET_BIT((reg), (bit)) && (_wait < (timeout))) { _wait++; } } while(0)

#endif /* MCU_REG_H */
