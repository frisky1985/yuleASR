/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Hardware Register Mock
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-15
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file mock_registers.h
* @brief Hardware register mock for unit testing
* @details Provides mock implementations of MCU hardware registers
==================================================================================================*/

#ifndef MOCK_REGISTERS_H
#define MOCK_REGISTERS_H

#include "Std_Types.h"

/*==================================================================================================
*                                      REGISTER BASE ADDRESSES
==================================================================================================*/
#define MCU_GPC_BASE_ADDR               (0x303A0000UL)
#define MCU_CCM_BASE_ADDR               (0x30380000UL)
#define MCU_SRC_BASE_ADDR               (0x30390000UL)
#define MCU_PLL_BASE_ADDR               (0x30360000UL)

/*==================================================================================================
*                                      GPC REGISTERS
==================================================================================================*/
#define MCU_GPC_PGC_CPU_MAPPING         (MCU_GPC_BASE_ADDR + 0x0EC)
#define MCU_GPC_PU_PGC_SW_PUP_REQ       (MCU_GPC_BASE_ADDR + 0x0F8)
#define MCU_GPC_PU_PGC_SW_PDN_REQ       (MCU_GPC_BASE_ADDR + 0x104)

/*==================================================================================================
*                                      CCM REGISTERS
==================================================================================================*/
#define MCU_CCM_CCR                     (MCU_CCM_BASE_ADDR + 0x0000)
#define MCU_CCM_CSR                     (MCU_CCM_BASE_ADDR + 0x0008)
#define MCU_CCM_CCSR                    (MCU_CCM_BASE_ADDR + 0x000C)
#define MCU_CCM_CACRR                   (MCU_CCM_BASE_ADDR + 0x0010)
#define MCU_CCM_CBCDR                   (MCU_CCM_BASE_ADDR + 0x0014)
#define MCU_CCM_CBCMR                   (MCU_CCM_BASE_ADDR + 0x0018)

/*==================================================================================================
*                                      SRC REGISTERS
==================================================================================================*/
#define MCU_SRC_SCR                     (MCU_SRC_BASE_ADDR + 0x0000)
#define MCU_SRC_SRSR                    (MCU_SRC_BASE_ADDR + 0x0004)
#define MCU_SRC_SBMR1                   (MCU_SRC_BASE_ADDR + 0x0008)
#define MCU_SRC_SBMR2                   (MCU_SRC_BASE_ADDR + 0x001C)

/*==================================================================================================
*                                      PLL REGISTERS
==================================================================================================*/
#define MCU_PLL_CTRL_OFFSET             (0x00)
#define MCU_PLL_CONFIG_OFFSET           (0x04)
#define MCU_PLL_POSTDIV_OFFSET          (0x08)

/*==================================================================================================
*                                      MOCK FUNCTIONS
==================================================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset all mock registers to default values
 */
void MockRegisters_Reset(void);

/**
 * @brief Read 32-bit value from mock register
 * @param address Register address
 * @return Register value
 */
uint32 MockRegisters_Read32(uint32 address);

/**
 * @brief Write 32-bit value to mock register
 * @param address Register address
 * @param value Value to write
 */
void MockRegisters_Write32(uint32 address, uint32 value);

/**
 * @brief Read 16-bit value from mock register
 * @param address Register address
 * @return Register value
 */
uint16 MockRegisters_Read16(uint32 address);

/**
 * @brief Write 16-bit value to mock register
 * @param address Register address
 * @param value Value to write
 */
void MockRegisters_Write16(uint32 address, uint16 value);

/**
 * @brief Read 8-bit value from mock register
 * @param address Register address
 * @return Register value
 */
uint8 MockRegisters_Read8(uint32 address);

/**
 * @brief Write 8-bit value to mock register
 * @param address Register address
 * @param value Value to write
 */
void MockRegisters_Write8(uint32 address, uint8 value);

/*==================================================================================================
*                                      MOCK REGISTER ACCESS
*                                      Used by source code
==================================================================================================*/

/**
 * @brief Mock register read macro
 */
#define REG_READ32(address)             MockRegisters_Read32(address)
#define REG_READ16(address)             MockRegisters_Read16(address)
#define REG_READ8(address)              MockRegisters_Read8(address)

/**
 * @brief Mock register write macro
 */
#define REG_WRITE32(address, value)     MockRegisters_Write32(address, value)
#define REG_WRITE16(address, value)     MockRegisters_Write16(address, value)
#define REG_WRITE8(address, value)      MockRegisters_Write8(address, value)

#ifdef __cplusplus
}
#endif

#endif /* MOCK_REGISTERS_H */
