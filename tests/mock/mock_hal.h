/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Mock HAL — Hardware Register Memory Table
*
* SW Version           : 1.0.0
* Build Date           : 2026-07-26
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file mock_hal.h
* @brief Mock HAL register access for macOS unit testing of MCAL modules
* @details Provides a memory-backed register table that intercepts REG_READ32/REG_WRITE32
*          calls. Designed to allow real MCAL production source code to compile and
*          run on macOS with zero hardware dependencies.
*
* Usage (compile-time macro replacement):
*   -DREG_READ32=mock_hal_read32 -DREG_WRITE32=mock_hal_write32
*   -DREG_READ16=mock_hal_read16 -DREG_WRITE16=mock_hal_write16
*
* or include this header before <Std_Types.h>:
*   #include "test/mock/mock_hal.h"
==================================================================================================*/

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>

/*==================================================================================================
*                                      COMPILE-TIME MACROS
*                                      (redirect REG_READ32/REG_WRITE32 to mock)
==================================================================================================*/

#ifndef REG_READ32
#define REG_READ32(addr)                mock_hal_read32((uintptr_t)(addr))
#endif
#ifndef REG_WRITE32
#define REG_WRITE32(addr, val)          mock_hal_write32((uintptr_t)(addr), (uint32_t)(val))
#endif

#ifndef REG_READ16
#define REG_READ16(addr)                mock_hal_read16((uintptr_t)(addr))
#endif
#ifndef REG_WRITE16
#define REG_WRITE16(addr, val)          mock_hal_write16((uintptr_t)(addr), (uint16_t)(val))
#endif

#ifndef REG_READ8
#define REG_READ8(addr)                 mock_hal_read8((uintptr_t)(addr))
#endif
#ifndef REG_WRITE8
#define REG_WRITE8(addr, val)           mock_hal_write8((uintptr_t)(addr), (uint8_t)(val))
#endif

/* Convenience RMW — relies on redirected REG_READ32/REG_WRITE32 */
#ifndef REG_RMW32
#define REG_RMW32(addr, mask, val)      REG_WRITE32((addr), (REG_READ32(addr) & ~(uint32_t)(mask)) | ((uint32_t)(val) & (uint32_t)(mask)))
#endif

/*==================================================================================================
*                                      MEMORY REGISTER TABLE
*
*  1M entries (4 MB RAM) covering 4 MB of register address space.
*  Index: (addr >> 2) & 0xFFFFF   (use low 20 bits of address word-index)
*
*  Sufficient for the i.MX8M Mini peripheral address range
*  (0x3020_0000–0x308C_0000 = 7 MB span, with no 4 MB aliasing between modules).
*
*  Aliasing-free check (any pair differs by a multiple of 4 MB = 0x400000):
*    DIO/GPIO base   0x30200000  -> idx = 0x80000
*    GPT1 base       0x302C0000  -> idx = 0xB0000
*    PWM1 base       0x30660000  -> idx = 0x198000
*    CAN1 base       0x308C0000  -> idx = 0x30000
*    Port/SIUL2      0x40049000  -> idx = 0x124000
*    WDOG base       0x40052000  -> idx = 0x148000
*    PCC base        0x40065000  -> idx = 0x194000
*    ADC base        0x4003B000  -> idx = 0xEC000
*    FTM0 base       0x40038000  -> idx = 0xE0000
*    PIT base        0x40037000  -> idx = 0xDC000
*    LPSPI0 base     0x4002C000  -> idx = 0xB000
*
*  No two modules alias under the 0xFFFFF mask.
==================================================================================================*/
#define MOCK_HAL_TABLE_SIZE             1048576U  /* 2^20 = 1M entries */
#define MOCK_HAL_ADDR_MASK              (MOCK_HAL_TABLE_SIZE - 1U)

/*==================================================================================================
*                                      PUBLIC API
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read 32-bit value from mock register table
 * @param addr  Register address
 * @return Stored value, or 0 if address was never written
 */
uint32_t mock_hal_read32(uintptr_t addr);

/**
 * @brief Write 32-bit value to mock register table
 * @param addr  Register address
 * @param val   Value to store
 */
void mock_hal_write32(uintptr_t addr, uint32_t val);

/**
 * @brief Read 16-bit value from mock register table
 */
uint16_t mock_hal_read16(uintptr_t addr);

/**
 * @brief Write 16-bit value to mock register table
 */
void mock_hal_write16(uintptr_t addr, uint16_t val);

/**
 * @brief Read 8-bit value from mock register table
 */
uint8_t mock_hal_read8(uintptr_t addr);

/**
 * @brief Write 8-bit value to mock register table
 */
void mock_hal_write8(uintptr_t addr, uint8_t val);

/**
 * @brief Reset all registers to zero, clear statistics and expectations
 */
void mock_hal_reset(void);

/**
 * @brief Pre-set a default value for a register address
 * @param addr  Register address
 * @param val   Default value (survives reset)
 */
void mock_hal_set_default(uintptr_t addr, uint32_t val);

/**
 * @brief Batch-set multiple defaults from a config table
 * @param table  Pointer to an array of {addr, value} pairs
 * @param count  Number of entries
 */
void mock_hal_set_defaults(const uint32_t (*table)[2], uint32_t count);

/**
 * @brief Register an expected write (addr, value)
 * @param addr  Register address
 * @param val   Expected value
 */
void mock_hal_expect_write(uintptr_t addr, uint32_t val);

/**
 * @brief Verify all expectations were met
 * @return true if all expected writes occurred, false otherwise
 */
bool mock_hal_verify(void);

/**
 * @brief Get read count for a register address
 */
uint32_t mock_hal_read_count(uintptr_t addr);

/**
 * @brief Get write count for a register address
 */
uint32_t mock_hal_write_count(uintptr_t addr);

/**
 * @brief Get total read count
 */
uint32_t mock_hal_total_reads(void);

/**
 * @brief Get total write count
 */
uint32_t mock_hal_total_writes(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_HAL_H */
