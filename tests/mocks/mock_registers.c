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
* @file mock_registers.c
* @brief Hardware register mock implementation for unit testing
==================================================================================================*/

#include "mock_registers.h"
#include <string.h>

/*==================================================================================================
*                                      MAX REGISTERS
==================================================================================================*/
#define MOCK_MAX_REGISTERS              (256U)

/*==================================================================================================
*                                      REGISTER STORAGE
==================================================================================================*/
typedef struct {
    uint32 address;
    uint32 value;
    boolean used;
} MockRegisterEntryType;

static MockRegisterEntryType MockRegisters[MOCK_MAX_REGISTERS];
static uint32 MockRegisterCount = 0;

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Find register index by address
 */
static sint32 FindRegisterIndex(uint32 address)
{
    for (uint32 i = 0; i < MockRegisterCount; i++)
    {
        if (MockRegisters[i].address == address && MockRegisters[i].used)
        {
            return (sint32)i;
        }
    }
    return -1;  /* Not found */
}

/**
 * @brief Add new register entry
 */
static sint32 AddRegisterEntry(uint32 address)
{
    /* Check if already exists */
    sint32 index = FindRegisterIndex(address);
    if (index >= 0)
    {
        return index;
    }

    /* Add new entry if space available */
    if (MockRegisterCount < MOCK_MAX_REGISTERS)
    {
        MockRegisters[MockRegisterCount].address = address;
        MockRegisters[MockRegisterCount].value = 0;
        MockRegisters[MockRegisterCount].used = TRUE;
        return (sint32)MockRegisterCount++;
    }

    /* Table full - reuse an entry (simple approach) */
    return -1;
}

/*==================================================================================================
*                                      GPIO REGISTER SIMULATION
==================================================================================================*/
/* GPIO base addresses (must match Dio.c) */
#define MOCK_GPIO1_BASE                 (0x30200000UL)
#define MOCK_GPIO5_BASE                 (0x30240000UL)
/* GPIO register offsets */
#define MOCK_GPIO_DR_OFF                (0x00U)
#define MOCK_GPIO_PSOR_OFF              (0x04U)  /* atomic bit set */
#define MOCK_GPIO_PCOR_OFF              (0x08U)  /* atomic bit clear */
#define MOCK_GPIO_PTOR_OFF              (0x0CU)  /* atomic bit toggle */

static boolean IsGpioBase(uint32 addr)
{
    return (addr == MOCK_GPIO1_BASE) || (addr == MOCK_GPIO5_BASE);
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

void MockRegisters_Reset(void)
{
    memset(MockRegisters, 0, sizeof(MockRegisters));
    MockRegisterCount = 0;
}

uint32 MockRegisters_Read32(uint32 address)
{
    sint32 index = FindRegisterIndex(address);
    if (index >= 0)
    {
        return MockRegisters[index].value;
    }

    /* Return 0 for uninitialized registers */
    return 0U;
}

void MockRegisters_Write32(uint32 address, uint32 value)
{
    /* Simulate GPIO atomic-bit registers: PSOR/PCOR/PTOR modify DR */
    uint32 base = address & ~0x0FU;  /* align to 16-byte boundary */
    uint32 offset = address & 0x0FU;

    if (IsGpioBase(base) && (offset >= MOCK_GPIO_PSOR_OFF))
    {
        sint32 drIndex = FindRegisterIndex(base + MOCK_GPIO_DR_OFF);
        if (drIndex < 0)
        {
            drIndex = AddRegisterEntry(base + MOCK_GPIO_DR_OFF);
        }
        if (drIndex >= 0)
        {
            switch (offset)
            {
                case MOCK_GPIO_PSOR_OFF:
                    MockRegisters[drIndex].value |= value;
                    break;
                case MOCK_GPIO_PCOR_OFF:
                    MockRegisters[drIndex].value &= ~value;
                    break;
                case MOCK_GPIO_PTOR_OFF:
                    MockRegisters[drIndex].value ^= value;
                    break;
                default:
                    break;
            }
        }
        /* Also store the raw write for observability */
        sint32 rawIndex = AddRegisterEntry(address);
        if (rawIndex >= 0)
        {
            MockRegisters[rawIndex].value = value;
        }
        return;
    }

    sint32 index = AddRegisterEntry(address);
    if (index >= 0)
    {
        MockRegisters[index].value = value;
    }
}

uint16 MockRegisters_Read16(uint32 address)
{
    return (uint16)(MockRegisters_Read32(address) & 0xFFFFU);
}

void MockRegisters_Write16(uint32 address, uint16 value)
{
    uint32 currentValue = MockRegisters_Read32(address);
    MockRegisters_Write32(address, (currentValue & 0xFFFF0000U) | value);
}

uint8 MockRegisters_Read8(uint32 address)
{
    return (uint8)(MockRegisters_Read32(address) & 0xFFU);
}

void MockRegisters_Write8(uint32 address, uint8 value)
{
    uint32 currentValue = MockRegisters_Read32(address);
    MockRegisters_Write32(address, (currentValue & 0xFFFFFF00U) | value);
}

/* Weak stubs for interrupt control - used by SchM critical sections */
__attribute__((weak)) void Mcal_DisableAllInterrupts(void)
{
    /* No-op in test environment */
}

__attribute__((weak)) void Mcal_EnableAllInterrupts(void)
{
    /* No-op in test environment */
}
