/**
 * @file Reg_Macros.h
 * @brief Register Access Macros - stub for compilation
 */
#ifndef REG_MACROS_H
#define REG_MACROS_H

/* 32-bit register access */
#define REG32(addr)                 (*(volatile uint32*)(uint32)(addr))
#define REG16(addr)                 (*(volatile uint16*)(uint32)(addr))
#define REG8(addr)                  (*(volatile uint8*)(uint32)(addr))

/* Bit operations */
#define BIT_MASK(pos)               (1u << (pos))
#define BIT_MASK32(pos)             (1UL << (pos))

#define SET_BIT(reg, bit)           ((reg) |= (1u << (bit)))
#define CLR_BIT(reg, bit)           ((reg) &= ~(1u << (bit)))
#define GET_BIT(reg, bit)           (((reg) >> (bit)) & 1u)
#define TOGGLE_BIT(reg, bit)        ((reg) ^= (1u << (bit)))

#define SET_REG_FIELD(reg, mask, offset, val) \
    ((reg) = ((reg) & ~((mask) << (offset))) | (((val) & (mask)) << (offset)))
#define GET_REG_FIELD(reg, mask, offset) \
    (((reg) >> (offset)) & (mask))

#define SET_REG32(reg, mask, val)   ((reg) = ((reg) & ~(uint32)(mask)) | ((uint32)(val) & (uint32)(mask)))

/* Register read/write with barrier */
#define REG_WRITE(addr, val)        do { REG32(addr) = (val); __asm__ volatile("dsb" : : : "memory"); } while(0)
#define REG_READ(addr)              REG32(addr)
#define REG_SET_BITS(addr, bits)    do { REG32(addr) |= (bits); __asm__ volatile("dsb" : : : "memory"); } while(0)
#define REG_CLR_BITS(addr, bits)    do { REG32(addr) &= ~(bits); __asm__ volatile("dsb" : : : "memory"); } while(0)

/* Wait for bit */
#define WAIT_BIT_SET(reg, bit, timeout) \
    do { uint32 _wait_ = 0; while (!GET_BIT((reg), (bit)) && (_wait_ < (timeout))) { _wait_++; } } while(0)
#define WAIT_BIT_CLR(reg, bit, timeout) \
    do { uint32 _wait_ = 0; while (GET_BIT((reg), (bit)) && (_wait_ < (timeout))) { _wait_++; } } while(0)

#endif /* REG_MACROS_H */
